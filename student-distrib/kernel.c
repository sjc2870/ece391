/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "tests.h"
#include "vga.h"
#include "intr.h"
#include "keyboard.h"

#define RUN_TESTS

extern int syscall_handler();
extern int timer_handler();

#define APIC_LOCAL_TIMER_ONESHOT_MODE  (0)
#define APIC_LOCAL_TIMER_PERIODIC_MODE (1 << 17)
#define APIC_LOCAL_TIMER_TSCDDL_MODE   (2 << 17)
#define APIC_LOCAL_TIMER_DELIVERT_IDLE (0)
#define APIC_LOCAL_TIMER_DELIVERT_PENDING (1 << 11)

#define LOCAL_APIC_TIMER 0xbf

static bool detect_apic()
{
        uint32_t regs[4] = {0};

        cpuid(1, regs);

        if (CHECK_FLAG(regs[3], 9)) {
            printf("APIC present\n");
            return true;
        } else {
            printf("WARNING APIC absent\n");
            return false;
        }
}

static uint8_t get_apic_id()
{
        uint32_t regs[4] = {0};

        cpuid(1, regs);

        if (CHECK_FLAG(regs[3], 9)) {
            printf("APIC present\n");
            return true;
        } else {
            printf("WARNING APIC absent\n");
            return false;
        }

        return (regs[3] >> 24);
}

static void self_test()
{
    asm volatile ("int $0x3");
}

extern void early_setup_idt();
extern void setup_idt();
void entry(unsigned long magic, unsigned long addr) {

    uint8_t apic_id = 0;

    console_init();
    /*
     * Check if MAGIC is valid and print the Multiboot information structure
     * pointed by ADDR.
     */
    multiboot_info(magic, addr);

    apic_id = get_apic_id();

    {
        char vendor[12];
        uint32_t regs[4];

        cpuid(0, regs);
        ((unsigned *)vendor)[0] = regs[1]; // EBX
        ((unsigned *)vendor)[1] = regs[3]; // EDX
        ((unsigned *)vendor)[2] = regs[2]; // ECX

        cpuid(1, regs);
        unsigned logical = (regs[1] >> 16) & 0xff;
        printf("there are %u logical cores\n", logical);
        cpuid(4, regs);
        uint32_t cores = ((regs[0] >> 26) & 0x3f) + 1;
        printf("there are %u physical cores\n", cores);
    }

    /* Construct an LDT entry in the GDT */
    /* {
        seg_desc_t the_ldt_desc;
        the_ldt_desc.granularity = 0x0;
        the_ldt_desc.opsize      = 0x1;
        the_ldt_desc.reserved    = 0x0;
        the_ldt_desc.avail       = 0x0;
        the_ldt_desc.present     = 0x1;
        the_ldt_desc.dpl         = 0x0;
        the_ldt_desc.sys         = 0x0;
        the_ldt_desc.type        = 0x2;

        SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
        ldt_desc_ptr = the_ldt_desc;
        lldt(KERNEL_LDT);
    } */

    /* Construct a TSS entry in the GDT */
    /* {
        seg_desc_t the_tss_desc;
        the_tss_desc.granularity   = 0x0;
        the_tss_desc.opsize        = 0x0;
        the_tss_desc.reserved      = 0x0;
        the_tss_desc.avail         = 0x0;
        the_tss_desc.seg_lim_19_16 = TSS_SIZE & 0x000F0000;
        the_tss_desc.present       = 0x1;
        the_tss_desc.dpl           = 0x0;
        the_tss_desc.sys           = 0x0;
        the_tss_desc.type          = 0x9;
        the_tss_desc.seg_lim_15_00 = TSS_SIZE & 0x0000FFFF;

        SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

        tss_desc_ptr = the_tss_desc;

        tss.ldt_segment_selector = KERNEL_LDT;
        tss.ss0 = KERNEL_DS;
        tss.esp0 = 0x800000;  // stack top 8MB
        ltr(KERNEL_TSS);
    } */

    if (detect_apic() == false)
        return;
    /* Init the PIC */
    i8259_init();
    early_setup_idt();
    clear();
    // init_8402_keyboard_mouse();
    sti();
    self_test();
    while (1) ;

    /* Initialize devices, memory, filesystem, enable device interrupts on the
     * PIC, any other initialization stuff... */

    /* Enable interrupts */
    /* Do not enable the following until after you have set up your
     * IDT correctly otherwise QEMU will triple fault and simple close
     * without showing you any output */
    /*printf("Enabling Interrupts\n");
    sti();*/

/* #ifdef RUN_TESTS
    Run tests
    launch_tests();
#endif */
    /* Execute the first program ("shell") ... */

    /* Spin (nicely, so we don't chew up cycles) */
    // asm volatile (".1: hlt; jmp .1;");
}
