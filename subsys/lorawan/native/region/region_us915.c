/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * US902-928 regional parameters from LoRaWAN Regional Parameters RP002-1.0.5,
 * section 3.5
 * https://resources.lora-alliance.org/technical-specifications/rp002-1-0-5-regional-parameters
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>

#include "region.h"
#include <lorawan.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_native_us915, CONFIG_LORAWAN_LOG_LEVEL);

/* Uplink channel plan (3.5.2) */
#define US915_UL_125_BASE_HZ  KHZ(902300)
#define US915_UL_125_STEP_HZ  KHZ(200)
#define US915_UL_125_CH_COUNT 64

#define US915_UL_500_BASE_HZ  KHZ(903000)
#define US915_UL_500_STEP_HZ  KHZ(1600)
#define US915_UL_500_CH_COUNT 8

#define US915_UL_CH_COUNT (US915_UL_125_CH_COUNT + US915_UL_500_CH_COUNT)

/* Downlink channel plan (3.5.2) */
#define US915_DL_BASE_HZ  KHZ(923300)
#define US915_DL_STEP_HZ  KHZ(600)
#define US915_DL_CH_COUNT 8

/* RX2 defaults (3.5.7) */
#define US915_RX2_FREQ US915_DL_BASE_HZ
#define US915_RX2_DR   8 /* DR8 = SF12/500kHz, in the downlink datarate space */

/*
 * TXPower table (Table 22): index 0 is the 30 dBm conducted maximum and each
 * further index drops 2 dB, down to 2 dBm at index 14.
 */
#define US915_MAX_POWER_DBM    30
#define US915_MAX_TX_POWER_IDX 14

/* The FCC digital transmission rule on the 500 kHz channels (3.5.2) */
#define US915_DTS_MAX_POWER_DBM 26

/*
 * Uplink datarates (Table 19), restricted to the DR0..DR4 set 3.5.3 lists as
 * the minimum a certified device implements.
 *
 * DR5 and DR6 are LR-FHSS, which no Zephyr LoRa driver offers, and DR7 and
 * DR8 are SF6 and SF5, which 3.5.3 tells a device not to use before the
 * network asks for them over LinkADRReq. Implementing DR7 and DR8 would also
 * pull in downlink DR14 and DR0, so the four stay out together and
 * validate_dr() rejects them.
 *
 * Maximum payloads are the repeater-compatible N values of Table 24.
 */
static const struct lwan_dr_params us915_ul_dr_table[] = {
	[0] = {.sf = SF_10, .bw = BW_125_KHZ, .max_payload = 11},
	[1] = {.sf = SF_9, .bw = BW_125_KHZ, .max_payload = 53},
	[2] = {.sf = SF_8, .bw = BW_125_KHZ, .max_payload = 125},
	[3] = {.sf = SF_7, .bw = BW_125_KHZ, .max_payload = 222},
	[4] = {.sf = SF_8, .bw = BW_500_KHZ, .max_payload = 222},
};

#define US915_UL_DR_COUNT ARRAY_SIZE(us915_ul_dr_table)

/*
 * Downlink datarates (Table 20). The band numbers its downlink datarates in a
 * space of their own that starts at DR8, so the table is indexed by
 * dr - US915_DL_DR_MIN rather than by dr. Downlink DR0 is SF5/500kHz and
 * DR14 is SF6/500kHz; both belong to the uplink DR7/DR8 option this region
 * leaves out, and DR1..DR7 are RFU.
 */
#define US915_DL_DR_MIN 8

static const struct lwan_dr_params us915_dl_dr_table[] = {
	[0] = {.sf = SF_12, .bw = BW_500_KHZ, .max_payload = 53},
	[1] = {.sf = SF_11, .bw = BW_500_KHZ, .max_payload = 129},
	[2] = {.sf = SF_10, .bw = BW_500_KHZ, .max_payload = 222},
	[3] = {.sf = SF_9, .bw = BW_500_KHZ, .max_payload = 222},
	[4] = {.sf = SF_8, .bw = BW_500_KHZ, .max_payload = 222},
	[5] = {.sf = SF_7, .bw = BW_500_KHZ, .max_payload = 222},
};

#define US915_DL_DR_COUNT ARRAY_SIZE(us915_dl_dr_table)
#define US915_DL_DR_MAX   (US915_DL_DR_MIN + US915_DL_DR_COUNT - 1)

/*
 * RX1 downlink datarate (Table 26), indexed by uplink datarate then by
 * RX1DROffset. 3.5.7 reserves offsets 4 to 7, so the rows stop at 3.
 */
#define US915_MAX_RX1_DR_OFFSET 3

/* clang-format off */
static const uint8_t us915_rx1_dr[US915_UL_DR_COUNT][US915_MAX_RX1_DR_OFFSET + 1] = {
	[0] = { 10,  9,  8,  8 },
	[1] = { 11, 10,  9,  8 },
	[2] = { 12, 11, 10,  9 },
	[3] = { 13, 12, 11, 10 },
	[4] = { 13, 13, 12, 11 },
};
/* clang-format on */

/* Join-Request datarates (3.5.2): DR0 on the 125 kHz channels, DR4 on the 500 kHz ones */
#define US915_JOIN_DR_125 0
#define US915_JOIN_DR_500 4

/*
 * 3.5.2 asks the device to probe the eight octet groups of 125 kHz channels
 * in turn and to close each pass on one 500 kHz channel, so a pass walks nine
 * groups and the next pass takes the next 500 kHz channel.
 */
#define US915_JOIN_GROUP_SIZE  8
#define US915_JOIN_GROUP_COUNT (US915_UL_125_CH_COUNT / US915_JOIN_GROUP_SIZE + 1)
#define US915_JOIN_GROUP_500   (US915_JOIN_GROUP_COUNT - 1)

static uint8_t us915_join_group;
static uint8_t us915_join_pass;

static uint32_t us915_channel_freq(size_t idx)
{
	if (idx < US915_UL_125_CH_COUNT) {
		return US915_UL_125_BASE_HZ + (uint32_t)idx * US915_UL_125_STEP_HZ;
	}

	return US915_UL_500_BASE_HZ +
	       (uint32_t)(idx - US915_UL_125_CH_COUNT) * US915_UL_500_STEP_HZ;
}

static int us915_get_default_channels(struct lwan_channel *ch, size_t *count)
{
	if (*count < US915_UL_CH_COUNT) {
		return -ENOMEM;
	}

	/* 3.5.2: a reset leaves all 72 channels enabled. */
	for (size_t i = 0; i < US915_UL_125_CH_COUNT; i++) {
		ch[i] = (struct lwan_channel){
			.frequency = us915_channel_freq(i),
			.min_dr = 0,
			.max_dr = 3,
			.enabled = true,
		};
	}

	for (size_t i = US915_UL_125_CH_COUNT; i < US915_UL_CH_COUNT; i++) {
		ch[i] = (struct lwan_channel){
			.frequency = us915_channel_freq(i),
			.min_dr = 4,
			.max_dr = 4,
			.enabled = true,
		};
	}

	*count = US915_UL_CH_COUNT;
	return 0;
}

static int us915_get_tx_params(uint8_t dr, uint8_t tx_power_idx, struct lwan_dr_params *p,
			       int8_t *power_dbm)
{
	uint8_t idx;

	if (dr >= US915_UL_DR_COUNT) {
		return -EINVAL;
	}

	/* Defensive clamp — validate_tx_power() is the authoritative check. */
	idx = MIN(tx_power_idx, US915_MAX_TX_POWER_IDX);

	*p = us915_ul_dr_table[dr];
	*power_dbm = US915_MAX_POWER_DBM - (int8_t)(2 * idx);
	return 0;
}

/*
 * Recover the channel number the stack transmitted on. The 500 kHz centres
 * fall between the 125 kHz ones, so a frequency belongs to at most one of the
 * two plans and the two tests cannot both match.
 */
static int us915_channel_index(uint32_t tx_freq, size_t *idx)
{
	if (tx_freq >= US915_UL_125_BASE_HZ) {
		uint32_t delta = tx_freq - US915_UL_125_BASE_HZ;

		if (delta % US915_UL_125_STEP_HZ == 0 &&
		    delta / US915_UL_125_STEP_HZ < US915_UL_125_CH_COUNT) {
			*idx = delta / US915_UL_125_STEP_HZ;
			return 0;
		}
	}

	if (tx_freq >= US915_UL_500_BASE_HZ) {
		uint32_t delta = tx_freq - US915_UL_500_BASE_HZ;

		if (delta % US915_UL_500_STEP_HZ == 0 &&
		    delta / US915_UL_500_STEP_HZ < US915_UL_500_CH_COUNT) {
			*idx = US915_UL_125_CH_COUNT + delta / US915_UL_500_STEP_HZ;
			return 0;
		}
	}

	return -EINVAL;
}

/* DR4 is the only datarate the 500 kHz channels carry, so the two agree. */
static void us915_clamp_tx_power(uint32_t freq, int8_t *power_dbm)
{
	size_t idx;

	if (us915_channel_index(freq, &idx) != 0) {
		return;
	}

	if (idx < US915_UL_125_CH_COUNT) {
		return;
	}

	if (*power_dbm > US915_DTS_MAX_POWER_DBM) {
		*power_dbm = US915_DTS_MAX_POWER_DBM;
	}
}

static int us915_get_rx1_params(uint32_t tx_freq, uint8_t tx_dr, uint8_t offset, uint32_t *rx1_freq,
				struct lwan_dr_params *p)
{
	uint8_t rx1_dr;
	size_t idx;
	int ret;

	if (tx_dr >= US915_UL_DR_COUNT || offset > US915_MAX_RX1_DR_OFFSET) {
		return -EINVAL;
	}

	ret = us915_channel_index(tx_freq, &idx);
	if (ret != 0) {
		LOG_WRN("TX frequency %u is not in the channel plan", tx_freq);
		return ret;
	}

	/* 3.5.7: RX1 channel = uplink channel modulo the eight downlink channels. */
	*rx1_freq = US915_DL_BASE_HZ + (uint32_t)(idx % US915_DL_CH_COUNT) * US915_DL_STEP_HZ;

	rx1_dr = us915_rx1_dr[tx_dr][offset];

	*p = us915_dl_dr_table[rx1_dr - US915_DL_DR_MIN];
	return 0;
}

static int us915_get_rx2_params(uint8_t dr, uint32_t *freq, struct lwan_dr_params *p)
{
	if (dr < US915_DL_DR_MIN || dr > US915_DL_DR_MAX) {
		return -EINVAL;
	}

	*freq = US915_RX2_FREQ;
	*p = us915_dl_dr_table[dr - US915_DL_DR_MIN];
	return 0;
}

static int us915_validate_dl_settings(uint8_t rx1_dr_offset, uint8_t rx2_datarate)
{
	if (rx1_dr_offset > US915_MAX_RX1_DR_OFFSET) {
		return -EINVAL;
	}

	if (rx2_datarate < US915_DL_DR_MIN || rx2_datarate > US915_DL_DR_MAX) {
		return -EINVAL;
	}

	return 0;
}

static int us915_validate_dr(uint8_t dr)
{
	if (dr >= US915_UL_DR_COUNT) {
		return -EINVAL;
	}

	return 0;
}

static int us915_validate_tx_power(uint8_t tx_power_idx)
{
	if (tx_power_idx > US915_MAX_TX_POWER_IDX) {
		return -EINVAL;
	}

	return 0;
}

/*
 * CFList type 1 (3.5.4): bytes 0..9 hold five 16-bit little-endian ChMask
 * fields covering channel numbers 0 to 79, and a cleared bit disables its
 * channel. Bits above 71 are RFU on a device that stores 72 channels.
 */
#define CFLIST_TYPE_CH_MASK  1
#define CFLIST_CH_MASK_COUNT 5

#define US915_CH_BLOCK_SIZE 16

static bool us915_cflist_bit(const uint8_t cflist[16], size_t idx)
{
	uint16_t mask = sys_get_le16(&cflist[(idx / US915_CH_BLOCK_SIZE) * 2]);

	return (mask & BIT(idx % US915_CH_BLOCK_SIZE)) != 0;
}

static int us915_apply_cflist(const uint8_t cflist[16], struct lwan_channel *ch, size_t *count)
{
	size_t span = MIN(*count, (size_t)US915_UL_CH_COUNT);
	uint8_t cflist_type;
	bool any = false;

	cflist_type = cflist[15];

	if (cflist_type != CFLIST_TYPE_CH_MASK) {
		LOG_WRN("Unsupported CFList type: %u", cflist_type);
		return 0;
	}

	for (size_t i = 0; i < span; i++) {
		if (us915_cflist_bit(cflist, i)) {
			any = true;
			break;
		}
	}

	/*
	 * A CFList that leaves no channel enabled would strand the device on
	 * a session it can never transmit in, so keep the plan the
	 * Join-Accept arrived with instead.
	 */
	if (!any) {
		LOG_WRN("CFList disables every channel, ignoring it");
		return 0;
	}

	for (size_t i = 0; i < span; i++) {
		ch[i].enabled = us915_cflist_bit(cflist, i);
	}

	return 0;
}

/*
 * US902-928 ChMaskCntl encoding (Table 23):
 *   0..4 = ChMask applies to a block of 16 channels starting at cntl * 16.
 *   5    = bits 0..7 each switch a bank of eight 125 kHz channels together
 *          with the 500 kHz channel above it; bit 9 switches a bank this
 *          device does not have, and bit 8 and bits 10..15 are RFU.
 *   6    = all 125 kHz channels on, and ChMask applies to channels 64..79.
 *   7    = all 125 kHz channels off, and ChMask applies to channels 64..79.
 */
#define US915_CH_MASK_CNTL_BLOCK_MAX 4
#define US915_CH_MASK_CNTL_BANKS     5
#define US915_CH_MASK_CNTL_ALL_ON    6
#define US915_CH_MASK_CNTL_ALL_OFF   7

#define US915_CH_BANK_SIZE  8
#define US915_CH_BANK_COUNT 8

/*
 * The ChMask bits that name a channel this device stores. Table 23 reaches
 * channel 79 and a tenth bank, both above the 72 of 3.5.2.
 */
static uint16_t us915_adr_mask_valid_bits(size_t span, uint8_t ch_mask_cntl)
{
	size_t first;

	switch (ch_mask_cntl) {
	case US915_CH_MASK_CNTL_BANKS:
		/* Bit 8 is RFU and bit 9 names the bank above channel 71. */
		return (uint16_t)BIT_MASK(US915_CH_BANK_COUNT);
	case US915_CH_MASK_CNTL_ALL_ON:
	case US915_CH_MASK_CNTL_ALL_OFF:
		first = US915_UL_125_CH_COUNT;
		break;
	default:
		first = (size_t)ch_mask_cntl * US915_CH_BLOCK_SIZE;
		break;
	}

	if (first >= span) {
		return 0;
	}

	return (uint16_t)BIT_MASK(MIN(span - first, (size_t)US915_CH_BLOCK_SIZE));
}

/* The state channel @p idx takes if this request is applied. */
static bool us915_adr_mask_result(size_t idx, uint8_t ch_mask_cntl, uint16_t ch_mask, bool current)
{
	size_t first;

	switch (ch_mask_cntl) {
	case US915_CH_MASK_CNTL_BANKS: {
		size_t bank = (idx < US915_UL_125_CH_COUNT) ? idx / US915_CH_BANK_SIZE
							    : idx - US915_UL_125_CH_COUNT;

		return (ch_mask & BIT(bank)) != 0;
	}
	case US915_CH_MASK_CNTL_ALL_ON:
	case US915_CH_MASK_CNTL_ALL_OFF:
		if (idx < US915_UL_125_CH_COUNT) {
			return ch_mask_cntl == US915_CH_MASK_CNTL_ALL_ON;
		}

		return (ch_mask & BIT(idx - US915_UL_125_CH_COUNT)) != 0;
	default:
		first = (size_t)ch_mask_cntl * US915_CH_BLOCK_SIZE;

		if (idx < first || idx >= first + US915_CH_BLOCK_SIZE) {
			return current;
		}

		return (ch_mask & BIT(idx - first)) != 0;
	}
}

static int us915_apply_adr_channel_mask(struct lwan_channel *ch, size_t count, uint8_t ch_mask_cntl,
					uint16_t ch_mask)
{
	size_t span = MIN(count, (size_t)US915_UL_CH_COUNT);
	bool any = false;

	if (ch_mask_cntl > US915_CH_MASK_CNTL_ALL_OFF) {
		return -EINVAL;
	}

	if (ch_mask_cntl <= US915_CH_MASK_CNTL_BLOCK_MAX &&
	    (size_t)ch_mask_cntl * US915_CH_BLOCK_SIZE >= span) {
		/* The block addresses channels this device does not have. */
		return -EINVAL;
	}

	if ((ch_mask & ~us915_adr_mask_valid_bits(span, ch_mask_cntl)) != 0) {
		return -EINVAL;
	}

	/*
	 * Every channel is a default channel here, so there is no pair to
	 * protect the way EU868 and AS923 protect theirs. What a device must
	 * not accept is a mask that leaves it nothing to transmit on, so the
	 * result is counted before anything is committed.
	 */
	for (size_t i = 0; i < span; i++) {
		if (us915_adr_mask_result(i, ch_mask_cntl, ch_mask, ch[i].enabled)) {
			any = true;
			break;
		}
	}

	if (!any) {
		return -EINVAL;
	}

	for (size_t i = 0; i < span; i++) {
		ch[i].enabled = us915_adr_mask_result(i, ch_mask_cntl, ch_mask, ch[i].enabled);
	}

	return 0;
}

static int us915_select_in_range(const struct lwan_channel *ch, size_t count, size_t first,
				 size_t span, uint8_t dr, uint32_t *freq)
{
	uint8_t candidates[US915_UL_CH_COUNT];
	uint8_t candidate_count = 0;

	for (size_t i = first; i < MIN(count, first + span); i++) {
		if (!ch[i].enabled || ch[i].frequency == 0 || dr < ch[i].min_dr ||
		    dr > ch[i].max_dr) {
			continue;
		}

		candidates[candidate_count++] = i;
	}

	if (candidate_count == 0) {
		return -ENOENT;
	}

	*freq = ch[candidates[sys_rand8_get() % candidate_count]].frequency;

	return 0;
}

static int us915_select_data_channel(const struct lwan_channel *ch, size_t count, uint8_t dr,
				     uint32_t *freq, int32_t *delay_ms)
{
	ARG_UNUSED(delay_ms);

	/*
	 * The band has no duty cycle and no listen-before-talk, so a channel
	 * that matches the datarate is always free and -ENOBUFS never
	 * arises.
	 */
	return us915_select_in_range(ch, count, 0, count, dr, freq);
}

static void us915_join_advance(void)
{
	if (us915_join_group < US915_JOIN_GROUP_COUNT - 1) {
		us915_join_group++;
		return;
	}

	us915_join_group = 0;
	us915_join_pass = (us915_join_pass + 1) % US915_UL_500_CH_COUNT;
}

static int us915_select_join_channel(const struct lwan_channel *ch, size_t count, uint32_t *freq,
				     uint8_t *dr, int32_t *delay_ms)
{
	ARG_UNUSED(delay_ms);

	/*
	 * 3.5.2 asks the device to change channel on every Join-Request and
	 * to sweep the octet groups rather than draw from the whole plan, so
	 * the cursor walks one group per call and the draw is random only
	 * inside it. A full turn without a usable channel means the plan has
	 * none.
	 */
	for (size_t tries = 0; tries < US915_JOIN_GROUP_COUNT; tries++) {
		uint8_t group = us915_join_group;
		size_t first;
		size_t span;
		uint8_t join_dr;
		int ret;

		if (group == US915_JOIN_GROUP_500) {
			first = US915_UL_125_CH_COUNT + us915_join_pass;
			span = 1;
			join_dr = US915_JOIN_DR_500;
		} else {
			first = (size_t)group * US915_JOIN_GROUP_SIZE;
			span = US915_JOIN_GROUP_SIZE;
			join_dr = US915_JOIN_DR_125;
		}

		ret = us915_select_in_range(ch, count, first, span, join_dr, freq);

		us915_join_advance();

		if (ret == 0) {
			*dr = join_dr;
			return 0;
		}
	}

	return -ENOENT;
}

static void us915_record_tx(uint32_t freq, uint32_t airtime_ms)
{
	ARG_UNUSED(freq);
	ARG_UNUSED(airtime_ms);

	/* The band holds no duty cycle, so a transmission buys no off-time. */
}

const struct lwan_region_ops us915_ops = {
	.default_rx2_dr = US915_RX2_DR,
	.get_default_channels = us915_get_default_channels,
	.get_tx_params = us915_get_tx_params,
	.get_rx1_params = us915_get_rx1_params,
	.get_rx2_params = us915_get_rx2_params,
	.validate_dl_settings = us915_validate_dl_settings,
	.apply_cflist = us915_apply_cflist,
	.validate_dr = us915_validate_dr,
	.validate_tx_power = us915_validate_tx_power,
	.apply_adr_channel_mask = us915_apply_adr_channel_mask,
	.select_join_channel = us915_select_join_channel,
	.select_data_channel = us915_select_data_channel,
	.clamp_tx_power = us915_clamp_tx_power,
	.record_tx = us915_record_tx,
};
