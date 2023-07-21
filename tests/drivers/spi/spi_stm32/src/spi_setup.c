/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <zephyr/ztest.h>

#include "spi_ll_stm32.h"
#include "spi_setup.h"

// Initialise memory to some value different than 0 to reduce the chance of
// false positives in the unit tests
#define MEM_DEF_VAL 0xaa

static struct device* create_gpio_port(void)
{
    struct gpio_driver_config* config =
        (struct gpio_driver_config*)malloc(sizeof(struct gpio_driver_config));

    struct gpio_driver_data* data =
        (struct gpio_driver_data*)malloc(sizeof(struct gpio_driver_data));
    // In this mask, each '1' indicates that the corresponding pin is
    // active-low.
    data->invert = 0xff;

    struct device* gpio = (struct device*)malloc(sizeof(struct device));
    gpio->config = config;
    gpio->data = data;

    return gpio;
}

static struct spi_stm32_config* create_spi_stm32_config(void)
{
    struct spi_stm32_config* spi_stm32_cfg =
        (struct spi_stm32_config*)malloc(sizeof(struct spi_stm32_config));
    memset(spi_stm32_cfg, MEM_DEF_VAL, sizeof(struct spi_stm32_config));

    return spi_stm32_cfg;
}

static struct spi_stm32_data* create_spi_stm32_data(void)
{
    struct spi_stm32_data* spi_stm32_d =
        (struct spi_stm32_data*)malloc(sizeof(struct spi_stm32_data));
    return spi_stm32_d;
}

static struct spi_buf* create_spi_buf(uint32_t data_len)
{
    uint8_t* data_buf = (uint8_t*)malloc(data_len * sizeof(uint8_t));
    memset(data_buf, MEM_DEF_VAL, data_len * sizeof(uint8_t));

    struct spi_buf* spi_data_buf =
        (struct spi_buf*)malloc(sizeof(struct spi_buf));
    spi_data_buf->buf = data_buf;
    spi_data_buf->len = data_len * sizeof(uint8_t);

    return spi_data_buf;
}

spi_setup_t spi_setup_create(uint32_t data_len)
{
    const uint32_t spi_default_sck_freq_hz = 12500000;

    spi_setup_t sps = {
        .spi = {
            .config = create_spi_stm32_config(),
            .data = create_spi_stm32_data()
        },
        .cfg = {
            .cs = {
                .gpio.port = create_gpio_port(),
                .gpio.pin = 0
            },
            .frequency = spi_default_sck_freq_hz,
            .operation = (SPI_OP_MODE_MASTER      |
                          SPI_WORD_SET(8)         |
                          SPI_TRANSFER_LSB),
            .slave = 0,
        },
        .tx_bufs = {
            .buffers = create_spi_buf(data_len),
            .count = 1
        },
        .rx_bufs = {
            .buffers = create_spi_buf(data_len),
            .count = 1
        }
    };

    return sps;
}

spi_stm32_t* spi_setup_get_native_dev(const spi_setup_t* sps)
{
    struct spi_stm32_config* cfg =
        (struct spi_stm32_config*)sps->spi.config;
    return cfg->spi;
}

void spi_setup_free(spi_setup_t* sps)
{
    free((void*)sps->spi.config);
    free((void*)sps->spi.data);
    free((void*)sps->cfg.cs.gpio.port);
    free((void*)sps->tx_bufs.buffers->buf);
    free((void*)sps->tx_bufs.buffers);
    free((void*)sps->rx_bufs.buffers->buf);
    free((void*)sps->rx_bufs.buffers);
}
