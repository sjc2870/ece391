#include "../types.h"
#include "../mm.h"
#include "../lib.h"

extern pgd_t* init_pgtbl_dir;

void test_paging()
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
        panic("BUG: paging error\n");
    }
}

void test_alloc_pages()
{
    void *p0, *p1, *p2;
    uint32_t stats1[MAX_ORDER] = {0};
    uint32_t stats2[MAX_ORDER] = {0};

    mm_show_statistics(stats1);
    p0 = alloc_page();
    clear();
    printf("after alloc 1 page 0x%x\n", p0);
    mm_show_statistics(stats2);

    p1 = alloc_pages(1);
    clear();
    printf("after alloc 2 page 0x%x\n", p1);
    mm_show_statistics(stats2);

    p2 = alloc_pages(2);
    clear();
    printf("after alloc 4 page 0x%x\n", p2);
    mm_show_statistics(stats2);

    clear();
    free_page(p0);
    mm_show_statistics(stats2);

    clear();
    free_pages(p1, 1);
    mm_show_statistics(stats2);

    clear();
    free_pages(p2, 2);
    mm_show_statistics(stats2);

    panic_on(memcmp(stats1, stats2, sizeof(stats1)), "buddy system error!\n");
}