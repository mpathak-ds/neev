#ifndef OS_VIDCON_DEF_H
#define OS_VIDCON_DEF_H

#include <stdint.h>
#include <osdef.h>

#define VIDEO_COLOR_BLACK 0x0
#define VIDEO_COLOR_BLUE 0x1
#define VIDEO_COLOR_GREEN 0x2
#define VIDEO_COLOR_CYAN 0x3
#define VIDEO_COLOR_RED 0x4
#define VIDEO_COLOR_MAGENTA 0x5
#define VIDEO_COLOR_BROWN 0x6
#define VIDEO_COLOR_LIGHT_GREY 0x7
#define VIDEO_COLOR_DARK_GREY 0x8
#define VIDEO_COLOR_LIGHT_BLUE 0x9
#define VIDEO_COLOR_LIGHT_GREEN 0xA
#define VIDEO_COLOR_LIGHT_CYAN 0xB
#define VIDEO_COLOR_LIGHT_RED 0xC
#define VIDEO_COLOR_LIGHT_MAGENTA 0xD
#define VIDEO_COLOR_YELLOW 0xE
#define VIDEO_COLOR_WHITE 0xF

#define VIDEO_FONT_W 8
#define VIDEO_FONT_H 16

#define VIDEO_MAX_COLS 256
#define VIDEO_MAX_ROWS 128

#define VIDEO_ATTR(fg, bg) ((uint8_t)((((bg) & 0xF) << 4) | ((fg) & 0xF)))
#define VIDEO_ATTR_FG(a) ((a) & 0xF)
#define VIDEO_ATTR_BG(a) (((a) >> 4) & 0xF)
#define VIDEO_DEFAULT_ATTR VIDEO_ATTR(VIDEO_COLOR_WHITE, VIDEO_COLOR_BLACK)

struct video_console
{
	uint32_t resolution_width;
	uint32_t resolution_height;
	uint32_t resolution_bpp;

	void (*clear_screen)(uint32_t color);
	void (*put_pixel)(uint32_t x, uint32_t y, uint32_t color);
	int (*initialize_screen)(void);
};

os_status
video_console_init (
	struct video_console *video_info
	);

void
video_puts (
	const char *s
	);

void
video_putc (
	char c
	);

void
video_putc_attr (
	char c,
	uint8_t attr
	);

#endif
