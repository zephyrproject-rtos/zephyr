/*
 * Copyright (c) 2023 Microchip Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SAMPLES_BOARDS_MEC5_ESPI_DEBUG_H_
#define __SAMPLES_BOARDS_MEC5_ESPI_DEBUG_H_

#include <stdint.h>

void espi_debug_print_config(void);
void espi_debug_print_io_bars(void);
void espi_debug_print_mem_bars(void);
void espi_debug_print_ctvw(void);
void espi_debug_print_tcvw(void);
void espi_debug_print_pc(void);
void espi_debug_print_oob(void);

void espi_debug_print_cap_word(uint16_t cap_offset, uint32_t cap);

#endif /* __SAMPLES_BOARDS_MEC5_ESPI_DEBUG_H_ */
