#ifndef _MM_h
#define _MM_h

#include "multiboot.h"
#include "types.h"

#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE-1)

#define SLOTS 4096

extern int init_paging(unsigned long addr);
extern void* kmalloc(uint32_t size);
extern void kfree();
extern void enable_paging();

typedef uint32_t pde_t;
typedef uint32_t pte_t;

#endif