#define THREAD_SIZE (2*PAGE_SIZE)

#define TASK_RUNNING 0
enum task_state {
    TASK_RUNNING = 0,
    TASK_INTERRUPTIBLE = 1,
    TASK_UNINTERRUPTIBLE = 2,
    TASK_ZOMBIE = 4,
    TASK_STOPPED = 8,
};

struct task_struct {
    volatile long state;
    pid_t pid;
    tss_t tss;
};