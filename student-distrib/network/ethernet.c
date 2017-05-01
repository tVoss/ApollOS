#include "ethernet.h"

#include "../lib.h"
#include "../types.h"
#include "driver.h"

uint8_t data[] = {
    0x00, 0x0A, 0xE6, 0xF0, 0x05, 0xA3, 0x00, 0x12,
    0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x45, 0x00,
    0x00, 0x30, 0xB3, 0xFE, 0x00, 0x00, 0x80, 0x11,
    0x72, 0xBA, 0x0A, 0x00, 0x00, 0x03, 0x0A, 0x00,
    0x00, 0x02, 0x04, 0x00, 0x04, 0x00, 0x00, 0x1C,
    0x89, 0x4D, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
    0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13
};

uint32_t crc_table[] = {
    0x4DBDF21C, 0x500AE278, 0x76D3D2D4, 0x6B64C2B0,
    0x3B61B38C, 0x26D6A3E8, 0x000F9344, 0x1DB88320,
    0xA005713C, 0xBDB26158, 0x9B6B51F4, 0x86DC4190,
    0xD6D930AC, 0xCB6E20C8, 0xEDB71064, 0xF0000000
};

uint32_t create_crc(uint8_t *data, uint16_t nbytes) {
    uint32_t crc = 0;

    int i;
    for (i = 0; i < nbytes; i++) {
        crc = (crc >> 4) ^ crc_table[(crc ^ (data[i] >> 0)) & 0x0F];  /* lower nibble */
        crc = (crc >> 4) ^ crc_table[(crc ^ (data[i] >> 4)) & 0x0F];  /* upper nibble */
    }

    return crc;
}

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
