/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/regulator/fake.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

static const struct device *const parent = DEVICE_DT_GET(DT_NODELABEL(regulator));
/* REG0: no Devicetree properties */
static const struct device *const reg0 = DEVICE_DT_GET(DT_NODELABEL(reg0));
/* REG1: regulator-always-on */
static const struct device *const reg1 = DEVICE_DT_GET(DT_NODELABEL(reg1));
/* REG2: regulator-boot-on */
static const struct device *const reg2 = DEVICE_DT_GET(DT_NODELABEL(reg2));
/* REG3: regulator-max/min-microvolt/microamp, regulator-allowed-modes */
static const struct device *const reg3 = DEVICE_DT_GET(DT_NODELABEL(reg3));
/* REG4: regulator-boot-off */
static const struct device *const reg4 = DEVICE_DT_GET(DT_NODELABEL(reg4));
/* REG5: regulator-boot-off and is_enabled */
static const struct device *const reg5 = DEVICE_DT_GET(DT_NODELABEL(reg5));

static DEVICE_API(regulator, dummy_regulator_api);
static DEVICE_API(regulator_parent, dummy_regulator_parent_api);

DEVICE_DEFINE(dummy_regulator, "dummy_regulator", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &dummy_regulator_api);
DEVICE_DEFINE(dummy_regulator_parent, "dummy_regulator_parent", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &dummy_regulator_parent_api);

/* Dummy regulator: empty API */
static const struct device *const dummy_reg = DEVICE_GET(dummy_regulator);
/* Dummy parent regulator: empty API */
static const struct device *const dummy_parent = DEVICE_GET(dummy_regulator_parent);

ZTEST(regulator_api, test_parent_dvs_state_set_not_implemented)
{
	int ret = regulator_parent_dvs_state_set(dummy_parent, 0);

	zassert_equal(ret, -ENOSYS);
}

ZTEST(regulator_api, test_parent_dvs_state_set_ok)
{
	RESET_FAKE(regulator_parent_fake_dvs_state_set);

	regulator_parent_fake_dvs_state_set_fake.return_val = 0;

	zassert_equal(regulator_parent_dvs_state_set(parent, 0U), 0);
	zassert_equal(regulator_parent_fake_dvs_state_set_fake.arg0_val, parent);
	zassert_equal(regulator_parent_fake_dvs_state_set_fake.arg1_val, 0U);
	zassert_equal(regulator_parent_fake_dvs_state_set_fake.call_count, 1U);
}

ZTEST(regulator_api, test_parent_dvs_state_set_fail)
{
	RESET_FAKE(regulator_parent_fake_dvs_state_set);

	regulator_parent_fake_dvs_state_set_fake.return_val = -ENOTSUP;

	zassert_equal(regulator_parent_dvs_state_set(parent, 0U), -ENOTSUP);
	zassert_equal(regulator_parent_fake_dvs_state_set_fake.arg0_val, parent);
	zassert_equal(regulator_parent_fake_dvs_state_set_fake.arg1_val, 0U);
	zassert_equal(regulator_parent_fake_dvs_state_set_fake.call_count, 1U);
}

ZTEST(regulator_api, test_parent_ship_mode_not_implemented)
{
	int ret = regulator_parent_ship_mode(dummy_parent);

	zassert_equal(ret, -ENOSYS);
}

ZTEST(regulator_api, test_parent_ship_mode_ok)
{
	RESET_FAKE(regulator_parent_fake_ship_mode);

	regulator_parent_fake_ship_mode_fake.return_val = 0;

	zassert_equal(regulator_parent_ship_mode(parent), 0);
	zassert_equal(regulator_parent_fake_ship_mode_fake.arg0_val, parent);
	zassert_equal(regulator_parent_fake_ship_mode_fake.call_count, 1U);
}

ZTEST(regulator_api, test_parent_ship_mode_fail)
{
	RESET_FAKE(regulator_parent_fake_ship_mode);

	regulator_parent_fake_ship_mode_fake.return_val = -ENOTSUP;

	zassert_equal(regulator_parent_ship_mode(parent), -ENOTSUP);
	zassert_equal(regulator_parent_fake_ship_mode_fake.arg0_val, parent);
	zassert_equal(regulator_parent_fake_ship_mode_fake.call_count, 1U);
}

ZTEST(regulator_api, test_common_config)
{
	const struct regulator_common_config *config;

	/* reg0: all defaults */
	config = reg0->config;
	zassert_equal(config->min_uv, INT32_MIN);
	zassert_equal(config->max_uv, INT32_MAX);
	zassert_equal(config->min_ua, INT32_MIN);
	zassert_equal(config->max_ua, INT32_MAX);
	zassert_equal(config->allowed_modes_cnt, 0U);
	zassert_equal(config->initial_mode, REGULATOR_INITIAL_MODE_UNKNOWN);
	zassert_equal(REGULATOR_ACTIVE_DISCHARGE_GET_BITS(config->flags),
		      REGULATOR_ACTIVE_DISCHARGE_DEFAULT);

	/* reg1: regulator-always-on */
	config = reg1->config;
	zassert_equal(config->flags & REGULATOR_ALWAYS_ON, REGULATOR_ALWAYS_ON);

	/* reg2: regulator-boot-on */
	config = reg2->config;
	zassert_equal(config->flags & REGULATOR_BOOT_ON, REGULATOR_BOOT_ON);

	/* reg3: regulator-min/max-microvolt/microamp */
	config = reg3->config;
	zassert_equal(config->min_uv, 100);
	zassert_equal(config->max_uv, 200);
	zassert_equal(config->min_ua, 100);
	zassert_equal(config->max_ua, 200);
	zassert_equal(config->allowed_modes[0], 1U);
	zassert_equal(config->allowed_modes[1], 10U);
	zassert_equal(config->allowed_modes_cnt, 2U);
	zassert_equal(REGULATOR_ACTIVE_DISCHARGE_GET_BITS(config->flags), 1U);

	/* reg4: regulator-boot-off */
	config = reg4->config;
	zassert_equal(config->flags & REGULATOR_BOOT_OFF, REGULATOR_BOOT_OFF);
}

ZTEST(regulator_api, test_common_is_init_enabled)
{
	zassert_false(regulator_common_is_init_enabled(reg0));
	zassert_true(regulator_common_is_init_enabled(reg1));
	zassert_true(regulator_common_is_init_enabled(reg2));
	zassert_false(regulator_common_is_init_enabled(reg3));
	zassert_false(regulator_common_is_init_enabled(reg4));
	zassert_false(regulator_common_is_init_enabled(reg5));
}

ZTEST(regulator_api, test_enable_disable)
{
	RESET_FAKE(regulator_fake_enable);
	RESET_FAKE(regulator_fake_disable);

	/* REG1 already enabled, not enabled again */
	zassert_equal(regulator_enable(reg1), 0);
	zassert_equal(regulator_fake_enable_fake.call_count, 0U);

	/* REG1: can't be disabled */
	zassert_equal(regulator_disable(reg1), 0);
	zassert_equal(regulator_fake_disable_fake.call_count, 0U);

	/* REG2: can be disabled */
	zassert_equal(regulator_disable(reg2), 0);
	zassert_equal(regulator_fake_disable_fake.arg0_val, reg2);
	zassert_equal(regulator_fake_disable_fake.call_count, 1U);

	/* REG2: enable again */
	zassert_equal(regulator_enable(reg2), 0);
	zassert_equal(regulator_fake_enable_fake.arg0_val, reg2);
	zassert_equal(regulator_fake_enable_fake.call_count, 1U);

	/* REG0: enable */
	zassert_equal(regulator_enable(reg0), 0);
	zassert_equal(regulator_fake_enable_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_enable_fake.call_count, 2U);

	/* REG0: disable */
	zassert_equal(regulator_disable(reg0), 0);
	zassert_equal(regulator_fake_disable_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_disable_fake.call_count, 2U);

	/* REG5: disabled at boot, can be enabled again */
	zassert_equal(regulator_enable(reg5), 0);
	zassert_equal(regulator_fake_enable_fake.call_count, 3U);

	/* REG5: disable */
	zassert_equal(regulator_disable(reg5), 0);
	zassert_equal(regulator_fake_disable_fake.call_count, 3U);
}

ZTEST(regulator_api, test_count_voltages_not_implemented)
{
	unsigned int count = regulator_count_voltages(dummy_reg);

	zassert_equal(count, 0U);
}

ZTEST(regulator_api, test_count_voltages)
{
	RESET_FAKE(regulator_fake_count_voltages);

	regulator_fake_count_voltages_fake.return_val = 10U;

	zassert_equal(regulator_count_voltages(reg0), 10U);
	zassert_equal(regulator_fake_count_voltages_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_count_voltages_fake.call_count, 1U);
}

ZTEST(regulator_api, test_list_voltage_not_implemented)
{
	int ret = regulator_list_voltage(dummy_reg, 0, NULL);

	zassert_equal(ret, -EINVAL);
}

static int list_voltage_ok(const struct device *dev, unsigned int idx, int32_t *volt_uv)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(idx);

	*volt_uv = 100;

	return 0;
}

ZTEST(regulator_api, test_list_voltage_valid)
{
	int32_t volt_uv;

	RESET_FAKE(regulator_fake_list_voltage);

	regulator_fake_list_voltage_fake.custom_fake = list_voltage_ok;

	zassert_equal(regulator_list_voltage(reg0, 0, &volt_uv), 0);
	zassert_equal(volt_uv, 100);
	zassert_equal(regulator_fake_list_voltage_fake.call_count, 1U);
	zassert_equal(regulator_fake_list_voltage_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_list_voltage_fake.arg1_val, 0);
	zassert_equal(regulator_fake_list_voltage_fake.arg2_val, &volt_uv);
}

static int list_voltage_invalid(const struct device *dev, unsigned int idx, int32_t *volt_uv)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(idx);
	ARG_UNUSED(volt_uv);

	return -EINVAL;
}

ZTEST(regulator_api, test_list_voltage_invalid)
{
	RESET_FAKE(regulator_fake_list_voltage);

	regulator_fake_list_voltage_fake.custom_fake = list_voltage_invalid;

	zassert_equal(regulator_list_voltage(reg0, 0, NULL), -EINVAL);
	zassert_equal(regulator_fake_list_voltage_fake.call_count, 1U);
	zassert_equal(regulator_fake_list_voltage_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_list_voltage_fake.arg1_val, 0);
	zassert_equal(regulator_fake_list_voltage_fake.arg2_val, NULL);
}

static int list_voltage(const struct device *dev, unsigned int idx, int32_t *volt_uv)
{
	ARG_UNUSED(dev);

	switch (idx) {
	case 0U:
		*volt_uv = 100;
		break;
	case 1U:
		*volt_uv = 200;
		break;
	case 2U:
		*volt_uv = 300;
		break;
	case 3U:
		*volt_uv = 400;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

ZTEST(regulator_api, test_is_supported_voltage)
{
	RESET_FAKE(regulator_fake_count_voltages);
	RESET_FAKE(regulator_fake_list_voltage);

	regulator_fake_count_voltages_fake.return_val = 4U;
	regulator_fake_list_voltage_fake.custom_fake = list_voltage;

	zassert_false(regulator_is_supported_voltage(reg0, 0, 50));
	zassert_true(regulator_is_supported_voltage(reg0, 50, 100));
	zassert_true(regulator_is_supported_voltage(reg0, 100, 200));
	zassert_true(regulator_is_supported_voltage(reg0, 150, 200));
	zassert_true(regulator_is_supported_voltage(reg0, 200, 300));
	zassert_true(regulator_is_supported_voltage(reg0, 300, 400));
	zassert_true(regulator_is_supported_voltage(reg0, 400, 500));
	zassert_false(regulator_is_supported_voltage(reg0, 500, 600));

	zassert_not_equal(regulator_fake_count_voltages_fake.call_count, 0U);
	zassert_not_equal(regulator_fake_list_voltage_fake.call_count, 0U);
}

ZTEST(regulator_api, test_is_supported_voltage_dt_limit)
{
	RESET_FAKE(regulator_fake_count_voltages);
	RESET_FAKE(regulator_fake_list_voltage);

	regulator_fake_count_voltages_fake.return_val = 4U;
	regulator_fake_list_voltage_fake.custom_fake = list_voltage;

	zassert_false(regulator_is_supported_voltage(reg3, 0, 50));
	zassert_true(regulator_is_supported_voltage(reg3, 50, 100));
	zassert_true(regulator_is_supported_voltage(reg3, 100, 200));
	zassert_true(regulator_is_supported_voltage(reg3, 150, 200));
	zassert_true(regulator_is_supported_voltage(reg3, 200, 300));
	zassert_false(regulator_is_supported_voltage(reg3, 300, 400));
	zassert_false(regulator_is_supported_voltage(reg3, 400, 500));
	zassert_false(regulator_is_supported_voltage(reg3, 500, 600));

	zassert_not_equal(regulator_fake_count_voltages_fake.call_count, 0U);
	zassert_not_equal(regulator_fake_list_voltage_fake.call_count, 0U);
}

ZTEST(regulator_api, test_set_voltage_not_implemented)
{
	int ret = regulator_set_voltage(dummy_reg, 0, 0);

	zassert_equal(ret, -ENOSYS);
}

ZTEST(regulator_api, test_set_voltage_ok)
{
	RESET_FAKE(regulator_fake_set_voltage);

	regulator_fake_set_voltage_fake.return_val = 0;

	zassert_equal(regulator_set_voltage(reg0, 0, 0), 0);
	zassert_equal(regulator_fake_set_voltage_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_set_voltage_fake.arg1_val, 0);
	zassert_equal(regulator_fake_set_voltage_fake.arg2_val, 0);
	zassert_equal(regulator_fake_set_voltage_fake.call_count, 1U);
}

ZTEST(regulator_api, test_set_voltage_fail)
{
	RESET_FAKE(regulator_fake_set_voltage);

	regulator_fake_set_voltage_fake.return_val = -EINVAL;

	zassert_equal(regulator_set_voltage(reg0, 0, 0), -EINVAL);
	zassert_equal(regulator_fake_set_voltage_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_set_voltage_fake.arg1_val, 0);
	zassert_equal(regulator_fake_set_voltage_fake.arg2_val, 0);
	zassert_equal(regulator_fake_set_voltage_fake.call_count, 1U);
}

ZTEST(regulator_api, test_set_voltage_dt_limit)
{
	RESET_FAKE(regulator_fake_set_voltage);

	regulator_fake_set_voltage_fake.return_val = 0;

	zassert_equal(regulator_set_voltage(reg3, 300, 400), -EINVAL);
	zassert_equal(regulator_fake_set_voltage_fake.call_count, 0U);
}

ZTEST(regulator_api, test_get_voltage_not_implemented)
{
	int ret = regulator_get_voltage(dummy_reg, NULL);

	zassert_equal(ret, -ENOSYS);
}

static int get_voltage_ok(const struct device *dev, int32_t *volt_uv)
{
	ARG_UNUSED(dev);

	*volt_uv = 100;

	return 0;
}

ZTEST(regulator_api, test_get_voltage_ok)
{
	int32_t volt_uv;

	RESET_FAKE(regulator_fake_get_voltage);

	regulator_fake_get_voltage_fake.custom_fake = get_voltage_ok;

	zassert_equal(regulator_get_voltage(reg0, &volt_uv), 0);
	zassert_equal(volt_uv, 100);
	zassert_equal(regulator_fake_get_voltage_fake.call_count, 1U);
	zassert_equal(regulator_fake_get_voltage_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_get_voltage_fake.arg1_val, &volt_uv);
}

static int get_voltage_fail(const struct device *dev, int32_t *volt_uv)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(volt_uv);

	return -EIO;
}

ZTEST(regulator_api, test_get_voltage_error)
{
	RESET_FAKE(regulator_fake_get_voltage);

	regulator_fake_get_voltage_fake.custom_fake = get_voltage_fail;

	zassert_equal(regulator_get_voltage(reg0, NULL), -EIO);
	zassert_equal(regulator_fake_get_voltage_fake.call_count, 1U);
	zassert_equal(regulator_fake_get_voltage_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_get_voltage_fake.arg1_val, NULL);
}

ZTEST(regulator_api, test_count_current_limits_not_implemented)
{
	unsigned int ret = regulator_count_current_limits(dummy_reg);

	zassert_equal(ret, 0);
}

ZTEST(regulator_api, test_count_current_limits_ok)
{
	RESET_FAKE(regulator_fake_count_current_limits);

	regulator_fake_count_current_limits_fake.return_val = 10;

	zassert_equal(regulator_count_current_limits(reg0), 10U);
	zassert_equal(regulator_fake_count_current_limits_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_count_current_limits_fake.call_count, 1U);
}

ZTEST(regulator_api, test_count_current_limits_fail)
{
	RESET_FAKE(regulator_fake_count_current_limits);

	regulator_fake_count_current_limits_fake.return_val = -EINVAL;

	zassert_equal(regulator_count_current_limits(reg0), -EINVAL);
	zassert_equal(regulator_fake_count_current_limits_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_count_current_limits_fake.call_count, 1U);
}

ZTEST(regulator_api, test_list_current_limit_not_implemented)
{
	zassert_equal(regulator_list_current_limit(dummy_reg, 0, NULL), -EINVAL);
}

static int list_current_limit_ok(const struct device *dev, unsigned int idx, int32_t *curr_ua)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(idx);

	*curr_ua = 100;

	return 0;
}

ZTEST(regulator_api, test_list_current_limit_ok)
{
	RESET_FAKE(regulator_fake_list_current_limit);

	int32_t curr_ua;

	regulator_fake_list_current_limit_fake.custom_fake = list_current_limit_ok;

	zassert_equal(regulator_list_current_limit(reg0, 1, &curr_ua), 0);
	zassert_equal(curr_ua, 100);
	zassert_equal(regulator_fake_list_current_limit_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_list_current_limit_fake.arg1_val, 1U);
	zassert_equal(regulator_fake_list_current_limit_fake.arg2_val, &curr_ua);
	zassert_equal(regulator_fake_list_current_limit_fake.call_count, 1U);
}

ZTEST(regulator_api, test_list_current_limit_fail)
{
	RESET_FAKE(regulator_fake_list_current_limit);

	int32_t curr_ua;

	regulator_fake_list_current_limit_fake.return_val = -EIO;

	zassert_equal(regulator_list_current_limit(reg0, 1, &curr_ua), -EIO);
	zassert_equal(regulator_fake_list_current_limit_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_list_current_limit_fake.arg1_val, 1U);
	zassert_equal(regulator_fake_list_current_limit_fake.arg2_val, &curr_ua);
	zassert_equal(regulator_fake_list_current_limit_fake.call_count, 1U);
}

ZTEST(regulator_api, test_set_current_limit_not_implemented)
{
	int ret = regulator_set_current_limit(dummy_reg, 0, 0);

	zassert_equal(ret, -ENOSYS);
}

ZTEST(regulator_api, test_set_current_limit_ok)
{
	RESET_FAKE(regulator_fake_set_current_limit);

	regulator_fake_set_current_limit_fake.return_val = 0;

	zassert_equal(regulator_set_current_limit(reg0, 0, 0), 0);
	zassert_equal(regulator_fake_set_current_limit_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_set_current_limit_fake.arg1_val, 0);
	zassert_equal(regulator_fake_set_current_limit_fake.arg2_val, 0);
	zassert_equal(regulator_fake_set_current_limit_fake.call_count, 1U);
}

ZTEST(regulator_api, test_set_current_limit_fail)
{
	RESET_FAKE(regulator_fake_set_current_limit);

	regulator_fake_set_current_limit_fake.return_val = -EINVAL;

	zassert_equal(regulator_set_current_limit(reg0, 0, 0), -EINVAL);
	zassert_equal(regulator_fake_set_current_limit_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_set_current_limit_fake.arg1_val, 0);
	zassert_equal(regulator_fake_set_current_limit_fake.arg2_val, 0);
	zassert_equal(regulator_fake_set_current_limit_fake.call_count, 1U);
}

ZTEST(regulator_api, test_set_current_limit_dt_limit)
{
	RESET_FAKE(regulator_fake_set_current_limit);

	regulator_fake_set_current_limit_fake.return_val = 0;

	zassert_equal(regulator_set_current_limit(reg3, 300, 400), -EINVAL);
	zassert_equal(regulator_fake_set_current_limit_fake.call_count, 0U);
}

ZTEST(regulator_api, test_get_current_limit_not_implemented)
{
	int ret = regulator_get_current_limit(dummy_reg, NULL);

	zassert_equal(ret, -ENOSYS);
}

static int get_current_limit_ok(const struct device *dev, int32_t *curr_ua)
{
	ARG_UNUSED(dev);

	*curr_ua = 100;

	return 0;
}

ZTEST(regulator_api, test_get_current_limit_ok)
{
	int32_t curr_ua;

	RESET_FAKE(regulator_fake_get_current_limit);

	regulator_fake_get_current_limit_fake.custom_fake = get_current_limit_ok;

	zassert_equal(regulator_get_current_limit(reg0, &curr_ua), 0);
	zassert_equal(curr_ua, 100);
	zassert_equal(regulator_fake_get_current_limit_fake.call_count, 1U);
	zassert_equal(regulator_fake_get_current_limit_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_get_current_limit_fake.arg1_val, &curr_ua);
}

static int get_current_limit_fail(const struct device *dev, int32_t *curr_ua)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(curr_ua);

	return -EIO;
}

ZTEST(regulator_api, test_get_current_limit_error)
{
	RESET_FAKE(regulator_fake_get_current_limit);

	regulator_fake_get_current_limit_fake.custom_fake = get_current_limit_fail;

	zassert_equal(regulator_get_current_limit(reg0, NULL), -EIO);
	zassert_equal(regulator_fake_get_current_limit_fake.call_count, 1U);
	zassert_equal(regulator_fake_get_current_limit_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_get_current_limit_fake.arg1_val, NULL);
}

ZTEST(regulator_api, test_set_mode_not_implemented)
{
	int ret = regulator_set_mode(dummy_reg, 0);

	zassert_equal(ret, -ENOSYS);
}

ZTEST(regulator_api, test_set_mode_ok)
{
	RESET_FAKE(regulator_fake_set_mode);

	regulator_fake_set_mode_fake.return_val = 0;

	zassert_equal(regulator_set_mode(reg0, 0), 0);
	zassert_equal(regulator_set_mode(reg0, 1), 0);
	zassert_equal(regulator_set_mode(reg0, 10), 0);
	zassert_equal(regulator_fake_set_mode_fake.call_count, 3U);
}

ZTEST(regulator_api, test_set_mode_fail)
{
	RESET_FAKE(regulator_fake_set_mode);

	regulator_fake_set_mode_fake.return_val = -ENOTSUP;

	zassert_equal(regulator_set_mode(reg0, 0), -ENOTSUP);
	zassert_equal(regulator_fake_set_mode_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_set_mode_fake.arg1_val, 0);
	zassert_equal(regulator_fake_set_mode_fake.call_count, 1U);
}

ZTEST(regulator_api, test_set_mode_dt_limit)
{
	RESET_FAKE(regulator_fake_set_mode);

	regulator_fake_set_mode_fake.return_val = 0;

	zassert_equal(regulator_set_mode(reg3, 0), -ENOTSUP);
	zassert_equal(regulator_set_mode(reg3, 1), 0);
	zassert_equal(regulator_set_mode(reg3, 10), 0);
	zassert_equal(regulator_fake_set_mode_fake.call_count, 2U);
}

ZTEST(regulator_api, test_get_mode_not_implemented)
{
	int ret = regulator_get_mode(dummy_reg, NULL);

	zassert_equal(ret, -ENOSYS);
}

ZTEST(regulator_api, test_set_active_discharge_not_implemented)
{
	int ret = regulator_set_active_discharge(dummy_reg, false);

	zassert_equal(ret, -ENOSYS);
}

static int get_active_discharge_ok(const struct device *dev, bool *active_discharge)
{
	ARG_UNUSED(dev);

	*active_discharge = true;

	return 0;
}

ZTEST(regulator_api, test_get_active_discharge_ok)
{
	bool active_discharge;

	RESET_FAKE(regulator_fake_get_active_discharge);

	regulator_fake_get_active_discharge_fake.custom_fake = get_active_discharge_ok;

	zassert_equal(regulator_get_active_discharge(reg0, &active_discharge), false);
	zassert_equal(active_discharge, true);
	zassert_equal(regulator_fake_get_active_discharge_fake.call_count, 1U);
	zassert_equal(regulator_fake_get_active_discharge_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_get_active_discharge_fake.arg1_val, &active_discharge);
}

ZTEST(regulator_api, test_get_active_discharge_not_implemented)
{
	int ret = regulator_get_active_discharge(dummy_reg, NULL);

	zassert_equal(ret, -ENOSYS);
}

static int get_mode_ok(const struct device *dev, regulator_mode_t *mode)
{
	ARG_UNUSED(dev);

	*mode = 10U;

	return 0;
}

ZTEST(regulator_api, test_get_mode_ok)
{
	regulator_mode_t mode;

	RESET_FAKE(regulator_fake_get_mode);

	regulator_fake_get_mode_fake.custom_fake = get_mode_ok;

	zassert_equal(regulator_get_mode(reg0, &mode), 0U);
	zassert_equal(mode, 10U);
	zassert_equal(regulator_fake_get_mode_fake.call_count, 1U);
	zassert_equal(regulator_fake_get_mode_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_get_mode_fake.arg1_val, &mode);
}

static int get_mode_fail(const struct device *dev, regulator_mode_t *mode)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mode);

	return -EIO;
}

ZTEST(regulator_api, test_get_mode_error)
{
	RESET_FAKE(regulator_fake_get_mode);

	regulator_fake_get_mode_fake.custom_fake = get_mode_fail;

	zassert_equal(regulator_get_mode(reg0, NULL), -EIO);
	zassert_equal(regulator_fake_get_mode_fake.call_count, 1U);
	zassert_equal(regulator_fake_get_mode_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_get_mode_fake.arg1_val, NULL);
}

ZTEST(regulator_api, test_get_error_flags_not_implemented)
{
	int ret = regulator_get_error_flags(dummy_reg, NULL);

	zassert_equal(ret, -ENOSYS);
}

static int get_error_flags_ok(const struct device *dev, regulator_error_flags_t *flags)
{
	ARG_UNUSED(dev);

	*flags = REGULATOR_ERROR_OVER_CURRENT;

	return 0;
}

ZTEST(regulator_api, test_get_error_flags_ok)
{
	regulator_error_flags_t flags;

	RESET_FAKE(regulator_fake_get_error_flags);

	regulator_fake_get_error_flags_fake.custom_fake = get_error_flags_ok;

	zassert_equal(regulator_get_error_flags(reg0, &flags), 0);
	zassert_equal(flags, REGULATOR_ERROR_OVER_CURRENT);
	zassert_equal(regulator_fake_get_error_flags_fake.call_count, 1U);
	zassert_equal(regulator_fake_get_error_flags_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_get_error_flags_fake.arg1_val, &flags);
}

static int get_error_flags_fail(const struct device *dev, regulator_error_flags_t *flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(flags);

	return -EIO;
}

ZTEST(regulator_api, test_get_max_voltage)
{
	int32_t max_uv = 0;
	int err = 0;

	err = regulator_common_get_max_voltage(reg0, &max_uv);
	zassert_equal(err, -ENOENT);

	err = regulator_common_get_max_voltage(reg3, &max_uv);
	zassert_equal(err, 0);
	zassert_equal(max_uv, 200);
}

ZTEST(regulator_api, test_get_min_voltage)
{
	int32_t min_uv = 0;
	int err = 0;

	err = regulator_common_get_min_voltage(reg0, &min_uv);
	zassert_equal(err, -ENOENT);

	err = regulator_common_get_min_voltage(reg3, &min_uv);
	zassert_equal(err, 0);
	zassert_equal(min_uv, 100);
}

ZTEST(regulator_api, test_get_error_flags_error)
{
	RESET_FAKE(regulator_fake_get_error_flags);

	regulator_fake_get_error_flags_fake.custom_fake = get_error_flags_fail;

	zassert_equal(regulator_get_error_flags(reg0, NULL), -EIO);
	zassert_equal(regulator_fake_get_error_flags_fake.call_count, 1U);
	zassert_equal(regulator_fake_get_error_flags_fake.arg0_val, reg0);
	zassert_equal(regulator_fake_get_error_flags_fake.arg1_val, NULL);
}

void *setup(void)
{
	zassert_true(device_is_ready(parent));
	zassert_true(device_is_ready(reg0));
	zassert_true(device_is_ready(reg1));
	zassert_true(device_is_ready(reg2));
	zassert_true(device_is_ready(reg3));
	zassert_true(device_is_ready(reg4));
	zassert_true(device_is_ready(reg5));

	/* REG1, REG2 initialized at init time (always-on/boot-on) */
	zassert_equal(regulator_fake_enable_fake.call_count, 2U);
	zassert_true(regulator_is_enabled(reg1));
	zassert_true(regulator_is_enabled(reg2));

	/* REG3 mode set at init time (initial-mode) */
	zassert_equal(regulator_fake_set_mode_fake.call_count, 1U);

	/* REG4 already disabled at init time (boot-off) */
	zassert_false(regulator_is_enabled(reg4));

	/* REG5 explicitly disabled at init time (boot-off) */
	zassert_equal(regulator_fake_disable_fake.call_count, 1U);
	zassert_false(regulator_is_enabled(reg5));

	return NULL;
}

ZTEST_SUITE(regulator_api, NULL, setup, NULL, NULL, NULL);
