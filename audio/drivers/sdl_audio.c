/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
 *  Copyright (C) 2023-2025 - Jesse Talavera-Greenberg
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <boolean.h>
#include <rthreads/rthreads.h>
#include <queues/fifo_queue.h>
#include <retro_inline.h>
#include <retro_math.h>
#include <lists/string_list.h>

#include "SDL.h"
#include "SDL_audio.h"

#include "../audio_driver.h"
#include "../../verbosity.h"

static INLINE int sdl_audio_find_num_frames(int rate, int latency)
{
   int frames = (rate * latency) / 1000;
   /* SDL only likes 2^n sized buffers. */
   return next_pow2(frames);
}

#ifdef HAVE_SDL2
#ifdef HAVE_MICROPHONE
#include "../microphone_driver.h"

typedef struct sdl_microphone_handle
{
#ifdef HAVE_THREADS
   slock_t *lock;
   scond_t *cond;
#endif

   /**
    * The queue used to store incoming samples from the driver.
    */
   fifo_buffer_t *sample_buffer;
   SDL_AudioDeviceID device_id;
   SDL_AudioSpec device_spec;
} sdl_microphone_handle_t;

typedef struct sdl_microphone
{
   bool nonblock;
} sdl_microphone_t;

static void *sdl_microphone_init(void)
{
   sdl_microphone_t *sdl        = NULL;
   uint32_t sdl_subsystem_flags = SDL_WasInit(0);
   /* Initialise audio subsystem, if required */
   if (sdl_subsystem_flags == 0)
   {
      if (SDL_Init(SDL_INIT_AUDIO) < 0)
         return NULL;
   }
   else if ((sdl_subsystem_flags & SDL_INIT_AUDIO) == 0)
   {
      if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
         return NULL;
   }
   if (!(sdl = (sdl_microphone_t*)calloc(1, sizeof(*sdl))))
      return NULL;
   return sdl;
}

static void sdl_microphone_close_mic(void *driver_context, void *mic_context)
{
   sdl_microphone_handle_t *mic = (sdl_microphone_handle_t *)mic_context;

   if (mic)
   {
      /* If the microphone was originally initialized successfully... */
      if (mic->device_id > 0)
         SDL_CloseAudioDevice(mic->device_id);

      fifo_free(mic->sample_buffer);

#ifdef HAVE_THREADS
      slock_free(mic->lock);
      scond_free(mic->cond);
#endif

      RARCH_LOG("[SDL audio] Freed microphone with former device ID %u.\n", mic->device_id);
      free(mic);
   }
}

static void sdl_microphone_free(void *data)
{
   sdl_microphone_t *sdl = (sdl_microphone_t*)data;

   if (sdl)
      SDL_QuitSubSystem(SDL_INIT_AUDIO);
   free(sdl);
   /* NOTE: The microphone frontend should've closed the mics by now */
}

static void sdl_audio_record_cb(void *data, Uint8 *stream, int len)
{
   sdl_microphone_handle_t *mic = (sdl_microphone_handle_t*)data;
   size_t                 avail = FIFO_WRITE_AVAIL(mic->sample_buffer);
   size_t             read_size = MIN(len, (int)avail);
   /* If the sample buffer is almost full, just write as much as we can into it*/
   fifo_write(mic->sample_buffer, stream, read_size);
#ifdef HAVE_THREADS
   scond_signal(mic->cond);
#endif
}

static void *sdl_microphone_open_mic(void *driver_context, const char *device,
      unsigned rate, unsigned latency, unsigned *new_rate)
{
   int frames;
   size_t bufsize;
   void *tmp                    = NULL;
   sdl_microphone_handle_t *mic = NULL;
   SDL_AudioSpec desired_spec   = {0};

#if __APPLE__
   if (!string_is_equal(audio_driver_get_ident(), "sdl2"))
   {
      const char *msg = msg_hash_to_str(MSG_SDL2_MIC_NEEDS_SDL2_AUDIO);
      runloop_msg_queue_push(msg, strlen(msg),
            1, 100, true, NULL,
            MESSAGE_QUEUE_ICON_DEFAULT, MESSAGE_QUEUE_CATEGORY_WARNING);
      return NULL;
   }
#endif

   /* If the audio driver wasn't initialized yet... */
   if (!SDL_WasInit(SDL_INIT_AUDIO))
   {
      RARCH_ERR("[SDL mic] Attempted to initialize input device before initializing the audio subsystem.\n");
      return NULL;
   }

   if (!(mic = (sdl_microphone_handle_t *)
            calloc(1, sizeof(sdl_microphone_handle_t))))
      return NULL;

   /* Only print SDL audio devices if verbose logging is enabled */
   if (verbosity_is_enabled())
   {
      int i;
      int num_available_microphones = SDL_GetNumAudioDevices(true);
      RARCH_DBG("[SDL mic] %d audio capture devices found:\n", num_available_microphones);
      for (i = 0; i < num_available_microphones; ++i)
         RARCH_DBG("[SDL mic]    - %s\n", SDL_GetAudioDeviceName(i, true));
   }

   /* We have to buffer up some data ourselves, so we let SDL
    * carry approximately half of the latency.
    *
    * SDL double buffers audio and we do as well. */
   frames                = sdl_audio_find_num_frames(rate, latency / 4);

   desired_spec.freq     = rate;
#ifdef HAVE_SDL2
   desired_spec.format   = AUDIO_F32SYS;
#else
   desired_spec.format   = AUDIO_S16SYS;
#endif
   desired_spec.channels = 1; /* Microphones only usually provide input in mono */
   desired_spec.samples  = frames;
   desired_spec.userdata = mic;
   desired_spec.callback = sdl_audio_record_cb;

   mic->device_id = SDL_OpenAudioDevice(
         NULL,
         true,
         &desired_spec,
         &mic->device_spec,
           SDL_AUDIO_ALLOW_FREQUENCY_CHANGE
         | SDL_AUDIO_ALLOW_FORMAT_CHANGE);

   if (mic->device_id == 0)
   {
      RARCH_ERR("[SDL mic] Failed to open SDL audio input device: %s.\n", SDL_GetError());
      goto error;
   }
   RARCH_DBG("[SDL mic] Opened SDL audio input device with ID %u.\n",
             mic->device_id);
   RARCH_DBG("[SDL mic] Requested a microphone frequency of %u Hz, received %u Hz.\n",
             desired_spec.freq, mic->device_spec.freq);
   RARCH_DBG("[SDL mic] Requested %u channels for microphone, received %u.\n",
             desired_spec.channels, mic->device_spec.channels);
   RARCH_DBG("[SDL mic] Requested a %u-sample microphone buffer, received %u samples (%u bytes).\n",
             frames, mic->device_spec.samples, mic->device_spec.size);
   RARCH_DBG("[SDL mic] Received a microphone silence value of %u.\n", mic->device_spec.silence);
   RARCH_DBG("[SDL mic] Requested microphone audio format: %u-bit %s %s %s endian.\n",
             SDL_AUDIO_BITSIZE(desired_spec.format),
             SDL_AUDIO_ISSIGNED(desired_spec.format) ? "signed" : "unsigned",
             SDL_AUDIO_ISFLOAT(desired_spec.format) ? "floating-point" : "integer",
             SDL_AUDIO_ISBIGENDIAN(desired_spec.format) ? "big" : "little");

   RARCH_DBG("[SDL mic] Received microphone audio format: %u-bit %s %s %s endian.\n",
             SDL_AUDIO_BITSIZE(mic->device_spec.format),
             SDL_AUDIO_ISSIGNED(mic->device_spec.format) ? "signed" : "unsigned",
             SDL_AUDIO_ISFLOAT(mic->device_spec.format) ? "floating-point" : "integer",
             SDL_AUDIO_ISBIGENDIAN(mic->device_spec.format) ? "big" : "little");

   if (new_rate)
      *new_rate = mic->device_spec.freq;

#ifdef HAVE_THREADS
   mic->lock = slock_new();
   mic->cond = scond_new();
#endif

   RARCH_LOG("[SDL audio] Requested %u ms latency for input device, received %d ms.\n",
             latency, (int)(mic->device_spec.samples * 4 * 1000 / mic->device_spec.freq));

   /* Create a buffer twice as big as needed and prefill the buffer. */
   bufsize            = mic->device_spec.samples * 2 * (SDL_AUDIO_BITSIZE(mic->device_spec.format) / 8);
   tmp                = calloc(1, bufsize);
   mic->sample_buffer = fifo_new(bufsize);

   RARCH_DBG("[SDL audio] Initialized microphone sample queue with %u bytes.\n", bufsize);

   if (tmp)
   {
      fifo_write(mic->sample_buffer, tmp, bufsize);
      free(tmp);
   }

   RARCH_LOG("[SDL audio] Initialized microphone with device ID %u.\n", mic->device_id);
   return mic;

error:
   free(mic);
   return NULL;
}

static bool sdl_microphone_mic_alive(const void *data, const void *mic_context)
{
   const sdl_microphone_handle_t *mic = (const sdl_microphone_handle_t*)mic_context;
   if (!mic)
      return false;
   /* Both params must be non-null */
   return SDL_GetAudioDeviceStatus(mic->device_id) == SDL_AUDIO_PLAYING;
}

static bool sdl_microphone_start_mic(void *driver_context, void *mic_context)
{
   sdl_microphone_handle_t *mic = (sdl_microphone_handle_t*)mic_context;
   if (!mic)
      return false;
   SDL_PauseAudioDevice(mic->device_id, false);
   if (SDL_GetAudioDeviceStatus(mic->device_id) != SDL_AUDIO_PLAYING)
   {
      RARCH_ERR("[SDL mic] Failed to start microphone %u: %s.\n", mic->device_id, SDL_GetError());
      return false;
   }
   RARCH_DBG("[SDL mic] Started microphone %u.\n", mic->device_id);
   return true;
}

static bool sdl_microphone_stop_mic(void *driver_context, void *mic_context)
{
   sdl_microphone_t        *sdl = (sdl_microphone_t*)driver_context;
   sdl_microphone_handle_t *mic = (sdl_microphone_handle_t*)mic_context;

   if (!sdl || !mic)
      return false;

   SDL_PauseAudioDevice(mic->device_id, true);

   switch (SDL_GetAudioDeviceStatus(mic->device_id))
   {
      case SDL_AUDIO_PLAYING:
         RARCH_ERR("[SDL mic] Microphone %u failed to pause.\n", mic->device_id);
         return false;
      case SDL_AUDIO_STOPPED:
         RARCH_WARN("[SDL mic] Microphone %u is in state STOPPED; it may not start again.\n",
               mic->device_id);
         /* fall-through */
      case SDL_AUDIO_PAUSED:
         break;
      default:
         RARCH_ERR("[SDL mic] Microphone %u is in unknown state.\n",
               mic->device_id);
         return false;
   }

   return true;
}

static void sdl_microphone_set_nonblock_state(void *driver_context, bool state)
{
   sdl_microphone_t *sdl = (sdl_microphone_t*)driver_context;
   if (sdl)
      sdl->nonblock = state;
}

static int sdl_microphone_read(void *driver_context, void *mic_context, void *s, size_t len)
{
   int ret = 0;
   sdl_microphone_t        *sdl = (sdl_microphone_t*)driver_context;
   sdl_microphone_handle_t *mic = (sdl_microphone_handle_t*)mic_context;

   if (!sdl || !mic || !s)
      return -1;

   /* If we shouldn't block on an empty queue... */
   if (sdl->nonblock)
   {
      size_t avail, read_amt;
      SDL_LockAudioDevice(mic->device_id); /* Stop the SDL mic thread */
      avail    = FIFO_READ_AVAIL(mic->sample_buffer);
      read_amt = avail > len ? len : avail;
      /* If the incoming queue isn't empty, then
       * read as much data as will fit in buf
       * */
      if (read_amt > 0)
         fifo_read(mic->sample_buffer, s, read_amt);
      SDL_UnlockAudioDevice(mic->device_id); /* Let the mic thread run again */
      ret = (int)read_amt;
   }
   else
   {
      size_t read = 0;

      /* Until we've given the caller as much data as they've asked for... */
      while (read < len)
      {
         size_t avail;

         SDL_LockAudioDevice(mic->device_id);
         /* Stop the SDL microphone thread from running */
         avail = FIFO_READ_AVAIL(mic->sample_buffer);

         if (avail == 0)
         { /* If the incoming sample queue is empty... */
            SDL_UnlockAudioDevice(mic->device_id);
            /* Let the SDL microphone thread run so it can
             * push some incoming samples */
#ifdef HAVE_THREADS
            slock_lock(mic->lock);
            /* Let *only* the SDL microphone thread access
             * the incoming sample queue. */
            scond_wait(mic->cond, mic->lock);
            /* Wait until the SDL microphone thread tells us
             * it's added some samples. */
            slock_unlock(mic->lock);
            /* Allow this thread to access the incoming sample queue,
             * which we'll do next iteration */
#endif
         }
         else
         {
            size_t read_amt = MIN(len - read, avail);
            fifo_read(mic->sample_buffer, s + read, read_amt);
            /* Read as many samples as we have available without
             * underflowing the queue */
            SDL_UnlockAudioDevice(mic->device_id);
            /* Let the SDL microphone thread run again */
            read += read_amt;
         }
      }
      ret = (int)read;
   }

   return ret;
}

static bool sdl_microphone_mic_use_float(const void *driver_context, const void *mic_context)
{
   sdl_microphone_handle_t *mic = (sdl_microphone_handle_t*)mic_context;
   return SDL_AUDIO_ISFLOAT(mic->device_spec.format);
}

microphone_driver_t microphone_sdl = {
      sdl_microphone_init,
      sdl_microphone_free,
      sdl_microphone_read,
      sdl_microphone_set_nonblock_state,
      "sdl2",
      NULL,
      NULL,
      sdl_microphone_open_mic,
      sdl_microphone_close_mic,
      sdl_microphone_mic_alive,
      sdl_microphone_start_mic,
      sdl_microphone_stop_mic,
      sdl_microphone_mic_use_float,
};
#endif
#else
typedef Uint32 SDL_AudioDeviceID;

/** Compatibility stub that defers to SDL_PauseAudio. */
#define SDL_PauseAudioDevice(dev, pause_on) SDL_PauseAudio(pause_on)

/** Compatibility stub that defers to SDL_LockAudio. */
#define SDL_LockAudioDevice(dev) SDL_LockAudio()

/** Compatibility stub that defers to SDL_UnlockAudio. */
#define SDL_UnlockAudioDevice(dev) SDL_UnlockAudio()

/** Compatibility stub that defers to SDL_CloseAudio. */
#define SDL_CloseAudioDevice(dev) SDL_CloseAudio()

/* Macros for checking audio format bits that were introduced in SDL 2 */
#define SDL_AUDIO_MASK_BITSIZE       (0xFF)
#define SDL_AUDIO_MASK_DATATYPE      (1<<8)
#define SDL_AUDIO_MASK_ENDIAN        (1<<12)
#define SDL_AUDIO_MASK_SIGNED        (1<<15)
#define SDL_AUDIO_BITSIZE(x)         (x & SDL_AUDIO_MASK_BITSIZE)
#define SDL_AUDIO_ISFLOAT(x)         (x & SDL_AUDIO_MASK_DATATYPE)
#define SDL_AUDIO_ISBIGENDIAN(x)     (x & SDL_AUDIO_MASK_ENDIAN)
#define SDL_AUDIO_ISSIGNED(x)        (x & SDL_AUDIO_MASK_SIGNED)
#define SDL_AUDIO_ISINT(x)           (!SDL_AUDIO_ISFLOAT(x))
#define SDL_AUDIO_ISLITTLEENDIAN(x)  (!SDL_AUDIO_ISBIGENDIAN(x))
#define SDL_AUDIO_ISUNSIGNED(x)      (!SDL_AUDIO_ISSIGNED(x))
#endif

typedef struct sdl_audio
{
#ifdef HAVE_THREADS
   slock_t *lock;
   scond_t *cond;
#endif
   /**
    * The queue used to store outgoing samples to be played by the driver.
    * Audio from the core ultimately makes its way here,
    * the last stop before the driver plays it.
    */
   fifo_buffer_t *speaker_buffer;
   bool nonblock;
   bool is_paused;
   SDL_AudioSpec device_spec;
   SDL_AudioDeviceID speaker_device;
} sdl_audio_t;

static void sdl_audio_playback_cb(void *data, Uint8 *stream, int len)
{
   sdl_audio_t  *sdl = (sdl_audio_t*)data;
   size_t      avail = FIFO_READ_AVAIL(sdl->speaker_buffer);
   size_t       _len = (len > (int)avail) ? avail : (size_t)len;
   fifo_read(sdl->speaker_buffer, stream, _len);
#ifdef HAVE_THREADS
   scond_signal(sdl->cond);
#endif
   /* If underrun, fill rest with silence. */
   memset(stream + _len, 0, len - _len);
}

static void *sdl_audio_list_new(void *u)
{
#ifdef HAVE_SDL2
   int i, num = 0;
   union string_list_elem_attr attr;
   struct string_list *sl = string_list_new();

   if (!sl)
      return NULL;

   attr.i = 0;
   num    = SDL_GetNumAudioDevices(false);

   for (i = 0; i < num; i++)
      string_list_append(sl, SDL_GetAudioDeviceName(i, false), attr);

   return sl;
#else
   /* TODO/FIXME - Any possible SDL1 implementation here, or
    * do we have to piggyback off OS-specific audio device
    * enumeration here? */
   return NULL;
#endif
}

static void *sdl_audio_init(const char *device,
      unsigned rate, unsigned latency,
      unsigned block_frames, unsigned *new_rate)
{
   int frames;
   size_t bufsize;
   SDL_AudioSpec spec           = {0};
   void *tmp                    = NULL;
   sdl_audio_t *sdl             = NULL;
   uint32_t sdl_subsystem_flags = SDL_WasInit(0);

   /* Initialise audio subsystem, if required */
   if (sdl_subsystem_flags == 0)
   {
      if (SDL_Init(SDL_INIT_AUDIO) < 0)
         return NULL;
   }
   else if ((sdl_subsystem_flags & SDL_INIT_AUDIO) == 0)
   {
      if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
         return NULL;
   }

   sdl = (sdl_audio_t*)calloc(1, sizeof(*sdl));
   if (!sdl)
      return NULL;

   /* We have to buffer up some data ourselves, so we let SDL
    * carry approximately half of the latency.
    *
    * SDL double buffers audio and we do as well. */
   frames        = sdl_audio_find_num_frames(rate, latency / 4);

   /* First, let's initialize the output device. */
   spec.freq     = rate;
#ifdef HAVE_SDL2
   spec.format   = AUDIO_F32SYS;
#else
   spec.format   = AUDIO_S16SYS;
#endif
   spec.channels = 2;
   spec.samples  = frames; /* This is in audio frames, not samples ... :( */
   spec.callback = sdl_audio_playback_cb;
   spec.userdata = sdl;

   /* No compatibility stub for SDL_OpenAudioDevice because its return value
    * is different from that of SDL_OpenAudio. */
#ifdef HAVE_SDL2
   sdl->speaker_device = SDL_OpenAudioDevice(NULL, false, &spec, &sdl->device_spec, 0);

   if (sdl->speaker_device == 0)
#else
   sdl->speaker_device = SDL_OpenAudio(&spec, &sdl->device_spec);

   if (sdl->speaker_device < 0)
#endif
   {
      RARCH_ERR("[SDL audio] Failed to open SDL audio output device: %s.\n", SDL_GetError());
      free(sdl);
      return NULL;
   }

   *new_rate                = sdl->device_spec.freq;
   RARCH_DBG("[SDL audio] Opened SDL audio out device with ID %u.\n",
             sdl->speaker_device);
   RARCH_DBG("[SDL audio] Requested a speaker frequency of %u Hz, received %u Hz.\n",
             spec.freq, sdl->device_spec.freq);
   RARCH_DBG("[SDL audio] Requested %u channels for speaker, received %u.\n",
             spec.channels, sdl->device_spec.channels);
   RARCH_DBG("[SDL audio] Requested a %u-frame speaker buffer, received %u frames (%u bytes).\n",
             frames, sdl->device_spec.samples, sdl->device_spec.size);
   RARCH_DBG("[SDL audio] Got a speaker silence value of %u.\n", sdl->device_spec.silence);
   RARCH_DBG("[SDL audio] Requested speaker audio format: %u-bit %s %s %s endian.\n",
             SDL_AUDIO_BITSIZE(spec.format),
             SDL_AUDIO_ISSIGNED(spec.format) ? "signed" : "unsigned",
             SDL_AUDIO_ISFLOAT(spec.format) ? "floating-point" : "integer",
             SDL_AUDIO_ISBIGENDIAN(spec.format) ? "big" : "little");
   RARCH_DBG("[SDL audio] Received speaker audio format: %u-bit %s %s %s endian.\n",
             SDL_AUDIO_BITSIZE(sdl->device_spec.format),
             SDL_AUDIO_ISSIGNED(sdl->device_spec.format) ? "signed" : "unsigned",
             SDL_AUDIO_ISFLOAT(sdl->device_spec.format) ? "floating-point" : "integer",
             SDL_AUDIO_ISBIGENDIAN(sdl->device_spec.format) ? "big" : "little");

#ifdef HAVE_THREADS
   sdl->lock                = slock_new();
   sdl->cond                = scond_new();
#endif

   RARCH_LOG("[SDL audio] Requested %u ms latency for output device, received %d ms.\n",
         latency, (int)(sdl->device_spec.samples * 4 * 1000 / (*new_rate)));

   /* Create a buffer twice as big as needed and prefill the buffer. */
   bufsize             = sdl->device_spec.samples * 4 * (SDL_AUDIO_BITSIZE(sdl->device_spec.format) / 8);
   tmp                 = calloc(1, bufsize);
   sdl->speaker_buffer = fifo_new(bufsize);

   if (tmp)
   {
      fifo_write(sdl->speaker_buffer, tmp, bufsize);
      free(tmp);
   }

   RARCH_DBG("[SDL audio] Initialized speaker sample queue with %u bytes.\n", bufsize);

   SDL_PauseAudioDevice(sdl->speaker_device, false);

   return sdl;
}

static ssize_t sdl_audio_write(void *data, const void *s, size_t len)
{
   size_t _len      = 0;
   sdl_audio_t *sdl = (sdl_audio_t*)data;

   /* If we shouldn't wait for space in a full outgoing sample queue... */
   if (sdl->nonblock)
   {
      size_t avail, write_amt;
      SDL_LockAudioDevice(sdl->speaker_device); /* Stop the SDL speaker thread from running */
      avail     = FIFO_WRITE_AVAIL(sdl->speaker_buffer);
      write_amt = (avail > len) ? len : avail; /* Enqueue as much data as we can */
      fifo_write(sdl->speaker_buffer, s, write_amt);
      SDL_UnlockAudioDevice(sdl->speaker_device); /* Let the speaker thread run again */
      _len      = write_amt; /* If the queue was full...well, too bad. */
   }
   else
   {
      /* Until we've written all the sample data we have available... */
      while (_len < len)
      {
         size_t avail;

         /* Stop the SDL speaker thread from running */
         SDL_LockAudioDevice(sdl->speaker_device);
         avail = FIFO_WRITE_AVAIL(sdl->speaker_buffer);

         /* If the outgoing sample queue is full... */
         if (avail == 0)
         {
            SDL_UnlockAudioDevice(sdl->speaker_device);
            /* Let the SDL speaker thread run so it can play the enqueued samples,
             * which will free up space for us to write new ones. */
#ifdef HAVE_THREADS
            slock_lock(sdl->lock);
            /* Let *only* the SDL speaker thread touch the outgoing sample queue */
            scond_wait(sdl->cond, sdl->lock);
            /* Block until SDL tells us that it's made room for new samples */
            slock_unlock(sdl->lock);
            /* Now let this thread use the outgoing sample queue (which we'll do next iteration) */
#endif
         }
         else
         {
            size_t write_amt = len - _len > avail ? avail : len - _len;
            fifo_write(sdl->speaker_buffer, (const char*)s + _len, write_amt);
            /* Enqueue as many samples as we have available without overflowing the queue */
            SDL_UnlockAudioDevice(sdl->speaker_device); /* Let the SDL speaker thread run again */
            _len += write_amt;
         }
      }
   }

   return _len;
}

static bool sdl_audio_stop(void *data)
{
   sdl_audio_t *sdl = (sdl_audio_t*)data;
   sdl->is_paused   = true;
   SDL_PauseAudioDevice(sdl->speaker_device, true);
   return true;
}

static bool sdl_audio_alive(void *data)
{
   sdl_audio_t *sdl = (sdl_audio_t*)data;
   if (!sdl)
      return false;
   return !sdl->is_paused;
}

static bool sdl_audio_start(void *data, bool is_shutdown)
{
   sdl_audio_t *sdl = (sdl_audio_t*)data;
   sdl->is_paused   = false;
   SDL_PauseAudioDevice(sdl->speaker_device, false);
   return true;
}

static void sdl_audio_set_nonblock_state(void *data, bool state)
{
   sdl_audio_t *sdl = (sdl_audio_t*)data;
   if (sdl)
      sdl->nonblock = state;
}

static void sdl_audio_free(void *data)
{
   sdl_audio_t *sdl = (sdl_audio_t*)data;

   if (sdl)
   {
      if (sdl->speaker_device > 0)
      {
         SDL_CloseAudioDevice(sdl->speaker_device);
      }

      if (sdl->speaker_buffer)
         fifo_free(sdl->speaker_buffer);

#ifdef HAVE_THREADS
      slock_free(sdl->lock);
      scond_free(sdl->cond);
#endif

      SDL_QuitSubSystem(SDL_INIT_AUDIO);
   }
   free(sdl);
}

static bool sdl_audio_use_float(void *data)
{
   sdl_audio_t *sdl = (sdl_audio_t*)data;
   return SDL_AUDIO_ISFLOAT(sdl->device_spec.format) ? true : false;
}

/* TODO/FIXME - implement */
static size_t sdl_audio_write_avail(void *data) { return 0; }

static void sdl_audio_list_free(void *u, void *slp)
{
   struct string_list *sl = (struct string_list*)slp;

   if (sl)
      string_list_free(sl);
}

audio_driver_t audio_sdl = {
   sdl_audio_init,
   sdl_audio_write,
   sdl_audio_stop,
   sdl_audio_start,
   sdl_audio_alive,
   sdl_audio_set_nonblock_state,
   sdl_audio_free,
   sdl_audio_use_float,
#ifdef HAVE_SDL2
   "sdl2",
#else
   "sdl",
#endif
   sdl_audio_list_new,
   sdl_audio_list_free,
   sdl_audio_write_avail,
   NULL
};
