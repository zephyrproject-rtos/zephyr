/*
 * Copyright 2025 Google LLC
 * Copyright (C) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/uart/uart_bridge.h>
#include <zephyr/kernel.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdc_acm_bridge, LOG_LEVEL_INF);

const struct device *const uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

static struct usbd_context *sample_usbd;

#define DEVICE_DT_GET_COMMA(node_id) DEVICE_DT_GET(node_id),

const struct device *uart_bridges[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_uart_bridge, DEVICE_DT_GET_COMMA)
};

/*
 * Optional coprocessor control signals.
 *
 * When the board overlay adds boot-gpios and reset-gpios to the /zephyr,user
 * node, the bridge drives them from the CDC-ACM control lines: a USB-serial
 * flashing tool toggles DTR/RTS to force the coprocessor into its ROM/UART
 * bootloader, and the bridge mirrors those lines onto the coprocessor boot and
 * reset pins. The mapping is the convention used by esptool (default_reset)
 * and other host flashers:
 *
 *   DTR -> boot (strap)     RTS -> reset (enable)
 *
 * Polarity is expressed with the GPIO_ACTIVE_* flag on each gpios property in
 * the overlay (typically GPIO_ACTIVE_LOW), so no code change is needed per
 * target.
 */
#define RESET_LINES_NODE DT_PATH(zephyr_user)

#if DT_NODE_HAS_PROP(RESET_LINES_NODE, boot_gpios) && \
	DT_NODE_HAS_PROP(RESET_LINES_NODE, reset_gpios)
#define HAS_RESET_LINES 1

static const struct gpio_dt_spec boot_gpio = GPIO_DT_SPEC_GET(RESET_LINES_NODE, boot_gpios);
static const struct gpio_dt_spec reset_gpio = GPIO_DT_SPEC_GET(RESET_LINES_NODE, reset_gpios);

static void reset_lines_update(const struct device *cdc)
{
	uint32_t dtr = 0U;
	uint32_t rts = 0U;

	(void)uart_line_ctrl_get(cdc, UART_LINE_CTRL_DTR, &dtr);
	(void)uart_line_ctrl_get(cdc, UART_LINE_CTRL_RTS, &rts);

	/* DTR drives boot, RTS drives reset; the overlay GPIO flags set polarity. */
	gpio_pin_set_dt(&boot_gpio, (int)dtr);
	gpio_pin_set_dt(&reset_gpio, (int)rts);

	LOG_DBG("ctrl-lines dtr=%u rts=%u", dtr, rts);
}

static int reset_lines_init(void)
{
	if (!gpio_is_ready_dt(&boot_gpio) || !gpio_is_ready_dt(&reset_gpio)) {
		LOG_ERR("reset-line GPIOs not ready");
		return -ENODEV;
	}

	/* Start both lines deasserted - their idle state, like a USB-serial
	 * adapter at rest. The flashing tool then drives the boot/reset
	 * sequence over the control lines.
	 */
	gpio_pin_configure_dt(&boot_gpio, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&reset_gpio, GPIO_OUTPUT_INACTIVE);
	return 0;
}
#endif /* reset lines */

static void sample_msg_cb(struct usbd_context *const ctx, const struct usbd_msg *msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (usbd_can_detect_vbus(ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(ctx)) {
				LOG_ERR("Failed to enable device support");
			}
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(ctx)) {
				LOG_ERR("Failed to disable device support");
			}
		}
	}

	if (msg->type == USBD_MSG_CDC_ACM_LINE_CODING ||
	    msg->type == USBD_MSG_CDC_ACM_CONTROL_LINE_STATE) {
		for (uint8_t i = 0; i < ARRAY_SIZE(uart_bridges); i++) {
			/* update all bridges, non valid combinations are
			 * skipped automatically.
			 */
			uart_bridge_settings_update(msg->dev, uart_bridges[i]);
		}
	}

#ifdef HAS_RESET_LINES
	if (msg->type == USBD_MSG_CDC_ACM_CONTROL_LINE_STATE) {
		reset_lines_update(msg->dev);
	}
#endif
}

int main(void)
{
	int err;

#ifdef HAS_RESET_LINES
	if (reset_lines_init()) {
		return -ENODEV;
	}
#endif

	sample_usbd = sample_usbd_init_device(sample_msg_cb);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	if (!usbd_can_detect_vbus(sample_usbd)) {
		err = usbd_enable(sample_usbd);
		if (err) {
			LOG_ERR("Failed to enable device support");
			return err;
		}
	}

	LOG_INF("USB device support enabled");

	k_sleep(K_FOREVER);

	return 0;
}
