#ifndef _PAGING_h
#define _PAGING_h

#include "multiboot.h"
extern int paging_init(unsigned long addr);
#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE-1)

#endif