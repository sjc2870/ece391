#include "tasks.h"
#include "lib.h"
#include "liballoc.h"
#include "list.h"
#include "x86_desc.h"
#include "mm.h"

extern void user0();
extern void *user_stk0;
extern void user1();
extern void *user_stk1;
extern void user2();
extern void *user_stk2;
extern void first_return_to_user();

struct list running_tasks;
struct list runnable_tasks;  // waiting for time slice
struct list waiting_tasks;   // waiing for io or lock or something

static unsigned long next_pid = 0;

void __init_task(struct task_struct *task, unsigned long eip, unsigned long user_stack, unsigned long kernel_stack)
{
    memset(task, 0, sizeof(struct task_struct));

    INIT_LIST(&task->task_list);
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
    task->mm = alloc_page();
    panic_on(task->mm == NULL, "allocate mm failed\n");
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
    list_add_tail(&runnable_tasks, &task->task_list);
}

static struct task_struct* alloc_task()
{
    void* p = alloc_pages(1);
    panic_on((((unsigned long)p) % STACK_SIZE !=0), "struct task_struct must stack_size aligned\n");

    return p;
}

extern char init_finish;
int test_tasks()
{
    /* Construct a TSS entry in the GDT */
    seg_desc_t the_tss_desc = {0};
    struct task_struct *task0 = NULL;
    struct task_struct *task1 = NULL;
    struct task_struct *task2 = NULL;

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
    task2 = alloc_task();
    if (!task0 || !task1 || !task2) {
        panic("alloc task failed\n");
    }

    tss.esp0 = (unsigned long)(((char*)task0) + STACK_SIZE);  // stack top
    tss.ss0 = KERNEL_DS;
    tss.cs = KERNEL_CS;
    ltr(KERNEL_TSS);
    __init_task(task0, (unsigned long)user0, (unsigned long)&user_stk0, (unsigned long)(((char*)task0) + STACK_SIZE));
    init_task(task1, (unsigned long)user1, (unsigned long)&user_stk1, (unsigned long)(((char*)task1) + STACK_SIZE));
    init_task(task2, (unsigned long)user2, (unsigned long)&user_stk2, (unsigned long)(((char*)task2) + STACK_SIZE));
    tss.cr3 = (unsigned long)init_pgtbl_dir;
    list_add_tail(&running_tasks, &task0->task_list);
    init_finish = 1;
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

static pid_t get_pid()
{
    pid_t ret;

    cli();
    ret = next_pid++;
    sti();

    return ret;
}

void init_tasks()
{
    struct task_struct *task0 = alloc_task();
    INIT_LIST(&runnable_tasks);
    INIT_LIST(&waiting_tasks);
    INIT_LIST(&running_tasks);

    current()->mm->pgdir = init_pgtbl_dir;
    current()->state = TASK_RUNNING;
    current()->parent = NULL;
    current()->pid = get_pid();
    list_add_tail(&running_tasks, &current()->task_list);
    // current()->cpu_state.esp0 = alloc_page();
}

static int do_syscall_fork(struct task_struct *old, struct task_struct *new)
{
    int ret = 0;
    if (!old || !new) {
        panic("invalid args:%p %p\n", old, new);
    }

    new->pid = get_pid();
    new->parent = old;

    return ret;
}

void new_kthread(unsigned long addr)
{
    struct task_struct *task = alloc_task();
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