target remote 127.0.0.1:1234
b start
layout split
b entry
b i8259_init
b intr0x3C_handler
