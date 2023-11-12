/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq2407x

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/charger.h>

struct ti_bq2407x_config {
	struct gpio_dt_spec ce_pin;
	struct gpio_dt_spec stat1_pin;
	struct gpio_dt_spec stat2_pin;
	struct gpio_dt_spec pg_pin;
	bool enable_charging;
};

struct ti_bq2407x_data {
	struct k_spinlock lock;
};

static int ti_bq2407x_get_prop_online(const struct device *dev, union charger_propval *val)
{
	const struct ti_bq2407x_config *config = dev->config;
	int pg;

	pg = gpio_pin_get_dt(&config->pg_pin);
	if (pg < 0) {
		return pg;
	}

	if (pg) {
		val->online = CHARGER_ONLINE_FIXED;
	} else {
		val->online = CHARGER_ONLINE_OFFLINE;
	}

	return 0;
}

static int ti_bq2407x_get_prop_status(const struct device *dev, union charger_propval *val)
{
	const struct ti_bq2407x_config *config = dev->config;
	int pg;
	int ce;
	int stat1;
	int stat2;

	pg = gpio_pin_get_dt(&config->pg_pin);
	if (pg < 0) {
		return pg;
	}

	if (!pg) {
		/* External supply is not present */
		val->status = CHARGER_STATUS_DISCHARGING;
		return 0;
	}

	ce = gpio_pin_get_dt(&config->pg_pin);
	if (ce < 0) {
		return ce;
	}

	if (!ce) {
		/* Charging is not enabled */
		val->status = CHARGER_STATUS_NOT_CHARGING;
		return 0;
	}

	/*
	 * The status table is as follows [stat1,stat2]
	 *   [1,1] CHARGER_STATUS_CHARGING (precharge)
	 *   [1,0] CHARGER_STATUS_CHARGING (fast charge)
	 *   [0,1] CHARGER_STATUS_FULL
	 *   [0,0] CHARGER_STATUS_NOT_CHARGING
	 */
	stat1 = gpio_pin_get_dt(&config->stat1_pin);
	if (stat1 < 0) {
		return stat1;
	}

	stat2 = gpio_pin_get_dt(&config->stat2_pin);
	if (stat2 < 0) {
		return stat2;
	}

	if (stat1) {
		val->status = CHARGER_STATUS_CHARGING;
	} else if (stat2) {
		val->status = CHARGER_STATUS_FULL;
	} else {
		val->status = CHARGER_STATUS_NOT_CHARGING;
	}

	return 0;
}

static int ti_bq2407x_get_property(const struct device *dev, const charger_prop_t prop,
				   union charger_propval *val)
{
	struct ti_bq2407x_data *data = dev->data;
	int ret = -ENOTSUP;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	switch (prop) {
	case CHARGER_PROP_ONLINE:
		ret = ti_bq2407x_get_prop_online(dev, val);
		break;

	case CHARGER_PROP_STATUS:
		ret = ti_bq2407x_get_prop_status(dev, val);
		break;

	default:
		break;
	}

	k_spin_unlock(&data->lock, key);
	return ret;
}

static int ti_bq2407x_charge_enable(const struct device *dev, const bool enable)
{
	struct ti_bq2407x_data *data = dev->data;
	const struct ti_bq2407x_config *config = dev->config;
	int ret;

	K_SPINLOCK(&data->lock) {
		ret = gpio_pin_set_dt(&config->ce_pin, enable);
	}

	return ret;
}

static const struct charger_driver_api ti_bq2407x_api = {
	.get_property = ti_bq2407x_get_property,
	.charge_enable = ti_bq2407x_charge_enable,
};

static int ti_bq2407x_init(const struct device *dev)
{
	const struct ti_bq2407x_config *config = dev->config;

	if (!gpio_is_ready_dt(&config->stat1_pin) ||
	    !gpio_is_ready_dt(&config->stat2_pin) ||
	    !gpio_is_ready_dt(&config->pg_pin)) {
		return -ENODEV;
	}

	if ((gpio_pin_configure_dt(&config->stat1_pin, 0) < 0) ||
	    (gpio_pin_configure_dt(&config->stat2_pin, 0) < 0) ||
	    (gpio_pin_configure_dt(&config->pg_pin, 0) < 0)) {
		return -ENODEV;
	}

	if (config->enable_charging) {
		return gpio_pin_configure_dt(&config->ce_pin, GPIO_OUTPUT_ACTIVE);
	}

	return gpio_pin_configure_dt(&config->ce_pin, GPIO_OUTPUT_INACTIVE);
}

#define TI_BQ2407X_DEVICE(inst)							\
	static struct ti_bq2407x_data ti_bq2407x_data##inst;			\
	static const struct ti_bq2407x_config ti_bq2407x_config##inst = {	\
		.ce_pin = GPIO_DT_SPEC_INST_GET(inst, ce_gpios),		\
		.stat1_pin = GPIO_DT_SPEC_INST_GET(inst, stat1_gpios),		\
		.stat2_pin = GPIO_DT_SPEC_INST_GET(inst, stat2_gpios),		\
		.pg_pin = GPIO_DT_SPEC_INST_GET(inst, pg_gpios),		\
		.enable_charging = DT_INST_PROP(inst, enable_charging),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, ti_bq2407x_init, NULL,			\
			      &ti_bq2407x_data##inst,				\
			      &ti_bq2407x_config##inst, POST_KERNEL,		\
			      CONFIG_CHARGER_INIT_PRIORITY, &ti_bq2407x_api);

DT_INST_FOREACH_STATUS_OKAY(TI_BQ2407X_DEVICE)
