/*
 * Copyright (c) 2021 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_tach

/**
 * @file
 * @brief ITE it8xxx2 tachometer sensor module driver
 *
 * This file contains a driver for the tachometer sensor module which contains
 * two independent counters (F1TL/MRR and F2TL/MRR). The content of the
 * Tachometer Reading Register is still update based on the sampling counter
 * that samples the tachometer input (T0A, T0B, T1A or T1B pins).
 * The following is block diagram of this module:
 *
 *                                    Sample Rate = TACH_FREQ / 128
 *                                                   |
 *                            |        Tachometer 0  |                T0A (GPD6)
 *                            |             |        | +-----------+   |
 *                            |       +-----+-----+  | |   _   _   |<--+
 *                            |------>| F1TL/MRR  |<-+-|  | |_| |_ |<--+
 *                            |       +-----------+    +-----------+   |
 *                            |       capture pulses                  T0B (GPJ2)
 *                            |       in sample rate
 *                            |       period
 *           +-----------+    |
 * Crystal-->| Prescaler |--->|        Tachometer 1                   T1A (GPD7)
 * 32.768k   +-----------+    |             |          +-----------+   |
 *                            |       +-----+-----+    |   _   _   |<--+
 *                            |------>| F2TL/MRR  |<-+-|  | |_| |_ |<--+
 *                            |       +-----------+    +-----------+   |
 *                            |       capture pulses                  T1B (GPJ3)
 *                            |       in one second
 *                            |       period
 *                            |
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
LOG_MODULE_REGISTER(tach_ite_it8xxx2, CONFIG_SENSOR_LOG_LEVEL);

/*
 * NOTE: The PWM output maximum is 324Hz in EC LPM, so if we need fan to work
 *       then don't let EC enter LPM.
 */
#define TACH_FREQ EC_FREQ

struct tach_it8xxx2_config {
	/* Fan x tachometer LSB reading register */
	uintptr_t reg_fxtlrr;
	/* Fan x tachometer MSB reading register */
	uintptr_t reg_fxtmrr;
	/* Tachometer switch control register */
	uintptr_t reg_tswctlr;
	/* Tachometer data valid bit of tswctlr register */
	int dvs_bit;
	/* Tachometer data valid status bit of tswctlr register */
	int chsel_bit;
	/* Tachometer alternate configuration */
	const struct pinctrl_dev_config *pcfg;
	/* Select channel of tachometer */
	int channel;
	/* Number of pulses per round of tachometer's input */
	int pulses_per_round;
};

/* Driver data */
struct tach_it8xxx2_data {
	/* Captured counts of tachometer */
	uint32_t capture;
};

static bool tach_ch_is_valid(const struct device *dev, int tach_ch)
{
	const struct tach_it8xxx2_config *const config = dev->config;
	volatile uint8_t *reg_tswctlr = (uint8_t *)config->reg_tswctlr;
	int dvs_bit = config->dvs_bit;
	int chsel_bit = config->chsel_bit;
	int mask = (dvs_bit | chsel_bit);
	bool valid = false;

	switch (tach_ch) {
	case IT8XXX2_TACH_CHANNEL_A:
		if ((*reg_tswctlr & mask) == dvs_bit) {
			valid = true;
		}
		break;
	case IT8XXX2_TACH_CHANNEL_B:
		if ((*reg_tswctlr & mask) == mask) {
			valid = true;
		}
		break;
	default:
		break;
	}

	return valid;
}

static int tach_it8xxx2_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tach_it8xxx2_config *const config = dev->config;
	volatile uint8_t *reg_fxtlrr = (uint8_t *)config->reg_fxtlrr;
	volatile uint8_t *reg_fxtmrr = (uint8_t *)config->reg_fxtmrr;
	volatile uint8_t *reg_tswctlr = (uint8_t *)config->reg_tswctlr;
	int tach_ch = config->channel;
	struct tach_it8xxx2_data *const data = dev->data;

	if ((chan != SENSOR_CHAN_RPM) && (chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	if (tach_ch_is_valid(dev, tach_ch)) {
		/* If channel data of tachometer is valid, then save it */
		data->capture = ((*reg_fxtmrr) << 8) | (*reg_fxtlrr);

		if (config->dvs_bit == IT8XXX2_PWM_T0DVS) {
			/* Only W/C tach 0 data valid status */
			*reg_tswctlr = (*reg_tswctlr & ~IT8XXX2_PWM_T1DVS);
		} else {
			/* Only W/C tach 1 data valid status */
			*reg_tswctlr = (*reg_tswctlr & ~IT8XXX2_PWM_T0DVS);
		}
	} else {
		/* If channel data of tachometer isn't valid, then clear it */
		data->capture = 0;
	}

	return 0;
}

static int tach_it8xxx2_channel_get(const struct device *dev, enum sensor_channel chan,
				    struct sensor_value *val)
{
	const struct tach_it8xxx2_config *const config = dev->config;
	int tachx = ((config->dvs_bit) == IT8XXX2_PWM_T0DVS) ? 0 : 1;
	int p = config->pulses_per_round;
	struct tach_it8xxx2_data *const data = dev->data;

	if (chan != SENSOR_CHAN_RPM) {
		LOG_ERR("Sensor chan %d, only support SENSOR_CHAN_RPM", chan);
		return -ENOTSUP;
	}

	/* Transform count unit to RPM */
	if (data->capture > 0) {
		if (tachx == 0) {
			/*
			 * Fan Speed (RPM) = 60 / (1/fs * {F1TMRR, F1TLRR} * P)
			 * - P denotes the numbers of pulses per round
			 * - {F1TMRR, F1TLRR} = 0000h denotes Fan Speed is zero
			 * - The sampling rate (fs) is TACH_FREQ / 128
			 */
			val->val1 = (60 * TACH_FREQ / 128 / p / (data->capture));
		} else {
			/*
			 * Fan Speed (RPM) = {F2TMRR, F2TLRR} * 60 / P
			 * - P denotes the numbers of pulses per round
			 * - {F2TMRR, F2TLRR} = 0000h denotes Fan Speed is zero
			 */
			val->val1 = ((data->capture) * 120 / (p * 2));
		}
	} else {
		val->val1 = 0U;
	}

	val->val2 = 0U;

	return 0;
}

static int tach_it8xxx2_init(const struct device *dev)
{
	const struct tach_it8xxx2_config *const config = dev->config;
	volatile uint8_t *reg_tswctlr = (uint8_t *)config->reg_tswctlr;
	int tach_ch = config->channel;
	int status;

	if (tach_ch > IT8XXX2_TACH_CHANNEL_B) {
		LOG_ERR("Tach channel %d, only support 0 or 1", tach_ch);
		return -EINVAL;
	}

	/* Select pin to alternate mode for tachometer */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure TACH pins");
		return status;
	}

	if (tach_ch == IT8XXX2_TACH_CHANNEL_A) {
		/* Select IT8XXX2_TACH_CHANNEL_A output to tachometer */
		*reg_tswctlr &= ~(config->chsel_bit);
	} else {
		/* Select IT8XXX2_TACH_CHANNEL_B output to tachometer */
		*reg_tswctlr |= config->chsel_bit;
	}

	if (config->dvs_bit == IT8XXX2_PWM_T0DVS) {
		/* Only W/C tach 0 data valid status */
		*reg_tswctlr = (*reg_tswctlr & ~IT8XXX2_PWM_T1DVS);
	} else {
		/* Only W/C tach 1 data valid status */
		*reg_tswctlr = (*reg_tswctlr & ~IT8XXX2_PWM_T0DVS);
	}

	/* Tachometer sensor already start */
	return 0;
}

static DEVICE_API(sensor, tach_it8xxx2_driver_api) = {
	.sample_fetch = tach_it8xxx2_sample_fetch,
	.channel_get = tach_it8xxx2_channel_get,
};

#define TACH_IT8XXX2_INIT(inst)                                                                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static const struct tach_it8xxx2_config tach_it8xxx2_cfg_##inst = {                        \
		.reg_fxtlrr = DT_INST_REG_ADDR_BY_IDX(inst, 0),                                    \
		.reg_fxtmrr = DT_INST_REG_ADDR_BY_IDX(inst, 1),                                    \
		.reg_tswctlr = DT_INST_REG_ADDR_BY_IDX(inst, 2),                                   \
		.dvs_bit = DT_INST_PROP(inst, dvs_bit),                                            \
		.chsel_bit = DT_INST_PROP(inst, chsel_bit),                                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.channel = DT_INST_PROP(inst, channel),                                            \
		.pulses_per_round = DT_INST_PROP(inst, pulses_per_round),                          \
	};                                                                                         \
                                                                                                   \
	static struct tach_it8xxx2_data tach_it8xxx2_data_##inst;                                  \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tach_it8xxx2_init, NULL, &tach_it8xxx2_data_##inst,     \
				     &tach_it8xxx2_cfg_##inst, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &tach_it8xxx2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TACH_IT8XXX2_INIT)
