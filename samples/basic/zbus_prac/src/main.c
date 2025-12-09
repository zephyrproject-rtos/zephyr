/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * STM32F4DISCOVERY + Zephyr + zbus + UART + GPIO + SPI + I2C demo
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/* ---------- GPIO: LED (from DT alias led0) ---------- */

#define LED0_NODE DT_ALIAS(led0)
#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Unsupported board: led0 alias is not defined in devicetree"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* ---------- UART: use console UART ---------- */

#define UART_NODE DT_CHOSEN(zephyr_console)
static const struct device *uart_dev = DEVICE_DT_GET(UART_NODE);

/* ---------- SPI / I2C devices (by label) ---------- */
/*
 * NOTE:
 *  - On many STM32F4 boards, SPI_1 / I2C_1 labels exist in the DTS.
 *  - If your DTS uses different labels, just change the strings below.
 *  - Using device_get_binding() keeps this generic and build-safe.
 */

static const struct device *spi_dev;
static const struct device *i2c_dev;

/* Simple SPI config – adjust if required for your board */
static struct spi_config spi_cfg = {
    .frequency = 1000000U, /* 1 MHz */
    .operation = SPI_OP_MODE_MASTER |
                 SPI_WORD_SET(8) |
                 SPI_TRANSFER_MSB,
    .slave = 0,            /* CS index (only used on some controllers) */
    .cs = NULL,            /* No HW-controlled CS here */
};

/* ---------- zbus message & channel ---------- */

struct bus_status_msg {
    uint32_t counter;
    bool led_state;
    int uart_rc;
    int spi_rc;
    int i2c_rc;
};

ZBUS_SUBSCRIBER_DEFINE(bus_status_sub, 4);

ZBUS_CHAN_DEFINE(
    bus_status_chan,              /* Name */
    struct bus_status_msg,        /* Message type */
    NULL,                         /* Validator */
    NULL,                         /* User data */
    ZBUS_OBSERVERS(bus_status_sub), /* Observers list */
    ZBUS_MSG_INIT(                /* Initial value */
        .counter = 0,
        .led_state = false,
        .uart_rc = 0,
        .spi_rc = 0,
        .i2c_rc = 0
    )
);

/* ---------- Helper: UART print ---------- */

static void uart_print(const char *s)
{
    if (!device_is_ready(uart_dev)) {
        return;
    }

    while (*s) {
        uart_poll_out(uart_dev, (unsigned char)*s);
        s++;
    }
}

/* ---------- Producer thread: toggles LED + pings buses + publishes ---------- */

#define PRODUCER_STACK_SIZE 1024
#define PRODUCER_PRIORITY   5

static void bus_producer_thread(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    int rc;
    uint32_t counter = 0;
    bool led_state = false;

    LOG_INF("bus_producer_thread started");

    for (;;) {
        struct bus_status_msg msg = {0};

        msg.counter = counter++;

        /* GPIO: toggle LED */
        if (device_is_ready(led.port)) {
            rc = gpio_pin_toggle_dt(&led);
            if (rc == 0) {
                led_state = !led_state;
                msg.led_state = led_state;
            } else {
                LOG_WRN("gpio_pin_toggle_dt failed: %d", rc);
                msg.led_state = led_state;
            }
        } else {
            LOG_WRN("LED device not ready");
            msg.led_state = led_state;
        }

        /* UART: print heartbeat */
        if (device_is_ready(uart_dev)) {
            uart_print("zbus bus_producer heartbeat\r\n");
            msg.uart_rc = 0;
        } else {
            msg.uart_rc = -ENODEV;
        }

        /* SPI: do a dummy transceive if device present */
        msg.spi_rc = -ENODEV;
        if (spi_dev && device_is_ready(spi_dev)) {
            uint8_t tx_buf_data[2] = {0xAA, 0x55};
            uint8_t rx_buf_data[2] = {0};

            struct spi_buf tx_buf = {
                .buf = tx_buf_data,
                .len = sizeof(tx_buf_data),
            };
            struct spi_buf rx_buf = {
                .buf = rx_buf_data,
                .len = sizeof(rx_buf_data),
            };

            const struct spi_buf_set tx = {
                .buffers = &tx_buf,
                .count = 1,
            };
            const struct spi_buf_set rx = {
                .buffers = &rx_buf,
                .count = 1,
            };

            rc = spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
            msg.spi_rc = rc;
            if (rc != 0) {
                LOG_WRN("SPI transceive failed: %d", rc);
            }
        }

        /* I2C: just check readiness (no actual target required) */
        msg.i2c_rc = -ENODEV;
        if (i2c_dev && device_is_ready(i2c_dev)) {
            /* If you have a real device, put i2c_read/i2c_write here */
            msg.i2c_rc = 0;
        }

        /* Publish to zbus */
        rc = zbus_chan_pub(&bus_status_chan, &msg, K_MSEC(100));
        if (rc != 0) {
            LOG_ERR("zbus_chan_pub failed: %d", rc);
        }

        k_sleep(K_SECONDS(1));
    }
}

K_THREAD_DEFINE(bus_producer_tid,
                PRODUCER_STACK_SIZE,
                bus_producer_thread,
                NULL, NULL, NULL,
                PRODUCER_PRIORITY, 0, 0);

/* ---------- Consumer thread: waits on subscriber & logs messages ---------- */

#define CONSUMER_STACK_SIZE 1024
#define CONSUMER_PRIORITY   6

static void bus_consumer_thread(void *sub_ptr, void *b, void *c)
{
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    const struct zbus_observer *subscriber = sub_ptr;
    const struct zbus_channel *chan;
    struct bus_status_msg msg;
    int rc;

    LOG_INF("bus_consumer_thread started");

    for (;;) {
        rc = zbus_sub_wait(subscriber, &chan, K_FOREVER);
        if (rc != 0) {
            LOG_ERR("zbus_sub_wait failed: %d", rc);
            continue;
        }

        if (chan != &bus_status_chan) {
            LOG_WRN("Received notification from unexpected channel: %s",
                    zbus_chan_name(chan));
            continue;
        }

        rc = zbus_chan_read(chan, &msg, K_MSEC(100));
        if (rc != 0) {
            LOG_WRN("zbus_chan_read failed: %d", rc);
            continue;
        }

        LOG_INF("BusStatus: cnt=%u led=%d uart_rc=%d spi_rc=%d i2c_rc=%d",
                msg.counter,
                msg.led_state,
                msg.uart_rc,
                msg.spi_rc,
                msg.i2c_rc);
    }
}

K_THREAD_DEFINE(bus_consumer_tid,
                CONSUMER_STACK_SIZE,
                bus_consumer_thread,
                &bus_status_sub, NULL, NULL,
                CONSUMER_PRIORITY, 0, 0);

/* ---------- main() ---------- */

int main(void)
{
    int rc;

    LOG_INF("STM32F4DISCOVERY zbus + UART + GPIO + SPI + I2C demo starting");

    /* Init LED */
    if (!device_is_ready(led.port)) {
        LOG_ERR("LED device not ready");
    } else {
        rc = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
        if (rc != 0) {
            LOG_ERR("Failed to configure LED GPIO: %d", rc);
        }
    }

    /* Init UART (console) */
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("Console UART device not ready");
    } else {
        uart_print("Console UART ready\r\n");
    }

    /* Bind SPI and I2C devices by label (adjust string if needed) */
    spi_dev = device_get_binding("SPI_1");
    if (!spi_dev) {
        LOG_WRN("SPI_1 device not found (update label if needed)");
    } else if (!device_is_ready(spi_dev)) {
        LOG_WRN("SPI_1 device not ready");
    } else {
        LOG_INF("SPI_1 device ready");
    }

    i2c_dev = device_get_binding("I2C_1");
    if (!i2c_dev) {
        LOG_WRN("I2C_1 device not found (update label if needed)");
    } else if (!device_is_ready(i2c_dev)) {
        LOG_WRN("I2C_1 device not ready");
    } else {
        LOG_INF("I2C_1 device ready");
    }

    LOG_INF("Main init done, threads will handle the rest");
    return 0;
}

