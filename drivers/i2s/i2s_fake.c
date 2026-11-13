/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_fake_i2s

#include <zephyr/drivers/i2s/fake.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif /* CONFIG_ZTEST */

DEFINE_FAKE_VALUE_FUNC(int,
		       i2s_fake_configure,
		       const struct device *,
		       enum i2s_dir,
		       const struct i2s_config *);

DEFINE_FAKE_VALUE_FUNC(const struct i2s_config *,
		       i2s_fake_config_get,
		       const struct device *,
		       enum i2s_dir);

DEFINE_FAKE_VALUE_FUNC(int,
		       i2s_fake_read,
		       const struct device *,
		       void **,
		       size_t *);

DEFINE_FAKE_VALUE_FUNC(int,
		       i2s_fake_write,
		       const struct device *,
		       void *,
		       size_t);

DEFINE_FAKE_VALUE_FUNC(int,
		       i2s_fake_trigger,
		       const struct device *,
		       enum i2s_dir,
		       enum i2s_trigger_cmd);

#ifdef CONFIG_I2S_RTIO
DEFINE_FAKE_VOID_FUNC(i2s_fake_iodev_submit,
		      const struct device *,
		      struct rtio_iodev_sqe *);
#endif /* CONFIG_I2S_RTIO */

#ifdef CONFIG_ZTEST
static void fake_i2s_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(i2s_fake_configure);
	RESET_FAKE(i2s_fake_config_get);
	RESET_FAKE(i2s_fake_read);
	RESET_FAKE(i2s_fake_write);
	RESET_FAKE(i2s_fake_trigger);

#ifdef CONFIG_I2S_RTIO
	RESET_FAKE(i2s_fake_iodev_submit);
#endif /* CONFIG_I2S_RTIO */
}

ZTEST_RULE(fake_i2s_reset_rule, fake_i2s_reset_rule_before, NULL);
#endif /* CONFIG_ZTEST */

static DEVICE_API(i2s, driver_api) = {
	.configure = i2s_fake_configure,
	.config_get = i2s_fake_config_get,
	.read = i2s_fake_read,
	.write = i2s_fake_write,
	.trigger = i2s_fake_trigger,
#ifdef CONFIG_I2S_RTIO
	.iodev_submit = i2s_fake_iodev_submit,
#endif /* CONFIG_I2S_RTIO */
};

#define I2S_FAKE_DEFINE(inst)									\
	DEVICE_DT_INST_DEFINE(									\
		inst,										\
		NULL,										\
		NULL,										\
		NULL,										\
		NULL,										\
		POST_KERNEL,									\
		CONFIG_I2S_INIT_PRIORITY,							\
		&driver_api									\
	);

DT_INST_FOREACH_STATUS_OKAY(I2S_FAKE_DEFINE);
