/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Register facts: TPS65214 datasheet SLVSHK7A, TPS65215-Q1 datasheet SLVSHQ9.
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/tps6521x.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mfd_tps6521x, CONFIG_MFD_LOG_LEVEL);

/* TPS65214 only; on TPS65215 offset 0x03 is BUCKS_CONFIG */
#define TPS65214_REG_LOCK         0x03U
#define TPS65214_REG_UNLOCK_CMD   0x5AU
#define TPS65214_REG_LOCK_CMD     0x00U
#define TPS65214_REG_USER_NVM_CMD 0x34U

#define TPS6521X_REG_INT_SOURCE  0x2BU
#define TPS65214_INT_SOURCE_MASK 0xBFU
#define TPS65215_INT_SOURCE_MASK 0xFFU

/*
 * TI_DEV_ID. TPS65214 (SLVSHK7A table 8-3): bits 7-5 NVM revision, bits 4-0
 * device ID. 0x14 as read from a TPS6521401 on a TMDS62LEVM; the TRMs
 * SLVUD30A and SLVUD82 give device ID 0x04. TPS65215 (SLVSHQ9 table 6-9):
 * bits 7-6 temperature grade, bits 5-0 GPN; a TPS6521501-Q1 reads 0x95
 * (SLVUCW5).
 */
#define TPS65214_DEVICE_ID 0x14U
#define TPS65215_GPN       0x15U

#define TPS65214_DEV_ID_NVM_REV GENMASK(7, 5)
#define TPS65214_DEV_ID_DEVICE  GENMASK(4, 0)
#define TPS65215_DEV_ID_GPN     GENMASK(5, 0)

#define TPS6521X_INT_RECHECK K_MSEC(100)

struct mfd_tps6521x_config {
	struct i2c_dt_spec i2c;
	enum mfd_tps6521x_variant variant;
#ifdef CONFIG_MFD_TPS6521X_INTERRUPT
	struct gpio_dt_spec int_gpio;
#endif
};

struct mfd_tps6521x_data {
	struct k_mutex lock;
#ifdef CONFIG_MFD_TPS6521X_INTERRUPT
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_work_delayable int_work;
	struct k_mutex cb_lock;
	sys_slist_t callbacks;
#endif
};

/*
 * Every register but REG_LOCK, MFP_CTRL and USER_NVM_CMD is treated as
 * protected. The datasheet does not say whether the write-one-to-clear flag
 * registers are, and unlocking an unprotected register is harmless.
 */
static bool mfd_tps6521x_is_protected(const struct mfd_tps6521x_config *config, uint8_t reg)
{
	if (config->variant != MFD_TPS6521X_VARIANT_TPS65214) {
		return false;
	}

	return reg != TPS65214_REG_LOCK && reg != MFD_TPS6521X_REG_MFP_CTRL &&
	       reg != TPS65214_REG_USER_NVM_CMD;
}

int mfd_tps6521x_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct mfd_tps6521x_config *config = dev->config;

	return i2c_reg_read_byte_dt(&config->i2c, reg, val);
}

/*
 * The lock is always restored after a write: REG_LOCK reads back 0 and any
 * value other than 0x5A locks the protected registers again (SLVSHK7A table
 * 8-6). Nothing else may write to the PMIC while the driver runs.
 */
static int mfd_tps6521x_write_locked(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct mfd_tps6521x_config *config = dev->config;
	bool protected = mfd_tps6521x_is_protected(config, reg);
	int lock_ret;
	int ret;

	if (protected) {
		ret = i2c_reg_write_byte_dt(&config->i2c, TPS65214_REG_LOCK,
					    TPS65214_REG_UNLOCK_CMD);
		if (ret < 0) {
			return ret;
		}
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, reg, val);

	if (protected) {
		/* lock again even when the write failed */
		lock_ret = i2c_reg_write_byte_dt(&config->i2c, TPS65214_REG_LOCK,
						 TPS65214_REG_LOCK_CMD);
		if (ret == 0) {
			ret = lock_ret;
		}
	}

	return ret;
}

int mfd_tps6521x_reg_update(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
	const struct mfd_tps6521x_config *config = dev->config;
	struct mfd_tps6521x_data *data = dev->data;
	uint8_t old;
	uint8_t new;
	int ret;

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	ret = i2c_reg_read_byte_dt(&config->i2c, reg, &old);
	if (ret < 0) {
		goto out;
	}

	new = (old & ~mask) | (val & mask);
	if (new != old) {
		ret = mfd_tps6521x_write_locked(dev, reg, new);
	}

out:
	(void)k_mutex_unlock(&data->lock);

	return ret;
}

enum mfd_tps6521x_variant mfd_tps6521x_get_variant(const struct device *dev)
{
	const struct mfd_tps6521x_config *config = dev->config;

	return config->variant;
}

#ifdef CONFIG_MFD_TPS6521X_INTERRUPT

static bool mfd_tps6521x_has_int(const struct mfd_tps6521x_config *config)
{
	return config->int_gpio.port != NULL;
}

static uint8_t mfd_tps6521x_int_reg(unsigned int source_bit)
{
	if (source_bit == 7U) {
		return MFD_TPS6521X_REG_INT_PB;
	}

	return MFD_TPS6521X_REG_INT_TIMEOUT_RV_SD - source_bit;
}

static void mfd_tps6521x_dispatch(const struct device *dev, uint8_t reg, uint8_t value)
{
	struct mfd_tps6521x_data *data = dev->data;
	struct mfd_tps6521x_int_callback *cb;
	struct mfd_tps6521x_int_callback *tmp;
	bool handled = false;

	(void)k_mutex_lock(&data->cb_lock, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&data->callbacks, cb, tmp, node) {
		if (cb->reg == reg && (value & cb->mask) != 0U) {
			cb->handler(dev, cb, value);
			handled = true;
		}
	}
	(void)k_mutex_unlock(&data->cb_lock);

	if (!handled) {
		LOG_DBG("unhandled interrupt: reg 0x%02x = 0x%02x", reg, value);
	}
}

static void mfd_tps6521x_int_pass(const struct device *dev)
{
	const struct mfd_tps6521x_config *config = dev->config;
	struct mfd_tps6521x_data *data = dev->data;
	uint8_t values[8];
	uint8_t source;
	uint8_t reg;
	int ret;

	ret = mfd_tps6521x_reg_read(dev, TPS6521X_REG_INT_SOURCE, &source);
	if (ret < 0) {
		LOG_ERR("INT_SOURCE read failed (%d)", ret);
		return;
	}

	source &= config->variant == MFD_TPS6521X_VARIANT_TPS65215 ? TPS65215_INT_SOURCE_MASK
								   : TPS65214_INT_SOURCE_MASK;

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	for (unsigned int bit = 0U; bit < ARRAY_SIZE(values); bit++) {
		uint8_t clear;

		values[bit] = 0U;
		if ((source & BIT(bit)) == 0U) {
			continue;
		}

		reg = mfd_tps6521x_int_reg(bit);
		ret = mfd_tps6521x_reg_read(dev, reg, &values[bit]);
		if (ret < 0) {
			LOG_ERR("reg 0x%02x read failed (%d)", reg, ret);
			continue;
		}

		clear = values[bit];
		if (reg == MFD_TPS6521X_REG_INT_PB) {
			clear &= ~MFD_TPS6521X_INT_PB_REAL_TIME_STATUS;
		}

		if (clear != 0U) {
			ret = mfd_tps6521x_write_locked(dev, reg, clear);
			if (ret < 0) {
				LOG_ERR("reg 0x%02x clear failed (%d)", reg, ret);
			}
		}
	}
	(void)k_mutex_unlock(&data->lock);

	for (unsigned int bit = 0U; bit < ARRAY_SIZE(values); bit++) {
		if ((source & BIT(bit)) != 0U) {
			mfd_tps6521x_dispatch(dev, mfd_tps6521x_int_reg(bit), values[bit]);
		}
	}
}

static void mfd_tps6521x_int_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct mfd_tps6521x_data *data = CONTAINER_OF(dwork, struct mfd_tps6521x_data, int_work);
	const struct mfd_tps6521x_config *config = data->dev->config;

	mfd_tps6521x_int_pass(data->dev);

	if (gpio_pin_get_dt(&config->int_gpio) > 0) {
		(void)k_work_reschedule(&data->int_work, TPS6521X_INT_RECHECK);
	}
}

static void mfd_tps6521x_gpio_handler(const struct device *port, struct gpio_callback *cb,
				      gpio_port_pins_t pins)
{
	struct mfd_tps6521x_data *data = CONTAINER_OF(cb, struct mfd_tps6521x_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	(void)k_work_reschedule(&data->int_work, K_NO_WAIT);
}

static int mfd_tps6521x_int_init(const struct device *dev)
{
	const struct mfd_tps6521x_config *config = dev->config;
	struct mfd_tps6521x_data *data = dev->data;
	int ret;

	data->dev = dev;
	(void)k_mutex_init(&data->cb_lock);
	sys_slist_init(&data->callbacks);
	k_work_init_delayable(&data->int_work, mfd_tps6521x_int_work);

	if (!mfd_tps6521x_has_int(config)) {
		return 0;
	}

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, mfd_tps6521x_gpio_handler, BIT(config->int_gpio.pin));
	ret = gpio_add_callback_dt(&config->int_gpio, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	mfd_tps6521x_int_pass(dev);

	if (gpio_pin_get_dt(&config->int_gpio) > 0) {
		(void)k_work_schedule(&data->int_work, TPS6521X_INT_RECHECK);
	}

	return 0;
}

int mfd_tps6521x_add_int_callback(const struct device *dev, struct mfd_tps6521x_int_callback *cb)
{
	const struct mfd_tps6521x_config *config = dev->config;
	struct mfd_tps6521x_data *data = dev->data;

	if (!mfd_tps6521x_has_int(config)) {
		return -ENOTSUP;
	}

	if (cb == NULL || cb->handler == NULL || cb->mask == 0U) {
		return -EINVAL;
	}

	(void)k_mutex_lock(&data->cb_lock, K_FOREVER);
	(void)sys_slist_find_and_remove(&data->callbacks, &cb->node);
	sys_slist_append(&data->callbacks, &cb->node);
	(void)k_mutex_unlock(&data->cb_lock);

	return 0;
}

int mfd_tps6521x_remove_int_callback(const struct device *dev, struct mfd_tps6521x_int_callback *cb)
{
	const struct mfd_tps6521x_config *config = dev->config;
	struct mfd_tps6521x_data *data = dev->data;
	bool found;

	if (!mfd_tps6521x_has_int(config)) {
		return -ENOTSUP;
	}

	(void)k_mutex_lock(&data->cb_lock, K_FOREVER);
	found = sys_slist_find_and_remove(&data->callbacks, &cb->node);
	(void)k_mutex_unlock(&data->cb_lock);

	return found ? 0 : -EINVAL;
}

#else

static int mfd_tps6521x_int_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

int mfd_tps6521x_add_int_callback(const struct device *dev, struct mfd_tps6521x_int_callback *cb)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);

	return -ENOTSUP;
}

int mfd_tps6521x_remove_int_callback(const struct device *dev, struct mfd_tps6521x_int_callback *cb)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);

	return -ENOTSUP;
}

#endif

static int mfd_tps6521x_init(const struct device *dev)
{
	const struct mfd_tps6521x_config *config = dev->config;
	struct mfd_tps6521x_data *data = dev->data;
	uint8_t tps65214_id;
	uint8_t tps65215_gpn;
	uint8_t id;
	int ret;

	(void)k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, MFD_TPS6521X_REG_TI_DEV_ID, &id);
	if (ret < 0) {
		LOG_ERR("PMIC not responding (%d)", ret);
		return ret;
	}

	tps65214_id = FIELD_GET(TPS65214_DEV_ID_DEVICE, id);
	tps65215_gpn = FIELD_GET(TPS65215_DEV_ID_GPN, id);

	/*
	 * The other variant's ID is a wrong node, and its register map would
	 * write the wrong registers. Any other value only warns: the ID is per
	 * orderable part, and not every part's value is published.
	 */
	if (config->variant == MFD_TPS6521X_VARIANT_TPS65215) {
		if (tps65214_id == TPS65214_DEVICE_ID) {
			LOG_ERR("device ID 0x%02x is a TPS65214, the node says TPS65215", id);
			return -ENODEV;
		}
		if (tps65215_gpn != TPS65215_GPN) {
			LOG_WRN("device ID 0x%02x, expected GPN 0x%02x", id, TPS65215_GPN);
		}
	} else {
		if (tps65215_gpn == TPS65215_GPN) {
			LOG_ERR("device ID 0x%02x is a TPS65215, the node says TPS65214", id);
			return -ENODEV;
		}
		if (tps65214_id != TPS65214_DEVICE_ID) {
			LOG_WRN("device ID 0x%02x, expected device 0x%02x", id,
				TPS65214_DEVICE_ID);
		}
		LOG_DBG("NVM revision %lu", FIELD_GET(TPS65214_DEV_ID_NVM_REV, id));
	}

	return mfd_tps6521x_int_init(dev);
}

#define MFD_TPS6521X_DEFINE(node_id, _variant)                                                     \
	static struct mfd_tps6521x_data mfd_tps6521x_data_##node_id;                               \
	static const struct mfd_tps6521x_config mfd_tps6521x_config_##node_id = {                  \
		.i2c = I2C_DT_SPEC_GET(node_id),                                                   \
		.variant = _variant,                                                               \
		IF_ENABLED(CONFIG_MFD_TPS6521X_INTERRUPT,                                          \
			   (.int_gpio = GPIO_DT_SPEC_GET_OR(node_id, int_gpios, {0}),))            \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, mfd_tps6521x_init, NULL, &mfd_tps6521x_data_##node_id,           \
			 &mfd_tps6521x_config_##node_id, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,    \
			 NULL);

#define MFD_TPS65214_DEFINE(node_id) MFD_TPS6521X_DEFINE(node_id, MFD_TPS6521X_VARIANT_TPS65214)
#define MFD_TPS65215_DEFINE(node_id) MFD_TPS6521X_DEFINE(node_id, MFD_TPS6521X_VARIANT_TPS65215)

DT_FOREACH_STATUS_OKAY(ti_tps65214, MFD_TPS65214_DEFINE)
DT_FOREACH_STATUS_OKAY(ti_tps65215, MFD_TPS65215_DEFINE)
