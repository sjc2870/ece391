Like linux, we use software context switching also.
The principle is that we push new task's eip into stack, and jmp to a c function, when c function return, it will return to address that we pushed into stack.
```
1   #define switch_to(prev,next) do {
2      unsigned long eax, edx, ecx;
3      asm volatile("pushl %%ebx\n\t"
4                   "pushl %%esi\n\t"
5                   "pushl %%edi\n\t"
6                   "pushl %%ebp\n\t"
7                   "movl %%esp,%0\n\t" /* save ESP */
8                   "movl %5,%%esp\n\t" /* restore ESP */
9                   "movl $1f,%1\n\t"   /* save EIP */
10                  "pushl %6\n\t"      /* restore EIP */
11                 "jmp __switch_to\n"
12                  "1:\t"
13                  "popl %%ebp\n\t"
14                  "popl %%edi\n\t"
15                  "popl %%esi\n\t"
16                  "popl %%ebx"
17                  :"=m" (prev->tss.esp),"=m" (prev->tss.eip),
18                   "=a" (eax), "=d" (edx), "=c" (ecx)
19                  :"m" (next->tss.esp),"m" (next->tss.eip),
20                   "a" (prev), "d" (next));
21 } while (0)
```
In line 10, We push the new tast's eip into stack, and then jmp to __switch_to which is a c function.
When __switch_to return, it's already a new task, context switching has completed.

And according to intel manual, it's said that
    When the processor performs a call to the exception- or interrupt-handler procedure:
    If the handler procedure is going to be executed at a numerically lower privilege level, a stack switch occurs.
    When the stack switch occurs:
    a. The segment selector and stack pointer for the stack to be used by the handler are obtained from the TSS
        for the currently executing task. On this new stack, the processor pushes the stack segment selector and
        stack pointer of the interrupted procedure.
    b. The processor then saves the current state of the EFLAGS, CS, and EIP registers on the new stack (see
        Figure 6-4).
    c. If an exception causes an error code to be saved, it is pushed on the new stack after the EIP value

所以事情的全貌应该是：
    当用户陷入到内核态后，由于这是陷入到更高的特权级，所以会造成栈的切换，内核栈的ss sp指针来自于tss中保存的地址。
    在内核栈中，cpu会压入被中断进程的ss esp eflags cs eip [error code]