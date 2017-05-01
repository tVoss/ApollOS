#ifndef ETHERNET_H_
#define ETHERNET_H_

#include "../types.h"
#include "ip.h"

#define CRC32_POLY      0xEDB88320

#define MIN_DATA_SIZE   46
#define MAX_DATA_SIZE   1500
#define MAX_FRAME_SIZE  1518

int32_t eth_send_data(uint8_t *data, uint16_t nbytes);
int32_t eth_recieve_data(uint8_t *data, uint16_t nbytes);

#endif
