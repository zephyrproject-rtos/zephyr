/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/ztest.h>

#include "emul_tps6521x.h"

#define PMIC_NODE   DT_NODELABEL(pmic)
#define IS_TPS65215 DT_NODE_HAS_COMPAT(PMIC_NODE, ti_tps65215)

#define REG_ENABLE_CTRL 0x02U
#define REG_BUCK3_VOUT  0x08U
#define REG_BUCK2_VOUT  0x09U
#define REG_BUCK1_VOUT  0x0AU
#define REG_LDO1_VOUT   (IS_TPS65215 ? 0x07U : 0x05U)
#define REG_LDO2_VOUT   (IS_TPS65215 ? 0x05U : 0x06U)
#define VSET(v)         ((v) & 0x3f)
#define BUCK2_VOUT_BOOT 0xC0U

static const struct emul *emul = EMUL_DT_GET(PMIC_NODE);
static const struct device *const buck1 = DEVICE_DT_GET(DT_NODELABEL(buck1));
static const struct device *const buck2 = DEVICE_DT_GET(DT_NODELABEL(buck2));
static const struct device *const buck3 = DEVICE_DT_GET(DT_NODELABEL(buck3));
static const struct device *const ldo1 = DEVICE_DT_GET(DT_NODELABEL(ldo1));
static const struct device *const ldo2 = DEVICE_DT_GET(DT_NODELABEL(ldo2));

static void check_locked(void)
{
	struct emul_tps6521x_stats stats;

	emul_tps6521x_stats(emul, &stats);
	zassert_equal(stats.blocked_writes, 0, "a protected write hit a locked register");
	if (IS_TPS65215) {
		zassert_equal(stats.reg3_writes, 0, "TPS65215 has no REG_LOCK");
	} else {
		zassert_true(emul_tps6521x_locked(emul), "registers left unlocked");
	}
}

ZTEST(regulator_tps6521x, test_boot_state)
{
	uint8_t en = emul_tps6521x_get(emul, REG_ENABLE_CTRL);

	zassert_true(device_is_ready(buck1));
	zassert_true(device_is_ready(buck2));
	zassert_true(device_is_ready(buck3));
	zassert_true(device_is_ready(ldo1));
	/* also on TPS65214, where it is a load switch with min/max in the DT */
	zassert_true(device_is_ready(ldo2));
	zassert_true(en & BIT(0), "always-on BUCK1 not enabled at boot");
	zassert_true(en & BIT(2), "boot-on BUCK3 not enabled at boot");
	zassert_false(en & BIT(1), "BUCK2 enabled without a request");
	/* 1.1 V = 0.6 V + 20 * 25 mV */
	zassert_equal(VSET(emul_tps6521x_get(emul, REG_BUCK3_VOUT)), 0x14);
	zassert_true(regulator_is_enabled(buck1));
	zassert_true(regulator_is_enabled(buck3));
	check_locked();
}

ZTEST(regulator_tps6521x, test_enable_disable)
{
	zassert_ok(regulator_enable(buck2));
	zassert_true(emul_tps6521x_get(emul, REG_ENABLE_CTRL) & BIT(1));
	zassert_true(regulator_is_enabled(buck2));
	zassert_ok(regulator_disable(buck2));
	zassert_false(emul_tps6521x_get(emul, REG_ENABLE_CTRL) & BIT(1));
	/* the other rails keep their state */
	zassert_true(emul_tps6521x_get(emul, REG_ENABLE_CTRL) & BIT(0));
	check_locked();
}

ZTEST(regulator_tps6521x, test_ldo_enable_bits)
{
	uint8_t bit = IS_TPS65215 ? BIT(3) : BIT(5);

	zassert_ok(regulator_enable(ldo1));
	zassert_equal(emul_tps6521x_get(emul, REG_ENABLE_CTRL) & (BIT(3) | BIT(4) | BIT(5)), bit);
	zassert_ok(regulator_disable(ldo1));
	zassert_false(emul_tps6521x_get(emul, REG_ENABLE_CTRL) & bit);
}

ZTEST(regulator_tps6521x, test_buck_voltage)
{
	int32_t uv;

	zassert_ok(regulator_set_voltage(buck2, 750000, 750000));
	zassert_equal(VSET(emul_tps6521x_get(emul, REG_BUCK2_VOUT)), 0x06);
	zassert_ok(regulator_get_voltage(buck2, &uv));
	zassert_equal(uv, 750000);

	zassert_ok(regulator_set_voltage(buck2, 1800000, 1800000));
	zassert_equal(VSET(emul_tps6521x_get(emul, REG_BUCK2_VOUT)), 0x24);
	zassert_ok(regulator_set_voltage(buck2, 3400000, 3400000));
	zassert_equal(VSET(emul_tps6521x_get(emul, REG_BUCK2_VOUT)), 0x34);

	/* bandwidth and UV threshold bits are kept */
	zassert_equal(emul_tps6521x_get(emul, REG_BUCK2_VOUT) & 0xc0, 0xc0);

	/* between 1.4 V and 1.5 V nothing is available */
	zassert_equal(regulator_set_voltage(buck2, 1450000, 1450000), -EINVAL);
	check_locked();
}

ZTEST(regulator_tps6521x, test_windows_across_range_edges)
{
	int32_t uv;

	/* spans both buck ranges: linear_range picks the end of the first */
	zassert_ok(regulator_set_voltage(buck2, 1300000, 3400000));
	zassert_ok(regulator_get_voltage(buck2, &uv));
	zassert_equal(uv, 1375000);

	/* nothing between 1.375 V and 1.4 V except the edges */
	zassert_ok(regulator_set_voltage(buck2, 1380000, 1450000));
	zassert_ok(regulator_get_voltage(buck2, &uv));
	zassert_equal(uv, 1400000);

	zassert_ok(regulator_set_voltage(buck2, 500000, 650000));
	zassert_ok(regulator_get_voltage(buck2, &uv));
	zassert_equal(uv, 600000);

	zassert_ok(regulator_set_voltage(ldo1, 500000, 700000));
	zassert_ok(regulator_get_voltage(ldo1, &uv));
	zassert_equal(uv, 600000);
	zassert_ok(regulator_set_voltage(ldo1, 3000000, 3400000));
	zassert_ok(regulator_get_voltage(ldo1, &uv));
	zassert_equal(uv, IS_TPS65215 ? 3000000 : 3300000);

	zassert_equal(regulator_set_voltage(buck2, 3450000, 3500000), -EINVAL);
	check_locked();
}

ZTEST(regulator_tps6521x, test_buck_readback_clamp)
{
	int32_t uv;

	emul_tps6521x_set(emul, REG_BUCK2_VOUT, 0x3f);
	zassert_ok(regulator_get_voltage(buck2, &uv));
	zassert_equal(uv, 3400000);
}

ZTEST(regulator_tps6521x, test_buck_list)
{
	int32_t uv;

	zassert_equal(regulator_count_voltages(buck2), 53);
	zassert_ok(regulator_list_voltage(buck2, 0, &uv));
	zassert_equal(uv, 600000);
	zassert_ok(regulator_list_voltage(buck2, 31, &uv));
	zassert_equal(uv, 1375000);
	zassert_ok(regulator_list_voltage(buck2, 32, &uv));
	zassert_equal(uv, 1400000);
	zassert_ok(regulator_list_voltage(buck2, 52, &uv));
	zassert_equal(uv, 3400000);
	zassert_true(regulator_is_supported_voltage(buck2, 1375000, 1375000));
	zassert_false(regulator_is_supported_voltage(buck2, 3500000, 3500000));
}

ZTEST(regulator_tps6521x, test_ldo1_voltage)
{
	int32_t uv;

	zassert_ok(regulator_set_voltage(ldo1, 1800000, 1800000));
	zassert_equal(VSET(emul_tps6521x_get(emul, REG_LDO1_VOUT)), IS_TPS65215 ? 0x18 : 0x1a);
	zassert_ok(regulator_get_voltage(ldo1, &uv));
	zassert_equal(uv, 1800000);

	emul_tps6521x_set(emul, REG_LDO1_VOUT, 0x00);
	zassert_ok(regulator_get_voltage(ldo1, &uv));
	zassert_equal(uv, 600000);

	emul_tps6521x_set(emul, REG_LDO1_VOUT, 0x3f);
	zassert_ok(regulator_get_voltage(ldo1, &uv));
	zassert_equal(uv, IS_TPS65215 ? 3400000 : 3300000);

	zassert_equal(regulator_count_voltages(ldo1), IS_TPS65215 ? 57 : 55);
	zassert_equal(regulator_set_voltage(ldo1, 3400000, 3400000), IS_TPS65215 ? 0 : -EINVAL);
	check_locked();
}

ZTEST(regulator_tps6521x, test_ldo2)
{
	int32_t uv;

	if (!IS_TPS65215) {
		/* configured as a load switch by the emulator NVM defaults */
		zassert_equal(regulator_count_voltages(ldo2), 0);
		zassert_equal(regulator_set_voltage(ldo2, 1800000, 1800000), -ENOSYS);
		zassert_equal(regulator_get_voltage(ldo2, &uv), -ENOSYS);
		zassert_ok(regulator_enable(ldo2));
		zassert_true(emul_tps6521x_get(emul, REG_ENABLE_CTRL) & BIT(4));
		zassert_ok(regulator_disable(ldo2));
		return;
	}

	zassert_ok(regulator_set_voltage(ldo2, 1200000, 1200000));
	zassert_equal(VSET(emul_tps6521x_get(emul, REG_LDO2_VOUT)), 0x0c);
	zassert_ok(regulator_set_voltage(ldo2, 3300000, 3300000));
	zassert_equal(VSET(emul_tps6521x_get(emul, REG_LDO2_VOUT)), 0x36);
	zassert_equal(regulator_set_voltage(ldo2, 1000000, 1000000), -EINVAL);
	emul_tps6521x_set(emul, REG_LDO2_VOUT, 0x03);
	zassert_ok(regulator_get_voltage(ldo2, &uv));
	zassert_equal(uv, 1200000);
	zassert_equal(regulator_count_voltages(ldo2), 43);
	check_locked();
}

ZTEST(regulator_tps6521x, test_no_write_when_unchanged)
{
	struct emul_tps6521x_stats before;
	struct emul_tps6521x_stats after;

	zassert_ok(regulator_set_voltage(buck2, 900000, 900000));
	emul_tps6521x_stats(emul, &before);
	zassert_ok(regulator_set_voltage(buck2, 900000, 900000));
	emul_tps6521x_stats(emul, &after);
	zassert_equal(after.writes, before.writes, "unchanged value was written");
}

ZTEST(regulator_tps6521x, test_write_error_relocks)
{
	struct emul_tps6521x_stats before;
	struct emul_tps6521x_stats after;

	emul_tps6521x_stats(emul, &before);
	emul_tps6521x_fail_writes_to(emul, REG_ENABLE_CTRL);
	zassert_equal(regulator_enable(buck2), -EIO);

	zassert_false(emul_tps6521x_get(emul, REG_ENABLE_CTRL) & BIT(1));
	zassert_false(regulator_is_enabled(buck2));
	emul_tps6521x_stats(emul, &after);
	/* TPS65214: unlock and lock again; TPS65215: nothing */
	zassert_equal(after.writes - before.writes, IS_TPS65215 ? 0 : 2);
	check_locked();
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	emul_tps6521x_fail_writes_to(emul, -1);
	emul_tps6521x_set(emul, REG_BUCK2_VOUT, BUCK2_VOUT_BOOT);
}

ZTEST_SUITE(regulator_tps6521x, NULL, NULL, NULL, after, NULL);
