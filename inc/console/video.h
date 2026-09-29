#ifndef OS_VIDCON_DEF_H
#define OS_VIDCON_DEF_H

#include <stdint.h>

struct video_console
{
	uint32_t resolution_width;
	uint32_t resolution_height;
	uint32_t resolution_bpp;

	void (*clear_screen)(uint32_t color);
	void (*put_pixel)(uint32_t x, uint32_t y, uint32_t color);
	int (*initialize_screen)(void);
};

#endif
