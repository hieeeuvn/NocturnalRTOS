# RetroAchievements in Phobos

The SF2000 and GB300 have no Wi-Fi hardware, so Phobos cannot talk to
retroachievements.org while a game runs. Achievements are evaluated offline
from definitions cached on the SD card, and unlocks are uploaded later by a
companion tool on a PC or phone.

This is the same design and the same on-disk format as
[UniFrog](https://github.com/axgdev/UniFrog); either firmware's
`tools/ra_sync.mjs` can service a card written by the other.

## How it works

1. When a game starts, Phobos computes the RetroAchievements hash with
   [rcheevos](https://github.com/RetroAchievements/rcheevos), loads
   `system/achievements/cache/<hash>.json`, and evaluates the triggers once per
   emulated frame.
2. Unlocks are appended to `system/achievements/queue/unlocks.jsonl` and shown
   as a banner. The id is also written to `system/achievements/earned/<hash>.txt`
   so replaying a game does not queue the same award again.
3. `tools/ra_sync.mjs` downloads definitions and uploads queued unlocks.
4. Runtime progress is saved every five seconds and when the game exits, so a
   power cut costs at most a few seconds.

Nothing in Phobos opens a socket. The `rc_client` layer of rcheevos is not
built at all; only the parser, runtime, memory mapping and hash helpers are.

## Player setup

1. Copy your **Web API Key** from retroachievements.org
   (Profile -> Settings -> Keys) into
   `system/achievements/ra_sync.ini` on the SD card:

   ```ini
   [retroachievements]
   username=YourName
   apikey=0123456789abcdef0123456789abcdef
   ```

   Only the companion tool reads this file. Phobos stores the username in
   `system/achievements/ra.ini`.

2. Play a game once. If no definitions are cached, Phobos writes
   `system/achievements/queue/<hash>.req`.

3. Mount the card on a computer and run:

   ```sh
   node tools/ra_sync.mjs --root /media/mmcblk0p2/system
   ```

   On a PC the path is the mounted card's `system` directory. The tool
   downloads one JSON file per game, uploads queued unlocks, and removes
   successful queue entries. `--list` inspects the card without the network,
   `--dry-run` reports what would be sent.

4. Eject the card safely and put it back in the handheld. Unlocks now appear as
   banners in game, and the queue is empty after a successful upload.

Node 18 or newer is the only requirement; there are no npm dependencies.

## In-game UI

Phobos has no pause menu. Achievements therefore use two overlays drawn on top
of the running frame:

- **Unlock banner** - shown at the top of the screen for `toast_ms`
  milliseconds whenever an achievement triggers. The banner is painted into the
  core frame before scaling, so it follows the configured scale and rotation.
- **Status card** - press **SELECT + L** to toggle a card with the game title,
  unlocked/total count, points, and the number of unlocks still waiting for
  `ra_sync`. Press **SELECT + L** again to hide it.

The existing **SELECT + START** exit hotkey is unchanged.

## Configuration

`system/achievements/ra.ini` is written by Phobos:

| Key | Default | Meaning |
| --- | --- | --- |
| `enabled` | `1` | Evaluate cached achievements while playing. |
| `hardcore` | `0` | Reserved. Only softcore is enforced today. |
| `toast_ms` | `4000` | Unlock banner duration, clamped to 500..15000. |
| `username` | empty | Cached username, informational. |

Set `enabled=0` (with the card mounted) to turn tracking off without deleting
caches. Definitions are used per game whenever a cache exists.

## Supported systems

Hashing and memory mapping are console specific. Phobos maps the core name it
loads to a RetroAchievements console:

| Core name | System |
| --- | --- |
| `fceumm`, `quicknes` | NES |
| `snes9x2002`, `snes9x2005`, `snes9x` | SNES |
| `gambatte`, `gearboy` | Game Boy / Game Boy Color |
| `gpsp` | Game Boy Advance |
| `picodrive` | Mega Drive, Master System, Game Gear, 32X, Sega CD (by extension) |
| `pce-fast`, `pce_fast` | PC Engine / TurboGrafx-16 |
| `prosystem`, `stella2014`, `a5200`, `handy`, `beetle-cygne`, `race`, `gearcoleco`, `bluemsx`, `frodo`, `qpsx` | Other known cores, when built |

An unknown core disables tracking instead of guessing a hash: an MD5 computed
for the wrong console is valid but will never match the server.

## Troubleshooting

- **"No achievement data - run ra_sync"** - the game was identified but no
  `cache/<hash>.json` exists. Run the tool with the card mounted.
- **"Achievement data rejected"** - the cache file is not the raw `patch`
  response. Delete it and run `--refresh <hash>`.
- **Achievements never trigger** - press SELECT + L; the card shows whether
  tracking is on and whether the game has definitions.
- **Unlocks stay in the queue** - run `--push-only` and read the error. Expired
  sessions, a wrong API key or Cloudflare blocks are reported and the entry is
  kept for the next run.

## Developer notes

- Engine: `components/applications/apps-phobos/source/phobos/achievements/`
  (`phobos_achievements.c`, public API in `phobos_achievements.h`).
- Platform glue: `phobos_achievements_platform.c` (SD paths, INI reader,
  FreeRTOS clock, framebuffer text). The engine is a port of UniFrog's; keep
  both engines' file formats identical even if the code diverges.
- Hooks: `libretro.c` registers `retro_get_memory_data/size`, starts tracking
  after `retro_load_game`, evaluates after each `retro_run`, and flushes on
  unload. `sf2000_gfx.c` paints the overlay before `blit_to_screen`.
  `sf2000_joypad.c` owns the SELECT+L hotkey.
- Build: rcheevos is a Buildroot package (`components/rcheevos/`) pinned to
  `v12.5.0`, built as a static library without `rc_client`, Lua or threads.
  `apps-phobos` selects it and links `-lrcheevos`.
- Companion protocol: `r=patch&m=<hash>` for definitions and
  `r=awardachievement` with the signature
  `md5(id + user + hardcore [+ id + seconds])`. The queue entry fields are
  `a` id, `p` points, `h` game hash, `m` hardcore flag, `t` unix time.
- rcheevos only serializes *unfinished* triggers, so earned ids live in
  `earned/<hash>.txt` (one id per line). When that file is absent the engine
  seeds it from `queue/unlocks.jsonl`, so upgrading does not re-queue awards.

## Limitations

- Softcore only. `hardcore=1` is stored but does not yet block save states or
  cheats.
- Leaderboards are parsed but not evaluated in the UI.
- The status card is a summary; a scrollable per-achievement list needs a pause
  menu, which Phobos does not have yet.
