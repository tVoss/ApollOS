#define COOL(name, func)    \
.globl name                         \
name:                               \
    pusha;                          \
    pushw %ds;                      \
    pushw %es;                      \
    pushw %fs;                      \
    pushw %gs;                      \
    movw $0x10, %ax;                \
    movw %ax, %ds;                  \
    movw %ax, %es;                  \
    movw %ax, %fs;                  \
    movw %ax, %gs;                  \
    movl %esp, %eax;                \
    pushl %eax;                     \
    movl func, %eax;                \
    call *%eax;                      \
    popl %eax;                      \
    popw %gs;                       \
    popw %fs;                       \
    popw %es;                       \
    popw %ds;                       \
    popa;                           \
    add $8, %esp;                   \
    iret;                           \

COOL(handle_rtc, rtc_handler);
