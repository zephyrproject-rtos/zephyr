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


#define SPI_NODE DT_ALIAS(spi0)

#define LED0_NODE DT_ALIAS(led0)

#define SPI_MODE SPI_OP_MODE_MASTER

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static const char tx_data[] = "Test Data from SPI Master";

static const struct device *spi_dev;

int main(void)
{
    static const struct spi_config spis_config = {
        .operation = SPI_MODE | SPI_TRANSFER_MSB | SPI_WORD_SET(8),
        .frequency = 100000,
        .slave = 0,
    };

    struct spi_buf tx_buf = {
        .buf = (void *)tx_data,
        .len = sizeof(tx_data)};

    struct spi_buf_set tx_bufs = {
        .buffers = &tx_buf,
        .count = 1};

    uint8_t rx_data[sizeof(tx_data)];
    struct spi_buf rx_buf = {
        .buf = rx_data,
        .len = sizeof(rx_data)};

    struct spi_buf_set rx_bufs = {
        .buffers = &rx_buf,
        .count = 1};

    spi_dev = DEVICE_DT_GET(SPI_NODE);

    if (!gpio_is_ready_dt(&led))
    {
        return 0;
    }

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
        int ret = spi_transceive(spi_dev, &spis_config, &tx_bufs, &rx_bufs);
        if (ret == 0)
        {
            printk("Sent: %s\n", tx_data);
            printk("Received: %s\n", rx_data);

            ret = gpio_pin_toggle_dt(&led);
        }
        else
        {
            printk("SPI transceive error: %d", ret);
        }

        k_msleep(1000);
    }
}
