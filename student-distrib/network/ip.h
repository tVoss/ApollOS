#ifndef IP_H_
#define IP_H_

#include "../types.h"

#define MAX_IP_DATA_SIZE    1480
#define MAX_IP_PACKET_SIZE  1500

int32_t ip_send_data(uint8_t *data, uint16_t nbytes);

#endif
