/*
 * Copyright (c) 2022 Fraunhofer AICOS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pcf85263a

#include <time.h>

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/drivers/rtc/nxp_pcf85263a.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(PCF85263A, CONFIG_COUNTER_LOG_LEVEL);

enum nxp_pcf85263a_registers {
	PCF85263A_REG_TIME = 0x00,
	PCF85263A_REG_ALARM1 = 0x08,
	PCF85263A_REG_ALARM2 = 0x0D,
	PCF85263A_REG_ALARM_ENABLES = 0x10,
	PCF85263A_REG_CTRL_OSCILLATOR = 0x25,
	PCF85263A_REG_PIN_IO = 0x27,
	PCF85263A_REG_CTRL_FUNCTION = 0x28,
	PCF85263A_REG_INTA_ENABLE,
	PCF85263A_REG_INTB_ENABLE,
	PCF85263A_REG_FLAGS,
	PCF85263A_REG_STOP = 0x2E,
	PCF85263A_REG_RESET = 0x2F,
};

#define PCF85263A_CTRL_FUNCTION_100TH            BIT(7)
#define PCF85263A_CTRL_FUNCTION_INT_PER_SEC      BIT(5)
#define PCF85263A_CTRL_FUNCTION_INT_PER_MIN      BIT(6)
#define PCF85263A_CTRL_FUNCTION_INT_PER_H        BIT(6) || BIT(5)
#define PCF85263A_CTRL_FUNCTION_STOPWATCH_MODE   BIT(4)
#define PCF85263A_CTRL_FUNCTION_STOP_MODE_TS_PIN BIT(3)

#define PCF85263A_FLAGS_PERIODIC_INTERRUPT  BIT(7)
#define PCF85263A_FLAGS_ALARM2              BIT(6)
#define PCF85263A_FLAGS_ALARM1              BIT(5)
#define PCF85263A_FLAGS_WATCHDOG            BIT(4)
#define PCF85263A_FLAGS_BATTERY_SWITCH      BIT(3)
#define PCF85263A_FLAGS_TSTAMP_REG3         BIT(2)
#define PCF85263A_FLAGS_TSTAMP_REG2         BIT(1)
#define PCF85263A_FLAGS_TSTAMP_REG1         BIT(0)

#define PCF85263A_ALARM_ENABLE_A1_SECONDS   BIT(0)
#define PCF85263A_ALARM_ENABLE_A1_MINUTES   BIT(1)
#define PCF85263A_ALARM_ENABLE_A1_HOURS     BIT(2)
#define PCF85263A_ALARM_ENABLE_A1_DAYS      BIT(3)
#define PCF85263A_ALARM_ENABLE_A1_MONTHS    BIT(4)
#define PCF85263A_ALARM_ENABLE_A2_MINUTES   BIT(5)
#define PCF85263A_ALARM_ENABLE_A2_HOURS     BIT(2)
#define PCF85263A_ALARM_ENABLE_A2_DAYS      BIT(3)
#define PCF85263A_ALARM_ENABLE_ALARM1       0x0F
#define PCF85263A_ALARM_ENABLE_ALARM2       0x70


enum nxp_pcf85263a_mode {
	NXP_PCF85263A_MODE_RTC = 0,
	NXP_PCF85263A_MODE_STOPWATCH
};

struct nxp_pcf85263a_data {
	enum nxp_pcf85263a_mode mode;
	nxp_pcf8523a_alarm_callback_t alarm_callbacks[2];
	void *alarm_user_data[2];
	struct gpio_callback int_cb;
	const struct device *dev;
	struct k_work interrupt_worker;
};

struct nxp_pcf85263a_config {
	struct counter_config_info generic;
	const struct i2c_dt_spec i2c;
	const struct gpio_dt_spec inta_gpio;
	const struct gpio_dt_spec ts_gpio;
};

static int configure_inta_pin(const struct device *dev)
{
	int rc = 0;
	const struct nxp_pcf85263a_config *cfg = dev->config;
	uint8_t intapm_value;

#ifdef CONFIG_NXP_PCF85263A_INTA_CLK_OUT
	intapm_value = 0x00;
#elif CONFIG_NXP_PCF85263A_INTA_BATT_OUT
	intapm_value = 0x01;
#elif CONFIG_NXP_PCF85263A_INTA_INT_OUT
	intapm_value = 0x02;
#elif CONFIG_NXP_PCF85263A_INTA_HIZ
	intapm_value = 0x03;
#endif

	rc = i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_PIN_IO,
	BIT(1) | BIT(0), intapm_value);

	return rc;
}

static int configure_ts_pin(const struct device *dev)
{
	int rc = 0;
	const struct nxp_pcf85263a_config *cfg = dev->config;
	uint8_t tspm_value = 0;

#ifdef CONFIG_NXP_PCF85263A_TS_DISABLED
	tspm_value = 0x00;
#elif CONFIG_NXP_PCF85263A_TS_INTB_OUT
	tspm_value = 0x01;
#elif CONFIG_NXP_PCF85263A_TS_CLK_OUT
	tspm_value = 0x02;
#elif CONFIG_NXP_PCF85263A_TS_INPUT
	tspm_value = 0x03;
#endif

	rc = i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_PIN_IO,
	BIT(3) | BIT(2), (tspm_value << 2));

	return rc;
}

#if defined(CONFIG_NXP_PCF85263A_INTA_INT_OUT) || defined(CONFIG_NXP_PCF85263A_TS_INTB_OUT)
static void nxp_pcf85263a_int_callback(const struct device *port, struct gpio_callback *cb,
	uint32_t pin)
{
	struct nxp_pcf85263a_data *data = CONTAINER_OF(cb, struct nxp_pcf85263a_data, int_cb);

	k_work_submit(&data->interrupt_worker);
}

static void nxp_pcf85263a_interrupt_worker(struct k_work *work)
{
	struct nxp_pcf85263a_data * const data = CONTAINER_OF(
		work, struct nxp_pcf85263a_data, interrupt_worker);
	const struct nxp_pcf85263a_config *cfg = (struct nxp_pcf85263a_config *)data->dev->config;
	uint8_t flags;
	uint64_t val = 0;

	i2c_reg_read_byte_dt(&cfg->i2c, PCF85263A_REG_FLAGS, &flags);
	nxp_pcf85263a_get_value(data->dev, &val);
	if (flags & PCF85263A_FLAGS_ALARM1) {
		data->alarm_callbacks[0](data->dev, 1, (uint32_t)val, data->alarm_user_data[0]);
		i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_FLAGS, PCF85263A_FLAGS_ALARM1,
			0x00);
	} else if (flags & PCF85263A_FLAGS_ALARM2) {
		data->alarm_callbacks[1](data->dev, 2, (uint32_t)val, data->alarm_user_data[1]);
		i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_FLAGS, PCF85263A_FLAGS_ALARM2,
			0x00);
	}

}
#endif

int nxp_pcf85263a_init(const struct device *dev)
{
	int rc = 0;
	uint8_t read_byte;
	struct nxp_pcf85263a_data *data = dev->data;
	const struct nxp_pcf85263a_config *cfg = dev->config;

	data->dev = dev;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	rc = i2c_reg_read_byte_dt(&cfg->i2c, PCF85263A_REG_CTRL_FUNCTION, &read_byte);
	if (rc < 0)
		return rc;

	if (read_byte & PCF85263A_CTRL_FUNCTION_STOPWATCH_MODE) {
		data->mode = NXP_PCF85263A_MODE_STOPWATCH;
	} else {
		data->mode = NXP_PCF85263A_MODE_RTC;
	}

	rc = configure_inta_pin(dev);
	if (rc != 0) {
		return rc;
	};

	rc = configure_ts_pin(dev);
	if (rc != 0) {
		return rc;
	};

	if (device_is_ready(cfg->inta_gpio.port)) {
		rc = gpio_pin_configure_dt(&cfg->inta_gpio, GPIO_INPUT);
		if (rc < 0) {
			return rc;
		}
	}

#if defined(CONFIG_NXP_PCF85263A_INTA_INT_OUT) || defined(CONFIG_NXP_PCF85263A_TS_INTB_OUT)
	k_work_init(&data->interrupt_worker, nxp_pcf85263a_interrupt_worker);
#endif

#if defined(CONFIG_NXP_PCF85263A_TS_INTB_OUT) || defined(CONFIG_NXP_PCF85263A_TS_CLK_OUT)
	if (device_is_ready(cfg->ts_gpio.port)) {
		rc = gpio_pin_configure_dt(&cfg->ts_gpio, GPIO_INPUT);
		if (rc < 0) {
			return rc;
		}
	}
#else
	if (device_is_ready(cfg->ts_gpio.port)) {
		rc = gpio_pin_configure_dt(&cfg->ts_gpio, GPIO_OUTPUT);
		if (rc < 0) {
			return rc;
		}
	}
#endif

	return rc;
}

int nxp_pcf85263a_get_value(const struct device *dev, uint64_t *value)
{
	int rc = 0;
	struct nxp_pcf85263a_data *data = dev->data;
	const struct nxp_pcf85263a_config *cfg = dev->config;
	uint8_t len = 0;
	uint8_t time_registers[8];
	static struct tm time = { 0 };

	if (data->mode == NXP_PCF85263A_MODE_RTC) {
		len = 8;
	} else {
		len = 6;
	}

	rc = i2c_burst_read_dt(&cfg->i2c, PCF85263A_REG_TIME, &time_registers[0], len);
	if (rc < 0) {
		return rc;
	};

	if (data->mode == NXP_PCF85263A_MODE_RTC) {
		time.tm_sec  = bcd2bin(time_registers[1]  & 0x7F);
		time.tm_min  = bcd2bin(time_registers[2]  & 0x7F);
		time.tm_hour = bcd2bin(time_registers[3]  & 0x3F);
		time.tm_mday = bcd2bin(time_registers[4]  & 0x3F);
		time.tm_wday = bcd2bin(time_registers[5]  & 0x07);
		time.tm_mon  = bcd2bin(time_registers[6]  & 0x1F) - 1;
		time.tm_year = bcd2bin(time_registers[7]) + 70;
		*value = timeutil_timegm(&time);
	} else {
		/* TODO: Implement stop-watch bcd2bin translation */
		LOG_ERR("Stop-watch mode is not implemented.");
		return -ENOSYS;
	}

	return rc;
}

int nxp_pcf85263a_start(const struct device *dev)
{
	int rc = 0;
	const struct nxp_pcf85263a_config *cfg = dev->config;

	rc = i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_STOP, BIT(0), 0x00);

	return rc;
}

int nxp_pcf85263a_stop(const struct device *dev)
{
	int rc = 0;
	const struct nxp_pcf85263a_config *cfg = dev->config;

	rc = i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_STOP, BIT(0), 0x01);

	return rc;
}

static int nxp_pcf85263a_clear_prescaler(const struct device *dev)
{
	int rc = 0;
	const struct nxp_pcf85263a_config *cfg = dev->config;

	rc = i2c_reg_write_byte_dt(&cfg->i2c, PCF85263A_REG_RESET, 0xA4);

	return rc;
}

int nxp_pcf85263a_set_value(const struct device *dev, uint64_t value)
{
	int rc = 0;
	struct nxp_pcf85263a_data *data = dev->data;
	const struct nxp_pcf85263a_config *cfg = dev->config;
	uint8_t len = 0;
	uint8_t time_registers[8];
	struct tm calendar_time;

	if (data->mode == NXP_PCF85263A_MODE_RTC) {
		len = 8;
		(void)gmtime_r((time_t *)&value, &calendar_time);
		time_registers[0] = 0;
		time_registers[1] = bin2bcd(calendar_time.tm_sec);
		time_registers[2] = bin2bcd(calendar_time.tm_min);
		time_registers[3] = bin2bcd(calendar_time.tm_hour);
		time_registers[4] = bin2bcd(calendar_time.tm_mday);
		time_registers[5] = bin2bcd(calendar_time.tm_wday);
		time_registers[6] = bin2bcd(calendar_time.tm_mon + 1);
		time_registers[7] = bin2bcd(calendar_time.tm_year-70);
	} else {
		len = 6;
		/* TODO: Implement stop-watch bin2bcd translation */
		LOG_ERR("Stop-watch mode is not implemented.");
		return -ENOSYS;
	}
	rc = nxp_pcf85263a_stop(dev);
	if (rc < 0) {
		return rc;
	};
	rc = nxp_pcf85263a_clear_prescaler(dev);
	if (rc < 0) {
		return rc;
	};
	rc = i2c_burst_write_dt(&cfg->i2c, PCF85263A_REG_TIME, &time_registers[0], len);
	if (rc < 0) {
		return rc;
	}
	rc = nxp_pcf85263a_start(dev);
	if (rc < 0) {
		return rc;
	};
	return rc;
}

int nxp_pcf85263a_set_alarm(const struct device *dev, uint8_t id,
	const struct nxp_pcf85263a_alarm_cfg *alarm_cfg)
{
	int rc = 0;
	struct nxp_pcf85263a_data *data = dev->data;
	const struct nxp_pcf85263a_config *cfg = dev->config;
	uint8_t addr, len;
	struct tm calendar_time;
	uint8_t time_registers[5];

	if (id == 1) {
		addr = PCF85263A_REG_ALARM1;
		len = 5;
		rc = i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_ALARM_ENABLES,
			PCF85263A_ALARM_ENABLE_ALARM1, 0x00);
		if (rc < 0) {
			return rc;
		};
	} else if (id == 2) {
		addr = PCF85263A_REG_ALARM2;
		len = 3;
		rc = i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_ALARM_ENABLES,
			PCF85263A_ALARM_ENABLE_ALARM2, 0x00);
		if (rc < 0) {
			return rc;
		};
	} else {
		return -EINVAL;
	}

	(void)gmtime_r(&alarm_cfg->time, &tm);

	if (id == 1) {
		time_registers[0] = bin2bcd(calendar_time.tm_sec);
		time_registers[1] = bin2bcd(calendar_time.tm_min);
		time_registers[2] = bin2bcd(calendar_time.tm_hour);
		time_registers[3] = bin2bcd(calendar_time.tm_mday);
		time_registers[4] = bin2bcd(calendar_time.tm_mon+1);
	} else if (id == 2) {
		time_registers[0] = bin2bcd(calendar_time.tm_min);
		time_registers[1] = bin2bcd(calendar_time.tm_hour);
		time_registers[2] = bin2bcd(calendar_time.tm_wday);
	}

	rc = i2c_burst_write_dt(&cfg->i2c, addr, time_registers, len);
	if (rc < 0) {
		return rc;
	};

	data->alarm_callbacks[id-1] = alarm_cfg->callback;
	data->alarm_user_data[id-1] = alarm_cfg->user_data;

	#if CONFIG_NXP_PCF85263A_INTA_INT_OUT
		if (alarm_cfg->flags & PCF85263A_ALARM_FLAGS_USE_INTA) {
			if (cfg->inta_gpio.port == NULL) {
				LOG_ERR("INTA pin not found.");
				return -EINVAL;
			}
			rc = gpio_pin_interrupt_configure_dt(&cfg->inta_gpio,
				GPIO_INT_EDGE_TO_ACTIVE);
			if (rc < 0) {
				return rc;
			};
			gpio_init_callback(&data->int_cb, nxp_pcf85263a_int_callback,
				BIT(cfg->inta_gpio.pin));
			rc = gpio_add_callback(cfg->inta_gpio.port, &data->int_cb);
			if (rc < 0) {
				return rc;
			};
			if (id == 1)
				rc = i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_INTA_ENABLE,
					BIT(4)|BIT(3), (0x02 << 3));
			if (id == 2)
				rc = i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_INTA_ENABLE,
					BIT(4)|BIT(3), (0x01 << 3));
			if (rc < 0)
				return rc;
		}
	#elif CONFIG_NXP_PCF85263A_TS_INTB_OUT
		if (alarm_cfg->flags & PCF85263A_ALARM_FLAGS_USE_INTB) {
			if (cfg->ts_gpio.port == NULL) {
				LOG_ERR("TS pin not found.");
				return -EINVAL;
			}
			rc = gpio_pin_interrupt_configure_dt(&cfg->ts_gpio,
				GPIO_INT_EDGE_TO_ACTIVE);
			if (rc < 0)
				return rc;
			gpio_init_callback(&data->int_cb, nxp_pcf85263a_int_callback,
				BIT(cfg->ts_gpio.pin));
			rc = gpio_add_callback(cfg->ts_gpio.port, &data->int_cb);
			if (rc < 0) {
				return rc;
			};
			if (id == 1)
				rc = i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_INTB_ENABLE,
					BIT(4)|BIT(3), (0x02 << 3));
			if (id == 2)
				rc = i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_INTB_ENABLE,
					BIT(4)|BIT(3), (0x01 << 3));
			if (rc < 0)
				return rc;
		}
	#else
	#warning "NXP_PXF85263A: No interrupt pin (INTA or INTB) configured."
	#endif

	if (id == 1) {
		i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_ALARM_ENABLES,
			PCF85263A_ALARM_ENABLE_ALARM1, 0x0F);
	}
	if (id == 2) {
		i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_ALARM_ENABLES,
			PCF85263A_ALARM_ENABLE_ALARM2, 0x70);
	}

	return rc;
}

int nxp_pcf85263a_cancel_alarm(const struct device *dev, uint8_t id)
{
	int rc = 0;
	struct nxp_pcf85263a_data *data = dev->data;
	const struct nxp_pcf85263a_config *cfg = dev->config;
	uint8_t reg;

	if (id == 1) {
		reg = PCF85263A_ALARM_ENABLE_ALARM1;
	} else if (id == 2) {
		reg = PCF85263A_ALARM_ENABLE_ALARM2;
	} else {
		return -EINVAL;
	}

	data->alarm_callbacks[id-1] = NULL;
	data->alarm_user_data[id-1] = NULL;

	rc = i2c_reg_update_byte_dt(&cfg->i2c, PCF85263A_REG_ALARM_ENABLES, reg, 0x00);
	return rc;
}

static inline int pcf85263a_counter_start(const struct device *dev)
{
	return nxp_pcf85263a_start(dev);
}

static inline int pcf85263a_counter_stop(const struct device *dev)
{
	return nxp_pcf85263a_stop(dev);
}

static int pcf85263a_counter_get_value(const struct device *dev, uint32_t *ticks)
{
	int rc = 0;
	uint64_t value;

	rc = nxp_pcf85263a_get_value(dev, &value);
	if (rc == 0)
		*ticks = (uint32_t)value;
	return rc;
}

static int pcf85263a_counter_set_alarm(const struct device *dev,
				 uint8_t id,
				 const struct counter_alarm_cfg *alarm_cfg)
{
	int rc = 0;
	const struct nxp_pcf85263a_config *dev_cfg = dev->config;
	struct nxp_pcf85263a_alarm_cfg nxp_cfg;

	nxp_cfg.time = (time_t)alarm_cfg->ticks;
	nxp_cfg.callback = (counter_alarm_callback_t)alarm_cfg->callback;
	nxp_cfg.user_data = alarm_cfg->user_data;

	if (device_is_ready(dev_cfg->inta_gpio.port)) {
		nxp_cfg.flags = PCF85263A_ALARM_FLAGS_USE_INTA;
	} else if (device_is_ready(dev_cfg->ts_gpio.port)) {
		nxp_cfg.flags = PCF85263A_ALARM_FLAGS_USE_INTB;
	}

	rc = nxp_pcf85263a_set_alarm(dev, id, &nxp_cfg);

	return rc;
}

static inline int pcf85263a_counter_cancel_alarm(const struct device *dev, uint8_t id)
{
	return nxp_pcf85263a_cancel_alarm(dev, id);
}

static const struct counter_driver_api pcf85263a_api = {
	.start = pcf85263a_counter_start,
	.stop = pcf85263a_counter_stop,
	.get_value = pcf85263a_counter_get_value,
	.set_alarm = pcf85263a_counter_set_alarm,
	.cancel_alarm = pcf85263a_counter_cancel_alarm,
};

#define INST_DT_PCF85263A(index)							\
											\
	static struct nxp_pcf85263a_data pcf85263a_data_##index;			\
											\
	static const struct nxp_pcf85263a_config pcf85263a_config_##index = {		\
		.generic = {								\
			.max_top_value = UINT32_MAX,					\
			.freq = 1,							\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,				\
			.channels = 2,							\
		},									\
		.i2c = I2C_DT_SPEC_INST_GET(index),					\
		.inta_gpio = GPIO_DT_SPEC_INST_GET_OR(index, inta_gpios, {0}),		\
		.ts_gpio = GPIO_DT_SPEC_INST_GET_OR(index, ts_gpios, {0}),		\
	};										\
											\
	DEVICE_DT_INST_DEFINE(index, nxp_pcf85263a_init, NULL, &pcf85263a_data_##index, \
		&pcf85263a_config_##index, POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,	\
		&pcf85263a_api);

DT_INST_FOREACH_STATUS_OKAY(INST_DT_PCF85263A);
