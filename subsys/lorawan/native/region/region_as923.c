/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * AS923-1 regional parameters from LoRaWAN Regional Parameters RP002-1.0.5,
 * section 3.10
 * https://resources.lora-alliance.org/technical-specifications/rp002-1-0-5-regional-parameters
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>

#include "region.h"
#include <lorawan.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_native_as923, CONFIG_LORAWAN_LOG_LEVEL);

/*
 * AS923_FREQ_OFFSET_HZ shifts the default channels and RX2 to the sub-band a
 * country has available; frequencies the network hands over in a CFList or a
 * MAC command are absolute (RP002-1.0.5, 3.10.10).
 *
 * Groups AS923-2, -3 and -4 are the same plan at -1.80, -6.60 and -5.90 MHz.
 * enum lorawan_region has one AS923 entry and no way to ask for a group, and
 * the loramac-node backend likewise builds AS923-1 only, so the offset stays
 * zero until the API can express the choice.
 */
#define AS923_FREQ_OFFSET_HZ 0

/* AS923 default channels (Table 65) */
#define AS923_FREQ_CH0 (KHZ(923200) + AS923_FREQ_OFFSET_HZ)
#define AS923_FREQ_CH1 (KHZ(923400) + AS923_FREQ_OFFSET_HZ)

/* AS923 RX2 defaults (3.10.7) */
#define AS923_RX2_FREQ (KHZ(923200) + AS923_FREQ_OFFSET_HZ)
#define AS923_RX2_DR   2 /* DR2 = SF10/125kHz */

/* AS923 max EIRP (3.10.3) */
#define AS923_MAX_EIRP_DBM 16

/*
 * AS923 TXPower table (RP002-1.0.5, Table 70):
 *   index 0 = MaxEIRP (16 dBm), step -2 dB per index, up to index 7 = 2 dBm.
 */
#define AS923_MAX_TX_POWER_IDX 7

/* AS923 mandatory default channels (Table 65) */
#define AS923_DEFAULT_CH_COUNT 2

/*
 * Join-Requests run at DR2 or faster (Table 66). DR2 is the slowest the range
 * allows, so it reaches furthest, and a Join-Request still fits the 400 ms
 * dwell time a network may later impose: Table 72 leaves 19 bytes at DR2 and
 * the frame needs 18.
 */
#define AS923_JOIN_DR 2

/* CFList type 0: number of extra frequencies */
#define CFLIST_CH_COUNT 5

/* CFList frequency unit (100 Hz) */
#define CFLIST_FREQ_STEP_HZ 100

/*
 * AS923 datarate table: DR0..DR6 (Table 67).
 *
 * DR7 is 50 kbps FSK and DR12/DR13 are SF6/SF5, none of which this region
 * describes yet; validate_dr() rejects them. The maximum payloads are the
 * repeater-compatible N values for DwellTime = 0 (Table 72).
 */
static const struct lwan_dr_params as923_dr_table[] = {
	[0] = {.sf = SF_12, .bw = BW_125_KHZ, .max_payload = 51},
	[1] = {.sf = SF_11, .bw = BW_125_KHZ, .max_payload = 51},
	[2] = {.sf = SF_10, .bw = BW_125_KHZ, .max_payload = 115},
	[3] = {.sf = SF_9, .bw = BW_125_KHZ, .max_payload = 115},
	[4] = {.sf = SF_8, .bw = BW_125_KHZ, .max_payload = 222},
	[5] = {.sf = SF_7, .bw = BW_125_KHZ, .max_payload = 222},
	[6] = {.sf = SF_7, .bw = BW_250_KHZ, .max_payload = 222},
};

#define AS923_DR_COUNT ARRAY_SIZE(as923_dr_table)

/*
 * RX1 downlink datarate for DownlinkDwellTime = 0 (RP002-1.0.5, Table 74),
 * indexed by uplink datarate then by RX1DROffset.
 *
 * Unlike EU868 this is not an offset subtraction: RX1DROffset 6 and 7 raise
 * the downlink datarate above the uplink one. Rows stop at DR6 because that
 * is the fastest uplink this region offers, and the cells reaching DR7 are
 * the combinations 3.10.7 tells a device without DR7 not to use.
 */
#define AS923_MAX_RX1_DR_OFFSET 7

/* clang-format off */
static const uint8_t as923_rx1_dr[AS923_DR_COUNT][AS923_MAX_RX1_DR_OFFSET + 1] = {
	[0] = { 0, 0, 0, 0, 0, 0, 1, 2 },
	[1] = { 1, 0, 0, 0, 0, 0, 2, 3 },
	[2] = { 2, 1, 0, 0, 0, 0, 3, 4 },
	[3] = { 3, 2, 1, 0, 0, 0, 4, 5 },
	[4] = { 4, 3, 2, 1, 0, 0, 5, 6 },
	[5] = { 5, 4, 3, 2, 1, 0, 6, 7 },
	[6] = { 6, 5, 4, 3, 2, 1, 7, 7 },
};
/* clang-format on */

/*
 * The default channels carry a duty cycle below 1% (Table 65). The band is
 * tracked as a whole rather than per sub-band: AS923 draws no sub-band map of
 * its own, and the limit a country places on an operator-added channel is not
 * something the device is told.
 */
#define AS923_DUTY_CYCLE_INV 100

static int64_t as923_available_at;

static bool as923_band_available(void)
{
	return as923_available_at <= k_uptime_get();
}

static int as923_get_default_channels(struct lwan_channel *ch, size_t *count)
{
	if (*count < AS923_DEFAULT_CH_COUNT) {
		return -ENOMEM;
	}

	ch[0] = (struct lwan_channel){
		.frequency = AS923_FREQ_CH0,
		.min_dr = 0,
		.max_dr = 5,
		.enabled = true,
	};
	ch[1] = (struct lwan_channel){
		.frequency = AS923_FREQ_CH1,
		.min_dr = 0,
		.max_dr = 5,
		.enabled = true,
	};

	*count = AS923_DEFAULT_CH_COUNT;
	return 0;
}

static int as923_get_tx_params(uint8_t dr, uint8_t tx_power_idx, struct lwan_dr_params *p,
			       int8_t *power_dbm)
{
	uint8_t idx;

	if (dr >= AS923_DR_COUNT) {
		return -EINVAL;
	}

	/* Defensive clamp — validate_tx_power() is the authoritative check. */
	idx = MIN(tx_power_idx, AS923_MAX_TX_POWER_IDX);

	*p = as923_dr_table[dr];
	*power_dbm = AS923_MAX_EIRP_DBM - (int8_t)(2 * idx);
	return 0;
}

static int as923_get_rx1_params(uint32_t tx_freq, uint8_t tx_dr, uint8_t offset, uint32_t *rx1_freq,
				struct lwan_dr_params *p)
{
	uint8_t rx1_dr;

	if (tx_dr >= AS923_DR_COUNT || offset > AS923_MAX_RX1_DR_OFFSET) {
		return -EINVAL;
	}

	/* AS923: RX1 frequency = TX frequency */
	*rx1_freq = tx_freq;

	rx1_dr = as923_rx1_dr[tx_dr][offset];

	/*
	 * The table reaches DR7, which this region does not carry. Refusing
	 * the window is better than listening on the wrong datarate; 3.10.7
	 * puts the duty of avoiding the combination on the network.
	 */
	if (rx1_dr >= AS923_DR_COUNT) {
		LOG_WRN("RX1 DR%u unsupported (uplink DR%u, offset %u)", rx1_dr, tx_dr, offset);
		return -EINVAL;
	}

	*p = as923_dr_table[rx1_dr];
	return 0;
}

static int as923_get_rx2_params(uint8_t dr, uint32_t *freq, struct lwan_dr_params *p)
{
	if (dr >= AS923_DR_COUNT) {
		return -EINVAL;
	}

	*freq = AS923_RX2_FREQ;
	*p = as923_dr_table[dr];
	return 0;
}

static int as923_validate_dl_settings(uint8_t rx1_dr_offset, uint8_t rx2_datarate)
{
	if (rx1_dr_offset > AS923_MAX_RX1_DR_OFFSET) {
		return -EINVAL;
	}

	if (rx2_datarate >= AS923_DR_COUNT) {
		return -EINVAL;
	}

	return 0;
}

static int as923_apply_cflist(const uint8_t cflist[16], struct lwan_channel *ch, size_t *count)
{
	uint8_t cflist_type;
	size_t cflist_end;

	cflist_type = cflist[15];

	if (cflist_type != 0) {
		LOG_WRN("Unsupported CFList type: %u", cflist_type);
		return 0;
	}

	/*
	 * CFList type 0: 5 frequencies encoded as 3-byte little-endian
	 * values (in units of 100 Hz). AS923_FREQ_OFFSET_HZ is not applied
	 * to them (3.10.4).
	 */
	for (int i = 0; i < CFLIST_CH_COUNT; i++) {
		uint32_t freq;
		size_t idx = AS923_DEFAULT_CH_COUNT + i;

		if (idx >= LWAN_MAX_CHANNELS) {
			break;
		}

		freq = sys_get_le24(&cflist[i * 3]);
		freq *= CFLIST_FREQ_STEP_HZ;

		if (freq == 0) {
			ch[idx].enabled = false;
			continue;
		}

		ch[idx] = (struct lwan_channel){
			.frequency = freq,
			.min_dr = 0,
			.max_dr = 5,
			.enabled = true,
		};
	}

	/* Extend count to cover all CFList slots so disabled entries are visible */
	cflist_end = AS923_DEFAULT_CH_COUNT + CFLIST_CH_COUNT;

	if (cflist_end > LWAN_MAX_CHANNELS) {
		cflist_end = LWAN_MAX_CHANNELS;
	}
	if (*count < cflist_end) {
		*count = cflist_end;
	}

	return 0;
}

/*
 * AS923 ChMaskCntl encoding (RP002-1.0.5, Table 71):
 *   0..4 = ChMask applies to a block of 16 channels starting at cntl * 16.
 *   5    = the 10 LSBs each switch a bank of 8 channels; the 6 MSBs are RFU.
 *   6    = all defined channels on, whatever the mask says.
 *   7    = RFU.
 */
#define AS923_CH_MASK_CNTL_BLOCK_MAX 4
#define AS923_CH_MASK_CNTL_BANKS     5
#define AS923_CH_MASK_CNTL_ALL_ON    6

#define AS923_CH_BLOCK_SIZE 16
#define AS923_CH_BANK_SIZE  8
#define AS923_CH_BANK_COUNT 10

static int as923_validate_dr(uint8_t dr)
{
	if (dr >= AS923_DR_COUNT) {
		return -EINVAL;
	}

	return 0;
}

static int as923_validate_tx_power(uint8_t tx_power_idx)
{
	if (tx_power_idx > AS923_MAX_TX_POWER_IDX) {
		return -EINVAL;
	}

	return 0;
}

/* Count the channels a proposed mask would leave usable. */
static uint8_t as923_count_enabled(const struct lwan_channel *ch, size_t count, size_t first,
				   size_t span, uint16_t ch_mask)
{
	uint8_t enabled = 0;

	for (size_t i = 0; i < count; i++) {
		bool on;

		if (ch[i].frequency == 0) {
			continue;
		}

		if (i >= first && i < first + span) {
			on = (ch_mask & BIT(i - first)) != 0;
		} else {
			on = ch[i].enabled;
		}

		if (on) {
			enabled++;
		}
	}

	return enabled;
}

static int as923_apply_adr_channel_mask(struct lwan_channel *ch, size_t count, uint8_t ch_mask_cntl,
					uint16_t ch_mask)
{
	size_t first;
	size_t span;

	if (ch_mask_cntl == AS923_CH_MASK_CNTL_ALL_ON) {
		for (size_t i = 0; i < count; i++) {
			if (ch[i].frequency != 0) {
				ch[i].enabled = true;
			}
		}
		return 0;
	}

	if (ch_mask_cntl == AS923_CH_MASK_CNTL_BANKS) {
		uint8_t enabled = 0;

		/* The 6 MSBs are RFU and a set bit makes the command invalid. */
		if ((ch_mask >> AS923_CH_BANK_COUNT) != 0) {
			return -EINVAL;
		}

		for (size_t i = 0; i < count; i++) {
			size_t bank = i / AS923_CH_BANK_SIZE;

			if (ch[i].frequency == 0 || bank >= AS923_CH_BANK_COUNT) {
				continue;
			}
			if (ch_mask & BIT(bank)) {
				enabled++;
			}
		}

		/*
		 * The two join channels are the only way back to the network,
		 * so refuse a request that would take one of them away.
		 */
		for (size_t i = 0; i < MIN(count, (size_t)AS923_DEFAULT_CH_COUNT); i++) {
			if ((ch_mask & BIT(i / AS923_CH_BANK_SIZE)) == 0) {
				return -EINVAL;
			}
		}

		if (enabled == 0) {
			return -EINVAL;
		}

		for (size_t i = 0; i < count; i++) {
			size_t bank = i / AS923_CH_BANK_SIZE;

			if (ch[i].frequency == 0 || bank >= AS923_CH_BANK_COUNT) {
				continue;
			}
			ch[i].enabled = (ch_mask & BIT(bank)) != 0;
		}

		return 0;
	}

	if (ch_mask_cntl > AS923_CH_MASK_CNTL_BLOCK_MAX) {
		return -EINVAL;
	}

	first = (size_t)ch_mask_cntl * AS923_CH_BLOCK_SIZE;
	if (first >= count) {
		/* The block addresses channels this device does not have. */
		return -EINVAL;
	}

	span = MIN((size_t)AS923_CH_BLOCK_SIZE, count - first);

	/* A mask that turns on a channel the device does not have is invalid. */
	for (size_t i = 0; i < AS923_CH_BLOCK_SIZE; i++) {
		if ((ch_mask & BIT(i)) == 0) {
			continue;
		}
		if (i >= span || ch[first + i].frequency == 0) {
			return -EINVAL;
		}
	}

	/*
	 * The two join channels are the only way back to the network, so
	 * refuse a request that would take one of them away.
	 */
	for (size_t i = 0; i < MIN(count, (size_t)AS923_DEFAULT_CH_COUNT); i++) {
		if (i < first || i >= first + span) {
			continue;
		}
		if ((ch_mask & BIT(i - first)) == 0) {
			return -EINVAL;
		}
	}

	/*
	 * Pre-count the resulting enabled channels without mutating ch[]:
	 * we must reject a mask that leaves zero channels enabled before
	 * committing any change.
	 */
	if (as923_count_enabled(ch, count, first, span, ch_mask) == 0) {
		return -EINVAL;
	}

	/* Validated — commit. */
	for (size_t i = 0; i < span; i++) {
		if (ch[first + i].frequency == 0) {
			continue;
		}
		ch[first + i].enabled = (ch_mask & BIT(i)) != 0;
	}

	return 0;
}

static int as923_select_channel(const struct lwan_channel *ch, size_t count, size_t limit,
				uint8_t dr, uint32_t *freq, int32_t *delay_ms)
{
	uint8_t candidates[LWAN_MAX_CHANNELS];
	uint8_t candidate_count = 0;

	for (size_t i = 0; i < MIN(count, limit); i++) {
		if (!ch[i].enabled || ch[i].frequency == 0 || dr < ch[i].min_dr ||
		    dr > ch[i].max_dr) {
			continue;
		}

		candidates[candidate_count++] = i;
	}

	if (candidate_count == 0) {
		return -ENOENT;
	}

	if (!as923_band_available()) {
		int64_t wait = as923_available_at - k_uptime_get() + 1;

		*delay_ms = (wait > 0) ? (int32_t)wait : 1;
		return -ENOBUFS;
	}

	*freq = ch[candidates[sys_rand8_get() % candidate_count]].frequency;

	return 0;
}

static int as923_select_data_channel(const struct lwan_channel *ch, size_t count, uint8_t dr,
				     uint32_t *freq, int32_t *delay_ms)
{
	return as923_select_channel(ch, count, LWAN_MAX_CHANNELS, dr, freq, delay_ms);
}

static int as923_select_join_channel(const struct lwan_channel *ch, size_t count, uint32_t *freq,
				     uint8_t *dr, int32_t *delay_ms)
{
	int ret;

	/*
	 * Only the two default channels carry a Join-Request, and the slowest
	 * datarate the range allows is DR2 (Table 66).
	 */
	ret = as923_select_channel(ch, count, AS923_DEFAULT_CH_COUNT, AS923_JOIN_DR, freq,
				   delay_ms);
	if (ret != 0) {
		return ret;
	}

	*dr = AS923_JOIN_DR;

	return 0;
}

static void as923_record_tx(uint32_t freq, uint32_t airtime_ms)
{
	uint32_t off_time = airtime_ms * (AS923_DUTY_CYCLE_INV - 1);

	ARG_UNUSED(freq);

	as923_available_at = k_uptime_get() + off_time;

	LOG_DBG("Band off-time %u ms (airtime %u ms, dc 1/%u)", off_time, airtime_ms,
		AS923_DUTY_CYCLE_INV);
}

const struct lwan_region_ops as923_ops = {
	.default_rx2_dr = AS923_RX2_DR,
	.get_default_channels = as923_get_default_channels,
	.get_tx_params = as923_get_tx_params,
	.get_rx1_params = as923_get_rx1_params,
	.get_rx2_params = as923_get_rx2_params,
	.validate_dl_settings = as923_validate_dl_settings,
	.apply_cflist = as923_apply_cflist,
	.validate_dr = as923_validate_dr,
	.validate_tx_power = as923_validate_tx_power,
	.apply_adr_channel_mask = as923_apply_adr_channel_mask,
	.select_join_channel = as923_select_join_channel,
	.select_data_channel = as923_select_data_channel,
	.record_tx = as923_record_tx,
};
