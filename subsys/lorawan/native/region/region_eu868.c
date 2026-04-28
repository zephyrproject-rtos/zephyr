/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * EU868 regional parameters from LoRaWAN Regional Parameters RP002-1.0.4
 * https://resources.lora-alliance.org/technical-specifications/rp002-1-0-4-regional-parameters
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include "region.h"
#include "lorawan.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_native_eu868, CONFIG_LORAWAN_LOG_LEVEL);

/* EU868 default join channels */
#define EU868_FREQ_CH0		KHZ(868100) /* 868.1 MHz */
#define EU868_FREQ_CH1		KHZ(868300) /* 868.3 MHz */
#define EU868_FREQ_CH2		KHZ(868500) /* 868.5 MHz */

/* EU868 RX2 defaults */
#define EU868_RX2_FREQ		KHZ(869525) /* 869.525 MHz */
#define EU868_RX2_DR		0         /* DR0 = SF12/125kHz */

/* EU868 max EIRP */
#define EU868_MAX_EIRP_DBM	16

/* EU868 mandatory default channels */
#define EU868_DEFAULT_CH_COUNT	3

/* CFList type 0: number of extra frequencies */
#define CFLIST_CH_COUNT		5

/* CFList frequency unit (100 Hz) */
#define CFLIST_FREQ_STEP_HZ	100

/* EU868 datarate table: DR0..DR6 */
static const struct lwan_dr_params eu868_dr_table[] = {
	[0] = { .sf = SF_12, .bw = BW_125_KHZ, .max_payload = 51 },
	[1] = { .sf = SF_11, .bw = BW_125_KHZ, .max_payload = 51 },
	[2] = { .sf = SF_10, .bw = BW_125_KHZ, .max_payload = 51 },
	[3] = { .sf = SF_9,  .bw = BW_125_KHZ, .max_payload = 115 },
	[4] = { .sf = SF_8,  .bw = BW_125_KHZ, .max_payload = 222 },
	[5] = { .sf = SF_7,  .bw = BW_125_KHZ, .max_payload = 222 },
	[6] = { .sf = SF_7,  .bw = BW_250_KHZ, .max_payload = 222 },
};

#define EU868_DR_COUNT		ARRAY_SIZE(eu868_dr_table)

/*
 * ETSI sub-band definitions for EU868.
 *
 * Each sub-band has a frequency range and a maximum duty cycle.
 * duty_cycle_inv is the inverse of the duty cycle (e.g. 100 for 1%).
 * The off-time after a TX of airtime T is: T * (duty_cycle_inv - 1).
 */
#define EU868_SUB_BAND_COUNT	5

struct eu868_sub_band {
	uint32_t freq_min;
	uint32_t freq_max;
	uint16_t duty_cycle_inv;
	int64_t available_at;
};

static struct eu868_sub_band eu868_sub_bands[EU868_SUB_BAND_COUNT] = {
	{ .freq_min = KHZ(863000), .freq_max = KHZ(868000), .duty_cycle_inv = 100 },
	{ .freq_min = KHZ(868000), .freq_max = KHZ(868600), .duty_cycle_inv = 100 },
	{ .freq_min = KHZ(868700), .freq_max = KHZ(869200), .duty_cycle_inv = 1000 },
	{ .freq_min = KHZ(869400), .freq_max = KHZ(869650), .duty_cycle_inv = 10 },
	{ .freq_min = KHZ(869700), .freq_max = KHZ(870000), .duty_cycle_inv = 100 },
};

static int eu868_get_sub_band(uint32_t freq)
{
	for (int i = 0; i < EU868_SUB_BAND_COUNT; i++) {
		if (freq >= eu868_sub_bands[i].freq_min &&
		    freq < eu868_sub_bands[i].freq_max) {
			return i;
		}
	}

	return -1;
}

static bool eu868_channel_available(uint32_t freq)
{
	int sb = eu868_get_sub_band(freq);

	if (sb < 0) {
		return true;
	}

	return eu868_sub_bands[sb].available_at <= k_uptime_get();
}

static int eu868_get_default_channels(struct lwan_channel *ch, size_t *count)
{
	if (*count < EU868_DEFAULT_CH_COUNT) {
		return -ENOMEM;
	}

	ch[0] = (struct lwan_channel){
		.frequency = EU868_FREQ_CH0,
		.min_dr = 0,
		.max_dr = 5,
		.enabled = true,
	};
	ch[1] = (struct lwan_channel){
		.frequency = EU868_FREQ_CH1,
		.min_dr = 0,
		.max_dr = 5,
		.enabled = true,
	};
	ch[2] = (struct lwan_channel){
		.frequency = EU868_FREQ_CH2,
		.min_dr = 0,
		.max_dr = 5,
		.enabled = true,
	};

	*count = EU868_DEFAULT_CH_COUNT;
	return 0;
}

static int eu868_get_tx_params(uint8_t dr, struct lwan_dr_params *p,
			       int8_t *power)
{
	if (dr >= EU868_DR_COUNT) {
		return -EINVAL;
	}

	*p = eu868_dr_table[dr];
	*power = EU868_MAX_EIRP_DBM;
	return 0;
}

static int eu868_get_rx1_params(uint32_t tx_freq, uint8_t tx_dr,
				uint8_t offset, uint32_t *rx1_freq,
				struct lwan_dr_params *p)
{
	uint8_t rx1_dr;

	/* EU868: RX1 frequency = TX frequency */
	*rx1_freq = tx_freq;

	/* RX1 DR = max(0, tx_dr - offset) */
	if (tx_dr >= offset) {
		rx1_dr = tx_dr - offset;
	} else {
		rx1_dr = 0;
	}

	if (rx1_dr >= EU868_DR_COUNT) {
		rx1_dr = EU868_DR_COUNT - 1;
	}

	*p = eu868_dr_table[rx1_dr];
	return 0;
}

static int eu868_get_rx2_params(uint8_t dr, uint32_t *freq,
				struct lwan_dr_params *p)
{
	if (dr >= EU868_DR_COUNT) {
		return -EINVAL;
	}

	*freq = EU868_RX2_FREQ;
	*p = eu868_dr_table[dr];
	return 0;
}

#define EU868_MAX_RX1_DR_OFFSET	5

static int eu868_validate_dl_settings(uint8_t rx1_dr_offset,
				      uint8_t rx2_datarate)
{
	if (rx1_dr_offset > EU868_MAX_RX1_DR_OFFSET) {
		return -EINVAL;
	}

	if (rx2_datarate >= EU868_DR_COUNT) {
		return -EINVAL;
	}

	return 0;
}

static int eu868_apply_cflist(const uint8_t cflist[16],
			      struct lwan_channel *ch, size_t *count)
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
	 * values (in units of 100 Hz).
	 */
	for (int i = 0; i < CFLIST_CH_COUNT; i++) {
		uint32_t freq;
		size_t idx = EU868_DEFAULT_CH_COUNT + i;

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
	cflist_end = EU868_DEFAULT_CH_COUNT + CFLIST_CH_COUNT;

	if (cflist_end > LWAN_MAX_CHANNELS) {
		cflist_end = LWAN_MAX_CHANNELS;
	}
	if (*count < cflist_end) {
		*count = cflist_end;
	}

	return 0;
}

static int eu868_select_data_channel(const struct lwan_channel *ch,
				     size_t count, uint8_t dr,
				     uint32_t *freq, int32_t *delay_ms)
{
	uint8_t enabled[LWAN_MAX_CHANNELS];
	uint8_t enabled_count = 0;
	uint8_t dr_match_count = 0;
	int64_t earliest = INT64_MAX;
	uint8_t idx;

	for (size_t i = 0; i < count; i++) {
		if (!ch[i].enabled || dr < ch[i].min_dr || dr > ch[i].max_dr) {
			continue;
		}

		dr_match_count++;

		if (eu868_channel_available(ch[i].frequency)) {
			enabled[enabled_count++] = i;
		} else {
			int sb = eu868_get_sub_band(ch[i].frequency);

			if (sb >= 0 && eu868_sub_bands[sb].available_at < earliest) {
				earliest = eu868_sub_bands[sb].available_at;
			}
		}
	}

	if (enabled_count == 0) {
		if (dr_match_count > 0) {
			int64_t wait = earliest - k_uptime_get() + 1;

			*delay_ms = (wait > 0) ? (int32_t)wait : 1;
			return -ENOBUFS;
		}
		return -ENOENT;
	}

	idx = enabled[sys_rand8_get() % enabled_count];

	*freq = ch[idx].frequency;

	return 0;
}

static int eu868_select_join_channel(const struct lwan_channel *ch,
				     size_t count, uint32_t *freq,
				     uint8_t *dr, int32_t *delay_ms)
{
	uint8_t enabled[EU868_DEFAULT_CH_COUNT];
	uint8_t enabled_count = 0;
	uint8_t ch_match_count = 0;
	int64_t earliest = INT64_MAX;
	uint8_t idx;

	/*
	 * For join requests, only the 3 default channels
	 * (868.1, 868.3, 868.5) may be used, at DR0-DR5.
	 * Select randomly among enabled default channels.
	 */
	for (size_t i = 0; i < MIN(count, (size_t)EU868_DEFAULT_CH_COUNT); i++) {
		if (!ch[i].enabled) {
			continue;
		}

		ch_match_count++;

		if (eu868_channel_available(ch[i].frequency)) {
			enabled[enabled_count++] = i;
		} else {
			int sb = eu868_get_sub_band(ch[i].frequency);

			if (sb >= 0 && eu868_sub_bands[sb].available_at < earliest) {
				earliest = eu868_sub_bands[sb].available_at;
			}
		}
	}

	if (enabled_count == 0) {
		if (ch_match_count > 0) {
			int64_t wait = earliest - k_uptime_get() + 1;

			*delay_ms = (wait > 0) ? (int32_t)wait : 1;
			return -ENOBUFS;
		}
		return -ENOENT;
	}

	idx = enabled[sys_rand8_get() % enabled_count];

	*freq = ch[idx].frequency;
	/* Use DR0 for join requests (maximum range) */
	*dr = 0;

	return 0;
}

static void eu868_record_tx(uint32_t freq, uint32_t airtime_ms)
{
	uint32_t off_time;
	int sb;

	sb = eu868_get_sub_band(freq);
	if (sb < 0) {
		return;
	}

	off_time = airtime_ms * (eu868_sub_bands[sb].duty_cycle_inv - 1);

	eu868_sub_bands[sb].available_at = k_uptime_get() + off_time;

	LOG_DBG("Sub-band %d: off-time %u ms (airtime %u ms, dc 1/%u)",
		sb, off_time, airtime_ms, eu868_sub_bands[sb].duty_cycle_inv);
}

const struct lwan_region_ops eu868_ops = {
	.get_default_channels = eu868_get_default_channels,
	.get_tx_params = eu868_get_tx_params,
	.get_rx1_params = eu868_get_rx1_params,
	.get_rx2_params = eu868_get_rx2_params,
	.validate_dl_settings = eu868_validate_dl_settings,
	.apply_cflist = eu868_apply_cflist,
	.select_join_channel = eu868_select_join_channel,
	.select_data_channel = eu868_select_data_channel,
	.record_tx = eu868_record_tx,
};
