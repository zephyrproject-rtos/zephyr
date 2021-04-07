/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr.h>
#include <kernel.h>
#include <sys/byteorder.h>
#include <usb/usb_common.h>
#include <usb/usbstruct.h>
#include <usb/usb_device.h>
#include <drivers/usb/usb_dc.h>
#include <net/socket.h>

#include "usb_dc_usbip.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(usbip, CONFIG_USB_DRIVER_LOG_LEVEL);

extern struct usb_desc_header __usb_descriptor_start[];

#define USBIP_IN_EP_NUM		8
#define USBIP_OUT_EP_NUM	8
#define USBIP_MAX_PACKET_SIZE	64

K_KERNEL_STACK_MEMBER(usbip_thread_stack, 2048);

static struct k_thread usbip_thread;
static bool attached;

struct usb_ep_ctrl_prv {
	uint8_t ep_ena;
	uint16_t mps;
	usb_dc_ep_callback cb;
	uint32_t data_len;
	uint8_t buf[64];
	uint8_t buf_len;
};

static struct usbip_ctrl_prv {
	usb_dc_status_callback status_cb;
	struct usb_ep_ctrl_prv in_ep_ctrl[USBIP_IN_EP_NUM];
	struct usb_ep_ctrl_prv out_ep_ctrl[USBIP_OUT_EP_NUM];
	int connfd;
	int seqnum;
	int devid;
	uint32_t cmd_num_in;
	uint32_t cmd_num_out;
	uint8_t attached;
} usbip_ctrl;

static struct usbip_devlist_data devlist_data = {
	.path = "/sys/devices/pci0000:00/0000:00:01.2/usb1/1-1",
	.busid = "1-1",
	.busnum = htonl(1),
	.devnum = htonl(2),
	.speed = htonl(2),
};

static int usbip_recv(uint8_t *buf, size_t len);
static int usbip_send(uint8_t ep, const uint8_t *data, size_t len);
static bool usbip_send_common(uint8_t ep, uint32_t data_len);

static bool usbip_ep_is_valid(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (USB_EP_DIR_IS_OUT(ep) && ep_idx < USBIP_OUT_EP_NUM) {
		return true;
	}

	if (USB_EP_DIR_IS_IN(ep) && ep_idx < USBIP_IN_EP_NUM) {
		return true;
	}

	return false;
}

static bool usbip_ep_is_enabled(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (USB_EP_DIR_IS_OUT(ep) && usbip_ctrl.out_ep_ctrl[ep_idx].ep_ena) {
		return true;
	}

	if (USB_EP_DIR_IS_IN(ep) && usbip_ctrl.in_ep_ctrl[ep_idx].ep_ena) {
		return true;
	}

	return false;
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
	LOG_DBG("Set address %u", addr);

	return 0;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);

	LOG_DBG("ep 0x%02x, mps %u, type %u", cfg->ep_addr, cfg->ep_mps,
		cfg->ep_type);

	if ((cfg->ep_type == USB_DC_EP_CONTROL) && ep_idx) {
		LOG_ERR("Wrong endpoint configuration");
		return -1;
	}

	if (cfg->ep_mps > USBIP_MAX_PACKET_SIZE) {
		LOG_WRN("Unsupported packet size");
		return -1;
	}

	if (USB_EP_DIR_IS_OUT(cfg->ep_addr) && (ep_idx >= USBIP_OUT_EP_NUM)) {
		LOG_WRN("OUT endpoint address out of range");
		return -1;
	}

	if (USB_EP_DIR_IS_IN(cfg->ep_addr) && (ep_idx >= USBIP_IN_EP_NUM)) {
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
		LOG_ERR("Not attached / wrong endpoint 0x%x", ep);
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
	LOG_DBG("ep 0x%02x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / wrong endpoint: 0x%x", ep);
		return -EINVAL;
	}

	/* Use standard reply for now */
	usb_dc_ep_write(0x80,  NULL, 0, NULL);

	return 0;
}

int usb_dc_ep_clear_stall(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	LOG_DBG("ep 0x%02x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / wrong endpoint: 0x%x", ep);
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

	LOG_DBG("Halt ep 0x%02x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / wrong endpoint: 0x%x", ep);
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
	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / wrong endpoint: 0x%x", ep);
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

	LOG_DBG("Enable endpoint 0x%02x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / wrong endpoint: 0x%x", ep);
		return -EINVAL;
	}

	/* Enable endpoint */
	if (USB_EP_DIR_IS_OUT(ep)) {
		usbip_ctrl.out_ep_ctrl[ep_idx].ep_ena = 1U;
	} else {
		usbip_ctrl.in_ep_ctrl[ep_idx].ep_ena = 1U;
	}

	return 0;
}

int usb_dc_ep_disable(const uint8_t ep)
{
	LOG_DBG("Disable endpoint 0x%02x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / wrong endpoint: 0x%x", ep);
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_flush(const uint8_t ep)
{
	LOG_DBG("Flush endpoint 0x%02x", ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / wrong endpoint: 0x%x", ep);
		return -EINVAL;
	}

	LOG_WRN("Flush endpoint 0x%02x not supported", ep);

	return 0;
}

int usb_dc_ep_write(const uint8_t ep, const uint8_t *const data,
		    const uint32_t data_len, uint32_t * const ret_bytes)
{
	LOG_DBG("ep 0x%02x len %u", ep, data_len);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / wrong endpoint: 0x%x", ep);
		return -EINVAL;
	}

	/* Check if IN ep */
	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_IN) {
		return -EINVAL;
	}

	/* Check if ep enabled */
	if (!usbip_ep_is_enabled(ep)) {
		LOG_WRN("Endpoint 0x%02x is not enabled", ep);
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
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint32_t to_copy;

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / wrong endpoint: 0x%x", ep);
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
		LOG_WRN("Endpoint 0x%02x is not enabled", ep);
		return -EINVAL;
	}

	if (data == NULL && max_data_len == 0 && read_bytes != NULL) {
		/* Return length of the available data in endpoint buffer */
		*read_bytes = usbip_ctrl.out_ep_ctrl[ep_idx].data_len;
		return 0;
	}

	to_copy = MIN(usbip_ctrl.out_ep_ctrl[ep_idx].data_len, max_data_len);
	LOG_DBG("ep 0x%02x, to_copy %u", ep, to_copy);
	memcpy(data, usbip_ctrl.out_ep_ctrl[ep_idx].buf, to_copy);

	if (read_bytes) {
		*read_bytes = to_copy;
	}

	return 0;
}

int usb_dc_ep_read_continue(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / wrong endpoint: 0x%x", ep);
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
	LOG_DBG("ep 0x%02x max_data_len %u", ep, max_data_len);

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

	LOG_DBG("Endpoint 0x%02x callback %p", ep, cb);

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / wrong endpoint: 0x%x", ep);
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

	if (!usbip_ctrl.attached || !usbip_ep_is_valid(ep)) {
		LOG_ERR("Not attached / wrong endpoint: 0x%x", ep);
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		return usbip_ctrl.out_ep_ctrl[ep_idx].mps;
	} else {
		return usbip_ctrl.in_ep_ctrl[ep_idx].mps;
	}
}

static int usbip_handle_control(struct usbip_command *cmd)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cmd->hdr.ep);
	struct usb_ep_ctrl_prv *ep_ctrl;

	ep_ctrl = &usbip_ctrl.out_ep_ctrl[ep_idx];
	if (ep_ctrl->cb == NULL) {
		LOG_ERR("Control endpoint callback not set");
		return -EIO;
	}

	if ((cmd->hdr.direction == USBIP_DIR_IN) ^
	    (REQTYPE_GET_DIR(cmd->submit.bmRequestType) ==
	     REQTYPE_DIR_TO_HOST)) {
		LOG_ERR("Failed to verify bmRequestType");
		return -EIO;
	}

	ep_ctrl->data_len = 8;
	LOG_DBG("SETUP event ep 0x%02x %u", ep_idx, ep_ctrl->data_len);
	memcpy(ep_ctrl->buf, &cmd->submit.bmRequestType, ep_ctrl->data_len);
	ep_ctrl->cb(ep_idx, USB_DC_EP_SETUP);

	if (cmd->hdr.direction == USBIP_DIR_OUT) {
		/* Data OUT stage availably */
		ep_ctrl->data_len = cmd->submit.length;
		usbip_recv(ep_ctrl->buf, ep_ctrl->data_len);
		LOG_DBG("DATA OUT event ep 0x%02x %u",
			ep_idx, ep_ctrl->data_len);
		ep_ctrl->cb(ep_idx, USB_DC_EP_DATA_OUT);
	}

	return 0;
}

static int usbip_handle_data(struct usbip_command *cmd)
{
	uint8_t ep_idx = cmd->hdr.ep;
	struct usb_ep_ctrl_prv *ep_ctrl;
	uint8_t ep;

	if (cmd->hdr.direction == USBIP_DIR_OUT) {
		if (ep_idx >= USBIP_OUT_EP_NUM) {
			return -EINVAL;
		}

		ep_ctrl = &usbip_ctrl.out_ep_ctrl[ep_idx];
		ep = ep_idx | USB_EP_DIR_OUT;
		ep_ctrl->data_len = cmd->submit.length;
		usbip_recv(ep_ctrl->buf, ep_ctrl->data_len);
		LOG_DBG("DATA OUT event ep 0x%02x %u", ep, ep_ctrl->data_len);

		ep_ctrl->cb(ep, USB_DC_EP_DATA_OUT);

		/* Send ACK reply */
		if (!usbip_send_common(ep, 0)) {
			return -EIO;
		}
	} else {
		if (ep_idx >= USBIP_IN_EP_NUM) {
			return -EINVAL;
		}

		ep_ctrl = &usbip_ctrl.in_ep_ctrl[ep_idx];
		ep = ep_idx | USB_EP_DIR_IN;
		LOG_DBG("DATA IN event ep 0x%02x %u", ep, ep_ctrl->buf_len);

		/* Send queued data */
		if (!usbip_send_common(ep, ep_ctrl->buf_len)) {
			return -EIO;
		}

		if (usbip_send(ep, ep_ctrl->buf, ep_ctrl->buf_len) !=
		    ep_ctrl->buf_len) {
			return -EIO;
		}

		LOG_HEXDUMP_DBG(ep_ctrl->buf, ep_ctrl->buf_len, ">");

		/*
		 * Call the callback only if data in usb_dc_ep_write()
		 * is actually written to the intermediate buffer and sent.
		 */
		if (ep_ctrl->buf_len != 0) {
			ep_ctrl->cb(ep, USB_DC_EP_DATA_IN);
			usbip_ctrl.in_ep_ctrl[ep_idx].buf_len = 0;
		}
	}

	return 0;
}

static int usbip_recv(uint8_t *buf, size_t len)
{
	return recv(usbip_ctrl.connfd, buf, len, MSG_WAITALL);
}

static int usbip_send(uint8_t ep, const uint8_t *data, size_t len)
{
	return send(usbip_ctrl.connfd, data, len, 0);
}

static bool usbip_send_common(uint8_t ep, uint32_t data_len)
{
	struct usbip_return rsp;
	uint32_t ep_dir = USB_EP_DIR_IS_IN(ep) ? USBIP_DIR_IN : USBIP_DIR_OUT;
	uint32_t ep_idx = USB_EP_GET_IDX(ep);
	int rc;

	rsp.hdr.command = htonl(USBIP_RET_SUBMIT);
	rsp.hdr.seqnum = htonl(usbip_ctrl.connfd);
	rsp.hdr.devid = htonl(0);
	rsp.hdr.direction = htonl(ep_dir);
	rsp.hdr.ep = htonl(ep_idx);

	memset(&rsp.submit, 0, sizeof(rsp.submit));
	rsp.submit.actual_length = htonl(data_len);

	rc = send(usbip_ctrl.connfd, &rsp, sizeof(rsp), 0);
	if (rc != sizeof(rsp)) {
		LOG_ERR("Failed to send response header, errno %d", errno);
		return false;
	}

	usbip_ctrl.cmd_num_out++;

	return true;
}

static void usbip_init_devlist_data(void)
{
	struct usb_device_descriptor *dev_desc;
	struct usb_cfg_descriptor *cfg_desc;

	dev_desc = (void *)__usb_descriptor_start;
	cfg_desc = (void *)((uint8_t *)dev_desc +
			    sizeof(struct usb_device_descriptor));

	devlist_data.idVendor = htons(dev_desc->idVendor);
	devlist_data.idProduct = htons(dev_desc->idProduct);
	devlist_data.bcdDevice = htons(dev_desc->bcdDevice);
	devlist_data.bDeviceClass = dev_desc->bDeviceClass;
	devlist_data.bDeviceSubClass = dev_desc->bDeviceSubClass;
	devlist_data.bDeviceProtocol = dev_desc->bDeviceProtocol;

	devlist_data.bConfigurationValue = cfg_desc->bConfigurationValue;
	devlist_data.bNumConfigurations = dev_desc->bNumConfigurations;
	devlist_data.bNumInterfaces = cfg_desc->bNumInterfaces;
}

static int usbip_handle_devlist(int connfd)
{
	struct usb_desc_header *desc_hdr = __usb_descriptor_start;
	struct usb_if_descriptor *if_desc = NULL;
	struct usbip_devlist_iface_data iface;
	struct usbip_devlist_header rep_hdr = {
		.version = htons(USBIP_VERSION),
		.code = htons(USBIP_OP_REP_DEVLIST),
		.status = 0,
		.ndev = htonl(1),
	};
	int rc;

	LOG_DBG("Handle OP_REQ_DEVLIST");

	rc = send(connfd, &rep_hdr, sizeof(rep_hdr), 0);
	if (rc != sizeof(rep_hdr)) {
		return -errno;
	}

	rc = send(connfd, &devlist_data, sizeof(devlist_data), 0);
	if (rc != sizeof(devlist_data)) {
		return -errno;
	}

	while (desc_hdr->bLength != 0U) {
		if (desc_hdr->bDescriptorType == USB_INTERFACE_DESC) {
			if_desc = (struct usb_if_descriptor *)desc_hdr;

			iface.bInterfaceClass = if_desc->bInterfaceClass;
			iface.bInterfaceSubClass = if_desc->bInterfaceSubClass;
			iface.bInterfaceProtocol = if_desc->bInterfaceProtocol;
			iface.padding = 0U;

			rc = send(connfd, &iface, sizeof(iface), 0);
			if (rc != sizeof(iface)) {
				return -errno;
			}
		}

		/* Move to next descriptor */
		desc_hdr = (struct usb_desc_header *)((uint8_t *)desc_hdr +
						      desc_hdr->bLength);
	}

	return 0;
}

static int usbip_handle_import(int connfd)
{
	struct usbip_req_header rep_hdr = {
		.version = htons(USBIP_VERSION),
		.code = htons(USBIP_OP_REP_IMPORT),
		.status = 0,
	};
	char busid[32];
	int rc;

	LOG_DBG("Handle OP_REQ_IMPORT");

	rc = recv(connfd, &busid, sizeof(busid), MSG_WAITALL);
	if (rc <= 0) {
		return rc == 0 ? -ENOTCONN : -errno;
	}

	LOG_HEXDUMP_DBG(busid, sizeof(busid), "busid");

	rc = send(connfd, &rep_hdr, sizeof(rep_hdr), 0);
	if (rc != sizeof(rep_hdr)) {
		return -errno;
	}

	rc = send(connfd, &devlist_data, sizeof(devlist_data), 0);
	if (rc != sizeof(devlist_data)) {
		return -errno;
	}

	usbip_ctrl.cmd_num_in = 0;
	usbip_ctrl.cmd_num_out = 0;

	return 0;
}

static void usbip_ntoh_submit(struct usbip_command *cmd)
{
	cmd->hdr.command = ntohl(cmd->hdr.command);
	cmd->hdr.seqnum = ntohl(cmd->hdr.seqnum);
	cmd->hdr.devid = ntohl(cmd->hdr.devid);
	cmd->hdr.direction = ntohl(cmd->hdr.direction);
	cmd->hdr.ep = ntohl(cmd->hdr.ep);

	if (cmd->hdr.command == USBIP_CMD_SUBMIT) {
		cmd->submit.flags = ntohl(cmd->submit.flags);
		cmd->submit.length = ntohl(cmd->submit.length);
		cmd->submit.start_frame = ntohl(cmd->submit.start_frame);
		cmd->submit.numof_iso_pkts = ntohl(cmd->submit.numof_iso_pkts);
		cmd->submit.interval = ntohl(cmd->submit.interval);
	} else {
		cmd->unlink.seqnum = ntohl(cmd->unlink.seqnum);
	}
}

static int usbip_handle_submit(int connfd, struct usbip_command *cmd)
{
	struct usbip_cmd_submit *req = &cmd->submit;
	int rc;

	rc = recv(connfd, req, sizeof(*req), MSG_WAITALL);
	if (rc <= 0) {
		return rc == 0 ? -ENOTCONN : -errno;
	}

	usbip_ntoh_submit(cmd);
	usbip_ctrl.devid = cmd->hdr.devid;
	usbip_ctrl.connfd = cmd->hdr.seqnum;

	LOG_DBG("Handle submit request seqnum %u ep 0x%02x",
		cmd->hdr.seqnum, cmd->hdr.ep);
	usbip_ctrl.cmd_num_in++;

	if (ntohl(cmd->hdr.ep) == 0) {
		usbip_handle_control(cmd);
	} else {
		usbip_handle_data(cmd);
	}

	LOG_DBG("Handle submit request finished");

	return 0;
}

static int usbip_handle_unlink(int connfd, struct usbip_command *cmd)
{
	struct usbip_cmd_unlink *req = &cmd->unlink;
	struct usbip_return rsp;
	int rc;

	rc = recv(connfd, req, sizeof(*req), MSG_WAITALL);
	if (rc <= 0) {
		return rc == 0 ? -ENOTCONN : -errno;
	}

	memcpy(&rsp.hdr, &cmd->hdr, sizeof(rsp.hdr));
	rsp.hdr.command = htonl(USBIP_RET_UNLINK);

	usbip_ntoh_submit(cmd);
	usbip_ctrl.devid = cmd->hdr.devid;
	usbip_ctrl.connfd = cmd->hdr.seqnum;

	LOG_INF("Unlink request seqnum %u ep 0x%02x seqnum %u",
		cmd->hdr.seqnum, cmd->hdr.ep, cmd->unlink.seqnum);

	/* Reply success to any unlink command */
	memset(&rsp.unlink, 0, sizeof(rsp.unlink));

	rc = send(usbip_ctrl.connfd, &rsp, sizeof(rsp), 0);
	if (rc != sizeof(rsp)) {
		return -errno;
	}

	return 0;
}

static int usbip_handle_connection(int connfd)
{
	struct usbip_command cmd;
	int rc;

	if (attached == false) {
		struct usbip_req_header req;

		rc = recv(connfd, &req, sizeof(req), MSG_WAITALL);
		if (rc <= 0) {
			return rc == 0 ? -ENOTCONN : -errno;
		}

		LOG_HEXDUMP_DBG((uint8_t *)&req, sizeof(req), "request");
		LOG_DBG("Code: 0x%x", ntohs(req.code));

		switch (ntohs(req.code)) {
		case USBIP_OP_REQ_DEVLIST:
			rc = usbip_handle_devlist(connfd);
			break;
		case USBIP_OP_REQ_IMPORT:
			rc = usbip_handle_import(connfd);
			attached = rc ? false : true;
			break;
		default:
			LOG_ERR("Unknown request: 0x%x", ntohs(req.code));
			rc = -1;
			break;
		}

	} else {

		rc = recv(connfd, &cmd.hdr, sizeof(cmd.hdr), MSG_WAITALL);
		if (rc <= 0) {
			return rc == 0 ? -ENOTCONN : -errno;
		}

		LOG_HEXDUMP_DBG((uint8_t *)&cmd.hdr, sizeof(cmd.hdr), "cmd.hdr");

		switch (ntohl(cmd.hdr.command)) {
		case USBIP_CMD_SUBMIT:
			rc = usbip_handle_submit(connfd, &cmd);
			break;
		case USBIP_CMD_UNLINK:
			rc = usbip_handle_unlink(connfd, &cmd);
			break;
		default:
			LOG_ERR("Unknown command: 0x%x", ntohl(cmd.hdr.command));
			attached = false;
			rc = -1;
			break;
		}
	}

	return rc;
}

static void usbip_thread_handler(void *a, void *b, void *c)
{
	struct sockaddr_in srv;
	int listenfd;
	int reuse = 1;

	LOG_DBG("Starting");
	usbip_init_devlist_data();

	listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenfd < 0) {
		LOG_ERR("socket() failed: %s", strerror(errno));
		return;
	}

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
		       (const char *)&reuse, sizeof(reuse)) < 0) {
		LOG_WRN("setsockopt() failed: %s", strerror(errno));
	}

	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = htonl(INADDR_ANY);
	srv.sin_port = htons(USBIP_PORT);

	if (bind(listenfd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
		LOG_ERR("bind() failed: %s", strerror(errno));
		return;
	}

	if (listen(listenfd, 1) < 0) {
		LOG_ERR("listen() failed: %s", strerror(errno));
		return;
	}

	while (true) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		char addr_str[INET_ADDRSTRLEN];
		int connfd;
		int rc;

		connfd = accept(listenfd, (struct sockaddr *)&client_addr,
				&client_addr_len);

		if (connfd < 0) {
			LOG_ERR("accept() failed: %d", errno);
			continue;
		}

		usbip_ctrl.connfd = connfd;
		inet_ntop(client_addr.sin_family, &client_addr.sin_addr,
			  addr_str, sizeof(addr_str));
		LOG_INF("Connection: %s", addr_str);

		do {
			rc = usbip_handle_connection(connfd);
		} while (!rc);


		LOG_INF("Connection from %s closed, errno %d, in %d out %d",
			addr_str, rc, usbip_ctrl.cmd_num_in,
			usbip_ctrl.cmd_num_out);
		attached = false;
		close(connfd);
	}
}

int usb_dc_attach(void)
{
	if (usbip_ctrl.attached) {
		LOG_WRN("Already attached");
		return 0;
	}

	k_thread_create(&usbip_thread, usbip_thread_stack,
			sizeof(usbip_thread_stack),
			usbip_thread_handler, NULL, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);

	usbip_ctrl.attached = 1U;

	return 0;
}

int usb_dc_detach(void)
{
	if (!usbip_ctrl.attached) {
		return 0;
	}

	usbip_ctrl.attached = 0U;

	return 0;
}
