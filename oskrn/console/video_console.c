/*
 * NEEV Kernel
 * Path: console/video_console.c
 *
 * Copyright (c) 2026 Driftless Software. All rights reserved.
 * Property of Driftless Software.
 *
 * NOTICE: This software is governed by a license agreement. 
 * Redistribution, modification, or use of this file, in whole or in part,
 * is strictly restricted to the terms specified in the 'LICENSE' file 
 * located at the root directory of this project repository.
 *
 * Author: Mayank Pathak (mpathak)
 */

#include <stdint.h>
#include <adefs.h>
#include <console/serial.h>
#include <console/video.h>
#include <osdef.h>

#include "def_font.c"

struct video_console g_video;

// vga palette
static const uint32_t video_vga_rgb[16] = {
	0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
	0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
	0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
	0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
};

struct video_cell
{
	uint8_t ch;
	uint8_t attr;
};

struct video_shown
{
	uint8_t ch;
	uint8_t attr;
	uint8_t cursor;
};

static uint32_t video_cols, video_rows;
static uint32_t video_x0, video_y0;
static uint32_t video_cx, video_cy;
static uint8_t video_attr = VIDEO_DEFAULT_ATTR;
static int video_cursor_visible = 1;

static struct video_cell video_cells[VIDEO_MAX_ROWS][VIDEO_MAX_COLS];
static struct video_shown video_shown_buffer[VIDEO_MAX_ROWS][VIDEO_MAX_COLS];

static void
video_draw_cell (
	uint32_t col,
	uint32_t row,
	uint8_t ch,
	uint8_t attr,
	int cursor
	)
{
	const uint8_t *glyph = def_font_8x16[ch];
	uint32_t fg = VIDEO_ATTR_FG(attr);
	uint32_t bg = VIDEO_ATTR_BG(attr);
	uint32_t px = video_x0+col*VIDEO_FONT_W;
	uint32_t py = video_y0+row*VIDEO_FONT_H;

	for (uint32_t y=0; y<VIDEO_FONT_H; y++) {
		uint8_t bits = glyph[y];
		int underline = cursor && y >= (VIDEO_FONT_H-2);

		for (uint32_t x=0; x<VIDEO_FONT_W; x++) {
			int on = underline || (bits & (0x80>>x));
			g_video.put_pixel(px+x, py+y, on ? fg : bg);
		}
	}
}

static void
video_fill_row (
	uint32_t row,
	uint8_t attr
	)
{
	// iterate over all cells of said row and fill with a blank of said attr
	for (uint32_t c=0; c<video_cols; c++) {
		video_cells[row][c].ch = ' ';
		video_cells[row][c].attr = attr;
	}
}

static void
video_scroll (
	void
	)
{
	for (uint32_t r=1; r<video_rows; r++) {
		for (uint32_t c=0; c<video_cols; c++) {
			video_cells[r-1][c] = video_cells[r][c];
		}
	}

	video_fill_row(video_rows-1, video_attr);
}

static void
video_newline (
	void
	)
{
	video_cx = 0;
	if (video_cy++ >= video_rows) {
		video_cy = video_rows-1;
		video_scroll();
	}
}

void
video_flush (
	void
	)
{
	for (uint32_t r = 0; r < video_rows; r++) {
		for (uint32_t c = 0; c < video_cols; c++) {
			struct video_cell *cell = &video_cells[r][c];
			struct video_shown *sh = &video_shown_buffer[r][c];
			uint8_t cur = (video_cursor_visible && r == video_cy && c == video_cx);

			if (sh->ch != cell->ch || sh->attr != cell->attr || sh->cursor != cur) {
				video_draw_cell(c, r, cell->ch, cell->attr, cur);
				sh->ch = cell->ch;
				sh->attr = cell->attr;
				sh->cursor = cur;
			}
		}
	}
}

void
video_putc_attr (
	char c,
	uint8_t attr
	)
{
	switch (c) {
		case '\n':
			video_newline();
			return;

		case '\r':
			video_cx = 0;
			return;

		case '\b':
			if (video_cx>0) video_cx--;
			return;

		case '\t':
			video_cx = (video_cx+8) & ~7u;
			if (video_cx >= video_cols) video_newline();
			return;
	
		default:
			break;
	}

	video_cells[video_cy][video_cx].ch = (uint8_t)c;
	video_cells[video_cy][video_cx].attr = attr;

	if (video_cx++ >= video_cols) {
		video_newline();
	}
}

void
video_putc (
	char c
	)
{
	video_putc_attr(c, video_attr);
}

void
video_puts (
	const char *s
	)
{
	while (*s) {
		video_putc(*s++);
	}

	video_flush();
}

void
video_console_display_boot (
	struct video_console *video_info
	)
{
	uint32_t x0, y0;

	if (video_info->resolution_width < BOOT_LOGO_W ||
		video_info->resolution_height < BOOT_LOGO_H) {
		return;
	}
 
	x0 = (video_info->resolution_width-BOOT_LOGO_W) / 2;
	y0 = (video_info->resolution_height-BOOT_LOGO_H) / 2;

	for (uint32_t y=0; y<BOOT_LOGO_H; y++) {
		for (uint32_t x=0; x<BOOT_LOGO_W; x++) {
			uint32_t i = y*BOOT_LOGO_W+x;
			uint8_t packed;

			if (!(boot_logo_mask[i>>3] & (0x80>>(i&7)))) {
				continue;
			}

			packed = boot_logo_pixels[i>>1];
			video_info->put_pixel(x0+x, y0+y, VIDEO_COLOR_LIGHT_MAGENTA); // original was (i&1) ? (packed&0x0F) : (packed>>4)
		}
	}
}

os_status
video_console_init (
	struct video_console *video_info
	)
{
	//
	// Initialize and clear before setting, incase the firmware is faulty
	//

	if (video_info->initialize_screen() != STATUS_SUCCESS) {
		console_puts("init: Failed to initialize display\n");
		return STATUS_FAILED_DEVICE_INIT;
	}

	video_info->clear_screen(VIDEO_COLOR_WHITE);

	// display boot logo
	video_console_display_boot(video_info);

	g_video = *video_info;

	// setup video console and check for invalid param
	video_cols = g_video.resolution_width / VIDEO_FONT_W;
	video_rows = g_video.resolution_height / VIDEO_FONT_H;
	if (video_cols > VIDEO_MAX_COLS) video_cols = VIDEO_MAX_COLS;
	if (video_rows > VIDEO_MAX_ROWS) video_rows = VIDEO_MAX_ROWS;
	if (video_cols == 0 || video_rows == 0) {
		return STATUS_FAILED_INVALID_PARAMS;
	}

	video_x0 = (g_video.resolution_width-video_cols*VIDEO_FONT_W) / 2;
	video_y0 = (g_video.resolution_height-video_rows*VIDEO_FONT_H) / 2;
	video_cx = video_cy = 0;

	return STATUS_SUCCESS;
}
