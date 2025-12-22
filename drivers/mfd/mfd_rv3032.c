/*
 * Copyright (c) 2025 Baylibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mfd/rv3032.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mfd_rv3032, CONFIG_MFD_LOG_LEVEL);

#define DT_DRV_COMPAT microcrystal_rv3032_mfd

#define RV3032_BSM_DISABLED 0x0
#define RV3032_BSM_DIRECT   0x1
#define RV3032_BSM_LEVEL    0x2

#define RV3032_BSM_FROM_DT_INST(inst)                                                              \
	UTIL_CAT(RV3032_BSM_, DT_INST_STRING_UPPER_TOKEN(inst, backup_switch_mode))

#define RV3032_BACKUP_FROM_DT_INST(inst) (FIELD_PREP(0, RV3032_BSM_FROM_DT_INST(inst)))


struct mfd_rv3032_child {
	const struct device *dev;
	child_isr_t child_isr;
};

struct mfd_rv3032_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec gpio_int;
	struct gpio_dt_spec gpio_evi;
	void (*irq_config_func)(const struct device *dev);
	uint8_t aon;
	uint8_t backup;
};

struct mfd_rv3032_data {
	struct k_sem lock;
	struct k_work work;
	struct gpio_callback int_callback;
	const struct device *dev;
	struct mfd_rv3032_child children[RV3032_DEV_MAX];
};

static void mfd_rv3032_lock_sem(const struct device *dev)
{
	struct mfd_rv3032_data *data = dev->data;

	(void)k_sem_take(&data->lock, K_FOREVER);

}

static void mfd_rv3032_unlock_sem(const struct device *dev)
{
	struct mfd_rv3032_data *data = dev->data;

	k_sem_give(&data->lock);

}

static void mfd_rv3032_fire_child_callback(struct mfd_rv3032_data *data, enum child_dev child_idx)
{
	struct mfd_rv3032_child *child = &data->children[child_idx];

	if (child->child_isr != NULL) {
		child->child_isr(child->dev);
	} else {
		LOG_WRN("child_isr missing (%d)", child_idx);
	}
}

static void mfd_rv3032_work_cb(struct k_work *work)
{
	struct mfd_rv3032_data *data = CONTAINER_OF(work, struct mfd_rv3032_data, work);
	uint8_t status;
	int ret;

	ret = mfd_rv3032_read_reg8(data->dev, RV3032_REG_STATUS, &status);
	if (ret) {
		return;
	}

	/* Voltage Low Flag */
	if (status & RV3032_STATUS_VLF) {
		ret = mfd_rv3032_clear_status(data->dev, RV3032_STATUS_VLF);
		LOG_DBG("(STATUS) Voltage Low Flag (%x)\n", status);
	}

	/* Power On Reset Flag */
	if (status & RV3032_STATUS_PORF) {
		ret = mfd_rv3032_clear_status(data->dev, RV3032_STATUS_PORF);
		LOG_DBG("(STATUS) Power On Reset Flag (%x)\n", status);
	}

	/* External Event Flag */
	if (status & RV3032_STATUS_EVF) {
		ret = mfd_rv3032_clear_status(data->dev, RV3032_STATUS_EVF);
		LOG_DBG("(STATUS) External Event Flag (%x)\n", status);
		/* TODO : Implement EVI handling */
	}

	/* Alarm RTC - RTC */
	if (status & RV3032_STATUS_AF) {

		ret = mfd_rv3032_clear_status(data->dev, RV3032_STATUS_AF);
		mfd_rv3032_fire_child_callback(data, RV3032_DEV_RTC_ALARM);
		LOG_DBG("(STATUS) Alarm RTC (%x)\n", status);
	}

	/* Periodic counter - COUNTER*/
	if (status & RV3032_STATUS_TF) {
		ret = mfd_rv3032_clear_status(data->dev, RV3032_STATUS_TF);
		mfd_rv3032_fire_child_callback(data, RV3032_DEV_COUNTER);
		LOG_DBG("(STATUS) Periodic counter (%x)\n", status);
	}

	/* Periodic time update Flag - RTC */
	if (status & RV3032_STATUS_UF) {
		ret = mfd_rv3032_clear_status(data->dev, RV3032_STATUS_UF);
		mfd_rv3032_fire_child_callback(data, RV3032_DEV_RTC_UPDATE);
		LOG_DBG("(STATUS) Periodic time update Flag (%x)\n", status);
	}

	/* Temperature Low/High Flags - SENSORS */
	if ((status & RV3032_STATUS_TLF) || (status & RV3032_STATUS_THF)) {
		ret = mfd_rv3032_clear_status(data->dev, RV3032_STATUS_TLF | RV3032_STATUS_THF);
		mfd_rv3032_fire_child_callback(data, RV3032_DEV_SENSOR);
		LOG_DBG("(STATUS) Temperature Low/High Flag (%x)\n", status);
	}

	/* Check if interrupt occurred between STATUS read/write */
	if (ret) {
		k_work_submit(&data->work);
	}

}

static void mfd_rv3032_isr(const struct device *port, struct gpio_callback *cb,
			       gpio_port_pins_t pins)
{
	struct mfd_rv3032_data *data = CONTAINER_OF(cb, struct mfd_rv3032_data, int_callback);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_submit(&data->work);
}

void mfd_rv3032_set_irq_handler(const struct device *dev, const struct device *child_dev,
				enum child_dev child_idx, child_isr_t handler)
{
	struct mfd_rv3032_data *data = dev->data;
	struct mfd_rv3032_child *child;

	if ((child_idx <= RV3032_DEV_REG) || (child_idx >= RV3032_DEV_MAX)) {
		LOG_ERR("Not valid child IRQ idx [%d]\n", child_idx);
		return;
	}

	if ((handler == NULL)  || (child_dev == NULL)) {
		LOG_ERR("Child handler or dev pointer is NULL");
		return;
	}

	/* Set child device pointer */
	child = &data->children[child_idx];

	switch (child_idx) {
	case RV3032_DEV_RTC_ALARM:
		LOG_DBG("Add IRQ handler for (RTC ALARM)");
		break;
	case RV3032_DEV_RTC_UPDATE:
		LOG_DBG("Add IRQ handler for (RTC UPDATE)");
		break;
	case RV3032_DEV_COUNTER:
		LOG_DBG("Add IRQ handler for (COUNTER)");
		break;
	case RV3032_DEV_SENSOR:
		LOG_DBG("Add IRQ handler for (SENSOR)");
		break;
	case RV3032_DEV_REG:
	case RV3032_DEV_MAX:
	default:
		LOG_ERR("Invalid child_id, out of usable range");
		return;
	}

	LOG_DBG(" child_dev[%p] handler[%p] (%d)\n", child_dev, handler, child_idx);

	/* Store the interrupt handler and device instance for child device */
	child->dev = child_dev;
	child->child_isr = handler;
}

int mfd_rv3032_read_regs(const struct device *dev, uint8_t addr, void *buf, size_t len)
{
	const struct mfd_rv3032_config *config = dev->config;
	int err = 0;

	mfd_rv3032_lock_sem(dev);
	err = i2c_write_read_dt(&config->i2c, &addr, sizeof(addr), buf, len);
	mfd_rv3032_unlock_sem(dev);
	if (err) {
		LOG_ERR("failed to read reg addr 0x%02x, len %d (err %d)", addr, len, err);
	}

	return err;
}

int mfd_rv3032_read_reg8(const struct device *dev, uint8_t addr, uint8_t *val)
{
	return mfd_rv3032_read_regs(dev, addr, val, sizeof(*val));
}

int mfd_rv3032_write_regs(const struct device *dev, uint8_t addr, void *buf, size_t len)
{
	const struct mfd_rv3032_config *config = dev->config;
	uint8_t block[sizeof(addr) + len];
	int err = 0;

	mfd_rv3032_lock_sem(dev);
	block[0] = addr;
	memcpy(&block[1], buf, len);
	err = i2c_write_dt(&config->i2c, block, sizeof(block));
	mfd_rv3032_unlock_sem(dev);
	if (err) {
		LOG_ERR("failed to write reg addr 0x%02x, len %d (err %d)", addr, len, err);
	}

	return err;
}

int mfd_rv3032_write_reg8(const struct device *dev, uint8_t addr, uint8_t val)
{
	return mfd_rv3032_write_regs(dev, addr, &val, sizeof(val));
}

int mfd_rv3032_update_reg8(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val)
{
	const struct mfd_rv3032_config *config = dev->config;
	int err;

	mfd_rv3032_lock_sem(dev);
	err = i2c_reg_update_byte_dt(&config->i2c, addr, mask, val);
	mfd_rv3032_unlock_sem(dev);
	if (err) {
		LOG_ERR("failed to update reg addr 0x%02x, mask 0x%02x, val 0x%02x (err %d)", addr,
			mask, val, err);
	}

	return err;
}

int mfd_rv3032_update_status(const struct device *dev, uint8_t mask, uint8_t val)
{
	const struct mfd_rv3032_config *config = dev->config;
	int err;
	uint8_t old_val, new_val;
	uint8_t addr = RV3032_REG_STATUS;

	err = i2c_reg_read_byte_dt(&config->i2c, addr, &old_val);
	if (err != 0) {
		return err;
	}

	new_val = (old_val & ~mask) | (val & mask);
	if (new_val == old_val) {
		return 0;
	}

	err = i2c_reg_write_byte_dt(&config->i2c, addr, new_val);
	if (err != 0) {
		return err;
	}

	if (new_val) {
		LOG_DBG("Pending event!");
	}

	return new_val;
}

static int mfd_rv3032_init(const struct device *dev)
{
	struct mfd_rv3032_data *data = dev->data;
	const struct mfd_rv3032_config *config = dev->config;
	int err;

	k_sem_init(&data->lock, 1, 1);

	if (!i2c_is_ready_dt(&(config->i2c))) {
		LOG_ERR("I2C bus not ready.");
		return -ENODEV;
	}

	/* Clean all pending alarms and interrupts if in AON or backup mode
	 * in case of AON enabled, chip have uninterruptible power supply
	 * so we act like in one of the active backup modes, otherwise
	 * POR bit and interrupt bit are discarded and cleaned
	 */
	if ((!config->aon) && (config->backup == RV3032_BSM_DISABLED)) {
		uint8_t status;

		/* Set to default all configuration register at once
		 * According to datasheet they all have default 0 values
		 *  - CONTROL1
		 *  - CONTROL2
		 *  - CONTROL3
		 *  - Time Stamp Control
		 *  - Clock Interrupt Control
		 *  - EVI Control
		 */

		/* Clean all IRQ (RTC, Update, Counter) */
		err = mfd_rv3032_read_reg8(dev, RV3032_REG_STATUS, &status);
		if (err) {
			LOG_ERR("Status register read failed after EEPROM refresh: %d", err);
			return err;
		}

		if (status & RV3032_STATUS_PORF) {
			LOG_WRN("POR detected with AON and BACKUP disabled (MCU reset?)");
		}

		status = 0;
		/* Clean all IRQs (RTC, Update, Counter) */
		err = mfd_rv3032_write_reg8(dev, RV3032_REG_STATUS, status);
		if (err) {
			LOG_ERR("Status register write failed: %d", err);
			return err;
		}

		uint8_t zero_buff[6] = {0};
		/* Clean all regs at once: CONTROL1, CONTROL2, CONTROL3, TIME STAMP CONTROL,
		 * CLOCK INTERRUPT CONTROL, EVI CONTROL
		 */
		err = mfd_rv3032_write_regs(dev, RV3032_REG_CONTROL1, zero_buff, 6);
		if (err) {
			LOG_ERR("CONTROL register write failed after EEPROM refresh: %d", err);
			return err;
		}
	}

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

		gpio_init_callback(&data->int_callback, mfd_rv3032_isr, BIT(config->gpio_int.pin));

		err = gpio_add_callback_dt(&config->gpio_int, &data->int_callback);
		if (err) {
			LOG_ERR("failed to add GPIO callback (err %d)", err);
			return -ENODEV;
		}

		data->dev = dev;
		data->work.handler = mfd_rv3032_work_cb;
	} else {
		LOG_DBG("No GPIO INT in use!");
	}

	return 0;
}

#define MFD_RV3032_DEFINE(inst)                                                                    \
	static const struct mfd_rv3032_config mfd_rv3032_config##inst = {                          \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
		.gpio_evi = GPIO_DT_SPEC_INST_GET_OR(inst, evi_gpios, {0}),                        \
		.backup = RV3032_BACKUP_FROM_DT_INST(inst),                                        \
		.aon = DT_INST_PROP_OR(ints, always_on, 0),                                        \
		};                                                                                 \
                                                                                                   \
	static struct mfd_rv3032_data mfd_rv3032_data##inst;                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &mfd_rv3032_init, NULL, &mfd_rv3032_data##inst,                \
			      &mfd_rv3032_config##inst, POST_KERNEL,                               \
			      CONFIG_MFD_MICROCRYSTAL_RV3032_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_RV3032_DEFINE)
