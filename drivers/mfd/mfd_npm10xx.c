/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm10xx

#include <zephyr/drivers/mfd/npm10xx.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mfd_npm10xx, CONFIG_MFD_LOG_LEVEL);

/* Register Offsets */
#define NPM10_RESET_TASKS       0xDDU
#define NPM10_RESET_SCRATCH0    0xDEU
#define NPM10_RESET_CONFIG      0xDFU
#define NPM10_RESET_POFWARNTHR  0xE0U
#define NPM10_RESET_POFRESETTHR 0xE1U
#define NPM10_RESET_POFWARN     0xE2U
#define NPM10_RESET_REASON0     0xE3U
#define NPM10_RESET_REASON1     0xE4U
#define NPM10_RESET_LONGPRESS   0xE7U
#define NPM10_RESET_SHPHLD      0xE8U
#define NPM10_RESET_VBUSCONFIG  0xEAU
#define NPM10_RESET_VBUSSTATUS  0xEBU

/* GPIO registers */
#define NPM10_GPIO_CONFIG  0xA0U
#define NPM10_GPIO_USAGE   0xA3U
#define GPIO_CONFIG_OUTPUT BIT(1)
#define GPIO_CONFIG_OPENDR BIT(2)
#define GPIO_CONFIG_PULLDN BIT(3)
#define GPIO_CONFIG_PULLUP BIT(4)
#define GPIO_USAGE_IRQ     0x01U
#define GPIO_USAGE_INV     BIT(4)

/* EVENTS registers */
#define NPM10_EVENTS_SET       0x00U
#define NPM10_EVENTS_CLR       0x07U
#define NPM10_EVENTS_INTEN_SET 0x0eU
#define NPM10_EVENTS_INTEN_CLR 0x15U
#define EVENTS_REG_NUM         7

/* TASKS (0xDD) */
#define RESET_TASKS_SWRST         BIT(0)
#define RESET_TASKS_HIBERNATEVBUS BIT(1)
#define RESET_TASKS_HIBERNATEVBAT BIT(2)
#define RESET_TASKS_STANDBY       BIT(3)
#define RESET_TASKS_STANDBYEXIT   BIT(4)
#define RESET_TASKS_SHIP          BIT(5)
#define RESET_TASKS_CLR           BIT(6)

/* CONFIG (0xDF) */
#define RESET_CONFIG_BOOTMON       BIT(0)
#define RESET_CONFIG_PWRWAIT_MASK  (BIT_MASK(2) << 1)
#define RESET_CONFIG_PWRWAIT_350MS (0U << 1)
#define RESET_CONFIG_PWRWAIT_250MS (1U << 1)
#define RESET_CONFIG_PWRWAIT_150MS (2U << 1)
#define RESET_CONFIG_PWRWAIT_50MS  (3U << 1)

/* POFWARNTHR (0xE0) */
#define RESET_POFWARNTHR_LVL_MASK (BIT_MASK(4) << 0)

/* POFRESETTHR (0xE1) */
#define RESET_POFRESETTHR_LVL_MASK (BIT_MASK(4) << 0)

/* POFWARN (0xE2) */
#define RESET_POFWARN_STATUS BIT(0)

/* LONGPRESS (0xE7) */
#define RESET_LONGPRESS_PINSEL_MASK   (BIT_MASK(2) << 0)
#define RESET_LONGPRESS_PINSEL_NONE   (0U << 0)
#define RESET_LONGPRESS_PINSEL_GPIO   (1U << 0)
#define RESET_LONGPRESS_PINSEL_SHPHLD (2U << 0)
#define RESET_LONGPRESS_PINSEL_BOTH   (3U << 0)

#define RESET_LONGPRESS_DEBOUNCE_MASK (BIT_MASK(2) << 2)
#define RESET_LONGPRESS_DEBOUNCE_3S   (0U << 2)
#define RESET_LONGPRESS_DEBOUNCE_5S   (1U << 2)
#define RESET_LONGPRESS_DEBOUNCE_10S  (2U << 2)
#define RESET_LONGPRESS_DEBOUNCE_20S  (3U << 2)

#define RESET_LONGPRESS_CFG BIT(4)

/* SHPHLD (0xE8) */
#define RESET_SHPHLD_DEBOUNCESEL BIT(0)

#define RESET_SHPHLD_DEBOUNCE_MASK  (BIT_MASK(2) << 1)
#define RESET_SHPHLD_DEBOUNCE_50MS  (0U << 1)
#define RESET_SHPHLD_DEBOUNCE_100MS (1U << 1)
#define RESET_SHPHLD_DEBOUNCE_500MS (2U << 1)
#define RESET_SHPHLD_DEBOUNCE_1S    (3U << 1)

#define RESET_SHPHLD_WAKEUP BIT(3)

/* VBUSCONFIG (0xEA) */
#define RESET_VBUSCONFIG_HIBERNATE        BIT(0)
#define RESET_VBUSCONFIG_HIBERNATE_NOWAIT 0U
#define RESET_VBUSCONFIG_HIBERNATE_WAIT   BIT(0)
#define RESET_VBUSCONFIG_STANDBY          BIT(1)
#define RESET_VBUSCONFIG_STANDBY_NOWAIT   0U
#define RESET_VBUSCONFIG_STANDBY_WAIT     BIT(1)
#define RESET_VBUSCONFIG_STANDBYSEL       BIT(2)
#define RESET_VBUSCONFIG_STANDBYSEL_1     0U
#define RESET_VBUSCONFIG_STANDBYSEL_2     BIT(2)

/* VBUSSTATUS (0xEB) */
#define RESET_VBUSSTATUS_HIBERNATE BIT(0)
#define RESET_VBUSSTATUS_STANDBY   BIT(1)

struct mfd_npm10xx_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec host_int_gpio;
	struct {
		uint8_t pin;
		gpio_flags_t flags;
	} pmic_int_gpio;
};

struct mfd_npm10xx_data {
	const struct device *mfd;
	struct k_work work;
	struct gpio_callback gpio_cb;
	sys_slist_t user_callbacks;
	struct k_mutex slist_mutex;
};

static void pmic_int_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct mfd_npm10xx_data *data = CONTAINER_OF(cb, struct mfd_npm10xx_data, gpio_cb);

	k_work_submit(&data->work);
}

static void events_handler(struct k_work *work)
{
	struct mfd_npm10xx_data *data = CONTAINER_OF(work, struct mfd_npm10xx_data, work);
	const struct mfd_npm10xx_config *config = data->mfd->config;
	uint8_t buf[EVENTS_REG_NUM];
	int ret;
	struct mfd_npm10xx_event_callback *cb;
	uint64_t events = 0U;
	uint64_t handled = 0U;

	ret = i2c_burst_read_dt(&config->i2c, NPM10_EVENTS_SET, buf, sizeof(buf));
	if (ret < 0) {
		k_work_submit(&data->work);
		return;
	}

	ARRAY_FOR_EACH(buf, idx) {
		events |= (uint64_t)buf[idx] << (idx * 8);
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&data->user_callbacks, cb, node) {
		if (cb->event_mask & events) {
			cb->handler(data->mfd, cb, cb->event_mask & events);
			handled |= cb->event_mask & events;
		}
	}

	ARRAY_FOR_EACH(buf, idx) {
		buf[idx] = FIELD_GET(0xFFULL << (idx * 8), handled);
	}

	ret = i2c_burst_write_dt(&config->i2c, NPM10_EVENTS_CLR, buf, sizeof(buf));
	if (ret < 0 || gpio_pin_get_dt(&config->host_int_gpio) != 0) {
		k_work_submit(&data->work);
	}
}

int mfd_npm10xx_reset(const struct device *dev)
{
	const struct mfd_npm10xx_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c, NPM10_RESET_TASKS, RESET_TASKS_SWRST);
}

int mfd_npm10xx_get_reset_reason(const struct device *dev, npm10xx_rstreas_t *reason)
{
	const struct mfd_npm10xx_config *config = dev->config;
	uint8_t reasons[2];

	int ret = i2c_burst_read_dt(&config->i2c, NPM10_RESET_REASON0, reasons, sizeof(reasons));

	if (ret < 0) {
		return ret;
	}

	*reason = (reasons[1] << 8) | reasons[0];

	return i2c_reg_write_byte_dt(&config->i2c, NPM10_RESET_TASKS, RESET_TASKS_CLR);
}

int mfd_npm10xx_hibernate(const struct device *dev, enum mfd_npm10xx_hibernate_mode mode,
			  k_timeout_t time)
{
	const struct mfd_npm10xx_config *config = dev->config;
	int ret;

	if (!K_TIMEOUT_EQ(time, K_FOREVER)) {
		LOG_ERR("Timed hibernation not supported yet");
		return -ENOTSUP;
	}

	switch (mode) {
	case NPM10XX_HIBERNATE_VBAT:
		return i2c_reg_write_byte_dt(&config->i2c, NPM10_RESET_TASKS,
					     RESET_TASKS_HIBERNATEVBAT);

	case NPM10XX_HIBERNATE_VBUS_WAIT:
		ret = i2c_reg_update_byte_dt(&config->i2c, NPM10_RESET_VBUSCONFIG,
					     RESET_VBUSCONFIG_HIBERNATE,
					     RESET_VBUSCONFIG_HIBERNATE_WAIT);
		break;

	case NPM10XX_HIBERNATE_VBUS_NOWAIT:
		ret = i2c_reg_update_byte_dt(&config->i2c, NPM10_RESET_VBUSCONFIG,
					     RESET_VBUSCONFIG_HIBERNATE,
					     RESET_VBUSCONFIG_HIBERNATE_NOWAIT);
		break;

	default:
		LOG_ERR("Invalid mode requested");
		return -EINVAL;
	}

	if (ret < 0) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&config->i2c, NPM10_RESET_TASKS, RESET_TASKS_HIBERNATEVBUS);
}

int mfd_npm10xx_standby(const struct device *dev, enum mfd_npm10xx_standby_op operation)
{
	const struct mfd_npm10xx_config *config = dev->config;
	int ret;
	uint8_t mask = RESET_VBUSCONFIG_STANDBY | RESET_VBUSCONFIG_STANDBYSEL;
	uint8_t reg = 0U;

	switch (operation) {
	case NPM10XX_STANDBY1_ENTER_WAIT:
		reg |= RESET_VBUSCONFIG_STANDBY_WAIT;
		/* fall through */
	case NPM10XX_STANDBY1_ENTER_NOWAIT:
		reg |= RESET_VBUSCONFIG_STANDBYSEL_1;
		break;

	case NPM10XX_STANDBY2_ENTER_WAIT:
		reg |= RESET_VBUSCONFIG_STANDBY_WAIT;
		/* fall through */
	case NPM10XX_STANDBY2_ENTER_NOWAIT:
		reg |= RESET_VBUSCONFIG_STANDBYSEL_2;
		break;

	case NPM10XX_STANDBY_EXIT:
		return i2c_reg_write_byte_dt(&config->i2c, NPM10_RESET_TASKS,
					     RESET_TASKS_STANDBYEXIT);

	default:
		LOG_ERR("Invalid operation requested");
		return -EINVAL;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, NPM10_RESET_VBUSCONFIG, mask, reg);
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&config->i2c, NPM10_RESET_TASKS, RESET_TASKS_STANDBY);
}

int mfd_npm10xx_manage_callback(const struct device *dev,
				struct mfd_npm10xx_event_callback *callback, bool add)
{
	const struct mfd_npm10xx_config *config = dev->config;
	struct mfd_npm10xx_data *data = dev->data;
	struct mfd_npm10xx_event_callback *cb;
	sys_snode_t *prev;
	int ret;
	uint8_t buf[EVENTS_REG_NUM];
	uint64_t unhandled = UINT64_MAX;

	ret = k_mutex_lock(&data->slist_mutex, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("Failed to lock mutex");
		return ret;
	}

	if (add) {
		if (sys_slist_find(&data->user_callbacks, &callback->node, NULL)) {
			LOG_ERR("Callback already added");
			ret = -EINVAL;
			goto unlock_n_return;
		}

		if (callback->event_mask == 0ULL) {
			LOG_ERR("Attempt to add a callback handling no events");
			ret = -EINVAL;
			goto unlock_n_return;
		}

		ARRAY_FOR_EACH(buf, idx) {
			buf[idx] = FIELD_GET(0xFFULL << (idx * 8), callback->event_mask);
		}

		/* Add the new callback before enabling its interrupts to avoid a race condition */
		/* Prepend for easier removal in case the I2C functions fail */
		sys_slist_prepend(&data->user_callbacks, &callback->node);

		/* Clear any pending old events */
		ret = i2c_burst_write_dt(&config->i2c, NPM10_EVENTS_CLR, buf, sizeof(buf));
		if (ret < 0) {
			sys_slist_remove(&data->user_callbacks, NULL, &callback->node);
			goto unlock_n_return;
		}

		ret = i2c_burst_write_dt(&config->i2c, NPM10_EVENTS_INTEN_SET, buf, sizeof(buf));
		if (ret < 0) {
			sys_slist_remove(&data->user_callbacks, NULL, &callback->node);
			goto unlock_n_return;
		}

	} else {
		if (!sys_slist_find(&data->user_callbacks, &callback->node, &prev)) {
			LOG_ERR("Callback was not previously added, cannot remove");
			ret = -EINVAL;
			goto unlock_n_return;
		}

		SYS_SLIST_FOR_EACH_CONTAINER(&data->user_callbacks, cb, node) {
			if (cb == callback) {
				/* Ignore the callback that is to be removed */
				continue;
			}
			unhandled &= ~cb->event_mask;
		}

		ARRAY_FOR_EACH(buf, idx) {
			buf[idx] = FIELD_GET(0xFFULL << (idx * 8), unhandled);
		}

		ret = i2c_burst_write_dt(&config->i2c, NPM10_EVENTS_INTEN_CLR, buf, sizeof(buf));
		if (ret < 0) {
			goto unlock_n_return;
		}

		/* Remove the callback after disabling its interrupts to avoid a race condition */
		sys_slist_remove(&data->user_callbacks, prev, &callback->node);
	}

unlock_n_return:
	(void)k_mutex_unlock(&data->slist_mutex);
	return ret;
}

static int mfd_npm10xx_init(const struct device *dev)
{
	const struct mfd_npm10xx_config *config = dev->config;
	struct mfd_npm10xx_data *data = dev->data;
	int ret;
	uint8_t pin_config, pin_usage;
	uint8_t disable_all_buf[EVENTS_REG_NUM] = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	data->mfd = dev;

	ret = k_mutex_init(&data->slist_mutex);
	if (ret < 0) {
		LOG_ERR("Failed to initialize mutex");
	}

	if (config->host_int_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->host_int_gpio)) {
			LOG_ERR("GPIO port used for interrupt input is not ready");
			return -ENODEV;
		}

		/* disable all interrupts to avoid the situation where an interrupt is active
		 * without a handler
		 */
		ret = i2c_burst_write_dt(&config->i2c, NPM10_EVENTS_INTEN_CLR, disable_all_buf,
					 sizeof(disable_all_buf));
		if (ret < 0) {
			return ret;
		}

		pin_config = GPIO_CONFIG_OUTPUT |
			     FIELD_PREP(GPIO_CONFIG_OPENDR,
					!!(config->pmic_int_gpio.flags & GPIO_OPEN_DRAIN)) |
			     FIELD_PREP(GPIO_CONFIG_PULLUP,
					!!(config->pmic_int_gpio.flags & GPIO_PULL_UP)) |
			     FIELD_PREP(GPIO_CONFIG_PULLDN,
					!!(config->pmic_int_gpio.flags & GPIO_PULL_DOWN));

		/* interrupt output is active low by default, set "invert" bit otherwise */
		pin_usage = GPIO_USAGE_IRQ |
			    FIELD_PREP(GPIO_USAGE_INV,
				       !(config->pmic_int_gpio.flags & GPIO_ACTIVE_LOW));

		ret = i2c_reg_write_byte_dt(
			&config->i2c, NPM10_GPIO_CONFIG + config->pmic_int_gpio.pin, pin_config);
		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(
			&config->i2c, NPM10_GPIO_USAGE + config->pmic_int_gpio.pin, pin_usage);
		if (ret < 0) {
			return ret;
		}

		ret = gpio_pin_configure_dt(&config->host_int_gpio, GPIO_INPUT);
		if (ret < 0) {
			return ret;
		}

		k_work_init(&data->work, events_handler);

		gpio_init_callback(&data->gpio_cb, pmic_int_handler,
				   BIT(config->host_int_gpio.pin));

		ret = gpio_add_callback(config->host_int_gpio.port, &data->gpio_cb);
		if (ret < 0) {
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->host_int_gpio,
						      GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

#define MFD_NPM10XX_DEFINE(n)                                                                      \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(n, pmic_int_gpio_config) ==                             \
			     DT_INST_NODE_HAS_PROP(n, host_int_gpios),                             \
		     "Only one of pmic-int-gpio-config or host-int-gpios is provided");            \
                                                                                                   \
	BUILD_ASSERT(COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pmic_int_gpio_config),                   \
				 (DT_INST_PROP_BY_IDX(n, pmic_int_gpio_config, 0)),                \
				 (0)) < 3,                                                         \
		     "PMIC output interrupt pin index out of range");                              \
                                                                                                   \
	static const struct mfd_npm10xx_config mfd_config##n = {                                   \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.host_int_gpio = GPIO_DT_SPEC_INST_GET_OR(n, host_int_gpios, {0}),                 \
		.pmic_int_gpio = DT_INST_PROP_OR(n, pmic_int_gpio_config, {UINT8_MAX}),            \
	};                                                                                         \
                                                                                                   \
	static struct mfd_npm10xx_data mfd_data##n;                                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mfd_npm10xx_init, NULL, &mfd_data##n, &mfd_config##n,             \
			      POST_KERNEL, CONFIG_MFD_NPM10XX_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_NPM10XX_DEFINE)
