/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/pmci/mctp/mctp_usb.h>
#include <zephyr/usb/usbd.h>
#include <libmctp.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_endpoint);

#define SELF_ID	10
#define BUS_OWNER_ID 20

K_SEM_DEFINE(mctp_rx, 0, 1);
MCTP_USB_DEFINE(mctp0, USBD_MCTP_SUBCLASS_MANAGED_DEVICE_ENDPOINT, USBD_MCTP_PROTOCOL_1_X);

static struct mctp *mctp_ctx;
static bool usb_configured;

static void sample_msg_cb(struct usbd_context *const ctx, const struct usbd_msg *msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (msg->type == USBD_MSG_CONFIGURATION) {
		usb_configured = true;
	}
}

static int enable_usb_device_next(void)
{
	static struct usbd_context *mctp_poc_usbd;
	int err;

	mctp_poc_usbd = sample_usbd_init_device(sample_msg_cb);
	if (mctp_poc_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	if (!usbd_can_detect_vbus(mctp_poc_usbd)) {
		err = usbd_enable(mctp_poc_usbd);
		if (err) {
			LOG_ERR("Failed to enable device support");
			return err;
		}
	}

	LOG_INF("USB device support enabled");

	return 0;
}

static void rx_message(uint8_t eid, bool tag_owner, uint8_t msg_tag, void *data, void *msg,
		       size_t len)
{
	switch (eid) {
	case BUS_OWNER_ID: {
		LOG_INF("Received MCTP message \"%s\" from EID %d", (char *)msg, eid);
		k_sem_give(&mctp_rx);
		break;
	}
	default: {
		LOG_INF("Unknown endpoint %d", eid);
		break;
	}
	}
}

int main(void)
{
	LOG_INF("MCTP Endpoint EID: %d on %s", SELF_ID, CONFIG_BOARD_TARGET);

	int ret = enable_usb_device_next();

	if (ret != 0) {
		LOG_ERR("Failed to enable USB device support");
		return 0;
	}

	while (!usb_configured) {
		k_msleep(5);
	}

	mctp_set_alloc_ops(malloc, free, realloc);
	mctp_ctx = mctp_init();
	__ASSERT_NO_MSG(mctp_ctx != NULL);

	mctp_register_bus(mctp_ctx, &mctp0.binding, SELF_ID);
	mctp_set_rx_all(mctp_ctx, rx_message, NULL);

	while (1) {
		k_sem_take(&mctp_rx, K_FOREVER);
		mctp_message_tx(mctp_ctx, BUS_OWNER_ID, false, 0, "Hello, bus owner",
				sizeof("Hello, bus owner"));
	}

	return 0;
}
