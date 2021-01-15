/*
 * Copyright (c) 2018-2019 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief   USB HID interface for DAP controller
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(dap_hid, CONFIG_CMSIS_DAP_LOG_LEVEL);

#include <zephyr.h>
#include <init.h>
#include <device.h>

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include "cmsis_dap.h"

#define DAP_PACKET_SIZE		CONFIG_HID_INTERRUPT_EP_MPS

#if (DAP_PACKET_SIZE < 64U)
#error "Minimum Packet Size is 64"
#endif
#if (DAP_PACKET_SIZE > 32768U)
#error "Maximum Packet Size is 32768"
#endif

const static struct device *hid0_dev;
K_SEM_DEFINE(hid_epin_sem, 0, 1);

static struct k_thread usb_dap_tdata;
static K_THREAD_STACK_DEFINE(usb_dap_stack,
			     CONFIG_CMSIS_DAP_USB_HID_STACK_SIZE);

K_MEM_POOL_DEFINE(iface_mpool, DAP_PACKET_SIZE, DAP_PACKET_SIZE,
		  CONFIG_CMSIS_DAP_PACKET_COUNT, 4);

static struct k_mbox *dap_mbox;
static k_tid_t dap_tid;
static uint8_t dap_response_buf[DAP_PACKET_SIZE];

static const uint8_t hid_report_desc[] = {
	HID_GI_USAGE_VENDOR, 0x00, 0xff,
	HID_LI_USAGE, USAGE_GEN_DESKTOP_POINTER,
	HID_MI_COLLECTION, COLLECTION_APPLICATION,
	HID_GI_LOGICAL_MIN(1), 0x00,
	HID_GI_LOGICAL_MAX(2), 0xFF, 0x00,
	HID_GI_REPORT_SIZE, 8,
	HID_GI_REPORT_COUNT, 64,
	HID_LI_USAGE, USAGE_GEN_DESKTOP_POINTER,
	HID_MI_INPUT, 0x02,
	HID_GI_REPORT_COUNT, 64,
	HID_LI_USAGE, USAGE_GEN_DESKTOP_POINTER,
	HID_MI_OUTPUT, 0x02,
	HID_GI_REPORT_COUNT, 0x01,
	HID_LI_USAGE, USAGE_GEN_DESKTOP_POINTER,
	0xb1, 0x02,
	HID_MI_COLLECTION_END,
};

static int debug_cb(const struct device *dev,
		    struct usb_setup_packet *setup, int32_t *len,
		    uint8_t **data)
{
	/* TODO: add register/deregister message */
	LOG_DBG("unused callback");
	return -ENOTSUP;
}

static void int_in_ready_cb(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_DBG("EP IN ready");
	k_sem_give(&hid_epin_sem);
}

static void usb_dap_thead(void *p1, void *p2, void *p3)
{
	struct k_mbox *rspns_mbox = (struct k_mbox *)p1;
	struct k_mbox_msg rspns_msg;

	rspns_msg.info = DAP_MBMSG_REGISTER_IFACE;
	rspns_msg.size = 0;
	rspns_msg.tx_data = NULL;
	rspns_msg.tx_block.data = NULL;
	rspns_msg.tx_target_thread = dap_tid;
	k_mbox_put(rspns_mbox, &rspns_msg, K_FOREVER);

	while (1) {
		rspns_msg.size = DAP_PACKET_SIZE;
		/* receive a message sent by any thread */
		rspns_msg.rx_source_thread = K_ANY;

		k_mbox_get(rspns_mbox, &rspns_msg, dap_response_buf, K_FOREVER);

		LOG_DBG("message source thread %p size %u",
			rspns_msg.rx_source_thread,
			rspns_msg.size);

		if (hid_int_ep_write(hid0_dev, dap_response_buf,
				     rspns_msg.size, NULL)) {
			LOG_ERR("Failed to send a response over USB HID");
			continue;
		}

		k_sem_take(&hid_epin_sem, K_FOREVER);
	}
}

void int_out_ready_cb(const struct device *dev)
{
	uint32_t len = 0;
	struct k_mbox_msg req_msg;

	if (k_is_in_isr()) {
		LOG_ERR("Running at interrupt level");
		k_panic();
	}

	k_mem_pool_alloc(&iface_mpool, &req_msg.tx_block, DAP_PACKET_SIZE,
			 K_FOREVER);

	hid_int_ep_read(hid0_dev, req_msg.tx_block.data,
			DAP_PACKET_SIZE, &len);

	if (len == 0) {
		LOG_WRN("drop empty packet");
		k_mem_pool_free(&req_msg.tx_block);
		return;
	}

	req_msg.info = DAP_MBMSG_FROM_IFACE;
	req_msg.size = len;
	req_msg.tx_data = NULL;
	req_msg.tx_target_thread = dap_tid;
	k_mbox_async_put(dap_mbox, &req_msg, NULL);
}

static const struct hid_ops ops = {
	.get_report = debug_cb,
	.get_idle = debug_cb,
	.get_protocol = debug_cb,
	.set_report = debug_cb,
	.set_idle = debug_cb,
	.set_protocol = debug_cb,
	.int_in_ready = int_in_ready_cb,
	.int_out_ready = int_out_ready_cb,
};

static int composite_pre_init(const struct device *dev)
{
	k_tid_t hid_iface_tid;

	hid0_dev = device_get_binding(CONFIG_CMSIS_DAP_USB_HID_DEVICE_NAME);
	if (hid0_dev == NULL) {
		LOG_ERR("Cannot get USB HID 0 Device");
		return -ENODEV;
	}

	if (dap_setup(&dap_mbox, &dap_tid)) {
		LOG_ERR("Failed to initialize DAP controller");
		return -ENODEV;
	}

	hid_iface_tid = k_thread_create(&usb_dap_tdata, usb_dap_stack,
					CONFIG_CMSIS_DAP_USB_HID_STACK_SIZE,
					usb_dap_thead, dap_mbox,
					NULL, NULL,
					K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	LOG_INF("DAP mbox %p DAP tid %p HID tid %p",
		dap_mbox, dap_tid, hid_iface_tid);

	if (!hid_iface_tid) {
		LOG_ERR("Failed to initialize HID interface thread");
		return -ENODEV;
	}

	usb_hid_register_device(hid0_dev, hid_report_desc,
				sizeof(hid_report_desc), &ops);

	return usb_hid_init(hid0_dev);
}

SYS_INIT(composite_pre_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
