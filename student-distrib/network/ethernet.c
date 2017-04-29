#include "ethernet.h"

uint16_t create_crc(uint8_t *data, int32_t nbytes) {
    if (nbytes & 1) {
        // Odd number of bytes - do something *BAD*
        data[nbytes] = 0;
    }

    // Turn bytes into words
    // XXX endian-ness??
    uint16_t *data_words = (uint16_t *) data;
    uint32_t sum = 0;

    // Add all bytes
    int i;
    for (i = 0; i < nbytes / 2; i++) {
        sum += data_words[i];
    }

    // Keep it within 16 bits
    while (sum > 0xFFFF) {
        sum = ((sum & 0xFFFF0000) >> 16) + (sum & 0xFFFF);
    }

    // Binary inverse
    return ~((uint16_t) sum);
}

int32_t send_data(uint8_t *data, int32_t nbytes) {
    if (data == NULL || nbytes < MIN_DATA_SIZE || nbytes > MAX_DATA_SIZE) {
        // Invlid buffer or data
        return -1;
    }

    // Max frame size
    uint8_t frame[MAX_FRAME_SIZE] = {0};

    int i;
    for (i = 0; i < 6; i++) {
        frame[i] = 0xFF;    // Only send broadcast packets for now
        frame[i + 6] = 0xFF; // Only send from broadcast
    }

    // XXX not finished
    return -1;

}
