#ifndef IP_H_
#define IP_H_

#include "../types.h"

typedef struct ip_v4_addr {
    uint8_t octets[4];
} ip_v4_addr_t;

typedef struct ip_v6_addr {
    uint8_t hextets[8];
} ip_v6_addr_t;

#endif
