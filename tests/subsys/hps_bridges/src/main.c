/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/fpga_bridge/bridge.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define SOC2FPGA_MASK	BIT(0)
#define LWSOC2FPGA_MASK BIT(1)
#define FPGA2SOC_MASK	BIT(2)
#define F2SDRAM0_MASK	BIT(3)

#define BRIDGE_ENABLE  1
#define BRIDGE_DISABLE 0

ZTEST(hps_bridges_stack, test_hps_bridge_disable_enable)
{
	int ret;

	/* No bridges or interfaces are enabled. LWSOC2FPGA, SOC2FPGA and FPGA2SOC are disabled.*/
	ret = do_bridge_reset(BRIDGE_DISABLE, LWSOC2FPGA_MASK | SOC2FPGA_MASK | FPGA2SOC_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* SOC2FPGA bridge is enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, SOC2FPGA_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* SOC2FPGA bridge is disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, SOC2FPGA_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* LWSOC2FPGA bridge is enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, LWSOC2FPGA_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* LWSOC2FPGA bridge is disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, LWSOC2FPGA_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the LWSOC2FPGA and SOC2FPGA bridges are enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, LWSOC2FPGA_MASK | SOC2FPGA_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the LWSOC2FPGA and SOC2FPGA bridges are disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, LWSOC2FPGA_MASK | SOC2FPGA_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* FPGA2SOC bridge is enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, FPGA2SOC_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* FPGA2SOC bridge is disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, FPGA2SOC_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the SOC2FPGA and FPGA2SOC bridges are enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, SOC2FPGA_MASK | FPGA2SOC_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the SOC2FPGA and FPGA2SOC bridges are disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, SOC2FPGA_MASK | FPGA2SOC_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the LWSOC2FPGA and FPGA2SOC bridges are enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, LWSOC2FPGA_MASK | FPGA2SOC_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the LWSOC2FPGA and FPGA2SOC bridges are disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, LWSOC2FPGA_MASK | FPGA2SOC_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* The LWSOC2FPGA, SOC2FPGA, and FPGA2SOC bridges are enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, LWSOC2FPGA_MASK | SOC2FPGA_MASK | FPGA2SOC_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* The LWSOC2FPGA, SOC2FPGA, and FPGA2SOC bridges are disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, LWSOC2FPGA_MASK | SOC2FPGA_MASK | FPGA2SOC_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

#ifdef CONFIG_SOC_AGILEX5

	/* FPGA2SDRAM interface is enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* FPGA2SDRAM interface is disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the SOC2FPGA bridge and FPGA2SDRAM interface are enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, SOC2FPGA_MASK | F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the SOC2FPGA bridge and FPGA2SDRAM interface are disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, SOC2FPGA_MASK | F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the LWSOC2FPGA bridge and FPGA2SDRAM interface are enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, LWSOC2FPGA_MASK | F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the LWSOC2FPGA bridge and FPGA2SDRAM interface are disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, LWSOC2FPGA_MASK | F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* The LWSOC2FPGA, SOC2FPGA, and FPGA2SDRAM are enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, SOC2FPGA_MASK | LWSOC2FPGA_MASK | F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* The LWSOC2FPGA, SOC2FPGA, and FPGA2SDRAM are disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, SOC2FPGA_MASK | LWSOC2FPGA_MASK | F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the FPGA2SOC bridge and FPGA2SDRAM interface are enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, FPGA2SOC_MASK | F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the FPGA2SOC bridge and FPGA2SDRAM interface are disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, FPGA2SOC_MASK | F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the SOC2FPGA, FPGA2SOC bridge and FPGA2SDRAM interface are enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, SOC2FPGA_MASK | FPGA2SOC_MASK | F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* Both the SOC2FPGA, FPGA2SOC bridge and FPGA2SDRAM interface are disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, SOC2FPGA_MASK | FPGA2SOC_MASK | F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* The LWSOC2FPGA, FPGA2SOC, and FPGA2SDRAM are enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE, LWSOC2FPGA_MASK | FPGA2SOC_MASK | F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* The LWSOC2FPGA, FPGA2SOC, and FPGA2SDRAM are disabled */
	ret = do_bridge_reset(BRIDGE_DISABLE, LWSOC2FPGA_MASK | FPGA2SOC_MASK | F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

	/* The LWSOC2FPGA, SOC2FPGA, FPGA2SOC, and FPGA2SDRAM are enabled */
	ret = do_bridge_reset(BRIDGE_ENABLE,
			      SOC2FPGA_MASK | LWSOC2FPGA_MASK | FPGA2SOC_MASK | F2SDRAM0_MASK);
	zassert_equal(ret, 0, "Bridge reset failed");

#endif

	/* Sending Incorrect Bridge Mask Value */
	ret = do_bridge_reset(0, -9);
	zassert_equal(ret, -ENOTSUP, "Please provide correct mask");

	/* Sending Incorrect Bridge Mask Value */
	ret = do_bridge_reset(0, 'A');
	zassert_equal(ret, -ENOTSUP, "Please provide correct mask");

	/* Sending Incorrect Bridge Mask Value */
	ret = do_bridge_reset(1, -9);
	zassert_equal(ret, -ENOTSUP, "Please provide correct mask");

	/* Sending Incorrect Bridge Mask Value */
	ret = do_bridge_reset(1, 'A');
	zassert_equal(ret, -ENOTSUP, "Please provide correct mask");
}

ZTEST_SUITE(hps_bridges_stack, NULL, NULL, NULL, NULL, NULL);
