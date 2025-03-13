/*
 * Copyright (c) 2025 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_tach

/**
 * @file
 * @brief ITE it51xxx tachometer sensor module driver
 *
 * This file contains a driver for the tachometer sensor module which contains
 * three independent counters (T0L/MR, T1L/MR and T2L/MR). The content of the
 * Tachometer Reading Register is still update based on the sampling counter
 * that samples the tachometer input (T0A, T0B, T1A, T1B, T2A or T2B pins).
 * The following is block diagram of this module:
 *
 *                                    Sample Rate = TACH_FREQ / 128
 *                                                   |
 *                            |        Tachometer 0  |                T0A (GPD6)
 *                            |             |        | +-----------+   |
 *                            |       +-----+-----+  | |   _   _   |<--+
 *                            |------>|  T0L/MR   |<-+-|  | |_| |_ |<--+
 *                            |       +-----------+    +-----------+   |
 *                            |       capture pulses                  T0B (GPC6)
 *                            |       in sample rate
 *                            |       period
 *                            |
 *                            |        Sample Rate = TACH_FREQ / 128
 *           +-----------+    |                      |
 * Crystal-->| Prescaler |--->|        Tachometer 1  |                T1A (GPD7)
 * 32.768k   +-----------+    |             |        | +-----------+   |
 *                            |       +-----+-----+  | |   _   _   |<--+
 *                            |------>|  T1L/MR   |<-+-|  | |_| |_ |<--+
 *                            |       +-----------+    +-----------+   |
 *                            |       capture pulses                  T1B (GPJ6)
 *                            |       in sample rate
 *                            |       period
 *                            |
 *                            |        Sample Rate = TACH_FREQ / 128
 *                            |                      |
 *                            |        Tachometer 2  |                T2A (GPJ0)
 *                            |             |        | +-----------+   |
 *                            |       +-----+-----+  | |   _   _   |<--+
 *                            |------>|  T2L/MR   |<-+-|  | |_| |_ |<--+
 *                            |       +-----------+    +-----------+   |
 *                            |       capture pulses                  T2B (GPJ1)
 *                            |       in one second
 *                            |       period
 *                            |
 *
 *
 * Based on the counter value, we can compute the current RPM of external signal
 * from encoders.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/it8xxx2_tach.h>
#include <errno.h>
#include <soc.h>
#include <soc_dt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tach_ite_it51xxx, CONFIG_SENSOR_LOG_LEVEL);

/*
 * NOTE: The PWM output maximum is 324Hz in EC power saving mode, so if we need
 *       fan to work, then don't let EC enter the mode.
 */
#define TACH_FREQ		EC_FREQ

/* 0xC0/0xD0/0xE0: Tach channel low byte */
#define REG_TACH_CH_L		(0x00)
/* 0xC1/0xD1/0xE1: Tach channel high byte */
#define REG_TACH_CH_H		(0x01)
/* 0xC6/0xD6/0xE6: Tach channel control 1 */
#define REG_TACH_CH_CTRL1	(0x06)
#define TACH_CH_DVS		BIT(1)
#define TACH_CH_SEL		BIT(0)

struct tach_it51xxx_config {
	/* Tach channel register base address */
	uintptr_t base;
	/* Tachometer alternate configuration */
	const struct pinctrl_dev_config *pcfg;
	/* Select channel of tachometer */
	int channel;
	/* Number of pulses per round of tachometer's input */
	int pulses_per_round;
};

struct tach_it51xxx_data {
	/* Captured counts of tachometer */
	uint32_t capture;
};

static bool tach_ch_is_valid(const struct device *dev, int tach_ch)
{
	const struct tach_it51xxx_config *const config = dev->config;
	const uintptr_t base = config->base;
	bool valid = false;
	uint8_t reg_val;

	switch (tach_ch) {
	case IT8XXX2_TACH_CHANNEL_A:
		reg_val = sys_read8(base + REG_TACH_CH_CTRL1);
		if ((reg_val & TACH_CH_DVS) &&
		    ((reg_val & TACH_CH_SEL) == IT8XXX2_TACH_CHANNEL_A)) {
			valid = true;
		}
		break;
	case IT8XXX2_TACH_CHANNEL_B:
		reg_val = sys_read8(base + REG_TACH_CH_CTRL1);
		if ((reg_val & TACH_CH_DVS) &&
		    ((reg_val & TACH_CH_SEL) == IT8XXX2_TACH_CHANNEL_B)) {
			valid = true;
		}
		break;
	default:
		break;
	}

	return valid;
}

static int tach_it51xxx_sample_fetch(const struct device *dev,
				     enum sensor_channel chan)
{
	const struct tach_it51xxx_config *const config = dev->config;
	const uintptr_t base = config->base;
	int tach_ch = config->channel;
	struct tach_it51xxx_data *const data = dev->data;
	uint8_t reg_val;

	if ((chan != SENSOR_CHAN_RPM) && (chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	if (tach_ch_is_valid(dev, tach_ch)) {
		/* If channel data of tachometer is valid, then save it */
		data->capture = sys_read16(base + REG_TACH_CH_L);
		/* Clear tachometer data valid status */
		reg_val = sys_read8(base + REG_TACH_CH_CTRL1);
		sys_write8(reg_val | TACH_CH_DVS, base + REG_TACH_CH_CTRL1);
	} else {
		/* If channel data of tachometer isn't valid, then clear it */
		data->capture = 0;
	}

	return 0;
}

static int tach_it51xxx_channel_get(const struct device *dev,
				    enum sensor_channel chan,
				    struct sensor_value *val)
{
	const struct tach_it51xxx_config *const config = dev->config;
	int p = config->pulses_per_round;
	struct tach_it51xxx_data *const data = dev->data;

	if (chan != SENSOR_CHAN_RPM) {
		LOG_ERR("Sensor chan %d, only support SENSOR_CHAN_RPM", chan);
		return -ENOTSUP;
	}

	/* Transform count unit to RPM */
	if (data->capture > 0) {
		/*
		 * Fan Speed (RPM) = 60 / (1/fs * {TACH_CH_(H & L)} * P)
		 * - P denotes the numbers of pulses per round
		 * - {TACH_CH_(H & L)} = 0000h denotes Fan Speed is zero
		 * - The sampling rate (fs) is TACH_FREQ / 128
		 */
		val->val1 = (60 * TACH_FREQ / 128 / p / (data->capture));
	} else {
		val->val1 = 0U;
	}

	val->val2 = 0U;

	return 0;
}

static int tach_it51xxx_init(const struct device *dev)
{
	const struct tach_it51xxx_config *const config = dev->config;
	const uintptr_t base = config->base;
	int tach_ch = config->channel;
	int status;
	uint8_t reg_val;

	if (tach_ch > IT8XXX2_TACH_CHANNEL_B) {
		LOG_ERR("Tach channel %d invalid, only support 0(A) or 1(B)", tach_ch);
		return -EINVAL;
	}

	/* Select pin to alternate mode for tachometer */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure TACH pins");
		return status;
	}

	if (tach_ch == IT8XXX2_TACH_CHANNEL_A) {
		/* Select TACH_CHANNEL_A output to tachometer */
		reg_val = sys_read8(base + REG_TACH_CH_CTRL1);
		reg_val &= ~TACH_CH_SEL;
		/* Clear tachometer data valid status */
		sys_write8(reg_val | TACH_CH_DVS, base + REG_TACH_CH_CTRL1);
	} else {
		/* Select TACH_CHANNEL_B output to tachometer */
		reg_val = sys_read8(base + REG_TACH_CH_CTRL1);
		reg_val |= TACH_CH_SEL;
		/* Clear tachometer data valid status */
		sys_write8(reg_val | TACH_CH_DVS, base + REG_TACH_CH_CTRL1);
	}

	/* Tachometer sensor already start */
	return 0;
}

static DEVICE_API(sensor, tach_it51xxx_driver_api) = {
	.sample_fetch = tach_it51xxx_sample_fetch,
	.channel_get = tach_it51xxx_channel_get,
};

#define TACH_IT51XXX_INIT(inst)						       \
	PINCTRL_DT_INST_DEFINE(inst);					       \
									       \
	static const struct tach_it51xxx_config tach_it51xxx_cfg_##inst = {    \
		.base = DT_INST_REG_ADDR(inst),				       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		       \
		.channel = DT_INST_PROP(inst, channel),			       \
		.pulses_per_round = DT_INST_PROP(inst, pulses_per_round),      \
	};								       \
									       \
	static struct tach_it51xxx_data tach_it51xxx_data_##inst;	       \
									       \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				       \
			      tach_it51xxx_init,			       \
			      NULL,					       \
			      &tach_it51xxx_data_##inst,		       \
			      &tach_it51xxx_cfg_##inst,			       \
			      POST_KERNEL,				       \
			      CONFIG_SENSOR_INIT_PRIORITY,		       \
			      &tach_it51xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TACH_IT51XXX_INIT)
