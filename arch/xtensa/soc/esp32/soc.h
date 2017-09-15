/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H__
#define __SOC_H__

#include <rom/ets_sys.h>

#include <zephyr/types.h>
#include <stdbool.h>

extern int esp32_rom_intr_matrix_set(int cpu_no,
				     int interrupt_src,
				     int interrupt_line);

extern int esp32_rom_gpio_matrix_in(u32_t gpio, u32_t signal_index,
				    bool inverted);
extern int esp32_rom_gpio_matrix_out(u32_t gpio, u32_t signal_index,
				     bool out_inverted,
				     bool out_enabled_inverted);

extern void esp32_rom_uart_attach(void);
extern STATUS esp32_rom_uart_tx_one_char(u8_t chr);
extern STATUS esp32_rom_uart_rx_one_char(u8_t *chr);

#endif /* __SOC_H__ */
