target remote 127.0.0.1:1234
b start
layout split
b entry
b i8259_init
b enable_irq
b self_test
b intr0x31_handler
