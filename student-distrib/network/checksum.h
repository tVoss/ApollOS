#ifndef CHECKSUM_H_
#define CHECKSUM_H_

#include "../types.h"

uint32_t create_crc(uint8_t *data, uint16_t nbytes);
uint16_t create_compliment(uint8_t *data, uint16_t nbytes);

#endif
