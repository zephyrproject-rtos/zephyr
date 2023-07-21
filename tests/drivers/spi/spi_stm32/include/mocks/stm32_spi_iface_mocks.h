/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STM32_SPI_IFACE_MOCKS_H
#define STM32_SPI_IFACE_MOCKS_H

#include <stdint.h>

#include <zephyr/fff.h>

#include "stm32_spi_iface.h"

DECLARE_FAKE_VOID_FUNC(ll_func_set_internal_ss_mode_high, spi_stm32_t*);
DECLARE_FAKE_VOID_FUNC(ll_func_set_transfer_direction_full_duplex, spi_stm32_t*);
DECLARE_FAKE_VOID_FUNC(ll_func_start_master_transfer, spi_stm32_t*);
DECLARE_FAKE_VOID_FUNC(ll_func_transmit_data_8, spi_stm32_t*, uint32_t);
DECLARE_FAKE_VOID_FUNC(ll_func_transmit_data_16, spi_stm32_t*, uint32_t);
DECLARE_FAKE_VOID_FUNC(ll_func_enable_int_tx_empty, spi_stm32_t*);
DECLARE_FAKE_VOID_FUNC(ll_func_disable_int_tx_empty, spi_stm32_t*);
DECLARE_FAKE_VOID_FUNC(ll_func_enable_int_rx_not_empty, spi_stm32_t*);
DECLARE_FAKE_VOID_FUNC(ll_func_disable_int_rx_not_empty, spi_stm32_t*);
DECLARE_FAKE_VOID_FUNC(ll_func_enable_int_errors, spi_stm32_t*);
DECLARE_FAKE_VOID_FUNC(ll_func_disable_int_errors, spi_stm32_t*);
DECLARE_FAKE_VOID_FUNC(ll_func_enable_spi, spi_stm32_t*, bool);
DECLARE_FAKE_VOID_FUNC(ll_func_set_data_width, spi_stm32_t*, stm32_spi_data_width_t);
DECLARE_FAKE_VOID_FUNC(ll_func_clear_modf_flag, spi_stm32_t*);
DECLARE_FAKE_VALUE_FUNC(bool, ll_func_is_modf_flag_set, spi_stm32_t*);
DECLARE_FAKE_VOID_FUNC(ll_func_set_polarity, spi_stm32_t*, stm32_spi_cpol_t);
DECLARE_FAKE_VOID_FUNC(ll_func_set_clock_phase, spi_stm32_t*, stm32_spi_cpha_t);
DECLARE_FAKE_VOID_FUNC(ll_func_set_bit_order, spi_stm32_t*, stm32_spi_bit_order_t);
DECLARE_FAKE_VOID_FUNC(ll_func_set_mode, spi_stm32_t*, stm32_spi_mode_t);
DECLARE_FAKE_VALUE_FUNC(stm32_spi_mode_t, ll_func_get_mode, spi_stm32_t*);
DECLARE_FAKE_VOID_FUNC(ll_func_set_nss_mode, spi_stm32_t*, stm32_spi_nss_mode_t);
DECLARE_FAKE_VOID_FUNC(ll_func_disable_crc, spi_stm32_t*);
DECLARE_FAKE_VOID_FUNC(ll_func_set_fifo_threshold_8bit, spi_stm32_t*);
DECLARE_FAKE_VALUE_FUNC(uint32_t, ll_func_tx_is_empty, spi_stm32_t*);
DECLARE_FAKE_VALUE_FUNC(uint32_t, ll_func_rx_is_not_empty, spi_stm32_t*);
DECLARE_FAKE_VALUE_FUNC(uint32_t, ll_func_spi_is_busy, spi_stm32_t*);
DECLARE_FAKE_VALUE_FUNC(int, ll_func_get_err, spi_stm32_t*);
DECLARE_FAKE_VALUE_FUNC(bool, ll_func_is_active_master_transfer, spi_stm32_t*);
DECLARE_FAKE_VALUE_FUNC(int, ll_func_set_baudrate_prescaler, spi_stm32_t*, uint32_t, uint32_t);
DECLARE_FAKE_VALUE_FUNC(bool, ll_func_is_nss_polarity_low, spi_stm32_t*);
DECLARE_FAKE_VALUE_FUNC(uint32_t, ll_func_receive_data_8, spi_stm32_t*);
DECLARE_FAKE_VALUE_FUNC(uint32_t, ll_func_receive_data_16, spi_stm32_t*);
DECLARE_FAKE_VALUE_FUNC(int, ll_func_set_standard, spi_stm32_t*, stm32_standard_t);

#endif /* STM32_SPI_IFACE_MOCKS_H */
