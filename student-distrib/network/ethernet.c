#include "ethernet.h"

#include "../lib.h"
#include "../types.h"
#include "driver.h"
#include "checksum.h"

int32_t eth_send_data(uint8_t *data, uint16_t nbytes) {
    if (data == NULL || nbytes > MAX_ETH_DATA_SIZE) {
        // Invlid buffer or data
        return -1;
    }

    // Max frame size
    uint8_t frame[MAX_ETH_FRAME_SIZE] = {0};

    // Destination
    memset(&frame, 0xFF, 6);
    // Source
    memcpy(&frame[6], get_mac_addr(), 6);

    // Type of data (IPv4)
    frame[12] = 0x08;
    frame[13] = 0x00;

    // Copy data
    memcpy(&frame[14], data, nbytes);

    // Add padding if needed
    if (nbytes < MIN_ETH_DATA_SIZE) {
        memset(&frame[14 + nbytes], 0, MIN_ETH_DATA_SIZE - nbytes);
        nbytes = MIN_ETH_DATA_SIZE;
    }

    // Generate CRC
    uint32_t crc = create_crc(frame, nbytes + 14);
    memcpy(&frame[14 + nbytes], &crc, 4);

    // Send
    return net_send_data(frame, nbytes + 18);
}
