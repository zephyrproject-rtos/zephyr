/*
 * Copyright (c) 2026 Maksim Salau <maksim.salau@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/init.h>
#include <zephyr/usb/usbd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(touch1200reset);

#define ADAFRUIT_NRF52_BOOTLOADER_UF2_MODE	0x57
#define ADAFRUIT_NRF52_BOOTLOADER_SERIAL_MODE	0x4e
#define ADAFRUIT_NRF52_BOOTLOADER_SKIP		0x6d

static void touch1200reset_cb(struct usbd_context *const ctx,
			      const struct usbd_msg *const msg)
{
	uint32_t baudrate;
	int ret;

	ARG_UNUSED(ctx);

	if (msg->type != USBD_MSG_CDC_ACM_LINE_CODING) {
		return;
	}

	ret = uart_line_ctrl_get(msg->dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret) {
		LOG_ERR("Failed to get baudrate: %d", ret);
		return;
	}

	if (baudrate != 1200) {
		return;
	}

	NRF_POWER->GPREGRET = ADAFRUIT_NRF52_BOOTLOADER_UF2_MODE;
	NVIC_SystemReset();
	CODE_UNREACHABLE;
}

static int touch1200reset_init(void)
{
	static const struct device *udc0_dev = DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0));
	struct usbd_context *const uds_ctx = (void *)udc_get_event_ctx(udc0_dev);
	int ret;

	if (udc0_dev == NULL || uds_ctx == NULL) {
		LOG_ERR("No USB device");
		return -ENODEV;
	}

	ret = usbd_msg_register_cb(uds_ctx, touch1200reset_cb);
	if (ret < 0) {
		LOG_ERR("Failed to register USB message callback: %d", ret);
		return ret;
	}

	ret = usbd_enable(uds_ctx);
	if (ret) {
		LOG_ERR("Failed to enable USB device support (%d)", ret);
		return ret;
	}

	return 0;
}

SYS_INIT(touch1200reset_init, APPLICATION, 99);
