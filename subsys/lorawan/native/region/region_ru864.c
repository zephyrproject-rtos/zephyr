/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * RU864-870 regional parameters from LoRaWAN Regional Parameters RP002-1.0.5,
 * section 3.13
 * https://resources.lora-alliance.org/technical-specifications/rp002-1-0-5-regional-parameters
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>

#include "region.h"
#include <lorawan.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_native_ru864, CONFIG_LORAWAN_LOG_LEVEL);

/* Default channels (Table 100) */
#define RU864_FREQ_CH0 KHZ(868900)
#define RU864_FREQ_CH1 KHZ(869100)

/* Ends of the channel plan (3.13) */
#define RU864_FREQ_MIN KHZ(864000) /* 864 MHz */
#define RU864_FREQ_MAX KHZ(870000) /* 870 MHz */

/* RX2 defaults (3.13.7): 869.1 MHz at DR0, the fallback lwan_region_ops holds */
#define RU864_RX2_FREQ KHZ(869100)

/* Max EIRP (3.13.3) */
#define RU864_MAX_EIRP_DBM 16

/*
 * TXPower table (Table 105): index 0 = MaxEIRP (16 dBm), step -2 dB per
 * index, up to index 7 = 2 dBm. Indices 8 to 14 are RFU.
 */
#define RU864_MAX_TX_POWER_IDX 7

/* Mandatory default channels (Table 100) */
#define RU864_DEFAULT_CH_COUNT 2

/*
 * Join-Requests go out on the two default channels, which Table 101 opens to
 * DR0 through DR5. DR0 is the slowest and so reaches furthest, and the band
 * carries no dwell time (3.13.3) to argue for anything faster.
 */
#define RU864_JOIN_DR 0

/*
 * Datarate table: DR0..DR6 (Table 102).
 *
 * DR0 to DR5 are the certification minimum 3.13.3 requires; DR6 is the one
 * further datarate the backend can describe. DR7 is 50 kbps FSK and DR12 and
 * DR13 are SF6 and SF5, none of which this region carries; validate_dr()
 * rejects them. The maximum payloads are the repeater-compatible N values of
 * Table 107.
 */
static const struct lwan_dr_params ru864_dr_table[] = {
	[0] = {.sf = SF_12, .bw = BW_125_KHZ, .max_payload = 51},
	[1] = {.sf = SF_11, .bw = BW_125_KHZ, .max_payload = 51},
	[2] = {.sf = SF_10, .bw = BW_125_KHZ, .max_payload = 51},
	[3] = {.sf = SF_9, .bw = BW_125_KHZ, .max_payload = 115},
	[4] = {.sf = SF_8, .bw = BW_125_KHZ, .max_payload = 222},
	[5] = {.sf = SF_7, .bw = BW_125_KHZ, .max_payload = 222},
	[6] = {.sf = SF_7, .bw = BW_250_KHZ, .max_payload = 222},
};

#define RU864_DR_COUNT ARRAY_SIZE(ru864_dr_table)

/*
 * RX1 takes the uplink channel and the uplink datarate lowered by the offset
 * (Table 109). Across DR0 to DR7 that table is plain subtraction clamped at
 * DR0, so it needs no array; the DR12 and DR13 rows that break the pattern
 * belong to datarates this region does not carry. 3.13.7 reserves offsets 6
 * and 7 and tells the device not to accept them.
 */
#define RU864_MAX_RX1_DR_OFFSET 5

/*
 * The default channels carry a duty cycle below 1% (Table 100). The band is
 * tracked as a whole rather than per sub-band: RP002 draws no sub-band map
 * for RU864, and the limit the regulator places on an operator-added channel
 * is not something the device is told.
 */
#define RU864_DUTY_CYCLE_INV 100

static int64_t ru864_available_at;

static bool ru864_band_available(void)
{
	return ru864_available_at <= k_uptime_get();
}

static int ru864_get_default_channels(struct lwan_channel *ch, size_t *count)
{
	if (*count < RU864_DEFAULT_CH_COUNT) {
		return -ENOMEM;
	}

	ch[0] = (struct lwan_channel){
		.frequency = RU864_FREQ_CH0,
		.min_dr = 0,
		.max_dr = 5,
		.enabled = true,
	};
	ch[1] = (struct lwan_channel){
		.frequency = RU864_FREQ_CH1,
		.min_dr = 0,
		.max_dr = 5,
		.enabled = true,
	};

	*count = RU864_DEFAULT_CH_COUNT;
	return 0;
}

static int ru864_get_tx_params(uint8_t dr, uint8_t tx_power_idx, struct lwan_dr_params *p,
			       int8_t *power_dbm)
{
	uint8_t idx;

	if (dr >= RU864_DR_COUNT) {
		return -EINVAL;
	}

	/* An index past the table steps the power down no further than its end. */
	idx = MIN(tx_power_idx, RU864_MAX_TX_POWER_IDX);

	*p = ru864_dr_table[dr];
	*power_dbm = RU864_MAX_EIRP_DBM - (int8_t)(2 * idx);
	return 0;
}

static int ru864_get_rx1_params(uint32_t tx_freq, uint8_t tx_dr, uint8_t offset, uint32_t *rx1_freq,
				struct lwan_dr_params *p)
{
	uint8_t rx1_dr;

	if (tx_dr >= RU864_DR_COUNT || offset > RU864_MAX_RX1_DR_OFFSET) {
		return -EINVAL;
	}

	/* RX1 uses the channel the uplink went out on (3.13.7). */
	*rx1_freq = tx_freq;

	rx1_dr = (tx_dr > offset) ? (tx_dr - offset) : 0;

	*p = ru864_dr_table[rx1_dr];
	return 0;
}

static int ru864_get_rx2_params(uint8_t dr, uint32_t *freq, struct lwan_dr_params *p)
{
	if (dr >= RU864_DR_COUNT) {
		return -EINVAL;
	}

	*freq = RU864_RX2_FREQ;
	*p = ru864_dr_table[dr];
	return 0;
}

static int ru864_validate_dl_settings(uint8_t rx1_dr_offset, uint8_t rx2_datarate)
{
	if (rx1_dr_offset > RU864_MAX_RX1_DR_OFFSET) {
		return -EINVAL;
	}

	if (rx2_datarate >= RU864_DR_COUNT) {
		return -EINVAL;
	}

	return 0;
}

static int ru864_apply_cflist(const uint8_t cflist[LWAN_CFLIST_SIZE], struct lwan_channel *ch,
			      size_t *count)
{
	uint8_t cflist_type;
	size_t cflist_end;

	cflist_type = cflist[LWAN_CFLIST_TYPE_OFFSET];

	/*
	 * 3.13.4 lets the band take a type 1 CFList as well, read against the
	 * 800 MHz fixed channel list of 3.3.1.1. 3.3.1 words that as a MAY for
	 * the dynamic channel plan regions and no region in this backend
	 * offers it yet, so a type 1 list is left unapplied rather than
	 * half-applied.
	 */
	if (cflist_type != LWAN_CFLIST_TYPE_FREQ) {
		LOG_WRN("Unsupported CFList type: %u", cflist_type);
		return 0;
	}

	/*
	 * CFList type 0: 5 frequencies encoded as 3-byte little-endian values
	 * in units of 100 Hz.
	 */
	for (int i = 0; i < LWAN_CFLIST_ENTRIES; i++) {
		uint32_t freq;
		size_t idx = RU864_DEFAULT_CH_COUNT + i;

		if (idx >= LWAN_MAX_CHANNELS) {
			break;
		}

		freq = sys_get_le24(&cflist[i * 3]);
		freq *= LWAN_CFLIST_FREQ_STEP_HZ;

		/*
		 * Clear the whole descriptor: a stale frequency from an
		 * earlier session would make the ADR channel mask treat the
		 * slot as a channel the network knows about.
		 */
		if (freq == 0) {
			ch[idx] = (struct lwan_channel){0};
			continue;
		}

		if (freq < RU864_FREQ_MIN || freq > RU864_FREQ_MAX) {
			LOG_WRN("CFList frequency %u Hz outside the plan", freq);
			ch[idx] = (struct lwan_channel){0};
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
	cflist_end = RU864_DEFAULT_CH_COUNT + LWAN_CFLIST_ENTRIES;

	if (cflist_end > LWAN_MAX_CHANNELS) {
		cflist_end = LWAN_MAX_CHANNELS;
	}
	if (*count < cflist_end) {
		*count = cflist_end;
	}

	return 0;
}

/*
 * ChMaskCntl encoding (Table 106):
 *   0..4 = ChMask applies to a block of 16 channels starting at cntl * 16.
 *   5    = the 10 LSBs each switch a bank of 8 channels; the 6 MSBs are RFU.
 *   6    = all defined channels on, whatever the mask says.
 *   7    = RFU.
 */
#define RU864_CH_MASK_CNTL_BLOCK_MAX 4
#define RU864_CH_MASK_CNTL_BANKS     5
#define RU864_CH_MASK_CNTL_ALL_ON    6

#define RU864_CH_BLOCK_SIZE 16
#define RU864_CH_BANK_SIZE  8
#define RU864_CH_BANK_COUNT 10

static int ru864_validate_dr(uint8_t dr)
{
	if (dr >= RU864_DR_COUNT) {
		return -EINVAL;
	}

	return 0;
}

static int ru864_validate_tx_power(uint8_t tx_power_idx)
{
	if (tx_power_idx > RU864_MAX_TX_POWER_IDX) {
		return -EINVAL;
	}

	return 0;
}

/* Count the channels a proposed mask would leave usable. */
static uint8_t ru864_count_enabled(const struct lwan_channel *ch, size_t count, size_t first,
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

static int ru864_apply_adr_channel_mask(struct lwan_channel *ch, size_t count, uint8_t ch_mask_cntl,
					uint16_t ch_mask)
{
	size_t first;
	size_t span;

	if (ch_mask_cntl == RU864_CH_MASK_CNTL_ALL_ON) {
		for (size_t i = 0; i < count; i++) {
			if (ch[i].frequency != 0) {
				ch[i].enabled = true;
			}
		}
		return 0;
	}

	if (ch_mask_cntl == RU864_CH_MASK_CNTL_BANKS) {
		uint16_t banks_present = 0;
		uint8_t enabled = 0;

		/* The 6 MSBs are RFU and a set bit makes the command invalid. */
		if ((ch_mask >> RU864_CH_BANK_COUNT) != 0) {
			return -EINVAL;
		}

		for (size_t i = 0; i < count; i++) {
			size_t bank = i / RU864_CH_BANK_SIZE;

			if (ch[i].frequency == 0 || bank >= RU864_CH_BANK_COUNT) {
				continue;
			}
			banks_present |= BIT(bank);
			if (ch_mask & BIT(bank)) {
				enabled++;
			}
		}

		/* A bank the device has no channel in is a yet undefined one. */
		if ((ch_mask & ~banks_present) != 0) {
			return -EINVAL;
		}

		/*
		 * The two join channels are the only way back to the network,
		 * so refuse a request that would take one of them away.
		 */
		for (size_t i = 0; i < MIN(count, (size_t)RU864_DEFAULT_CH_COUNT); i++) {
			if ((ch_mask & BIT(i / RU864_CH_BANK_SIZE)) == 0) {
				return -EINVAL;
			}
		}

		if (enabled == 0) {
			return -EINVAL;
		}

		for (size_t i = 0; i < count; i++) {
			size_t bank = i / RU864_CH_BANK_SIZE;

			if (ch[i].frequency == 0 || bank >= RU864_CH_BANK_COUNT) {
				continue;
			}
			ch[i].enabled = (ch_mask & BIT(bank)) != 0;
		}

		return 0;
	}

	if (ch_mask_cntl > RU864_CH_MASK_CNTL_BLOCK_MAX) {
		return -EINVAL;
	}

	first = (size_t)ch_mask_cntl * RU864_CH_BLOCK_SIZE;
	if (first >= count) {
		/* The block addresses channels this device does not have. */
		return -EINVAL;
	}

	span = MIN((size_t)RU864_CH_BLOCK_SIZE, count - first);

	/* A mask that turns on a channel the device does not have is invalid. */
	for (size_t i = 0; i < RU864_CH_BLOCK_SIZE; i++) {
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
	for (size_t i = 0; i < MIN(count, (size_t)RU864_DEFAULT_CH_COUNT); i++) {
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
	if (ru864_count_enabled(ch, count, first, span, ch_mask) == 0) {
		return -EINVAL;
	}

	/* Validated, commit. */
	for (size_t i = 0; i < span; i++) {
		if (ch[first + i].frequency == 0) {
			continue;
		}
		ch[first + i].enabled = (ch_mask & BIT(i)) != 0;
	}

	return 0;
}

static int ru864_select_channel(const struct lwan_channel *ch, size_t count, size_t limit,
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

	if (!ru864_band_available()) {
		int64_t wait = ru864_available_at - k_uptime_get() + 1;

		*delay_ms = (wait > 0) ? (int32_t)wait : 1;
		return -ENOBUFS;
	}

	*freq = ch[candidates[sys_rand8_get() % candidate_count]].frequency;

	return 0;
}

static int ru864_select_data_channel(const struct lwan_channel *ch, size_t count, uint8_t dr,
				     uint32_t *freq, int32_t *delay_ms)
{
	return ru864_select_channel(ch, count, LWAN_MAX_CHANNELS, dr, freq, delay_ms);
}

static int ru864_select_join_channel(const struct lwan_channel *ch, size_t count, uint32_t *freq,
				     uint8_t *dr, int32_t *delay_ms)
{
	int ret;

	/* Only the two default channels carry a Join-Request (Table 101). */
	ret = ru864_select_channel(ch, count, RU864_DEFAULT_CH_COUNT, RU864_JOIN_DR, freq,
				   delay_ms);
	if (ret != 0 && ret != -ENOBUFS) {
		return ret;
	}

	*dr = RU864_JOIN_DR;

	return ret;
}

static void ru864_record_tx(uint32_t freq, uint32_t airtime_ms)
{
	uint32_t off_time = airtime_ms * (RU864_DUTY_CYCLE_INV - 1);

	ARG_UNUSED(freq);

	ru864_available_at = k_uptime_get() + off_time;

	LOG_DBG("Band off-time %u ms (airtime %u ms, dc 1/%u)", off_time, airtime_ms,
		RU864_DUTY_CYCLE_INV);
}

const struct lwan_region_ops ru864_ops = {
	.get_default_channels = ru864_get_default_channels,
	.get_tx_params = ru864_get_tx_params,
	.get_rx1_params = ru864_get_rx1_params,
	.get_rx2_params = ru864_get_rx2_params,
	.validate_dl_settings = ru864_validate_dl_settings,
	.apply_cflist = ru864_apply_cflist,
	.validate_dr = ru864_validate_dr,
	.validate_tx_power = ru864_validate_tx_power,
	.apply_adr_channel_mask = ru864_apply_adr_channel_mask,
	.select_join_channel = ru864_select_join_channel,
	.select_data_channel = ru864_select_data_channel,
	.record_tx = ru864_record_tx,
};
