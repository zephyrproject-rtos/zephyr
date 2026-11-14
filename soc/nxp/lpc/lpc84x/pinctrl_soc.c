/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <pinctrl_soc.h>

/* @brief Map port and pin to IOCON index.
 *
 * @param port Port number (0 or 1).
 * @param pin Pin number (0..31 for port 0, 0..21 for port 1).
 * @return IOCON index on success, -EINVAL on failure.
 */

static const uint8_t lpc84x_iocon_idx_p0[32] = {
	17, 11, 6,  5,  4,  3,  16, 15, /* P0.0  - P0.7  */
	14, 13, 8,  7,  2,  1,  18, 10, /* P0.8  - P0.15 */
	9,  0,  30, 29, 28, 27, 26, 25, /* P0.16 - P0.23 */
	24, 23, 22, 21, 20, 50, 51, 35, /* P0.24 - P0.31 */
};

static const uint8_t lpc84x_iocon_idx_p1[22] = {
	36, 37, 38, 41, 42, 43, 46, 49, /* P1.0  - P1.7  */
	31, 32, 55, 54, 33, 34, 39, 40, /* P1.8  - P1.15 */
	44, 45, 47, 48, 52, 53,         /* P1.16 - P1.21 */
};

int lpc84x_iocon_index(uint8_t port, uint8_t pin)
{
	if (port == 0 && pin < 32) {
		return lpc84x_iocon_idx_p0[pin];
	} else if (port == 1 && pin < 22) {
		return lpc84x_iocon_idx_p1[pin];
	}

	return -EINVAL;
}
