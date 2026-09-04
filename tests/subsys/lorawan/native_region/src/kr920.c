/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include "region.h"

/* Channels the KR920 plan brings up on its own */
#define KR920_FREQ_CH0 922100000U
#define KR920_FREQ_CH1 922300000U
#define KR920_FREQ_CH2 922500000U

#define KR920_DEFAULT_CH_COUNT 3
#define KR920_DR_COUNT         6

#define CH_MASK_CNTL_ALL_ON 6
#define CH_MASK_CNTL_BLOCKS 5

static const struct lwan_region_ops *kr920;
static struct lwan_channel ch[CONFIG_LORAWAN_NATIVE_MAX_CHANNELS];
static size_t ch_count;

static void reset_channels(void)
{
	memset(ch, 0, sizeof(ch));
	ch_count = ARRAY_SIZE(ch);
	zassert_ok(kr920->get_default_channels(ch, &ch_count));
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

/* The plan reaches 923.3 MHz, which leaves room for four more channels
 * above the defaults; the fifth list slot stays empty.
 */
#define KR920_CFLIST_CH_COUNT 4
#define KR920_TOTAL_CH_COUNT  (KR920_DEFAULT_CH_COUNT + KR920_CFLIST_CH_COUNT)

/*
 * Give the plan more channels than the defaults alone provide, so that a mask
 * can be aimed at one that is not a join channel.
 */
static void add_cflist_channels(void)
{
	uint8_t cflist[16] = {0};

	for (int i = 0; i < KR920_CFLIST_CH_COUNT; i++) {
		sys_put_le24((922700000U + i * 200000U) / 100, &cflist[i * 3]);
	}

	zassert_ok(kr920->apply_cflist(cflist, ch, &ch_count));
}

ZTEST(lorawan_kr920_region, test_region_is_available)
{
	zassert_not_null(kr920, "KR920 is enabled but the dispatcher has no ops");
}

ZTEST(lorawan_kr920_region, test_default_channels)
{
	reset_channels();

	zassert_equal(ch_count, KR920_DEFAULT_CH_COUNT);
	zassert_equal(ch[0].frequency, KR920_FREQ_CH0);
	zassert_equal(ch[1].frequency, KR920_FREQ_CH1);
	zassert_equal(ch[2].frequency, KR920_FREQ_CH2);

	for (int i = 0; i < KR920_DEFAULT_CH_COUNT; i++) {
		zassert_true(ch[i].enabled);
		zassert_equal(ch[i].min_dr, 0);
		zassert_equal(ch[i].max_dr, 5, "the band stops at DR5");
	}
}

ZTEST(lorawan_kr920_region, test_default_channels_needs_room)
{
	size_t count = KR920_DEFAULT_CH_COUNT - 1;

	zassert_equal(kr920->get_default_channels(ch, &count), -ENOMEM);
}

ZTEST(lorawan_kr920_region, test_datarates)
{
	struct lwan_dr_params p;
	int8_t power;

	for (uint8_t dr = 0; dr < KR920_DR_COUNT; dr++) {
		zassert_ok(kr920->validate_dr(dr));
		zassert_ok(kr920->get_tx_params(dr, 0, &p, &power));
		zassert_equal(p.bw, BW_125_KHZ, "every KR920 datarate is 125 kHz");
	}

	/* DR6 to DR11 and DR14 are reserved; DR12 and DR13 are optional and
	 * not implemented, so nothing above DR5 is accepted.
	 */
	for (uint8_t dr = KR920_DR_COUNT; dr < 16; dr++) {
		zassert_equal(kr920->validate_dr(dr), -EINVAL, "DR%u accepted", dr);
	}

	zassert_ok(kr920->get_tx_params(0, 0, &p, &power));
	zassert_equal(p.sf, SF_12);
	zassert_equal(p.max_payload, 51);

	zassert_ok(kr920->get_tx_params(5, 0, &p, &power));
	zassert_equal(p.sf, SF_7);
	zassert_equal(p.max_payload, 222);
}

ZTEST(lorawan_kr920_region, test_tx_power)
{
	struct lwan_dr_params p;
	int8_t power;

	/* Index 0 is the band's maximum, and each step drops 2 dB. */
	for (uint8_t idx = 0; idx <= 7; idx++) {
		zassert_ok(kr920->validate_tx_power(idx));
		zassert_ok(kr920->get_tx_params(0, idx, &p, &power));
		zassert_equal(power, 14 - 2 * idx, "index %u", idx);
	}

	for (uint8_t idx = 8; idx < 16; idx++) {
		zassert_equal(kr920->validate_tx_power(idx), -EINVAL, "index %u", idx);
	}
}

ZTEST(lorawan_kr920_region, test_tx_power_capped_below_922_mhz)
{
	int8_t power = 14;

	zassert_not_null(kr920->clamp_tx_power);

	/* Anything under 922 MHz is held to 10 dBm however much was asked. */
	kr920->clamp_tx_power(921900000U, &power);
	zassert_equal(power, 10);

	power = 8;
	kr920->clamp_tx_power(921900000U, &power);
	zassert_equal(power, 8, "a lower power is left alone");

	power = 14;
	kr920->clamp_tx_power(922000000U, &power);
	zassert_equal(power, 14, "922 MHz itself is not below the split");

	power = 14;
	kr920->clamp_tx_power(KR920_FREQ_CH0, &power);
	zassert_equal(power, 14);
}

ZTEST(lorawan_kr920_region, test_listen_before_talk_parameters)
{
	uint32_t bandwidth_hz;
	uint32_t scan_time_ms;
	int16_t threshold_dbm;

	zassert_not_null(kr920->get_lbt_params, "the band listens before it talks");
	zassert_ok(kr920->get_lbt_params(KR920_FREQ_CH0, &bandwidth_hz, &threshold_dbm,
					 &scan_time_ms));
	zassert_equal(bandwidth_hz, 200000);
	zassert_equal(threshold_dbm, -65);
	zassert_equal(scan_time_ms, 6);
}

ZTEST(lorawan_kr920_region, test_receive_windows)
{
	struct lwan_dr_params p;
	uint32_t freq;

	/* RX1 sits on the channel the uplink went out on. */
	zassert_ok(kr920->get_rx1_params(KR920_FREQ_CH1, 5, 0, &freq, &p));
	zassert_equal(freq, KR920_FREQ_CH1);
	zassert_equal(p.sf, SF_7);

	/* The offset walks the datarate down and stops at DR0. */
	zassert_ok(kr920->get_rx1_params(KR920_FREQ_CH1, 5, 3, &freq, &p));
	zassert_equal(p.sf, SF_10);
	zassert_ok(kr920->get_rx1_params(KR920_FREQ_CH1, 2, 5, &freq, &p));
	zassert_equal(p.sf, SF_12);

	zassert_ok(kr920->get_rx2_params(0, &freq, &p));
	zassert_equal(freq, 921900000U, "RX2 is fixed at 921.9 MHz");
	zassert_equal(p.sf, SF_12);
}

ZTEST(lorawan_kr920_region, test_downlink_settings)
{
	zassert_ok(kr920->validate_dl_settings(0, 0));
	zassert_ok(kr920->validate_dl_settings(5, 5));

	/* RX1DROffset above 5 is reserved and must not be taken. */
	zassert_equal(kr920->validate_dl_settings(6, 0), -EINVAL);
	zassert_equal(kr920->validate_dl_settings(7, 0), -EINVAL);
	zassert_equal(kr920->validate_dl_settings(0, KR920_DR_COUNT), -EINVAL);
}

ZTEST(lorawan_kr920_region, test_cflist_adds_channels)
{
	reset_channels();
	add_cflist_channels();

	zassert_equal(ch_count, 8, "the list covers five slots whether or not they are used");
	for (int i = 0; i < KR920_CFLIST_CH_COUNT; i++) {
		zassert_equal(ch[KR920_DEFAULT_CH_COUNT + i].frequency, 922700000U + i * 200000U);
		zassert_true(ch[KR920_DEFAULT_CH_COUNT + i].enabled);
	}
	zassert_false(ch[7].enabled, "the empty slot stays off");
}

ZTEST(lorawan_kr920_region, test_cflist_rejects_out_of_band)
{
	uint8_t cflist[16] = {0};

	reset_channels();

	/* Below the plan, and above it. */
	sys_put_le24(920000000U / 100, &cflist[0]);
	sys_put_le24(925000000U / 100, &cflist[3]);

	zassert_ok(kr920->apply_cflist(cflist, ch, &ch_count));
	zassert_false(ch[3].enabled, "920.0 MHz is outside the plan");
	zassert_false(ch[4].enabled, "925.0 MHz is outside the plan");
}

ZTEST(lorawan_kr920_region, test_cflist_ignores_other_types)
{
	uint8_t cflist[16] = {0};

	reset_channels();
	cflist[15] = 1;

	zassert_ok(kr920->apply_cflist(cflist, ch, &ch_count));
	zassert_equal(ch_count, KR920_DEFAULT_CH_COUNT, "a type 1 list is left alone");
}

ZTEST(lorawan_kr920_region, test_adr_mask_direct)
{
	reset_channels();
	add_cflist_channels();

	/* Keep the three defaults and one added channel. */
	zassert_ok(kr920->apply_adr_channel_mask(ch, ch_count, 0, 0x000F));
	zassert_equal(enabled_count(), 4);
	zassert_true(ch[3].enabled);
	zassert_false(ch[4].enabled);
}

ZTEST(lorawan_kr920_region, test_adr_mask_all_on)
{
	reset_channels();
	add_cflist_channels();

	zassert_ok(kr920->apply_adr_channel_mask(ch, ch_count, 0, 0x0007));
	zassert_equal(enabled_count(), KR920_DEFAULT_CH_COUNT);

	/* The mask is ignored for this control value. */
	zassert_ok(kr920->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_ALL_ON, 0x0000));
	zassert_equal(enabled_count(), KR920_TOTAL_CH_COUNT);
}

ZTEST(lorawan_kr920_region, test_adr_mask_protects_join_channels)
{
	reset_channels();
	add_cflist_channels();

	/* Turning off a default channel would strand the device. */
	zassert_equal(kr920->apply_adr_channel_mask(ch, ch_count, 0, 0x00FE), -EINVAL);
	zassert_equal(enabled_count(), KR920_TOTAL_CH_COUNT, "a rejected mask changes nothing");
}

ZTEST(lorawan_kr920_region, test_adr_mask_rejects_silence)
{
	reset_channels();
	add_cflist_channels();

	zassert_equal(kr920->apply_adr_channel_mask(ch, ch_count, 0, 0x0000), -EINVAL);
	zassert_equal(enabled_count(), KR920_TOTAL_CH_COUNT);
}

ZTEST(lorawan_kr920_region, test_adr_mask_banks)
{
	reset_channels();
	add_cflist_channels();

	/*
	 * Bank 1 covers channels 16 to 31, none of which this plan has, so the
	 * request enables nothing and has to be turned down rather than
	 * silencing the device.
	 */
	zassert_equal(kr920->apply_adr_channel_mask(ch, ch_count, 1, 0xFFFF), -EINVAL);
	zassert_equal(enabled_count(), KR920_TOTAL_CH_COUNT);
}

ZTEST(lorawan_kr920_region, test_adr_mask_blocks)
{
	reset_channels();
	add_cflist_channels();

	/* Block 0 is channels 0 to 7, which is every channel here. */
	zassert_ok(kr920->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_BLOCKS, 0x0001));
	zassert_equal(enabled_count(), KR920_TOTAL_CH_COUNT);

	/* The six bits above the ten block bits are reserved. */
	zassert_equal(kr920->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_BLOCKS, 0x0400),
		      -EINVAL);
}

ZTEST(lorawan_kr920_region, test_adr_mask_rejects_reserved_control)
{
	reset_channels();

	zassert_equal(kr920->apply_adr_channel_mask(ch, ch_count, 7, 0xFFFF), -EINVAL);
}

ZTEST(lorawan_kr920_region, test_channel_selection)
{
	uint32_t freq;
	int32_t delay_ms;
	uint8_t dr;

	reset_channels();
	add_cflist_channels();

	/* A join may only go out on one of the three default channels. */
	for (int i = 0; i < 20; i++) {
		zassert_ok(kr920->select_join_channel(ch, ch_count, &freq, &dr, &delay_ms));
		zassert_true(freq == KR920_FREQ_CH0 || freq == KR920_FREQ_CH1 ||
				     freq == KR920_FREQ_CH2,
			     "join left the default channels");
		zassert_equal(dr, 0);
	}

	for (int i = 0; i < 20; i++) {
		zassert_ok(kr920->select_data_channel(ch, ch_count, 5, &freq, &delay_ms));
		zassert_true(freq >= 920900000U && freq <= 923300000U);
	}
}

ZTEST(lorawan_kr920_region, test_channel_selection_without_channels)
{
	uint32_t freq;
	int32_t delay_ms;
	uint8_t dr;

	memset(ch, 0, sizeof(ch));

	zassert_equal(kr920->select_join_channel(ch, ARRAY_SIZE(ch), &freq, &dr, &delay_ms),
		      -ENOENT);
	zassert_equal(kr920->select_data_channel(ch, ARRAY_SIZE(ch), 0, &freq, &delay_ms), -ENOENT);
}

ZTEST(lorawan_kr920_region, test_no_duty_cycle_wait)
{
	uint32_t freq;
	int32_t delay_ms;

	reset_channels();

	/*
	 * The band listens before it talks instead of holding an off-time, so
	 * recording a transmission never makes the next one wait.
	 */
	for (int i = 0; i < 10; i++) {
		kr920->record_tx(KR920_FREQ_CH0, 1000);
	}

	zassert_ok(kr920->select_data_channel(ch, ch_count, 0, &freq, &delay_ms));
}

static void *suite_setup(void)
{
	kr920 = lwan_region_get(LORAWAN_REGION_KR920);
	zassume_not_null(kr920);

	return NULL;
}

ZTEST_SUITE(lorawan_kr920_region, NULL, suite_setup, NULL, NULL, NULL);
