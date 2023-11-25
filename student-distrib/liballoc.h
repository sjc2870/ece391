#ifndef _LIBALLOC_H
#define _LIBALLOC_H

#include "types.h"
#include "lib.h"

typedef	unsigned long	ptr_t;

/** This function is supposed to lock the memory data structures. It
 * could be as simple as disabling interrupts or acquiring a spinlock.
 * It's up to you to decide.
 *
 * \return 0 if the lock was acquired successfully. Anything else is
 * failure.
 */
extern void liballoc_lock(unsigned long *flags);

/** This function unlocks what was previously locked by the liballoc_lock
 * function.  If it disabled interrupts, it enables interrupts. If it
 * had acquiried a spinlock, it releases the spinlock. etc.
 *
 * \return 0 if the lock was successfully released.
 */
extern void liballoc_unlock(unsigned long flag);

/** This is the hook into the local system which allocates pages. It
 * accepts an integer parameter which is the number of pages
 * required.  The page size was set up in the liballoc_init function.
 *
 * \return NULL if the pages were not allocated.
 * \return A pointer to the allocated memory.
 */
extern void* liballoc_alloc(size_t);

/** This frees previously allocated memory. The void* parameter passed
 * to the function is the exact same value returned from a previous
 * liballoc_alloc call.
 *
 * The integer value is the number of pages to free.
 *
 * \return 0 if the memory was successfully freed.
 */
extern void liballoc_free(void*,size_t);


extern void    *malloc(size_t);				///< The standard function.
extern void    *realloc(void *, size_t);		///< The standard function.
extern void    *calloc(size_t, size_t);		///< The standard function.
extern void     free(void *);					///< The standard function.

#endif