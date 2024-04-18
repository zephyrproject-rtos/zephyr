/*
 * Copyright (c) 2023 Microchip Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SAMPLES_BOARDS_MEC_ASSY6906_ESPI_DEBUG_H_
#define __SAMPLES_BOARDS_MEC_ASSY6906_ESPI_DEBUG_H_

#include <stdint.h>
#include "espi_hc_emu.h"

void espi_debug_pr_status(uint16_t espi_status);
void espi_debug_print_cap_word(uint16_t cap_offset, uint32_t cap);
void espi_debug_print_hc(struct espi_hc_context *hc, uint8_t decode_caps);
void espi_debug_print_vws(struct espi_hc_vw *vws, size_t nvws);
const char *get_vw_name(uint32_t vwire_enum_val);

#endif /* __SAMPLES_BOARDS_MEC_ASSY6906_ESPI_DEBUG_H_ */
