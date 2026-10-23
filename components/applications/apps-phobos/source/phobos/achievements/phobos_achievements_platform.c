/*
 * Platform glue for the Phobos offline RetroAchievements engine.
 *
 * The engine is a port of UniFrog's implementation and shares its on-disk
 * format. Everything the two frontends do not have in common lives here:
 * the SD path root, the INI reader, the FreeRTOS clock, logging and the
 * framebuffer primitives used for the unlock banner.
 */
#include "phobos_achievements_platform.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <kernel/drivers/lcd_font.h>

/* ------------------------------------------------------------------------- */
/* Clock and logging                                                         */
/* ------------------------------------------------------------------------- */

uint32_t phobos_achievements_time_ms(void)
{
   return (uint32_t)(xTaskGetTickCount() * (TickType_t)portTICK_PERIOD_MS);
}

void phobos_achievements_log(enum phobos_achievements_log_level level,
   const char *component, const char *format, ...)
{
   va_list args;

   printf("[ACH][%s][%s] ", level == PHOBOS_ACH_LOG_WARN ? "WARN" : "INFO",
      component ? component : "?");
   va_start(args, format);
   vprintf(format, args);
   va_end(args);
   printf("\n");
}

/* ------------------------------------------------------------------------- */
/* INI reader                                                                */
/* ------------------------------------------------------------------------- */

static char *config_trim(char *text)
{
   char *end;

   while (*text && isspace((unsigned char)*text))
      text++;
   end = text + strlen(text);
   while (end > text && isspace((unsigned char)end[-1]))
      end--;
   *end = '\0';
   return text;
}

int phobos_achievements_config_read(const char *path,
   phobos_achievements_config_entry_cb callback, void *userdata,
   unsigned *error_count)
{
   FILE *file;
   char line[256];
   char section[64] = "";
   unsigned number = 0;
   unsigned errors = 0;

   if (error_count)
      *error_count = 0;
   file = fopen(path, "rb");
   if (!file)
      return -1;
   while (fgets(line, sizeof(line), file)) {
      char *text;
      char *separator;

      number++;
      /* Strip a UTF-8 BOM on the first line so hand-edited files work. */
      text = line;
      if (number == 1 && (unsigned char)text[0] == 0xEF &&
          (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF)
         text += 3;
      text = config_trim(text);
      if (!*text || *text == '#' || *text == ';')
         continue;
      if (*text == '[') {
         char *close = strchr(text, ']');

         if (!close) {
            errors++;
            continue;
         }
         *close = '\0';
         snprintf(section, sizeof(section), "%s", config_trim(text + 1));
         continue;
      }
      separator = strchr(text, '=');
      if (!separator) {
         errors++;
         continue;
      }
      *separator = '\0';
      if (callback)
         callback(userdata, section, config_trim(text),
            config_trim(separator + 1), number);
   }
   fclose(file);
   if (error_count)
      *error_count = errors;
   return 0;
}

/* ------------------------------------------------------------------------- */
/* Framebuffer drawing                                                       */
/* ------------------------------------------------------------------------- */

static unsigned surface_bytes_per_pixel(
   const struct phobos_achievements_surface *surface)
{
   return surface->rgb32 ? 4u : 2u;
}

static void put_pixel(struct phobos_achievements_surface *surface,
   int x, int y, uint16_t color)
{
   uint8_t *pixel;

   if (!surface || !surface->pixels)
      return;
   if (x < 0 || y < 0 || (unsigned)x >= surface->width ||
       (unsigned)y >= surface->height)
      return;
   pixel = (uint8_t *)surface->pixels + (size_t)y * surface->pitch +
      (size_t)x * surface_bytes_per_pixel(surface);
   if (surface->rgb32) {
      /* XRGB8888, same channel order the core buffers use on this SoC. */
      pixel[0] = (uint8_t)((color & 0x1F) << 3);
      pixel[1] = (uint8_t)(((color >> 5) & 0x3F) << 2);
      pixel[2] = (uint8_t)(((color >> 11) & 0x1F) << 3);
      pixel[3] = 0xFF;
   } else {
      pixel[0] = (uint8_t)(color & 0xFF);
      pixel[1] = (uint8_t)(color >> 8);
   }
}

void phobos_achievements_fill_rect(struct phobos_achievements_surface *surface,
   int x, int y, int w, int h, uint16_t color)
{
   int row;
   int column;

   if (!surface || !surface->pixels || w <= 0 || h <= 0)
      return;
   for (row = 0; row < h; row++) {
      for (column = 0; column < w; column++)
         put_pixel(surface, x + column, y + row, color);
   }
}

void phobos_achievements_draw_text(struct phobos_achievements_surface *surface,
   int x, int y, const char *text, uint16_t color, int scale)
{
   int cursor = x;

   if (!surface || !surface->pixels || !text)
      return;
   if (scale < 1)
      scale = 1;
   for (; *text; text++) {
      unsigned char glyph = (unsigned char)*text;
      int j;
      int i;

      if (glyph < 0x20 || glyph > 0x7E) {
         cursor += FONT_WIDTH_STRIDE * scale;
         continue;
      }
      for (j = 0; j < FONT_HEIGHT; j++) {
         for (i = 0; i < FONT_WIDTH; i++) {
            unsigned bit = (unsigned)i + (unsigned)j * FONT_WIDTH;
            unsigned char mask = 1u << (bit & 7u);
            unsigned offset = bit >> 3;

            if (!(lcd_font[FONT_OFFSET(glyph) + offset] & mask))
               continue;
            if (scale == 1) {
               put_pixel(surface, cursor + i, y + j, color);
            } else {
               int sy;
               int sx;

               for (sy = 0; sy < scale; sy++)
                  for (sx = 0; sx < scale; sx++)
                     put_pixel(surface, cursor + i * scale + sx,
                        y + j * scale + sy, color);
            }
         }
      }
      cursor += FONT_WIDTH_STRIDE * scale;
      if (cursor >= (int)surface->width)
         break;
   }
}

int phobos_achievements_text_width_limit(const char *text, int max_px)
{
   int width;

   width = text ? (int)strlen(text) * FONT_WIDTH_STRIDE : 0;
   return width > max_px ? max_px : width;
}
