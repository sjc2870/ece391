target remote 127.0.0.1:1234
layout split
set pagination off

b start
b entry
b __panic
b intr0xD_handler
b init_sched
b user0
b timer_handler
