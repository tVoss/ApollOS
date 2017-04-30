#include "driver.h"

#include "../lib.h"

// All constants for qemu
uint8_t bar_type = 0;           // Type of BAR0 (memory)
uint16_t io_base = 0xC000;      // IO Base Address
uint16_t io_base_next = 0xC004; // IO + 4 for data
uint32_t mem_base = 0xFEBC0000; // MMIO Base Address
uint8_t eeprom_exists = 1;      // Flag if eeprom exists

uint8_t mac[6];         // MAC Address

// We have no malloc so must statically allocate space
e1000_rx_desc_t rx_descs[E1000_NUM_RX_DESC];
uint8_t rx_buffers[E1000_NUM_RX_DESC][8192 + 16];
e1000_tx_desc_t tx_descs[E1000_NUM_TX_DESC];

// Current rx and tx index
uint16_t rx_cur;
uint16_t tx_cur;

// Write a command to the device
void write_cmd(uint16_t addr, uint32_t value) {
    if (bar_type == 0) {
        write32(value, mem_base + addr);
    } else {
        outl(addr, io_base);
        outl(value, io_base_next);
    }
}

// Read a command from the device
uint32_t read_cmd(uint16_t addr) {
    if (bar_type == 0) {
        return read32(mem_base + addr);
    } else {
        outl(addr, io_base);
        return inl(io_base_next);
    }
}

/* Read a value from eeprom */
uint32_t read_eeprom(uint8_t addr) {
    uint32_t data = 0;
    if (eeprom_exists) {
        write_cmd(REG_EEPROM, 1 | ((uint32_t) addr) << 8);
        while(!((data = read_cmd(REG_EEPROM)) & 0x10));
    } else {
        write_cmd(REG_EEPROM, 1 | ((uint32_t) addr) << 2);
        while(!((data = read_cmd(REG_EEPROM)) & 0x02));
    }
    return (uint16_t)((data >> 16) & 0xFFFF);
}

/* Get the mac address */
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

// Initialize rx desc's
void init_rx() {

    int i;

    // Set initial values
    for (i = 0; i < E1000_NUM_RX_DESC; i++) {
        rx_descs[i].addr = (uint32_t) &rx_buffers[i];
        rx_descs[i].status = 0;
    }

    // Set addresses for RX descs
    write_cmd(REG_RXDESCLO, (uint32_t) rx_descs);
    write_cmd(REG_RXDESCHI, 0);

    // Set size of rx descs
    write_cmd(REG_RXDESCLEN, E1000_NUM_RX_DESC * sizeof(e1000_rx_desc_t));

    // Set number of rx descs
    write_cmd(REG_RXDESCHEAD, 0);
    write_cmd(REG_RXDESCTAIL, E1000_NUM_RX_DESC - 1);

    // Set current rx to 0
    rx_cur = 0;

    // Init the R Ctrl
    write_cmd(REG_RCTRL,
        RCTL_EN         |
        RCTL_SBP        |
        RCTL_UPE        |
        RCTL_MPE        |
        RCTL_LBM_NONE   |
        RTCL_RDMTS_HALF |
        RCTL_BAM        |
        RCTL_SECRC      |
        RCTL_BSIZE_2048
    );
}

// Initialize the tx descs
void init_tx() {
    int i;

    // Set inital values
    for (i = 0; i < E1000_NUM_TX_DESC; i++) {
        tx_descs[i].addr = 0;
        tx_descs[i].cmd = 0;
        tx_descs[i].status = TSTA_DD;
    }

    // Write address to register
    write_cmd(REG_TXDESCLO, (uint32_t) tx_descs);
    write_cmd(REG_TXDESCHI, 0);

    // Set size of tx descs
    write_cmd(REG_TXDESCLEN, E1000_NUM_TX_DESC * sizeof(e1000_tx_desc_t));

    // Set number of tx descs
    write_cmd(REG_TXDESCHEAD, 0);
    write_cmd(REG_TXDESCTAIL, E1000_NUM_TX_DESC - 1);

    // Set current tx to 0
    tx_cur = 0;

    // Init the T Ctrl
    write_cmd(REG_TCTRL,
        TCTL_EN                 |
        TCTL_PSP                |
        (15 << TCTL_CT_SHIFT)   |
        (64 << TCTL_COLD_SHIFT) |
        TCTL_RTLC
    );

    write_cmd(REG_TCTRL, 0x3003F0FA);
    write_cmd(REG_TIPG, 0x0060200A);
}

void enable_interrupts() {
    // Magic numbers yay!
    write_cmd(REG_IMASK, 0x1F6DC);
    write_cmd(REG_IMASK, 0xFF & ~4);
    read_cmd(0xC0);
}

void start_link() {

}

void init_network() {
    read_mac();
    start_link()

    int i;
    // Clear out memory
    for (i = 0; i < 0x80; i++) {
        write_cmd(0x5200 + i * 4, 0);
    }

    enable_interrupts();
    init_rx();
    init_tx();
}

void handle_recieve() {
    uint16_t old_cur;

    while((rx_descs[rx_cur].status & 0x1)) {
        uint8_t *buf = (uint8_t *) (uint32_t) rx_descs[rx_cur].addr;
        uint16_t len = rx_descs[rx_cur].length;

        // Send to ethernet

        rx_descs[rx_cur].status = 0;
        old_cur = rx_cur;
        rx_cur = (rx_cur + 1) % E1000_NUM_RX_DESC;
        write_cmd(REG_RXDESCTAIL, old_cur);
    }
}

void network_handler() {
    uint32_t status = read_cmd(0xC0);
    if (status & 0x04) {
        // start_link
    } else if (status & 0x10) {
        // Nothing?
    } else if (status & 0x80) {

    }
}

int32_t send_packet(const uint8_t *data, uint16_t nbytes) {
    tx_descs[tx_cur].addr = (uint64_t) (uint32_t) data;
    tx_descs[tx_cur].length = nbytes;
    tx_descs[tx_cur].cmd = CMD_EOP | CMD_IFCS | CMD_RS | CMD_RPS;
    tx_descs[tx_cur].status = 0;

    uint8_t old_cur = tx_cur;
    tx_cur = (tx_cur + 1) % E1000_NUM_TX_DESC;
    write_cmd(REG_TXDESCTAIL, tx_cur);
    while(!(tx_descs[old_cur].status & 0xFF));

    return 0;
}
