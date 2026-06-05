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
 */

#ifndef SUBSYS_LORAWAN_NATIVE_MAC_MAC_COMMANDS_H_
#define SUBSYS_LORAWAN_NATIVE_MAC_MAC_COMMANDS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/lorawan/lorawan.h>

struct lwan_ctx;

#ifdef __cplusplus
extern "C" {
#endif

/* MAC command identifiers (CID) */
enum lwan_mac_cmd {
	MAC_CMD_RESET = 0x01,
	MAC_CMD_LINK_CHECK = 0x02,
	MAC_CMD_LINK_ADR = 0x03,
	MAC_CMD_DUTY_CYCLE = 0x04,
	MAC_CMD_RX_PARAM_SETUP = 0x05,
	MAC_CMD_DEV_STATUS = 0x06,
	MAC_CMD_NEW_CHANNEL = 0x07,
	MAC_CMD_RX_TIMING_SETUP = 0x08,
	MAC_CMD_TX_PARAM_SETUP = 0x09,
	MAC_CMD_DL_CHANNEL = 0x0a,
	MAC_CMD_REKEY = 0x0b,
	MAC_CMD_ADR_PARAM_SETUP = 0x0c,
	MAC_CMD_DEVICE_TIME = 0x0d,
	MAC_CMD_FORCE_REJOIN = 0x0e,
	MAC_CMD_REJOIN_PARAM = 0x0f,
	MAC_CMD_PING_SLOT_INFO = 0x10,
	MAC_CMD_PING_SLOT_CHAN = 0x11,
	MAC_CMD_BEACON_TIMING = 0x12,
	MAC_CMD_BEACON_FREQ = 0x13,
	MAC_CMD_DEVICE_MODE = 0x20,
};

/**
 * @brief Build the FOpts bytes for the next uplink frame.
 *
 * Called by the frame builder before each uplink.  Records a snapshot
 * of what was emitted so mac_cmd_commit_ul_fopts() can drain the right
 * state once the frame is actually on the wire.
 *
 * @param ctx Stack context.
 * @param buf Output buffer for FOpts bytes.
 * @param max_len Capacity of @p buf (at most 15 per spec).
 * @return Number of bytes written to @p buf (0..@p max_len).
 */
size_t mac_cmd_build_ul_fopts(struct lwan_ctx *ctx,
			      uint8_t *buf, size_t max_len);

/**
 * @brief Commit the FOpts snapshot recorded by the last build call.
 *
 * Called from the post-TX hook once the frame has been transmitted
 * successfully.  Clears the pending state for the commands that
 * actually went on the wire so they aren't re-emitted.  A failed TX
 * skips the commit, leaving the pending flags set for the next attempt.
 *
 * @param ctx Stack context.
 */
void mac_cmd_commit_ul_fopts(struct lwan_ctx *ctx);

/**
 * @brief Process the MAC commands carried in a downlink FOpts field.
 *
 * Called by the frame parser after MIC verification and replay
 * checks.  Known commands are dispatched to their handlers; an
 * unknown CID terminates parsing since its length is unknown.
 *
 * @param ctx Stack context.
 * @param fopts FOpts bytes from the downlink frame.
 * @param fopts_len Length of @p fopts in bytes.
 */
void mac_cmd_process_dl_fopts(struct lwan_ctx *ctx,
			      const uint8_t *fopts, size_t fopts_len);

/**
 * @brief Register the application's LinkCheckAns callback.
 *
 * @param cb Callback invoked on the system workqueue when a
 *           LinkCheckAns is received; NULL clears the registration.
 */
void mac_cmd_set_link_check_cb(lorawan_link_check_ans_cb_t cb);

/**
 * @brief Deliver any pending LinkCheckAns to the registered callback.
 *
 * Called from the downlink work handler so the application callback
 * runs on the system workqueue rather than the engine thread.
 *
 * @param ctx Stack context.
 */
void mac_cmd_deliver_link_check_ans(struct lwan_ctx *ctx);

/**
 * @brief Test whether the MAC layer has notifications pending delivery.
 *
 * Lets the downlink path know it must schedule the work handler even
 * if the frame carries no FRMPayload and no ACK.
 *
 * @param ctx Stack context.
 * @return true if at least one notification is pending.
 */
bool mac_cmd_has_pending_delivery(struct lwan_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_LORAWAN_NATIVE_MAC_MAC_COMMANDS_H_ */
