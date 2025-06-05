/*
 * Copyright (c) 2025 Applied Materials
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/misc/usb3803/usb3803.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT microchip_usb3803

LOG_MODULE_REGISTER(usb3803, LOG_LEVEL_DBG);

int usb3803_reset(const struct usb3803_config *cfg, struct usb3803_dev *dev,
		  enum usb3803_modes mode)
{
	bool comms_ready = false;
	bool connected = false;
	uint8_t write_data[4];
	uint8_t read_data[4];
	int retry, err = 0;
	int i;

    /* Hardware reset the USB3803
     * Pin Status:
     *   Reset pin: Assert low. (active low)
     *   Bypass pin: Assert low. (active low)
     */
	err = gpio_is_ready_dt(&cfg->rst_pin);
	if (!err) {
		LOG_ERR("reset pin not ready %d", cfg->rst_pin.pin);
		err = -ENODEV;
		goto out;
	} else {
		err = gpio_pin_configure_dt(&cfg->rst_pin,
					    GPIO_PULL_UP | GPIO_OUTPUT |
					    GPIO_OUTPUT_LOW);
		if (err) {
			LOG_ERR("failed to configure reset pin: %d", err);
			goto out;
		}
	}

	if (!gpio_is_ready_dt(&cfg->bypass_pin)) {
		LOG_ERR("bypass pin not ready");
		err = -ENODEV;
		goto out;
	} else {
		err = gpio_pin_configure_dt(&cfg->bypass_pin,
					    GPIO_OUTPUT | GPIO_OUTPUT_LOW);
		if (err) {
			LOG_ERR("failed to bypass reset pin: %d", err);
			goto out;
		}
	}

	gpio_pin_set_dt(&cfg->rst_pin, 0);
	k_busy_wait(50000);

	switch (mode) {
	case USB3803_BYPASS:
		err = gpio_pin_configure_dt(&cfg->rst_pin, GPIO_DISCONNECTED);
		if (err) {
			LOG_ERR("failed to configure reset pin: %d", err);
			return -ENODEV;
		}

		gpio_pin_set_dt(&cfg->bypass_pin, 0);
		k_busy_wait(50000);

		dev->mode = mode;
		break;
	case USB3803_HUB:
		/* TODO: This needs to be tests at some point */
		err = gpio_pin_configure_dt(&cfg->rst_pin, GPIO_DISCONNECTED);
		if (err) {
			LOG_ERR("failed to configure reset pin: %d", err);
			goto out;
		}

		err = gpio_pin_configure_dt(&cfg->bypass_pin, GPIO_DISCONNECTED);
		if (err) {
			LOG_ERR("failed to configure reset pin: %d", err);
			goto out;
		}
		k_busy_wait(50000);

		// Stuck the usb3803 in the connect state.
		// Keep trying till it exceed the max try.
		retry = 10;
		connected = false;

		for (i = 0; i < retry; i++) {
			memset(write_data, 0, sizeof(write_data));
			memset(read_data, 0, sizeof(read_data));

			// Write the data
			write_data[0] = USB3803_SP_INTR_LOCK_CTRL;
			write_data[1] = 0x10;
			write_data[2] = 0x0b;
			err = i2c_write_dt(&cfg->bus, write_data, sizeof(write_data));
			if (err) {
				LOG_ERR("Can't write data to uwb3803 due to: %d ", err);
				goto out;
			} else
				i2c_read_dt(&cfg->bus, read_data, sizeof(read_data));

			// if the read data is not zero on connect_n bit jump out.
			if (read_data[0] & 0b10) {
				connected = true;
				break;
			}
		}

		if (!connected) {
			err = -ENODEV;
			goto out;
		}

		// Triggered usb3803 to enter communication stage.
		for (i = 0; i < retry; i++) {
			memset(write_data, 0, sizeof(write_data));
			memset(read_data, 0, sizeof(read_data));

			// Write the data
			write_data[0] = USB3803_SP_INTR_LOCK_CTRL;
			write_data[1] = 0x00;
			write_data[2] = 0x0b;
			err = i2c_write_dt(&cfg->bus, write_data, sizeof(write_data));
			if (err) {
				LOG_ERR("Can't write data to uwb3803 due to: %d ", err);
				goto out;
			} else
				i2c_read_dt(&cfg->bus, read_data, sizeof(read_data));

			// if the read data is not zero on connect_n bit jump out.
			if (!(read_data[0] & 10)) {
				comms_ready = true;
				break;
			}
		}

		if (comms_ready)
			dev->mode = mode;
		else {
			LOG_ERR("Failed to turn USB3803 back to communication stage.\n");
			dev->mode = USB3803_ERROR;
			err = -ENODEV;
			goto out;
		}
	break;
	case USB3803_UNINIT:
	case USB3803_STANDBY:
	case USB3803_ERROR:
	default:
		LOG_ERR("usb3803 unsported mode %d\n", mode);
		dev->mode = mode;
		break;
	}

out:
    return err;
}

static int usb3803_init(const struct device *dev)
{
	const struct usb3803_config *cfg = dev->config;
	const struct usb3803_data *data = dev->data;
	struct usb3803_dev *usb_dev = data->usb_dev;
	int err = 0;

	LOG_INF("USB3803 initialize called");

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("USB3803 device not ready.");
		err = -ENODEV;
		goto out;
	}

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("USB3803 i2c device not ready.");
		err = -ENODEV;
		goto out;
	}

	err = usb3803_reset(cfg, usb_dev, USB3803_BYPASS);
	if (err) {
		LOG_ERR("Failed to set up the USB3803 mode\n");
	}

out:
	return err;
}

#define USB3803_INIT(n)							      \
	static struct usb3803_dev udev_##n = {				      \
		.mode = USB3803_UNINIT,					      \
	};								      \
									      \
	static struct usb3803_data usb3803_data_##n = {			      \
		.dev = n,						      \
		.usb_dev = &udev_##n					      \
	};								      \
	static const struct usb3803_config usb3803_cfg_##n = {		      \
		.bus = I2C_DT_SPEC_INST_GET(n),				      \
		.rst_pin = GPIO_DT_SPEC_INST_GET(n, rst_pin_gpios),	      \
		.bypass_pin = GPIO_DT_SPEC_INST_GET(n, bypass_pin_gpios),     \
	};								      \
                                                                              \
	DEVICE_DT_INST_DEFINE(n, usb3803_init, NULL, &usb3803_data_##n,       \
			      &usb3803_cfg_##n, POST_KERNEL,		      \
			      CONFIG_USB3803_INIT_PRIORITY,                   \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(USB3803_INIT)
