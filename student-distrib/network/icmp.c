#include "icmp.h"

#include "../lib.h"

#include "ip.h"
#include "checksum.h"

int32_t icmp_send_data(uint8_t *data, uint16_t nbytes) {
    if (data == NULL || nbytes > MAX_ICMP_DATA_SIZE) {
        return -1;
    }

    uint8_t packet[MAX_ICMP_PACKET_SIZE] = {0};

    // Ping request
    packet[0] = 8;
    packet[1] = 0;

    // Identifier
    packet[5] = 0x42;

    // Sequence num
    packet[7] = 1;

    // Payload
    memcpy(&packet[8], data, nbytes);

    uint16_t checksum = create_compliment(packet, nbytes + 8);
    packet[2] = checksum >> 8;
    packet[3] = checksum & 0xFF;

    return ip_send_data(packet, nbytes + 8);
}
