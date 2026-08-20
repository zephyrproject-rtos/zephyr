/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_linux_fuel_gauge

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/fuel_gauge.h>

#include "linux_fuel_gauge_bottom.h"

struct linux_fuel_gauge_config {
	const char *path;
};

static int linux_fuel_gauge_get_property(const struct device *dev, fuel_gauge_prop_t prop,
					 union fuel_gauge_prop_val *val)
{
	const struct linux_fuel_gauge_config *cfg = dev->config;
	long raw;
	int ret;

	switch (prop) {
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT:
		ret = linux_fuel_gauge_read(cfg->path, "capacity", &raw);
		if (ret == 0) {
			val->relative_state_of_charge_pct = (uint8_t)raw;
		}
		break;

	case FUEL_GAUGE_VOLTAGE_UV:
		ret = linux_fuel_gauge_read(cfg->path, "voltage_now", &raw);
		if (ret == 0) {
			val->voltage_uv = (int32_t)raw;
		}
		break;

	case FUEL_GAUGE_CURRENT_UA:
		ret = linux_fuel_gauge_read(cfg->path, "current_now", &raw);
		if (ret == 0) {
			val->current_ua = (int32_t)raw;
		}
		break;

	case FUEL_GAUGE_CYCLE_COUNT:
		ret = linux_fuel_gauge_read(cfg->path, "cycle_count", &raw);
		if (ret == 0) {
			val->cycle_count = (uint32_t)raw;
		}
		break;

	case FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH:
		ret = linux_fuel_gauge_read(cfg->path, "charge_full", &raw);
		if (ret == 0) {
			val->full_charge_capacity_uah = (uint32_t)raw;
		}
		break;

	case FUEL_GAUGE_REMAINING_CAPACITY_UAH:
		ret = linux_fuel_gauge_read(cfg->path, "charge_now", &raw);
		if (ret == 0) {
			val->remaining_capacity_uah = (uint32_t)raw;
		}
		break;

	/* FUEL_GAUGE_DESIGN_CAPACITY is mAh in a uint16_t; sysfs reports uAh */
	case FUEL_GAUGE_DESIGN_CAPACITY:
		ret = linux_fuel_gauge_read(cfg->path, "charge_full_design", &raw);
		if (ret == 0) {
			val->design_cap = (uint16_t)(raw / 1000);
		}
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int linux_fuel_gauge_get_buffer_property(const struct device *dev,
						fuel_gauge_prop_t prop_type, void *dst,
						size_t dst_len)
{
	const struct linux_fuel_gauge_config *cfg = dev->config;
	char buf[MAX(SBS_GAUGE_MANUFACTURER_NAME_MAX_SIZE, SBS_GAUGE_DEVICE_NAME_MAX_SIZE) + 1];
	int ret = 0;

	if (dst == NULL) {
		return -EINVAL;
	}

	switch (prop_type) {
	case FUEL_GAUGE_MANUFACTURER_NAME:
		if (dst_len == sizeof(struct sbs_gauge_manufacturer_name)) {
			struct sbs_gauge_manufacturer_name *mfgname = dst;

			ret = linux_fuel_gauge_read_buffer(cfg->path, "manufacturer", buf,
							   sizeof(buf));
			if (ret == 0) {
				mfgname->manufacturer_name_length =
					MIN(strlen(buf), SBS_GAUGE_MANUFACTURER_NAME_MAX_SIZE - 1);
				memcpy(mfgname->manufacturer_name, buf,
				       mfgname->manufacturer_name_length);
				mfgname->manufacturer_name[mfgname->manufacturer_name_length] =
					'\0';
			}
		} else {
			ret = -EINVAL;
		}
		break;

	case FUEL_GAUGE_DEVICE_NAME:
		if (dst_len == sizeof(struct sbs_gauge_device_name)) {
			struct sbs_gauge_device_name *devname = dst;

			ret = linux_fuel_gauge_read_buffer(cfg->path, "model_name", buf,
							   sizeof(buf));
			if (ret == 0) {
				devname->device_name_length =
					MIN(strlen(buf), SBS_GAUGE_DEVICE_NAME_MAX_SIZE - 1);
				memcpy(devname->device_name, buf, devname->device_name_length);
				devname->device_name[devname->device_name_length] = '\0';
			}
		} else {
			ret = -EINVAL;
		}
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static DEVICE_API(fuel_gauge, linux_fuel_gauge_api) = {
	.get_property = linux_fuel_gauge_get_property,
	.get_buffer_property = linux_fuel_gauge_get_buffer_property,
};

#define LINUX_FUEL_GAUGE_INIT(inst)                                                                \
	static const struct linux_fuel_gauge_config linux_fuel_gauge_cfg_##inst = {                \
		.path = DT_INST_PROP(inst, path),                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &linux_fuel_gauge_cfg_##inst, POST_KERNEL,   \
			      CONFIG_FUEL_GAUGE_INIT_PRIORITY, &linux_fuel_gauge_api);

DT_INST_FOREACH_STATUS_OKAY(LINUX_FUEL_GAUGE_INIT)
