/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB native_posix device driver
 */

#include <string.h>
#include <stdio.h>
#include <sys/byteorder.h>
#include <drivers/usb/usb_dc.h>
#include <usb/usb_device.h>
#include <net/net_ip.h>

#include "usb_dc_native_posix_adapt.h"

#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(native_posix);

#define USBIP_IN_EP_NUM		8
#define USBIP_OUT_EP_NUM	8

#define USBIP_MAX_PACKET_SIZE	64

K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
static struct k_thread thread;

static void thread_main(void *a, void *b, void *c)
{
	LOG_DBG("");

	usbip_start();
}

/*
 * USBIP private structures and logic initially copied from
 * Designware USB driver
 */

/*
 * USB endpoint private structure.
 */
struct usb_ep_ctrl_prv {
	uint8_t ep_ena;
	uint16_t mps;
	usb_dc_ep_callback cb;
	uint32_t data_len;
	uint8_t buf[64];
	uint8_t buf_len;
};

/*
 * USB controller private structure.
 */
static struct usbip_ctrl_prv {
	usb_dc_status_callback status_cb;
	struct usb_ep_ctrl_prv in_ep_ctrl[USBIP_IN_EP_NUM];
	struct usb_ep_ctrl_prv out_ep_ctrl[USBIP_OUT_EP_NUM];
	uint8_t attached;
} usbip_ctrl;

static uint8_t usbip_ep_is_valid(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	/* Check if ep is valid */
	if ((USB_EP_DIR_IS_OUT(ep)) &&
	    ep_idx < USBIP_OUT_EP_NUM) {
		return 1;
	} else if ((USB_EP_DIR_IS_IN(ep)) &&
	    ep_idx < USBIP_IN_EP_NUM) {
		return 1;
	}

	return 0;
}

static uint8_t usbip_ep_is_enabled(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	LOG_DBG("ep %x", ep);

	/* Check if ep enabled */
	if ((USB_EP_DIR_IS_OUT(ep)) &&
	    usbip_ctrl.out_ep_ctrl[ep_idx].ep_ena) {
		return 1;
	} else if ((USB_EP_DIR_IS_IN(ep)) &&
	    usbip_ctrl.in_ep_ctrl[ep_idx].ep_ena) {
		return 1;
	}

	return 0;
}

int usb_dc_attach(void)
{
	LOG_DBG("");

	if (usbip_ctrl.attached) {
		LOG_WRN("Already attached");
		return 0;
	}

	k_thread_create(&thread, thread_stack,
			CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE,
			thread_main, NULL, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);

	usbip_ctrl.attached = 1U;

	return 0;
}

int usb_dc_detach(void)
{
	LOG_DBG("");

	if (!usbip_ctrl.attached) {
		return 0;
	}

	usbip_ctrl.attached = 0U;

	return 0;
}

int usb_dc_reset(void)
{
	LOG_DBG("");

	/* Clear private data */
	memset(&usbip_ctrl, 0, sizeof(usbip_ctrl));

	return 0;
}

int usb_dc_set_address(const uint8_t addr)
{
	LOG_DBG("");

	return 0;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);

	LOG_DBG("ep %x, mps %d, type %d", cfg->ep_addr, cfg->ep_mps,
		cfg->ep_type);

	if ((cfg->ep_type == USB_DC_EP_CONTROL) && ep_idx) {
		LOG_ERR("invalid endpoint configuration");
		return -1;
	}

	if (cfg->ep_mps > USBIP_MAX_PACKET_SIZE) {
		LOG_WRN("unsupported packet size");
		return -1;
	}

	if ((USB_EP_DIR_IS_OUT(cfg->ep_addr)) &&
	    (ep_idx >= USBIP_OUT_EP_NUM)) {
		LOG_WRN("OUT endpoint address out of range");
		return -1;
	}

	if ((USB_EP_DIR_IS_IN(cfg->ep_addr)) &&
	    (ep_idx >= USBIP_IN_EP_NUM)) {
		LOG_WRN("IN endpoint address out of range");
		return -1;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data * const cfg)
{
	uint16_t ep_mps = cfg->ep_mps;
	uint8_t ep = cfg->ep_addr;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (usb_dc_ep_check_cap(cfg)) {
		return -EINVAL;
	}

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		usbip_ctrl.out_ep_ctrl[ep_idx].mps = ep_mps;
	} else {
		usbip_ctrl.in_ep_ctrl[ep_idx].mps = ep_mps;
	}

	return 0;
}

int usb_dc_ep_set_stall(const uint8_t ep)
{
	LOG_DBG("ep %x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* Use standard reply for now */
	usb_dc_ep_write(0x80,  NULL, 0, NULL);

	return 0;
}

int usb_dc_ep_clear_stall(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	LOG_DBG("ep %x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (!ep_idx) {
		/* Not possible to clear stall for EP0 */
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_halt(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	LOG_DBG("ep %x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (!ep_idx) {
		/* Cannot disable EP0, just set stall */
		usb_dc_ep_set_stall(ep);
	}

	return 0;
}

int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *const stalled)
{
	LOG_DBG("ep %x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (!stalled) {
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_enable(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	LOG_DBG("ep %x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* Enable Ep */
	if (USB_EP_DIR_IS_OUT(ep)) {
		usbip_ctrl.out_ep_ctrl[ep_idx].ep_ena = 1U;
	} else {
		usbip_ctrl.in_ep_ctrl[ep_idx].ep_ena = 1U;
	}

	return 0;
}

int usb_dc_ep_disable(const uint8_t ep)
{
	LOG_DBG("ep %x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_flush(const uint8_t ep)
{
	LOG_DBG("ep %x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		/* RX FIFO is global and cannot be flushed per EP */
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_write(const uint8_t ep, const uint8_t *const data,
		    const uint32_t data_len, uint32_t * const ret_bytes)
{
	LOG_DBG("ep %x len %u", ep, data_len);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* Check if IN ep */
	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_IN) {
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!usbip_ep_is_enabled(ep)) {
		LOG_WRN("ep %x disabled", ep);
		return -EINVAL;
	}

	if (USB_EP_GET_IDX(ep) == 0) {
		if (!usbip_send_common(ep, data_len)) {
			return -EIO;
		}

		if (usbip_send(ep, data, data_len) != data_len) {
			return -EIO;
		}
	} else {
		uint8_t ep_idx = USB_EP_GET_IDX(ep);
		struct usb_ep_ctrl_prv *ctrl = &usbip_ctrl.in_ep_ctrl[ep_idx];

		memcpy(ctrl->buf, data, data_len);
		ctrl->buf_len = data_len;
	}

	if (ret_bytes) {
		*ret_bytes = data_len;
	}

	return 0;
}

int usb_dc_ep_read_wait(uint8_t ep, uint8_t *data, uint32_t max_data_len,
			uint32_t *read_bytes)
{
	uint32_t bytes;

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_OUT) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	/* Allow to read 0 bytes */
	if (!data && max_data_len) {
		LOG_ERR("Wrong arguments");
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!usbip_ep_is_enabled(ep)) {
		LOG_ERR("Not enabled endpoint");
		return -EINVAL;
	}

	if (!data && !max_data_len) {
		/* When both buffer and max data to read are zero return
		 * the available data in buffer
		 */
		if (read_bytes) {
			uint8_t ep_idx = USB_EP_GET_IDX(ep);

			*read_bytes = usbip_ctrl.out_ep_ctrl[ep_idx].data_len;
		}

		return 0;
	}

	LOG_DBG("ep %x max_data_len %u", ep, max_data_len);

	bytes = usbip_recv(data, max_data_len);

	if (read_bytes) {
		*read_bytes = bytes;
	}

	return 0;
}

int usb_dc_ep_read_continue(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_OUT) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	if (!usbip_ctrl.out_ep_ctrl[ep_idx].data_len) {
		/* TODO: continue read */
		/* usbip_prep_rx(ep_idx, 0); */
	}

	return 0;
}

int usb_dc_ep_read(const uint8_t ep, uint8_t *const data,
		   const uint32_t max_data_len, uint32_t * const read_bytes)
{
	LOG_DBG("ep %x max_data_len %u", ep, max_data_len);

	if (usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes) != 0) {
		return -EINVAL;
	}

	if (!data && !max_data_len) {
		/* When both buffer and max data to read are zero the above
		 * call would fetch the data len and we simply return.
		 */
		return 0;
	}

	if (usb_dc_ep_read_continue(ep) != 0) {
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	LOG_DBG("ep %x callback %p", ep, cb);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		usbip_ctrl.in_ep_ctrl[ep_idx].cb = cb;
	} else {
		usbip_ctrl.out_ep_ctrl[ep_idx].cb = cb;
	}

	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	usbip_ctrl.status_cb = cb;
}

int usb_dc_ep_mps(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	LOG_DBG("ep %x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		return usbip_ctrl.out_ep_ctrl[ep_idx].mps;
	} else {
		return usbip_ctrl.in_ep_ctrl[ep_idx].mps;
	}
}

int handle_usb_control(struct usbip_header *hdr)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ntohl(hdr->common.ep));
	usb_dc_ep_callback ep_cb = usbip_ctrl.out_ep_ctrl[ep_idx].cb;

	LOG_DBG("ep %x idx %u", ntohl(hdr->common.ep), ep_idx);

	if (ep_cb) {
		LOG_DBG("Call ep_cb");
		ep_cb(ntohl(hdr->common.ep), USB_DC_EP_SETUP);
		if (ntohl(hdr->common.command) == USBIP_CMD_SUBMIT &&
		    hdr->u.submit.transfer_buffer_length != 0) {
			ep_cb(ntohl(hdr->common.ep), USB_DC_EP_DATA_OUT);
		}
	}

	return 0;
}

int handle_usb_data(struct usbip_header *hdr)
{
	uint8_t ep_idx = ntohl(hdr->common.ep);
	usb_dc_ep_callback ep_cb;
	uint8_t ep;

	LOG_DBG("ep_idx %u", ep_idx);

	if (ntohl(hdr->common.direction) == USBIP_DIR_OUT) {
		if (ep_idx >= USBIP_OUT_EP_NUM) {
			return -EINVAL;
		}

		ep = ep_idx | USB_EP_DIR_OUT;
		ep_cb = usbip_ctrl.out_ep_ctrl[ep_idx].cb;

		ep_cb(ep, USB_DC_EP_DATA_OUT);

		/* Send ACK reply */
		if (!usbip_send_common(ep, 0)) {
			return -EIO;
		}
	} else {
		uint8_t buf_len = usbip_ctrl.in_ep_ctrl[ep_idx].buf_len;
		uint8_t *buf = usbip_ctrl.in_ep_ctrl[ep_idx].buf;

		if (ep_idx >= USBIP_IN_EP_NUM) {
			return -EINVAL;
		}

		ep = ep_idx | USB_EP_DIR_IN;
		ep_cb = usbip_ctrl.in_ep_ctrl[ep_idx].cb;

		/* Read USB setup, not handled */
		if (!usbip_skip_setup()) {
			return -EIO;
		}

		LOG_DBG("Send %u bytes", buf_len);

		/* Send queued data */
		if (!usbip_send_common(ep, buf_len)) {
			return -EIO;
		}

		if (usbip_send(ep, buf, buf_len) != buf_len) {
			return -EIO;
		}

		LOG_HEXDUMP_DBG(buf, buf_len, ">");

		/* Indicate data sent */
		ep_cb(ep, USB_DC_EP_DATA_IN);
	}

	LOG_DBG("ep %x ep_cb %p", ep, ep_cb);

	return 0;
}
