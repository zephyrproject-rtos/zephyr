/*
 * Copyright (c) 2024 ANITRA system s.r.o.
 * Copyright (c) 2026 Janez Ugovsek <janez@ugovsek.info>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include "rv3028.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mfd_rv3028, CONFIG_MFD_LOG_LEVEL);

#define DT_DRV_COMPAT microcrystal_rv3028

/* Helper macro to guard int-gpios related code */
#if (DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) > 0)
#if !(defined(CONFIG_GPIO))
#error "CONFIG_GPIO is not defined, cannot use int-gpios"
#endif
#define RV3028_INT_GPIOS_IN_USE 1
#endif

struct mfd_rv3028_child {
	const struct device *dev;
	child_isr_t child_isr;
};

struct mfd_rv3028_config {
	const struct i2c_dt_spec i2c;
#ifdef RV3028_INT_GPIOS_IN_USE
	struct gpio_dt_spec gpio_int;
#endif /* RV3028_INT_GPIOS_IN_USE */
	uint8_t cof;
	uint8_t backup;
};

struct mfd_rv3028_data {
	struct k_sem lock;
#ifdef RV3028_INT_GPIOS_IN_USE
	const struct device *dev;
	struct gpio_callback int_callback;
	struct k_work work;
	struct mfd_rv3028_child children[RV3028_DEV_MAX];
#endif /* RV3028_INT_GPIOS_IN_USE */
};

#ifdef RV3028_INT_GPIOS_IN_USE
void mfd_rv3028_set_irq_handler(const struct device *dev, const struct device *child_dev,
				enum child_dev child_idx, child_isr_t handler)
{
	struct mfd_rv3028_data *data = dev->data;
	struct mfd_rv3028_child *child;

	if (child_idx <= RV3028_DEV_REG || child_idx >= RV3028_DEV_MAX) {
		LOG_ERR("Invalid child IRQ index %d", child_idx);
		return;
	}

	if ((handler == NULL) || (child_dev == NULL)) {
		LOG_ERR("Child handler or dev pointer is NULL");
		return;
	}

	/* Set child device pointer */
	child = &data->children[child_idx];

	switch (child_idx) {
	case RV3028_DEV_RTC_ALARM:
		LOG_DBG("Add IRQ handler for (RTC ALARM)");
		break;
	case RV3028_DEV_RTC_UPDATE:
		LOG_DBG("Add IRQ handler for (RTC UPDATE)");
		break;
	case RV3028_DEV_COUNTER:
		LOG_DBG("Add IRQ handler for (COUNTER)");
		break;
	case RV3028_DEV_SENSOR:
		LOG_DBG("Add IRQ handler for (SENSOR)");
		break;
	case RV3028_DEV_EVI:
		LOG_DBG("Add IRQ handler for (EVI)");
		break;
	case RV3028_DEV_REG:
	case RV3028_DEV_MAX:
	default:
		LOG_ERR("Invalid child_id, out of usable range");
		return;
	}

	LOG_DBG("child_dev[%p] handler[%p] (%d)", child_dev, handler, child_idx);

	/* Store the interrupt handler and device instance for child device */
	child->dev = child_dev;
	child->child_isr = handler;
}

#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE) || defined(CONFIG_COUNTER)
static void mfd_rv3028_fire_child_callback(struct mfd_rv3028_data *data, enum child_dev child_idx)
{
	struct mfd_rv3028_child *child = &data->children[child_idx];

	if (child->child_isr != NULL) {
		child->child_isr(child->dev);
	} else {
		LOG_WRN("child_isr missing (%d)", child_idx);
	}
}
#endif

static void mfd_rv3028_work_cb(struct k_work *work)
{
	struct mfd_rv3028_data *data = CONTAINER_OF(work, struct mfd_rv3028_data, work);
	const struct device *dev = data->dev;
	uint8_t status;
	int err;

	mfd_rv3028_lock_sem(dev);

	err = mfd_rv3028_read_reg8(data->dev, RV3028_REG_STATUS, &status);
	if (err) {
		goto unlock;
	}
	mfd_rv3028_unlock_sem(dev);

	/* Callback call if IRQ is detected - if feature is not supported clean flag */
#ifdef CONFIG_RTC_ALARM
	if (status & RV3028_STATUS_AF) {
		status &= ~(RV3028_STATUS_AF);
		mfd_rv3028_fire_child_callback(data, RV3028_DEV_RTC_ALARM);
	}
	if (status & RV3028_STATUS_EVF) {
		status &= ~(RV3028_STATUS_EVF);
		mfd_rv3028_fire_child_callback(data, RV3028_DEV_EVI);
	}
#else
	status &= ~(RV3028_STATUS_AF);
	status &= ~(RV3028_STATUS_EVF);
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
	if (status & RV3028_STATUS_UF) {
		status &= ~(RV3028_STATUS_UF);
		mfd_rv3028_fire_child_callback(data, RV3028_DEV_RTC_UPDATE);
	}
#else
	status &= ~(RV3028_STATUS_UF);
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_COUNTER
	if (status & RV3028_STATUS_TF) {
		status &= ~(RV3028_STATUS_TF);
		mfd_rv3028_fire_child_callback(data, RV3028_DEV_COUNTER);
	}
#else
	status &= ~(RV3028_STATUS_TF);
#endif /* CONFIG_COUNTER */

	mfd_rv3028_lock_sem(dev);
	err = mfd_rv3028_write_reg8(dev, RV3028_REG_STATUS, status);
	if (err) {
		goto unlock;
	}

	/* Check if interrupt occurred between STATUS read/write */
	err = mfd_rv3028_read_reg8(dev, RV3028_REG_STATUS, &status);
	if (err) {
		goto unlock;
	}

	if ((status & RV3028_STATUS_AF) || (status & RV3028_STATUS_UF) ||
	    (status & RV3028_STATUS_EVF) || (status & RV3028_STATUS_TF)) {
		/* Another interrupt occurred while servicing this one */
		k_work_submit(&data->work);
	}

unlock:
	mfd_rv3028_unlock_sem(dev);
}

static void mfd_rv3028_int_handler(const struct device *port, struct gpio_callback *cb,
				   gpio_port_pins_t pins)
{
	struct mfd_rv3028_data *data = CONTAINER_OF(cb, struct mfd_rv3028_data, int_callback);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_submit(&data->work);
}
#endif /* RV3028_INT_GPIOS_IN_USE */

#ifndef RV3028_INT_GPIOS_IN_USE
void mfd_rv3028_set_irq_handler(const struct device *dev, const struct device *child_dev,
				enum child_dev child_idx, child_isr_t handler)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(child_dev);
	ARG_UNUSED(child_idx);
	ARG_UNUSED(handler);
}
#endif /* RV3028_INT_GPIOS_IN_USE */

void mfd_rv3028_lock_sem(const struct device *dev)
{
	struct mfd_rv3028_data *data = dev->data;

	(void)k_sem_take(&data->lock, K_FOREVER);
}

void mfd_rv3028_unlock_sem(const struct device *dev)
{
	struct mfd_rv3028_data *data = dev->data;

	k_sem_give(&data->lock);
}

int mfd_rv3028_read_regs(const struct device *dev, uint8_t addr, void *buf, size_t len)
{
	const struct mfd_rv3028_config *config = dev->config;
	int err;

	err = i2c_write_read_dt(&config->i2c, &addr, sizeof(addr), buf, len);
	if (err) {
		LOG_ERR("failed to read reg addr 0x%02x, len %d (err %d)", addr, len, err);
		return err;
	}

	return 0;
}

int mfd_rv3028_read_reg8(const struct device *dev, uint8_t addr, uint8_t *val)
{
	return mfd_rv3028_read_regs(dev, addr, val, sizeof(*val));
}

int mfd_rv3028_write_regs(const struct device *dev, uint8_t addr, const void *buf, size_t len)
{
	const struct mfd_rv3028_config *config = dev->config;
	uint8_t block[sizeof(addr) + len];
	int err;

	block[0] = addr;
	memcpy(&block[1], buf, len);

	err = i2c_write_dt(&config->i2c, block, sizeof(block));
	if (err) {
		LOG_ERR("failed to write reg addr 0x%02x, len %d (err %d)", addr, len, err);
		return err;
	}

	return 0;
}

int mfd_rv3028_write_reg8(const struct device *dev, uint8_t addr, uint8_t val)
{
	return mfd_rv3028_write_regs(dev, addr, &val, sizeof(val));
}

int mfd_rv3028_update_reg8(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val)
{
	const struct mfd_rv3028_config *config = dev->config;
	int err;

	err = i2c_reg_update_byte_dt(&config->i2c, addr, mask, val);
	if (err) {
		LOG_ERR("failed to update reg addr 0x%02x, mask 0x%02x, val 0x%02x (err %d)", addr,
			mask, val, err);
		return err;
	}

	return 0;
}

int mfd_rv3028_eeprom_wait_busy(const struct device *dev, int poll_ms)
{
	uint8_t status = 0;
	int err;
	int64_t timeout_time = k_uptime_get() + RV3028_EEBUSY_TIMEOUT_MS;

	/* Wait while the EEPROM is busy */
	for (;;) {
		err = mfd_rv3028_read_reg8(dev, RV3028_REG_STATUS, &status);
		if (err) {
			return err;
		}

		if (!(status & RV3028_STATUS_EEBUSY)) {
			break;
		}

		if (k_uptime_get() > timeout_time) {
			return -ETIME;
		}

		k_msleep(poll_ms);
	}

	return 0;
}

int mfd_rv3028_exit_eerd(const struct device *dev)
{
	return mfd_rv3028_update_reg8(dev, RV3028_REG_CONTROL1, RV3028_CONTROL1_EERD, 0);
}

int mfd_rv3028_enter_eerd(const struct device *dev)
{
	uint8_t ctrl1;
	bool eerd;
	int ret;

	ret = mfd_rv3028_read_reg8(dev, RV3028_REG_CONTROL1, &ctrl1);
	if (ret) {
		return ret;
	}

	eerd = ctrl1 & RV3028_CONTROL1_EERD;
	if (eerd) {
		return 0;
	}

	ret = mfd_rv3028_update_reg8(dev, RV3028_REG_CONTROL1, RV3028_CONTROL1_EERD,
				     RV3028_CONTROL1_EERD);
	if (ret) {
		return ret;
	}

	ret = mfd_rv3028_eeprom_wait_busy(dev, RV3028_EEBUSY_WRITE_POLL_MS);
	if (ret) {
		mfd_rv3028_exit_eerd(dev);
		return ret;
	}

	return ret;
}

int mfd_rv3028_eeprom_command(const struct device *dev, uint8_t command)
{
	int err;

	err = mfd_rv3028_write_reg8(dev, RV3028_REG_EEPROM_COMMAND, RV3028_EEPROM_CMD_INIT);
	if (err) {
		return err;
	}

	return mfd_rv3028_write_reg8(dev, RV3028_REG_EEPROM_COMMAND, command);
}

int mfd_rv3028_update(const struct device *dev)
{
	int err;

	err = mfd_rv3028_eeprom_command(dev, RV3028_EEPROM_CMD_UPDATE);
	if (err) {
		goto exit_eerd;
	}

	err = mfd_rv3028_eeprom_wait_busy(dev, RV3028_EEBUSY_WRITE_POLL_MS);

exit_eerd:
	mfd_rv3028_exit_eerd(dev);

	return err;
}

int mfd_rv3028_refresh(const struct device *dev)
{
	int err;

	err = mfd_rv3028_eeprom_command(dev, RV3028_EEPROM_CMD_REFRESH);
	if (err) {
		goto exit_eerd;
	}

	err = mfd_rv3028_eeprom_wait_busy(dev, RV3028_EEBUSY_READ_POLL_MS);

exit_eerd:
	mfd_rv3028_exit_eerd(dev);

	return err;
}

int mfd_rv3028_update_cfg(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val)
{
	uint8_t val_old, val_new;
	int err;

	err = mfd_rv3028_read_reg8(dev, addr, &val_old);
	if (err) {
		return err;
	}

	val_new = (val_old & ~mask) | (val & mask);
	if (val_new == val_old) {
		return 0;
	}

	err = mfd_rv3028_enter_eerd(dev);

	if (err) {
		return err;
	}

	err = mfd_rv3028_write_reg8(dev, addr, val_new);
	if (err) {
		mfd_rv3028_exit_eerd(dev);
		return err;
	}

	return mfd_rv3028_update(dev);
}

static int mfd_rv3028_init(const struct device *dev)
{
	const struct mfd_rv3028_config *config = dev->config;
	struct mfd_rv3028_data *data = dev->data;
	uint8_t val;
	int err;

	k_sem_init(&data->lock, 1, 1);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	err = mfd_rv3028_read_reg8(dev, RV3028_REG_ID, &val);
	if (err) {
		return -ENODEV;
	}

	LOG_DBG("HID: 0x%02x, VID: 0x%02x", (val & 0xF0) >> 0x04, val & 0x0F);

#ifdef RV3028_INT_GPIOS_IN_USE
	if (config->gpio_int.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_int)) {
			LOG_ERR("GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->gpio_int, GPIO_INPUT);
		if (err) {
			LOG_ERR("failed to configure GPIO (err %d)", err);
			return -ENODEV;
		}

		err = gpio_pin_interrupt_configure_dt(&config->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("failed to enable GPIO interrupt (err %d)", err);
			return err;
		}

		gpio_init_callback(&data->int_callback, mfd_rv3028_int_handler,
				   BIT(config->gpio_int.pin));

		err = gpio_add_callback_dt(&config->gpio_int, &data->int_callback);
		if (err) {
			LOG_ERR("failed to add GPIO callback (err %d)", err);
			return -ENODEV;
		}

		data->dev = dev;
		k_work_init(&data->work, mfd_rv3028_work_cb);
	}
#endif /* RV3028_INT_GPIOS_IN_USE */

	err = mfd_rv3028_read_reg8(dev, RV3028_REG_STATUS, &val);
	if (err) {
		return -ENODEV;
	}

	if (val & RV3028_STATUS_PORF) {
		LOG_WRN("power on detected - clock is not valid");
	}

	if (val & RV3028_STATUS_AF) {
		LOG_WRN("an alarm may have been missed");
	}

	if (val & RV3028_STATUS_UF) {
		LOG_WRN("an update may have been missed");
	}

	if (val & RV3028_STATUS_BSF) {
		LOG_WRN("an backup switch flag was detected");
	}

	/* Refresh the settings in the RAM with the settings from the EEPROM */
	err = mfd_rv3028_enter_eerd(dev);
	if (err) {
		return -ENODEV;
	}
	err = mfd_rv3028_refresh(dev);
	if (err) {
		return -ENODEV;
	}

	/* Configure the CLKOUT register */
	val = FIELD_PREP(RV3028_CLKOUT_FD, config->cof) |
	      (config->cof != RV3028_CLKOUT_FD_LOW ? RV3028_CLKOUT_CLKOE : 0);
	err = mfd_rv3028_update_cfg(dev, RV3028_REG_CLKOUT, RV3028_CLKOUT_FD | RV3028_CLKOUT_CLKOE,
				    val);
	if (err) {
		return -ENODEV;
	}

	err = mfd_rv3028_update_cfg(dev, RV3028_REG_BACKUP,
				    RV3028_BACKUP_TCE | RV3028_BACKUP_TCR | RV3028_BACKUP_BSM,
				    config->backup);
	if (err) {
		return -ENODEV;
	}

	return 0;
}

#define RV3028_BSM_FROM_DT_INST(inst)                                                              \
	UTIL_CAT(RV3028_BSM_, DT_INST_STRING_UPPER_TOKEN(inst, backup_switch_mode))

#define RV3028_BACKUP_FROM_DT_INST(inst)                                                           \
	((FIELD_PREP(RV3028_BACKUP_BSM, RV3028_BSM_FROM_DT_INST(inst))) |                          \
	 (FIELD_PREP(RV3028_BACKUP_TCR, DT_INST_ENUM_IDX_OR(inst, trickle_resistor_ohms, 0))) |    \
	 (DT_INST_NODE_HAS_PROP(inst, trickle_resistor_ohms) ? RV3028_BACKUP_TCE : 0))

#define MFD_RV3028_DEFINE(inst)                                                                    \
	static const struct mfd_rv3028_config mfd_rv3028_config##inst = {                          \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.backup = RV3028_BACKUP_FROM_DT_INST(inst),                                        \
		.cof = DT_INST_ENUM_IDX_OR(inst, clkout_frequency, RV3028_CLKOUT_FD_LOW),          \
		IF_ENABLED(RV3028_INT_GPIOS_IN_USE, (.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst,    \
						    int_gpios, {0}))),              \
	};                                                                                         \
                                                                                                   \
	static struct mfd_rv3028_data mfd_rv3028_data##inst;                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &mfd_rv3028_init, NULL, &mfd_rv3028_data##inst,                \
			      &mfd_rv3028_config##inst, POST_KERNEL,                               \
			      CONFIG_MFD_RV3028_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_RV3028_DEFINE)
