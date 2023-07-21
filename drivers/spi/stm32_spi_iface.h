/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Interface to abstract the STM32Cube low level functions. This is used
 * to avoid including directly STMCube-generated code, which enables the SPI
 * driver to be unit-tested easier.
 *
 */

#ifndef STM32_SPI_IFACE_H
#define STM32_SPI_IFACE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef ZTEST_UNITTEST
#define DT_HAS_COMPAT_STATUS_OKAY(...) 0
#endif /* ZTEST_UNITTEST */

typedef enum {
    STM32_SPI_MASTER,
    STM32_SPI_SLAVE
} stm32_spi_mode_t;

typedef void spi_stm32_t;

int ll_func_get_err(spi_stm32_t* spi);


#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)

bool ll_func_is_active_master_transfer(spi_stm32_t* spi);

void ll_func_start_master_transfer(spi_stm32_t* spi);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */


stm32_spi_mode_t ll_func_get_mode(spi_stm32_t* spi);

void ll_func_transmit_data_8(spi_stm32_t* spi, uint32_t val);

void ll_func_transmit_data_16(spi_stm32_t* spi, uint32_t val);

uint32_t ll_func_receive_data_8(spi_stm32_t* spi);

uint32_t ll_func_receive_data_16(spi_stm32_t* spi);

uint32_t ll_func_tx_is_empty(spi_stm32_t *spi);

uint32_t ll_func_rx_is_not_empty(spi_stm32_t *spi);

void ll_func_disable_int_tx_empty(spi_stm32_t *spi);

void ll_func_clear_modf_flag(spi_stm32_t* spi);

bool ll_func_is_modf_flag_set(spi_stm32_t* spi);

void ll_func_enable_spi(spi_stm32_t* spi, bool enable);

void ll_func_disable_int_rx_not_empty(spi_stm32_t *spi);

void ll_func_disable_int_errors(spi_stm32_t *spi);

uint32_t ll_func_spi_is_busy(spi_stm32_t *spi);

#endif /* STM32_SPI_IFACE_H */
