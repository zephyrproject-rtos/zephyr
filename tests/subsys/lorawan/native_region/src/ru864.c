/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include "region.h"

/* Channels the RU864 plan brings up on its own (RP002-1.0.5, Table 100) */
#define RU864_FREQ_CH0 868900000U
#define RU864_FREQ_CH1 869100000U

#define RU864_DEFAULT_CH_COUNT  2
#define RU864_DR_COUNT          7
#define RU864_JOIN_DR           0
#define RU864_MAX_RX1_DR_OFFSET 5

#define CH_MASK_CNTL_ALL_ON 6
#define CH_MASK_CNTL_BANKS  5

#define CFLIST_CH_COUNT      5
#define RU864_TOTAL_CH_COUNT (RU864_DEFAULT_CH_COUNT + CFLIST_CH_COUNT)

/* Operator channels in the 864 to 865 MHz sub-band the regulator allows */
#define CFLIST_BASE_HZ 864100000U
#define CFLIST_STEP_HZ 200000U

static const struct lwan_region_ops *ru864;
static struct lwan_channel ch[CONFIG_LORAWAN_NATIVE_MAX_CHANNELS];
static size_t ch_count;

static void reset_channels(void)
{
	memset(ch, 0, sizeof(ch));
	ch_count = ARRAY_SIZE(ch);
	zassert_ok(ru864->get_default_channels(ch, &ch_count));
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
 * not a join channel.
 */
static void add_cflist_channels(void)
{
	uint8_t cflist[16] = {0};

	for (int i = 0; i < CFLIST_CH_COUNT; i++) {
		sys_put_le24((CFLIST_BASE_HZ + i * CFLIST_STEP_HZ) / 100, &cflist[i * 3]);
	}

	zassert_ok(ru864->apply_cflist(cflist, ch, &ch_count));
}

ZTEST(lorawan_ru864_region, test_region_is_available)
{
	zassert_not_null(ru864, "RU864 is enabled but the dispatcher has no ops");
}

ZTEST(lorawan_ru864_region, test_channel_table_holds_the_spec_minimum)
{
	/* RP002-1.0.5 3.13.2: the device stores at least 24 channels. */
	zassert_true(CONFIG_LORAWAN_NATIVE_MAX_CHANNELS >= 24, "the table holds %d channels",
		     CONFIG_LORAWAN_NATIVE_MAX_CHANNELS);
}

ZTEST(lorawan_ru864_region, test_default_channels)
{
	reset_channels();

	zassert_equal(ch_count, RU864_DEFAULT_CH_COUNT);
	zassert_equal(ch[0].frequency, RU864_FREQ_CH0);
	zassert_equal(ch[1].frequency, RU864_FREQ_CH1);

	for (int i = 0; i < RU864_DEFAULT_CH_COUNT; i++) {
		zassert_true(ch[i].enabled);
		zassert_equal(ch[i].min_dr, 0);
		zassert_equal(ch[i].max_dr, 5, "Table 100 carries DR0 to DR5");
	}
}

ZTEST(lorawan_ru864_region, test_default_channels_needs_room)
{
	size_t count = RU864_DEFAULT_CH_COUNT - 1;

	zassert_equal(ru864->get_default_channels(ch, &count), -ENOMEM);
}

ZTEST(lorawan_ru864_region, test_datarates)
{
	struct lwan_dr_params p;
	int8_t power;

	for (uint8_t dr = 0; dr < RU864_DR_COUNT; dr++) {
		zassert_ok(ru864->validate_dr(dr));
		zassert_ok(ru864->get_tx_params(dr, 0, &p, &power));
	}

	/*
	 * DR7 is 50 kbps FSK and DR12/DR13 are SF6/SF5; none are described
	 * here, so nothing above DR6 is accepted.
	 */
	for (uint8_t dr = RU864_DR_COUNT; dr < 16; dr++) {
		zassert_equal(ru864->validate_dr(dr), -EINVAL, "DR%u accepted", dr);
	}

	zassert_ok(ru864->get_tx_params(0, 0, &p, &power));
	zassert_equal(p.sf, SF_12);
	zassert_equal(p.bw, BW_125_KHZ);
	zassert_equal(p.max_payload, 51);

	/*
	 * Table 107 holds the payload at 51 bytes through DR2, unlike the
	 * EU868 plan this band otherwise resembles.
	 */
	zassert_ok(ru864->get_tx_params(2, 0, &p, &power));
	zassert_equal(p.sf, SF_10);
	zassert_equal(p.max_payload, 51);
	zassert_ok(ru864->get_tx_params(3, 0, &p, &power));
	zassert_equal(p.max_payload, 115);

	/* DR6 is the one entry that leaves 125 kHz (Table 102). */
	zassert_ok(ru864->get_tx_params(6, 0, &p, &power));
	zassert_equal(p.sf, SF_7);
	zassert_equal(p.bw, BW_250_KHZ);
	zassert_equal(p.max_payload, 222);

	zassert_equal(ru864->get_tx_params(RU864_DR_COUNT, 0, &p, &power), -EINVAL);
}

ZTEST(lorawan_ru864_region, test_tx_power)
{
	struct lwan_dr_params p;
	int8_t power;

	/* Index 0 is MaxEIRP at 16 dBm and each step drops 2 dB (Table 105). */
	for (uint8_t idx = 0; idx <= 7; idx++) {
		zassert_ok(ru864->validate_tx_power(idx));
		zassert_ok(ru864->get_tx_params(0, idx, &p, &power));
		zassert_equal(power, 16 - 2 * idx, "index %u", idx);
	}

	for (uint8_t idx = 8; idx < 16; idx++) {
		zassert_equal(ru864->validate_tx_power(idx), -EINVAL, "index %u", idx);
	}
}

ZTEST(lorawan_ru864_region, test_no_listen_before_talk)
{
	/* The band holds to a duty cycle instead (Table 100). */
	zassert_is_null(ru864->get_lbt_params);
	zassert_equal(ru864->max_lbt_retries, 0);
	zassert_is_null(ru864->clamp_tx_power, "one EIRP limit covers the band");
}

ZTEST(lorawan_ru864_region, test_receive_windows)
{
	struct lwan_dr_params p;
	uint32_t freq;

	/* RX1 sits on the channel the uplink went out on (3.13.7). */
	zassert_ok(ru864->get_rx1_params(RU864_FREQ_CH1, 5, 0, &freq, &p));
	zassert_equal(freq, RU864_FREQ_CH1);
	zassert_equal(p.sf, SF_7);

	/*
	 * Across DR0 to DR6 Table 109 is the uplink datarate lowered by the
	 * offset and clamped at DR0.
	 */
	for (uint8_t dr = 0; dr < RU864_DR_COUNT; dr++) {
		for (uint8_t off = 0; off <= RU864_MAX_RX1_DR_OFFSET; off++) {
			uint8_t expect = (dr > off) ? (dr - off) : 0;
			struct lwan_dr_params want;

			zassert_ok(ru864->get_rx1_params(RU864_FREQ_CH0, dr, off, &freq, &p),
				   "DR%u offset %u", dr, off);
			zassert_ok(ru864->get_rx2_params(expect, &freq, &want));
			zassert_equal(p.sf, want.sf, "DR%u offset %u", dr, off);
			zassert_equal(p.bw, want.bw, "DR%u offset %u", dr, off);
		}
	}

	/* 3.13.7 reserves offsets 6 and 7 and tells the device to refuse them. */
	for (uint8_t off = RU864_MAX_RX1_DR_OFFSET + 1; off <= 7; off++) {
		zassert_equal(ru864->get_rx1_params(RU864_FREQ_CH0, 0, off, &freq, &p), -EINVAL,
			      "offset %u", off);
	}

	zassert_equal(ru864->get_rx1_params(RU864_FREQ_CH1, RU864_DR_COUNT, 0, &freq, &p), -EINVAL);
}

ZTEST(lorawan_ru864_region, test_rx2_defaults_to_dr0)
{
	struct lwan_dr_params p;
	uint32_t freq;

	zassert_ok(ru864->get_rx2_params(0, &freq, &p));
	zassert_equal(freq, RU864_FREQ_CH1, "RX2 is fixed at 869.1 MHz");
	zassert_equal(p.sf, SF_12, "DR0 is SF12/125 kHz");

	/*
	 * DR0 is what lwan_region_ops falls back to, so unlike AS923 this
	 * region leaves default_rx2_dr unset (3.13.7).
	 */
	zassert_equal(ru864->default_rx2_dr, 0);

	zassert_equal(ru864->get_rx2_params(RU864_DR_COUNT, &freq, &p), -EINVAL);
}

ZTEST(lorawan_ru864_region, test_downlink_settings)
{
	zassert_ok(ru864->validate_dl_settings(0, 0));
	zassert_ok(ru864->validate_dl_settings(RU864_MAX_RX1_DR_OFFSET, RU864_DR_COUNT - 1));

	zassert_equal(ru864->validate_dl_settings(RU864_MAX_RX1_DR_OFFSET + 1, 0), -EINVAL);
	zassert_equal(ru864->validate_dl_settings(0, RU864_DR_COUNT), -EINVAL);
}

ZTEST(lorawan_ru864_region, test_cflist_adds_channels)
{
	reset_channels();
	add_cflist_channels();

	zassert_equal(ch_count, RU864_TOTAL_CH_COUNT);
	for (int i = 0; i < CFLIST_CH_COUNT; i++) {
		zassert_equal(ch[RU864_DEFAULT_CH_COUNT + i].frequency,
			      CFLIST_BASE_HZ + i * CFLIST_STEP_HZ);
		zassert_true(ch[RU864_DEFAULT_CH_COUNT + i].enabled);
	}
}

ZTEST(lorawan_ru864_region, test_cflist_rejects_out_of_band)
{
	uint8_t cflist[16] = {0};

	reset_channels();
	add_cflist_channels();

	sys_put_le24(863900000U / 100, &cflist[0]);
	sys_put_le24(870100000U / 100, &cflist[3]);
	sys_put_le24(864100000U / 100, &cflist[6]);

	zassert_ok(ru864->apply_cflist(cflist, ch, &ch_count));
	zassert_false(ch[2].enabled, "863.9 MHz is outside the plan");
	zassert_equal(ch[2].frequency, 0);
	zassert_false(ch[3].enabled, "870.1 MHz is outside the plan");
	zassert_equal(ch[3].frequency, 0);
	zassert_equal(ch[4].frequency, 864100000U);
	zassert_true(ch[4].enabled, "an in-band entry is still applied");
}

ZTEST(lorawan_ru864_region, test_cflist_accepts_plan_edges)
{
	uint8_t cflist[16] = {0};

	reset_channels();

	/* The first and last frequencies the plan carries. */
	sys_put_le24(864000000U / 100, &cflist[0]);
	sys_put_le24(870000000U / 100, &cflist[3]);

	zassert_ok(ru864->apply_cflist(cflist, ch, &ch_count));
	zassert_equal(ch[2].frequency, 864000000U);
	zassert_true(ch[2].enabled);
	zassert_equal(ch[3].frequency, 870000000U);
	zassert_true(ch[3].enabled);
}

ZTEST(lorawan_ru864_region, test_cflist_clears_slots_it_omits)
{
	uint8_t cflist[16] = {0};

	reset_channels();
	add_cflist_channels();

	/* Join again against a list that names no channel at all. */
	zassert_ok(ru864->apply_cflist(cflist, ch, &ch_count));

	for (int i = 0; i < CFLIST_CH_COUNT; i++) {
		zassert_false(ch[RU864_DEFAULT_CH_COUNT + i].enabled, "slot %d stayed on", i);
		zassert_equal(ch[RU864_DEFAULT_CH_COUNT + i].frequency, 0, "slot %d kept %u Hz", i,
			      ch[RU864_DEFAULT_CH_COUNT + i].frequency);
	}
	zassert_equal(enabled_count(), RU864_DEFAULT_CH_COUNT);

	/* A channel the network dropped must not come back with ChMaskCntl=6. */
	zassert_ok(ru864->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_ALL_ON, 0xffff));
	zassert_equal(enabled_count(), RU864_DEFAULT_CH_COUNT);
}

ZTEST(lorawan_ru864_region, test_cflist_ignores_other_types)
{
	uint8_t cflist[16] = {0};

	reset_channels();

	/*
	 * 3.13.4 also allows a type 1 list, read against the 800 MHz fixed
	 * channel list, but 3.3.1 words that as a MAY for the dynamic plan
	 * regions and the backend does not offer it.
	 */
	cflist[15] = 1;

	zassert_ok(ru864->apply_cflist(cflist, ch, &ch_count));
	zassert_equal(ch_count, RU864_DEFAULT_CH_COUNT, "a type 1 list is left alone");
}

ZTEST(lorawan_ru864_region, test_adr_mask_direct)
{
	reset_channels();
	add_cflist_channels();

	/* Keep the two defaults and one added channel. */
	zassert_ok(ru864->apply_adr_channel_mask(ch, ch_count, 0, 0x0007));
	zassert_equal(enabled_count(), 3);
	zassert_true(ch[2].enabled);
	zassert_false(ch[3].enabled);
}

ZTEST(lorawan_ru864_region, test_adr_mask_all_on)
{
	reset_channels();
	add_cflist_channels();

	zassert_ok(ru864->apply_adr_channel_mask(ch, ch_count, 0, 0x0003));
	zassert_equal(enabled_count(), RU864_DEFAULT_CH_COUNT);

	/* The mask is ignored for this control value (Table 106). */
	zassert_ok(ru864->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_ALL_ON, 0x0000));
	zassert_equal(enabled_count(), RU864_TOTAL_CH_COUNT);
}

ZTEST(lorawan_ru864_region, test_adr_mask_protects_join_channels)
{
	reset_channels();
	add_cflist_channels();

	/* Turning off a default channel would strand the device. */
	zassert_equal(ru864->apply_adr_channel_mask(ch, ch_count, 0, 0x007C), -EINVAL);
	zassert_equal(enabled_count(), RU864_TOTAL_CH_COUNT, "a rejected mask changes nothing");

	/* Bank 0 holds both of them, so clearing it is refused as well. */
	zassert_equal(ru864->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_BANKS, 0x0002),
		      -EINVAL);
	zassert_equal(enabled_count(), RU864_TOTAL_CH_COUNT);
}

ZTEST(lorawan_ru864_region, test_adr_mask_rejects_silence)
{
	reset_channels();
	add_cflist_channels();

	zassert_equal(ru864->apply_adr_channel_mask(ch, ch_count, 0, 0x0000), -EINVAL);
	zassert_equal(enabled_count(), RU864_TOTAL_CH_COUNT);
}

ZTEST(lorawan_ru864_region, test_adr_mask_banks)
{
	reset_channels();
	add_cflist_channels();

	/* Bank 0 covers channels 0 to 7, which is every channel here. */
	zassert_ok(ru864->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_BANKS, 0x0001));
	zassert_equal(enabled_count(), RU864_TOTAL_CH_COUNT);

	/* The six bits above the ten bank bits are RFU. */
	zassert_equal(ru864->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_BANKS, 0x0400),
		      -EINVAL);

	/* Bank 2 is channels 16 to 23, none of which this plan has. */
	zassert_equal(ru864->apply_adr_channel_mask(ch, ch_count, CH_MASK_CNTL_BANKS, 0x0005),
		      -EINVAL);
	zassert_equal(enabled_count(), RU864_TOTAL_CH_COUNT);
}

ZTEST(lorawan_ru864_region, test_adr_mask_rejects_undefined_channel)
{
	reset_channels();

	/* Bit 15 names a channel the two defaults do not reach. */
	zassert_equal(ru864->apply_adr_channel_mask(ch, ch_count, 0, 0x8003), -EINVAL);
	zassert_equal(enabled_count(), RU864_DEFAULT_CH_COUNT);
}

ZTEST(lorawan_ru864_region, test_adr_mask_rejects_block_past_the_table)
{
	reset_channels();
	add_cflist_channels();

	/* Block 1 is channels 16 to 31, none of which this plan has. */
	zassert_equal(ru864->apply_adr_channel_mask(ch, ch_count, 1, 0xFFFF), -EINVAL);
	zassert_equal(enabled_count(), RU864_TOTAL_CH_COUNT);
}

ZTEST(lorawan_ru864_region, test_adr_mask_rejects_reserved_control)
{
	reset_channels();

	zassert_equal(ru864->apply_adr_channel_mask(ch, ch_count, 7, 0xFFFF), -EINVAL);
}

ZTEST(lorawan_ru864_region, test_channel_selection)
{
	uint32_t freq;
	int32_t delay_ms;
	uint8_t dr;

	reset_channels();
	add_cflist_channels();

	/*
	 * A join may only go out on one of the two default channels. Table 101
	 * opens them to DR0 through DR5 and the region takes the slowest.
	 */
	for (int i = 0; i < 20; i++) {
		zassert_ok(ru864->select_join_channel(ch, ch_count, &freq, &dr, &delay_ms));
		zassert_true(freq == RU864_FREQ_CH0 || freq == RU864_FREQ_CH1,
			     "join left the default channels");
		zassert_equal(dr, RU864_JOIN_DR);
	}

	/*
	 * Data may leave on a default channel or on one the CFList added.
	 * One draw can be either, so the run has to show that a CFList
	 * channel is actually reachable.
	 */
	bool saw_cflist = false;

	for (int i = 0; i < 20; i++) {
		bool on_cflist;

		zassert_ok(ru864->select_data_channel(ch, ch_count, 5, &freq, &delay_ms));
		on_cflist = (freq >= CFLIST_BASE_HZ && freq < RU864_FREQ_CH0);
		zassert_true(freq == RU864_FREQ_CH0 || freq == RU864_FREQ_CH1 || on_cflist,
			     "DR5 left the plan: %u Hz", freq);
		saw_cflist = saw_cflist || on_cflist;
	}

	zassert_true(saw_cflist, "DR5 never selected a CFList channel");
}

ZTEST(lorawan_ru864_region, test_channel_selection_skips_undefined_slots)
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
	ch_count = RU864_DEFAULT_CH_COUNT + 1;
	ch[0].enabled = false;
	ch[1].enabled = false;
	ch[RU864_DEFAULT_CH_COUNT].enabled = true;

	zassert_equal(ch[RU864_DEFAULT_CH_COUNT].frequency, 0, "the slot under test carries one");
	zassert_equal(ru864->select_data_channel(ch, ch_count, 0, &freq, &delay_ms), -ENOENT);
}

ZTEST(lorawan_ru864_region, test_channel_selection_without_channels)
{
	uint32_t freq;
	int32_t delay_ms;
	uint8_t dr;

	memset(ch, 0, sizeof(ch));

	zassert_equal(ru864->select_join_channel(ch, ARRAY_SIZE(ch), &freq, &dr, &delay_ms),
		      -ENOENT);
	zassert_equal(ru864->select_data_channel(ch, ARRAY_SIZE(ch), 0, &freq, &delay_ms), -ENOENT);
}

ZTEST(lorawan_ru864_region, test_duty_cycle_holds_the_band_off)
{
	uint32_t freq;
	int32_t delay_ms = 0;
	uint8_t dr = 0xff;

	reset_channels();

	/*
	 * The band holds to 1% rather than listening, so a second of airtime
	 * owes 99 seconds of silence across every channel (Table 100).
	 */
	ru864->record_tx(RU864_FREQ_CH0, 1000);

	zassert_equal(ru864->select_data_channel(ch, ch_count, 0, &freq, &delay_ms), -ENOBUFS);
	zassert_true(delay_ms > 98000 && delay_ms <= 99001, "waiting %d ms", delay_ms);

	zassert_equal(ru864->select_data_channel(ch, ch_count, 0, &freq, &delay_ms), -ENOBUFS,
		      "the off-time covers the other default channel too");

	zassert_equal(ru864->select_join_channel(ch, ch_count, &freq, &dr, &delay_ms), -ENOBUFS);
	zassert_equal(dr, RU864_JOIN_DR, "join still reports DR0");
}

static void *suite_setup(void)
{
	ru864 = lwan_region_get(LORAWAN_REGION_RU864);
	zassume_not_null(ru864);

	return NULL;
}

/* Zero airtime leaves no off-time, clearing what an earlier test recorded. */
static void open_the_band(void *fixture)
{
	ARG_UNUSED(fixture);

	ru864->record_tx(RU864_FREQ_CH0, 0);
}

ZTEST_SUITE(lorawan_ru864_region, NULL, suite_setup, open_the_band, NULL, NULL);
