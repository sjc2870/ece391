#ifndef _VGA_H
#define _VGA_H

#include "types.h"

#define NUM_COLS    80
#define NUM_ROWS    25

void get_console();
void set_console(uint32_t addr);
void set_cursor(u16 x, u16 y);
void get_cursor(u16 *x, u16 *y);
void console_init();
void reset_console();

#define VIDEO_MEM       0xB8000
#define NUM_COLS    80
#define NUM_ROWS    25
#define ATTRIB      0x7

#endif