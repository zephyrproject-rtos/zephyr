/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2023 Linumiz
 * Author: Sri Surya  <srisurya@linumiz.com>
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT ambiq_am1805

LOG_MODULE_REGISTER(am1805, CONFIG_RTC_LOG_LEVEL);

#define AM1805_IDENTITY_CODE	0x69

/* AM1805 register address */
#define REG_HUNDREDS_ADDR	0x00
#define REG_SECONDS_ADDR	0x01
#define REG_MINUTES_ADDR	0x02
#define REG_HOURS_ADDR		0x03
#define REG_MDAY_ADDR		0x04
#define REG_MONTH_ADDR		0x05
#define REG_YEAR_ADDR		0x06
#define REG_WDAY_ADDR		0x07

#define REG_ALM_HUNDREDS_ADDR	0x08
#define REG_ALM_SECONDS_ADDR	0x09
#define REG_ALM_MINUTES_ADDR	0x0A
#define REG_ALM_HOURS_ADDR	0x0B
#define REG_ALM_MDAY_ADDR	0x0C
#define REG_ALM_MONTH_ADDR	0x0D
#define REG_ALM_WDAY_ADDR	0x0E
#define REG_STATUS_ADDR		0x0F
#define REG_CONTROL1_ADDR	0x10
#define REG_CONTROL2_ADDR	0x11
#define REG_XT_CALIB_ADDR	0x14
#define REG_TIMER_CTRL_ADDR	0x18
#define REG_IRQ_MASK_ADDR       0x12
#define REG_WATCHDOG_ADDR	0x1B
#define REG_OSC_STATUS_ADDR	0x1D

/* AM1805 control bits */
#define SECONDS_BITS		GENMASK(6, 0)
#define MINUTES_BITS		GENMASK(6, 0)
#define HOURS_BITS		GENMASK(5, 0)
#define DATE_BITS		GENMASK(5, 0)
#define MONTHS_BITS		GENMASK(4, 0)
#define WEEKDAY_BITS		GENMASK(2, 0)
#define YEAR_BITS		GENMASK(7, 0)
#define REG_CONTROL2_OUT2S_BITS GENMASK(4, 2)
#define REG_TIMER_CTRL_RPT_BITS GENMASK(4, 2)
#define REG_XT_CALIB_OFF_MASK	GENMASK(6, 0)

#define REG_STATUS_ALM		BIT(2)
#define REG_CONTROL1_STOP	BIT(7)
#define REG_IRQ_MASK_AIE        BIT(2)
#define REG_XT_CALIB_CMDX       BIT(7)

#define TIMER_CTRL_ALM_OFF	0x00
#define TIMER_CTRL_ALM_DAY	BIT(4)
#define TIMER_CTRL_ALM_YEAR	BIT(2)
#define TIMER_CTRL_ALM_HR	(BIT(2) | BIT(4))
#define TIMER_CTRL_ALM_SEC	GENMASK(4, 2)
#define TIMER_CTRL_ALM_MIN	GENMASK(4, 3)
#define TIMER_CTRL_ALM_WEEK	GENMASK(3, 2)

#define REG_WATCHDOG_WDS	BIT(7)
#define WRB_1_SECOND		BIT(1)
#define WRB_4_SECONDS		GENMASK(1, 0)

#define REG_OSC_STATUS_ACAL_0	0x00
#define REG_OSC_STATUS_ACAL_1	BIT(6)
#define REG_OSC_STATUS_ACAL_2	BIT(7)
#define REG_OSC_STATUS_ACAL_3	GENMASK(7, 6)
#define REG_OSC_STATUS_MASK	BIT(1)
#define REG_STATUS_DEFAULT	0x00

#define AM1805_RTC_ALARM_TIME_MASK                                                                 \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_WEEKDAY)

#ifdef CONFIG_RTC_ALARM
/* am1805-gpios property must be in the devicetree inorder to use the RTC_ALARM */
#if !DT_ANY_INST_HAS_PROP_STATUS_OKAY(am1805_gpios)
#error "am1805-gpios" - property not available in devicetree.
#endif
#endif

struct am1805_config {
	const struct i2c_dt_spec int_i2c;
#ifdef CONFIG_RTC_ALARM
	struct gpio_dt_spec int_gpio;
#endif
};

struct am1805_data {
	struct k_mutex lock;
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_user_callback;
	void *alarm_user_data;
	/* For gpio-interrupt */
	struct gpio_callback am1805_callback;
	struct k_thread am1805_thread;
	struct k_sem int_sem;

	K_KERNEL_STACK_MEMBER(am1805_stack, CONFIG_RTC_AM1805_THREAD_STACK_SIZE);
#endif
};

/* To set the timer registers */
static int am1805_set_time(const struct device *dev, const struct rtc_time *tm)
{
	int err;
	uint8_t regs[7];

	struct am1805_data *data = dev->data;
	const struct am1805_config *config = dev->config;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* To unlock Stop-bit */
	err = i2c_reg_update_byte_dt(&config->int_i2c, REG_CONTROL1_ADDR,
					REG_CONTROL1_STOP, REG_CONTROL1_STOP);
	if (err != 0) {
		goto unlock;
	}

	LOG_DBG("set time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
			"min = %d, sec = %d",
			tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_wday, tm->tm_hour, tm->tm_min,
			tm->tm_sec);

	regs[0] = bin2bcd(tm->tm_sec) & SECONDS_BITS;
	regs[1] = bin2bcd(tm->tm_min) & MINUTES_BITS;
	regs[2] = bin2bcd(tm->tm_hour) & HOURS_BITS;
	regs[3] = bin2bcd(tm->tm_mday) & DATE_BITS;
	regs[4] = bin2bcd(tm->tm_mon) & MONTHS_BITS;
	regs[5] = bin2bcd(tm->tm_year) & YEAR_BITS;
	regs[6] = bin2bcd(tm->tm_wday) & WEEKDAY_BITS;

	err = i2c_burst_write_dt(&config->int_i2c, REG_SECONDS_ADDR, regs, sizeof(regs));
	if (err != 0) {
		goto unlock;
	}

	/* To lock Stop-bit */
	err = i2c_reg_update_byte_dt(&config->int_i2c, REG_CONTROL1_ADDR, REG_CONTROL1_STOP, 0);

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

/* To get from the timer registers */
static int am1805_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	int err;
	uint8_t ctl_reg;
	uint8_t regs[7];

	struct am1805_data *data = dev->data;
	const struct am1805_config *config = dev->config;

	k_mutex_lock(&data->lock, K_FOREVER);

	err = i2c_reg_read_byte_dt(&config->int_i2c, REG_CONTROL1_ADDR, &ctl_reg);
	if (err != 0) {
		goto unlock;
	}

	err = ctl_reg & REG_CONTROL1_STOP;
	if (err != 0) {
		LOG_WRN("No control to get time now!!");
		goto unlock;
	}

	err = i2c_burst_read_dt(&config->int_i2c, REG_SECONDS_ADDR, regs, sizeof(regs));
	if (err != 0) {
		goto unlock;
	}

	timeptr->tm_sec = bcd2bin(regs[0] & SECONDS_BITS);
	timeptr->tm_min = bcd2bin(regs[1] & MINUTES_BITS);
	timeptr->tm_hour = bcd2bin(regs[2] & HOURS_BITS);
	timeptr->tm_mday = bcd2bin(regs[3] & DATE_BITS);
	timeptr->tm_mon = bcd2bin(regs[4] & MONTHS_BITS);
	timeptr->tm_year = bcd2bin(regs[5] & YEAR_BITS);
	timeptr->tm_wday = bcd2bin(regs[6] & WEEKDAY_BITS);

	LOG_DBG("get time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
			"min = %d, sec = %d",
			timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
			timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

#ifdef CONFIG_RTC_CALIBRATION
/* To Calibrate the XT oscillator */
static int am1805_set_calibration(const struct device *dev, int32_t xt_clock_adj)
{
	int err;
	uint8_t xt_calib_value;

	struct am1805_data *data = dev->data;
	const struct am1805_config *config = dev->config;
	uint8_t reg = REG_OSC_STATUS_MASK;

	if (xt_clock_adj < -320 || xt_clock_adj > 127) {
		LOG_DBG("Cannot be calibrated adj = %d\n", xt_clock_adj);
		return -EINVAL;
	} else if (xt_clock_adj < -256) {
		/* XTCAL=3 CMDX=1 OFFSETX=(adj+192)/2 */
		reg |= REG_OSC_STATUS_ACAL_3;
		xt_calib_value = ((uint8_t)(xt_clock_adj + 192) >> 1);
		xt_calib_value &= REG_XT_CALIB_OFF_MASK;
		xt_calib_value |= REG_XT_CALIB_CMDX;
	} else if (xt_clock_adj < -192) {
		/* XTCAL=3 CMDX=0 OFFSETX=(adj+192) */
		reg |= REG_OSC_STATUS_ACAL_3;
		xt_calib_value = (uint8_t)(xt_clock_adj + 192);
		xt_calib_value &= REG_XT_CALIB_OFF_MASK;
	} else if (xt_clock_adj < -128) {
		/* XTCAL=2 CMDX=0 OFFSETX=(adj+128) */
		reg |= REG_OSC_STATUS_ACAL_2;
		xt_calib_value = (uint8_t)(xt_clock_adj + 128);
		xt_calib_value &= REG_XT_CALIB_OFF_MASK;
	} else if (xt_clock_adj < -64) {
		/* XTCAL=1 CMDX=0 OFFSETX=(adj+64) */
		reg |= REG_OSC_STATUS_ACAL_1;
		xt_calib_value = (uint8_t)(xt_clock_adj + 64);
		xt_calib_value &= REG_XT_CALIB_OFF_MASK;
	} else if (xt_clock_adj < 64) {
		/* XTCAL=0 CMDX=0 OFFSETX=(adj) */
		reg |= REG_OSC_STATUS_ACAL_0;
		xt_calib_value = (uint8_t)(xt_clock_adj);
		xt_calib_value &= REG_XT_CALIB_OFF_MASK;
	} else if (xt_clock_adj < 128) {
		/* XTCAL=0 CMDX=1 OFFSETX=(adj)/2 */
		reg |= REG_OSC_STATUS_ACAL_0;
		xt_calib_value = ((uint8_t)(xt_clock_adj >> 1));
		xt_calib_value &= REG_XT_CALIB_OFF_MASK;
		xt_calib_value |= REG_XT_CALIB_CMDX;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	err = i2c_reg_write_byte_dt(&config->int_i2c, REG_OSC_STATUS_ADDR, reg);
	if (err != 0) {
		LOG_DBG("fail to set oscillator status register\n");
		goto unlock;
	}

	/* Calibration XT Register */
	reg = xt_calib_value;
	err = i2c_reg_write_byte_dt(&config->int_i2c, REG_XT_CALIB_ADDR, reg);
	if (err != 0) {
		LOG_DBG("fail to set XT calibration register\n");
	}

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

static int am1805_get_calibration(const struct device *dev, int32_t *calib)
{
	int err;
	bool cmdx;
	uint8_t reg;
	uint8_t xtcal;

	struct am1805_data *data = dev->data;
	const struct am1805_config *config = dev->config;

	k_mutex_lock(&data->lock, K_FOREVER);

	err = i2c_reg_read_byte_dt(&config->int_i2c, REG_OSC_STATUS_ADDR, &reg);
	if (err != 0) {
		goto unlock;
	}

	/* First 2-bits (from MSB) */
	xtcal = reg >> 6;

	err = i2c_reg_read_byte_dt(&config->int_i2c, REG_XT_CALIB_ADDR, &reg);
	if (err != 0) {
		goto unlock;
	}

	*calib = reg;
	/* First bit (from MSB) */
	cmdx = reg & BIT(7);
	/* Set or Clear the bit-7 based on bit-6, to achieve the given range (in datasheet) */
	WRITE_BIT(reg, 7, (reg & BIT(6)));
	WRITE_BIT(reg, 6, 0);

	LOG_DBG("XTCAL = %d, CMDX = %d, OFFSETX = %d\n", xtcal, cmdx, (int8_t) reg);

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}
#endif

#ifdef CONFIG_RTC_ALARM
/* To get from the alarm registers */
static int am1805_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
		struct rtc_time *timeptr)
{

	int err;
	uint8_t regs[6];

	struct am1805_data *data = dev->data;
	const struct am1805_config *config = dev->config;

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	err = i2c_burst_read_dt(&config->int_i2c, REG_ALM_SECONDS_ADDR, regs, sizeof(regs));
	if (err != 0) {
		goto unlock;
	}

	timeptr->tm_sec = bcd2bin(regs[0] & SECONDS_BITS);
	timeptr->tm_min = bcd2bin(regs[1] & MINUTES_BITS);
	timeptr->tm_hour = bcd2bin(regs[2] & HOURS_BITS);
	timeptr->tm_mday = bcd2bin(regs[3] & DATE_BITS);
	timeptr->tm_mon = bcd2bin(regs[4] & MONTHS_BITS);
	timeptr->tm_wday = bcd2bin(regs[5] & WEEKDAY_BITS);

	*mask = (AM1805_RTC_ALARM_TIME_MASK);

	LOG_DBG("get alarm: wday = %d, mon = %d, mday = %d, hour = %d, min = %d, sec = %d, "
		"mask = 0x%04x", timeptr->tm_wday, timeptr->tm_mon, timeptr->tm_mday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec, *mask);

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

static int am1805_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
		const struct rtc_time *timeptr)
{
	int err;
	uint8_t regs[6];

	struct am1805_data *data = dev->data;
	const struct am1805_config *config = dev->config;

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	if ((mask & ~(AM1805_RTC_ALARM_TIME_MASK)) != 0U) {
		LOG_ERR("unsupported alarm field mask 0x%04x", mask);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Disable timer control registers before the initialization */
	err = i2c_reg_update_byte_dt(&config->int_i2c, REG_TIMER_CTRL_ADDR,
					REG_TIMER_CTRL_RPT_BITS, 0);
	if (err != 0) {
		goto unlock;
	}

	/* Clear the interrupt mask for alarm */
	err = i2c_reg_update_byte_dt(&config->int_i2c, REG_IRQ_MASK_ADDR,
					REG_IRQ_MASK_AIE, 0);
	if (err != 0) {
		goto unlock;
	}

	/* Clear the status bit */
	err = i2c_reg_update_byte_dt(&config->int_i2c, REG_STATUS_ADDR,
					REG_STATUS_ALM, 0);
	if (err != 0) {
		goto unlock;
	}

	/* When mask is 0 */
	if (mask == 0) {
		LOG_DBG("The alarm is disabled");
		goto unlock;
	}

	regs[0] = bin2bcd(timeptr->tm_sec) & SECONDS_BITS;
	regs[1] = bin2bcd(timeptr->tm_min) & MINUTES_BITS;
	regs[2] = bin2bcd(timeptr->tm_hour) & HOURS_BITS;
	regs[3] = bin2bcd(timeptr->tm_mday) & DATE_BITS;
	regs[4] = bin2bcd(timeptr->tm_mon) & MONTHS_BITS;
	regs[5] = bin2bcd(timeptr->tm_wday) & WEEKDAY_BITS;

	LOG_DBG("set alarm: second = %d, min = %d, hour = %d, mday = %d, month = %d,"
			"wday = %d,  mask = 0x%04x",
			timeptr->tm_sec, timeptr->tm_min, timeptr->tm_hour, timeptr->tm_mday,
			timeptr->tm_mon, timeptr->tm_wday, mask);

	err = i2c_burst_write_dt(&config->int_i2c, REG_ALM_SECONDS_ADDR, regs, sizeof(regs));
	if (err != 0) {
		goto unlock;
	}

	/* Enable irq timer after the initialization */
	err = i2c_reg_update_byte_dt(&config->int_i2c, REG_IRQ_MASK_ADDR,
					REG_IRQ_MASK_AIE, REG_IRQ_MASK_AIE);
	if (err != 0) {
		goto unlock;
	}

	/* Enable timer after the initialization for the config of repetation */
	err = i2c_reg_update_byte_dt(&config->int_i2c, REG_TIMER_CTRL_ADDR,
					TIMER_CTRL_ALM_SEC, TIMER_CTRL_ALM_SEC);

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

static int am1805_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	*mask = AM1805_RTC_ALARM_TIME_MASK;

	return 0;
}

static int am1805_alarm_set_callback(const struct device *dev, uint16_t id,
		rtc_alarm_callback callback, void *user_data)
{

	struct am1805_data *data = dev->data;
	const struct am1805_config *config = dev->config;

	if (config->int_gpio.port == NULL) {
		return -ENOTSUP;
	}

	if (id != 0U) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Passing the callback function and userdata filled by the user */
	data->alarm_user_callback = callback;
	data->alarm_user_data = user_data;

	k_mutex_unlock(&data->lock);

	return 0;
}

static void am1805_interrupt_thread(const struct device *dev)
{
	struct am1805_data *data = dev->data;

	while (1) {
		k_sem_take(&data->int_sem, K_FOREVER);

		if (data->alarm_user_callback == NULL) {
			LOG_DBG("Interrupt received, But No Alarm-Callback Initilized!!\n");
			continue;
		}
		data->alarm_user_callback(dev, 0, data->alarm_user_data);
	}
}

static void am1805_gpio_callback_handler(const struct device *port, struct gpio_callback *cb,
		gpio_port_pins_t pins)
{
	struct am1805_data *data = CONTAINER_OF(cb, struct am1805_data, am1805_callback);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_sem_give(&data->int_sem);
}
#endif

static int am1805_init(const struct device *dev)
{
	int err;
	uint8_t reg;

	const struct am1805_config *config = dev->config;
	struct am1805_data *data = dev->data;

	k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&config->int_i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	err = i2c_reg_read_byte_dt(&config->int_i2c, REG_STATUS_ADDR, &reg);
	if (err != 0) {
		LOG_ERR("failed to read the status register");
		return -ENODEV;
	}

#ifdef CONFIG_RTC_ALARM
	k_tid_t tid;

	k_sem_init(&data->int_sem, 0, INT_MAX);

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("GPIO not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (err != 0) {
		LOG_ERR("failed to configure GPIO (err %d)", err);
		return -ENODEV;
	}

	err = gpio_pin_interrupt_configure_dt(&config->int_gpio,
			GPIO_INT_EDGE_TO_INACTIVE);
	if (err != 0) {
		LOG_ERR("failed to configure interrupt (err %d)", err);
		return -ENODEV;
	}

	gpio_init_callback(&data->am1805_callback, am1805_gpio_callback_handler,
			BIT(config->int_gpio.pin));

	err = gpio_add_callback_dt(&config->int_gpio, &data->am1805_callback);
	if (err != 0) {
		LOG_ERR("failed to add GPIO callback (err %d)", err);
		return -ENODEV;
	}

	tid = k_thread_create(&data->am1805_thread, data->am1805_stack,
			K_THREAD_STACK_SIZEOF(data->am1805_stack),
			(k_thread_entry_t)am1805_interrupt_thread, (void *)dev, NULL,
			NULL, CONFIG_RTC_AM1805_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, dev->name);
#endif
	return 0;
}

static DEVICE_API(rtc, am1805_driver_api) = {
	.set_time = am1805_set_time,
	.get_time = am1805_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = am1805_alarm_get_supported_fields,
	.alarm_set_time = am1805_alarm_set_time,
	.alarm_get_time = am1805_alarm_get_time,
	.alarm_set_callback = am1805_alarm_set_callback,
#endif
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = am1805_set_calibration,
	.get_calibration = am1805_get_calibration,
#endif
};

#define AM1805_INIT(inst)									\
static const struct am1805_config am1805_config_##inst = {					\
	.int_i2c = I2C_DT_SPEC_INST_GET(inst),							\
	IF_ENABLED(CONFIG_RTC_ALARM,								\
		(.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, am1805_gpios, {0})))};		\
												\
static struct am1805_data am1805_data_##inst;							\
												\
DEVICE_DT_INST_DEFINE(inst, &am1805_init, NULL, &am1805_data_##inst,				\
		      &am1805_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,		\
		      &am1805_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AM1805_INIT)
