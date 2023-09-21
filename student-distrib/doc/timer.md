#  There are many timer interrupt sources(RTC,PIT,HPET,local APIC timer), which one should we use?
    According to 'What's best? APIC timer or PIT?'(https://forum.osdev.org/viewtopic.php?f=1&t=10462),
    'So, what's best? Without doubt the local APIC timer is technically better, but when the OS can't use the local APIC it needs to use the PIT (or IRQ 8/RTC/CMOS).
    IMHO the best thing an OS can do is support both, and use the local APIC (instead of the PIT) if it can.'
    So, we use local APIC timer for timer interruption source, and support RTC to keep track of real time.
    And we use one-shot mode in local APIC timer, because that we can support nohz and always run only one user-level services.

# Reference
1. https://wiki.osdev.org/Timer_Interrupt_Sources
2. https://wiki.osdev.org/APIC_Timer
3. https://forum.osdev.org/viewtopic.php?f=1&t=10462