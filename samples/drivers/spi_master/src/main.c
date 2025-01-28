/*
 * Copyright (c) 2025 Toney Abraham
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_master, LOG_LEVEL_INF);


// Use the alias 'spi-0' to identify the SPI node.
#define SPI_NODE DT_ALIAS(spi_0)

// Define the node identifier for the led0 which indicates the write success
#define LED0_NODE DT_ALIAS(led0)

// Set SPI as Master
#define SPI_MODE SPI_OP_MODE_MASTER

// Get DT spec of LED0
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

// Data to sent
const char tx_data[] = "Test Data from SPI Master";

static const struct device *spi_dev;

int main(void)
{
    // SPI config for transmit
    static const struct spi_config spis_config = {
        .operation = SPI_MODE | SPI_TRANSFER_MSB | SPI_WORD_SET(8),
        .frequency = 100000,
        .slave = 0,
    };

    // Structure for SPI transfer
    struct spi_buf tx_buf = {
        .buf = (void *)tx_data,
        .len = sizeof(tx_data)};

    struct spi_buf_set tx_bufs = {
        .buffers = &tx_buf,
        .count = 1};

    // Prepare a buffer to hold the data read from the slave
    uint8_t rx_data[sizeof(tx_data)];
    struct spi_buf rx_buf = {
        .buf = rx_data,
        .len = sizeof(rx_data)};

    struct spi_buf_set rx_bufs = {
        .buffers = &rx_buf,
        .count = 1};

    // Get the DT spec of SPI_NODE
    spi_dev = DEVICE_DT_GET(SPI_NODE);

    if (!gpio_is_ready_dt(&led))
    {
        return 0;
    }

    // configure the led
    int ledret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ledret < 0)
    {
        return 0;
    }

    if (!device_is_ready(spi_dev))
    {
        printk("Error: SPI device not ready");
        return 0;
    }

    while (1)
    {
        // Send and receive data over SPI (full-duplex communication)
        int ret = spi_transceive(spi_dev, &spis_config, &tx_bufs, &rx_bufs);
        if (ret == 0)
        {
            printk("Sent: %s\n", tx_data);
            printk("Received: %s\n", rx_data);

            // Toggle the LED
            ret = gpio_pin_toggle_dt(&led);
        }
        else
        {
            printk("SPI transceive error: %d", ret);
        }

        // Add a small delay before the next transmission
        k_msleep(1000); // 1-second interval
    }
}