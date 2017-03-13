#ifndef IDT_H_
#define IDT_H_

#define INT_PIT         0x20
#define INT_KEYBOARD    0x21
#define INT_RTC         0x28
#define INT_SYSCALL     0x80

extern void init_idt();

typedef struct regs {
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, error_code;
    unsigned int eip, cs, eflags, useresp, ss;
} regs_t;

// Marco to create generic exception handler
#define CREATE_EXCEPTION(type, message) \
void type() {                           \
    printf("%s\n", #message);           \
    while(1);                           \
}

#endif
