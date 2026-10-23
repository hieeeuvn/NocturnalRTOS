#ifndef PHOBOS_ACHIEVEMENTS_H
#define PHOBOS_ACHIEVEMENTS_H

#include <stddef.h>
#include <stdint.h>

#include "phobos_achievements_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Offline-first RetroAchievements support for Phobos.
 *
 * The console has no network stack, so achievement definitions and unlock
 * uploads travel through the SD card:
 *
 *   achievements/cache/<hash>.json    raw "patch" response for a game
 *   achievements/earned/<hash>.txt    achievement ids already awarded here
 *   achievements/queue/<hash>.req     games the device could not identify
 *   achievements/queue/unlocks.jsonl  unlocks waiting to be uploaded
 *
 * tools/ra_sync.mjs, run on a PC or phone, fills the cache and drains the
 * queue. The formats match UniFrog so one tool serves both firmwares.
 */

#define PHOBOS_ACHIEVEMENTS_TITLE_MAX 48
#define PHOBOS_ACHIEVEMENTS_DESCRIPTION_MAX 112
#define PHOBOS_ACHIEVEMENTS_STATUS_MAX 96
#define PHOBOS_ACHIEVEMENTS_NAME_MAX 64
#define PHOBOS_ACHIEVEMENTS_HASH_MAX 33

enum phobos_achievements_state {
   PHOBOS_ACHIEVEMENTS_OFF = 0,
   PHOBOS_ACHIEVEMENTS_WAITING_HASH,
   PHOBOS_ACHIEVEMENTS_WAITING_CACHE,
   PHOBOS_ACHIEVEMENTS_READY,
   PHOBOS_ACHIEVEMENTS_ERROR,
};

struct phobos_achievements_summary {
   enum phobos_achievements_state state;
   int hardcore;
   unsigned total;
   unsigned unlocked;
   unsigned points_total;
   unsigned points_unlocked;
   unsigned pending_uploads;
   char game_title[PHOBOS_ACHIEVEMENTS_NAME_MAX];
   char hash[PHOBOS_ACHIEVEMENTS_HASH_MAX];
   char status[PHOBOS_ACHIEVEMENTS_STATUS_MAX];
};

struct phobos_achievements_entry {
   unsigned id;
   unsigned points;
   int unlocked;
   /* -1 when the achievement has no measured progress. */
   int progress_percent;
   char title[PHOBOS_ACHIEVEMENTS_TITLE_MAX];
   char description[PHOBOS_ACHIEVEMENTS_DESCRIPTION_MAX];
};

/*
 * Register the core memory accessors so the engine can read emulated RAM.
 * Phobos passes the core's retro_get_memory_data/retro_get_memory_size.
 */
void phobos_achievements_set_memory_access(
   void *(*data)(unsigned id), size_t (*size)(unsigned id));

/*
 * Start tracking the content that is about to run. Called after
 * retro_load_game() so core memory is available. Returns 0 when the subsystem
 * is disabled or the core has no RetroAchievements console.
 */
int phobos_achievements_begin(const char *core_id, const char *content_path);

/* Release runtime/cache resources. Safe to call without a matching begin. */
void phobos_achievements_end(void);

/* Evaluate triggers and advance unlock toasts. Called once per core frame. */
void phobos_achievements_frame(void);

/* Draw the unlock toast and the optional status card over a core frame.
 * Returns 1 if anything was painted. */
int phobos_achievements_draw_overlay(struct phobos_achievements_surface *surface);

/* Overlay card toggled by the SELECT+L hotkey. */
void phobos_achievements_toggle_overlay(void);
int phobos_achievements_overlay_visible(void);

/* Current game summary, list access, and the transient toast text. */
void phobos_achievements_summary(struct phobos_achievements_summary *summary);
unsigned phobos_achievements_count(void);
int phobos_achievements_entry(unsigned index,
   struct phobos_achievements_entry *entry);
const char *phobos_achievements_toast(void);

/* Persisted to achievements/ra.ini. */
int phobos_achievements_enabled(void);
int phobos_achievements_set_enabled(int enabled);
int phobos_achievements_hardcore(void);
int phobos_achievements_set_hardcore(int hardcore);
void phobos_achievements_refresh(void);

#ifdef __cplusplus
}
#endif

#endif
