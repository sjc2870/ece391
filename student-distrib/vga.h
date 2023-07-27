#define NUM_COLS    80
#define NUM_ROWS    25

void get_console();
void set_console(uint32_t addr);
void set_cursor(u16 x, u16 y);
void get_cursor(u16 *x, u16 *y);
void console_init();