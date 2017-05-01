#include "ethernet.h"

#include "../lib.h"
#include "../types.h"
#include "driver.h"
#include "crc.h"

int32_t eth_send_data(uint8_t *data, uint16_t nbytes) {
    if (data == NULL || nbytes > MAX_DATA_SIZE) {
        // Invlid buffer or data
        return -1;
    }

    // Max frame size
    uint8_t frame[MAX_FRAME_SIZE] = {0};

    // Destination
    memset(&frame, 0xFF, 6);
    // Source
    memcpy(&frame[6], get_mac_addr(), 6);

    // Size of data
    frame[12] = (uint8_t) (nbytes & 0xFF00) >> 8;
    frame[13] = (uint8_t) nbytes & 0x00FF;

    // Copy data
    memcpy(&frame[14], data, nbytes);

    // Add padding if needed
    if (nbytes < MIN_DATA_SIZE) {
        memset(&frame[14 + nbytes], 0, MIN_DATA_SIZE - nbytes);
        nbytes = MIN_DATA_SIZE;
    }

    // Generate CRC
    uint32_t crc = create_crc(frame, nbytes + 14);
    memcpy(&frame[14 + nbytes], &crc, 4);

    // Send
    return net_send_packet(frame, nbytes + 18);
}
