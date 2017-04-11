#ifndef INTERRUPT_HANDLERS_H_
#define INTERRUPT_HANDLERS_H_

/* Handler for Real Time Clock interrupts */
void handle_rtc();

/* Handler for Keyboard interrupts */
void handle_keyboard();

/* Handler for Syscalls */
void handle_syscall();

#endif
