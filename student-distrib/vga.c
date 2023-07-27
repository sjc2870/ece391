#include "lib.h"
#include "vga.h"

#define MISC_OUTPUT_PORT_READ  (0x3cc)
#define MISC_OUTPUT_PORT_WRITE (0x3c2)

#define STATR_ADDRESS_IDX_H (0xc)
#define STATR_ADDRESS_IDX_L (0xd)

#define CURSOR_LOCATION_IDX_H (0xe)
#define CURSOR_LOCATION_IDX_L (0xf)

static inline uint8_t in_idx(uint16_t port, uint8_t idx)
{
    outb(idx, port);
    return inb(port+1);
}

static inline void out_idx(uint8_t v, uint16_t port, uint8_t idx)
{
    outb(idx, port);
    outb(v, port+1);
}

/* reference: http://www.osdever.net/FreeVGA/vga/extreg.htm#3CCR3C2W */
static inline uint16_t crtc_addr(void)
{
    return (inb(MISC_OUTPUT_PORT_READ) & 1) ? 0x03D4 : 0x03B4;
}

static inline uint16_t crtc_data(void)
{
    return (inb(MISC_OUTPUT_PORT_READ) & 1) ? 0x03D5 : 0x03B5;
}

void get_cursor(u16 *x, u16 *y)
{
    uint16_t cursor = 0;
    uint16_t crtc = crtc_addr();

    cursor = in_idx(crtc, CURSOR_LOCATION_IDX_L);
    cursor |= (in_idx(crtc, CURSOR_LOCATION_IDX_H) << 8);

    *x = cursor % NUM_COLS;
    *y = cursor / NUM_COLS;
}

void set_cursor(u16 x, u16 y)
{
    u16 crtc = crtc_addr();
    u16 cursor = NUM_COLS * y + x;

    out_idx(cursor >> 8, crtc, CURSOR_LOCATION_IDX_H);
    out_idx(cursor, crtc, CURSOR_LOCATION_IDX_L);
}

void get_console()
{
    uint32_t addr = 0;
    uint16_t crtc = crtc_addr();

    addr = in_idx(crtc, STATR_ADDRESS_IDX_L);
    addr |= (in_idx(crtc, STATR_ADDRESS_IDX_H) << 8);
}

void set_console(uint32_t addr)
{
    uint16_t crtc = crtc_addr();

    out_idx((addr - 0xB8000) >> 8, crtc, STATR_ADDRESS_IDX_H); // high
    out_idx((addr - 0xB8000), crtc, STATR_ADDRESS_IDX_L);      // low
}

void console_init()
{
    /* Clear the screen. */
    clear();
    set_cursor(0, 0);
}