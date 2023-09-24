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
    在第一次从内核返回到用户态之前，会设置tss的ss sp指针，指向内核栈的栈顶。
    当用户陷入到内核态后，由于这是陷入到更高的特权级，所以会造成栈的切换，内核栈的ss sp指针来自于tss中保存的地址。
    在内核栈中，cpu会压入被中断进程的ss esp eflags cs eip [error code]

# multi-tasking workded
## 难点记录
    pre: 代码本身只是一堆二进制数据，并没有优先级之分。有优先级之分的是寄存器，也就代表了cpu当前运行代码的优先级
         所以，同一份代码，内核能跑，用户也能跑。只是跑代码时候的cs ss所代表的优先级切换了，只不过在用用户态的优先级在跑和内核优先级能跑的代码
    1. 首先是如何返回到用户态：只需要构造iret所需要的ss esp eflags cs eip，然后压入栈中，再执行iret指令即可
        如果没能按照预期返回用户态，
            1. 检查压入栈中的ss esp eflags cs eip是否符合预期。
            2. 检查ss selector和cs selector所选中的gdt descriptor的各个值是否符合预期，如dpl等等
    2. 新建task如何与context switching结合，从而可以正常轮询到新建task
        这里的难点在于：
        switch_to会pop寄存器，如果想要通过switch_to切换运行的task，那么task的内核栈中需要预先push了寄存器
        但是新建task的内核栈是空的，从而不能走switch_to后的pop寄存器的逻辑(绕过switch_to中的lable 1)
        那么如何解决呢？
        既然不能走lable 1的逻辑，那我们就新建一个逻辑，让新建task的第一次返回用户态直接iret回用户态。我们已经
        知道，如果想iret，需要预先往栈中push ss esp eflags cs eip，未知的是esp和eip，也就是用户态的栈和用户态的ip寄存器。
        那我们可以在新建task时，把这两个数据压入到内核栈中(kernel_stk -= 2)，然后在first_return_to_user时，把esp和eip给
        拿出来，然后构造iret所需要的栈结构。

## 实现要点记录
    1. 没有使用tss进行基于硬件的切换，而是使用基于软件的context-switching.
        原理是：手动保存寄存器到内核栈，然后保存内核栈的esp指针，该task被切换走再切换回来时，把该task的esp指针恢复，
        也就等于恢复了之前保存的寄存器。
    2.  使用tss进行context switching时，切换任务会导致自动切换内核栈。但是如果不使用tss，该如何切换内核栈呢？
        在用户态陷入内核时，cpu会检测到这是一次特权级改变的操作，会自动根据tss->esp0切换内核栈，并且会把用户态的ss esp eflags cs eip压入内核栈
    3. 新建task的第一次context switching时，会把用户态eip和esp保存到内核栈，然后jmp到first_return_to_user，从
        内核栈取出esp和eip，然后构造iret所需要的栈结构，进行iret到用户态

## Reference
    1. https://www.maizure.org/projects/evolution_x86_context_switch_linux/
    2. https://stackoverflow.com/questions/68946642/x86-hardware-software-tss-usage
    3. https://forum.osdev.org/viewtopic.php?t=13678
