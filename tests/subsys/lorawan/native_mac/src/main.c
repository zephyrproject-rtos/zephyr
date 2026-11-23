/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "lorawan.h"
#include "mac/mac_commands.h"
#include "region/region.h"

/* TxParamSetupReq, and the answer it is owed */
#define CID_TX_PARAM_SETUP	0x09
#define TX_PARAM_DL_DWELL	BIT(5)
#define TX_PARAM_UL_DWELL	BIT(4)

/*
 * A region that caps the dwell time, and one that does not. Their presence
 * decides whether the command is taken; the floored pair below also gives
 * the lookups an answer, so the datarate floor is visible.
 */
static int stub_tx_params(uint8_t dr, uint8_t tx_power_idx, struct lwan_dr_params *p,
			  int8_t *power_dbm)
{
	ARG_UNUSED(dr);
	ARG_UNUSED(tx_power_idx);
	ARG_UNUSED(p);
	ARG_UNUSED(power_dbm);

	return 0;
}

static int stub_rx1_params(uint32_t tx_freq, uint8_t tx_dr, uint8_t offset, uint32_t *rx1_freq,
			   struct lwan_dr_params *p)
{
	ARG_UNUSED(tx_freq);
	ARG_UNUSED(tx_dr);
	ARG_UNUSED(offset);
	ARG_UNUSED(rx1_freq);
	ARG_UNUSED(p);

	return 0;
}

static const struct lwan_region_ops capped_ops = {
	.get_tx_params = stub_tx_params,
	.get_rx1_params = stub_rx1_params,
	.get_dwell_tx_params = stub_tx_params,
	.get_dwell_rx1_params = stub_rx1_params,
};

static const struct lwan_region_ops uncapped_ops = {
	.get_tx_params = stub_tx_params,
	.get_rx1_params = stub_rx1_params,
};

/* A band whose TXPower ladder starts at 30 dBm and steps 2 dB per index */
#define EIRP_REGION_MAX_DBM 30

static int eirp_tx_params(uint8_t dr, uint8_t tx_power_idx, struct lwan_dr_params *p,
			  int8_t *power_dbm)
{
	ARG_UNUSED(dr);
	ARG_UNUSED(p);

	*power_dbm = EIRP_REGION_MAX_DBM - (int8_t)(2 * tx_power_idx);
	return 0;
}

static const struct lwan_region_ops eirp_ops = {
	.max_eirp_dbm = EIRP_REGION_MAX_DBM,
	.get_tx_params = eirp_tx_params,
	.get_rx1_params = stub_rx1_params,
	.get_dwell_tx_params = eirp_tx_params,
	.get_dwell_rx1_params = stub_rx1_params,
};

/* Datarate the floored region refuses below while the cap is in force */
#define FLOORED_DWELL_MIN_DR 2

static int floored_dwell_tx_params(uint8_t dr, uint8_t tx_power_idx, struct lwan_dr_params *p,
				   int8_t *power_dbm)
{
	if (dr < FLOORED_DWELL_MIN_DR) {
		return -EINVAL;
	}

	return stub_tx_params(dr, tx_power_idx, p, power_dbm);
}

static const struct lwan_region_ops floored_ops = {
	.get_tx_params = stub_tx_params,
	.get_rx1_params = stub_rx1_params,
	.get_dwell_tx_params = floored_dwell_tx_params,
	.get_dwell_rx1_params = stub_rx1_params,
};

static struct lwan_ctx ctx;

static void feed(uint8_t param)
{
	uint8_t fopts[] = {CID_TX_PARAM_SETUP, param};

	mac_cmd_process_dl_fopts(&ctx, fopts, sizeof(fopts));
}

static void before(void *unused)
{
	ARG_UNUSED(unused);

	memset(&ctx, 0, sizeof(ctx));
	ctx.region = &capped_ops;
}

ZTEST(lorawan_native_mac, test_tx_param_setup_sets_dwell)
{
	feed(TX_PARAM_UL_DWELL | TX_PARAM_DL_DWELL | 5U);

	zassert_true(ctx.mac.ul_dwell_time);
	zassert_true(ctx.mac.dl_dwell_time);
	zassert_equal(ctx.mac.max_eirp_dbm, 16);
	zassert_true(ctx.mac.tx_param_setup_ans_pending);
}

ZTEST(lorawan_native_mac, test_tx_param_setup_clears_dwell)
{
	feed(TX_PARAM_UL_DWELL | TX_PARAM_DL_DWELL | 5U);
	feed(0U);

	zassert_false(ctx.mac.ul_dwell_time);
	zassert_false(ctx.mac.dl_dwell_time);
}

ZTEST(lorawan_native_mac, test_tx_param_setup_eirp_table)
{
	/* The index reaches both ends of the TS001 MaxEIRP table. */
	feed(0U);
	zassert_equal(ctx.mac.max_eirp_dbm, 8);

	feed(15U);
	zassert_equal(ctx.mac.max_eirp_dbm, 36);
}

ZTEST(lorawan_native_mac, test_tx_param_setup_uplink_only_dwell)
{
	feed(TX_PARAM_UL_DWELL | 5U);

	zassert_true(ctx.mac.ul_dwell_time);
	zassert_false(ctx.mac.dl_dwell_time);
}

ZTEST(lorawan_native_mac, test_uncapped_region_ignores_the_command)
{
	ctx.region = &uncapped_ops;

	feed(TX_PARAM_UL_DWELL | TX_PARAM_DL_DWELL | 15U);

	zassert_false(ctx.mac.ul_dwell_time);
	zassert_false(ctx.mac.dl_dwell_time);
	zassert_equal(ctx.mac.max_eirp_dbm, 0);
	zassert_false(ctx.mac.tx_param_setup_ans_pending, "an unanswered command was answered");
}

ZTEST(lorawan_native_mac, test_answer_rides_the_next_uplink)
{
	uint8_t fopts[LWAN_MAX_FOPTS_LEN];
	size_t len;

	feed(TX_PARAM_UL_DWELL | 5U);
	zassert_equal(mac_cmd_next_ul_fopts_len(&ctx), 1);

	len = mac_cmd_build_ul_fopts(&ctx, fopts, sizeof(fopts));
	zassert_equal(len, 1);
	zassert_equal(fopts[0], CID_TX_PARAM_SETUP);
}

ZTEST(lorawan_native_mac, test_answer_survives_a_failed_uplink)
{
	uint8_t fopts[LWAN_MAX_FOPTS_LEN];

	feed(TX_PARAM_UL_DWELL | 5U);
	(void)mac_cmd_build_ul_fopts(&ctx, fopts, sizeof(fopts));

	/* No commit: the transmission failed, so the answer is still owed. */
	zassert_true(ctx.mac.tx_param_setup_ans_pending);
	zassert_equal(mac_cmd_next_ul_fopts_len(&ctx), 1);
}

/* TS001 5.8: the answer rides every uplink until a Class A downlink arrives. */
ZTEST(lorawan_native_mac, test_answer_repeats_until_a_downlink)
{
	uint8_t fopts[LWAN_MAX_FOPTS_LEN];

	feed(TX_PARAM_UL_DWELL | 5U);

	for (int i = 0; i < 3; i++) {
		zassert_equal(mac_cmd_build_ul_fopts(&ctx, fopts, sizeof(fopts)), 1,
			      "uplink %d", i);
		zassert_equal(fopts[0], CID_TX_PARAM_SETUP, "uplink %d", i);
		mac_cmd_commit_ul_fopts(&ctx);
		zassert_true(ctx.mac.tx_param_setup_ans_pending, "uplink %d", i);
		zassert_equal(mac_cmd_next_ul_fopts_len(&ctx), 1, "uplink %d", i);
	}

	mac_cmd_dl_received(&ctx);

	zassert_false(ctx.mac.tx_param_setup_ans_pending);
	zassert_equal(mac_cmd_next_ul_fopts_len(&ctx), 0);
	zassert_equal(mac_cmd_build_ul_fopts(&ctx, fopts, sizeof(fopts)), 0);
}

/*
 * The downlink that carries the request is not the one that acknowledges
 * the answer, so an answer is still owed after it.
 */
ZTEST(lorawan_native_mac, test_a_request_outlives_its_own_downlink)
{
	mac_cmd_dl_received(&ctx);
	feed(TX_PARAM_UL_DWELL | 5U);

	zassert_true(ctx.mac.tx_param_setup_ans_pending);
}

/*
 * TS001 5.8: the device uses the lower of the band's maximum EIRP and the
 * one the command named, and the TXPower index counts down from that, so a
 * lowered ceiling moves the whole ladder rather than trimming its bottom.
 */
ZTEST(lorawan_native_mac, test_max_eirp_moves_the_whole_power_ladder)
{
	struct lwan_dr_params p;
	int8_t power;

	ctx.region = &eirp_ops;

	/* Coded value 7 is 20 dBm, below the band's 30. */
	feed(7U);

	for (uint8_t idx = 0; idx <= 7; idx++) {
		zassert_ok(lwan_region_tx_params(&ctx, 0, idx, &p, &power), "index %u", idx);
		zassert_equal(power, 20 - 2 * idx, "index %u", idx);
	}
}

/* A ceiling at or above the band's own leaves the ladder alone. */
ZTEST(lorawan_native_mac, test_max_eirp_above_the_band_changes_nothing)
{
	struct lwan_dr_params p;
	int8_t power;

	ctx.region = &eirp_ops;

	/* Coded value 13 is 30 dBm, the band's own maximum. */
	feed(13U);

	zassert_ok(lwan_region_tx_params(&ctx, 0, 3, &p, &power));
	zassert_equal(power, EIRP_REGION_MAX_DBM - 6);
}

ZTEST(lorawan_native_mac, test_answer_goes_out_before_a_request)
{
	uint8_t fopts[LWAN_MAX_FOPTS_LEN];
	size_t len;

	ctx.mac.link_check_pending = true;
	feed(TX_PARAM_UL_DWELL | 5U);

	len = mac_cmd_build_ul_fopts(&ctx, fopts, sizeof(fopts));
	zassert_equal(len, 2);
	zassert_equal(fopts[0], CID_TX_PARAM_SETUP, "the answer did not go first");
}

ZTEST(lorawan_native_mac, test_min_datarate_follows_the_dwell_time)
{
	ctx.region = &floored_ops;

	zassert_equal(lwan_region_min_dr(&ctx), 0,
		      "no cap in force, DR0 carries a payload");

	ctx.mac.ul_dwell_time = true;
	zassert_equal(lwan_region_min_dr(&ctx), FLOORED_DWELL_MIN_DR,
		      "the cap leaves DR0 and DR1 without a payload");
}

ZTEST(lorawan_native_mac, test_min_datarate_of_an_uncapped_region)
{
	ctx.region = &uncapped_ops;
	ctx.mac.ul_dwell_time = true;

	/* No dwell lookup to route to, so the plain one answers. */
	zassert_equal(lwan_region_min_dr(&ctx), 0);
}

ZTEST(lorawan_native_mac, test_a_dwell_cap_raises_the_datarate)
{
	ctx.region = &floored_ops;
	ctx.current_dr = LORAWAN_DR_0;

	feed(TX_PARAM_UL_DWELL);

	zassert_equal(ctx.current_dr, FLOORED_DWELL_MIN_DR,
		      "a session on DR0 cannot transmit under the cap");
}

ZTEST(lorawan_native_mac, test_lifting_a_dwell_cap_leaves_the_datarate)
{
	ctx.region = &floored_ops;
	ctx.current_dr = LORAWAN_DR_5;

	feed(0);

	zassert_equal(ctx.current_dr, LORAWAN_DR_5,
		      "lifting the cap only widens the choice");
}

ZTEST_SUITE(lorawan_native_mac, NULL, NULL, before, NULL, NULL);
