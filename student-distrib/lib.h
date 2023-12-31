/* lib.h - Defines for useful library functions
 * vim:ts=4 noexpandtab
 */

#ifndef _LIB_H
#define _LIB_H

#include "types.h"

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))

int32_t printf(int8_t *format, ...);
uint32_t mprintf(char *fmt, ...);
void putc(uint8_t c);
int32_t puts(int8_t *s);
int8_t itoa(uint32_t value, int8_t* buf, int32_t radix);
int8_t itollu(uint64_t value, int8_t* buf, int32_t radix);
int8_t *strrev(int8_t* s);
uint32_t strlen(const int8_t* s);
void clear(void);


int set_bits(int num, int n, int m);
int clear_bits(int num, int n, int m);
uint32_t get_bits(uint32_t num, int n, int m);
void __panic(int8_t *format, ...);

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define KERN_INFO(format, args...)                      \
do {                                                    \
    printf("func: %s file: %s line: %d " format,        \
           __func__, __FILE__, __LINE__, ## args);      \
} while(0)

#define panic(fmt, args...) \
do {                        \
    printf("############  PANIC  ############\n");  \
    printf("    ");                                 \
    KERN_INFO("\n");   \
    __panic(fmt, ## args);                  \
} while(0)

#define panic_on(cond, fmt, args...)\
do {                                \
    if (unlikely(cond))             \
        panic(fmt, ## args);        \
} while(0)

void* memset(void* s, int32_t c, uint32_t n);
void* memset_word(void* s, int32_t c, uint32_t n);
void* memset_dword(void* s, int32_t c, uint32_t n);
void* memcpy(void* dest, const void* src, uint32_t n);
void* memmove(void* dest, const void* src, uint32_t n);
int memcmp(const void *s1, const void *s2, size_t n);
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n);
int8_t* strcpy(int8_t* dest, const int8_t*src);
int8_t* strncpy(int8_t* dest, const int8_t*src, uint32_t n);

/* Userspace address-check functions */
int32_t bad_userspace_addr(const void* addr, int32_t len);
int32_t safe_strncpy(int8_t* dest, const int8_t* src, int32_t n);

/* Port read functions */
/* Inb reads a byte and returns its value as a zero-extended 32-bit
 * unsigned int */
static inline uint32_t inb(uint16_t port) {
    uint32_t val;
    asm volatile ("             \n\
            xorl %0, %0         \n\
            inb  (%w1), %b0     \n\
            "
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Reads two bytes from two consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them zero-extended
 * */
static inline uint32_t inw(uint16_t port) {
    uint32_t val;
    asm volatile ("             \n\
            xorl %0, %0         \n\
            inw  (%w1), %w0     \n\
            "
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Reads four bytes from four consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them */
static inline uint32_t inl(uint16_t port) {
    uint32_t val;
    asm volatile ("inl (%w1), %0"
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/*
 * Sends a 8-bit value on a I/O location.
 * The a modifier enforces val to be placed in the eax register before the asm command is issued
 * and Nd allows for one-byte constant values to be assembled as constants,
 * freeing the edx register for other cases.
 */
#define outb(data, port)                \
do {                                    \
    asm volatile ("outb %b1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Writes two bytes to two consecutive ports */
#define outw(data, port)                \
do {                                    \
    asm volatile ("outw %w1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Writes four bytes to four consecutive ports */
#define outl(data, port)                \
do {                                    \
    asm volatile ("outl %l1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Clear interrupt flag - disables interrupts on this processor */
#define cli()                           \
do {                                    \
    asm volatile ("cli"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Save flags and then clear interrupt flag
 * Saves the EFLAGS register into the variable "flags", and then
 * disables interrupts on this processor */
#define cli_and_save(flags)             \
do {                                    \
    asm volatile ("                   \n\
            pushfl                    \n\
            popl %0                   \n\
            cli                       \n\
            "                           \
            : "=r"(flags)               \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Set interrupt flag - enable interrupts on this processor */
#define sti()                           \
do {                                    \
    asm volatile ("sti"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Restore flags
 * Puts the value in "flags" into the EFLAGS register.  Most often used
 * after a cli_and_save_flags(flags) */
#define restore_flags(flags)            \
do {                                    \
    asm volatile ("                   \n\
            pushl %0                  \n\
            popfl                     \n\
            "                           \
            :                           \
            : "r"(flags)                \
            : "memory", "cc"            \
    );                                  \
} while (0)

static inline void cpuid(uint32_t op, uint32_t regs[4])
{
	asm volatile("cpuid"
	    : "=a" (regs[0]),				// output
	      "=b" (regs[1]),
	      "=c" (regs[2]),
	      "=d" (regs[3])
	    : "a" (op), "c" (0)	// input
	    : "memory");
}

/*
 * Wait a very small amount of time (1 to 4 microseconds, generally).
 * Useful for implementing a small delay for PIC remapping on old hardware or generally as a simple but imprecise wait.
 * You can do an IO operation on any unused port: the Linux kernel by default uses port 0x80,
 * which is often used during POST to log information on the motherboard's hex display but almost always unused after boot.
 */
static inline void io_delay(void)
{
    outb(0, 0x80);
}


static inline void outb_d(uint8_t val, uint16_t port)
{
    outb(val, port);
    io_delay();
}
#endif /* _LIB_H */
