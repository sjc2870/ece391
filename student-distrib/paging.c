#include "paging.h"
#include "lib.h"
#include "errno.h"
#include "multiboot.h"
#include "types.h"

extern int __text_start;
extern int __text_end;
extern int __data_start;
extern int __data_end;
extern int __bss_start;
extern int __bss_end;
extern int __kernel_start;
extern int __kernel_end;

// 4k used for memory bitmap, every bit represents PAGE_SIZE memory
/* @TODO: bitmap has heavy external memory fragment problem, use buddy system to resolve it */
uint32_t mem_bitmap[1024];

int paging_init(unsigned long addr)
{
    multiboot_info_t *mbi = (multiboot_info_t*)addr;
    uint64_t phy_mem_base = 0;
    uint64_t phy_mem_len = 0;
    uint32_t nr_pages = 0;
    uint32_t nr_slots = 0;
    uint32_t i = 0, j = 0;
    if (CHECK_FLAG(mbi->flags, 6)) {
        memory_map_t *mmap;
        printf("phy memory:\n");
        for (mmap = (memory_map_t *)mbi->mmap_addr;
                (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
                mmap = (memory_map_t *)((unsigned long)mmap + mmap->size + sizeof (mmap->size))) {
            if (mmap->type != 1)
                continue;
            if (!mmap->base_addr_high && !mmap->base_addr_low)
                continue;
            printf("base_addr = 0x%#x%#x, length = 0x%#x%#x\n",
                    (unsigned)mmap->base_addr_high,
                    (unsigned)mmap->base_addr_low,
                    (unsigned)mmap->length_high,
                    (unsigned)mmap->length_low);
            phy_mem_base = (((unsigned long long)mmap->base_addr_high) << 32) | (unsigned)mmap->base_addr_low;
            phy_mem_len = (((unsigned long long)mmap->length_high) << 32) | (unsigned)mmap->length_low;
        }
    }

    if (!phy_mem_len) {
        KERN_INFO("ERROR: no usable memory\n");
        return -ENOMEM;
    }
    /* make memory size aligned to page_size*/
    phy_mem_len &= ~PAGE_MASK;
    /* mark all memory as used */
    memset(mem_bitmap, 0xffffffff, sizeof(mem_bitmap));

    nr_pages = phy_mem_len / PAGE_SIZE;
    nr_slots = (nr_pages+32)/32;
    printf("Memory size is %llx, there are %u pages, used %u slots\n", phy_mem_len, nr_pages, nr_slots);
    printf("bss start is %lx, bss end is %lx\n", (unsigned long)&__bss_start, (unsigned long)&__bss_end);
    printf("kernel start is %lx, kernel end is %lx\n", (unsigned long)&__kernel_start, (unsigned long)&__kernel_end);

    /* every uint32 in mem_bitmap array */
    for (i = 0; i < nr_slots; ++i) {
        /* every bit in uint32 */
        for (j = 0; j < 32; ++j) {
            uint32_t cur_addr = PAGE_SIZE * (i*32 + j) + phy_mem_base;
            /* mark kernel memory as used */
            if (cur_addr >= (unsigned long)&__kernel_start && cur_addr < (unsigned long)&__kernel_end) {
                printf("cur addr %x is in kernel region, continue\n", cur_addr);
                continue;
            }
            /* mark kernel stack as used */
            if (cur_addr >= STACK_BOTTOM && cur_addr < STACK_TOP) {
                printf("cur addr %x is in stack region, continue\n", cur_addr);
                continue;
            }
            /* We set bit from up to down. */
            mem_bitmap[i] &= ~(1 << (32-j-1));
        }
    }
    /* every uint32 in mem_bitmap array */
    for (i = 0; i < nr_slots; ++i) {
        /* every bit in uint32 */
        for (j = 0; j < 32; ++j) {
            /* every uint32 represents 32 bit, which means 32 pages */
            uint64_t cur_addr = PAGE_SIZE * (i*32 + j) + phy_mem_base;
            /*
             * See if bit (32-j-1) was set. Cause we set bit from up to down.
             * For example: j=0 represents bit 31, j=1 represents bit 30 ... j=31 represents bit 0
             */
            if (mem_bitmap[i] & (1 << (32-j-1))) {
                printf("address %llx is used\n", cur_addr);
            }
        }
    }

    return 0;
}