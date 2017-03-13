#ifndef IDT_H_
#define IDT_H_

#define INT_PIT         0x20
#define INT_KEYBOARD    0x21
#define INT_RTC         0x28
#define INT_SYSCALL     0x80

extern void init_idt();

// Marco to create generic exception handler
#define CREATE_EXCEPTION(type, message) \
void type() {                           \
    printf("%s\n", #message);           \
    while(1);                           \
}

#endif
