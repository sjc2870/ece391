#include "timer.h"
#include "i8259.h"
#include "intr.h"
#include "tasks.h"
#include "x86_desc.h"

static void __switch_to();

#define update_tss(task) tss.esp0 = (unsigned long)(((char*)task)+STACK_SIZE)

#define switch_to(cur,new) \
do  {   \
    asm volatile ("pusha;"           \
         "pushl %%ds;"       \
         "pushl %%es;"       \
         "pushl %%fs;"       \
         "pushl %%gs;"       \
         "sti;"              \
         "movl %%esp, %0;" /* save esp */     \
         "movl %2, %%esp;" /* restore esp */  \
         "movl $1f, %1;"  /* save eip */    \
         "jmp %3;"      /* restore eip */   \
         /* "pushl %3"
            "jmp __switch_to;"
            We cannot jmp __switch_to function, because the 'push %ebp; movl %esp %ebp' instrutions in function header
            will corrupt the new task's stack.
            So we use jmp directly
          */ \
         "1: popl %%gs;"    \
         "popl %%fs;"        \
         "popl %%es;"        \
         "popl %%ds;"        \
         "popa;"            \
        :"=m"(cur->cpu_state.esp0), "=m"(cur->cpu_state.eip)     \
        :"m"((new)->cpu_state.esp0), "m"((new)->cpu_state.eip) \
    );  \
} while(0)

void schedule()
{
    cli();
    if (current() == task0) {
        update_tss(task1);
        switch_to(task0, task1);
    } else if (current() == task1) {
        update_tss(task0);
        switch_to(task1, task0);
    }
}

void __switch_to()
{

}

void timer_handler(struct regs *cpu_state)
{
    send_eoi(PIC_TIMER_INTR);
    schedule();
}

int init_timer()
{
    /* @TODO: support local APIC timer */
    return 0;
}