#include <stdio.h>
#include <cstdlib>
#include <vector>
#include <cstring>

#include "mctp.hpp"
#include "i2c/i2c.h"

extern "C" {
#include "libmctp.h"
#include "core-internal.h"
#include "libmctp-i2c.h"
#include "i2c-internal.h"
}

namespace MCTP {
    static const uint8_t I2C_ADDR = 0x21;
    static const uint8_t EID = 0x50;
    struct mctp *mctp;
    struct mctp_binding_i2c *i2c;
    static void rx(uint8_t src_eid, bool tag_owner, uint8_t msg_tag, void *ctx,
        void *msg, size_t len);
    static int tx(const void *buf, size_t len, void *ctx);
    void recv(uint8_t *buf, size_t len) {
        mctp_i2c_rx(i2c, buf, len);
    }
    ;
    void send(uint8_t eid, const uint8_t *buf, size_t len, void **rx_buf, size_t *rx_len) {
        eid = 0x51;
        mctp_message_tx(mctp, eid, true, 2, buf, len);
        while (!mctp_is_tx_ready(mctp, eid)) {
            mctp_i2c_tx_poll(i2c);
        }

        int bus = i2c_open("/dev/i2c-1");
        if (bus == -1) {
            printf("bus error\n");
            return;
        }

        I2CDevice device;
        memset(&device, 0, sizeof(device));

        device.bus = bus;
        device.addr = 0x12;
        device.iaddr_bytes = 0;
        device.page_bytes = 16;

        uint8_t rx_buffer[300];
        ssize_t rx_bytes;
        rx_bytes = i2c_ioctl_read(&device, 0x0, rx_buffer, 3);
        if (rx_bytes != 3) {
            printf("rx header error\n");
            return;
        }
        ssize_t payload_bytes = rx_buffer[2];
        printf("%d payload bytes\n", payload_bytes);

        rx_bytes = i2c_ioctl_read(&device, 0x0, rx_buffer, payload_bytes + 3);
        printf("%d readed bytes\n", rx_bytes);

        i2c_close(bus);

        *rx_buf = malloc(payload_bytes - 6);
        memcpy(*rx_buf, &rx_buffer[3 + 6], payload_bytes - 6);
        *rx_len = payload_bytes - 6;
    }
}

void MCTP::init() {
    mctp_set_log_stdio(MCTP_LOG_DEBUG);
    mctp = mctp_init();
    assert(mctp);
    i2c = reinterpret_cast<struct mctp_binding_i2c *>(malloc(
        sizeof(struct mctp_binding_i2c)));
    assert(i2c);

    mctp_i2c_setup(i2c, I2C_ADDR, tx, NULL);
    mctp_register_bus(mctp, mctp_binding_i2c_core(i2c), EID);
    mctp_set_rx_all(mctp, rx, NULL);
    mctp_i2c_set_neighbour(i2c, 0x51, 0x22);
}

static void MCTP::rx(uint8_t src_eid, bool tag_owner, uint8_t msg_tag,
    void *ctx, void *msg, size_t len) {
    printf("I2C RX\n");
}

static int MCTP::tx(const void *buf, size_t len, void *ctx) {
    int bus = i2c_open("/dev/i2c-1");
    if (bus == -1) {
        printf("bus error\n");
        return -1;
    }

    I2CDevice device;
    memset(&device, 0, sizeof(device));

    device.bus = bus;
    device.addr = 0x12;
    device.iaddr_bytes = 0;
    device.page_bytes = 16;

    ssize_t tx_bytes = i2c_ioctl_write(&device, 0x0, buf, len);
    if (tx_bytes < (ssize_t)len) {
        printf("tx error\n");
        return -1;
    }

    i2c_close(bus);

    return 0;
}