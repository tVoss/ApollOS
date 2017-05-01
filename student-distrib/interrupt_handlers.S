#define ASM 1

# Macro for creating generic handler connection
#define MAKE_HANDLER(name, func)    \
.globl name;                        \
name:                               \
    pushal;                         \
    pushfl;                         \
    call func;                      \
    popfl;                          \
    popal;                          \
    sti;                            \
    iret;

MAKE_HANDLER(handle_rtc, rtc_handler);
MAKE_HANDLER(handle_keyboard, keyboard_handler);
MAKE_HANDLER(handle_pit, pit_handler);

syscalls:
    .long 0, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn

.globl handle_syscall
handle_syscall:
    pushfl          # Push flags to stack

    cmpl $1, %eax   # Test if syscall is a valid number
    jl bad_syscall
    cmpl $10, %eax
    jg bad_syscall

    pushl %ebx          # Push all registers to stack
    pushl %ecx
    pushl %edx
    pushl %ebp
    pushl %esi
    pushl %edi

    pushl %edx      # Manually push arguments
    pushl %ecx
    pushl %ebx

    sti             # Call function with interrupts on
    call *syscalls(, %eax, 4)
    cli

    addl $12, %esp  # Pop arguments

    popl %edi      # Restore registers
    popl %esi
    popl %ebp
    popl %edx
    popl %ecx
    popl %ebx
    popfl
    sti

    iret

bad_syscall:
    popfl           # Restore flags
    movl $-1, %eax  # Return error
    sti
    iret
