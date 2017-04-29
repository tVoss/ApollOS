#include "driver.h"

#include "../lib.h"

uint8_t bar_type;           // Type of BOR0
uint16_t io_base = 0;       // IO Base Address
uint16_t io_base_next = 4;  // IO + 4 for data
uint32_t mem_base;          // MMIO Base Address

uint8_t eeprom_exists;  // Flag if eeprom exists
uint8_t mac[6];         // MAC Address

// We have no malloc so must statically allocate space
e1000_rx_desc_t rx_descs[E1000_NUM_RX_DESC];
uint8_t rx_buffers[E1000_NUM_RX_DESC][8192 + 16];

e1000_tx_desc_t tx_descs[E1000_NUM_TX_DESC];

void write_cmd(uint16_t addr, uint32_t value) {
    if (bar_type == 0) {
        write32(value, mem_base + addr);
    } else {
        outl(addr, io_base);
        outl(value, io_base_next);
    }
}

uint32_t read_cmd(uint16_t addr) {
    if (bar_type == 0) {
        return read32(mem_base + addr);
    } else {
        outl(addr, io_base);
        return inl(io_base_next);
    }
}

uint8_t detect_eeprom() {
    uint32_t val = 0;
    write_cmd(REG_EEPROM, 0x1);

    int i;
    for (i = 0; i < 1000 && !eeprom_exists; i++) {
        val = read_cmd(REG_EEPROM);
        eeprom_exists = val & 0x10;
    }

    return eeprom_exists;
}

uint32_t read_eeprom(uint8_t addr) {
    uint16_t data = 0;
    if (eeprom_exists) {
        write_cmd(REG_EEPROM, 1 | ((uint32_t) addr) << 8);
        while(!((data = read_cmd(REG_EEPROM)) & 0x10));
    } else {
        write_cmd(REG_EEPROM, 1 | ((uint32_t) addr) << 2);
        while(!((data = read_cmd(REG_EEPROM)) & 0x02));
    }
    return (uint16_t)((data >> 16) & 0xFFFF);
}

uint8_t read_mac() {
    if (eeprom_exists) {
        uint32_t data = read_eeprom(0);
        mac[0] = data & 0xFF;
        mac[1] = data >> 8;
        data = read_eeprom(1);
        mac[2] = data & 0xFF;
        mac[3] = data >> 8;
        data = read_eeprom(2);
        mac[4] = data & 0xFF;
        mac[5] = data >> 8;
    } else {
        uint8_t *mem_base_mac_8 = (uint8_t *)(mem_base + 0x5400);
        uint32_t *mem_base_mac_32 = (uint32_t *)(mem_base + 0x5400);
        if (mem_base_mac_32[0] != 0) {
            int i;
            for (i = 0; i < 6; i++) {
                mac[i] = mem_base_mac_8[i];
            }
        } else {
            // Failure
            return 0;
        }
    }

    // Success
    return 1;
}

void init_rx() {
    int i;
    for (i = 0; i < E1000_NUM_RX_DESC; i++) {
        rx_descs[i].addr = (uint32_t) &rx_buffers[i];
        rx_descs[i].status = 0;
    }
}
