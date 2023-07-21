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

typedef enum {
    STM32_SPI_CPOL_0,
    STM32_SPI_CPOL_1,
} stm32_spi_cpol_t;

typedef enum {
    STM32_SPI_CPHA_0,
    STM32_SPI_CPHA_1,
} stm32_spi_cpha_t;

typedef enum {
    STM32_SPI_LSB_FIRST,
    STM32_SPI_MSB_FIRST,
} stm32_spi_bit_order_t;

typedef enum {
    STM32_SPI_NSS_SOFT,
    STM32_SPI_NSS_HARD_OUTPUT,
    STM32_SPI_NSS_HARD_INPUT
} stm32_spi_nss_mode_t;

typedef enum {
    STM32_SPI_DATA_WIDTH_8,
    STM32_SPI_DATA_WIDTH_16,
    STM32_SPI_DATA_WIDTH_32
} stm32_spi_data_width_t;

typedef enum {
    STM32_SPI_STANDARD_TI,
    STM32_SPI_STANDARD_MOTOROLA,
} stm32_standard_t;


typedef void spi_stm32_t;

int ll_func_get_err(spi_stm32_t* spi);


#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)

bool ll_func_is_active_master_transfer(spi_stm32_t* spi);

void ll_func_start_master_transfer(spi_stm32_t* spi);

bool ll_func_is_nss_polarity_low(spi_stm32_t* spi);

void ll_func_set_internal_ss_mode_high(spi_stm32_t* spi);

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

int ll_func_set_standard(spi_stm32_t* spi, stm32_standard_t st);

int ll_func_set_baudrate_prescaler(spi_stm32_t* spi,
                                   uint32_t spi_periph_clk,
                                   uint32_t target_spi_frequency);

void ll_func_set_polarity(spi_stm32_t* spi, stm32_spi_cpol_t cpol);

void ll_func_set_clock_phase(spi_stm32_t* spi, stm32_spi_cpha_t cpha);

void ll_func_set_transfer_direction_full_duplex(spi_stm32_t* spi);

void ll_func_set_bit_order(spi_stm32_t* spi, stm32_spi_bit_order_t bit_order);

void ll_func_disable_crc(spi_stm32_t* spi);

void ll_func_set_nss_mode(spi_stm32_t* spi, stm32_spi_nss_mode_t mode);

void ll_func_set_mode(spi_stm32_t* spi, stm32_spi_mode_t mode);

void ll_func_set_data_width(
    spi_stm32_t* spi, stm32_spi_data_width_t data_width);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_fifo)

void ll_func_set_fifo_threshold_8bit(spi_stm32_t *spi);

#endif /* #if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_fifo) */

#endif /* STM32_SPI_IFACE_H */
