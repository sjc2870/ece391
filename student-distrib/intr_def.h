#ifndef _INTR_DEF_H
#define _INTR_DEF_H
#include "types.h"
#include "intr.h"
#include "keyboard.h"
#include "mouse.h"
#include "timer.h"


typedef void (*intr_handler_t)();

struct intr_entry {
    intr_handler_t intr_handler;
};

extern struct intr_entry intr_entry[256];

#define SET_EXTERN_INTR_HANDLER(intr_num) \
    extern void intr ## intr_num ## _handler(); \
    intr_entry[intr_num].intr_handler = intr ## intr_num ## _handler;

#define SET_STATIC_INTR_HANDLER(intr_num) \
    intr_entry[intr_num].intr_handler = intr ## intr_num ## _handler;

extern void early_setup_idt();
extern void ignore_intr();
extern void intr0x30_entry();
extern void intr0x31_entry();
extern void intr0x3C_entry();


#endif
