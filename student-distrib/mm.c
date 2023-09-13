#include "mm.h"
#include "lib.h"
#include "errno.h"
#include "multiboot.h"
#include "types.h"
#include "vga.h"
#include "list.h"

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
pgd_t *init_pgtbl_dir;

#define MAX_ORDER 11    // max free list is 4M(4K * 2^(11-1))

static struct list free_pages_head[MAX_ORDER];

/* @NOTE: caller must hold mm lock */
#define ITERATE_PAGES(free_statements, used_statements)     \
    int __cur_slot, __cur_bit;                                         \
    int ___nr_slots = ((phy_mem_len / PAGE_SIZE) + 32)/32;  \
    for (__cur_slot = 0; __cur_slot < ___nr_slots; ++__cur_slot) {    \
        /* every bit in uint32 */                   \
        for (__cur_bit = 0; __cur_bit < 32; ++__cur_bit) {         \
            /*                                                                  \
             * See if bit (32-j-1) was set. Cause we set bit from up to down.   \
             * For example: j=0 represents bit 31, j=1 represents bit 30 ... j=31 represents bit 0  \
             */                                                                                     \
            if (!(mem_bitmap[__cur_slot] & (1 << (32-__cur_bit-1)))) {                                         \
                free_statements;                                                                    \
            } else {                                                                                \
                used_statements;                                                                    \
            }                                                                                       \
        }                                                                                           \
    }                                                                                               \


/* only alloc 4k size memory, used for alloc page table */
void* alloc_pgdir()
{
    ITERATE_PAGES({
        uint32_t cur_addr = PAGE_SIZE *(__cur_slot*32+__cur_bit)+phy_mem_base;
        mem_bitmap[__cur_slot] |= (1 << (32-__cur_bit-1));
        return (void*)cur_addr;
        }, {});

    KERN_INFO("failed to alloc pgdir\n");
    return NULL;
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
            if (cur_addr >= STACK_TOP && cur_addr < STACK_BOTTOM) {
                // printf("cur addr %x is in stack region, continue\n", cur_addr);
                continue;
            }
            /* We clear bit from up to down. */
            mem_bitmap[i] &= ~(1 << (32-j-1));
        }
    }
    ITERATE_PAGES({}, {
        unsigned long cur_addr = PAGE_SIZE * (__cur_slot*32 + __cur_bit) + phy_mem_base;
        printf("address %x is used\n", cur_addr);
    });

    return 0;
}

/* @NOTE: caller must hold mm lock */
int add_page_mapping(uint32_t linear_addr, uint32_t phy_addr)
{
    uint32_t pgd_offset = 0;
    uint32_t pde_offset = 0;
    uint32_t pte_offset = 0;
    pde_t pde;  // pde represents 4M size memory
    pte_t pte;  // pte represents 4K size memory


    pgd_offset = get_bits(linear_addr, 22, 31);
    pde_offset = get_bits(linear_addr, 12, 21);
    pte_offset = get_bits(linear_addr, 0, 11);

    pde = (uint32_t)init_pgtbl_dir[pgd_offset];
    if (!pde) {
        pde = (uint32_t)alloc_pgdir();
        pde |= (1 << PRESENT_BIT);
        pde |= (1 << RW_BIT);
        init_pgtbl_dir[pgd_offset] = (uint32_t)pde;
    }

    pde &= ~(PAGE_MASK);
    pte = ((uint32_t*)pde)[pde_offset];
    if (!pte) {
        pte = (uint32_t)(phy_addr & ~(PAGE_MASK));
        pte |= (1 << PRESENT_BIT);
        pte |= (1 << RW_BIT);
        ((uint32_t*)pde)[pde_offset] = pte;
    }
    return 0;
}

static void paging_test()
{
    uint32_t linear_addr = 0x5038fb;
    // uint32_t linear_addr = 0x503841;
    uint32_t pgd_offset = 0;
    uint32_t pde_offset = 0;
    uint32_t pte_offset = 0;
    pde_t pde;
    pte_t pte;
    uint32_t phy_addr = 0;

    pgd_offset = get_bits(linear_addr, 22, 31);
    pde_offset = get_bits(linear_addr, 12, 21);
    pte_offset = get_bits(linear_addr, 0, 11);

    pde = (uint32_t)*(init_pgtbl_dir + pgd_offset);
    pde &= ~(PAGE_MASK);
    pte = *(uint32_t*)(pde + pde_offset*4);
    pte &= ~(PAGE_MASK);
    // pte = ((uint32_t*)pde)[pde_offset];
    phy_addr = pte & ~(PAGE_MASK);
    phy_addr |= pte_offset;

    if (phy_addr != linear_addr) {
        printf("BUG: paging error\n");
    }
}

int page_table_init()
{
    /* A pgd_t pointer points to a page, which contains 1024 pde_t */
    init_pgtbl_dir = alloc_pgdir();
    memset(init_pgtbl_dir, 0, PAGE_SIZE);
    /*
     * We must ensure that
     * linear address 0x&__kenel_start map to phy addr 0x&_kernel_start
     *                0x(&__kernel_start + 4k) to phy addr 0x(&__kernel_start + 4k)
     *                ...
     *                0x(&__kernel_end) to phy addr 0x(&__kernel_end)
     */

    ITERATE_PAGES({}, {
       unsigned long cur_addr = PAGE_SIZE * (__cur_slot*32 + __cur_bit) + phy_mem_base;
       if (cur_addr >= (unsigned long)&__kernel_start)
           add_page_mapping(cur_addr, cur_addr);
    });
    add_page_mapping(VIDEO_MEM , VIDEO_MEM);

    paging_test();

    return 0;
}

static bool pages_is_free(unsigned long addr, uint8_t order)
{
    int cur_slot = 0;
    int cur_bit = 0;
    int nr_bits = 1 >> order;

    cur_slot = (addr - phy_mem_base)/PAGE_SIZE/32;
    cur_bit = ((addr - phy_mem_base)/PAGE_SIZE)%32;
    if (cur_bit >= 8) {
        panic("invalid bits %d\n", cur_bit);
    }

    if (cur_bit != 0 && nr_bits >= (8-cur_bit)) {
        // 先处理那些非byte对齐的bit
        int bs = 8 - cur_bit;
        char c = mem_bitmap[cur_slot];
    }

    if (nr_bits >= sizeof(uint64_t)) {
        nr_bits %= 64;

        return false;
    }

    if (nr_bits >= sizeof(uint32_t)) {
        nr_bits %= 32;

        return false;
    }

    if (nr_bits >= sizeof(uint16_t)) {
        nr_bits %= 16;

        return false;
    }

    if (nr_bits >= sizeof(uint8_t)) {
        nr_bits %= 8;

        return false;
    }

    if (nr_bits >= 0) {

        return false;
    }

    return true;
}

int init_free_pages_list()
{
    int i = 0;
    for (i = 0; i < MAX_ORDER; ++i) {
        INIT_LIST(&free_pages_head[i]);
    }

    ITERATE_PAGES({
        uint32_t cur_addr = PAGE_SIZE *(__cur_slot*32+__cur_bit)+phy_mem_base;
        INIT_LIST((struct list*)cur_addr);
        list_add_tail(&free_pages_head, (void*)cur_addr);
        return (void*)cur_addr;
        }, {});
}

int init_paging(unsigned long addr)
{
    int ret = 0;

    if ((ret = page_bitmap_init(addr))) {
        return ret;
    }

    if ((ret = init_free_pages_list())) {
        return ret;
    }

    if ((ret = page_table_init()))
        return ret;

    return ret;
}

void *kmalloc(uint32_t size)
{
    /* lock */
    ITERATE_PAGES({
        uint32_t cur_addr = PAGE_SIZE *(__cur_slot*32+__cur_bit)+phy_mem_base;
        mem_bitmap[__cur_slot] |= (1 << (32-__cur_bit-1));
        return (void*)cur_addr;
        }, {});
    return NULL;

    /* unlock */
}

void kfree()
{
    /* lock */
    /* unlock */
}

void enable_paging()
{
    uint32_t regs[4] = {0};// rax rbx rcx rdx

    /*
    * As we said in the comments(NOTE2) at the beginning of this file, we use 32-bit paging mode
    * About the details of how to enable paging, see chapter 4.1.2(Paging-mode Enabling) intel manual volume 3.
    */
    /* load init_pgtbl_dir to CR3 register */
    asm volatile (  "movl %0, %%cr3;"
                    "movl %%cr0, %%eax;"
                    "orl $0x80000000, %%eax;"
                    "movl %%eax, %%cr0;"
                    :  /* no output */
                    :"r"(init_pgtbl_dir)  /* input pgtable dir */
                    : "eax");

    /* set CR0.PG = 1 and CR4.PAE(disable PAE) = 0, CR4.PSE = 0(disable reserved bits) */

    /* After enable paging, do some sanity check. Chapter 4.1.4 */
    cpuid(1, regs);

    /* test stack memory paging */
    int a = 10;
    a += 100;
    /* test video memory paging */
    printf("a is %d\n", a);
}
