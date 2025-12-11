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
 *                            |       in sample rate
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
#include <zephyr/dt-bindings/sensor/it51xxx_tach.h>
#include <errno.h>
#include <soc.h>
#include <soc_dt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tach_ite_it51xxx, CONFIG_SENSOR_LOG_LEVEL);

#define TACH_FREQ IT51XXX_EC_FREQ

/* 0xC0/0xD0/0xE0: Tach channel 0~2 tachometer speed (2 bytes value) */
#define REG_TACH_CH       0x00
/* 0xC6/0xD6/0xE6: Tach channel 0~2 control 1 */
#define REG_TACH_CH_CTRL1 0x06
#define TACH_CH_DVS       BIT(1)
#define TACH_CH_SEL       BIT(0)

struct tach_it51xxx_config {
	/* Tach channel register base address */
	uintptr_t base;
	/* Tachometer alternate configuration */
	const struct pinctrl_dev_config *pcfg;
	/* Select input pin to tachometer */
	int input_pin;
	/* Number of pulses per round of tachometer's input */
	int pulses_per_round;
};

struct tach_it51xxx_data {
	/* Captured counts of tachometer */
	uint16_t capture;
};

static bool tach_ch_is_valid(const struct device *dev, int input_pin)
{
	const struct tach_it51xxx_config *const config = dev->config;
	const uintptr_t base = config->base;
	uint8_t reg_val;

	if (input_pin > IT51XXX_TACH_INPUT_PIN_B) {
		LOG_ERR("Tach input pin %d invalid, only support 0(A) or 1(B)", input_pin);
		return false;
	}

	reg_val = sys_read8(base + REG_TACH_CH_CTRL1);
	if (((reg_val & TACH_CH_SEL) == input_pin) && (reg_val & TACH_CH_DVS)) {
		/* Input pin match register setting and tachometer data valid */
		return true;
	}

	return false;
}

static int tach_it51xxx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tach_it51xxx_config *const config = dev->config;
	const uintptr_t base = config->base;
	int input_pin = config->input_pin;
	struct tach_it51xxx_data *const data = dev->data;
	uint8_t reg_val;

	if ((chan != SENSOR_CHAN_RPM) && (chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	if (tach_ch_is_valid(dev, input_pin)) {
		/* If tachometer data is valid, then save it */
		data->capture = sys_read16(base + REG_TACH_CH);
		/* Clear tachometer data valid status */
		reg_val = sys_read8(base + REG_TACH_CH_CTRL1);
		sys_write8(reg_val | TACH_CH_DVS, base + REG_TACH_CH_CTRL1);
	} else {
		/* If tachometer data isn't valid, then clear it */
		data->capture = 0;
	}

	return 0;
}

static int tach_it51xxx_channel_get(const struct device *dev, enum sensor_channel chan,
				    struct sensor_value *val)
{
	const struct tach_it51xxx_config *const config = dev->config;
	int p = config->pulses_per_round;
	struct tach_it51xxx_data *const data = dev->data;

	__ASSERT(p > 0, "pulses_per_round must be bigger than 0");

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
	int input_pin = config->input_pin;
	int status;
	uint8_t reg_val;

	if (input_pin > IT51XXX_TACH_INPUT_PIN_B) {
		LOG_ERR("Tach input pin %d invalid, only support 0(A) or 1(B)", input_pin);
		return -EINVAL;
	}

	/* Select input pin to tachometer alternate mode */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure TACH pins");
		return status;
	}

	reg_val = sys_read8(base + REG_TACH_CH_CTRL1);
	if (input_pin == IT51XXX_TACH_INPUT_PIN_A) {
		/* Select TACH_INPUT_PIN_A to tachometer */
		reg_val &= ~TACH_CH_SEL;
	} else {
		/* Select TACH_INPUT_PIN_B to tachometer */
		reg_val |= TACH_CH_SEL;
	}
	/* Clear tachometer data valid status */
	sys_write8(reg_val | TACH_CH_DVS, base + REG_TACH_CH_CTRL1);

	/* Tachometer sensor already start */
	return 0;
}

static DEVICE_API(sensor, tach_it51xxx_driver_api) = {
	.sample_fetch = tach_it51xxx_sample_fetch,
	.channel_get = tach_it51xxx_channel_get,
};

#define TACH_IT51XXX_INIT(inst)                                                                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static const struct tach_it51xxx_config tach_it51xxx_cfg_##inst = {                        \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.input_pin = DT_INST_PROP(inst, input_pin),                                        \
		.pulses_per_round = DT_INST_PROP(inst, pulses_per_round),                          \
	};                                                                                         \
                                                                                                   \
	static struct tach_it51xxx_data tach_it51xxx_data_##inst;                                  \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tach_it51xxx_init, NULL, &tach_it51xxx_data_##inst,     \
				     &tach_it51xxx_cfg_##inst, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &tach_it51xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TACH_IT51XXX_INIT)
