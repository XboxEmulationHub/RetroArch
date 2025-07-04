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

#ifndef __MENU_DEFINES__H
#define __MENU_DEFINES__H

#include <retro_common_api.h>

#include "../audio/audio_defines.h"

RETRO_BEGIN_DECLS

#define MENU_SETTINGS_AUDIO_MIXER_MAX_STREAMS        (AUDIO_MIXER_MAX_SYSTEM_STREAMS-1)

enum menu_state_flags
{
   MENU_ST_FLAG_ALIVE                       = (1 << 0),
   MENU_ST_FLAG_IS_BINDING                  = (1 << 1),
   MENU_ST_FLAG_INP_DLG_KB_DISPLAY          = (1 << 2),
   /* When enabled, on next iteration the 'Quick Menu'
    * list will be pushed onto the stack */
   MENU_ST_FLAG_PENDING_QUICK_MENU          = (1 << 3),
   MENU_ST_FLAG_PREVENT_POPULATE            = (1 << 4),
   /* The menu driver owns the userdata */
   MENU_ST_FLAG_DATA_OWN                    = (1 << 5),
   /* Flagged when menu entries need to be refreshed */
   MENU_ST_FLAG_ENTRIES_NEED_REFRESH        = (1 << 6),
   MENU_ST_FLAG_ENTRIES_NONBLOCKING_REFRESH = (1 << 7),
   /* 'Close Content'-hotkey menu resetting */
   MENU_ST_FLAG_PENDING_CLOSE_CONTENT       = (1 << 8),
   /* Flagged when a core calls RETRO_ENVIRONMENT_SHUTDOWN,
    * requiring the menu to be flushed on the next iteration */
   MENU_ST_FLAG_PENDING_ENV_SHUTDOWN_FLUSH  = (1 << 9),
   /* Screensaver status
    * - Does menu driver support screensaver functionality?
    * - Is screensaver currently active? */
   MENU_ST_FLAG_SCREENSAVER_SUPPORTED       = (1 << 10),
   MENU_ST_FLAG_SCREENSAVER_ACTIVE          = (1 << 11),
   MENU_ST_FLAG_PENDING_RELOAD_CORE         = (1 << 12)
};

enum menu_scroll_mode
{
   MENU_SCROLL_PAGE = 0,
   MENU_SCROLL_START_LETTER
};

enum contentless_core_runtime_status
{
   CONTENTLESS_CORE_RUNTIME_UNKNOWN = 0,
   CONTENTLESS_CORE_RUNTIME_MISSING,
   CONTENTLESS_CORE_RUNTIME_VALID
};

enum action_iterate_type
{
   ITERATE_TYPE_DEFAULT = 0,
   ITERATE_TYPE_HELP,
   ITERATE_TYPE_INFO,
   ITERATE_TYPE_BIND
};


enum menu_image_type
{
   MENU_IMAGE_NONE = 0,
   MENU_IMAGE_WALLPAPER,
   MENU_IMAGE_THUMBNAIL,
   MENU_IMAGE_LEFT_THUMBNAIL,
   MENU_IMAGE_SAVESTATE_THUMBNAIL
};

enum menu_environ_cb
{
   MENU_ENVIRON_NONE = 0,
   MENU_ENVIRON_RESET_HORIZONTAL_LIST,
   MENU_ENVIRON_ENABLE_MOUSE_CURSOR,
   MENU_ENVIRON_DISABLE_MOUSE_CURSOR,
   MENU_ENVIRON_ENABLE_SCREENSAVER,
   MENU_ENVIRON_DISABLE_SCREENSAVER,
   MENU_ENVIRON_LAST
};

enum menu_state_changes
{
   MENU_STATE_RENDER_FRAMEBUFFER = 0,
   MENU_STATE_RENDER_MESSAGEBOX,
   MENU_STATE_BLIT,
   MENU_STATE_POP_STACK,
   MENU_STATE_POST_ITERATE
};

enum rarch_menu_ctl_state
{
   RARCH_MENU_CTL_NONE = 0,
   RARCH_MENU_CTL_SET_PENDING_QUICK_MENU,
   RARCH_MENU_CTL_DEINIT,
   RARCH_MENU_CTL_POINTER_DOWN,
   RARCH_MENU_CTL_POINTER_UP,
   RARCH_MENU_CTL_OSK_PTR_AT_POS,
   RARCH_MENU_CTL_BIND_INIT,
   MENU_NAVIGATION_CTL_CLEAR
};

enum menu_timedate_style_type
{
   MENU_TIMEDATE_STYLE_YMD_HMS = 0,
   MENU_TIMEDATE_STYLE_YMD_HM,
   MENU_TIMEDATE_STYLE_YMD,
   MENU_TIMEDATE_STYLE_YM,
   MENU_TIMEDATE_STYLE_MDYYYY_HMS,
   MENU_TIMEDATE_STYLE_MDYYYY_HM,
   MENU_TIMEDATE_STYLE_MD_HM,
   MENU_TIMEDATE_STYLE_MDYYYY,
   MENU_TIMEDATE_STYLE_MD,
   MENU_TIMEDATE_STYLE_DDMMYYYY_HMS,
   MENU_TIMEDATE_STYLE_DDMMYYYY_HM,
   MENU_TIMEDATE_STYLE_DDMM_HM,
   MENU_TIMEDATE_STYLE_DDMMYYYY,
   MENU_TIMEDATE_STYLE_DDMM,
   MENU_TIMEDATE_STYLE_HMS,
   MENU_TIMEDATE_STYLE_HM,
   MENU_TIMEDATE_STYLE_YMD_HMS_AMPM,
   MENU_TIMEDATE_STYLE_YMD_HM_AMPM,
   MENU_TIMEDATE_STYLE_MDYYYY_HMS_AMPM,
   MENU_TIMEDATE_STYLE_MDYYYY_HM_AMPM,
   MENU_TIMEDATE_STYLE_MD_HM_AMPM,
   MENU_TIMEDATE_STYLE_DDMMYYYY_HMS_AMPM,
   MENU_TIMEDATE_STYLE_DDMMYYYY_HM_AMPM,
   MENU_TIMEDATE_STYLE_DDMM_HM_AMPM,
   MENU_TIMEDATE_STYLE_HMS_AMPM,
   MENU_TIMEDATE_STYLE_HM_AMPM,
   MENU_TIMEDATE_STYLE_LAST
};

enum menu_remember_selection_type
{
   MENU_REMEMBER_SELECTION_OFF = 0,
   MENU_REMEMBER_SELECTION_ALWAYS,
   MENU_REMEMBER_SELECTION_PLAYLISTS,
   MENU_REMEMBER_SELECTION_MAIN,
   MENU_REMEMBER_SELECTION_LAST
};

/* Note: These must be kept synchronised with
 * 'enum playlist_sublabel_last_played_date_separator_type'
 * in 'runtime_file.h' */
enum menu_timedate_date_separator_type
{
   MENU_TIMEDATE_DATE_SEPARATOR_HYPHEN = 0,
   MENU_TIMEDATE_DATE_SEPARATOR_SLASH,
   MENU_TIMEDATE_DATE_SEPARATOR_PERIOD,
   MENU_TIMEDATE_DATE_SEPARATOR_LAST
};

/* Specifies location of the 'Import Content' menu */
enum menu_add_content_entry_display_type
{
   MENU_ADD_CONTENT_ENTRY_DISPLAY_HIDDEN = 0,
   MENU_ADD_CONTENT_ENTRY_DISPLAY_MAIN_TAB,
   MENU_ADD_CONTENT_ENTRY_DISPLAY_PLAYLISTS_TAB,
   MENU_ADD_CONTENT_ENTRY_DISPLAY_LAST
};

/* Specifies which type of core will be displayed
 * in the 'contentless cores' menu */
enum menu_contentless_cores_display_type
{
   MENU_CONTENTLESS_CORES_DISPLAY_NONE = 0,
   MENU_CONTENTLESS_CORES_DISPLAY_ALL,
   MENU_CONTENTLESS_CORES_DISPLAY_SINGLE_PURPOSE,
   MENU_CONTENTLESS_CORES_DISPLAY_CUSTOM,
   MENU_CONTENTLESS_CORES_DISPLAY_LAST
};

enum rgui_color_theme
{
   RGUI_THEME_CUSTOM = 0,
   RGUI_THEME_CLASSIC_RED,
   RGUI_THEME_CLASSIC_ORANGE,
   RGUI_THEME_CLASSIC_YELLOW,
   RGUI_THEME_CLASSIC_GREEN,
   RGUI_THEME_CLASSIC_BLUE,
   RGUI_THEME_CLASSIC_VIOLET,
   RGUI_THEME_CLASSIC_GREY,
   RGUI_THEME_LEGACY_RED,
   RGUI_THEME_DARK_PURPLE,
   RGUI_THEME_MIDNIGHT_BLUE,
   RGUI_THEME_GOLDEN,
   RGUI_THEME_ELECTRIC_BLUE,
   RGUI_THEME_APPLE_GREEN,
   RGUI_THEME_VOLCANIC_RED,
   RGUI_THEME_LAGOON,
   RGUI_THEME_BROGRAMMER,
   RGUI_THEME_DRACULA,
   RGUI_THEME_FAIRYFLOSS,
   RGUI_THEME_FLATUI,
   RGUI_THEME_GRUVBOX_DARK,
   RGUI_THEME_GRUVBOX_LIGHT,
   RGUI_THEME_HACKING_THE_KERNEL,
   RGUI_THEME_NORD,
   RGUI_THEME_NOVA,
   RGUI_THEME_ONE_DARK,
   RGUI_THEME_PALENIGHT,
   RGUI_THEME_SOLARIZED_DARK,
   RGUI_THEME_SOLARIZED_LIGHT,
   RGUI_THEME_TANGO_DARK,
   RGUI_THEME_TANGO_LIGHT,
   RGUI_THEME_ZENBURN,
   RGUI_THEME_ANTI_ZENBURN,
   RGUI_THEME_FLUX,
   RGUI_THEME_DYNAMIC,
   RGUI_THEME_GRAY_DARK,
   RGUI_THEME_GRAY_LIGHT,
   RGUI_THEME_LAST
};

enum materialui_color_theme
{
   MATERIALUI_THEME_BLUE = 0,
   MATERIALUI_THEME_BLUE_GREY,
   MATERIALUI_THEME_DARK_BLUE,
   MATERIALUI_THEME_GREEN,
   MATERIALUI_THEME_RED,
   MATERIALUI_THEME_YELLOW,
   MATERIALUI_THEME_NVIDIA_SHIELD,
   MATERIALUI_THEME_MATERIALUI,
   MATERIALUI_THEME_MATERIALUI_DARK,
   MATERIALUI_THEME_OZONE_DARK,
   MATERIALUI_THEME_NORD,
   MATERIALUI_THEME_GRUVBOX_DARK,
   MATERIALUI_THEME_SOLARIZED_DARK,
   MATERIALUI_THEME_CUTIE_BLUE,
   MATERIALUI_THEME_CUTIE_CYAN,
   MATERIALUI_THEME_CUTIE_GREEN,
   MATERIALUI_THEME_CUTIE_ORANGE,
   MATERIALUI_THEME_CUTIE_PINK,
   MATERIALUI_THEME_CUTIE_PURPLE,
   MATERIALUI_THEME_CUTIE_RED,
   MATERIALUI_THEME_VIRTUAL_BOY,
   MATERIALUI_THEME_HACKING_THE_KERNEL,
   MATERIALUI_THEME_GRAY_DARK,
   MATERIALUI_THEME_GRAY_LIGHT,
   MATERIALUI_THEME_LAST
};

enum materialui_transition_animation
{
   MATERIALUI_TRANSITION_ANIM_AUTO = 0,
   MATERIALUI_TRANSITION_ANIM_FADE,
   MATERIALUI_TRANSITION_ANIM_SLIDE,
   MATERIALUI_TRANSITION_ANIM_NONE,
   MATERIALUI_TRANSITION_ANIM_LAST
};

enum materialui_thumbnail_view_portrait
{
   MATERIALUI_THUMBNAIL_VIEW_PORTRAIT_DISABLED = 0,
   MATERIALUI_THUMBNAIL_VIEW_PORTRAIT_LIST_SMALL,
   MATERIALUI_THUMBNAIL_VIEW_PORTRAIT_LIST_MEDIUM,
   MATERIALUI_THUMBNAIL_VIEW_PORTRAIT_DUAL_ICON,
   MATERIALUI_THUMBNAIL_VIEW_PORTRAIT_LAST
};

enum materialui_thumbnail_view_landscape
{
   MATERIALUI_THUMBNAIL_VIEW_LANDSCAPE_DISABLED = 0,
   MATERIALUI_THUMBNAIL_VIEW_LANDSCAPE_LIST_SMALL,
   MATERIALUI_THUMBNAIL_VIEW_LANDSCAPE_LIST_MEDIUM,
   MATERIALUI_THUMBNAIL_VIEW_LANDSCAPE_LIST_LARGE,
   MATERIALUI_THUMBNAIL_VIEW_LANDSCAPE_DESKTOP,
   MATERIALUI_THUMBNAIL_VIEW_LANDSCAPE_LAST
};

enum materialui_landscape_layout_optimization_type
{
   MATERIALUI_LANDSCAPE_LAYOUT_OPTIMIZATION_DISABLED = 0,
   MATERIALUI_LANDSCAPE_LAYOUT_OPTIMIZATION_ALWAYS,
   MATERIALUI_LANDSCAPE_LAYOUT_OPTIMIZATION_EXCLUDE_THUMBNAIL_VIEWS,
   MATERIALUI_LANDSCAPE_LAYOUT_OPTIMIZATION_LAST
};

enum xmb_color_theme
{
   XMB_THEME_LEGACY_RED  = 0,
   XMB_THEME_DARK_PURPLE,
   XMB_THEME_MIDNIGHT_BLUE,
   XMB_THEME_GOLDEN,
   XMB_THEME_ELECTRIC_BLUE,
   XMB_THEME_APPLE_GREEN,
   XMB_THEME_UNDERSEA,
   XMB_THEME_VOLCANIC_RED,
   XMB_THEME_DARK,
   XMB_THEME_LIGHT,
   XMB_THEME_WALLPAPER,
   XMB_THEME_MORNING_BLUE,
   XMB_THEME_SUNBEAM,
   XMB_THEME_LIME,
   XMB_THEME_PIKACHU_YELLOW,
   XMB_THEME_GAMECUBE_PURPLE,
   XMB_THEME_FAMICOM_RED,
   XMB_THEME_FLAMING_HOT,
   XMB_THEME_ICE_COLD,
   XMB_THEME_MIDGAR,
   XMB_THEME_GRAY_DARK,
   XMB_THEME_GRAY_LIGHT,
   XMB_THEME_LAST
};

enum xmb_icon_theme
{
   XMB_ICON_THEME_MONOCHROME = 0,
   XMB_ICON_THEME_FLATUI,
   XMB_ICON_THEME_FLATUX,
   XMB_ICON_THEME_PIXEL,
   XMB_ICON_THEME_SYSTEMATIC,
   XMB_ICON_THEME_DOTART,
   XMB_ICON_THEME_CUSTOM,
   XMB_ICON_THEME_RETROSYSTEM,
   XMB_ICON_THEME_MONOCHROME_INVERTED,
   XMB_ICON_THEME_AUTOMATIC,
   XMB_ICON_THEME_AUTOMATIC_INVERTED,
   XMB_ICON_THEME_DAITE,
   XMB_ICON_THEME_LAST
};

enum xmb_shader_pipeline
{
#ifndef HAVE_PSGL
   XMB_SHADER_PIPELINE_WALLPAPER = 0,
   XMB_SHADER_PIPELINE_SIMPLE_RIBBON,
   XMB_SHADER_PIPELINE_RIBBON,
   XMB_SHADER_PIPELINE_SIMPLE_SNOW,
   XMB_SHADER_PIPELINE_SNOW,
   XMB_SHADER_PIPELINE_BOKEH,
   XMB_SHADER_PIPELINE_SNOWFLAKE,
   XMB_SHADER_PIPELINE_LAST
#else
   XMB_SHADER_PIPELINE_WALLPAPER = 0,
   XMB_SHADER_PIPELINE_SIMPLE_RIBBON,
   XMB_SHADER_PIPELINE_RIBBON,
   XMB_SHADER_PIPELINE_LAST,
   XMB_SHADER_PIPELINE_SIMPLE_SNOW,
   XMB_SHADER_PIPELINE_SNOW,
   XMB_SHADER_PIPELINE_BOKEH,
   XMB_SHADER_PIPELINE_SNOWFLAKE
#endif
};

enum rgui_thumbnail_scaler
{
   RGUI_THUMB_SCALE_POINT = 0,
   RGUI_THUMB_SCALE_BILINEAR,
   RGUI_THUMB_SCALE_SINC,
   RGUI_THUMB_SCALE_LAST
};

enum rgui_upscale_level
{
   RGUI_UPSCALE_NONE = 0,
   RGUI_UPSCALE_AUTO,
   RGUI_UPSCALE_X2,
   RGUI_UPSCALE_X3,
   RGUI_UPSCALE_X4,
   RGUI_UPSCALE_X5,
   RGUI_UPSCALE_X6,
   RGUI_UPSCALE_X7,
   RGUI_UPSCALE_X8,
   RGUI_UPSCALE_X9, /* All the way to 4k */
   RGUI_UPSCALE_LAST
};

enum rgui_aspect_ratio
{
   RGUI_ASPECT_RATIO_4_3 = 0,
   RGUI_ASPECT_RATIO_16_9,
   RGUI_ASPECT_RATIO_16_9_CENTRE,
   RGUI_ASPECT_RATIO_16_10,
   RGUI_ASPECT_RATIO_16_10_CENTRE,
   RGUI_ASPECT_RATIO_21_9,
   RGUI_ASPECT_RATIO_21_9_CENTRE,
   RGUI_ASPECT_RATIO_3_2,
   RGUI_ASPECT_RATIO_3_2_CENTRE,
   RGUI_ASPECT_RATIO_5_3,
   RGUI_ASPECT_RATIO_5_3_CENTRE,
   RGUI_ASPECT_RATIO_AUTO,
   RGUI_ASPECT_RATIO_LAST
};

enum rgui_aspect_ratio_lock
{
   RGUI_ASPECT_RATIO_LOCK_NONE = 0,
   RGUI_ASPECT_RATIO_LOCK_FIT_SCREEN,
   RGUI_ASPECT_RATIO_LOCK_INTEGER,
   RGUI_ASPECT_RATIO_LOCK_FILL_SCREEN,
   RGUI_ASPECT_RATIO_LOCK_LAST
};

enum rgui_particle_animation_effect
{
   RGUI_PARTICLE_EFFECT_NONE = 0,
   RGUI_PARTICLE_EFFECT_SNOW,
   RGUI_PARTICLE_EFFECT_SNOW_ALT,
   RGUI_PARTICLE_EFFECT_RAIN,
   RGUI_PARTICLE_EFFECT_VORTEX,
   RGUI_PARTICLE_EFFECT_STARFIELD,
   RGUI_PARTICLE_EFFECT_LAST
};

enum ozone_color_theme
{
   OZONE_COLOR_THEME_BASIC_WHITE = 0,
   OZONE_COLOR_THEME_BASIC_BLACK,
   OZONE_COLOR_THEME_NORD,
   OZONE_COLOR_THEME_GRUVBOX_DARK,
   OZONE_COLOR_THEME_BOYSENBERRY,
   OZONE_COLOR_THEME_HACKING_THE_KERNEL,
   OZONE_COLOR_THEME_TWILIGHT_ZONE,
   OZONE_COLOR_THEME_DRACULA,
   OZONE_COLOR_THEME_SOLARIZED_DARK,
   OZONE_COLOR_THEME_SOLARIZED_LIGHT,
   OZONE_COLOR_THEME_GRAY_DARK,
   OZONE_COLOR_THEME_GRAY_LIGHT,
   OZONE_COLOR_THEME_PURPLE_RAIN,
   OZONE_COLOR_THEME_SELENIUM,
   OZONE_COLOR_THEME_LAST
};

enum ozone_header_separator
{
   OZONE_HEADER_SEPARATOR_NONE = 0,
   OZONE_HEADER_SEPARATOR_NORMAL,
   OZONE_HEADER_SEPARATOR_MAXIMUM,
   OZONE_HEADER_SEPARATOR_LAST
};

enum ozone_font_scale
{
   OZONE_FONT_SCALE_NONE = 0,
   OZONE_FONT_SCALE_GLOBAL,
   OZONE_FONT_SCALE_SEPARATE,
   OZONE_FONT_SCALE_LAST
};

enum menu_action
{
   MENU_ACTION_NOOP = 0,
   MENU_ACTION_UP,
   MENU_ACTION_DOWN,
   MENU_ACTION_LEFT,
   MENU_ACTION_RIGHT,
   MENU_ACTION_OK,
   MENU_ACTION_SEARCH,
   MENU_ACTION_SCAN,
   MENU_ACTION_CANCEL,
   MENU_ACTION_INFO,
   MENU_ACTION_SELECT,
   MENU_ACTION_START,
   MENU_ACTION_SCROLL_DOWN,
   MENU_ACTION_SCROLL_UP,
   MENU_ACTION_SCROLL_HOME,
   MENU_ACTION_SCROLL_END,
   MENU_ACTION_CYCLE_THUMBNAIL_PRIMARY,
   MENU_ACTION_CYCLE_THUMBNAIL_SECONDARY,
   MENU_ACTION_TOGGLE,
   MENU_ACTION_RESUME,
   MENU_ACTION_POINTER_MOVED,
   MENU_ACTION_POINTER_PRESSED,
   MENU_ACTION_ACCESSIBILITY_SPEAK_TITLE,
   MENU_ACTION_ACCESSIBILITY_SPEAK_LABEL,
   MENU_ACTION_ACCESSIBILITY_SPEAK_TITLE_LABEL
};

enum playlist_inline_core_display_type
{
   PLAYLIST_INLINE_CORE_DISPLAY_HIST_FAV = 0,
   PLAYLIST_INLINE_CORE_DISPLAY_ALWAYS,
   PLAYLIST_INLINE_CORE_DISPLAY_NEVER,
   PLAYLIST_INLINE_CORE_DISPLAY_LAST
};

enum playlist_entry_remove_enable_type
{
   PLAYLIST_ENTRY_REMOVE_ENABLE_HIST_FAV = 0,
   PLAYLIST_ENTRY_REMOVE_ENABLE_ALL,
   PLAYLIST_ENTRY_REMOVE_ENABLE_NONE,
   PLAYLIST_ENTRY_REMOVE_ENABLE_LAST
};

enum playlist_show_history_icons_type
{
   PLAYLIST_SHOW_HISTORY_ICONS_DEFAULT = 0,
   PLAYLIST_SHOW_HISTORY_ICONS_MAIN,
   PLAYLIST_SHOW_HISTORY_ICONS_CONTENT,
   PLAYLIST_SHOW_HISTORY_ICONS_LAST
};

enum quit_on_close_content_type
{
   QUIT_ON_CLOSE_CONTENT_DISABLED = 0,
   QUIT_ON_CLOSE_CONTENT_ENABLED,
   QUIT_ON_CLOSE_CONTENT_CLI,
   QUIT_ON_CLOSE_CONTENT_LAST
};

#if defined(DINGUX) && (defined(RS90) || defined(MIYOO))
enum dingux_rs90_softfilter_type
{
   DINGUX_RS90_SOFTFILTER_POINT = 0,
   DINGUX_RS90_SOFTFILTER_BRESENHAM_HORZ,
   DINGUX_RS90_SOFTFILTER_LAST
};
#endif

/* Specifies all available screensaver effects */
enum menu_screensaver_effect
{
   MENU_SCREENSAVER_BLANK = 0,
   MENU_SCREENSAVER_SNOW,
   MENU_SCREENSAVER_STARFIELD,
   MENU_SCREENSAVER_VORTEX,
   MENU_SCREENSAVER_LAST
};

enum menu_dialog_type
{
   MENU_DIALOG_NONE = 0,
   MENU_DIALOG_WELCOME,
   MENU_DIALOG_HELP_EXTRACT,
   MENU_DIALOG_HELP_CONTROLS,
   MENU_DIALOG_HELP_CHEEVOS_DESCRIPTION,
   MENU_DIALOG_HELP_LOADING_CONTENT,
   MENU_DIALOG_HELP_WHAT_IS_A_CORE,
   MENU_DIALOG_HELP_CHANGE_VIRTUAL_GAMEPAD,
   MENU_DIALOG_HELP_AUDIO_VIDEO_TROUBLESHOOTING,
   MENU_DIALOG_HELP_SCANNING_CONTENT,
   MENU_DIALOG_QUIT_CONFIRM,
   MENU_DIALOG_INFORMATION,
   MENU_DIALOG_QUESTION,
   MENU_DIALOG_WARNING,
   MENU_DIALOG_ERROR,
   MENU_DIALOG_LAST
};

enum menu_input_binds_ctl_state
{
   MENU_INPUT_BINDS_CTL_BIND_NONE = 0,
   MENU_INPUT_BINDS_CTL_BIND_SINGLE,
   MENU_INPUT_BINDS_CTL_BIND_ALL
};

struct menu_dialog
{
   unsigned              current_id;
   enum menu_dialog_type current_type;
   bool                  pending_push;
};

typedef struct menu_dialog menu_dialog_t;

#ifdef HAVE_RUNAHEAD
enum menu_runahead_mode
{
   MENU_RUNAHEAD_MODE_OFF = 0,
   MENU_RUNAHEAD_MODE_SINGLE_INSTANCE,
#if (defined(HAVE_DYNAMIC) || defined(HAVE_DYLIB))
   MENU_RUNAHEAD_MODE_SECOND_INSTANCE,
#endif
   MENU_RUNAHEAD_MODE_PREEMPTIVE_FRAMES,
   MENU_RUNAHEAD_MODE_LAST
};
#endif

RETRO_END_DECLS

#endif
