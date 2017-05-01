#include "driver.h"

#include "../i8259.h"
#include "../lib.h"

#include "../paging.h"

uint16_t mac[MAC_WORDS];         // MAC Address

// We have no malloc so must statically allocate space
// Recieve
e1000_rx_desc_t rx_descs[E1000_NUM_RX_DESC] __attribute__ ((aligned (16)));
packet_t rx_packets[E1000_NUM_RX_DESC];

// Transmit
e1000_tx_desc_t tx_descs[E1000_NUM_TX_DESC] __attribute__ ((aligned (16)));
packet_t tx_packets[E1000_NUM_TX_DESC];

// Write a command to the device
void write_cmd(uint32_t addr, uint32_t value) {
    write32(value, E1000_MEM_BASE + addr);
}

// Read a command from the device
uint32_t read_cmd(uint32_t addr) {
    return read32(E1000_MEM_BASE + addr);
}

/* Read a value from eeprom */
uint16_t read_eeprom(uint8_t addr) {
    uint32_t data = 0;
    write_cmd(REG_EEPROM, 1 | ((uint32_t) addr) << 8);
    while(!((data = read_cmd(REG_EEPROM)) & 0x10));
    return (uint16_t)((data >> 16) & 0xFFFF);
}

/* Get the mac address */
uint8_t read_mac() {
    int i;
    for (i = 0; i < MAC_WORDS; i++) {
        mac[i] = read_eeprom(i);
    }

    // Success
    return 1;
}

void enable_mastering() {
    outl(PCI_NET_ADDR, PCI_ADDR_PORT);
    outl(PCI_CMD_ENABLE, PCI_DATA_PORT);

    // XXX TEST
    outl(PCI_NET_ADDR, PCI_ADDR_PORT);
    uint32_t data = inl(PCI_DATA_PORT);
    printf("Read %x from PCI_NET_ADDR\n", data);
}

// Initialize Recieve
void init_rx() {
    // Set recieve address
    write_cmd(RECV_ADRL, mac[0] | (mac[1] << 16));
    write_cmd(RECV_ADRH, mac[2] | RAH_AV);

    // Clear out multicast table
    int i;
    for (i = 0; i < MTA_ELEMS; i++) {
        write_cmd(REG_MTA, 0);
    }

    // Set addresses for RX descs
    write_cmd(REG_RXDESCLO, (uint32_t) rx_descs);
    write_cmd(REG_RXDESCHI, 0);

    // Set size of rx descs
    write_cmd(REG_RXDESCLEN, E1000_NUM_RX_DESC * sizeof(e1000_rx_desc_t));

    // Set number of rx descs
    write_cmd(REG_RXDESCHEAD, 0);
    write_cmd(REG_RXDESCTAIL, E1000_NUM_RX_DESC - 1);

    // Clear out descs
    memset(rx_descs, 0, E1000_NUM_RX_DESC * sizeof(e1000_rx_desc_t));

    // Set initial values
    for (i = 0; i < E1000_NUM_RX_DESC; i++) {
        rx_descs[i].addr = (uint64_t) (uint32_t) &rx_descs[i];
    }

    // Init the R control register
    uint32_t cmd = read_cmd(REG_RCTRL);
    cmd |= RCTL_RXT0;
    cmd &= RCTL_LBM_NONE;
    cmd &= RCTL_BSIZE_2048;
    cmd |= RCTL_SECRC;
    cmd &= RCTL_LPE_NONE;
    cmd |= RCTL_EN;

    write_cmd(REG_RCTRL, cmd);
}

// Initialize Transmit
void init_tx() {
    // Write address to register
    write_cmd(REG_TXDESCLO, (uint32_t) tx_descs);
    write_cmd(REG_TXDESCHI, 0);

    // Set size of tx descs
    write_cmd(REG_TXDESCLEN, E1000_NUM_TX_DESC * sizeof(e1000_tx_desc_t));

    // Write 0 to head and tail
    write_cmd(REG_TXDESCHEAD, 0);
    write_cmd(REG_TXDESCTAIL, 0);

    // Init the T Ctrl
    write_cmd(REG_TCTRL,
        TCTL_EN     |
        TCTL_PSP    |
        TCTL_CT     |
        TCTL_COLD   |
        TCTL_RTLC
    );

    write_cmd(REG_TIPG,
        (0xA << E1000_TIPG_IPGT)    |
        (0x8 << E1000_TIPG_IPGR1)   |
        (0xC << E1000_TIPG_IPGR2)
    );

    // Clear out descs
    memset(tx_descs, 0, sizeof(e1000_rx_desc_t) * E1000_NUM_TX_DESC);

    // Set inital values
    int i;
    for (i = 0; i < E1000_NUM_TX_DESC; i++) {
        tx_descs[i].addr = (uint64_t) (uint32_t) &tx_packets[i];
        tx_descs[i].cmd |= CMD_RS;
        tx_descs[i].cmd &= ~CMD_DEXT;
        tx_descs[i].status = TSTA_DD;
    }
}

void enable_interrupts() {
    // Magic numbers yay!
    enable_irq(NET_IRQ_LINE);
    write_cmd(REG_IMASK, 0x1F6DC);
    write_cmd(REG_IMASK, 0xFF & ~4);
    read_cmd(REG_IREAD);
}

void init_network() {
    remap(E1000_PAGE_BASE, E1000_PAGE_BASE);

    read_mac();
    enable_mastering();

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
    uint32_t tail = (read_cmd(REG_RXDESCTAIL) + 1) % E1000_NUM_RX_DESC;

    if (!(rx_descs[tail].status & 1)) {
        // No packet
        return;
    }

    uint16_t len = rx_descs[tail].length;

    // Send to ethernet
    printf("Recieved packet of %d bytes\n", len);
    printf("%s\n", &rx_packets[tail]);

    rx_descs[tail].status = 0;
    write_cmd(REG_RXDESCTAIL, tail);

}

void network_handler() {
    cli();
    uint32_t status = read_cmd(REG_IREAD);

    printf("Network int with status %d\n", status);

    if (status & 0x04) {
        // start_link
    } else if (status & 0x10) {
        // Nothing?
    } else if (status & 0x80) {
        handle_recieve();
    }

    write_cmd(REG_IREAD, (1 << 7));
    send_eoi(NET_IRQ_LINE);
    sti();
}

int32_t net_send_data(const uint8_t *data, uint16_t nbytes) {
    // Get Tail index
    uint32_t tail = read_cmd(REG_TXDESCTAIL);

    if (!(tx_descs[tail].status & 1)) {
        // Queue full
        return -1;
    }

    printf("%s %s\n", "Sending", (uint8_t *) data);

    // Set Descriptor
    memcpy(&tx_packets[tail], data, nbytes);
    tx_descs[tail].length = nbytes;
    tx_descs[tail].cmd = CMD_EOP | CMD_IFCS | CMD_RS | CMD_RPS;
    tx_descs[tail].status = 0;

    // Update tail
    write_cmd(REG_TXDESCTAIL, (tail + 1) % E1000_NUM_TX_DESC);

    // Wait for it to send
    while(!(tx_descs[tail].status & 0xFF));

    return 0;
}

uint16_t *get_mac_addr() {
    return mac;
}
