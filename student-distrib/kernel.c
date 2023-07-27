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

#define RUN_TESTS


extern int syscall_handler();
extern int timer_handler();

/*
 * The only difference between trap and interrupt is that
 * interrupt gate will disable interrupt auto and trap gate will not.
 */
#define TRAP_GATE 15
#define INTR_GATE 14
#define SYSTEM_GATE TRAP_GATE

#define KERNEL_RPL 0
#define USER_RPL   3

#define APIC_LOCAL_TIMER_ONESHOT_MODE  (0)
#define APIC_LOCAL_TIMER_PERIODIC_MODE (1 << 17)
#define APIC_LOCAL_TIMER_TSCDDL_MODE   (2 << 17)
#define APIC_LOCAL_TIMER_DELIVERT_IDLE (0)
#define APIC_LOCAL_TIMER_DELIVERT_PENDING (1 << 11)

#define LOCAL_APIC_TIMER 0xbf

static void _set_gate(idt_desc_t *gate, int type, int dpl, void *addr)
{
    memset(gate, 0, sizeof(*gate));
    gate->dpl = dpl;
    gate->present = 1;
    gate->type = type;
    gate->seg_selector = KERNEL_CS;
    SET_IDT_ENTRY(*gate, addr);
}

static void set_trap_gate(unsigned int n, void *addr)
{
    _set_gate(&idt[n], TRAP_GATE, KERNEL_RPL, addr);
}

static void set_intr_gate(unsigned int n, void *addr)
{
    _set_gate(&idt[n], INTR_GATE, KERNEL_RPL, addr);
}

static void set_system_gate(unsigned int n, void *addr)
{
    _set_gate(&idt[n], SYSTEM_GATE, USER_RPL, addr);
}

void setup_idt()
{
    /* 0-31 is reserved, see "EXCEPTION AND INTERRUPT VECTORS" in intel manual volume2 */
    lidt(idt_desc_ptr);
    printf("done\n");
}

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

    setup_idt();

    /* Initialize devices, memory, filesystem, enable device interrupts on the
     * PIC, any other initialization stuff... */

    /* Enable interrupts */
    /* Do not enable the following until after you have set up your
     * IDT correctly otherwise QEMU will triple fault and simple close
     * without showing you any output */
    /*printf("Enabling Interrupts\n");
    sti();*/

#ifdef RUN_TESTS
    /* Run tests */
    launch_tests();
#endif
    /* Execute the first program ("shell") ... */

    /* Spin (nicely, so we don't chew up cycles) */
    asm volatile (".1: hlt; jmp .1;");
}
