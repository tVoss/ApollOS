#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "lib.h"
#include "terminal.h"

void pit_init(void);
void pit_handler(void);
/*
uint32_t scheduler_c(void);
void schedule_new(uint32_t pid);
void unschedule_old(uint32_t pid);
*/
#endif
