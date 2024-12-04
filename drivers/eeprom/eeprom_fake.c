/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/eeprom/eeprom_fake.h>
#include <zephyr/fff.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif /* CONFIG_ZTEST */

#define DT_DRV_COMPAT zephyr_fake_eeprom

struct fake_eeprom_config {
	size_t size;
};

DEFINE_FAKE_VALUE_FUNC(int, fake_eeprom_read, const struct device *, off_t, void *, size_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_eeprom_write, const struct device *, off_t, const void *, size_t);

DEFINE_FAKE_VALUE_FUNC(size_t, fake_eeprom_size, const struct device *);

size_t fake_eeprom_size_delegate(const struct device *dev)
{
	const struct fake_eeprom_config *config = dev->config;

	return config->size;
}

#ifdef CONFIG_ZTEST
static void fake_eeprom_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(fake_eeprom_read);
	RESET_FAKE(fake_eeprom_write);
	RESET_FAKE(fake_eeprom_size);

	/* Re-install default delegate for reporting the EEPROM size */
	fake_eeprom_size_fake.custom_fake = fake_eeprom_size_delegate;
}

ZTEST_RULE(fake_eeprom_reset_rule, fake_eeprom_reset_rule_before, NULL);
#endif /* CONFIG_ZTEST */

static DEVICE_API(eeprom, fake_eeprom_driver_api) = {
	.read = fake_eeprom_read,
	.write = fake_eeprom_write,
	.size = fake_eeprom_size,
};

static int fake_eeprom_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Install default delegate for reporting the EEPROM size */
	fake_eeprom_size_fake.custom_fake = fake_eeprom_size_delegate;

	return 0;
}

#define FAKE_EEPROM_INIT(inst)							\
	static const struct fake_eeprom_config fake_eeprom_config_##inst = {	\
		.size = DT_INST_PROP(inst, size),				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, &fake_eeprom_init, NULL, NULL,		\
			      &fake_eeprom_config_##inst,			\
			      POST_KERNEL, CONFIG_EEPROM_INIT_PRIORITY,		\
			      &fake_eeprom_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_EEPROM_INIT)
