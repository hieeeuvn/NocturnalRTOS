#ifndef PHOBOS_ACHIEVEMENTS_PLATFORM_H
#define PHOBOS_ACHIEVEMENTS_PLATFORM_H

/*
 * Platform glue for the offline RetroAchievements engine.
 *
 * Phobos runs on hcrtos, not on UniFrog's foundation layer, so the engine
 * talks to the SD card, the clock, the log and the framebuffer through these
 * helpers. Keep the on-disk layout in sync with UniFrog and tools/ra_sync.mjs.
 */

#include <stddef.h>
#include <stdint.h>

/* phobos.h owns this define; repeat it so the engine does not need libretro.h.
 * Keep in sync with SDCARD_DIRECTORY in ../phobos.h. */
#ifndef SDCARD_DIRECTORY
#define SDCARD_DIRECTORY "/media/mmcblk0p2"
#endif

#define PHOBOS_ACHIEVEMENTS_ROOT SDCARD_DIRECTORY "/system/achievements"
#define PHOBOS_ACHIEVEMENTS_CONFIG_PATH \
   PHOBOS_ACHIEVEMENTS_ROOT "/ra.ini"
#define PHOBOS_ACHIEVEMENTS_CACHE_ROOT \
   PHOBOS_ACHIEVEMENTS_ROOT "/cache"
#define PHOBOS_ACHIEVEMENTS_PROGRESS_ROOT \
   PHOBOS_ACHIEVEMENTS_ROOT "/progress"
#define PHOBOS_ACHIEVEMENTS_EARNED_ROOT \
   PHOBOS_ACHIEVEMENTS_ROOT "/earned"
#define PHOBOS_ACHIEVEMENTS_QUEUE_ROOT \
   PHOBOS_ACHIEVEMENTS_ROOT "/queue"
#define PHOBOS_ACHIEVEMENTS_UNLOCK_PATH \
   PHOBOS_ACHIEVEMENTS_QUEUE_ROOT "/unlocks.jsonl"

/* Same palette as the UniFrog banner. */
#ifndef RGB565
#define RGB565(r, g, b) \
   ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))
#endif

/* A core frame as handed to frontend_video_cb, writable for the overlay. */
struct phobos_achievements_surface {
   void *pixels;
   unsigned width;
   unsigned height;
   size_t pitch;
   int rgb32;
};

enum phobos_achievements_log_level {
   PHOBOS_ACH_LOG_INFO = 0,
   PHOBOS_ACH_LOG_WARN,
};

typedef int (*phobos_achievements_config_entry_cb)(void *userdata,
   const char *section, const char *key, const char *value,
   unsigned line_number);

int phobos_achievements_config_read(const char *path,
   phobos_achievements_config_entry_cb callback, void *userdata,
   unsigned *error_count);

void phobos_achievements_log(enum phobos_achievements_log_level level,
   const char *component, const char *format, ...);

#define PHOBOS_ACH_LOG_INFO(...) \
   phobos_achievements_log(PHOBOS_ACH_LOG_INFO, __VA_ARGS__)
#define PHOBOS_ACH_LOG_WARN(...) \
   phobos_achievements_log(PHOBOS_ACH_LOG_WARN, __VA_ARGS__)

/* Milliseconds since boot, used for toast timing and progress flushes. */
uint32_t phobos_achievements_time_ms(void);

void phobos_achievements_fill_rect(struct phobos_achievements_surface *surface,
   int x, int y, int w, int h, uint16_t color);
void phobos_achievements_draw_text(struct phobos_achievements_surface *surface,
   int x, int y, const char *text, uint16_t color, int scale);
int phobos_achievements_text_width_limit(const char *text, int max_px);

#endif
