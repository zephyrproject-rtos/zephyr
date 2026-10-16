/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MSPI_MCHP_QSPI_G1_H_
#define MSPI_MCHP_QSPI_G1_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

/* Timing parameter flags for mspi_timing_config() */
enum mspi_mchp_timing_param {
	MSPI_MCHP_TIMING_DLYBS = BIT(0),
	MSPI_MCHP_TIMING_DLYBCT = BIT(1),
	MSPI_MCHP_TIMING_DLYCS = BIT(2),
};

struct mchp_qspi_timing_cfg {
	uint8_t dlybs;
	uint8_t dlybct;
	uint8_t dlycs;
};

#endif /* MSPI_MCHP_QSPI_G1_H_ */
