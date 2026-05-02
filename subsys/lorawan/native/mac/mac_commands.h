/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Native LoRaWAN MAC command framework (FOpts).
 *
 * MAC commands (LinkCheck, LinkADR, DutyCycle, etc.) ride in the
 * FOpts region of data frames (up to 15 bytes) — inline with the
 * frame header, not in FRMPayload.  This module owns the parse/emit
 * path: the frame builder asks it for pending uplink commands, the
 * frame parser hands it incoming downlink commands.
 *
 * At present the module is a skeleton: future commits populate
 * per-command handlers.
 */

#ifndef SUBSYS_LORAWAN_NATIVE_MAC_MAC_COMMANDS_H_
#define SUBSYS_LORAWAN_NATIVE_MAC_MAC_COMMANDS_H_

#include <stddef.h>
#include <stdint.h>

struct lwan_ctx;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build the FOpts bytes for the next uplink frame.
 *
 * Called by the frame builder before each uplink.  Drains pending
 * MAC command state from @p ctx into @p buf.
 *
 * @param ctx Stack context.
 * @param buf Output buffer for FOpts bytes.
 * @param max_len Capacity of @p buf (at most 15 per spec).
 * @return Number of bytes written to @p buf (0..@p max_len).
 */
size_t mac_cmd_build_ul_fopts(struct lwan_ctx *ctx,
			      uint8_t *buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_LORAWAN_NATIVE_MAC_MAC_COMMANDS_H_ */
