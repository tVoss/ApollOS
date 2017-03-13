#ifndef RTC_H_
#define RTC_H_

#include "idt.h"

extern void init_rtc();
extern void rtc_handler(regs_t regs);

#endif
