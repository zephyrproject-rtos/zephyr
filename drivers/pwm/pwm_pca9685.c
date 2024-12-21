/*
 * Copyright (c) 2022 Nick Ward <nix.ward@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca9685_pwm

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_pca9685, CONFIG_PWM_LOG_LEVEL);

#define OSC_CLOCK_MAX	  50000000
#define CHANNEL_CNT	  16

#define ADDR_MODE1	  0x00
#define SLEEP		  BIT(4)
#define AUTO_INC	  BIT(5)
#define RESTART		  BIT(7)

#define ADDR_MODE2	  0x01
#define OUTDRV_TOTEM_POLE BIT(2)
#define OCH_ON_ACK	  BIT(3)

#define ADDR_LED0_ON_L	  0x06
#define ADDR_LED_ON_L(n)  (ADDR_LED0_ON_L + (4 * n))
#define LED_FULL_ON	  BIT(4)
#define LED_FULL_OFF	  BIT(4)
#define BYTE_N(word, n)	  ((word >> (8 * n)) & 0xFF)

#define ADDR_PRE_SCALE	  0xFE


/* See PCA9585 datasheet Rev. 4 - 16 April 2015 section 7.3.5 */
#define OSC_CLOCK_INTERNAL 25000000

#define PWM_STEPS 4096

#define PRE_SCALE_MIN 0x03
#define PRE_SCALE_MAX 0xFF

#define PRE_SCALE_DEFAULT 0x1E

#define PWM_PERIOD_COUNT_PS(pre_scale) (PWM_STEPS * (pre_scale + 1))

/*
 * When using the internal oscillator this corresponds to an
 * update rate of 1526 Hz
 */
#define PWM_PERIOD_COUNT_MIN PWM_PERIOD_COUNT_PS(PRE_SCALE_MIN)

/*
 * When using the internal oscillator this corresponds to an
 * update rate of 24 Hz
 */
#define PWM_PERIOD_COUNT_MAX PWM_PERIOD_COUNT_PS(PRE_SCALE_MAX)

/*
 * When using the internal oscillator this corresponds to an
 * update rate of 200 Hz
 */
#define PWM_PERIOD_COUNT_DEFAULT PWM_PERIOD_COUNT_PS(PRE_SCALE_DEFAULT)


/*
 * Time allowed for the oscillator to stabilize after the PCA9685's
 * RESTART mode is used to wake the chip from SLEEP.
 * See PCA9685 datasheet section 7.3.1.1
 */
#define OSCILLATOR_STABILIZE K_USEC(500)

struct pca9685_config {
	struct i2c_dt_spec i2c;
	bool outdrv_open_drain;
	bool och_on_ack;
	bool invrt;
};

struct pca9685_data {
	struct k_mutex mutex;
	uint8_t pre_scale;
};

static int set_reg(const struct device *dev, uint8_t addr, uint8_t value)
{
	const struct pca9685_config *config = dev->config;
	uint8_t buf[] = {addr, value};
	int ret;

	ret = i2c_write_dt(&config->i2c, buf, sizeof(buf));
	if (ret != 0) {
		LOG_ERR("I2C write [0x%02X]=0x%02X: %d", addr, value, ret);
	} else {
		LOG_DBG("[0x%02X]=0x%02X", addr, value);
	}
	return ret;
}

static int get_reg(const struct device *dev, uint8_t addr, uint8_t *value)
{
	const struct pca9685_config *config = dev->config;
	int ret;

	ret = i2c_write_read_dt(&config->i2c, &addr, sizeof(addr), value,
				sizeof(*value));
	if (ret != 0) {
		LOG_ERR("I2C write [0x%02X]=0x%02X: %d", addr, *value, ret);
	}
	return ret;
}

static int set_pre_scale(const struct device *dev, uint8_t value)
{
	struct pca9685_data *data = dev->data;
	uint8_t mode1;
	int ret;
	uint8_t restart = RESTART;

	k_mutex_lock(&data->mutex, K_FOREVER);

	/* Unblock write to PRE_SCALE by setting SLEEP bit to logic 1 */
	ret = set_reg(dev, ADDR_MODE1, AUTO_INC | SLEEP);
	if (ret != 0) {
		goto out;
	}

	ret = get_reg(dev, ADDR_MODE1, &mode1);
	if (ret != 0) {
		goto out;
	}

	if ((mode1 & RESTART) == 0x00) {
		restart = 0;
	}

	ret = set_reg(dev, ADDR_PRE_SCALE, value);
	if (ret != 0) {
		goto out;
	}

	/* Clear SLEEP bit - See datasheet section 7.3.1.1 */
	ret = set_reg(dev, ADDR_MODE1, AUTO_INC);
	if (ret != 0) {
		goto out;
	}

	k_sleep(OSCILLATOR_STABILIZE);

	ret = set_reg(dev, ADDR_MODE1, AUTO_INC | restart);
	if (ret != 0) {
		goto out;
	}

out:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int pca9685_set_cycles(const struct device *dev,
			      uint32_t channel, uint32_t period_count,
			      uint32_t pulse_count, pwm_flags_t flags)
{
	const struct pca9685_config *config = dev->config;
	struct pca9685_data *data = dev->data;
	uint8_t buf[5] = { 0 };
	uint32_t led_off_count;
	int32_t pre_scale;
	int ret;

	ARG_UNUSED(flags);

	if (channel >= CHANNEL_CNT) {
		LOG_WRN("channel out of range: %u", channel);
		return -EINVAL;
	}

	pre_scale = DIV_ROUND_UP((int64_t)period_count, PWM_STEPS) - 1;

	if (pre_scale < PRE_SCALE_MIN) {
		LOG_ERR("period_count %u < %u (min)", period_count,
			PWM_PERIOD_COUNT_MIN);
		return -ENOTSUP;
	} else if (pre_scale > PRE_SCALE_MAX) {
		LOG_ERR("period_count %u > %u (max)", period_count,
			PWM_PERIOD_COUNT_MAX);
		return -ENOTSUP;
	}

	/* Only one output frequency - simple conversion to equivalent PWM */
	if (pre_scale != data->pre_scale) {
		data->pre_scale = pre_scale;
		ret = set_pre_scale(dev, pre_scale);
		if (ret != 0) {
			return ret;
		}
	}

	/* Adjust PWM output for the resolution of the PCA9685 */
	led_off_count = DIV_ROUND_UP(pulse_count * PWM_STEPS, period_count);

	buf[0] = ADDR_LED_ON_L(channel);
	if (led_off_count == 0) {
		buf[4] = LED_FULL_OFF;
	} else if (led_off_count < PWM_STEPS) {
		/* LED turns on at count 0 */
		buf[3] = BYTE_N(led_off_count, 0);
		buf[4] = BYTE_N(led_off_count, 1);
	} else {
		buf[2] = LED_FULL_ON;
	}

	return i2c_write_dt(&config->i2c, buf, sizeof(buf));
}

static int pca9685_get_cycles_per_sec(const struct device *dev,
				      uint32_t channel, uint64_t *cycles)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	*cycles = OSC_CLOCK_INTERNAL;

	return 0;
}

static DEVICE_API(pwm, pca9685_api) = {
	.set_cycles = pca9685_set_cycles,
	.get_cycles_per_sec = pca9685_get_cycles_per_sec,
};

static int pca9685_init(const struct device *dev)
{
	const struct pca9685_config *config = dev->config;
	struct pca9685_data *data = dev->data;
	uint8_t value;
	int ret;

	(void)k_mutex_init(&data->mutex);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	ret = set_reg(dev, ADDR_MODE1, AUTO_INC);
	if (ret != 0) {
		LOG_ERR("set_reg MODE1: %d", ret);
		return ret;
	}

	value = 0x00;
	if (!config->outdrv_open_drain) {
		value |= OUTDRV_TOTEM_POLE;
	}
	if (config->och_on_ack) {
		value |= OCH_ON_ACK;
	}
	ret = set_reg(dev, ADDR_MODE2, value);
	if (ret != 0) {
		LOG_ERR("set_reg MODE2: %d", ret);
		return ret;
	}

	return 0;
}

#define PCA9685_INIT(inst)                                              \
	static const struct pca9685_config pca9685_##inst##_config = {  \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                      \
		.outdrv_open_drain = DT_INST_PROP(inst, open_drain),    \
		.och_on_ack = DT_INST_PROP(inst, och_on_ack),           \
		.invrt = DT_INST_PROP(inst, invert),                    \
	};                                                              \
                                                                        \
	static struct pca9685_data pca9685_##inst##_data;               \
                                                                        \
	DEVICE_DT_INST_DEFINE(inst, pca9685_init, NULL,                 \
			      &pca9685_##inst##_data,                   \
			      &pca9685_##inst##_config, POST_KERNEL,    \
			      CONFIG_PWM_INIT_PRIORITY,                 \
			      &pca9685_api);

DT_INST_FOREACH_STATUS_OKAY(PCA9685_INIT);
