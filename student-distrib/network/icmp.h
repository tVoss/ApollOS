#ifndef ICMP_H_
#define ICMP_H_

#include "../types.h"

#define MAX_ICMP_DATA_SIZE      1460
#define MAX_ICMP_PACKET_SIZE    1480

int32_t icmp_send_data(uint8_t *data, uint16_t nbytes);

#endif
