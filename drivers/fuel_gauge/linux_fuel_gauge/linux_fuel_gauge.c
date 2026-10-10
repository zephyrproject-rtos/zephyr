/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_linux_fuel_gauge

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <nsi_errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/sys/util.h>

#include "linux_fuel_gauge_bottom.h"

struct linux_fuel_gauge_config {
	const char *path;
};

/*
 * A property is unsupported when the battery does not provide its attribute.
 * A value that is not a number or does not fit keeps its error, anything
 * else is an I/O error.
 */
static int linux_fuel_gauge_error(int ret)
{
	int err = nsi_errno_from_mid(-ret);

	switch (err) {
	case ENOENT:
		return -ENOTSUP;
	case EINVAL:
	case ERANGE:
		return -err;
	default:
		return -EIO;
	}
}

static int linux_fuel_gauge_read_attr(const struct device *dev, const char *attr, int64_t min,
				      int64_t max, int64_t *value)
{
	const struct linux_fuel_gauge_config *cfg = dev->config;
	int ret;

	ret = linux_fuel_gauge_read(cfg->path, attr, value);
	if (ret != 0) {
		return linux_fuel_gauge_error(ret);
	}

	if (*value < min || *value > max) {
		return -ERANGE;
	}

	return 0;
}

static int linux_fuel_gauge_get_property(const struct device *dev, fuel_gauge_prop_t prop,
					 union fuel_gauge_prop_val *val)
{
	int64_t raw;
	int ret;

	switch (prop) {
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT:
		ret = linux_fuel_gauge_read_attr(dev, "capacity", 0, UINT8_MAX, &raw);
		if (ret == 0) {
			val->relative_state_of_charge_pct = (uint8_t)raw;
		}
		break;

	case FUEL_GAUGE_VOLTAGE_UV:
		ret = linux_fuel_gauge_read_attr(dev, "voltage_now", INT32_MIN, INT32_MAX, &raw);
		if (ret == 0) {
			val->voltage_uv = (int32_t)raw;
		}
		break;

	case FUEL_GAUGE_CURRENT_UA:
		ret = linux_fuel_gauge_read_attr(dev, "current_now", INT32_MIN, INT32_MAX, &raw);
		if (ret == 0) {
			val->current_ua = (int32_t)raw;
		}
		break;

	case FUEL_GAUGE_CYCLE_COUNT:
		ret = linux_fuel_gauge_read_attr(dev, "cycle_count", 0, UINT32_MAX, &raw);
		if (ret == 0) {
			val->cycle_count = (uint32_t)raw;
		}
		break;

	case FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH:
		ret = linux_fuel_gauge_read_attr(dev, "charge_full", 0, UINT32_MAX, &raw);
		if (ret == 0) {
			val->full_charge_capacity_uah = (uint32_t)raw;
		}
		break;

	case FUEL_GAUGE_REMAINING_CAPACITY_UAH:
		ret = linux_fuel_gauge_read_attr(dev, "charge_now", 0, UINT32_MAX, &raw);
		if (ret == 0) {
			val->remaining_capacity_uah = (uint32_t)raw;
		}
		break;

	/* FUEL_GAUGE_DESIGN_CAPACITY is mAh in a uint16_t; sysfs reports uAh */
	case FUEL_GAUGE_DESIGN_CAPACITY:
		ret = linux_fuel_gauge_read_attr(dev, "charge_full_design", 0,
						 (int64_t)UINT16_MAX * 1000 + 999, &raw);
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

/*
 * Fill an SBS name field from a string attribute. The length field describes
 * the stored data, so a name as long as the field uses every byte of it and
 * has no terminator.
 */
static int linux_fuel_gauge_read_name(const struct device *dev, const char *attr, char *name,
				      size_t size, uint8_t *length)
{
	const struct linux_fuel_gauge_config *cfg = dev->config;
	char buf[MAX(SBS_GAUGE_MANUFACTURER_NAME_MAX_SIZE, SBS_GAUGE_DEVICE_NAME_MAX_SIZE) + 1];
	int ret;

	ret = linux_fuel_gauge_read_buffer(cfg->path, attr, buf, size + 1);
	if (ret != 0) {
		return linux_fuel_gauge_error(ret);
	}

	*length = strlen(buf);
	memset(name, 0, size);
	memcpy(name, buf, *length);

	return 0;
}

static int linux_fuel_gauge_get_buffer_property(const struct device *dev,
						fuel_gauge_prop_t prop_type, void *dst,
						size_t dst_len)
{
	int ret;

	if (dst == NULL) {
		return -EINVAL;
	}

	switch (prop_type) {
	case FUEL_GAUGE_MANUFACTURER_NAME:
		if (dst_len == sizeof(struct sbs_gauge_manufacturer_name)) {
			struct sbs_gauge_manufacturer_name *mfgname = dst;

			ret = linux_fuel_gauge_read_name(dev, "manufacturer",
							 mfgname->manufacturer_name,
							 sizeof(mfgname->manufacturer_name),
							 &mfgname->manufacturer_name_length);
		} else {
			ret = -EINVAL;
		}
		break;

	case FUEL_GAUGE_DEVICE_NAME:
		if (dst_len == sizeof(struct sbs_gauge_device_name)) {
			struct sbs_gauge_device_name *devname = dst;

			ret = linux_fuel_gauge_read_name(dev, "model_name", devname->device_name,
							 sizeof(devname->device_name),
							 &devname->device_name_length);
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
