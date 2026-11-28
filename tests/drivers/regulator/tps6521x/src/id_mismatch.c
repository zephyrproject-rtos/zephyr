/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/ztest.h>

#include "emul_tps6521x.h"

#define PMIC_NODE DT_NODELABEL(pmic)

ZTEST(regulator_tps6521x_id_mismatch, test_other_variant_is_refused)
{
	struct emul_tps6521x_stats stats;

	zassert_false(device_is_ready(DEVICE_DT_GET(PMIC_NODE)), "MFD ready on the wrong variant");
	zassert_false(device_is_ready(DEVICE_DT_GET(DT_NODELABEL(buck1))), "BUCK1 ready");
	zassert_false(device_is_ready(DEVICE_DT_GET(DT_NODELABEL(buck2))), "BUCK2 ready");
	zassert_false(device_is_ready(DEVICE_DT_GET(DT_NODELABEL(buck3))), "BUCK3 ready");
	zassert_false(device_is_ready(DEVICE_DT_GET(DT_NODELABEL(ldo1))), "LDO1 ready");
	zassert_false(device_is_ready(DEVICE_DT_GET(DT_NODELABEL(ldo2))), "LDO2 ready");

	/* no lock sequence and no rail write reached the other variant's registers */
	emul_tps6521x_stats(EMUL_DT_GET(PMIC_NODE), &stats);
	zassert_equal(stats.writes, 0U, "the PMIC was written");
}

ZTEST_SUITE(regulator_tps6521x_id_mismatch, NULL, NULL, NULL, NULL, NULL);
