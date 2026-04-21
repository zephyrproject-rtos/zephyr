/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include "lorawan.h"
#include "mac_commands.h"

size_t mac_cmd_build_ul_fopts(struct lwan_ctx *ctx,
			      uint8_t *buf, size_t max_len)
{
	(void)ctx;
	(void)buf;
	(void)max_len;

	/* No MAC commands are emitted yet; future commits populate this. */
	return 0;
}
