/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include "region.h"

/* Channels the AS923 plan brings up on its own (RP002-1.0.5, Table 65) */
#define AS923_FREQ_CH0 923200000U
#define AS923_FREQ_CH1 923400000U

#define AS923_DEFAULT_CH_COUNT 2
#define AS923_DR_COUNT         7
#define AS923_JOIN_DR          2

#define CH_MASK_CNTL_ALL_ON 6
#define CH_MASK_CNTL_BANKS  5

#define CFLIST_CH_COUNT      5
#define AS923_TOTAL_CH_COUNT (AS923_DEFAULT_CH_COUNT + CFLIST_CH_COUNT)

static const struct lwan_region_ops *as923;
static struct lwan_channel ch[CONFIG_LORAWAN_NATIVE_MAX_CHANNELS];
static size_t ch_count;

static void reset_channels(void)
{
	memset(ch, 0, sizeof(ch));
	ch_count = ARRAY_SIZE(ch);
	zassert_ok(as923->get_default_channels(ch, &ch_count));
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

/*
 * Fill every CFList slot so that a mask can be aimed at a channel that is
 * not a join channel. The plan places no band limit on an added frequency,
 * so these sit just above the defaults.
 */
static void add_cflist_channels(void)
{
	uint8_t cflist[16] = {0};

	for (int i = 0; i < CFLIST_CH_COUNT; i++) {
		sys_put_le24((923600000U + i * 200000U) / 100, &cflist[i * 3]);
	}

	zassert_ok(as923->apply_cflist(cflist, ch, &ch_count));
}

ZTEST(lorawan_as923_region, test_region_is_available)
{
	zassert_not_null(as923, "AS923 is enabled but the dispatcher has no ops");
}

ZTEST(lorawan_as923_region, test_channel_table_holds_the_spec_minimum)
{
	/* RP002-1.0.5 3.10.2: the device stores at least 24 channels. */
	zassert_true(CONFIG_LORAWAN_NATIVE_MAX_CHANNELS >= 24, "the table holds %d channels",
		     CONFIG_LORAWAN_NATIVE_MAX_CHANNELS);
}

ZTEST(lorawan_as923_region, test_default_channels)
{
	reset_channels();

	zassert_equal(ch_count, AS923_DEFAULT_CH_COUNT);
	zassert_equal(ch[0].frequency, AS923_FREQ_CH0);
	zassert_equal(ch[1].frequency, AS923_FREQ_CH1);

	for (int i = 0; i < AS923_DEFAULT_CH_COUNT; i++) {
		zassert_true(ch[i].enabled);
		zassert_equal(ch[i].min_dr, 0);
		zassert_equal(ch[i].max_dr, 5, "Table 65 carries DR0 to DR5");
	}
}

ZTEST(lorawan_as923_region, test_default_channels_needs_room)
{
	size_t count = AS923_DEFAULT_CH_COUNT - 1;

	zassert_equal(as923->get_default_channels(ch, &count), -ENOMEM);
}

ZTEST(lorawan_as923_region, test_datarates)
{
	struct lwan_dr_params p;
	int8_t power;

	for (uint8_t dr = 0; dr < AS923_DR_COUNT; dr++) {
		zassert_ok(as923->validate_dr(dr));
		zassert_ok(as923->get_tx_params(dr, 0, &p, &power));
	}

	/*
	 * DR7 is 50 kbps FSK and DR12/DR13 are SF6/SF5; none are described
	 * here, so nothing above DR6 is accepted.
	 */
	for (uint8_t dr = AS923_DR_COUNT; dr < 16; dr++) {
		zassert_equal(as923->validate_dr(dr), -EINVAL, "DR%u accepted", dr);
	}

	zassert_ok(as923->get_tx_params(0, 0, &p, &power));
	zassert_equal(p.sf, SF_12);
	zassert_equal(p.bw, BW_125_KHZ);
	zassert_equal(p.max_payload, 51);

	/* DR6 is the one entry that leaves 125 kHz (Table 67). */
	zassert_ok(as923->get_tx_params(6, 0, &p, &power));
	zassert_equal(p.sf, SF_7);
	zassert_equal(p.bw, BW_250_KHZ);
	zassert_equal(p.max_payload, 222);

	zassert_equal(as923->get_tx_params(AS923_DR_COUNT, 0, &p, &power), -EINVAL);
}

ZTEST(lorawan_as923_region, test_tx_power)
{
	struct lwan_dr_params p;
	int8_t power;

	/* Index 0 is MaxEIRP at 16 dBm and each step drops 2 dB (Table 70). */
	for (uint8_t idx = 0; idx <= 7; idx++) {
		zassert_ok(as923->validate_tx_power(idx));
		zassert_ok(as923->get_tx_params(0, idx, &p, &power));
		zassert_equal(power, 16 - 2 * idx, "index %u", idx);
	}

	for (uint8_t idx = 8; idx < 16; idx++) {
		zassert_equal(as923->validate_tx_power(idx), -EINVAL, "index %u", idx);
	}
}

ZTEST(lorawan_as923_region, test_no_listen_before_talk)
{
	/*
	 * RP002-1.0.5 3.10.2 asks for LBT only of a device operated in Japan,
	 * which also needs the AS923_FREQ_OFFSET this region cannot select.
	 */
	zassert_is_null(as923->get_lbt_params);
	zassert_equal(as923->max_lbt_retries, 0);
}

ZTEST(lorawan_as923_region, test_receive_windows)
{
	struct lwan_dr_params p;
	uint32_t freq;

	/* RX1 sits on the channel the uplink went out on. */
	zassert_ok(as923->get_rx1_params(AS923_FREQ_CH1, 5, 0, &freq, &p));
	zassert_equal(freq, AS923_FREQ_CH1);
	zassert_equal(p.sf, SF_7);

	/* Table 74 walks the datarate down and stops at DR0. */
	zassert_ok(as923->get_rx1_params(AS923_FREQ_CH1, 5, 3, &freq, &p));
	zassert_equal(p.sf, SF_10);
	zassert_ok(as923->get_rx1_params(AS923_FREQ_CH1, 2, 5, &freq, &p));
	zassert_equal(p.sf, SF_12);

	/* Offsets 6 and 7 move the other way and can reach past the plan. */
	zassert_equal(as923->get_rx1_params(AS923_FREQ_CH1, 6, 7, &freq, &p), -EINVAL);

	zassert_equal(as923->get_rx1_params(AS923_FREQ_CH1, AS923_DR_COUNT, 0, &freq, &p), -EINVAL);
	zassert_equal(as923->get_rx1_params(AS923_FREQ_CH1, 0, 8, &freq, &p), -EINVAL);
}

ZTEST(lorawan_as923_region, test_rx2_defaults_to_dr2)
{
	struct lwan_dr_params p;
	uint32_t freq;

	zassert_ok(as923->get_rx2_params(2, &freq, &p));
	zassert_equal(freq, AS923_FREQ_CH0, "RX2 is fixed at 923.2 MHz");
	zassert_equal(p.sf, SF_10, "DR2 is SF10/125 kHz");

	/*
	 * The window that receives the Join-Accept opens before the session
	 * holds a datarate, so the region has to name DR2 itself (3.10.7).
	 */
	zassert_equal(as923->default_rx2_dr, 2);

	zassert_equal(as923->get_rx2_params(AS923_DR_COUNT, &freq, &p), -EINVAL);
}

ZTEST(lorawan_as923_region, test_downlink_settings)
{
	zassert_ok(as923->validate_dl_settings(0, 0));

	/* Table 74 spans eight RX1DROffset columns, 0 through 7. */
	zassert_ok(as923->validate_dl_settings(7, 6));

	zassert_equal(as923->validate_dl_settings(8, 0), -EINVAL);
	zassert_equal(as923->validate_dl_settings(0, AS923_DR_COUNT), -EINVAL);
}

ZTEST(lorawan_as923_region, test_cflist_adds_channels)
{
	reset_channels();
	add_cflist_channels();

	zassert_equal(ch_count, AS923_TOTAL_CH_COUNT);
	for (int i = 0; i < CFLIST_CH_COUNT; i++) {
		zassert_equal(ch[AS923_DEFAULT_CH_COUNT + i].frequency, 923600000U + i * 200000U);
		zassert_true(ch[AS923_DEFAULT_CH_COUNT + i].enabled);
	}
}

ZTEST(lorawan_as923_region, test_cflist_clears_slots_it_omits)
{
	uint8_t cflist[16] = {0};

	reset_channels();
	add_cflist_channels();

	/* Join again against a list that names no channel at all. */
	zassert_ok(as923->apply_cflist(cflist, ch, &ch_count));

	for (int i = 0; i < CFLIST_CH_COUNT; i++) {
		zassert_false(ch[AS923_DEFAULT_CH_COUNT + i].enabled, "slot %d stayed on", i);
	}
	zassert_equal(enabled_count(), AS923_DEFAULT_CH_COUNT);
}

ZTEST(lorawan_as923_region, test_cflist_ignores_other_types)
{
	uint8_t cflist[16] = {0};

	reset_channels();

	/*
	 * The plan is dynamic, so a type 1 list carries nothing it can use
	 * (3.10.4).
	 */
	cflist[15] = 1;

	zassert_ok(as923->apply_cflist(cflist, ch, &ch_count));
	zassert_equal(ch_count, AS923_DEFAULT_CH_COUNT, "a type 1 list is left alone");
}

ZTEST(lorawan_as923_region, test_adr_mask_direct)
{
	reset_channels();
	add_cflist_channels();

	/* Keep the two defaults and one added channel. */
	zassert_ok(as923->apply_adr_channel_mask(ch, ch_count, 0, 0x0007));
	zassert_equal(enabled_count(), 3);
	zassert_true(ch[2].enabled);
	zassert_false(ch[3].enabled);
}

ZTEST(lorawan_as923_region, test_adr_mask_all_on)
{
	reset_channels();
	add_cflist_channels();

	zassert_ok(as923->apply_adr_channel_mask(ch, ch_count, 0, 0x0003));
	zassert_equal(enabled_count(), AS923_DEFAULT_CH_COUNT);

	/* The mask is ignored for this control value (Table 71). */
	zassert_ok(as923->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_ALL_ON, 0x0000));
	zassert_equal(enabled_count(), AS923_TOTAL_CH_COUNT);
}

ZTEST(lorawan_as923_region, test_adr_mask_protects_join_channels)
{
	reset_channels();
	add_cflist_channels();

	/* Turning off a default channel would strand the device. */
	zassert_equal(as923->apply_adr_channel_mask(ch, ch_count, 0, 0x007C), -EINVAL);
	zassert_equal(enabled_count(), AS923_TOTAL_CH_COUNT, "a rejected mask changes nothing");

	/* Bank 0 holds both of them, so clearing it is refused as well. */
	zassert_equal(as923->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_BANKS, 0x0002),
		      -EINVAL);
	zassert_equal(enabled_count(), AS923_TOTAL_CH_COUNT);
}

ZTEST(lorawan_as923_region, test_adr_mask_rejects_silence)
{
	reset_channels();
	add_cflist_channels();

	zassert_equal(as923->apply_adr_channel_mask(ch, ch_count, 0, 0x0000), -EINVAL);
	zassert_equal(enabled_count(), AS923_TOTAL_CH_COUNT);
}

ZTEST(lorawan_as923_region, test_adr_mask_banks)
{
	reset_channels();
	add_cflist_channels();

	/* Bank 0 covers channels 0 to 7, which is every channel here. */
	zassert_ok(as923->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_BANKS, 0x0001));
	zassert_equal(enabled_count(), AS923_TOTAL_CH_COUNT);

	/* The six bits above the ten bank bits are RFU. */
	zassert_equal(as923->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_BANKS, 0x0400),
		      -EINVAL);
}

ZTEST(lorawan_as923_region, test_adr_mask_rejects_undefined_channel)
{
	reset_channels();

	/* Bit 15 names a channel the two defaults do not reach. */
	zassert_equal(as923->apply_adr_channel_mask(ch, ch_count, 0, 0x8003), -EINVAL);
	zassert_equal(enabled_count(), AS923_DEFAULT_CH_COUNT);
}

ZTEST(lorawan_as923_region, test_adr_mask_rejects_block_past_the_table)
{
	reset_channels();
	add_cflist_channels();

	/* Block 1 is channels 16 to 31, none of which this plan has. */
	zassert_equal(as923->apply_adr_channel_mask(ch, ch_count, 1, 0xFFFF), -EINVAL);
	zassert_equal(enabled_count(), AS923_TOTAL_CH_COUNT);
}

ZTEST(lorawan_as923_region, test_adr_mask_rejects_reserved_control)
{
	reset_channels();

	zassert_equal(as923->apply_adr_channel_mask(ch, ch_count, 7, 0xFFFF), -EINVAL);
}

ZTEST(lorawan_as923_region, test_channel_selection)
{
	uint32_t freq;
	int32_t delay_ms;
	uint8_t dr;

	reset_channels();
	add_cflist_channels();

	/* A join may only go out on one of the two default channels, at DR2. */
	for (int i = 0; i < 20; i++) {
		zassert_ok(as923->select_join_channel(ch, ch_count, &freq, &dr, &delay_ms));
		zassert_true(freq == AS923_FREQ_CH0 || freq == AS923_FREQ_CH1,
			     "join left the default channels");
		zassert_equal(dr, AS923_JOIN_DR);
	}

	for (int i = 0; i < 20; i++) {
		zassert_ok(as923->select_data_channel(ch, ch_count, 5, &freq, &delay_ms));
		zassert_true(freq >= AS923_FREQ_CH0);
	}
}

ZTEST(lorawan_as923_region, test_channel_selection_skips_undefined_slots)
{
	uint32_t freq;
	int32_t delay_ms;

	reset_channels();

	/*
	 * A channel mask set from the application enables slots by index
	 * without reading the frequency, so a slot the CFList left empty can
	 * arrive here enabled, carrying a zeroed datarate range that DR0
	 * falls inside. Selecting it would put the radio on 0 Hz.
	 */
	ch_count = AS923_DEFAULT_CH_COUNT + 1;
	ch[0].enabled = false;
	ch[1].enabled = false;
	ch[AS923_DEFAULT_CH_COUNT].enabled = true;

	zassert_equal(ch[AS923_DEFAULT_CH_COUNT].frequency, 0, "the slot under test carries one");
	zassert_equal(as923->select_data_channel(ch, ch_count, 0, &freq, &delay_ms), -ENOENT);
}

ZTEST(lorawan_as923_region, test_channel_selection_without_channels)
{
	uint32_t freq;
	int32_t delay_ms;
	uint8_t dr;

	memset(ch, 0, sizeof(ch));

	zassert_equal(as923->select_join_channel(ch, ARRAY_SIZE(ch), &freq, &dr, &delay_ms),
		      -ENOENT);
	zassert_equal(as923->select_data_channel(ch, ARRAY_SIZE(ch), 0, &freq, &delay_ms), -ENOENT);
}

ZTEST(lorawan_as923_region, test_duty_cycle_holds_the_band_off)
{
	uint32_t freq;
	int32_t delay_ms = 0;

	reset_channels();

	/*
	 * The band holds to 1% rather than listening, so a second of airtime
	 * owes 99 seconds of silence across every channel (Table 65).
	 */
	as923->record_tx(AS923_FREQ_CH0, 1000);

	zassert_equal(as923->select_data_channel(ch, ch_count, 0, &freq, &delay_ms), -ENOBUFS);
	zassert_true(delay_ms > 98000 && delay_ms <= 99001, "waiting %d ms", delay_ms);

	zassert_equal(as923->select_data_channel(ch, ch_count, 0, &freq, &delay_ms), -ENOBUFS,
		      "the off-time covers the other default channel too");
}

static void *suite_setup(void)
{
	as923 = lwan_region_get(LORAWAN_REGION_AS923);
	zassume_not_null(as923);

	return NULL;
}

ZTEST_SUITE(lorawan_as923_region, NULL, suite_setup, NULL, NULL, NULL);
