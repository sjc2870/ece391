#include "tasks.h"
#include "lib.h"
#include "x86_desc.h"
#include "mm.h"

extern void user0();
extern void *user_stk0;
extern void user1();
extern void *user_stk1;
extern void first_return_to_user();

struct task_struct *task0, *task1;

void __init_task(struct task_struct *task, unsigned long eip, unsigned long user_stack, unsigned long kernel_stack)
{
    memset(task, 0, sizeof(struct task_struct));

    task->cpu_state.cs = USER_CS;
    task->cpu_state.ds = USER_DS;
    task->cpu_state.es = USER_DS;
    task->cpu_state.es = USER_DS;
    task->cpu_state.fs = USER_DS;
    task->cpu_state.gs = USER_DS;
    task->cpu_state.eip = eip;
    task->cpu_state.esp = user_stack;
    task->cpu_state.esp0 = kernel_stack;
    task->state = TASK_RUNNABLE;
    task->parent = NULL;
}

void init_task(struct task_struct *task, unsigned long eip, unsigned long user_stack, unsigned long kernel_stack)
{
    unsigned long *kernel_stk = (unsigned long*)kernel_stack;

    memset(task, 0, sizeof(*task));

    /*
     * 这里可能不太容易看的懂，结合doc/sched.md一起看
     * 大概解释如下:因为是新进程，内核栈是空的。所以我们在switch_to时，不能走到lable 1处(因为lable 1的逻辑需要内核栈已经预先push了寄存器)。
     * 在这里，我们把eip设置为first_return_to_user，然后预先往内核栈中放入iret所需要的eip, esp。
     * 我们通过switch_to切换task时，通过jmp指令直接跳转到 first_return_to_user处，然后拿出预先放好的esp和eip，进行iret到用户态
     */
    __init_task(task, (unsigned long)first_return_to_user, user_stack, (unsigned long)kernel_stack);
    kernel_stk -= 2;
    /* push eip/esp that iret needed, see first_return_to_user */
    kernel_stk[0] = eip;
    kernel_stk[1] = user_stack;
}

static struct task_struct* alloc_task()
{
    return alloc_pages(1);
}

int init_sched()
{
    /* Construct a TSS entry in the GDT */
    seg_desc_t the_tss_desc = {0};
    the_tss_desc.granularity   = 0x0;
    the_tss_desc.opsize        = 0x0;
    the_tss_desc.reserved      = 0x0;
    the_tss_desc.avail         = 0x0;
    the_tss_desc.seg_lim_19_16 = 0xF;
    the_tss_desc.present       = 0x1;
    the_tss_desc.dpl           = 0x0;
    the_tss_desc.sys           = 0x0;
    the_tss_desc.type          = 0x9;
    the_tss_desc.seg_lim_15_00 = 0xFFFF;

    SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);
    tss_desc_ptr = the_tss_desc;

    task0 = alloc_task();
    task1 = alloc_task();
    if (!task0 || !task1) {
        panic("alloc task failed\n");
    }

    tss.esp0 = (unsigned long)(((char*)task0) + STACK_SIZE);  // stack top
    tss.ss0 = KERNEL_DS;
    tss.cs = KERNEL_CS;
    // tss.cr3 = (unsigned long)init_pgtbl_dir; Can't enable paging yet, see kernel.c line 116
    ltr(KERNEL_TSS);
    __init_task(task0, (unsigned long)user0, (unsigned long)&user_stk0, (unsigned long)(((char*)task0) + STACK_SIZE));
    init_task(task1, (unsigned long)user1, (unsigned long)&user_stk1, (unsigned long)(((char*)task1) + STACK_SIZE));
    asm volatile ("pushfl;"
                  "andl $0xffffbfff, %esp;" // clear busy flag
                  "popfl;"
                  "movl $user_stk0, %eax;"
                  "sti;"
                  "pushl $0x2B;"        // ss
                  "pushl %eax;"    // esp
                  "pushfl;"
                  "pushl $0x23;"        // cs
                  "pushl $user0;"        // eip 这里必须是$user0，而不能是user0.后者的话就变成了寻址，是压入user0地址处存储的数据，而前者是压入user0地址
                  "iret;"
    );

    return 0;
}


void new_kthread(unsigned long addr)
{
    struct task_struct *task = NULL;
    if (!task) {
        panic("failed to malloc memory\n");
        return;
    }

    task->cpu_state.ds = KERNEL_DS;
    task->cpu_state.fs = KERNEL_DS;
    task->cpu_state.gs = KERNEL_DS;
    task->cpu_state.es = KERNEL_DS;

    task->cpu_state.cs = KERNEL_CS;
    task->cpu_state.eip = addr;
    task->cpu_state.ss = KERNEL_DS;
    task->cpu_state.esp = (unsigned long)task + STACK_SIZE;

    task->cpu_state.eax = 0;
    task->cpu_state.ebx = 0;
    task->cpu_state.ecx = 0;
    task->cpu_state.edx = 0;
    task->cpu_state.esi = 0;
    task->cpu_state.edi = 0;
    task->cpu_state.ebp = 0;
}