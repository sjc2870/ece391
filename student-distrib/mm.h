#ifndef _MM_h
#define _MM_h

#include "multiboot.h"
#include "types.h"

#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE-1)

#define SLOTS 4096*4

extern int init_paging(unsigned long addr);
extern void* kmalloc(uint32_t size);
extern void kfree();
extern void enable_paging();

typedef uint32_t pgd_t;
typedef uint32_t pde_t;
typedef uint32_t pte_t;

#define PRESENT_BIT 0
#define RW_BIT 1    // 0 read only, 1 read & write
#define US_BIT 2    // User/supervisor; if 0, user-mode accesses are not allowed to the 4-KByte page referenced by this entry
#define PWT_BIT 3   // Page-level write-through. Not used
#define PCD_BIT 4   // Page-level cache disable. Not used
#define ACCESS_BIT 5 // Accessed;indicates whether this entry has been used for linear-address translation
#define DIRTY_BIT 6  // Only used in pde. Dirty;
                    //  indicates whether software has written to the 4-KByte page referenced by this entry
#define PS_BIT 7     // determine that if there is a 4M huge page, we always set to 0, means we disable 4M page
#define GLOBAL_BIT 8 // global page. Not used

#endif