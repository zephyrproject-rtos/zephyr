/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/ztest.h>

#include <fsl_resetcon.h>

#define WARM_REBOOT_PENDING 0x5741524DU
#define COLD_REBOOT_PENDING 0x434F4C44U
#define SOFTWARE_COLD_RESET (kRESETCON_SrcResetSw | kRESETCON_SrcWarmEsca)

struct rt2660_reset_cause_fixture {
	uint32_t supported;
	uint32_t cause;
};

static void *reset_cause_setup(void)
{
	static struct rt2660_reset_cause_fixture fixture;
	RESETCON_Type *const resetcon = (RESETCON_Type *)DT_REG_ADDR(DT_NODELABEL(resetcon));
	const uint32_t pending_marker = IS_ENABLED(CONFIG_TEST_RESET_CAUSE_COLD)
						? COLD_REBOOT_PENDING
						: WARM_REBOOT_PENDING;
	const uint32_t sources = RESETCON_GetStickyResetStatus(resetcon);
	int32_t ret;

	ret = hwinfo_get_supported_reset_cause(&fixture.supported);
	zassert_ok(ret, "Cannot read supported reset causes: %d", ret);
	zassert_not_equal(fixture.supported & RESET_SOFTWARE, 0U);

	ret = hwinfo_get_reset_cause(&fixture.cause);
	zassert_ok(ret, "Cannot read reset cause: %d", ret);
	TC_PRINT("Initial reset cause: 0x%08x\n", fixture.cause);

	if (IS_ENABLED(CONFIG_TEST_RESET_CAUSE_COLD)) {
		/* A later pin reset can replace RSTSTAT; sticky sources retain the sequence. */
		if ((sources & (kRESETCON_SrcWarmEsca | kRESETCON_SrcRstErr)) != 0U) {
			zassert_equal(sources & SOFTWARE_COLD_RESET, SOFTWARE_COLD_RESET,
				      "Missing software reset escalation: sources=0x%08x", sources);
			return &fixture;
		}
		/* A retained pending marker means the requested cold reset failed. */
		if (SYSCON__POWERCON_SOC_CTRL->GPR_COLD[3] == COLD_REBOOT_PENDING) {
			return &fixture;
		}
	} else {
		zassert_equal(RESETCON_GetResetEscalation(resetcon) & kRESETCON_WarmResetSw, 0U,
			      "Warm reboot requires software reset escalation to be disabled");
		if (SYSCON__POWERCON_SOC_CTRL->GPR_COLD[3] == WARM_REBOOT_PENDING) {
			zassert_not_equal(sources & kRESETCON_SrcResetSw, 0U,
					  "Software reset source is missing: 0x%08x", sources);
			zassert_equal(sources & kRESETCON_SrcWarmEsca, 0U,
				      "Warm reboot escalated to cold reset");
			return &fixture;
		}
	}

	ret = hwinfo_clear_reset_cause();
	zassert_ok(ret, "Cannot clear reset cause before reboot: %d", ret);
	barrier_dsync_fence_full();
	{
		const uint32_t cleared = RESETCON_GetStickyResetStatus(resetcon);

		zassert_equal(cleared & kRESETCON_SrcAll, 0U,
			      "Reset sources remain set before reboot: 0x%08x", cleared);
	}
	SYSCON__POWERCON_SOC_CTRL->GPR_COLD[3] = pending_marker;
	barrier_dsync_fence_full();
	zassert_equal(SYSCON__POWERCON_SOC_CTRL->GPR_COLD[3], pending_marker,
		      "Cannot write reboot marker");
	TC_PRINT("Requesting %s reboot\n",
		 IS_ENABLED(CONFIG_TEST_RESET_CAUSE_COLD) ? "cold" : "warm");
	k_msleep(100);
	sys_reboot(IS_ENABLED(CONFIG_TEST_RESET_CAUSE_COLD) ? SYS_REBOOT_COLD : SYS_REBOOT_WARM);
}

ZTEST_F(rt2660_reset_cause, test_software_reboot)
{
	if (IS_ENABLED(CONFIG_TEST_RESET_CAUSE_COLD)) {
		RESETCON_Type *const resetcon =
			(RESETCON_Type *)DT_REG_ADDR(DT_NODELABEL(resetcon));

		zassert_equal(SYSCON__POWERCON_SOC_CTRL->GPR_COLD[3], 0U,
			      "Cold reboot did not clear GPR_COLD[3]");
		zassert_equal(fixture->cause & ~RESET_PIN, RESET_SOFTWARE | RESET_HARDWARE,
			      "Unexpected cold reboot cause: 0x%08x", fixture->cause);
		zassert_equal(RESETCON_GetResetEscalation(resetcon) & kRESETCON_WarmResetSw, 0U,
			      "Cold reboot did not clear software reset escalation");
	} else {
		zassert_equal(SYSCON__POWERCON_SOC_CTRL->GPR_COLD[3], WARM_REBOOT_PENDING,
			      "GPR_COLD[3] did not survive warm reboot");
		/* The EVK can also report a pin reset during the software reset sequence. */
		zassert_equal(fixture->cause & ~RESET_PIN, RESET_SOFTWARE,
			      "Unexpected warm reboot cause: 0x%08x", fixture->cause);
	}
}

ZTEST_F(rt2660_reset_cause, test_supported_reset_causes)
{
	uint32_t supported;
	int32_t ret;

	ret = hwinfo_get_supported_reset_cause(&supported);
	zassert_ok(ret);
	zassert_equal(supported, fixture->supported);
	zassert_not_equal(supported & RESET_SOFTWARE, 0U);
	zassert_not_equal(supported & RESET_HARDWARE, 0U);
	zassert_equal(fixture->cause & ~supported, 0U);
}

ZTEST(rt2660_reset_cause, test_get_reset_cause)
{
	uint32_t first;
	uint32_t repeated;
	int32_t ret;

	ret = hwinfo_get_reset_cause(&first);
	zassert_ok(ret);
	ret = hwinfo_get_reset_cause(&repeated);
	zassert_ok(ret);
	zassert_equal(first, repeated, "Reading reset causes changed them");
}

ZTEST(rt2660_reset_cause, test_clear_reset_cause)
{
	uint32_t cause;
	int32_t ret;

	/* Clearing an already empty set must also succeed. */
	for (uint32_t i = 0U; i < 2U; i++) {
		ret = hwinfo_clear_reset_cause();
		zassert_ok(ret);
		ret = hwinfo_get_reset_cause(&cause);
		zassert_ok(ret);
		zassert_equal(cause, 0U, "Reset causes were not cleared");
	}
}

static void reset_cause_teardown(void *data)
{
	ARG_UNUSED(data);
	SYSCON__POWERCON_SOC_CTRL->GPR_COLD[3] = 0U;
}

ZTEST_SUITE(rt2660_reset_cause, NULL, reset_cause_setup, NULL, NULL, reset_cause_teardown);
