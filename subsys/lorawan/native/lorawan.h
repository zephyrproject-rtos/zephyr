/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_LORAWAN_NATIVE_LORAWAN_H_
#define SUBSYS_LORAWAN_NATIVE_LORAWAN_H_

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/slist.h>
#include <zephyr/lorawan/lorawan.h>
#include <psa/crypto.h>

#include "region/region.h"

#ifdef __cplusplus
extern "C" {
#endif

enum lwan_flag {
	LWAN_FLAG_STARTED,
	LWAN_FLAG_JOINED,
	LWAN_FLAG_COUNT,
};

#define LWAN_MAX_CHANNELS	16

struct lwan_session {
	/* Device address assigned during join */
	uint32_t dev_addr;
	/* AppSKey for payload encrypt/decrypt (FPort > 0) */
	psa_key_id_t app_s_key;
	/* FNwkSIntKey for MIC computation (CMAC) */
	psa_key_id_t fnwk_s_int_cmac;
	/* FNwkSIntKey for payload encrypt/decrypt (FPort 0, ECB) */
	psa_key_id_t fnwk_s_int_ecb;
	/* RX1 datarate offset from DLSettings */
	uint8_t rx1_dr_offset;
	/* RX2 datarate index from DLSettings */
	uint8_t rx2_datarate;
	/* RX delay in seconds; 0 means 1 second per spec */
	uint8_t rx_delay;
	/* Uplink frame counter */
	uint32_t fcnt_up;
	/* Downlink frame counter */
	uint32_t fcnt_down;
};

struct lwan_ctx {
	/* Current session keys and counters */
	struct lwan_session session;
	/* Region-specific operations (channel plan, DR tables) */
	const struct lwan_region_ops *region;
	/* Configured channel list */
	struct lwan_channel channels[LWAN_MAX_CHANNELS];
	/* Number of active channels */
	size_t channel_count;
	/* Current uplink datarate */
	enum lorawan_datarate current_dr;
	/* State flags (started, joined) */
	ATOMIC_DEFINE(flags, LWAN_FLAG_COUNT);
	/* Max retries for confirmed uplinks */
	uint8_t conf_tries;
	/* Pending action bitmask (e.g. ACK to send) */
	uint8_t pending;
	/* Registered downlink callbacks */
	sys_slist_t dl_callbacks;
};

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_LORAWAN_NATIVE_LORAWAN_H_ */
