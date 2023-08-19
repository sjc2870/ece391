extern int keyboard_init();
extern void intr0x31_handler();

#define intr_keyboard_handler intr0x31_handler