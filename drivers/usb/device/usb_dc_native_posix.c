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
#include <usb/usb_dc.h>
#include <usb/usb_device.h>
#include <net/net_ip.h>

#include "usb_dc_native_posix_adapt.h"

#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(native_posix);

/* convert from endpoint address to hardware endpoint index */
#define USBIP_EP_ADDR2IDX(ep)  ((ep) & ~USB_EP_DIR_MASK)
/* get direction from endpoint address */
#define USBIP_EP_ADDR2DIR(ep)  ((ep) & USB_EP_DIR_MASK)
/* convert from hardware endpoint index and direction to endpoint address */
#define USBIP_EP_IDX2ADDR(idx, dir)    ((idx) | ((dir) & USB_EP_DIR_MASK))

#define USBIP_IN_EP_NUM		8
#define USBIP_OUT_EP_NUM	8

#define USBIP_MAX_PACKET_SIZE	64

K_THREAD_STACK_MEMBER(thread_stack, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
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
	u8_t ep_ena;
	u16_t mps;
	usb_dc_ep_callback cb;
	u32_t data_len;
	u8_t buf[64];
	u8_t buf_len;
};

/*
 * USB controller private structure.
 */
static struct usbip_ctrl_prv {
	usb_dc_status_callback status_cb;
	struct usb_ep_ctrl_prv in_ep_ctrl[USBIP_IN_EP_NUM];
	struct usb_ep_ctrl_prv out_ep_ctrl[USBIP_OUT_EP_NUM];
	u8_t attached;
} usbip_ctrl;

static u8_t usbip_ep_is_valid(u8_t ep)
{
	u8_t ep_idx = USBIP_EP_ADDR2IDX(ep);

	/* Check if ep is valid */
	if ((USBIP_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) &&
	    ep_idx < USBIP_OUT_EP_NUM) {
		return 1;
	} else if ((USBIP_EP_ADDR2DIR(ep) == USB_EP_DIR_IN) &&
	    ep_idx < USBIP_IN_EP_NUM) {
		return 1;
	}

	return 0;
}

static u8_t usbip_ep_is_enabled(u8_t ep)
{
	u8_t ep_idx = USBIP_EP_ADDR2IDX(ep);

	LOG_DBG("ep %x", ep);

	/* Check if ep enabled */
	if ((USBIP_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) &&
	    usbip_ctrl.out_ep_ctrl[ep_idx].ep_ena) {
		return 1;
	} else if ((USBIP_EP_ADDR2DIR(ep) == USB_EP_DIR_IN) &&
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

int usb_dc_set_address(const u8_t addr)
{
	LOG_DBG("");

	return 0;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	u8_t ep_idx = USBIP_EP_ADDR2IDX(cfg->ep_addr);

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

	if ((USBIP_EP_ADDR2DIR(cfg->ep_addr) == USB_EP_DIR_OUT) &&
	    (ep_idx >= USBIP_OUT_EP_NUM)) {
		LOG_WRN("OUT endpoint address out of range");
		return -1;
	}

	if ((USBIP_EP_ADDR2DIR(cfg->ep_addr) == USB_EP_DIR_IN) &&
	    (ep_idx >= USBIP_IN_EP_NUM)) {
		LOG_WRN("IN endpoint address out of range");
		return -1;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data * const cfg)
{
	u16_t ep_mps = cfg->ep_mps;
	u8_t ep = cfg->ep_addr;
	u8_t ep_idx = USBIP_EP_ADDR2IDX(ep);

	if (usb_dc_ep_check_cap(cfg)) {
		return -EINVAL;
	}

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (USBIP_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		usbip_ctrl.out_ep_ctrl[ep_idx].mps = ep_mps;
	} else {
		usbip_ctrl.in_ep_ctrl[ep_idx].mps = ep_mps;
	}

	return 0;
}

int usb_dc_ep_set_stall(const u8_t ep)
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

int usb_dc_ep_clear_stall(const u8_t ep)
{
	u8_t ep_idx = USBIP_EP_ADDR2IDX(ep);

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

int usb_dc_ep_halt(const u8_t ep)
{
	u8_t ep_idx = USBIP_EP_ADDR2IDX(ep);

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

int usb_dc_ep_is_stalled(const u8_t ep, u8_t *const stalled)
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

int usb_dc_ep_enable(const u8_t ep)
{
	u8_t ep_idx = USBIP_EP_ADDR2IDX(ep);

	LOG_DBG("ep %x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* Enable Ep */
	if (USBIP_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		usbip_ctrl.out_ep_ctrl[ep_idx].ep_ena = 1U;
	} else {
		usbip_ctrl.in_ep_ctrl[ep_idx].ep_ena = 1U;
	}

	return 0;
}

int usb_dc_ep_disable(const u8_t ep)
{
	LOG_DBG("ep %x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_flush(const u8_t ep)
{
	LOG_DBG("ep %x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (USBIP_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		/* RX FIFO is global and cannot be flushed per EP */
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_write(const u8_t ep, const u8_t *const data,
		    const u32_t data_len, u32_t * const ret_bytes)
{
	LOG_DBG("ep %x len %u", ep, data_len);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* Check if IN ep */
	if (USBIP_EP_ADDR2DIR(ep) != USB_EP_DIR_IN) {
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!usbip_ep_is_enabled(ep)) {
		LOG_WRN("ep %x disabled", ep);
		return -EINVAL;
	}

	if (USBIP_EP_ADDR2IDX(ep) == 0) {
		usbip_send_common(ep, data_len);
		usbip_send(ep, data, data_len);
	} else {
		u8_t ep_idx = USBIP_EP_ADDR2IDX(ep);
		struct usb_ep_ctrl_prv *ctrl = &usbip_ctrl.in_ep_ctrl[ep_idx];

		memcpy(ctrl->buf, data, data_len);
		ctrl->buf_len = data_len;
	}

	if (ret_bytes) {
		*ret_bytes = data_len;
	}

	return 0;
}

int usb_dc_ep_read_wait(u8_t ep, u8_t *data, u32_t max_data_len,
			u32_t *read_bytes)
{
	u32_t bytes;

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (USBIP_EP_ADDR2DIR(ep) != USB_EP_DIR_OUT) {
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

	LOG_DBG("ep %x max_data_len %u", ep, max_data_len);

	bytes = usbip_recv(data, max_data_len);

	if (read_bytes) {
		*read_bytes = bytes;
	}

	return 0;
}

int usb_dc_ep_read_continue(u8_t ep)
{
	u8_t ep_idx = USBIP_EP_ADDR2IDX(ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	/* Check if OUT ep */
	if (USBIP_EP_ADDR2DIR(ep) != USB_EP_DIR_OUT) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	if (!usbip_ctrl.out_ep_ctrl[ep_idx].data_len) {
		/* TODO: continue read */
		/* usbip_prep_rx(ep_idx, 0); */
	}

	return 0;
}

int usb_dc_ep_read(const u8_t ep, u8_t *const data,
		   const u32_t max_data_len, u32_t * const read_bytes)
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

int usb_dc_ep_set_callback(const u8_t ep, const usb_dc_ep_callback cb)
{
	u8_t ep_idx = USBIP_EP_ADDR2IDX(ep);

	LOG_DBG("ep %x callback %p", ep, cb);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (USBIP_EP_ADDR2DIR(ep) == USB_EP_DIR_IN) {
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

int usb_dc_ep_mps(const u8_t ep)
{
	u8_t ep_idx = USBIP_EP_ADDR2IDX(ep);

	LOG_DBG("ep %x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / Invalid endpoint: EP 0x%x", ep);
		return -EINVAL;
	}

	if (USBIP_EP_ADDR2DIR(ep) == USB_EP_DIR_OUT) {
		return usbip_ctrl.out_ep_ctrl[ep_idx].mps;
	} else {
		return usbip_ctrl.in_ep_ctrl[ep_idx].mps;
	}
}

int handle_usb_control(struct usbip_header *hdr)
{
	u8_t ep_idx = USBIP_EP_ADDR2IDX(ntohl(hdr->common.ep));
	usb_dc_ep_callback ep_cb = usbip_ctrl.out_ep_ctrl[ep_idx].cb;

	LOG_DBG("ep %x idx %u", ntohl(hdr->common.ep), ep_idx);

	if (ep_cb) {
		LOG_DBG("Call ep_cb");
		ep_cb(ntohl(hdr->common.ep), USB_DC_EP_SETUP);
	}

	return 0;
}

static void usbip_skip_setup(void)
{
	u64_t setup;

	LOG_DBG("Skip 8 bytes");

	usbip_recv((void *)&setup, sizeof(setup));
}

int handle_usb_data(struct usbip_header *hdr)
{
	u8_t ep_idx = ntohl(hdr->common.ep);
	usb_dc_ep_callback ep_cb;
	int bytes;
	u8_t ep;

	LOG_DBG("ep_idx %u", ep_idx);

	if (ntohl(hdr->common.direction) == USBIP_DIR_OUT) {
		if (ep_idx >= USBIP_OUT_EP_NUM) {
			return -EINVAL;
		}

		ep = ep_idx | USB_EP_DIR_OUT;
		ep_cb = usbip_ctrl.out_ep_ctrl[ep_idx].cb;

		ep_cb(ep, USB_DC_EP_DATA_OUT);

		/* Send ACK reply */
		bytes = usbip_send_common(ep, 0);
	} else {
		if (ep_idx >= USBIP_IN_EP_NUM) {
			return -EINVAL;
		}

		ep = ep_idx | USB_EP_DIR_IN;
		ep_cb = usbip_ctrl.in_ep_ctrl[ep_idx].cb;

		usbip_skip_setup();

		LOG_DBG("Send %u bytes", usbip_ctrl.in_ep_ctrl[ep_idx].buf_len);

		/* Send queued data */
		usbip_send_common(ep, usbip_ctrl.in_ep_ctrl[ep_idx].buf_len);
		usbip_send(ep, usbip_ctrl.in_ep_ctrl[ep_idx].buf,
			   usbip_ctrl.in_ep_ctrl[ep_idx].buf_len);

		LOG_HEXDUMP_DBG(usbip_ctrl.in_ep_ctrl[ep_idx].buf,
				usbip_ctrl.in_ep_ctrl[ep_idx].buf_len, ">");

		/* Indicate data sent */
		ep_cb(ep, USB_DC_EP_DATA_IN);
	}

	LOG_DBG("ep %x ep_cb %p", ep, ep_cb);

	return 0;
}
