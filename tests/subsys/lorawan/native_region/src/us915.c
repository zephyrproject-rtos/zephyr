/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include "region.h"

/* Uplink channel plan (RP002-1.0.5, 3.5.2) */
#define US915_UL_125_BASE_HZ  902300000U
#define US915_UL_125_STEP_HZ  200000U
#define US915_UL_125_CH_COUNT 64

#define US915_UL_500_BASE_HZ  903000000U
#define US915_UL_500_STEP_HZ  1600000U
#define US915_UL_500_CH_COUNT 8

#define US915_UL_CH_COUNT (US915_UL_125_CH_COUNT + US915_UL_500_CH_COUNT)

/* Downlink channel plan (3.5.2) and RX2 defaults (3.5.7) */
#define US915_DL_BASE_HZ  923300000U
#define US915_DL_STEP_HZ  600000U
#define US915_DL_CH_COUNT 8
#define US915_RX2_DR      8

#define US915_UL_DR_COUNT       5
#define US915_DL_DR_MIN         8
#define US915_DL_DR_MAX         13
#define US915_MAX_RX1_DR_OFFSET 3
#define US915_MAX_TX_POWER_IDX  14
#define US915_MAX_POWER_DBM     30
#define US915_DTS_MAX_POWER_DBM 26

#define US915_JOIN_DR_125 0
#define US915_JOIN_DR_500 4

/* ChMaskCntl values (Table 23) */
#define CH_MASK_CNTL_BANKS   5
#define CH_MASK_CNTL_ALL_ON  6
#define CH_MASK_CNTL_ALL_OFF 7

#define CFLIST_TYPE_CH_MASK 1

static const struct lwan_region_ops *us915;
static struct lwan_channel ch[CONFIG_LORAWAN_NATIVE_MAX_CHANNELS];
static size_t ch_count;

static void reset_channels(void)
{
	memset(ch, 0, sizeof(ch));
	ch_count = ARRAY_SIZE(ch);
	zassert_ok(us915->get_default_channels(ch, &ch_count));
}

static size_t enabled_count(void)
{
	size_t n = 0;

	for (size_t i = 0; i < ARRAY_SIZE(ch); i++) {
		if (ch[i].enabled) {
			n++;
		}
	}

	return n;
}

static uint32_t channel_freq(size_t idx)
{
	if (idx < US915_UL_125_CH_COUNT) {
		return US915_UL_125_BASE_HZ + (uint32_t)idx * US915_UL_125_STEP_HZ;
	}

	return US915_UL_500_BASE_HZ +
	       (uint32_t)(idx - US915_UL_125_CH_COUNT) * US915_UL_500_STEP_HZ;
}

static void *suite_setup(void)
{
	us915 = lwan_region_get(LORAWAN_REGION_US915);
	zassert_not_null(us915, "US915 region not registered");

	return NULL;
}

ZTEST_SUITE(lorawan_us915_region, NULL, suite_setup, NULL, NULL, NULL);

/*
 * 3.5.2 asks the device to store at least 72 channels and to bring all of
 * them up after a reset.
 */
ZTEST(lorawan_us915_region, test_default_channels)
{
	reset_channels();

	zassert_equal(ch_count, US915_UL_CH_COUNT);
	zassert_equal(enabled_count(), US915_UL_CH_COUNT);

	for (size_t i = 0; i < US915_UL_CH_COUNT; i++) {
		zassert_equal(ch[i].frequency, channel_freq(i), "channel %zu", i);
		zassert_true(ch[i].enabled, "channel %zu", i);
	}
}

ZTEST(lorawan_us915_region, test_default_channel_datarates)
{
	reset_channels();

	/* The 64 125 kHz channels carry uplink DR0 to DR3 (3.5.2). */
	for (size_t i = 0; i < US915_UL_125_CH_COUNT; i++) {
		zassert_equal(ch[i].min_dr, 0, "channel %zu", i);
		zassert_equal(ch[i].max_dr, 3, "channel %zu", i);
	}

	/* The 8 500 kHz channels carry uplink DR4 alone, LR-FHSS being out. */
	for (size_t i = US915_UL_125_CH_COUNT; i < US915_UL_CH_COUNT; i++) {
		zassert_equal(ch[i].min_dr, 4, "channel %zu", i);
		zassert_equal(ch[i].max_dr, 4, "channel %zu", i);
	}
}

ZTEST(lorawan_us915_region, test_default_channels_need_room)
{
	struct lwan_channel small[US915_UL_CH_COUNT - 1];
	size_t count = ARRAY_SIZE(small);

	zassert_equal(us915->get_default_channels(small, &count), -ENOMEM);
}

/* Uplink datarates (Table 19) */
ZTEST(lorawan_us915_region, test_tx_params)
{
	static const struct {
		enum lora_datarate sf;
		enum lora_signal_bandwidth bw;
		uint8_t max_payload;
	} expect[US915_UL_DR_COUNT] = {
		{SF_10, BW_125_KHZ, 11}, {SF_9, BW_125_KHZ, 53},  {SF_8, BW_125_KHZ, 125},
		{SF_7, BW_125_KHZ, 222}, {SF_8, BW_500_KHZ, 222},
	};
	struct lwan_dr_params p;
	int8_t power;

	for (uint8_t dr = 0; dr < US915_UL_DR_COUNT; dr++) {
		zassert_ok(us915->get_tx_params(dr, 0, &p, &power), "DR%u", dr);
		zassert_equal(p.sf, expect[dr].sf, "DR%u", dr);
		zassert_equal(p.bw, expect[dr].bw, "DR%u", dr);
		zassert_equal(p.max_payload, expect[dr].max_payload, "DR%u", dr);
	}
}

/*
 * LR-FHSS DR5 and DR6 need a modulation no Zephyr LoRa driver offers, and
 * DR7 and DR8 are the SF6 and SF5 option 3.5.3 leaves optional.
 */
ZTEST(lorawan_us915_region, test_unsupported_uplink_datarates_rejected)
{
	struct lwan_dr_params p;
	int8_t power;

	for (uint8_t dr = 0; dr < US915_UL_DR_COUNT; dr++) {
		zassert_ok(us915->validate_dr(dr), "DR%u", dr);
	}

	for (uint8_t dr = US915_UL_DR_COUNT; dr <= 15; dr++) {
		zassert_equal(us915->get_tx_params(dr, 0, &p, &power), -EINVAL, "DR%u", dr);
		zassert_equal(us915->validate_dr(dr), -EINVAL, "DR%u", dr);
	}
}

/* TXPower: 30 dBm conducted at index 0, 2 dB per step, 2 dBm at index 14 */
ZTEST(lorawan_us915_region, test_tx_power_table)
{
	struct lwan_dr_params p;
	int8_t power;

	for (uint8_t idx = 0; idx <= US915_MAX_TX_POWER_IDX; idx++) {
		zassert_ok(us915->validate_tx_power(idx), "index %u", idx);
		zassert_ok(us915->get_tx_params(0, idx, &p, &power), "index %u", idx);
		zassert_equal(power, US915_MAX_POWER_DBM - 2 * idx, "index %u", idx);
	}

	zassert_equal(us915->validate_tx_power(US915_MAX_TX_POWER_IDX + 1), -EINVAL);
}

/*
 * 3.5.7: the RX1 channel is the uplink channel number modulo the eight
 * downlink channels, which holds across both halves of the uplink plan.
 */
ZTEST(lorawan_us915_region, test_rx1_frequency_wraps_every_eight_channels)
{
	struct lwan_dr_params p;
	uint32_t freq;

	for (size_t i = 0; i < US915_UL_CH_COUNT; i++) {
		uint32_t expect =
			US915_DL_BASE_HZ + (uint32_t)(i % US915_DL_CH_COUNT) * US915_DL_STEP_HZ;

		zassert_ok(us915->get_rx1_params(channel_freq(i), 0, 0, &freq, &p), "channel %zu",
			   i);
		zassert_equal(freq, expect, "channel %zu", i);
	}
}

ZTEST(lorawan_us915_region, test_rx1_rejects_frequency_outside_the_plan)
{
	struct lwan_dr_params p;
	uint32_t freq;

	/* Between two 125 kHz centres. */
	zassert_equal(us915->get_rx1_params(US915_UL_125_BASE_HZ + 100000, 0, 0, &freq, &p),
		      -EINVAL);
	/* One step past the last 125 kHz channel, and not a 500 kHz centre. */
	zassert_equal(us915->get_rx1_params(US915_UL_125_BASE_HZ +
						    US915_UL_125_CH_COUNT * US915_UL_125_STEP_HZ,
					    0, 0, &freq, &p),
		      -EINVAL);
	/* Below the band. */
	zassert_equal(us915->get_rx1_params(868100000, 0, 0, &freq, &p), -EINVAL);
}

/* RX1 downlink datarate mapping (Table 26) */
ZTEST(lorawan_us915_region, test_rx1_datarate_mapping)
{
	static const uint8_t expect[US915_UL_DR_COUNT][US915_MAX_RX1_DR_OFFSET + 1] = {
		{10, 9, 8, 8}, {11, 10, 9, 8}, {12, 11, 10, 9}, {13, 12, 11, 10}, {13, 13, 12, 11},
	};
	struct lwan_dr_params rx1;
	struct lwan_dr_params rx2;
	uint32_t freq;

	for (uint8_t dr = 0; dr < US915_UL_DR_COUNT; dr++) {
		for (uint8_t off = 0; off <= US915_MAX_RX1_DR_OFFSET; off++) {
			zassert_ok(
				us915->get_rx1_params(US915_UL_125_BASE_HZ, dr, off, &freq, &rx1),
				"DR%u offset %u", dr, off);

			/*
			 * The table gives a downlink datarate, so compare
			 * against what RX2 reports for that same index
			 * rather than restating Table 20 here.
			 */
			zassert_ok(us915->get_rx2_params(expect[dr][off], &freq, &rx2));
			zassert_equal(rx1.sf, rx2.sf, "DR%u offset %u", dr, off);
			zassert_equal(rx1.bw, rx2.bw, "DR%u offset %u", dr, off);
			zassert_equal(rx1.max_payload, rx2.max_payload, "DR%u offset %u", dr, off);
		}
	}
}

/* 3.5.7 reserves RX1DROffset 4 to 7 and tells the device not to accept them. */
ZTEST(lorawan_us915_region, test_rx1_rejects_reserved_offsets)
{
	struct lwan_dr_params p;
	uint32_t freq;

	for (uint8_t off = US915_MAX_RX1_DR_OFFSET + 1; off <= 7; off++) {
		zassert_equal(us915->get_rx1_params(US915_UL_125_BASE_HZ, 0, off, &freq, &p),
			      -EINVAL, "offset %u", off);
		zassert_equal(us915->validate_dl_settings(off, US915_RX2_DR), -EINVAL, "offset %u",
			      off);
	}
}

/* us915_rx1_dr[] has no row for them, so this guards an out-of-bounds read. */
ZTEST(lorawan_us915_region, test_rx1_rejects_unsupported_uplink_datarates)
{
	struct lwan_dr_params p;
	uint32_t freq;

	for (uint8_t dr = US915_UL_DR_COUNT; dr <= 15; dr++) {
		for (uint8_t off = 0; off <= US915_MAX_RX1_DR_OFFSET; off++) {
			zassert_equal(
				us915->get_rx1_params(US915_UL_125_BASE_HZ, dr, off, &freq, &p),
				-EINVAL, "DR%u offset %u", dr, off);
		}
	}
}

/* 3.5.7 makes RX1 a function of the uplink channel alone. */
ZTEST(lorawan_us915_region, test_rx1_frequency_ignores_the_datarate_and_offset)
{
	for (size_t i = 0; i < US915_UL_CH_COUNT; i++) {
		uint32_t expect =
			US915_DL_BASE_HZ + (uint32_t)(i % US915_DL_CH_COUNT) * US915_DL_STEP_HZ;

		for (uint8_t dr = 0; dr < US915_UL_DR_COUNT; dr++) {
			for (uint8_t off = 0; off <= US915_MAX_RX1_DR_OFFSET; off++) {
				struct lwan_dr_params p;
				uint32_t freq;

				zassert_ok(
					us915->get_rx1_params(channel_freq(i), dr, off, &freq, &p),
					"channel %zu DR%u offset %u", i, dr, off);
				zassert_equal(freq, expect, "channel %zu DR%u offset %u", i, dr,
					      off);
			}
		}
	}
}

/* Downlink datarates (Table 20) */
ZTEST(lorawan_us915_region, test_rx2_params)
{
	static const enum lora_datarate expect[] = {SF_12, SF_11, SF_10, SF_9, SF_8, SF_7};
	static const uint8_t payload[] = {53, 129, 222, 222, 222, 222};
	struct lwan_dr_params p;
	uint32_t freq;

	for (uint8_t dr = US915_DL_DR_MIN; dr <= US915_DL_DR_MAX; dr++) {
		zassert_ok(us915->get_rx2_params(dr, &freq, &p), "DR%u", dr);
		zassert_equal(freq, US915_DL_BASE_HZ, "DR%u", dr);
		zassert_equal(p.sf, expect[dr - US915_DL_DR_MIN], "DR%u", dr);
		zassert_equal(p.bw, BW_500_KHZ, "DR%u", dr);
		zassert_equal(p.max_payload, payload[dr - US915_DL_DR_MIN], "DR%u", dr);
	}
}

/*
 * The band numbers downlink datarates in a space of their own that starts at
 * DR8, so an index from the uplink space is not a downlink datarate.
 */
ZTEST(lorawan_us915_region, test_rx2_rejects_uplink_datarate_space)
{
	struct lwan_dr_params p;
	uint32_t freq;

	for (uint8_t dr = 0; dr < US915_DL_DR_MIN; dr++) {
		zassert_equal(us915->get_rx2_params(dr, &freq, &p), -EINVAL, "DR%u", dr);
		zassert_equal(us915->validate_dl_settings(0, dr), -EINVAL, "DR%u", dr);
	}

	for (uint8_t dr = US915_DL_DR_MAX + 1; dr <= 15; dr++) {
		zassert_equal(us915->get_rx2_params(dr, &freq, &p), -EINVAL, "DR%u", dr);
		zassert_equal(us915->validate_dl_settings(0, dr), -EINVAL, "DR%u", dr);
	}
}

/*
 * The RX2 window that receives the Join-Accept opens before the network has
 * named a datarate, and 3.5.7 puts the default at DR8.
 */
ZTEST(lorawan_us915_region, test_default_rx2_datarate)
{
	zassert_equal(us915->default_rx2_dr, US915_RX2_DR);
}

ZTEST(lorawan_us915_region, test_dl_settings_accepts_the_defined_range)
{
	for (uint8_t off = 0; off <= US915_MAX_RX1_DR_OFFSET; off++) {
		for (uint8_t dr = US915_DL_DR_MIN; dr <= US915_DL_DR_MAX; dr++) {
			zassert_ok(us915->validate_dl_settings(off, dr), "offset %u DR%u", off, dr);
		}
	}
}

/* CFList type 1 (3.5.4): five ChMask fields covering channels 0 to 79 */
static void apply_cflist(uint16_t m0, uint16_t m1, uint16_t m2, uint16_t m3, uint16_t m4)
{
	uint8_t cflist[16] = {0};

	sys_put_le16(m0, &cflist[0]);
	sys_put_le16(m1, &cflist[2]);
	sys_put_le16(m2, &cflist[4]);
	sys_put_le16(m3, &cflist[6]);
	sys_put_le16(m4, &cflist[8]);
	cflist[15] = CFLIST_TYPE_CH_MASK;

	zassert_ok(us915->apply_cflist(cflist, ch, &ch_count));
}

ZTEST(lorawan_us915_region, test_cflist_enables_the_named_channels)
{
	reset_channels();

	/* The first eight 125 kHz channels and the 500 kHz channel above them. */
	apply_cflist(0x00ff, 0, 0, 0, BIT(0));

	zassert_equal(enabled_count(), 9);

	for (size_t i = 0; i < 8; i++) {
		zassert_true(ch[i].enabled, "channel %zu", i);
	}
	for (size_t i = 8; i < US915_UL_125_CH_COUNT; i++) {
		zassert_false(ch[i].enabled, "channel %zu", i);
	}
	zassert_true(ch[US915_UL_125_CH_COUNT].enabled);
	for (size_t i = US915_UL_125_CH_COUNT + 1; i < US915_UL_CH_COUNT; i++) {
		zassert_false(ch[i].enabled, "channel %zu", i);
	}
}

/*
 * A CFList with every bit clear would leave the device with no channel to
 * transmit on, which is worse than keeping the plan the Join-Accept arrived
 * with.
 */
ZTEST(lorawan_us915_region, test_cflist_leaving_nothing_enabled_is_ignored)
{
	reset_channels();

	apply_cflist(0, 0, 0, 0, 0);

	zassert_equal(enabled_count(), US915_UL_CH_COUNT);
}

/* 3.5.4: only CFListType 1 carries a channel mask in this band. */
ZTEST(lorawan_us915_region, test_cflist_of_another_type_is_ignored)
{
	uint8_t cflist[16] = {0};

	reset_channels();

	sys_put_le16(0x00ff, &cflist[0]);
	cflist[15] = 0;

	zassert_ok(us915->apply_cflist(cflist, ch, &ch_count));
	zassert_equal(enabled_count(), US915_UL_CH_COUNT);
}

/* ChMaskCntl 0 to 4: blocks of 16 channels (Table 23) */
ZTEST(lorawan_us915_region, test_adr_mask_block_switches_sixteen_channels)
{
	reset_channels();

	zassert_ok(us915->apply_adr_channel_mask(ch, ch_count, 1, 0x0001));

	/* Block 1 covers channels 16 to 31, of which only 16 survives. */
	zassert_true(ch[16].enabled);
	for (size_t i = 17; i < 32; i++) {
		zassert_false(ch[i].enabled, "channel %zu", i);
	}

	/* Channels outside the block keep the state they had. */
	for (size_t i = 0; i < 16; i++) {
		zassert_true(ch[i].enabled, "channel %zu", i);
	}
	for (size_t i = 32; i < US915_UL_CH_COUNT; i++) {
		zassert_true(ch[i].enabled, "channel %zu", i);
	}
}

/* Block 4 covers channels 64 to 79, of which this device has 64 to 71. */
ZTEST(lorawan_us915_region, test_adr_mask_block_four_reaches_the_500_khz_channels)
{
	reset_channels();

	zassert_ok(us915->apply_adr_channel_mask(ch, ch_count, 4, 0x0003));

	zassert_true(ch[64].enabled);
	zassert_true(ch[65].enabled);
	for (size_t i = 66; i < US915_UL_CH_COUNT; i++) {
		zassert_false(ch[i].enabled, "channel %zu", i);
	}
	for (size_t i = 0; i < US915_UL_125_CH_COUNT; i++) {
		zassert_true(ch[i].enabled, "channel %zu", i);
	}
}

/*
 * ChMaskCntl 5: bit k switches the bank of eight 125 kHz channels starting at
 * k * 8 together with the 500 kHz channel 64 + k.
 */
ZTEST(lorawan_us915_region, test_adr_mask_banks_pair_each_octet_with_a_500_khz_channel)
{
	reset_channels();

	zassert_ok(us915->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_BANKS, BIT(2)));

	for (size_t i = 0; i < US915_UL_125_CH_COUNT; i++) {
		bool expect = (i >= 16 && i < 24);

		zassert_equal(ch[i].enabled, expect, "channel %zu", i);
	}

	zassert_true(ch[US915_UL_125_CH_COUNT + 2].enabled);
	zassert_equal(enabled_count(), 9);
}

/*
 * Bit 9 names a bank above channel 71 that this device does not store, and
 * bit 8 and bits 10 to 15 are RFU.
 */
ZTEST(lorawan_us915_region, test_adr_mask_banks_rejects_reserved_bits)
{
	reset_channels();

	for (uint8_t bit = 8; bit < 16; bit++) {
		zassert_equal(us915->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_BANKS,
							    BIT(0) | BIT(bit)),
			      -EINVAL, "bit %u", bit);
	}

	zassert_equal(enabled_count(), US915_UL_CH_COUNT);
}

/* Table 23 reaches channel 79, and this device stores up to 71. */
ZTEST(lorawan_us915_region, test_adr_mask_rejects_bits_above_channel_71)
{
	static const uint8_t cntls[] = {4, CH_MASK_CNTL_ALL_ON, CH_MASK_CNTL_ALL_OFF};

	reset_channels();

	ARRAY_FOR_EACH(cntls, i) {
		for (uint8_t bit = 8; bit < 16; bit++) {
			zassert_equal(us915->apply_adr_channel_mask(ch, ch_count, cntls[i],
								    BIT(0) | BIT(bit)),
				      -EINVAL, "cntl %u bit %u", cntls[i], bit);
		}
	}

	zassert_equal(enabled_count(), US915_UL_CH_COUNT);
}

/* The blocks that sit inside the plan still take all sixteen bits. */
ZTEST(lorawan_us915_region, test_adr_mask_low_blocks_take_every_bit)
{
	for (uint8_t cntl = 0; cntl <= 3; cntl++) {
		reset_channels();

		zassert_ok(us915->apply_adr_channel_mask(ch, ch_count, cntl, 0xffff), "cntl %u",
			   cntl);
		zassert_equal(enabled_count(), US915_UL_CH_COUNT, "cntl %u", cntl);
	}
}

/* ChMaskCntl 6: all 125 kHz channels on, ChMask applies to 64 to 79. */
ZTEST(lorawan_us915_region, test_adr_mask_all_125_khz_on)
{
	reset_channels();

	zassert_ok(us915->apply_adr_channel_mask(ch, ch_count, 0, 0x0000));
	zassert_ok(us915->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_ALL_ON, BIT(7)));

	for (size_t i = 0; i < US915_UL_125_CH_COUNT; i++) {
		zassert_true(ch[i].enabled, "channel %zu", i);
	}
	for (size_t i = US915_UL_125_CH_COUNT; i < US915_UL_CH_COUNT - 1; i++) {
		zassert_false(ch[i].enabled, "channel %zu", i);
	}
	zassert_true(ch[US915_UL_CH_COUNT - 1].enabled);
}

/* ChMaskCntl 7: all 125 kHz channels off, ChMask applies to 64 to 79. */
ZTEST(lorawan_us915_region, test_adr_mask_all_125_khz_off)
{
	reset_channels();

	zassert_ok(us915->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_ALL_OFF, BIT(0)));

	for (size_t i = 0; i < US915_UL_125_CH_COUNT; i++) {
		zassert_false(ch[i].enabled, "channel %zu", i);
	}
	zassert_true(ch[US915_UL_125_CH_COUNT].enabled);
	zassert_equal(enabled_count(), 1);
}

/*
 * Every channel here is a default channel, so there is no pair to protect.
 * What the device must refuse is a request that leaves it nothing to
 * transmit on.
 */
ZTEST(lorawan_us915_region, test_adr_mask_leaving_nothing_enabled_is_refused)
{
	reset_channels();

	zassert_equal(us915->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_ALL_OFF, 0x0000),
		      -EINVAL);
	zassert_equal(us915->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_BANKS, 0x0000),
		      -EINVAL);
	zassert_equal(enabled_count(), US915_UL_CH_COUNT);
}

/*
 * The two-command sequence 3.5.5 describes: disable all 125 kHz channels,
 * then bring one bank back.
 */
ZTEST(lorawan_us915_region, test_adr_mask_all_off_then_one_bank_back)
{
	reset_channels();

	zassert_ok(us915->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_ALL_OFF, BIT(0)));
	zassert_ok(us915->apply_adr_channel_mask(ch, ch_count, 0, 0x00ff));

	for (size_t i = 0; i < 8; i++) {
		zassert_true(ch[i].enabled, "channel %zu", i);
	}
	for (size_t i = 8; i < US915_UL_125_CH_COUNT; i++) {
		zassert_false(ch[i].enabled, "channel %zu", i);
	}
	zassert_true(ch[US915_UL_125_CH_COUNT].enabled);
	zassert_equal(enabled_count(), 9);
}

ZTEST(lorawan_us915_region, test_adr_mask_rejects_reserved_control_value)
{
	reset_channels();

	for (uint8_t cntl = CH_MASK_CNTL_ALL_OFF + 1; cntl < 16; cntl++) {
		zassert_equal(us915->apply_adr_channel_mask(ch, ch_count, cntl, 0xffff), -EINVAL,
			      "cntl %u", cntl);
	}
}

/* Data channel selection stays inside the datarate each half of the plan carries. */
ZTEST(lorawan_us915_region, test_data_channel_selection_follows_the_datarate)
{
	uint32_t freq;
	int32_t delay_ms;

	reset_channels();

	for (uint8_t dr = 0; dr <= 3; dr++) {
		for (int i = 0; i < 32; i++) {
			zassert_ok(us915->select_data_channel(ch, ch_count, dr, &freq, &delay_ms),
				   "DR%u", dr);
			zassert_true(freq >= US915_UL_125_BASE_HZ, "DR%u gave %u", dr, freq);
			zassert_true(freq <= channel_freq(US915_UL_125_CH_COUNT - 1),
				     "DR%u gave %u", dr, freq);
		}
	}

	for (int i = 0; i < 32; i++) {
		zassert_ok(us915->select_data_channel(ch, ch_count, 4, &freq, &delay_ms));
		zassert_true(freq >= US915_UL_500_BASE_HZ, "DR4 gave %u", freq);
	}
}

ZTEST(lorawan_us915_region, test_data_channel_selection_without_a_match)
{
	uint32_t freq;
	int32_t delay_ms;

	reset_channels();

	/* Leave only the 500 kHz half, which carries DR4 alone. */
	zassert_ok(us915->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_ALL_OFF, 0x00ff));

	zassert_equal(us915->select_data_channel(ch, ch_count, 0, &freq, &delay_ms), -ENOENT);
	zassert_ok(us915->select_data_channel(ch, ch_count, 4, &freq, &delay_ms));
}

/*
 * A channel mask set from the application enables slots by index without
 * reading the frequency, so a slot the plan left empty can arrive here
 * enabled. Selecting it would put the radio on 0 Hz.
 */
ZTEST(lorawan_us915_region, test_data_channel_selection_skips_empty_slots)
{
	uint32_t freq;
	int32_t delay_ms;

	reset_channels();

	for (size_t i = 0; i < ch_count; i++) {
		ch[i].enabled = false;
	}

	/* An empty slot with a zeroed datarate range that DR0 falls inside. */
	ch[0].frequency = 0;
	ch[0].min_dr = 0;
	ch[0].max_dr = 0;
	ch[0].enabled = true;

	zassert_equal(us915->select_data_channel(ch, ch_count, 0, &freq, &delay_ms), -ENOENT);
}

/*
 * 3.5.2: the Join-Request sweeps the octet groups of 125 kHz channels in turn
 * and closes each pass on one 500 kHz channel, never repeating a group within
 * a pass. The cursor is free-running, so a pass is checked for its shape
 * rather than for the group it happens to start on.
 */
static uint32_t collect_join_pass(bool group_seen[US915_UL_125_CH_COUNT / 8])
{
	uint32_t ch_500 = 0;
	uint8_t dr;
	int32_t delay_ms;

	memset(group_seen, 0, sizeof(group_seen[0]) * (US915_UL_125_CH_COUNT / 8));

	for (size_t i = 0; i < US915_UL_125_CH_COUNT / 8 + 1; i++) {
		uint32_t freq;
		size_t group;

		zassert_ok(us915->select_join_channel(ch, ch_count, &freq, &dr, &delay_ms),
			   "attempt %zu", i);

		if (dr == US915_JOIN_DR_500) {
			zassert_true(freq >= US915_UL_500_BASE_HZ, "attempt %zu gave %u", i, freq);
			zassert_equal(ch_500, 0, "two 500 kHz channels in one pass");
			ch_500 = freq;
			continue;
		}

		zassert_equal(dr, US915_JOIN_DR_125, "attempt %zu", i);
		zassert_true(freq >= US915_UL_125_BASE_HZ, "attempt %zu gave %u", i, freq);

		group = (freq - US915_UL_125_BASE_HZ) / US915_UL_125_STEP_HZ / 8;
		zassert_true(group < US915_UL_125_CH_COUNT / 8, "attempt %zu gave %u", i, freq);
		zassert_false(group_seen[group], "group %zu drawn twice in one pass", group);
		group_seen[group] = true;
	}

	return ch_500;
}

ZTEST(lorawan_us915_region, test_join_channel_sweeps_the_octet_groups)
{
	bool group_seen[US915_UL_125_CH_COUNT / 8];
	uint32_t first;
	uint32_t second;

	reset_channels();

	first = collect_join_pass(group_seen);
	for (size_t g = 0; g < ARRAY_SIZE(group_seen); g++) {
		zassert_true(group_seen[g], "group %zu missed in the first pass", g);
	}
	zassert_not_equal(first, 0, "the first pass drew no 500 kHz channel");

	second = collect_join_pass(group_seen);
	for (size_t g = 0; g < ARRAY_SIZE(group_seen); g++) {
		zassert_true(group_seen[g], "group %zu missed in the second pass", g);
	}

	/* Each pass closes on a different 500 kHz channel. */
	zassert_not_equal(first, second);
}

/* The sweep steps over a group the network has switched off. */
ZTEST(lorawan_us915_region, test_join_channel_skips_disabled_groups)
{
	uint32_t freq;
	uint8_t dr;
	int32_t delay_ms;

	reset_channels();

	/* Keep bank 3 only: channels 24 to 31 and the 500 kHz channel 67. */
	zassert_ok(us915->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_BANKS, BIT(3)));

	for (int i = 0; i < 8; i++) {
		zassert_ok(us915->select_join_channel(ch, ch_count, &freq, &dr, &delay_ms),
			   "attempt %d", i);

		if (dr == US915_JOIN_DR_125) {
			zassert_true(freq >= channel_freq(24), "attempt %d gave %u", i, freq);
			zassert_true(freq <= channel_freq(31), "attempt %d gave %u", i, freq);
		} else {
			zassert_equal(dr, US915_JOIN_DR_500, "attempt %d", i);
			zassert_equal(freq, channel_freq(US915_UL_125_CH_COUNT + 3), "attempt %d",
				      i);
		}
	}
}

ZTEST(lorawan_us915_region, test_join_channel_without_an_enabled_channel)
{
	uint32_t freq;
	uint8_t dr;
	int32_t delay_ms;

	reset_channels();

	for (size_t i = 0; i < ch_count; i++) {
		ch[i].enabled = false;
	}

	zassert_equal(us915->select_join_channel(ch, ch_count, &freq, &dr, &delay_ms), -ENOENT);
}

/*
 * The band keeps the air clear with frequency hopping rather than a duty
 * cycle or listen-before-talk, so nothing gates the next transmission.
 */
/* 3.5.2 puts the 500 kHz channels under the digital transmission limit. */
ZTEST(lorawan_us915_region, test_500_khz_channels_cap_the_tx_power)
{
	int8_t power;

	zassert_not_null(us915->clamp_tx_power);

	for (size_t i = 0; i < US915_UL_125_CH_COUNT; i++) {
		power = US915_MAX_POWER_DBM;
		us915->clamp_tx_power(channel_freq(i), &power);
		zassert_equal(power, US915_MAX_POWER_DBM, "channel %zu", i);
	}

	for (size_t i = US915_UL_125_CH_COUNT; i < US915_UL_CH_COUNT; i++) {
		power = US915_MAX_POWER_DBM;
		us915->clamp_tx_power(channel_freq(i), &power);
		zassert_equal(power, US915_DTS_MAX_POWER_DBM, "channel %zu", i);
	}

	/* A power already under the cap is left where the TXPower table put it. */
	power = 2;
	us915->clamp_tx_power(channel_freq(US915_UL_125_CH_COUNT), &power);
	zassert_equal(power, 2);

	/* A frequency the plan does not name is not this region's to cap. */
	power = US915_MAX_POWER_DBM;
	us915->clamp_tx_power(868100000U, &power);
	zassert_equal(power, US915_MAX_POWER_DBM);
}

ZTEST(lorawan_us915_region, test_no_duty_cycle_or_lbt)
{
	uint32_t freq;
	int32_t delay_ms;

	zassert_is_null(us915->get_lbt_params);
	zassert_equal(us915->max_lbt_retries, 0);

	reset_channels();

	for (int i = 0; i < 16; i++) {
		zassert_ok(us915->select_data_channel(ch, ch_count, 0, &freq, &delay_ms));
		us915->record_tx(freq, 400);
	}
}
