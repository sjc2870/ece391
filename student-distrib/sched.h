#ifndef _SCHED_H
#define _SCHED_H
#include "x86_desc.h"

#define THREAD_SIZE (2*PAGE_SIZE)

typedef enum task_state {
    TASK_RUNNING = 0,
    TASK_INTERRUPTIBLE = 1,
    TASK_UNINTERRUPTIBLE = 2,
    TASK_ZOMBIE = 4,
    TASK_STOPPED = 8,
} task_state;

typedef unsigned long pid_t;

struct task_struct {
    volatile task_state state;
    pid_t pid;
    tss_t tss;
};

extern int init_sched();

#endif
