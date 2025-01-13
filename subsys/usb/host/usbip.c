/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/net/socket.h>
#include <zephyr/net_buf.h>

#include <usbh_device.h>
#include <usbh_ch9.h>
#include <usbip.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbip, CONFIG_USBIP_LOG_LEVEL);

/* For now, we will use the Zephyr default controller. */
USBH_CONTROLLER_DEFINE(usbip_uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

#define USBIP_MAX_PKT_SIZE	2048
NET_BUF_POOL_DEFINE(usbip_pool, 32, USBIP_MAX_PKT_SIZE, 0, NULL);

K_THREAD_STACK_DEFINE(usbip_thread_stack, CONFIG_USBIP_THREAD_STACK_SIZE);
K_THREAD_STACK_ARRAY_DEFINE(dev_thread_stacks, CONFIG_USBIP_DEVICES_COUNT,
			    CONFIG_USBIP_THREAD_STACK_SIZE);

static struct k_thread usbip_thread;

#define USBIP_DEFAULT_PATH	"/sys/bus/usb/devices/usb1/1-"

#define USBIP_EXPORTED		BIT(0)

/* Context of the exported device */
struct usbip_dev_ctx {
	struct usb_device *udev;
	struct k_thread thread;
	struct k_event event;
	sys_dlist_t dlist;
	int connfd;
	uint32_t devid;
};

/* Context of the exported bus (not really used yet) */
struct usbip_bus_ctx {
	struct usbh_contex *uhs_ctx;
	struct usbip_dev_ctx devs[CONFIG_USBIP_DEVICES_COUNT];
	uint8_t busnum;
};

static struct usbip_bus_ctx default_bus_ctx;

/* Command reference structure to find the way back */
struct usbip_cmd_node {
	sys_dnode_t node;
	struct usbip_command cmd;
	struct usbip_dev_ctx *ctx;
	struct uhc_transfer *xfer;
};

K_MEM_SLAB_DEFINE(usbip_slab, sizeof(struct usbip_cmd_node),
		  CONFIG_USBIP_SUBMIT_BACKLOG_COUNT, sizeof(void *));

static void usbip_ntoh_command(struct usbip_command *const cmd)
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

static void check_ctrl_request(struct usb_device *const udev,
			       const int ep, const uint8_t *const setup_pkt)
{
	struct usb_setup_packet setup = {0};

	memcpy(&setup, setup_pkt, sizeof(struct usb_setup_packet));
	if (setup.RequestType.type != USB_REQTYPE_TYPE_STANDARD) {
		return;
	}

	if (setup.RequestType.direction) {
		return;
	}

	switch (setup.bRequest) {
	case USB_SREQ_SET_ADDRESS:
		LOG_INF("Set Address");
		break;
	case USB_SREQ_SET_CONFIGURATION:
		LOG_INF("Set Configuration");
		break;
	case USB_SREQ_SET_INTERFACE:
		LOG_INF("Set Interface");
		if (usbh_device_interface_set(udev, setup.wIndex, setup.wValue, true)) {
			LOG_ERR("Failed to apply Set Interface request");
		}
		break;
	default:
		break;
	}
}

static int usbip_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct usbip_cmd_node *const cmd_nd = xfer->priv;
	struct usbip_dev_ctx *const dev_ctx = cmd_nd->ctx;
	struct usbip_command *const cmd = &cmd_nd->cmd;
	struct net_buf *buf = xfer->buf;
	struct usbip_return ret;
	unsigned int key;
	int err;

	LOG_INF("SUBMIT seqnum %u finished err %d ep 0x%02x",
		cmd->hdr.seqnum, xfer->err, xfer->ep);

	ret.hdr.command = htonl(USBIP_RET_SUBMIT);
	ret.hdr.seqnum = htonl(cmd->hdr.seqnum);
	ret.hdr.devid = htonl(cmd->hdr.devid);
	ret.hdr.ep = htonl(xfer->ep);
	ret.hdr.direction = htonl(cmd->hdr.direction);

	memset(&ret.submit, 0, sizeof(ret.submit));
	ret.submit.status = htonl(xfer->err);
	ret.submit.start_frame = htonl(cmd->submit.start_frame);
	ret.submit.numof_iso_pkts = htonl(0xFFFFFFFFUL);

	if (xfer->err == -ECONNRESET) {
		LOG_INF("URB seqnum %u unlinked (ECONNRESET)", cmd->hdr.seqnum);
		goto usbip_req_cb_error;
	}

	if (xfer->err == -EPIPE) {
		LOG_INF("RET_SUBMIT status is EPIPE");
	}

	if (xfer->err == 0 && cmd->submit.length != 0) {
		ret.submit.actual_length = htonl(buf->len);
	}


	if (USB_EP_GET_IDX(xfer->ep) == 0U) {
		check_ctrl_request(dev_ctx->udev, xfer->ep, xfer->setup_pkt);
	}

	err = zsock_send(dev_ctx->connfd, &ret, sizeof(ret), 0);
	if (err != sizeof(ret)) {
		LOG_ERR("Send RET_SUBMIT failed err %d errno %d", err, errno);
		err = -errno;
		goto usbip_req_cb_error;
	}

	if (ret.submit.actual_length != 0) {
		LOG_INF("Send RET_SUBMIT transfer_buffer len %u", buf->len);
		err = zsock_send(dev_ctx->connfd, buf->data, buf->len, 0);
		if (err != buf->len) {
			LOG_ERR("Send transfer_buffer failed err %d errno %d",
				err, errno);
			err = -errno;
			goto usbip_req_cb_error;
		}
	}

usbip_req_cb_error:
	key = irq_lock();
	sys_dlist_remove(&cmd_nd->node);
	irq_unlock(key);

	k_mem_slab_free(&usbip_slab, (void *)cmd_nd);
	if (xfer->buf) {
		net_buf_unref(buf);
	}

	usbh_xfer_free(dev_ctx->udev, xfer);

	return 0;
}

static int usbip_submit_req(struct usbip_cmd_node *const cmd_nd, const uint8_t ep,
			    const struct usb_setup_packet *const setup,
			    struct net_buf *const buf)
{
	struct usbip_dev_ctx *const dev_ctx = cmd_nd->ctx;
	struct usbip_command *const cmd = &cmd_nd->cmd;
	struct usb_device *const udev = dev_ctx->udev;
	struct uhc_transfer *xfer;
	int ret;

	xfer = usbh_xfer_alloc(udev, ep, usbip_req_cb, cmd_nd);
	if (xfer == NULL) {
		return -ENOMEM;
	}

	if (setup != NULL) {
		memcpy(xfer->setup_pkt, setup, sizeof(struct usb_setup_packet));
		LOG_HEXDUMP_DBG(xfer->setup_pkt, 8U, "setup");
	}

	if (buf != NULL) {
		ret = usbh_xfer_buf_add(dev_ctx->udev, xfer, buf);
		if (ret) {
			goto submit_req_err;
		}
	}

	xfer->interval = cmd->submit.interval;
	cmd_nd->xfer = xfer;
	ret = usbh_xfer_enqueue(dev_ctx->udev, xfer);
	if (ret) {
		goto submit_req_err;
	}

	return 0;

submit_req_err:
	usbh_xfer_free(dev_ctx->udev, xfer);

	return ret;
}

static int usbip_handle_submit(struct usbip_dev_ctx *const dev_ctx,
			       struct usbip_command *const cmd)
{
	struct usbip_cmd_submit *submit = &cmd->submit;
	struct usb_setup_packet setup = {0};
	struct usbip_cmd_node *cmd_nd;
	struct net_buf *buf = NULL;
	uint8_t ep;
	int ret;

	ret = zsock_recv(dev_ctx->connfd, submit, sizeof(*submit), ZSOCK_MSG_WAITALL);
	if (ret <= 0) {
		return ret == 0 ? -ENOTCONN : -errno;
	}

	usbip_ntoh_command(cmd);

	ep = cmd->hdr.ep;
	if (cmd->submit.length != 0) {
		buf = net_buf_alloc(&usbip_pool, K_NO_WAIT);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate net_buf");
			return -ENOMEM;
		}
	}

	if (cmd->hdr.direction == USBIP_DIR_OUT && cmd->submit.length != 0) {
		/* Receive data */
		ret = zsock_recv(dev_ctx->connfd,
				 buf->data, cmd->submit.length,
				 ZSOCK_MSG_WAITALL);
		if (ret <= 0) {
			net_buf_unref(buf);
			return ret == 0 ? -ENOTCONN : -errno;
		}

		net_buf_add(buf, cmd->submit.length);
	}

	if (USB_EP_GET_IDX(ep) == 0U) {
		/* Setup packet */
		memcpy(&setup, cmd->submit.setup, sizeof(setup));
		ep = usb_reqtype_is_to_device(&setup) ? 0x00 : 0x80;
	} else {
		/* Set endpoint depending on direction */
		if (cmd->hdr.direction == USBIP_DIR_IN) {
			ep |= USB_EP_DIR_IN;
		}
	}

	LOG_INF("Handle SUBMIT devid %x seqnum %u length %u ep 0x%02x flags 0x%08x",
		cmd->hdr.devid, cmd->hdr.seqnum, cmd->submit.length, ep, cmd->submit.flags);

	if (k_mem_slab_alloc(&usbip_slab, (void **)&cmd_nd, K_MSEC(1000)) != 0) {
		LOG_ERR("Failed to allocate slab");
		net_buf_unref(buf);
		return -ENOMEM;
	}

	/* Make a copy of the command and add it to the backlog */
	memcpy(&cmd_nd->cmd, cmd, sizeof(struct usbip_command));
	cmd_nd->ctx = dev_ctx;
	sys_dlist_append(&dev_ctx->dlist, &cmd_nd->node);

	ret = usbip_submit_req(cmd_nd, ep, &setup, buf);
	if (ret != 0) {
		LOG_ERR("Failed to submit request %d", ret);
		return ret;
	}

	LOG_INF("Append %u ep 0x%02x to list", cmd->hdr.seqnum, ep);
	if (cmd->submit.length != 0) {
		LOG_HEXDUMP_DBG(buf->data, buf->len, "SUBMIT data");
	}

	return 0;
}

static int usbip_handle_unlink(struct usbip_dev_ctx *const dev_ctx,
			       struct usbip_command *const cmd)
{
	struct usbip_cmd_unlink *unlink = &cmd->unlink;
	struct usbip_return rsp;
	struct usbip_cmd_node *cmd_nd;
	unsigned int key;
	int ret;

	ret = zsock_recv(dev_ctx->connfd, unlink, sizeof(*unlink), ZSOCK_MSG_WAITALL);
	if (ret <= 0) {
		return ret == 0 ? -ENOTCONN : -errno;
	}

	memcpy(&rsp.hdr, &cmd->hdr, sizeof(rsp.hdr));
	rsp.hdr.command = htonl(USBIP_RET_UNLINK);

	usbip_ntoh_command(cmd);

	LOG_INF("Unlink request (seqnum %u) seqnum %u",
		cmd->hdr.seqnum, cmd->unlink.seqnum);

	memset(&rsp.unlink, 0, sizeof(rsp.unlink));

	key = irq_lock();
	SYS_DLIST_FOR_EACH_CONTAINER(&dev_ctx->dlist, cmd_nd, node) {
		if (cmd_nd->cmd.hdr.seqnum == cmd->unlink.seqnum) {
			rsp.unlink.status = htonl(-ECONNRESET);
			usbh_xfer_dequeue(dev_ctx->udev, cmd_nd->xfer);
			break;
		}
	}
	irq_unlock(key);

	ret = zsock_send(dev_ctx->connfd, &rsp, sizeof(rsp), 0);
	if (ret != sizeof(rsp)) {
		return -errno;
	}

	return 0;
}

static int usbip_handle_cmd(struct usbip_dev_ctx *const dev_ctx)
{
	struct usbip_command cmd;
	int ret;

	ret = zsock_recv(dev_ctx->connfd, &cmd.hdr, sizeof(cmd.hdr), ZSOCK_MSG_WAITALL);
	if (ret <= 0) {
		return ret == 0 ? -ENOTCONN : -errno;
	}

	LOG_HEXDUMP_DBG((uint8_t *)&cmd.hdr, sizeof(cmd.hdr), "cmd.hdr");

	switch (ntohl(cmd.hdr.command)) {
	case USBIP_CMD_SUBMIT:
		ret = usbip_handle_submit(dev_ctx, &cmd);
		break;
	case USBIP_CMD_UNLINK:
		ret = usbip_handle_unlink(dev_ctx, &cmd);
		break;
	default:
		LOG_ERR("Unknown command: 0x%x", ntohl(cmd.hdr.command));
		break;
	}

	return ret;
}

static void usbip_thread_cmd(void *const a, void *const b, void *const c)
{
	struct usbip_dev_ctx *dev_ctx = a;
	int ret;

	LOG_INF("CMD thread started");
	while (true) {
		k_event_wait(&dev_ctx->event, USBIP_EXPORTED, false, K_FOREVER);

		ret = usbip_handle_cmd(dev_ctx);

		if (ret) {
			zsock_close(dev_ctx->connfd);
			LOG_INF("CMD connection closed, errno %d", ret);
			k_event_set_masked(&dev_ctx->event, 0, USBIP_EXPORTED);
			dev_ctx->udev = NULL;
		}
	}
}

static int handle_devlist_device(struct usb_device *const udev,
				 const uint32_t busnum, int connfd)
{
	struct usb_device_descriptor *d_desc = &udev->dev_desc;
	struct usb_cfg_descriptor *c_desc = udev->cfg_desc;
	static struct usbip_devlist_data devlist;
	const uint32_t devnum = udev->addr;
	int err;

	devlist.busnum = htonl(busnum);
	devlist.devnum = htonl(devnum);

	devlist.speed = htonl(udev->speed);
	devlist.idVendor = htons(d_desc->idVendor);
	devlist.idProduct = htons(d_desc->idProduct);
	devlist.bcdDevice = htons(d_desc->bcdDevice);
	devlist.bDeviceClass = d_desc->bDeviceClass;
	devlist.bDeviceSubClass = d_desc->bDeviceSubClass;
	devlist.bDeviceProtocol = d_desc->bDeviceProtocol;

	devlist.bConfigurationValue = c_desc->bConfigurationValue;
	devlist.bNumConfigurations = d_desc->bNumConfigurations;
	devlist.bNumInterfaces = c_desc->bNumInterfaces;

	snprintf(devlist.path, sizeof(devlist.path), USBIP_DEFAULT_PATH "%d", devnum);
	snprintf(devlist.busid, sizeof(devlist.busid), "1-%d", devnum);

	LOG_INF("bLength\t\t\t%u", c_desc->bLength);
	LOG_INF("bDescriptorType\t\t%u", c_desc->bDescriptorType);
	LOG_INF("wTotalLength\t\t%u", c_desc->wTotalLength);
	LOG_INF("bNumInterfaces\t\t%u", c_desc->bNumInterfaces);
	LOG_INF("bConfigurationValue\t%u", c_desc->bConfigurationValue);
	LOG_INF("iConfiguration\t\t%u", c_desc->iConfiguration);
	LOG_INF("bmAttributes\t\t%02x", c_desc->bmAttributes);
	LOG_INF("bMaxPower\t\t%u mA", c_desc->bMaxPower * 2);

	err = zsock_send(connfd, &devlist, sizeof(devlist), 0);
	if (err != sizeof(devlist)) {
		return -errno;
	}

	return 0;
}

static int handle_devlist_device_iface(struct usb_device *const udev, int connfd)
{
	struct usb_cfg_descriptor *c_desc = udev->cfg_desc;
	static struct usbip_devlist_iface_data iface;
	int err;

	LOG_INF("Handle OP_REQ_DEVLIST, bNumInterfaces %u wTotalLength %u",
		c_desc->bNumInterfaces, c_desc->wTotalLength);

	for (unsigned int i = 0; i < c_desc->bNumInterfaces; i++) {
		struct usb_if_descriptor *if_d = (void *)udev->ifaces[i].dhp;

		LOG_DBG("bInterfaceNumber %u", if_d->bInterfaceNumber);

		iface.bInterfaceClass = if_d->bInterfaceClass;
		iface.bInterfaceSubClass = if_d->bInterfaceSubClass;
		iface.bInterfaceProtocol = if_d->bInterfaceProtocol;
		iface.padding = 0U;

		err = zsock_send(connfd, &iface, sizeof(iface), 0);
		if (err != sizeof(iface)) {
			LOG_ERR("Failed to send interface info %d", err);
			return -errno;
		}
	}

	return 0;
}

static int usbip_handle_devlist(struct usbip_bus_ctx *const bus_ctx, int connfd)
{
	struct usbip_devlist_header rep_hdr = {
		.version = htons(USBIP_VERSION),
		.code = htons(USBIP_OP_REP_DEVLIST),
		.status = 0,
		.ndev = htonl(1),
	};
	struct usb_device *udev;
	uint32_t ndev = 0;
	int err;

	SYS_DLIST_FOR_EACH_CONTAINER(&bus_ctx->uhs_ctx->udevs, udev, node) {
		ndev++;
	}

	rep_hdr.ndev = htonl(ndev);
	/* Send reply header with the number of USB devices  */
	err = zsock_send(connfd, &rep_hdr, sizeof(rep_hdr), 0);
	if (err != sizeof(rep_hdr)) {
		return -errno;
	}

	/* Send device info for each USB devices */
	SYS_DLIST_FOR_EACH_CONTAINER(&bus_ctx->uhs_ctx->udevs, udev, node) {
		err = handle_devlist_device(udev, bus_ctx->busnum, connfd);
		if (err) {
			return err;
		}

		err = handle_devlist_device_iface(udev, connfd);
		if (err) {
			return err;
		}
	}

	return 0;
}

static struct usb_device *get_device_by_busid(struct usbip_bus_ctx *const bus_ctx,
					      const char busid[static 32])
{
	struct usb_device *udev;
	char my_busid[32];

	LOG_HEXDUMP_DBG(busid, sizeof(my_busid), "import busid");

	SYS_DLIST_FOR_EACH_CONTAINER(&bus_ctx->uhs_ctx->udevs, udev, node) {
		snprintf(my_busid, sizeof(my_busid), "1-%d", udev->addr);

		LOG_HEXDUMP_DBG(my_busid, sizeof(my_busid), "my busid");
		if (!strncmp(busid, my_busid, sizeof(my_busid))) {
			return udev;
		}
	}

	return NULL;
}

static struct usbip_dev_ctx *get_free_dev_ctx(struct usbip_bus_ctx *const bus_ctx)
{
	for (unsigned int i = 0; i < CONFIG_USBIP_DEVICES_COUNT; i++) {
		struct usbip_dev_ctx *const dev_ctx = &bus_ctx->devs[i];

		if (k_event_wait(&dev_ctx->event, USBIP_EXPORTED, false, K_NO_WAIT)) {
			continue;
		}

		if (dev_ctx->udev != NULL) {
			LOG_WRN("USB device pointer is not cleaned");
		}

		return &bus_ctx->devs[i];
	}

	return NULL;
}

static int usbip_handle_import(struct usbip_bus_ctx *const bus_ctx, int connfd)
{
	struct usbip_req_header rep_hdr = {
		.version = htons(USBIP_VERSION),
		.code = htons(USBIP_OP_REP_IMPORT),
		.status = 0,
	};
	struct usbip_dev_ctx *dev_ctx;
	uint8_t busid[32];
	int ret;

	ret = zsock_recv(connfd, &busid, sizeof(busid), ZSOCK_MSG_WAITALL);
	if (ret <= 0) {
		return ret == 0 ? -ENOTCONN : -errno;
	}

	dev_ctx = get_free_dev_ctx(bus_ctx);
	if (dev_ctx == NULL) {
		rep_hdr.status = htonl(-1);
		LOG_ERR("No free device context to export a device");
	} else {
		dev_ctx->udev = get_device_by_busid(bus_ctx, busid);
		if (dev_ctx->udev == NULL) {
			rep_hdr.status = htonl(-1);
			dev_ctx = NULL;
			LOG_ERR("No USB device with busid %s", busid);
		}
	}

	ret = zsock_send(connfd, &rep_hdr, sizeof(rep_hdr), 0);
	if (ret != sizeof(rep_hdr)) {
		return -errno;
	}

	if (rep_hdr.status || dev_ctx == NULL) {
		LOG_ERR("Device does not exits or cannot be exported");
		return -1;
	}

	ret = handle_devlist_device(dev_ctx->udev, bus_ctx->busnum, connfd);
	if (ret) {
		return ret;
	}

	dev_ctx->connfd = connfd;
	k_event_post(&dev_ctx->event, USBIP_EXPORTED);
	LOG_INF("USB device %s exported", busid);

	return 0;
}

static int usbip_handle_connection(struct usbip_bus_ctx *const bus_ctx, int connfd)
{
	struct usbip_req_header hdr;
	int ret;

	ret = zsock_recv(connfd, &hdr, sizeof(hdr), ZSOCK_MSG_WAITALL);
	if (ret <= 0) {
		return ret == 0 ? -ENOTCONN : -errno;
	}

	LOG_HEXDUMP_DBG((uint8_t *)&hdr, sizeof(hdr), "header");
	LOG_INF("Code: 0x%x", ntohs(hdr.code));

	switch (ntohs(hdr.code)) {
	case USBIP_OP_REQ_DEVLIST:
		ret = usbip_handle_devlist(bus_ctx, connfd);
		zsock_close(connfd);
		break;
	case USBIP_OP_REQ_IMPORT:
		ret = usbip_handle_import(bus_ctx, connfd);
		if (ret) {
			zsock_close(connfd);
		}
		break;
	default:
		LOG_ERR("Unknown request: 0x%x", ntohs(hdr.code));
		ret = -1;
		break;
	}

	return ret;
}

static void usbip_thread_handler(void *const a, void *const b, void *const c)
{
	struct usbip_bus_ctx *const bus_ctx = a;
	struct sockaddr_in srv;
	int listenfd;
	int connfd;
	int reuse = 1;

	LOG_DBG("Started connection handling thread");

	listenfd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenfd < 0) {
		LOG_ERR("socket() failed: %s", strerror(errno));
		return;
	}

	if (zsock_setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
			     (const char *)&reuse, sizeof(reuse)) < 0) {
		LOG_INF("setsockopt() failed: %s", strerror(errno));
	}

	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = htonl(INADDR_ANY);
	srv.sin_port = htons(USBIP_PORT);

	if (zsock_bind(listenfd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
		LOG_ERR("bind() failed: %s", strerror(errno));
		return;
	}

	if (zsock_listen(listenfd, 1) < 0) {
		LOG_ERR("listen() failed: %s", strerror(errno));
		return;
	}

	while (true) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		char addr_str[INET_ADDRSTRLEN];
		int err;

		connfd = zsock_accept(listenfd, (struct sockaddr *)&client_addr,
				      &client_addr_len);
		if (connfd < 0) {
			LOG_ERR("accept() failed: %d", errno);
			continue;
		}

		zsock_inet_ntop(client_addr.sin_family, &client_addr.sin_addr,
				addr_str, sizeof(addr_str));
		LOG_INF("Connection: %s", addr_str);

		err = usbip_handle_connection(bus_ctx, connfd);
		LOG_INF("Connection from %s closed, errno %d", addr_str, err);
	}
}

/*
 * We are just using a standard host controller, which is fine to get USBIP
 * support working and stable, but it needs a better solution in the future.
 */
static int usbip_init(void)
{
	struct usbip_bus_ctx *const bus_ctx = &default_bus_ctx;
	int err;

	err = usbh_init(&usbip_uhs_ctx);
	if (err) {
		LOG_ERR("Failed to initialize host support");
		return err;
	}

	err = usbh_enable(&usbip_uhs_ctx);
	if (err) {
		LOG_ERR("Failed to enable host support");
		return err;
	}

	err = uhc_sof_enable(usbip_uhs_ctx.dev);
	if (err) {
		LOG_ERR("Failed to start SoF");
		return err;
	}

	LOG_INF("Host controller enabled");
	bus_ctx->uhs_ctx = &usbip_uhs_ctx;
	bus_ctx->busnum = 1;

	for (int i = 0; i < CONFIG_USBIP_DEVICES_COUNT; i++) {
		struct usbip_dev_ctx *ctx = &bus_ctx->devs[i];

		/* busnum << 16 | devnum */
		ctx->devid = (1U << 16) | i;
		sys_dlist_init(&ctx->dlist);
		k_event_init(&ctx->event);
		k_thread_create(&ctx->thread, dev_thread_stacks[i],
				K_THREAD_STACK_SIZEOF(dev_thread_stacks[i]),
				usbip_thread_cmd, ctx, NULL, NULL,
				K_PRIO_COOP(3), 0, K_NO_WAIT);
	}

	k_thread_create(&usbip_thread, usbip_thread_stack,
			K_THREAD_STACK_SIZEOF(usbip_thread_stack),
			usbip_thread_handler, bus_ctx, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);

	return 0;
}

SYS_INIT(usbip_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
