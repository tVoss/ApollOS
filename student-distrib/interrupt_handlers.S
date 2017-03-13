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
