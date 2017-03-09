#ifndef IDT_H_
#define IDT_H_

#define INT_SYSCALL 0x80

extern void init_idt();

// Marco to create generic exception handler
#define CREATE_EXCEPTION(type, message) \
void type() {                           \
    printf("%s\n", #message);           \
    while(1);                           \
}

#endif
