#ifndef _INTR_H
#define _INTR_H

#define PIC_MASTER_FIRST_INTR 0x30 // first intr vector in 8259 master
#define PIC_SLAVE_FIRST_INTR  (PIC_MASTER_FIRST_INTR + 8) // first intr vector in 8259 slave
#define PIC_MAX_INTR  (PIC_SLAVE_FIRST_INTR + 8)

#endif