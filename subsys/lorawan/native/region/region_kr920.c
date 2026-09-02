/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * KR920 regional parameters from LoRaWAN Regional Parameters RP002-1.0.5
 * https://resources.lora-alliance.org/technical-specifications/rp002-1-0-5-regional-parameters
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include "region.h"
#include <lorawan.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_native_kr920, CONFIG_LORAWAN_LOG_LEVEL);

/* KR920 default join channels */
#define KR920_FREQ_CH0 KHZ(922100) /* 922.1 MHz */
#define KR920_FREQ_CH1 KHZ(922300) /* 922.3 MHz */
#define KR920_FREQ_CH2 KHZ(922500) /* 922.5 MHz */

/* KR920 RX2 defaults */
#define KR920_RX2_FREQ KHZ(921900) /* 921.9 MHz */
#define KR920_RX2_DR   0           /* DR0 = SF12/125kHz */

/* Ends of the channel plan */
#define KR920_FREQ_MIN KHZ(920900) /* 920.9 MHz */
#define KR920_FREQ_MAX KHZ(923300) /* 923.3 MHz */

/*
 * The plan carries two EIRP limits (3.11.3): a channel below 922 MHz is
 * held to 10 dBm whatever power the network asked for, the rest may
 * reach the 14 dBm the band defaults to.
 */
#define KR920_MAX_EIRP_DBM   14
#define KR920_LOW_EIRP_DBM   10
#define KR920_LOW_EIRP_LIMIT KHZ(922000)

/*
 * KR920 TXPower table (RP002-1.0.5, Table 83):
 *   index 0 = MaxEIRP, step -2 dB per index, up to index 7.
 */
#define KR920_MAX_TX_POWER_IDX 7

/* KR920 mandatory default channels */
#define KR920_DEFAULT_CH_COUNT 3

/* Highest datarate a KR920 channel carries */
#define KR920_CH_MAX_DR 5

/* CFList type 0: number of extra frequencies */
#define CFLIST_CH_COUNT 5

/* CFList frequency unit (100 Hz) */
#define CFLIST_FREQ_STEP_HZ 100

/*
 * The band reaches the medium by listening before it talks rather than by
 * holding to a duty cycle (3.11.2), so there is no off-time to track.
 *
 * RP002 carries no numbers for it -- 3.11.10 says the band has no default
 * settings of its own -- so these come from Korean radio regulation. Both
 * Semtech reference stacks have long shipped 200 kHz, measured over a
 * slice wider than the 125 kHz channel being cleared, though they disagree
 * on whether that figure is one sideband or two. It is taken here as both,
 * because reading it as one sideband would sweep 400 kHz and call a
 * channel busy on its neighbour's traffic.
 */
#define KR920_LBT_THRESHOLD_DBM (-65)
#define KR920_LBT_SCAN_TIME_MS  6
#define KR920_LBT_BANDWIDTH_HZ  200000 /* both sidebands */
#define KR920_LBT_MAX_RETRIES   7

/*
 * KR920 datarate table: DR0..DR5, the set every end-device has to carry
 * (3.11.3).  DR12 and DR13 (SF6 and SF5) were added in RP002-1.0.5 and are
 * optional, so they are left out for now; DR6 to DR11 and DR14 are RFU.
 * The band defines no FSK datarate at all.
 */
static const struct lwan_dr_params kr920_dr_table[] = {
	[0] = {.sf = SF_12, .bw = BW_125_KHZ, .max_payload = 51},
	[1] = {.sf = SF_11, .bw = BW_125_KHZ, .max_payload = 51},
	[2] = {.sf = SF_10, .bw = BW_125_KHZ, .max_payload = 51},
	[3] = {.sf = SF_9, .bw = BW_125_KHZ, .max_payload = 115},
	[4] = {.sf = SF_8, .bw = BW_125_KHZ, .max_payload = 222},
	[5] = {.sf = SF_7, .bw = BW_125_KHZ, .max_payload = 222},
};

#define KR920_DR_COUNT ARRAY_SIZE(kr920_dr_table)

static int kr920_get_default_channels(struct lwan_channel *ch, size_t *count)
{
	static const uint32_t freqs[KR920_DEFAULT_CH_COUNT] = {
		KR920_FREQ_CH0,
		KR920_FREQ_CH1,
		KR920_FREQ_CH2,
	};

	if (*count < KR920_DEFAULT_CH_COUNT) {
		return -ENOMEM;
	}

	for (int i = 0; i < KR920_DEFAULT_CH_COUNT; i++) {
		ch[i] = (struct lwan_channel){
			.frequency = freqs[i],
			.min_dr = 0,
			.max_dr = KR920_CH_MAX_DR,
			.enabled = true,
		};
	}

	*count = KR920_DEFAULT_CH_COUNT;
	return 0;
}

static int kr920_get_tx_params(uint8_t dr, uint8_t tx_power_idx, struct lwan_dr_params *p,
			       int8_t *power_dbm)
{
	uint8_t idx;

	if (dr >= KR920_DR_COUNT) {
		return -EINVAL;
	}

	/* Defensive clamp — validate_tx_power() is the authoritative check. */
	idx = MIN(tx_power_idx, KR920_MAX_TX_POWER_IDX);

	*p = kr920_dr_table[dr];
	*power_dbm = KR920_MAX_EIRP_DBM - (int8_t)(2 * idx);
	return 0;
}

static void kr920_clamp_tx_power(uint32_t freq, int8_t *power_dbm)
{
	if (freq >= KR920_LOW_EIRP_LIMIT) {
		return;
	}

	if (*power_dbm > KR920_LOW_EIRP_DBM) {
		*power_dbm = KR920_LOW_EIRP_DBM;
	}
}

static int kr920_get_lbt_params(uint32_t freq, uint32_t *bandwidth_hz, int16_t *threshold_dbm,
				uint32_t *scan_time_ms)
{
	ARG_UNUSED(freq);

	*bandwidth_hz = KR920_LBT_BANDWIDTH_HZ;
	*threshold_dbm = KR920_LBT_THRESHOLD_DBM;
	*scan_time_ms = KR920_LBT_SCAN_TIME_MS;

	return 0;
}

static int kr920_get_rx1_params(uint32_t tx_freq, uint8_t tx_dr, uint8_t offset, uint32_t *rx1_freq,
				struct lwan_dr_params *p)
{
	uint8_t rx1_dr;

	/* KR920: RX1 frequency = TX frequency */
	*rx1_freq = tx_freq;

	/* RX1 DR = max(0, tx_dr - offset) */
	if (tx_dr >= offset) {
		rx1_dr = tx_dr - offset;
	} else {
		rx1_dr = 0;
	}

	if (rx1_dr >= KR920_DR_COUNT) {
		rx1_dr = KR920_DR_COUNT - 1;
	}

	*p = kr920_dr_table[rx1_dr];
	return 0;
}

static int kr920_get_rx2_params(uint8_t dr, uint32_t *freq, struct lwan_dr_params *p)
{
	if (dr >= KR920_DR_COUNT) {
		return -EINVAL;
	}

	*freq = KR920_RX2_FREQ;
	*p = kr920_dr_table[dr];
	return 0;
}

#define KR920_MAX_RX1_DR_OFFSET 5

static int kr920_validate_dl_settings(uint8_t rx1_dr_offset, uint8_t rx2_datarate)
{
	if (rx1_dr_offset > KR920_MAX_RX1_DR_OFFSET) {
		return -EINVAL;
	}

	if (rx2_datarate >= KR920_DR_COUNT) {
		return -EINVAL;
	}

	return 0;
}

static int kr920_apply_cflist(const uint8_t cflist[16], struct lwan_channel *ch, size_t *count)
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
		size_t idx = KR920_DEFAULT_CH_COUNT + i;

		if (idx >= LWAN_MAX_CHANNELS) {
			break;
		}

		freq = sys_get_le24(&cflist[i * 3]);
		freq *= CFLIST_FREQ_STEP_HZ;

		if (freq == 0) {
			ch[idx].enabled = false;
			continue;
		}

		if (freq < KR920_FREQ_MIN || freq > KR920_FREQ_MAX) {
			LOG_WRN("CFList frequency %u Hz outside the plan", freq);
			ch[idx].enabled = false;
			continue;
		}

		ch[idx] = (struct lwan_channel){
			.frequency = freq,
			.min_dr = 0,
			.max_dr = KR920_CH_MAX_DR,
			.enabled = true,
		};
	}

	/* Extend count to cover all CFList slots so disabled entries are visible */
	cflist_end = KR920_DEFAULT_CH_COUNT + CFLIST_CH_COUNT;

	if (cflist_end > LWAN_MAX_CHANNELS) {
		cflist_end = LWAN_MAX_CHANNELS;
	}
	if (*count < cflist_end) {
		*count = cflist_end;
	}

	return 0;
}

/*
 * KR920 ChMaskCntl encoding (RP002-1.0.5, Table 84):
 *   0..4 = ChMask applies to a bank of 16 channels starting at cntl * 16.
 *   5    = the 10 LSBs each control a block of 8 channels; 6 MSBs are RFU.
 *   6    = "all defined channels on"; the mask value is ignored.
 *   7    = RFU, and the spec asks that it be rejected.
 */
#define KR920_CH_MASK_CNTL_BANK_MAX 4
#define KR920_CH_MASK_CNTL_BLOCKS   5
#define KR920_CH_MASK_CNTL_ALL_ON   6

#define KR920_CH_MASK_BANK_SIZE   16
#define KR920_CH_MASK_BLOCK_SIZE  8
#define KR920_CH_MASK_BLOCK_COUNT 10

static int kr920_validate_dr(uint8_t dr)
{
	if (dr >= KR920_DR_COUNT) {
		return -EINVAL;
	}

	return 0;
}

static int kr920_validate_tx_power(uint8_t tx_power_idx)
{
	if (tx_power_idx > KR920_MAX_TX_POWER_IDX) {
		return -EINVAL;
	}

	return 0;
}

/*
 * Resolve one bit of the request onto a channel index, or SIZE_MAX when the
 * bit addresses nothing this device has.
 */
static size_t kr920_mask_channel(uint8_t ch_mask_cntl, uint8_t bit, uint8_t sub)
{
	if (ch_mask_cntl == KR920_CH_MASK_CNTL_BLOCKS) {
		return (size_t)bit * KR920_CH_MASK_BLOCK_SIZE + sub;
	}

	return (size_t)ch_mask_cntl * KR920_CH_MASK_BANK_SIZE + bit;
}

static int kr920_apply_adr_channel_mask(struct lwan_channel *ch, size_t count, uint8_t ch_mask_cntl,
					uint16_t ch_mask)
{
	uint8_t bits;
	uint8_t sub_count;
	uint8_t enabled_count;

	if (ch_mask_cntl == KR920_CH_MASK_CNTL_ALL_ON) {
		for (size_t i = 0; i < count; i++) {
			if (ch[i].frequency != 0) {
				ch[i].enabled = true;
			}
		}
		return 0;
	}

	if (ch_mask_cntl > KR920_CH_MASK_CNTL_BLOCKS) {
		return -EINVAL;
	}

	if (ch_mask_cntl == KR920_CH_MASK_CNTL_BLOCKS) {
		/* The 6 bits above the ten block bits are reserved. */
		if (ch_mask >> KR920_CH_MASK_BLOCK_COUNT) {
			return -EINVAL;
		}
		bits = KR920_CH_MASK_BLOCK_COUNT;
		sub_count = KR920_CH_MASK_BLOCK_SIZE;
	} else {
		bits = KR920_CH_MASK_BANK_SIZE;
		sub_count = 1;
	}

	/*
	 * A control value naming a bank this device has no channels in asks
	 * about nothing at all. Committing it would leave the network holding
	 * a channel set the device never adopted, so turn it down.
	 */
	if (kr920_mask_channel(ch_mask_cntl, 0, 0) >= count) {
		return -EINVAL;
	}

	/*
	 * The three join channels are the only way back to the network, so
	 * refuse a request that would take one of them away.
	 */
	for (uint8_t b = 0; b < bits; b++) {
		for (uint8_t sub = 0; sub < sub_count; sub++) {
			size_t idx = kr920_mask_channel(ch_mask_cntl, b, sub);

			if (idx >= KR920_DEFAULT_CH_COUNT || idx >= count) {
				continue;
			}
			if ((ch_mask & BIT(b)) == 0) {
				return -EINVAL;
			}
		}
	}

	/*
	 * Count what the request would leave enabled before touching ch[]:
	 * a request that silences the device has to be rejected whole.
	 */
	enabled_count = 0;
	for (size_t i = 0; i < count; i++) {
		bool covered = false;
		bool on = ch[i].enabled;

		if (ch[i].frequency == 0) {
			continue;
		}

		for (uint8_t b = 0; b < bits && !covered; b++) {
			for (uint8_t sub = 0; sub < sub_count; sub++) {
				if (kr920_mask_channel(ch_mask_cntl, b, sub) == i) {
					covered = true;
					on = (ch_mask & BIT(b)) != 0;
					break;
				}
			}
		}

		if (on) {
			enabled_count++;
		}
	}

	if (enabled_count == 0) {
		return -EINVAL;
	}

	/* Validated — commit. */
	for (uint8_t b = 0; b < bits; b++) {
		for (uint8_t sub = 0; sub < sub_count; sub++) {
			size_t idx = kr920_mask_channel(ch_mask_cntl, b, sub);

			if (idx >= count || ch[idx].frequency == 0) {
				continue;
			}
			ch[idx].enabled = (ch_mask & BIT(b)) != 0;
		}
	}

	return 0;
}

/*
 * Listen before talk decides whether a channel may be used, and that
 * happens once one has been picked, so selection here only has to find an
 * enabled channel. Neither selector ever reports -ENOBUFS.
 */
static int kr920_select_data_channel(const struct lwan_channel *ch, size_t count, uint8_t dr,
				     uint32_t *freq, int32_t *delay_ms)
{
	uint8_t enabled[LWAN_MAX_CHANNELS];
	uint8_t enabled_count = 0;
	uint8_t idx;

	ARG_UNUSED(delay_ms);

	for (size_t i = 0; i < count; i++) {
		if (!ch[i].enabled || dr < ch[i].min_dr || dr > ch[i].max_dr) {
			continue;
		}

		enabled[enabled_count++] = i;
	}

	if (enabled_count == 0) {
		return -ENOENT;
	}

	idx = enabled[sys_rand8_get() % enabled_count];

	*freq = ch[idx].frequency;

	return 0;
}

static int kr920_select_join_channel(const struct lwan_channel *ch, size_t count, uint32_t *freq,
				     uint8_t *dr, int32_t *delay_ms)
{
	uint8_t enabled[KR920_DEFAULT_CH_COUNT];
	uint8_t enabled_count = 0;
	uint8_t idx;

	ARG_UNUSED(delay_ms);

	/*
	 * For join requests, only the 3 default channels
	 * (922.1, 922.3, 922.5) may be used, at DR0-DR5.
	 * Select randomly among enabled default channels.
	 */
	for (size_t i = 0; i < MIN(count, (size_t)KR920_DEFAULT_CH_COUNT); i++) {
		if (!ch[i].enabled) {
			continue;
		}

		enabled[enabled_count++] = i;
	}

	if (enabled_count == 0) {
		return -ENOENT;
	}

	idx = enabled[sys_rand8_get() % enabled_count];

	*freq = ch[idx].frequency;
	/* Use DR0 for join requests (maximum range) */
	*dr = 0;

	return 0;
}

static void kr920_record_tx(uint32_t freq, uint32_t airtime_ms)
{
	ARG_UNUSED(freq);
	ARG_UNUSED(airtime_ms);

	/* Listen before talk gates the next transmission, not an off-time. */
}

const struct lwan_region_ops kr920_ops = {
	.get_default_channels = kr920_get_default_channels,
	.get_tx_params = kr920_get_tx_params,
	.clamp_tx_power = kr920_clamp_tx_power,
	.get_lbt_params = kr920_get_lbt_params,
	.max_lbt_retries = KR920_LBT_MAX_RETRIES,
	.get_rx1_params = kr920_get_rx1_params,
	.get_rx2_params = kr920_get_rx2_params,
	.validate_dl_settings = kr920_validate_dl_settings,
	.apply_cflist = kr920_apply_cflist,
	.validate_dr = kr920_validate_dr,
	.validate_tx_power = kr920_validate_tx_power,
	.apply_adr_channel_mask = kr920_apply_adr_channel_mask,
	.select_join_channel = kr920_select_join_channel,
	.select_data_channel = kr920_select_data_channel,
	.record_tx = kr920_record_tx,
};
