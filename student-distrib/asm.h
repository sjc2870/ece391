#ifndef _ASM_H
#define _ASM_H

#define ENTRY(sym)   \
.global sym;         \
.type sym @function; \
sym

#endif