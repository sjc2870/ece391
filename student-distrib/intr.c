#include "intr.h"
#include "types.h"
#include "x86_desc.h"
#include "i8259.h"
#include "lib.h"

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
static void do_de_intr()
{
    printf("devide error occured\n");
}

/* debug exception */
static void do_db_intr()
{
    printf("debug exception occured\n");
}

/* none maskable interrupt */
static void do_nmi_intr()
{
    printf("nmi interrupt occured\n");
}

/* break point */
static void do_bp_intr()
{
    printf("break point occured\n");
}

/* overflow */
static void do_of_intr()
{
    printf("overflow occured\n");
}

/* bound range exceeded */
static void do_br_intr()
{
    printf("bound range exceed occured\n");
}

/* undefined opcode */
static void do_ud_intr()
{
    printf("undefined opcode occured\n");
}

/* device not available(no math coprocessor) */
static void do_nm_intr()
{

    printf("device not available occured\n");
}

/* double fault, with error code(zero) */
static void do_df_intr()
{

    printf("double fault occured\n");
}

/* invalid tss, with error code  */
static void do_ts_intr()
{

    printf("invalid tss occured\n");
}

/* segment not present, with error code  */
static void do_np_intr()
{
    printf("segment not present occured\n");
}

/* stack segment fault, with error code  */
static void do_ss_intr()
{
    printf("stack segment fault occured\n");
}

/* general protection, with error code  */
static void do_gp_intr()
{
    printf("general protection occured\n");
}

/* page fault, with error code  */
static void do_pf_intr()
{
    printf("page fault occured\n");
}

/* x87 fpu floating-point error(Math fault) */
static void do_mf_intr()
{
    printf("fpu floating-point error occured\n");
}
/* alignment check, with error code(zero) */
static void do_ac()
{
    printf("alignment check occured\n");
}

/* machine check */
static void do_mc()
{
    printf("machine check occured\n");
}

/* SIMD floating-point exception */
static void do_xm()
{
    printf("SIML floating-point exception occured\n");
}

/* Virtualization Exception */
static void do_ve()
{
    printf("Virtualization Exception occured\n");
}

/*  Control Protection Exception, with error code  */
static void do_cp()
{
    printf("Control Protection Exception occured\n");
}

/* system call: SYSCALL_INTR */
static void do_syscall()
{
    printf("system call occured\n");
}

/* APIC_MASTER_FIRST_INTR */
static void do_timer()
{
    printf("timer interrupt occured\n");
}

/* APIC_MASTER_FIRST_INTR +1 */
static void do_keyboard()
{
    uint32_t v = inb(0x60);
    printf("keyboard interrupt occured\n");
}

/* APIC_MASTER_FIRST_INTR +3 */
static void do_serial2()
{
    printf("serial2 interrupt occured\n");
}

/* APIC_MASTER_FIRST_INTR +4 */
static void do_serial1()
{
    printf("serial1 interrupt occured\n");
}

/* APIC_SLAVE_FIRST_INTR */
static void do_rtc()
{
    printf("real time counter occured\n");
}

/* APIC_SLAVE_FIRST_INTR +4 */
static void do_mouse()
{
    printf("mouse interrupt occured\n");
}

/* APIC_SLAVE_FIRST_INTR +5 */
static void do_math_cooperation()
{
    printf("math cooperation occured\n");
}

/* APIC_SLAVE_FIRST_INTR +6 */
static void do_hardisk()
{
    printf("hardisk interrput occured\n");
}

void setup_idt()
{
    set_system_gate(PIC_KEYBOARD_INTR, do_keyboard);
    enable_irq(PIC_KEYBOARD_INTR - PIC_MASTER_FIRST_INTR);
}

extern void ignore_intr();
extern void generic_intr_entry();
extern void intr0x30_handler();
extern void intr0x31_handler();

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

    set_intr_gate(PIC_KEYBOARD_INTR, intr0x31_handler);
    set_intr_gate(PIC_TIMER_INTR, intr0x30_handler);
    asm volatile ("lidt %0" ::"m"(idt_desc));

    enable_irq(PIC_KEYBOARD_INTR - PIC_MASTER_FIRST_INTR);
    enable_irq(PIC_TIMER_INTR - PIC_MASTER_FIRST_INTR);
}

u32 generic_intr_handler(unsigned long intr_num, unsigned long esp)
{
    printf("interrupt 0x%x happened\n", intr_num);

    return esp;
}