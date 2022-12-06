/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* PI3USB9201 USB BC 1.2 Charger Detector driver. */

#define DT_DRV_COMPAT diodes_pi3usb9201

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/usb/usb_bc12.h>
#include <zephyr/logging/log.h>
#include "bc12_pi3usb9201.h"

LOG_MODULE_REGISTER(PI3USB9201, CONFIG_USB_BC12_LOG_LEVEL);

/* Constant configuration data */
struct pi3usb9201_config {
	struct i2c_dt_spec i2c;

	struct gpio_dt_spec intb_gpio;

	enum bc12_type charging_mode;
};

/* Run-time configuration data */
struct pi3usb9201_data {
	const struct device *dev;
	struct k_work work;
	enum bc12_type partner_type;
	struct gpio_callback gpio_cb;

	bc12_callback_t result_cb;
	void *result_cb_data;
};

enum pi3usb9201_client_sts {
	CHG_OTHER = 0,
	CHG_2_4A = 1,
	CHG_2_0A = 2,
	CHG_1_0A = 3,
	CHG_RESERVED = 4,
	CHG_CDP = 5,
	CHG_SDP = 6,
	CHG_DCP = 7,
};

struct bc12_status {
	enum bc12_type partner_type;
	int current_limit;
};

/*
 * The USB Type-C specification limits the maximum amount of current from BC 1.2
 * suppliers to 1.5A.  Technically, proprietary methods are not allowed, but we
 * will continue to allow those.
 */
static const struct bc12_status bc12_chg_limits[] = {
	/* For unknown chargers return Isusp. */
	[CHG_OTHER] = {BC12_TYPE_PROPRIETARY, BC12_CURR_UA(BC12_CHARGER_MIN_CURR_UA)},
	[CHG_2_4A] = {BC12_TYPE_PROPRIETARY, BC12_CURR_UA(2400000)},
	[CHG_2_0A] = {BC12_TYPE_PROPRIETARY, BC12_CURR_UA(2000000)},
	[CHG_1_0A] = {BC12_TYPE_PROPRIETARY, BC12_CURR_UA(1000000)},
	[CHG_RESERVED] = {BC12_TYPE_NONE, 0},
	[CHG_CDP] = {BC12_TYPE_CDP, BC12_CURR_UA(1500000)},
	/* BC1.2 driver contract specifies to return Isusp for SDP ports. */
	[CHG_SDP] = {BC12_TYPE_SDP, BC12_CURR_UA(BC12_CHARGER_MIN_CURR_UA)},
	[CHG_DCP] = {BC12_TYPE_DCP, BC12_CURR_UA(1500000)},
};

static int pi3usb9201_interrupt_enable(const struct device *dev, const bool enable)
{
	const struct pi3usb9201_config *cfg = dev->config;

	/* Clear the interrupt mask bit to enable the interrupt */
	return i2c_reg_update_byte_dt(&cfg->i2c, PI3USB9201_REG_CTRL_1,
				      PI3USB9201_REG_CTRL_1_INT_MASK,
				      enable ? 0 : PI3USB9201_REG_CTRL_1_INT_MASK);
}

static int pi3usb9201_bc12_detect_ctrl(const struct device *dev, const bool enable)
{
	const struct pi3usb9201_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->i2c, PI3USB9201_REG_CTRL_2,
				      PI3USB9201_REG_CTRL_2_START_DET,
				      enable ? PI3USB9201_REG_CTRL_2_START_DET : 0);
}

static int pi3usb9201_bc12_usb_switch(const struct device *dev, bool enable)
{
	const struct pi3usb9201_config *cfg = dev->config;

	/* USB data switch enabled when PI3USB9201_REG_CTRL_2_AUTO_SW is clear */
	return i2c_reg_update_byte_dt(&cfg->i2c, PI3USB9201_REG_CTRL_2,
				      PI3USB9201_REG_CTRL_2_START_DET,
				      enable ? 0 : PI3USB9201_REG_CTRL_2_AUTO_SW);
}

static int pi3usb9201_set_mode(const struct device *dev, enum pi3usb9201_mode mode)
{
	const struct pi3usb9201_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->i2c, PI3USB9201_REG_CTRL_1,
				      PI3USB9201_REG_CTRL_1_MODE_MASK
					      << PI3USB9201_REG_CTRL_1_MODE_SHIFT,
				      mode << PI3USB9201_REG_CTRL_1_MODE_SHIFT);
}

static int pi3usb9201_get_status(const struct device *dev, uint8_t *const client,
				 uint8_t *const host)
{
	const struct pi3usb9201_config *cfg = dev->config;
	uint8_t status;
	int rv;

	rv = i2c_reg_read_byte_dt(&cfg->i2c, PI3USB9201_REG_CLIENT_STS, &status);
	if (rv < 0) {
		return rv;
	}

	if (client != NULL) {
		*client = status;
	}

	rv = i2c_reg_read_byte_dt(&cfg->i2c, PI3USB9201_REG_HOST_STS, &status);
	if (rv < 0) {
		return rv;
	}

	if (host != NULL) {
		*host = status;
	}

	return 0;
}

static void pi3usb9201_notify_callback(const struct device *dev,
				       struct bc12_partner_state *const state)
{
	struct pi3usb9201_data *pi3usb9201_data = dev->data;

	if (pi3usb9201_data->result_cb) {
		pi3usb9201_data->result_cb(dev, state, pi3usb9201_data->result_cb_data);
	}
}

static void pi3usb9201_update_charging_partner(const struct device *dev,
					       struct bc12_partner_state *const state)
{
	struct pi3usb9201_data *pi3usb9201_data = dev->data;

	if (state && state->type == pi3usb9201_data->partner_type) {
		/* No change to the charging partner */
		return;
	}

	if (state && state->type != BC12_TYPE_NONE) {
		/* Now update the current charger type */
		pi3usb9201_data->partner_type = state->type;
		pi3usb9201_notify_callback(dev, state);
	} else {
		pi3usb9201_data->partner_type = BC12_TYPE_NONE;
		pi3usb9201_notify_callback(dev, NULL);
	}
}

static int pi3usb9201_client_detect_start(const struct device *dev)
{
	int rv;

	/*
	 * Read both status registers to ensure that all interrupt indications
	 * are cleared prior to starting bc1.2 detection.
	 */
	pi3usb9201_get_status(dev, NULL, NULL);

	/* Put pi3usb9201 into client mode */
	rv = pi3usb9201_set_mode(dev, PI3USB9201_CLIENT_MODE);
	if (rv < 0) {
		return rv;
	}

	/* Have pi3usb9201 start bc1.2 detection */
	rv = pi3usb9201_bc12_detect_ctrl(dev, true);
	if (rv < 0) {
		return rv;
	}

	/* Enable interrupt to wake task when detection completes */
	return pi3usb9201_interrupt_enable(dev, true);
}

static void pi3usb9201_client_detect_finish(const struct device *dev, const int status)
{
	struct bc12_partner_state new_chg;
	int bit_pos;
	bool enable_usb_data;

	/* Set charge voltage to 5V */
	new_chg.voltage_uv = BC12_CHARGER_VOLTAGE_UV;

	/*
	 * Find set bit position. Note that this function is only called if a
	 * bit was set in client_status, so bit_pos won't be negative.
	 */
	bit_pos = __builtin_ffs(status) - 1;

	new_chg.current_ua = bc12_chg_limits[bit_pos].current_limit;
	new_chg.type = bc12_chg_limits[bit_pos].partner_type;

	LOG_DBG("client status = 0x%x, current = %d mA, type = %d",
		status, new_chg.current_ua, new_chg.type);

	/* bc1.2 is complete and start bit does not auto clear */
	if (pi3usb9201_bc12_detect_ctrl(dev, false) < 0) {
		LOG_ERR("failed to clear client detect");
	}

	/* If DCP mode, disable USB swtich */
	if (status & BIT(CHG_DCP)) {
		enable_usb_data = false;
	} else {
		enable_usb_data = true;
	}
	if (pi3usb9201_bc12_usb_switch(dev, enable_usb_data) < 0) {
		LOG_ERR("failed to set USB data mode");
	}

	/* Inform charge manager of new supplier type and current limit */
	pi3usb9201_update_charging_partner(dev, &new_chg);
}

static int pi3usb9201_disconnect(const struct device *dev)
{
	int rv;

	/* Ensure USB switch auto-on is enabled */
	rv = pi3usb9201_bc12_usb_switch(dev, true);
	if (rv < 0) {
		return rv;
	}

	/* Put pi3usb9201 into its power down mode */
	rv = pi3usb9201_set_mode(dev, PI3USB9201_POWER_DOWN);
	if (rv < 0) {
		return rv;
	}

	/* The start bc1.2 bit does not auto clear */
	rv = pi3usb9201_bc12_detect_ctrl(dev, false);
	if (rv < 0) {
		return rv;
	}

	/* Mask interrupts until next bc1.2 detection event */
	rv = pi3usb9201_interrupt_enable(dev, false);
	if (rv < 0) {
		return rv;
	}

	/*
	 * Let the application know there's no more charge available for the
	 * supplier type that was most recently detected.
	 */
	pi3usb9201_update_charging_partner(dev, NULL);

	return 0;
}

static int pi3usb9201_set_portable_device(const struct device *dev)
{
	int rv;

	/* Disable interrupts during mode change */
	rv = pi3usb9201_interrupt_enable(dev, false);
	if (rv < 0) {
		return rv;
	}

	if (pi3usb9201_client_detect_start(dev) < 0) {
		struct bc12_partner_state new_result;

		/*
		 * VBUS is present, but starting bc1.2 detection failed
		 * for some reason. Set the partner type to unknown limit
		 * current to the minimum allowed for a suspended USB device.
		 */
		new_result.voltage_uv = BC12_CHARGER_VOLTAGE_UV;
		new_result.current_ua = BC12_CHARGER_MIN_CURR_UA;
		new_result.type = BC12_TYPE_UNKNOWN;
		/* Save supplier type and notify callbacks */
		pi3usb9201_update_charging_partner(dev, &new_result);
		LOG_ERR("bc1.2 detection failed, using defaults");

		return -EIO;
	}

	return 0;
}

static void pi3usb9201_isr_work(struct k_work *item)
{
	struct pi3usb9201_data *pi3usb9201_data = CONTAINER_OF(item, struct pi3usb9201_data, work);
	const struct device *dev = pi3usb9201_data->dev;
	uint8_t client;
	uint8_t host;
	int rv;

	rv = pi3usb9201_get_status(dev, &client, &host);
	if (rv < 0) {
		LOG_ERR("Failed to get host/client status");
		return;
	}

	if (client != 0) {
		/*
		 * Any bit set in client status register indicates that
		 * BC1.2 detection has completed.
		 */
		pi3usb9201_client_detect_finish(dev, client);
	}
}

static void pi3usb9201_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				     uint32_t pins)
{
	struct pi3usb9201_data *pi3usb9201_data = CONTAINER_OF(cb, struct pi3usb9201_data, gpio_cb);

	k_work_submit(&pi3usb9201_data->work);
}

static int pi3usb9201_set_role(const struct device *dev, const enum bc12_role role)
{
	switch (role) {
	case BC12_DISCONNECTED:
		return pi3usb9201_disconnect(dev);
	case BC12_PORTABLE_DEVICE:
		return pi3usb9201_set_portable_device(dev);

	default:
		LOG_ERR("unsupported BC12 role: %d", role);
		return -EINVAL;
	}

	return 0;
}

int pi3usb9201_set_result_cb(const struct device *dev, bc12_callback_t cb, void *const user_data)
{
	struct pi3usb9201_data *pi3usb9201_data = dev->data;

	pi3usb9201_data->result_cb = cb;
	pi3usb9201_data->result_cb_data = user_data;

	return 0;
}

static const struct bc12_driver_api pi3usb9201_driver_api = {
	.set_role = pi3usb9201_set_role,
	.set_result_cb = pi3usb9201_set_result_cb,
};

static int pi3usb9201_init(const struct device *dev)
{
	const struct pi3usb9201_config *cfg = dev->config;
	struct pi3usb9201_data *pi3usb9201_data = dev->data;
	int rv;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("Bus device is not ready.");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->intb_gpio)) {
		LOG_ERR("intb_gpio device is not ready.");
		return -ENODEV;
	}

	pi3usb9201_data->dev = dev;

	/*
	 * Set most recent bc1.2 detection type result to
	 * BC12_TYPE_NONE for the port.
	 */
	pi3usb9201_data->partner_type = BC12_TYPE_NONE;

	rv = gpio_pin_configure_dt(&cfg->intb_gpio, GPIO_INPUT);
	if (rv < 0) {
		LOG_DBG("Failed to set gpio callback.");
		return rv;
	}

	gpio_init_callback(&pi3usb9201_data->gpio_cb, pi3usb9201_gpio_callback,
			   BIT(cfg->intb_gpio.pin));
	k_work_init(&pi3usb9201_data->work, pi3usb9201_isr_work);

	rv = gpio_add_callback(cfg->intb_gpio.port, &pi3usb9201_data->gpio_cb);
	if (rv < 0) {
		LOG_DBG("Failed to set gpio callback.");
		return rv;
	}

	rv = gpio_pin_interrupt_configure_dt(&cfg->intb_gpio, GPIO_INT_EDGE_FALLING);
	if (rv < 0) {
		LOG_DBG("Failed to configure gpio interrupt.");
		return rv;
	}

	/*
	 * The is no specific initialization required for the pi3usb9201 other
	 * than disabling the interrupt.
	 */
	return pi3usb9201_interrupt_enable(dev, false);
}

#define PI2USB9201_DEFINE(inst)									\
	static struct pi3usb9201_data pi3usb9201_data_##inst;					\
												\
	static const struct pi3usb9201_config pi3usb9201_config_##inst = {			\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		.intb_gpio = GPIO_DT_SPEC_INST_GET(inst, intb_gpios),				\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, pi3usb9201_init, NULL, &pi3usb9201_data_##inst,		\
			      &pi3usb9201_config_##inst, POST_KERNEL,				\
			      CONFIG_APPLICATION_INIT_PRIORITY, &pi3usb9201_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PI2USB9201_DEFINE)
