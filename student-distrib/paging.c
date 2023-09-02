#include "paging.h"
#include "lib.h"

int test = 0;

extern void _start();
extern int intr_num;
extern int __text_start;
extern int __text_end;
extern int __data_start;
extern int __data_end;
extern int __bss_start;
extern int __bss_end;
int paging_init(unsigned long addr)
{
    multiboot_info_t *mbi = (multiboot_info_t*)addr;
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
        }
    }
    printf("stack var:%x global var: %x text var:%x data var:%x\n"
          "text start:%x text end:%x data start:%x data end:%x bss start:%x bss end:%x\n",
           &addr, &test, _start, &intr_num, &__text_start, &__text_end, &__data_start, &__data_end, &__bss_start, &__bss_end);
           // &addr, &test, _start, &intr_num);
    return 0;
}