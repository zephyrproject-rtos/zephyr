/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Register-level test for the NXP EVTG Zephyr driver: check the driver
 * programmed the devicetree configuration by reading the registers back.
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#include <fsl_evtg.h>

#define EVTG_NODE      DT_NODELABEL(evtg0)
#define EVTG_INST_NODE DT_CHILD(EVTG_NODE, instance_0)

BUILD_ASSERT(DT_NODE_HAS_STATUS(EVTG_NODE, okay),
	     "evtg0 devicetree node must be enabled for this test");
BUILD_ASSERT(DT_NODE_HAS_STATUS(EVTG_INST_NODE, okay),
	     "evtg0 instance@0 devicetree node must be enabled for this test");

#define TEST_EVTG_INDEX ((uint8_t)DT_REG_ADDR(EVTG_INST_NODE))

static EVTG_Type *const evtg = (EVTG_Type *)DT_REG_ADDR(EVTG_NODE);
static const struct device *const evtg_dev = DEVICE_DT_GET(EVTG_NODE);

/* Expected AOI0 product term 0 (inputs A, B, C, D), straight from devicetree. */
static const uint8_t aoi0_term0_expected[] = DT_PROP(EVTG_INST_NODE, aoi0_term0);

static uint16_t evtg_ctrl(void)
{
	return evtg->EVTG_INST[TEST_EVTG_INDEX].EVTG_CTRL;
}

static uint16_t evtg_aoi0_bft01(void)
{
	return evtg->EVTG_INST[TEST_EVTG_INDEX].EVTG_AOI0_BFT01;
}

ZTEST_SUITE(nxp_evtg, NULL, NULL, NULL, NULL, NULL);

/* The driver must have bound and initialized the EVTG device. */
ZTEST(nxp_evtg, test_device_ready)
{
	zassert_true(device_is_ready(evtg_dev), "EVTG device is not ready");
}

/* The driver must program the flip-flop mode selected in devicetree. */
ZTEST(nxp_evtg, test_flipflop_mode_from_dt)
{
	uint16_t mode = FIELD_GET(EVTG_EVTG_INST_EVTG_CTRL_MODE_SEL_MASK, evtg_ctrl());

	zassert_equal(mode, (uint16_t)DT_ENUM_IDX(EVTG_INST_NODE, flip_flop_mode),
		      "flip-flop mode not programmed from devicetree");
}

/* The driver must latch the flip-flop initial output value from devicetree. */
ZTEST(nxp_evtg, test_flipflop_init_output_from_dt)
{
	zassert_equal(FIELD_GET(EVTG_EVTG_INST_EVTG_CTRL_FF_INIT_MASK, evtg_ctrl()),
		      (uint16_t)DT_PROP(EVTG_INST_NODE, flip_flop_init_output),
		      "flip-flop init output not programmed from devicetree");
}

/* D-FF mode must keep all inputs synced (SYNC_CTRL == 0xF) with no DT override. */
ZTEST(nxp_evtg, test_input_sync_defaults_preserved)
{
	uint16_t sync = FIELD_GET(EVTG_EVTG_INST_EVTG_CTRL_SYNC_CTRL_MASK, evtg_ctrl());

	zassert_equal(sync, 0xFU, "input sync defaults not preserved for D-FF mode");
}

/* The driver must program the AOI0 product term 0 inputs from devicetree. */
ZTEST(nxp_evtg, test_aoi0_product_term_from_dt)
{
	uint16_t bft01 = evtg_aoi0_bft01();

	zassert_equal(FIELD_GET(EVTG_EVTG_INST_EVTG_AOI0_BFT01_PT0_AC_MASK, bft01),
		      aoi0_term0_expected[0], "AOI0 PT0 input A read-back mismatch");
	zassert_equal(FIELD_GET(EVTG_EVTG_INST_EVTG_AOI0_BFT01_PT0_BC_MASK, bft01),
		      aoi0_term0_expected[1], "AOI0 PT0 input B read-back mismatch");
	zassert_equal(FIELD_GET(EVTG_EVTG_INST_EVTG_AOI0_BFT01_PT0_CC_MASK, bft01),
		      aoi0_term0_expected[2], "AOI0 PT0 input C read-back mismatch");
	zassert_equal(FIELD_GET(EVTG_EVTG_INST_EVTG_AOI0_BFT01_PT0_DC_MASK, bft01),
		      aoi0_term0_expected[3], "AOI0 PT0 input D read-back mismatch");
}
