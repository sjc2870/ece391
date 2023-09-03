#include "mm.h"
#include "lib.h"
#include "errno.h"
#include "multiboot.h"

extern int __text_start;
extern int __text_end;
extern int __data_start;
extern int __data_end;
extern int __bss_start;
extern int __bss_end;
extern int __kernel_start;
extern int __kernel_end;

/*
 * @reference:
 *  1. Chapter 4 intel manual volume 3
 */

/*
 * @NOTE 1:
 *  对于内核数据(是指__kernel_start~__kernel_end之间的内存，不包含内核栈)，需要有两个不同的虚拟地址映射到同一物理地址。
 *  这是因为1. 为了确保在开启paging后，内核的指令还能继续执行 2. 在跳转到高地址后，内核的指令还能继续执行
 *  对于STACK_TOP到STACK_BOTTOM之间的内存来说，只需要一次映射，这是因为栈并不需要跳转到高地址
 * @NOTE 2:
 *  According to Chapter 4.1.1 intel manual volume 3, there are four paging modes, we use the first mode (32-bit paging),
 *  which means that CR0.PG=1 && CR4.PAE=0. We don't need PCID(which used for tlb cache between multi user processes)
 *  and protection key, and we doesn't support 48bit phy addr, so there is no need to use 4-level and 5-level paging.
 * @TODO: Currently we disabled 4MB page. Support it in future.
*/

/* @NOTE: about mem_bitmap
 *   Currently we use 16k memory used for memory bitmap, every bit represents PAGE_SIZE memory.
 *   mem_bitmap was stored in bss section, because uninited global variable was stored in bss section.
 * @TODO: bitmap has heavy external memory fragment problem, use buddy system to resolve it.
 * @WARN: when memory is larger than 512MB, SLOTS MUST be larger, otherwise unexpected memory overwritten
          will happen.
 */
uint32_t mem_bitmap[SLOTS];
uint64_t phy_mem_base;
uint64_t phy_mem_len;

/* only alloc 4k size memory, used for alloc page table */
void* alloc_pgdir()
{
    /* called in init stage, so doesn't need lock */
    int i, j;
    int nr_slots = ((phy_mem_len / PAGE_SIZE) + 32)/32;
    for (i = 0; i < nr_slots; ++i) {
        /* every bit in uint32 */
        for (j = 0; j < 32; ++j) {
            /* every uint32 represents 32 bit, which means 32 pages */
            uint32_t cur_addr = PAGE_SIZE * (i*32 + j) + phy_mem_base;
            /*
             * See if bit (32-j-1) was set. Cause we set bit from up to down.
             * For example: j=0 represents bit 31, j=1 represents bit 30 ... j=31 represents bit 0
             */
            if (!(mem_bitmap[i] & (1 << (32-j-1)))) {
                mem_bitmap[i] |= (1 << (32-j-1));
                return (void*)cur_addr;
            }
        }
    }

}

int page_bitmap_init(unsigned long addr)
{
    multiboot_info_t *mbi = (multiboot_info_t*)addr;
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
            printf("    base_addr = 0x%#x%#x, length = 0x%#x%#x\n",
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
    printf("Memory base is %llx, size is %llx, there are %u pages, used %u slots\n",
           phy_mem_base, phy_mem_len, nr_pages, nr_slots);
    printf("bss start is %lx, bss end is %lx\n", (unsigned long)&__bss_start, (unsigned long)&__bss_end);
    printf("kernel start is %lx, kernel end is %lx\n", (unsigned long)&__kernel_start, (unsigned long)&__kernel_end);

    if (nr_slots >= SLOTS) {
        /*
         * qemu -m $memory is too large, or SLOTS was defined too small
         * Ajust one of them, otherwise will happen unexpected memory overwritten.
         */
        KERN_INFO("BUG: error memory\n");
        return -EINVAL;
    }

    /* every uint32 in mem_bitmap array */
    for (i = 0; i < nr_slots; ++i) {
        /* every bit in uint32 */
        for (j = 0; j < 32; ++j) {
            unsigned long cur_addr = PAGE_SIZE * (i*32 + j) + phy_mem_base;
            /* mark kernel memory as used */
            if (cur_addr >= (unsigned long)&__kernel_start && cur_addr < (unsigned long)&__kernel_end) {
                // printf("cur addr %x is in kernel region, continue\n", cur_addr);
                continue;
            }
            /* mark kernel stack as used */
            if (cur_addr >= STACK_BOTTOM && cur_addr < STACK_TOP) {
                // printf("cur addr %x is in stack region, continue\n", cur_addr);
                continue;
            }
            /* We clear bit from up to down. */
            mem_bitmap[i] &= ~(1 << (32-j-1));
        }
    }
    /* every uint32 in mem_bitmap array */
    for (i = 0; i < nr_slots; ++i) {
        /* every bit in uint32 */
        for (j = 0; j < 32; ++j) {
            /* every uint32 represents 32 bit, which means 32 pages */
            unsigned long cur_addr = PAGE_SIZE * (i*32 + j) + phy_mem_base;
            /*
             * See if bit (32-j-1) was set. Cause we set bit from up to down.
             * For example: j=0 represents bit 31, j=1 represents bit 30 ... j=31 represents bit 0
             */
            if (mem_bitmap[i] & (1 << (32-j-1))) {
                printf("address %x is used\n", cur_addr);
            }
        }
    }

    return 0;
}

int page_table_init()
{
    uint32_t *pgtbl_dir = alloc_pgdir();
    pgtbl_dir[0] = 1;

    return 0;
}

int init_paging(unsigned long addr)
{
    int ret = 0;

    if ((ret = page_bitmap_init(addr))) {
        return ret;
    }

    if ((ret = page_table_init()))
        return ret;

    return ret;
}

void * kmalloc(uint32_t size)
{
    uint32_t pages = (size + PAGE_SIZE)/PAGE_SIZE;
    unsigned long ret_addr = 0;
    /* lock */

    /* unlock */
}

void kfree()
{
    /* lock */
    /* unlock */
}

void enable_paging()
{
    /*
    * As we said in the comments(NOTE2) at the beginning of this file, we use 32-bit paging mode
    * About the details of how to enable paging, see chapher 4.1.2(Paging-mode Enabling) intel manual volume 3.
    */
}