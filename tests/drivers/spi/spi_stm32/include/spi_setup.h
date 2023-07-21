/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SPI_SETUP_H
#define SPI_SETUP_H

#include <zephyr/drivers/spi.h>

/**
 * This is an SPI setup. It contains all common, SPI-related structs required
 * to perform an SPI transfer in unit tests.
 *
 */
typedef struct  {
    struct device spi;
    struct spi_config cfg;
    struct spi_buf_set tx_bufs;
    struct spi_buf_set rx_bufs;
} spi_setup_t;

/**
 * Creates an SPI setup with default values.
 *
 * @param data_len Length of the buffers to be used to send and receive data
 * from the SPI. It's the same for both rx and tx buffers.
 *
 * @warning The returned struct needs to be freed with @ref spi_setup_free
 *
 */
spi_setup_t spi_setup_create(uint32_t data_len);

/**
 * Retrieve the SPI pointer that will be provided to the STMCube SPI low level
 * functions.
 *
 */
spi_stm32_t* spi_setup_get_native_dev(const spi_setup_t* sps);

void spi_setup_free(spi_setup_t* sps);

#endif /* SPI_TEST_UTILS_H */
