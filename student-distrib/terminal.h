#ifndef TERMINAL_H_
#define TERMINAL_H_

#include "types.h"

/* terminal driver system calls */
extern int32_t terminal_open();
extern int32_t terminal_close();
extern int32_t terminal_read (int32_t fd, uint8_t *buf, int32_t nbytes);
extern int32_t terminal_write (int32_t fd, uint8_t *buf, int32_t nbytes);

#endif
