/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_LORAWAN_NATIVE_REGION_REGION_H_
#define SUBSYS_LORAWAN_NATIVE_REGION_REGION_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <zephyr/drivers/lora.h>
#include <zephyr/lorawan/lorawan.h>

#ifdef __cplusplus
extern "C" {
#endif

struct lwan_channel {
	/* Frequency in Hz */
	uint32_t frequency;
	/* Lowest datarate index allowed on this channel */
	uint8_t min_dr;
	/* Highest datarate index allowed on this channel */
	uint8_t max_dr;
	/* Channel enabled for TX selection */
	bool enabled;
};

struct lwan_dr_params {
	/* LoRa spreading factor */
	enum lora_datarate sf;
	/* LoRa bandwidth */
	enum lora_signal_bandwidth bw;
	/* Maximum application payload in bytes */
	uint8_t max_payload;
};

struct lwan_region_ops {
	/**
	 * @brief Get the default channels for this region.
	 *
	 * @param ch Output channel array.
	 * @param count In: max entries; Out: number of channels written.
	 * @return 0 on success, negative errno on failure.
	 */
	int (*get_default_channels)(struct lwan_channel *ch, size_t *count);

	/**
	 * @brief Get TX parameters for a given datarate and power index.
	 *
	 * The @p tx_power_idx parameter selects a region-specific power
	 * level (typically updated by LinkADRReq); pass 0 for the region's
	 * maximum EIRP.  Callers must ensure @p tx_power_idx is valid via
	 * @ref validate_tx_power before calling this; out-of-range values
	 * are clamped silently rather than rejected.
	 *
	 * @param dr Datarate index.
	 * @param tx_power_idx TX power index (0 = region maximum EIRP).
	 * @param p Output: SF/BW/max_payload parameters.
	 * @param power_dbm Output: TX power in dBm for this (dr, idx).
	 * @return 0 on success, negative errno on failure.
	 */
	int (*get_tx_params)(uint8_t dr, uint8_t tx_power_idx,
			     struct lwan_dr_params *p, int8_t *power_dbm);

	/**
	 * @brief Get RX1 window parameters.
	 *
	 * @param tx_freq TX frequency used.
	 * @param tx_dr TX datarate index.
	 * @param offset RX1 DR offset from join accept.
	 * @param rx1_freq Output: RX1 frequency.
	 * @param p Output: RX1 datarate parameters.
	 * @return 0 on success, negative errno on failure.
	 */
	int (*get_rx1_params)(uint32_t tx_freq, uint8_t tx_dr,
			      uint8_t offset, uint32_t *rx1_freq,
			      struct lwan_dr_params *p);

	/**
	 * @brief Get RX2 window parameters.
	 *
	 * @param dr RX2 datarate index.
	 * @param freq Output: RX2 frequency.
	 * @param p Output: RX2 datarate parameters.
	 * @return 0 on success, negative errno on failure.
	 */
	int (*get_rx2_params)(uint8_t dr, uint32_t *freq,
			      struct lwan_dr_params *p);

	/**
	 * @brief Validate DLSettings from a JoinAccept.
	 *
	 * @param rx1_dr_offset RX1 datarate offset (0-7 from wire).
	 * @param rx2_datarate  RX2 datarate index (0-15 from wire).
	 * @return 0 if valid, -EINVAL if either value is out of range.
	 */
	int (*validate_dl_settings)(uint8_t rx1_dr_offset,
				    uint8_t rx2_datarate);

	/**
	 * @brief Apply a CFList from join accept.
	 *
	 * @param cflist 16-byte CFList from join accept.
	 * @param ch Channel array to update.
	 * @param count In: current count; Out: updated count.
	 * @return 0 on success, negative errno on failure.
	 */
	int (*apply_cflist)(const uint8_t cflist[16],
			    struct lwan_channel *ch, size_t *count);

	/**
	 * @brief Validate a datarate index for this region.
	 *
	 * Used to sanity-check a DR received from the network (e.g. in a
	 * LinkADRReq) before the stack commits to it.
	 *
	 * @param dr Datarate index (0..15 from the wire).
	 * @return 0 if the DR is defined and usable, -EINVAL otherwise.
	 */
	int (*validate_dr)(uint8_t dr);

	/**
	 * @brief Validate a TX power index for this region.
	 *
	 * The index is region-specific; each region maps it to a dBm value.
	 * Used to sanity-check a LinkADRReq TXPower field.
	 *
	 * @param tx_power_idx TX power index (0..15 from the wire).
	 * @return 0 if the index is defined, -EINVAL otherwise.
	 */
	int (*validate_tx_power)(uint8_t tx_power_idx);

	/**
	 * @brief Validate and apply a ChMaskCntl + ChMask from a LinkADRReq.
	 *
	 * Validates the (@p ch_mask_cntl, @p ch_mask) pair against the
	 * region's rules (reserved control values, default-channel
	 * protection, non-empty result) and, only on success, commits the
	 * new enabled set to @p ch in place.  On failure @p ch is left
	 * untouched.  The meaning of @p ch_mask_cntl is region-specific.
	 *
	 * @param ch Channel array (modified in place on success).
	 * @param count Number of entries in @p ch.
	 * @param ch_mask_cntl ChMaskCntl field from the Redundancy byte (0..7).
	 * @param ch_mask 16-bit channel mask from the LinkADRReq.
	 * @return 0 on success, -EINVAL if the request is rejected.
	 */
	int (*apply_adr_channel_mask)(struct lwan_channel *ch, size_t count,
				      uint8_t ch_mask_cntl, uint16_t ch_mask);

	/**
	 * @brief Select a random channel for join request TX.
	 *
	 * @param ch Channel array.
	 * @param count Number of channels.
	 * @param freq Output: selected frequency.
	 * @param dr Output: selected datarate index.
	 * @param delay_ms Output (only on -ENOBUFS): ms until the earliest
	 *                 matching channel opens.  Not touched on success
	 *                 or -ENOENT.
	 * @return 0 on success, -ENOENT if no enabled channel exists,
	 *         -ENOBUFS if channels exist but all are duty-cycle-blocked.
	 */
	int (*select_join_channel)(const struct lwan_channel *ch,
				   size_t count, uint32_t *freq,
				   uint8_t *dr, int32_t *delay_ms);

	/**
	 * @brief Select a random channel for data TX at the given datarate.
	 *
	 * Picks a random enabled channel whose DR range includes the
	 * requested datarate.
	 *
	 * @param ch Channel array.
	 * @param count Number of channels.
	 * @param dr Desired datarate index.
	 * @param freq Output: selected frequency.
	 * @param delay_ms Output (only on -ENOBUFS): ms until the earliest
	 *                 matching channel opens.  Not touched on success
	 *                 or -ENOENT.
	 * @return 0 on success, -ENOENT if no channel supports the DR,
	 *         -ENOBUFS if channels match the DR but all are
	 *         duty-cycle-blocked.
	 */
	int (*select_data_channel)(const struct lwan_channel *ch,
				    size_t count, uint8_t dr,
				    uint32_t *freq, int32_t *delay_ms);

	/**
	 * @brief Record a completed TX for duty cycle tracking.
	 *
	 * @param freq TX frequency in Hz.
	 * @param airtime_ms On-air time in milliseconds.
	 */
	void (*record_tx)(uint32_t freq, uint32_t airtime_ms);
};

/* Returns NULL if the region is not supported by this backend */
const struct lwan_region_ops *lwan_region_get(enum lorawan_region region);

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_LORAWAN_NATIVE_REGION_REGION_H_ */
