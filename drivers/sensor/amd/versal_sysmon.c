/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc. (AMD)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_versal_sysmon

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <string.h>
#include <errno.h>

#include "versal_sysmon.h"

LOG_MODULE_REGISTER(versal_sysmon_sensor, CONFIG_SENSOR_LOG_LEVEL);

struct sysmon_chan_data {
	uint8_t reg;
	const char *name;
	uint8_t match_idx;
};

struct sysmon_data {
	DEVICE_MMIO_RAM;

	const struct device *dev;

	uint32_t supply_raw[SYSMON_NUM_SUPPLY_CHANNELS];
	uint32_t temp_raw;

	sensor_trigger_handler_t event_handler;
	struct sensor_trigger event_trig;

	struct k_work events_work;
	struct k_work_delayable unmask_work;

	uint32_t enabled_alarm_mask;
	atomic_t pending_events;
};

struct versal_sysmon_cfg {
	DEVICE_MMIO_ROM;

	uint8_t num_channels;
	struct sysmon_chan_data *chan;

	void (*irq_config_func)(const struct device *dev);
};

struct sysmon_chan_name {
	const char *name;
	uint8_t idx;
};

static const struct sysmon_chan_name sysmon_chan_names[] = {
	{"vcc_batt", VERSAL_SYSMON_VCC_BAT},
	{"vcc_pmc", VERSAL_SYSMON_VCC_PMC},
	{"vcc_psfp", VERSAL_SYSMON_VCC_PSFP},
	{"vcc_pslp", VERSAL_SYSMON_VCC_PSLP},
	{"vcc_ram", VERSAL_SYSMON_VCC_RAM},
	{"vcc_soc", VERSAL_SYSMON_VCC_SOC},
	{"vccint", VERSAL_SYSMON_VCCINT},
	{"vccaux", VERSAL_SYSMON_VCCAUX},
	{"vccaux_pmc", VERSAL_SYSMON_VCCAUX_PMC},
	{"vccaux_smon", VERSAL_SYSMON_VCCAUX_SMON},
	{"vp_vn", VERSAL_SYSMON_VP_VN},

	{"vcco_306", VERSAL_SYSMON_VCCO_306},
	{"vcco_406", VERSAL_SYSMON_VCCO_406},
	{"vcco_500", VERSAL_SYSMON_VCCO_500},
	{"vcco_501", VERSAL_SYSMON_VCCO_501},
	{"vcco_502", VERSAL_SYSMON_VCCO_502},
	{"vcco_503", VERSAL_SYSMON_VCCO_503},
	{"vcco_700", VERSAL_SYSMON_VCCO_700},
	{"vcco_701", VERSAL_SYSMON_VCCO_701},
	{"vcco_702", VERSAL_SYSMON_VCCO_702},
	{"vcco_703", VERSAL_SYSMON_VCCO_703},
	{"vcco_704", VERSAL_SYSMON_VCCO_704},
	{"vcco_705", VERSAL_SYSMON_VCCO_705},
	{"vcco_706", VERSAL_SYSMON_VCCO_706},
	{"vcco_707", VERSAL_SYSMON_VCCO_707},
	{"vcco_708", VERSAL_SYSMON_VCCO_708},
	{"vcco_709", VERSAL_SYSMON_VCCO_709},
	{"vcco_710", VERSAL_SYSMON_VCCO_710},
	{"vcco_711", VERSAL_SYSMON_VCCO_711},

	{"gty_avcc_103", VERSAL_SYSMON_GTY_AVCC_103},
	{"gty_avcc_104", VERSAL_SYSMON_GTY_AVCC_104},
	{"gty_avcc_105", VERSAL_SYSMON_GTY_AVCC_105},
	{"gty_avcc_106", VERSAL_SYSMON_GTY_AVCC_106},
	{"gty_avcc_200", VERSAL_SYSMON_GTY_AVCC_200},
	{"gty_avcc_201", VERSAL_SYSMON_GTY_AVCC_201},
	{"gty_avcc_202", VERSAL_SYSMON_GTY_AVCC_202},
	{"gty_avcc_203", VERSAL_SYSMON_GTY_AVCC_203},
	{"gty_avcc_204", VERSAL_SYSMON_GTY_AVCC_204},
	{"gty_avcc_205", VERSAL_SYSMON_GTY_AVCC_205},
	{"gty_avcc_206", VERSAL_SYSMON_GTY_AVCC_206},

	{"gty_avccaux_103", VERSAL_SYSMON_GTY_AVCCAUX_103},
	{"gty_avccaux_104", VERSAL_SYSMON_GTY_AVCCAUX_104},
	{"gty_avccaux_105", VERSAL_SYSMON_GTY_AVCCAUX_105},
	{"gty_avccaux_106", VERSAL_SYSMON_GTY_AVCCAUX_106},
	{"gty_avccaux_200", VERSAL_SYSMON_GTY_AVCCAUX_200},
	{"gty_avccaux_201", VERSAL_SYSMON_GTY_AVCCAUX_201},
	{"gty_avccaux_202", VERSAL_SYSMON_GTY_AVCCAUX_202},
	{"gty_avccaux_203", VERSAL_SYSMON_GTY_AVCCAUX_203},
	{"gty_avccaux_204", VERSAL_SYSMON_GTY_AVCCAUX_204},
	{"gty_avccaux_205", VERSAL_SYSMON_GTY_AVCCAUX_205},
	{"gty_avccaux_206", VERSAL_SYSMON_GTY_AVCCAUX_206},

	{"gty_avtt_103", VERSAL_SYSMON_GTY_AVTT_103},
	{"gty_avtt_104", VERSAL_SYSMON_GTY_AVTT_104},
	{"gty_avtt_105", VERSAL_SYSMON_GTY_AVTT_105},
	{"gty_avtt_106", VERSAL_SYSMON_GTY_AVTT_106},
	{"gty_avtt_200", VERSAL_SYSMON_GTY_AVTT_200},
	{"gty_avtt_201", VERSAL_SYSMON_GTY_AVTT_201},
	{"gty_avtt_202", VERSAL_SYSMON_GTY_AVTT_202},
	{"gty_avtt_203", VERSAL_SYSMON_GTY_AVTT_203},
	{"gty_avtt_204", VERSAL_SYSMON_GTY_AVTT_204},
	{"gty_avtt_205", VERSAL_SYSMON_GTY_AVTT_205},
	{"gty_avtt_206", VERSAL_SYSMON_GTY_AVTT_206},
};

static inline int twoscomp(int val)
{
	return (~val + 1);
}

static void sysmon_raw_to_millicelsius(int32_t raw_data, int32_t *val)
{
	*val = (((raw_data & 0x8000) ? -(twoscomp(raw_data)) : raw_data) * SYSMON_MILLI_SCALE) >>
	       SYSMON_FRACTIONAL_SHIFT;
}

static void sysmon_millicelsius_to_raw(int32_t *raw_data, int32_t val)
{
	*raw_data = (val << SYSMON_FRACTIONAL_SHIFT) / SYSMON_MILLI_SCALE;
}

void sysmon_supply_rawtoprocessed(int raw_data, int *val)
{
	int mantissa, format, exponent;

	mantissa = raw_data & SYSMON_MANTISSA_MASK;
	exponent = 16 - ((raw_data & SYSMON_MODE_MASK) >> SYSMON_MODE_SHIFT);
	format = (raw_data & SYSMON_FMT_MASK) >> SYSMON_FMT_SHIFT;

	if (format && (mantissa >> SYSMON_MANTISSA_SIGN_SHIFT)) {
		*val = ((~mantissa & SYSMON_MANTISSA_MASK) * (-SYSMON_MILLI_SCALE)) >> exponent;
	} else {
		*val = (mantissa * SYSMON_MILLI_SCALE) >> exponent;
	}
}

void sysmon_processed_to_supply_raw(int val, uint32_t reg_val, uint32_t *raw)
{
	int exponent, format;
	int32_t mantissa;
	uint32_t cfg;

	cfg = reg_val & (SYSMON_MODE_MASK | SYSMON_FMT_MASK);
	exponent = 16 - ((cfg & SYSMON_MODE_MASK) >> SYSMON_MODE_SHIFT);
	format = (cfg & SYSMON_FMT_MASK) >> SYSMON_FMT_SHIFT;
	mantissa = (val << exponent) / SYSMON_MILLI_SCALE;

	if (format) {
		if (mantissa > SYSMON_UPPER_SATURATION_SIGNED) {
			mantissa = SYSMON_UPPER_SATURATION_SIGNED;
		}

		if (mantissa < SYSMON_LOWER_SATURATION_SIGNED) {
			mantissa = SYSMON_LOWER_SATURATION_SIGNED;
		}
	} else {
		if (mantissa > SYSMON_UPPER_SATURATION) {
			mantissa = SYSMON_UPPER_SATURATION;
		}

		if (mantissa < 0) {
			mantissa = 0;
		}
	}

	if (format && mantissa < 0) {
		mantissa = (~(-mantissa) + 1) & SYSMON_MANTISSA_MASK;
	} else {
		mantissa &= SYSMON_MANTISSA_MASK;
	}

	*raw = cfg | mantissa;
}

static uint32_t sysmon_read(const struct device *dev, uint32_t off)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + off);
}

static void sysmon_write(const struct device *dev, uint32_t off, uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_GET(dev) + off);
}

static int sysmon_strcmp(const char *config_chan_names, uint8_t *match_idx)
{
	for (size_t i = 0; i < ARRAY_SIZE(sysmon_chan_names); i++) {
		if (strcmp(config_chan_names, sysmon_chan_names[i].name) == 0) {
			*match_idx = sysmon_chan_names[i].idx;
			return 0;
		}
	}

	LOG_ERR("Unsupported channel %s", config_chan_names);

	return -ENOTSUP;
}

static inline void sysmon_notify(const struct device *dev, sensor_trigger_handler_t handler,
				 struct sensor_trigger *trig)
{
	if (handler) {
		handler(dev, trig);
	}
}

static void versal_sysmon_handle_event(const struct device *dev, uint32_t event)
{
	struct sysmon_data *data = dev->data;

	switch (event) {
	case SYSMON_BIT_TEMP:
		sysmon_notify(dev, data->event_handler, &data->event_trig);
		break;

	case SYSMON_BIT_OT:
		sysmon_notify(dev, data->event_handler, &data->event_trig);
		break;

	case SYSMON_BIT_ALARM4:
	case SYSMON_BIT_ALARM3:
	case SYSMON_BIT_ALARM2:
	case SYSMON_BIT_ALARM1:
	case SYSMON_BIT_ALARM0: {
		uint32_t bank = event - SYSMON_BIT_ALARM0;
		uint32_t alarm_flag_offset = SYSMON_ALARM_FLAG + (bank * 4U);
		uint32_t reg_val = sysmon_read(dev, alarm_flag_offset);
		uint32_t pending = reg_val;

		while (pending) {
			uint32_t bit = __builtin_ctz(pending);

			pending &= ~BIT(bit);
			sysmon_notify(dev, data->event_handler, &data->event_trig);
		}

		sysmon_write(dev, alarm_flag_offset, reg_val);
		break;
	}
	default:
		LOG_ERR("Unsupported event %u", event);
		break;
	}
}

static void versal_sysmon_handle_events(const struct device *dev, uint32_t events)
{
	for (uint32_t bit = 0; bit < SYSMON_NO_OF_EVENTS; bit++) {
		if (events & BIT(bit)) {
			versal_sysmon_handle_event(dev, bit);
		}
	}
}

static int versal_sysmon_find_chan(const struct versal_sysmon_cfg *cfg, enum sensor_channel chan)
{
	for (int i = 0; i < cfg->num_channels; i++) {
		if (cfg->chan[i].match_idx == chan) {
			return cfg->chan[i].reg;
		}
	}

	LOG_ERR("Invalid channel");
	return -EINVAL;
}

static int sysmon_trigger_chan_to_id(const struct sensor_trigger *trig, uint32_t *chan_id,
				     const struct versal_sysmon_cfg *cfg, uint32_t *alarm_chan)
{
	int reg;
	int ch = (int)trig->chan;

	*alarm_chan = UINT32_MAX;

	if (ch >= VERSAL_SYSMON_VCC_BAT && ch <= VERSAL_SYSMON_GTY_AVTT_206) {
		reg = versal_sysmon_find_chan(cfg, trig->chan);
		if (reg < 0) {
			return reg;
		}

		*alarm_chan = (uint32_t)reg;
		*chan_id = SYSMON_BIT_ALARM0 + ((uint32_t)reg / 32U);
		return 0;
	}

	switch ((int)trig->chan) {
	case VERSAL_SYSMON_DIE_TEMP:
		*chan_id = SYSMON_BIT_TEMP;
		break;

	case VERSAL_SYSMON_DIE_TEMP_OT:
		*chan_id = SYSMON_BIT_OT;
		break;

	default:
		LOG_ERR("Unsupported channel for trigger");
		return -ENOTSUP;
	}
	return 0;
}

static void sysmon_alarm_bit_set(const struct device *dev, uint32_t chan_id, bool enable)
{
	uint32_t reg_index = chan_id / 32U;
	uint32_t bit = chan_id % 32U;
	uint32_t off = SYSMON_ALARM_REG + (reg_index * 4U);
	uint32_t v = sysmon_read(dev, off);

	if (enable) {
		v |= BIT(bit);
	} else {
		v &= ~BIT(bit);
	}

	sysmon_write(dev, off, v);
}

static int versal_sysmon_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				     sensor_trigger_handler_t handler)
{
	struct sysmon_data *data = dev->data;
	int32_t chan_id, mask, alarm_chan;
	int ret;
	unsigned int key;
	const struct versal_sysmon_cfg *cfg = dev->config;

	ret = sysmon_trigger_chan_to_id(trig, &chan_id, cfg, &alarm_chan);
	if (ret) {
		LOG_ERR("trigger_set: map failed chan=%d type=%d ret=%d\n", trig->chan, trig->type,
			ret);
		return ret;
	}
	mask = BIT(chan_id);
	if (handler == NULL) {
		sysmon_write(dev, SYSMON_IDR_OFFSET, mask);

		if (alarm_chan != UINT32_MAX) {
			sysmon_alarm_bit_set(dev, alarm_chan, false);
			sysmon_write(dev, SYSMON_ALARM_FLAG + ((alarm_chan / 32U) * 4U),
				     BIT(alarm_chan % 32U));
		}

		sysmon_write(dev, SYSMON_ISR, mask);
		return 0;
	}

	key = irq_lock();
	data->event_handler = handler;
	data->event_trig = *trig;
	if (handler) {
		data->enabled_alarm_mask |= mask;
	} else {
		data->enabled_alarm_mask &= ~mask;
	}
	irq_unlock(key);

	return 0;
}

static void sysmon_supply_attr_set(const struct device *dev, enum sensor_attribute attr,
				   uint8_t chan, const struct sensor_value *val)
{
	uint32_t offset, reg, raw;
	int32_t temp_val = 0;
	uint8_t reg_index, chan_index;

	switch ((int)attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		offset = (chan * 4) + SYSMON_SUPPLY_LOWER_THRESH;
		reg = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		sysmon_processed_to_supply_raw(val->val1, reg, &raw);
		sys_write32(raw, DEVICE_MMIO_GET(dev) + offset);
		break;

	case SENSOR_ATTR_UPPER_THRESH:
		offset = (chan * 4) + SYSMON_SUPPLY_UPPER_THRESH;
		reg = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		sysmon_processed_to_supply_raw(val->val1, reg, &raw);
		sys_write32(raw, DEVICE_MMIO_GET(dev) + offset);
		break;

	case SENSOR_ATTR_ALERT:
		temp_val = (val->val1 != 0);
		reg_index = chan / 32;
		chan_index = chan % 32U;
		offset = (reg_index * 4) + SYSMON_ALARM_REG;
		reg = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		if (temp_val) {
			reg |= BIT(chan_index);
		} else {
			reg &= ~BIT(chan_index);
		}

		if (reg != 0U) {
			sys_write32(BIT(reg_index), DEVICE_MMIO_GET(dev) + SYSMON_IER_OFFSET);
			sys_write32(reg, DEVICE_MMIO_GET(dev) + offset);
		} else {
			sys_write32(BIT(reg_index), DEVICE_MMIO_GET(dev) + SYSMON_IDR_OFFSET);
			sys_write32(reg, DEVICE_MMIO_GET(dev) + offset);
		}
		break;

	default:
		LOG_ERR("Invalid Attribute");
		break;
	}
}

static void versal_sysmon_events_work_handler(struct k_work *work)
{
	struct sysmon_data *data = CONTAINER_OF(work, struct sysmon_data, events_work);
	const struct device *dev = data->dev;
	uint32_t events = data->pending_events;

	data->pending_events = 0;
	versal_sysmon_handle_events(dev, events);
	k_work_reschedule(&data->unmask_work, K_MSEC(1));
}

void sysmon_temp_attr_set(const struct device *dev, enum sensor_attribute attr,
			  const struct sensor_value *val)
{
	uint32_t offset;
	int32_t raw, temp_val = 0;
	struct sysmon_data *data = dev->data;

	switch ((int)attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		offset = SYSMON_TEMP_LOWER_THRESH;
		raw = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		sysmon_millicelsius_to_raw(&raw, val->val1);
		sys_write32(raw, DEVICE_MMIO_GET(dev) + offset);
		break;

	case SENSOR_ATTR_UPPER_THRESH:
		offset = SYSMON_TEMP_UPPER_THRESH;
		raw = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		sysmon_millicelsius_to_raw(&raw, val->val1);
		sys_write32(raw, DEVICE_MMIO_GET(dev) + offset);
		break;

	case SENSOR_ATTR_HYSTERESIS:
		offset = SYSMON_TEMP_EV_CFG;
		sys_write32(val->val1, DEVICE_MMIO_GET(dev) + offset);
		break;

	case SENSOR_ATTR_ALERT:
		temp_val = !!val->val1;
		if (temp_val) {
			if (data->enabled_alarm_mask & BIT(SYSMON_BIT_TEMP)) {
				sys_write32(BIT(SYSMON_BIT_TEMP),
					    DEVICE_MMIO_GET(dev) + SYSMON_IER_OFFSET);
			}

			if (data->enabled_alarm_mask & BIT(SYSMON_BIT_OT)) {
				sys_write32(BIT(SYSMON_BIT_OT),
					    DEVICE_MMIO_GET(dev) + SYSMON_IER_OFFSET);
			}
		} else {
			if (data->enabled_alarm_mask & BIT(SYSMON_BIT_TEMP)) {
				sys_write32(BIT(SYSMON_BIT_TEMP),
					    DEVICE_MMIO_GET(dev) + SYSMON_IDR_OFFSET);
			}

			if (data->enabled_alarm_mask & BIT(SYSMON_BIT_OT)) {
				sys_write32(BIT(SYSMON_BIT_OT),
					    DEVICE_MMIO_GET(dev) + SYSMON_IDR_OFFSET);
			}
		}
		break;

	case SENSOR_ATTR_UPPER_THRESH_OT:
		offset = SYSMON_OT_TH_UP;
		raw = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		sysmon_millicelsius_to_raw(&raw, val->val1);
		sys_write32(raw, DEVICE_MMIO_GET(dev) + offset);
		break;

	case SENSOR_ATTR_LOWER_THRESH_OT:
		offset = SYSMON_OT_TH_LOW;
		raw = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		sysmon_millicelsius_to_raw(&raw, val->val1);
		sys_write32(raw, DEVICE_MMIO_GET(dev) + offset);
		break;

	default:
		LOG_ERR("Invalid Attribute");
		break;
	}
}

static int versal_sysmon_attr_set(const struct device *dev, enum sensor_channel chan,
				  enum sensor_attribute attr, const struct sensor_value *val)
{
	int chan_num;
	const struct versal_sysmon_cfg *cfg = dev->config;

	if ((uint32_t)chan > (uint32_t)VERSAL_SYSMON_DIE_TEMP ||
	    (uint32_t)chan < (uint32_t)VERSAL_SYSMON_VCC_BAT) {
		LOG_ERR("Unsupported channel");
		return -ENOTSUP;
	}

	if ((uint32_t)chan == (uint32_t)VERSAL_SYSMON_DIE_TEMP) {
		sysmon_temp_attr_set(dev, attr, val);
	} else {
		chan_num = versal_sysmon_find_chan(cfg, chan);
		if (chan_num < 0) {
			return chan_num;
		}

		sysmon_supply_attr_set(dev, attr, chan_num, val);
	}

	return 0;
}

static void sysmon_supply_attr_get(const struct device *dev, enum sensor_attribute attr,
				   uint8_t chan, struct sensor_value *val)
{
	uint32_t offset, raw;
	uint8_t reg_index;

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		offset = (chan * 4) + SYSMON_SUPPLY_LOWER_THRESH;
		raw = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		sysmon_supply_rawtoprocessed(raw, &val->val1);
		break;

	case SENSOR_ATTR_UPPER_THRESH:
		offset = (chan * 4) + SYSMON_SUPPLY_UPPER_THRESH;
		raw = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		sysmon_supply_rawtoprocessed(raw, &val->val1);
		break;

	case SENSOR_ATTR_ALERT:
		reg_index = chan / 32;
		offset = (reg_index * 4) + SYSMON_ALARM_REG;
		val->val1 = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		val->val2 = 0;
		break;

	default:
		LOG_ERR("Invalid Attribute");
		break;
	}
}

void sysmon_temp_attr_get(const struct device *dev, enum sensor_attribute attr,
			  struct sensor_value *val)
{
	uint32_t offset, raw;

	switch ((int)attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		offset = SYSMON_TEMP_LOWER_THRESH;
		raw = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		sysmon_raw_to_millicelsius(raw, &val->val1);
		break;

	case SENSOR_ATTR_UPPER_THRESH:
		offset = SYSMON_TEMP_UPPER_THRESH;
		raw = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		sysmon_raw_to_millicelsius(raw, &val->val1);
		break;

	case SENSOR_ATTR_HYSTERESIS:
		offset = SYSMON_TEMP_EV_CFG;
		val->val1 = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		break;

	case SENSOR_ATTR_UPPER_THRESH_OT:
		offset = SYSMON_OT_TH_UP;
		raw = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		sysmon_raw_to_millicelsius(raw, &val->val1);
		break;

	case SENSOR_ATTR_LOWER_THRESH_OT:
		offset = SYSMON_OT_TH_LOW;
		raw = sys_read32(DEVICE_MMIO_GET(dev) + offset);
		sysmon_raw_to_millicelsius(raw, &val->val1);
		break;

	default:
		LOG_ERR("Invalid Attribute");
		break;
	}
}

static int32_t versal_sysmon_attr_get(const struct device *dev, enum sensor_channel chan,
				      enum sensor_attribute attr, struct sensor_value *val)
{
	int chan_num;
	const struct versal_sysmon_cfg *cfg = dev->config;

	if ((uint32_t)chan > (uint32_t)VERSAL_SYSMON_DIE_TEMP ||
	    (uint32_t)chan < (uint32_t)VERSAL_SYSMON_VCC_BAT) {
		LOG_ERR("Unsupported channel");
		return -ENOTSUP;
	}

	if ((uint32_t)chan == (uint32_t)VERSAL_SYSMON_DIE_TEMP) {
		sysmon_temp_attr_get(dev, attr, val);
	} else {
		chan_num = versal_sysmon_find_chan(cfg, chan);
		if (chan_num < 0) {
			return chan_num;
		}

		sysmon_supply_attr_get(dev, attr, chan_num, val);
	}

	return 0;
}

static int versal_sysmon_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct sysmon_data *data = dev->data;
	const struct versal_sysmon_cfg *cfg = dev->config;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t offset;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Unsupported channel");
		return -ENOTSUP;
	}

	data->temp_raw = sys_read32(base + SYSMON_TEMP_REG);

	for (uint32_t i = 0; i < cfg->num_channels; i++) {
		offset = SYSMON_SUPPLY_BASE + (cfg->chan[i].reg * 4);
		data->supply_raw[cfg->chan[i].reg] = sys_read32(base + offset);
	}

	return 0;
}

static int versal_sysmon_channel_get(const struct device *dev, enum sensor_channel chan,
				     struct sensor_value *val)
{
	const struct versal_sysmon_cfg *cfg = dev->config;
	struct sysmon_data *data = dev->data;
	int channel_address;
	uint32_t raw;

	if ((uint32_t)chan > (uint32_t)VERSAL_SYSMON_DIE_TEMP ||
	    (uint32_t)chan < (uint32_t)VERSAL_SYSMON_VCC_BAT) {
		LOG_ERR("Unsupported channel");
		return -ENOTSUP;
	}

	if ((uint32_t)chan == (uint32_t)VERSAL_SYSMON_DIE_TEMP) {
		raw = data->temp_raw;
		sysmon_raw_to_millicelsius(raw, &val->val1);
		val->val2 = 0;
	} else {
		channel_address = versal_sysmon_find_chan(cfg, chan);
		if (channel_address < 0) {
			return channel_address;
		}

		raw = data->supply_raw[channel_address];
		sysmon_supply_rawtoprocessed(raw, &val->val1);
		val->val2 = 0;
	}

	return 0;
}

static void versal_sysmon_unmask_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct sysmon_data *data = CONTAINER_OF(dwork, struct sysmon_data, unmask_work);
	const struct device *dev = data->dev;

	sysmon_write(dev, SYSMON_IER_OFFSET, (data->enabled_alarm_mask));
}

static int versal_sysmon_init(const struct device *dev)
{
	const struct versal_sysmon_cfg *cfg = dev->config;
	struct sysmon_data *data = dev->data;
	uint32_t cfg_val = 0;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	sysmon_write(dev, SYSMON_NPI_LOCK, NPI_UNLOCK);
	sysmon_write(dev, SYSMON_IDR_OFFSET, 0xFFFFFFFFu);
	sysmon_write(dev, SYSMON_ISR, 0xFFFFFFFFu);
	data->dev = dev;

	k_work_init(&data->events_work, versal_sysmon_events_work_handler);
	k_work_init_delayable(&data->unmask_work, versal_sysmon_unmask_work_handler);

	sysmon_write(dev, SYSMON_CONFIG, cfg_val);
	if (cfg->irq_config_func != NULL) {
		cfg->irq_config_func(dev);
	}
	data->enabled_alarm_mask = 0;

	for (uint8_t i = 0U; i < cfg->num_channels; i++) {
		if (sysmon_strcmp(cfg->chan[i].name, &cfg->chan[i].match_idx) != 0) {
			return -EINVAL;
		}
	}

	return 0;
}

static DEVICE_API(sensor, versal_sysmon_api) = {
	.attr_set = versal_sysmon_attr_set,
	.attr_get = versal_sysmon_attr_get,
	.sample_fetch = versal_sysmon_sample_fetch,
	.channel_get = versal_sysmon_channel_get,
	.trigger_set = versal_sysmon_trigger_set,
};

#define VERSAL_SYSMON_IRQ_CONNECT_BODY(inst)                                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),                       \
			    versal_sysmon_irq_handler##inst, DEVICE_DT_INST_GET(inst),             \
			    DT_INST_IRQ(inst, flags));                                             \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

#define VERSAL_SYSMON_INTR_CONFIG(inst)                                                            \
	static void versal_sysmon_irq_handler##inst(const void *arg)                               \
	{                                                                                          \
		const struct device *dev = (const struct device *)arg;                             \
		struct sysmon_data *data = (struct sysmon_data *)dev->data;                        \
		uint32_t isr, imr;                                                                 \
		isr = sysmon_read(dev, SYSMON_ISR);                                                \
		imr = sysmon_read(dev, SYSMON_IMR);                                                \
		uint32_t pending = isr & (~imr);                                                   \
		if (pending == 0U) {                                                               \
			return;                                                                    \
		}                                                                                  \
		sysmon_write(dev, SYSMON_IDR_OFFSET, pending);                                     \
		sysmon_write(dev, SYSMON_ISR, pending);                                            \
		atomic_set(&data->pending_events, pending);                                        \
		k_work_submit(&data->events_work);                                                 \
	}                                                                                          \
	static void versal_sysmon_config_intr##inst(const struct device *dev)                      \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		VERSAL_SYSMON_IRQ_CONNECT_BODY(inst);                                              \
	}

#define SUPPLY_CHAN_DATA_INIT(node_id)                                                             \
	{                                                                                          \
		.reg = (uint8_t)DT_PROP(node_id, xlnx_register),                                   \
		.name = DT_PROP_OR(node_id, xlnx_name, "unknown"),                                 \
	},

#define VERSAL_SYSMON_INIT(inst)                                                                   \
	VERSAL_SYSMON_INTR_CONFIG(inst)                                                            \
	static struct sysmon_chan_data chans_##inst[] = {                                          \
		DT_INST_FOREACH_CHILD(inst, SUPPLY_CHAN_DATA_INIT)};                               \
                                                                                                   \
	const static struct versal_sysmon_cfg sysmon_config_##inst = {                             \
		DEVICE_MMIO_ROM_INIT(DT_INST(inst, DT_DRV_COMPAT)),                                \
		.num_channels = DT_INST_PROP(inst, xlnx_numchannels), .chan = chans_##inst,        \
		.irq_config_func = versal_sysmon_config_intr##inst};                               \
                                                                                                   \
	static struct sysmon_data sysmon_data_##inst;                                              \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, versal_sysmon_init, NULL, &sysmon_data_##inst,          \
				     &sysmon_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &versal_sysmon_api);
DT_INST_FOREACH_STATUS_OKAY(VERSAL_SYSMON_INIT)
