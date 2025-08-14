/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
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

#if !defined(_XBOX) && (_MSC_VER == 1310) || defined(_MSC_VER) && (_WIN32_WINNT <= _WIN32_WINNT_WIN2K)
#ifndef _WIN32_DCOM
/* needed for CoInitializeEx */
#define _WIN32_DCOM
#endif
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <boolean.h>

#include <compat/msvc.h>
#include <retro_miscellaneous.h>
#include <string/stdstring.h>
#include <encodings/utf.h>
#include <lists/string_list.h>

#include "xaudio.h"

#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600 /*_WIN32_WINNT_VISTA */)
#ifndef HAVE_MMDEVICE
#define HAVE_MMDEVICE
#endif
#endif

#ifdef HAVE_MMDEVICE
#include "../common/mmdevice_common.h"
#include "../common/mmdevice_common_inline.h"
#endif

#include "../audio_driver.h"
#include "../../verbosity.h"

typedef struct xaudio2 xaudio2_t;

#define MAX_BUFFERS      16

#define MAX_BUFFERS_MASK (MAX_BUFFERS - 1)

#ifndef COINIT_MULTITHREADED
#define COINIT_MULTITHREADED 0x00
#endif

#define XAUDIO2_WRITE_AVAILABLE(handle) ((handle)->bufsize * (MAX_BUFFERS - (handle)->buffers - 1))

enum xa_flags
{
   XA2_FLAG_NONBLOCK  = (1 << 0),
   XA2_FLAG_IS_PAUSED = (1 << 1)
};

typedef struct
{
   xaudio2_t *xa;
   size_t bufsize;
   uint8_t flags;
} xa_t;

#if defined(__cplusplus) && !defined(CINTERFACE)
struct xaudio2 : public IXAudio2VoiceCallback
#else
struct xaudio2
#endif
{
#if defined(__cplusplus) && !defined(CINTERFACE)
   xaudio2() :
      buf(0), pXAudio2(0), pMasterVoice(0),
      pSourceVoice(0), hEvent(0), buffers(0), bufsize(0),
      bufptr(0), write_buffer(0)
   {}

   virtual ~xaudio2() {}

   STDMETHOD_(void, OnBufferStart) (void *) {}
   STDMETHOD_(void, OnBufferEnd) (void *)
   {
      InterlockedDecrement((LONG volatile*)&buffers);
      SetEvent(hEvent);
   }
   STDMETHOD_(void, OnLoopEnd) (void *) {}
   STDMETHOD_(void, OnStreamEnd) () {}
   STDMETHOD_(void, OnVoiceError) (void *, HRESULT) {}
   STDMETHOD_(void, OnVoiceProcessingPassEnd) () {}
   STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32) {}
#else
   const IXAudio2VoiceCallbackVtbl *lpVtbl;
#endif

   uint8_t *buf;
   IXAudio2 *pXAudio2;
   IXAudio2MasteringVoice *pMasterVoice;
   IXAudio2SourceVoice *pSourceVoice;
   WAVEFORMATEX wf;
   HANDLE hEvent;

   unsigned long volatile buffers;
   size_t bufsize;
   unsigned bufptr;
   unsigned write_buffer;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
static void WINAPI xa_voice_on_buffer_end(IXAudio2VoiceCallback *handle_, void *data)
{
   xaudio2_t *handle = (xaudio2_t*)handle_;
   (void)data;
   InterlockedDecrement((LONG volatile*)&handle->buffers);
   SetEvent(handle->hEvent);
}

static void WINAPI xa_dummy_voidp(IXAudio2VoiceCallback *handle, void *data) { (void)handle; (void)data; }
static void WINAPI xa_dummy_nil(IXAudio2VoiceCallback *handle) { (void)handle; }
static void WINAPI xa_dummy_uint32(IXAudio2VoiceCallback *handle, UINT32 dummy) { (void)handle; (void)dummy; }
static void WINAPI xa_dummy_voidp_hresult(IXAudio2VoiceCallback *handle, void *data, HRESULT dummy) { (void)handle; (void)data; (void)dummy; }

const struct IXAudio2VoiceCallbackVtbl xa_voice_vtable = {
   xa_dummy_uint32,
   xa_dummy_nil,
   xa_dummy_nil,
   xa_dummy_voidp,
   xa_voice_on_buffer_end,
   xa_dummy_voidp,
   xa_dummy_voidp_hresult,
};
#endif

static void xaudio2_set_format(WAVEFORMATEX *wf,
      bool float_fmt, unsigned channels, unsigned rate)
{
   WORD wBitsPerSample   = float_fmt ? 32 : 16;
   WORD nBlockAlign      = (channels * wBitsPerSample) / 8;
   DWORD nAvgBytesPerSec = rate * nBlockAlign;

   if (float_fmt)
      wf->wFormatTag     = WAVE_FORMAT_IEEE_FLOAT;
   else
      wf->wFormatTag     = WAVE_FORMAT_PCM;

   wf->nChannels         = channels;
   wf->nSamplesPerSec    = rate;
   wf->nAvgBytesPerSec   = nAvgBytesPerSec;
   wf->nBlockAlign       = nBlockAlign;
   wf->wBitsPerSample    = wBitsPerSample;

   wf->cbSize            = 0;
}

static void xaudio2_free(xaudio2_t *handle)
{
   if (!handle)
      return;

   if (handle->pSourceVoice)
   {
      IXAudio2SourceVoice_Stop(handle->pSourceVoice,
            0, XAUDIO2_COMMIT_NOW);
      IXAudio2SourceVoice_DestroyVoice(handle->pSourceVoice);
   }

   if (handle->pMasterVoice)
   {
      IXAudio2MasteringVoice_DestroyVoice(handle->pMasterVoice);
   }

   if (handle->pXAudio2)
   {
      IXAudio2_Release(handle->pXAudio2);
   }

   if (handle->hEvent)
      CloseHandle(handle->hEvent);

   free(handle->buf);

#if defined(__cplusplus) && !defined(CINTERFACE)
   delete handle;
#else
   free(handle);
#endif

#if !defined(_XBOX) && !defined(__WINRT__)
   CoUninitialize();
#endif
}

static const char *xaudio2_wave_format_name(WAVEFORMATEX *format)
{
   switch (format->wFormatTag)
   {
      case WAVE_FORMAT_PCM:
         return "WAVE_FORMAT_PCM";
      case WAVE_FORMAT_IEEE_FLOAT:
         return "WAVE_FORMAT_IEEE_FLOAT";
      default:
         break;
   }

   return "<unknown>";
}

static size_t xa_device_get_samplerate(int id)
{
#if defined(_XBOX) || !defined(HAVE_MMDEVICE)
   size_t _len    = 0;
   XAUDIO2_DEVICE_DETAILS dev_detail;
   IXAudio2 *ixa2 = NULL;
   if (SUCCEEDED(XAudio2Create(&ixa2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
   {
      if (IXAudio2_GetDeviceDetails(ixa2, id, &dev_detail) == S_OK)
         _len = dev_detail.OutputFormat.Format.nSamplesPerSec;
      IXAudio2_Release(ixa2);
   }
   return _len;
#elif defined(__WINRT__)
   /* TODO/FIXME - implement? */
   return 0;
#else
   return mmdevice_get_samplerate(id);
#endif
}

static void *xa_list_new(void *u)
{
#if defined(_XBOX) || !defined(HAVE_MMDEVICE)
   unsigned i;
   union string_list_elem_attr attr;
   uint32_t dev_count              = 0;
   IXAudio2 *ixa2                  = NULL;
   struct string_list *sl          = string_list_new();

   if (!sl)
      return NULL;

   attr.i = 0;

   if (FAILED(XAudio2Create(&ixa2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
      return NULL;

   IXAudio2_GetDeviceCount(ixa2, &dev_count);

   for (i = 0; i < dev_count; i++)
   {
      XAUDIO2_DEVICE_DETAILS dev_detail;
      if (IXAudio2_GetDeviceDetails(ixa2, i, &dev_detail) == S_OK)
      {
         char *str = utf16_to_utf8_string_alloc(dev_detail.DisplayName);

         if (str)
         {
            string_list_append(sl, str, attr);
            free(str);
         }
      }
   }

   IXAudio2_Release(ixa2);

   return sl;
#elif defined(__WINRT__)
   return NULL;
#else
   return mmdevice_list_new(u, 0 /* eRender */);
#endif
}


static xaudio2_t *xaudio2_new(unsigned *rate, unsigned channels,
      unsigned latency, size_t len, const char *dev_id)
{
   int32_t idx_found        = -1;
   WAVEFORMATEX desired_wf  = {0};
   struct string_list *list = NULL;
   xaudio2_t *handle        = NULL;

#if !defined(_XBOX) && !defined(__WINRT__)
   if (FAILED(CoInitialize(NULL)))
      return NULL;
#endif

#if defined(__cplusplus) && !defined(CINTERFACE)
   handle = new xaudio2;
#else
   handle = (xaudio2_t*)calloc(1, sizeof(*handle));
#endif

   if (!handle)
   {
#if !defined(_XBOX) && !defined(__WINRT__)
      CoUninitialize();
#endif
      return NULL;
   }

   list = (struct string_list*)xa_list_new(NULL);

#if !defined(__cplusplus) || defined(CINTERFACE)
   handle->lpVtbl = &xa_voice_vtable;
#endif

   if (FAILED(XAudio2Create(&handle->pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
      goto error;

   xaudio2_set_format(&desired_wf, true, channels, *rate);
   RARCH_DBG("[XAudio2] Requesting %u-bit %u-channel client with %s samples at %uHz %ums.\n",
         desired_wf.wBitsPerSample,
         desired_wf.nChannels,
         xaudio2_wave_format_name(&desired_wf),
         desired_wf.nSamplesPerSec,
         latency);
   *rate = desired_wf.nSamplesPerSec;

   if (dev_id)
   {
      /* Search for device name first */
      if (list && list->elems)
      {
         /* If any devices were found... */
         size_t i;
         for (i = 0; i < list->size; i++)
         {
            if (string_is_equal(dev_id, list->elems[i].data))
            {
               size_t new_rate = 0;
               RARCH_DBG("[XAudio2] Found device #%d: \"%s\".\n", i,
                     list->elems[i].data);
               idx_found = i;
               new_rate  = xa_device_get_samplerate(i);
               if (new_rate > 0)
               {
                  xaudio2_set_format(&desired_wf, true, channels, new_rate);
                  *rate = desired_wf.nSamplesPerSec;
               }
               break;
            }
         }

         /* Index was not found yet based on name string,
          * just assume id is a one-character number index. */
         if (idx_found == -1 && isdigit(dev_id[0]))
         {
            idx_found = strtoul(dev_id, NULL, 0);
            RARCH_LOG("[XAudio2] Fallback, device index is a single number index instead: %d.\n",
                  idx_found);
         }
      }
   }

   if (idx_found == -1)
      idx_found = 0;

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
   {
      wchar_t *temp = NULL;
      if (dev_id)
         temp = utf8_to_utf16_string_alloc((const char*)list->elems[idx_found].userdata);

      if (FAILED(IXAudio2_CreateMasteringVoice(handle->pXAudio2,
                  &handle->pMasterVoice, channels, *rate, 0,
                  (LPCWSTR)(uintptr_t)temp, NULL, AudioCategory_GameEffects)))
      {
         free(temp);
         goto error;
      }
      if (temp)
         free(temp);
   }
#else
   if (FAILED(IXAudio2_CreateMasteringVoice(handle->pXAudio2,
               &handle->pMasterVoice, channels, *rate, 0, idx_found, NULL)))
      goto error;
#endif

   if (FAILED(IXAudio2_CreateSourceVoice(handle->pXAudio2,
               &handle->pSourceVoice, &desired_wf,
               XAUDIO2_VOICE_NOSRC, XAUDIO2_DEFAULT_FREQ_RATIO,
               (IXAudio2VoiceCallback*)handle, 0, 0)))
      goto error;

   handle->hEvent  = CreateEvent(0, FALSE, FALSE, 0);
   if (!handle->hEvent)
      goto error;

   handle->wf      = desired_wf;
   handle->bufsize = len / MAX_BUFFERS;
   handle->buf     = (uint8_t*)calloc(1, handle->bufsize * MAX_BUFFERS);
   if (!handle->buf)
      goto error;

   if (FAILED(IXAudio2SourceVoice_Start(handle->pSourceVoice, 0,
               XAUDIO2_COMMIT_NOW)))
      goto error;

   if (list)
      string_list_free(list);
   return handle;

error:
   if (list)
      string_list_free(list);
   xaudio2_free(handle);
   return NULL;
}

static void *xa_init(const char *dev_id, unsigned rate, unsigned latency,
      unsigned block_frames, unsigned *new_rate)
{
   size_t bufsize;
   xa_t *xa    = (xa_t*)calloc(1, sizeof(*xa));

   if (!xa)
      return NULL;

   if (latency < 8)
      latency  = 8; /* Do not allow shenanigans. */

   bufsize     = latency * rate / 1000;
   xa->bufsize = bufsize * 2 * sizeof(float);

   if (!(xa->xa = xaudio2_new(&rate, 2, latency, xa->bufsize, dev_id)))
   {
      RARCH_ERR("[XAudio2] Failed to init driver.\n");
      free(xa);
      return NULL;
   }

   *new_rate = rate;

   RARCH_LOG("[XAudio2] Requesting %u ms latency, using %d ms latency.\n",
         latency, (int)bufsize * 1000 / rate);

   return xa;
}

static ssize_t xa_write(void *data, const void *s, size_t len)
{
   size_t _len           = 0;
   unsigned bytes        = len;
   xa_t *xa              = (xa_t*)data;
   xaudio2_t *handle     = xa->xa;
   const uint8_t *buffer = (const uint8_t*)s;

   if (xa->flags & XA2_FLAG_NONBLOCK)
   {
      size_t avail = XAUDIO2_WRITE_AVAILABLE(xa->xa);

      if (avail == 0)
         return 0;
      if (avail < len)
         bytes = len = avail;
   }

   while (bytes)
   {
      unsigned need   = MIN(bytes, handle->bufsize - handle->bufptr);

      if (need > 0)
      {
         memcpy(handle->buf + handle->write_buffer *
               handle->bufsize + handle->bufptr,
               buffer, need);
         handle->bufptr += need;
         buffer         += need;
         _len           += need;
         bytes          -= need;
      }

      if (handle->bufptr == handle->bufsize)
      {
         XAUDIO2_BUFFER xa2buffer;

         while (handle->buffers == MAX_BUFFERS - 1)
            if (!(WaitForSingleObject(handle->hEvent, 50) == WAIT_OBJECT_0))
               return -1;

         xa2buffer.Flags      = 0;
         xa2buffer.AudioBytes = handle->bufsize;
         xa2buffer.pAudioData = handle->buf + handle->write_buffer * handle->bufsize;
         xa2buffer.PlayBegin  = 0;
         xa2buffer.PlayLength = 0;
         xa2buffer.LoopBegin  = 0;
         xa2buffer.LoopLength = 0;
         xa2buffer.LoopCount  = 0;
         xa2buffer.pContext   = NULL;

         if (FAILED(IXAudio2SourceVoice_SubmitSourceBuffer(
                     handle->pSourceVoice, &xa2buffer, NULL)))
         {
            if (len > 0)
               return -1;
            return 0;
         }

         InterlockedIncrement((LONG volatile*)&handle->buffers);
         handle->bufptr       = 0;
         handle->write_buffer = (handle->write_buffer + 1) & MAX_BUFFERS_MASK;
      }
   }

   return _len;
}

static bool xa_stop(void *data)
{
   xa_t *xa   = (xa_t*)data;
   xa->flags |= XA2_FLAG_IS_PAUSED;
   return true;
}

static bool xa_alive(void *data)
{
   xa_t *xa = (xa_t*)data;
   if (!xa)
      return false;
   return !(xa->flags & XA2_FLAG_IS_PAUSED);
}

static void xa_set_nonblock_state(void *data, bool state)
{
   xa_t *xa = (xa_t*)data;
   if (xa)
   {
      if (state)
         xa->flags |=  XA2_FLAG_NONBLOCK;
      else
         xa->flags &= ~XA2_FLAG_NONBLOCK;
   }
}

static bool xa_start(void *data, bool is_shutdown)
{
   xa_t *xa   = (xa_t*)data;
   if (!xa)
      return false;
   xa->flags &= ~(XA2_FLAG_IS_PAUSED);
   return true;
}

static bool xa_use_float(void *data)
{
   xa_t *xa              = (xa_t*)data;
   xaudio2_t *handle     = xa->xa;
   return (handle && handle->wf.wBitsPerSample == 32);
}

static void xa_free(void *data)
{
   xa_t *xa = (xa_t*)data;

   if (!xa)
      return;

   if (xa->xa)
      xaudio2_free(xa->xa);
   free(xa);
}

static size_t xa_write_avail(void *data)
{
   xa_t *xa = (xa_t*)data;
   return XAUDIO2_WRITE_AVAILABLE(xa->xa);
}

static size_t xa_buffer_size(void *data)
{
   xa_t *xa = (xa_t*)data;
   if (!xa)
      return 0;
   return xa->bufsize;
}

static void xa_device_list_free(void *u, void *slp)
{
   struct string_list *sl = (struct string_list*)slp;

   if (sl)
      string_list_free(sl);
}

audio_driver_t audio_xa = {
   xa_init,
   xa_write,
   xa_stop,
   xa_start,
   xa_alive,
   xa_set_nonblock_state,
   xa_free,
   xa_use_float,
   "xaudio",
   xa_list_new,
   xa_device_list_free,
   xa_write_avail,
   xa_buffer_size,
};
