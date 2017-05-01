#include "ip.h"
#include "ethernet.h"

#include "../lib.h"

// Preset IP address
uint8_t ip[] = {192, 168, 10, 3};
uint8_t friend_ip[] = {192, 168, 10, 2};

int32_t ip_send_data(uint8_t *data, uint16_t nbytes) {
    if (data == NULL || nbytes > MAX_IP_DATA_SIZE) {
        return -1;
    }

    uint8_t packet[MAX_IP_PACKET_SIZE] = {0};

    // IPv4 with 20 byte header
    packet[0] = 0x45;

    // No special services
    packet[1] = 0;

    // Total length of packet
    uint16_t total_length = 20 + nbytes;
    packet[2] = (total_length & 0xFF00) >> 8;
    packet[3] = total_length & 0x00FF;

    // No identification
    packet[4] = 0;
    packet[5] = 0;

    // Don't fragment this packet
    packet[6] = 0x40;
    packet[7] = 0;

    // Average time to live
    packet[8] = 64;

    // ICPM Protocol to test
    packet[9] = 1;

    // Source address
    memcpy(&packet[12], ip, 4);
    // Destination
    memcpy(&packet[16], friend_ip, 4);

    uint16_t *words = (uint16_t *) packet;
    int i;
    uint32_t sum = 0;
    for (i = 0; i < 10; i++) {
        sum += words[i];
    }

    while (sum & 0xFFFF0000) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    words[5] = ~sum;

    memcpy(&packet[20], data, nbytes);

    return eth_send_data(packet, nbytes + 20);
}
