#include "intr.h"
#include "intr_def.h"
#include "types.h"
#include "x86_desc.h"
#include "i8259.h"
#include "lib.h"

struct intr_entry intr_entry[256];

static void _set_gate(idt_desc_t *gate, int type, int dpl, void *addr)
{
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

// If procedure is going to be executed at a numerically lower privilege level, a stack switch occurs
static void set_system_gate(unsigned int n, void *addr)
{
    _set_gate(&idt[n], SYSTEM_GATE, USER_RPL, addr);
}

/* devide error */
static void intr0x0_handler()
{
    printf("devide error occured\n");
}

/* debug exception */
static void intr0x1_handler()
{
    printf("debug exception occured\n");
}

/* none maskable interrupt */
static void intr0x2_handler()
{
    printf("nmi interrupt occured\n");
}

/* break point */
static void intr0x3_handler()
{
    printf("break point occured\n");
}

/* overflow */
static void intr0x4_handler()
{
    printf("overflow occured\n");
}

/* bound range exceeded */
static void intr0x5_handler()
{
    printf("bound range exceed occured\n");
}

/* undefined opcode */
static void intr0x6_handler()
{
    printf("undefined opcode occured\n");
}

/* device not available(no math coprocessor) */
static void intr0x7_handler()
{

    printf("device not available occured\n");
}

/* double fault, with error code(zero) */
static void intr0x8_handler()
{
    printf("double fault occured\n");
}

/* coprocessor segment overrun */
static void intr0x9_handler()
{
    printf("coprocessor segment overrun occured\n");

}
/* invalid tss, with error code  */
static void intr0xA_handler()
{
    printf("invalid tss occured\n");
}

/* segment not present, with error code */
static void intr0xB_handler()
{
    printf("segment not present occured\n");
}

/* stack segment fault, with error code */
static void intr0xC_handler()
{
    printf("stack segment fault occured\n");
}

/* general protection, with error code  */
static void intr0xD_handler()
{
    printf("general protection occured\n");
}

/* page fault, with error code  */
static void intr0xE_handler()
{
    printf("page fault occured\n");
}

/* x87 fpu floating-point error(Math fault) */
static void intr0x10_handler()
{
    printf("fpu floating-point error occured\n");
}

/* alignment check, with error code(zero) */
static void intr0x11_handler()
{
    printf("alignment check occured\n");
}

/* machine check */
static void intr0x12_handler()
{
    printf("machine check occured\n");
}

/* SIMD floating-point exception */
void intr0x13_handler()
{
    printf("SIML floating-point exception occured\n");
}

/* Virtualization Exception */
static void intr0x14_handler()
{
    printf("Virtualization Exception occured\n");
}

/*  Control Protection Exception, with error code  */
static void intr0x15_handler()
{
    printf("Control Protection Exception occured\n");
}

/* system call: SYSCALL_INTR */
static void intr0x80_handler()
{
    printf("system call occured\n");
}

/* APIC_MASTER_FIRST_INTR */
static void intr0x30_handler()
{
}

/* APIC_MASTER_FIRST_INTR +3 */
static void intr0x33_handler()
{
    printf("serial2 interrupt occured\n");
}

/* APIC_MASTER_FIRST_INTR +4 */
static void intr0x34_handler()
{
    printf("serial1 interrupt occured\n");
}

/* APIC_SLAVE_FIRST_INTR */
static void intr0x38_handler()
{
    printf("real time counter occured\n");
}

/* APIC_SLAVE_FIRST_INTR +4 */
static void intr0x3C_handler()
{
    printf("mouse interrupt occured\n");
}

/* APIC_SLAVE_FIRST_INTR +5 */
static void intr0x3D_handler()
{
    printf("math cooperation occured\n");
}

/* APIC_SLAVE_FIRST_INTR +6 */
static void intr0x3E_handler()
{
    printf("hardisk interrput occured\n");
}

void setup_intr_handler()
{
    memset(&intr_entry, 0, sizeof(intr_entry));

    SET_STATIC_INTR_HANDLER(0x0);
    SET_STATIC_INTR_HANDLER(0x1);
    SET_STATIC_INTR_HANDLER(0x2);
    SET_STATIC_INTR_HANDLER(0x3);
    SET_STATIC_INTR_HANDLER(0x4);
    SET_STATIC_INTR_HANDLER(0x5);
    SET_STATIC_INTR_HANDLER(0x6);
    SET_STATIC_INTR_HANDLER(0x7);
    SET_STATIC_INTR_HANDLER(0x8);
    SET_STATIC_INTR_HANDLER(0x9);
    SET_STATIC_INTR_HANDLER(0xA);
    SET_STATIC_INTR_HANDLER(0xB);
    SET_STATIC_INTR_HANDLER(0xC);
    SET_STATIC_INTR_HANDLER(0xD);
    SET_STATIC_INTR_HANDLER(0xE);
    // 0xF was reserved
    SET_STATIC_INTR_HANDLER(0x10);
    SET_STATIC_INTR_HANDLER(0x11);
    SET_STATIC_INTR_HANDLER(0x12);
    SET_STATIC_INTR_HANDLER(0x13);
    SET_STATIC_INTR_HANDLER(0x14);
    SET_STATIC_INTR_HANDLER(0x15);
    SET_STATIC_INTR_HANDLER(0x30);
    SET_EXTERN_INTR_HANDLER(0x31);
    SET_STATIC_INTR_HANDLER(0x80);
}

void early_setup_idt()
{
    struct x86_desc idt_desc =
    {
        .size = sizeof(idt)-1,
        .addr = (u32)&idt,
    };

    for (int i = 0; i < 256; ++i) {
        set_intr_gate(i, ignore_intr);
    }

    set_intr_gate(PIC_TIMER_INTR, intr0x30_entry);
    set_intr_gate(PIC_KEYBOARD_INTR, intr0x31_entry);
    asm volatile ("lidt %0" ::"m"(idt_desc));

    setup_intr_handler();

    enable_irq(PIC_KEYBOARD_INTR - PIC_MASTER_FIRST_INTR);
    enable_irq(PIC_TIMER_INTR - PIC_MASTER_FIRST_INTR);
}

unsigned long generic_intr_handler(unsigned long intr_num, unsigned long esp)
{
    if (intr_entry[intr_num].intr_handler)
        intr_entry[intr_num].intr_handler();
    else
        printf("unsupported intr 0x%x\n", intr_num);
    send_eoi(intr_num - PIC_MASTER_FIRST_INTR);

    return esp;
}