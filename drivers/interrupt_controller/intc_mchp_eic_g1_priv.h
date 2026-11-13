/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Microchip Technology Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_MCHP_EIC_G1_PRIV_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_MCHP_EIC_G1_PRIV_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

#define EIC_MCHP_G1_LINES_LEN 16
#define EIC_MCHP_G1_LINE_EVEN GENMASK(3, 0)
#define EIC_MCHP_G1_LINE_ODD  GENMASK(7, 4)

/* Byte n of lines holds pin 2n in its low nibble and pin 2n + 1 in its high nibble */
static inline uint8_t eic_mchp_g1_line_from_pin(int pin, const uint8_t *lines)
{
	uint8_t pair = lines[pin / 2];

	if ((pin % 2) != 0) {
		return (uint8_t)FIELD_GET(EIC_MCHP_G1_LINE_ODD, pair);
	}

	return (uint8_t)FIELD_GET(EIC_MCHP_G1_LINE_EVEN, pair);
}

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_MCHP_EIC_G1_PRIV_H_ */
