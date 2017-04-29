#ifndef ETHERNET_H_
#define ETHERNET_H_

#include "../types.h"
#include "ip.h"

#define MIN_DATA_SIZE   46
#define MAX_DATA_SIZE   1500
#define MAX_FRAME_SIZE  1518

int32_t send_data(uint8_t *data, int32_t nbytes);
int32_t recieve_data(uint8_t *data, int32_t nbytes);

#endif
