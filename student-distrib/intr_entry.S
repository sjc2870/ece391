#include "asm.h"
#include "intr.h"

.section .text

.global generic_intr_entry
.global ignore_intr
.global generic_intr_handler

# reference chapter 6.12.1 (Exception- or Interrupt-Handler Procedures) volume 3 intel manual
# After interruption or exception happened, there are 2 circumstances
# 1. No Privilege-Level Change, stack will be
#   EFLAGS
#   CS
#   EIP
#   Error Code    -< ESP
# 2. Privilege-Level Change, stack will be
#   SS
#   ESP
#   EFLAGS
#   CS
#   EIP
#   Error code    -< ESP


common_intr_entry:
    # pusha instruction pushes the contents of the general-purpose registers onto stack.
    pusha
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    pushl %esp
    pushl intr_num
    call generic_intr_handler
    # The above call instruction will overwrite esp regesiter, so we need restore esp using eax
    movl %eax, %esp

    popl %gs
    popl %fs
    popl %es
    popl %ds
    popa

    iret

ENTRY(ignore_intr):
    iret

.macro MAKE_INTR_HANDLER_WITHOUT_ERRCODE num
ENTRY(intr\num\()_handler):
    movl $\num, (intr_num)
    jmp common_intr_entry
.endm

.macro MAKE_INTR_HANDLER_WITH_ERRCODE num
ENTRY(intr\num\()_handler):
    movl $\num, (intr_num)
    jmp common_intr_entry
.endm

# intel predefined
# note: according to 6.4.2 volume3 intel manual,
#       can NOT use INT instruction to produce these predefined intr, for that will damage stack.
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x0
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x1
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x2
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x3
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x4
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x5
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x6
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x7
MAKE_INTR_HANDLER_WITH_ERRCODE    0x8
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x9
MAKE_INTR_HANDLER_WITH_ERRCODE    0xA
MAKE_INTR_HANDLER_WITH_ERRCODE    0xB
MAKE_INTR_HANDLER_WITH_ERRCODE    0xC
MAKE_INTR_HANDLER_WITH_ERRCODE    0xD
MAKE_INTR_HANDLER_WITH_ERRCODE    0xE
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0xF
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x10
MAKE_INTR_HANDLER_WITH_ERRCODE    0x11
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x12
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x13
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x14
MAKE_INTR_HANDLER_WITHOUT_ERRCODE 0x15

MAKE_INTR_HANDLER_WITHOUT_ERRCODE PIC_TIMER_INTR
MAKE_INTR_HANDLER_WITHOUT_ERRCODE PIC_KEYBOARD_INTR
MAKE_INTR_HANDLER_WITHOUT_ERRCODE PIC_HARDISK_INTR
MAKE_INTR_HANDLER_WITHOUT_ERRCODE PIC_SOFTDISK_INTR
MAKE_INTR_HANDLER_WITHOUT_ERRCODE PIC_PRINTER_INTR
MAKE_INTR_HANDLER_WITHOUT_ERRCODE PIC_RTC_INTR

MAKE_INTR_HANDLER_WITHOUT_ERRCODE SYSCALL_INTR

.data
    intr_num: .long 0