/*
 * Copyright (c) 2026 jeck chen <baidxi404629@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Fastboot class implementation
 *
 * Implements Android Fastboot protocol as a USB device_next class.
 * Hardware-specific flash/partition operations are provided by the
 * application via fastboot_ops callbacks.
 */

#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usbd_fastboot.h>
#include <zephyr/drivers/usb/udc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_fastboot, CONFIG_USBD_FASTBOOT_LOG_LEVEL);

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define FASTBOOT_IFACE_CLASS    0xFF
#define FASTBOOT_IFACE_SUBCLASS 0x42
#define FASTBOOT_IFACE_PROTOCOL 0x03

/* Placeholder endpoint addresses - framework assigns real ones */
#define FASTBOOT_BULK_OUT_EP 0x01
#define FASTBOOT_BULK_IN_EP  0x81

/* Full-Speed descriptor */
static struct usb_if_descriptor fastboot_fs_iface = {
	.bLength = sizeof(struct usb_if_descriptor),
	.bDescriptorType = USB_DESC_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = FASTBOOT_IFACE_CLASS,
	.bInterfaceSubClass = FASTBOOT_IFACE_SUBCLASS,
	.bInterfaceProtocol = FASTBOOT_IFACE_PROTOCOL,
	.iInterface = 0,
};

static struct usb_ep_descriptor fastboot_fs_bulk_out = {
	.bLength = sizeof(struct usb_ep_descriptor),
	.bDescriptorType = USB_DESC_ENDPOINT,
	.bEndpointAddress = FASTBOOT_BULK_OUT_EP,
	.bmAttributes = USB_EP_TYPE_BULK,
	.wMaxPacketSize = sys_cpu_to_le16(USB_CONTROL_EP_MPS),
	.bInterval = 0x00,
};

static struct usb_ep_descriptor fastboot_fs_bulk_in = {
	.bLength = sizeof(struct usb_ep_descriptor),
	.bDescriptorType = USB_DESC_ENDPOINT,
	.bEndpointAddress = FASTBOOT_BULK_IN_EP,
	.bmAttributes = USB_EP_TYPE_BULK,
	.wMaxPacketSize = sys_cpu_to_le16(USB_CONTROL_EP_MPS),
	.bInterval = 0x00,
};

/* High-Speed descriptor */
static struct usb_if_descriptor fastboot_hs_iface = {
	.bLength = sizeof(struct usb_if_descriptor),
	.bDescriptorType = USB_DESC_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = FASTBOOT_IFACE_CLASS,
	.bInterfaceSubClass = FASTBOOT_IFACE_SUBCLASS,
	.bInterfaceProtocol = FASTBOOT_IFACE_PROTOCOL,
	.iInterface = 0,
};

static struct usb_ep_descriptor fastboot_hs_bulk_out = {
	.bLength = sizeof(struct usb_ep_descriptor),
	.bDescriptorType = USB_DESC_ENDPOINT,
	.bEndpointAddress = FASTBOOT_BULK_OUT_EP,
	.bmAttributes = USB_EP_TYPE_BULK,
	.wMaxPacketSize = sys_cpu_to_le16(512),
	.bInterval = 0x00,
};

static struct usb_ep_descriptor fastboot_hs_bulk_in = {
	.bLength = sizeof(struct usb_ep_descriptor),
	.bDescriptorType = USB_DESC_ENDPOINT,
	.bEndpointAddress = FASTBOOT_BULK_IN_EP,
	.bmAttributes = USB_EP_TYPE_BULK,
	.wMaxPacketSize = sys_cpu_to_le16(512),
	.bInterval = 0x00,
};

static struct usb_desc_header *fastboot_fs_desc[] = {
	(struct usb_desc_header *)&fastboot_fs_iface,
	(struct usb_desc_header *)&fastboot_fs_bulk_out,
	(struct usb_desc_header *)&fastboot_fs_bulk_in,
	NULL
};

static struct usb_desc_header *fastboot_hs_desc[] = {
	(struct usb_desc_header *)&fastboot_hs_iface,
	(struct usb_desc_header *)&fastboot_hs_bulk_out,
	(struct usb_desc_header *)&fastboot_hs_bulk_in,
	NULL
};

/*
 * Fastboot Context
 */

/* Protocol constants */
#define FASTBOOT_DOWNLOAD_BUF CONFIG_USBD_FASTBOOT_DOWNLOAD_BUF_SIZE
#define FASTBOOT_MAX_CMDBUF   64
#define FASTBOOT_MAX_RSPBUF   64

enum fastboot_state {
	FASTBOOT_STATE_OFFLINE,
	FASTBOOT_STATE_IDLE,
	FASTBOOT_STATE_DOWNLOAD,
	FASTBOOT_STATE_DOWNLOADED,
	FASTBOOT_STATE_ERROR,
};

struct fastboot_ctx {
	enum fastboot_state state;
	uint8_t *download_buf;
	uint32_t download_size;
	uint32_t download_received;
};

static struct fastboot_ctx fb_ctx;
static struct usbd_class_data *fb_c_data;
static const struct fastboot_ops *fb_ops;
static int iface_is_hs;

static void *fastboot_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	ARG_UNUSED(c_data);

	if (speed == USBD_SPEED_HS) {
		iface_is_hs = true;
		return (void *)fastboot_hs_desc;
	}
	return (void *)fastboot_fs_desc;
}

/*
 * Response Helpers
 */

static inline uint8_t get_ep_in(void)
{
	return iface_is_hs ? fastboot_hs_bulk_in.bEndpointAddress
			   : fastboot_fs_bulk_in.bEndpointAddress;
}

static inline uint8_t get_ep_out(void)
{
	return iface_is_hs ? fastboot_hs_bulk_out.bEndpointAddress
			   : fastboot_fs_bulk_out.bEndpointAddress;
}

static int fastboot_send(struct usbd_class_data *c_data, const char *data, size_t len)
{
	struct net_buf *buf;

	if (!c_data) {
		return -ENODEV;
	}

	uint8_t ep_in = get_ep_in();

	buf = usbd_ep_buf_alloc(c_data, ep_in, len);
	if (!buf) {
		return -ENOMEM;
	}
	net_buf_add_mem(buf, data, len);
	return usbd_ep_enqueue(c_data, buf);
}

#define fastboot_okay(c) fastboot_send((c), "OKAY", 4)

static int fastboot_fail(struct usbd_class_data *c_data, const char *msg)
{
	char buf[64];
	int n = snprintf(buf, sizeof(buf), "FAIL:%s", msg);

	return fastboot_send(c_data, buf, n);
}

/*
 * Command Handlers
 */

static void cmd_getvar(struct usbd_class_data *c_data, const char *arg)
{
	char buf[64];

	if (strcmp(arg, "version") == 0) {
		fastboot_send(c_data, "OKAY0.4", 7);
	} else if (strcmp(arg, "board") == 0) {
		fastboot_send(c_data, "OKAYart-pi2", 12);
	} else if (strcmp(arg, "serial") == 0) {
		fastboot_send(c_data, "OKAY000000000000", 16);
	} else if (strcmp(arg, "max-download-size") == 0) {
		snprintf(buf, sizeof(buf), "OKAY0x%08x", FASTBOOT_DOWNLOAD_BUF);
		fastboot_send(c_data, buf, strlen(buf));
	} else if (strcmp(arg, "all") == 0) {
		fastboot_send(c_data, "INFOversion: 0.4", 16);
		fastboot_send(c_data, "INFOboard: " CONFIG_USBD_FASTBOOTD_MANUFACTURER_STRING, 18);
		fastboot_send(c_data, "INFOserial: 000000000000", 22);
		snprintf(buf, sizeof(buf), "INFOmax-download-size: 0x%08x", FASTBOOT_DOWNLOAD_BUF);
		fastboot_send(c_data, buf, strlen(buf));
		fastboot_okay(c_data);
	} else if (strncmp(arg, "partition-size:", 15) == 0) {
		if (fb_ops && fb_ops->get_partition) {
			struct fastboot_part p;

			if (fb_ops->get_partition(arg + 15, &p) == 0) {
				snprintf(buf, sizeof(buf), "OKAY0x%08x", p.size);
				fastboot_send(c_data, buf, strlen(buf));
			} else {
				fastboot_fail(c_data, "no such partition");
			}
		} else {
			fastboot_fail(c_data, "no partition table");
		}
	} else if (strncmp(arg, "partition-type:", 15) == 0) {
		if (fb_ops && fb_ops->get_partition) {
			struct fastboot_part p;

			if (fb_ops->get_partition(arg + 15, &p) == 0) {
				fastboot_send(c_data, "OKAYraw", 7);
			} else {
				fastboot_fail(c_data, "no such partition");
			}
		} else {
			fastboot_fail(c_data, "no partition table");
		}
	} else {
		fastboot_okay(c_data);
	}
}

static void cmd_download(struct usbd_class_data *c_data, const char *arg)
{
	uint32_t size = (uint32_t)strtoul(arg, NULL, 16);

	if (size == 0 || size > FASTBOOT_DOWNLOAD_BUF) {
		fastboot_fail(c_data, "bad size");
		return;
	}

	if (!fb_ctx.download_buf) {
		fb_ctx.download_buf = k_malloc(FASTBOOT_DOWNLOAD_BUF);
		if (!fb_ctx.download_buf) {
			fastboot_fail(c_data, "oom");
			return;
		}
	}

	fb_ctx.download_size = size;
	fb_ctx.download_received = 0;
	fb_ctx.state = FASTBOOT_STATE_DOWNLOAD;

	char resp[16];

	snprintf(resp, sizeof(resp), "DATA%08x", size);
	fastboot_send(c_data, resp, strlen(resp));
}

static void cmd_flash(struct usbd_class_data *c_data, const char *arg)
{
	struct fastboot_part p;
	int rc;

	if (fb_ctx.state != FASTBOOT_STATE_DOWNLOADED) {
		fastboot_fail(c_data, "no firmware downloaded");
		return;
	}

	if (!fb_ops || !fb_ops->get_partition || !fb_ops->flash_erase || !fb_ops->flash_write ||
	    !fb_ops->firmware_verify) {
		fastboot_fail(c_data, "flash ops not registered");
		return;
	}

	rc = fb_ops->get_partition(arg, &p);
	if (rc != 0) {
		fastboot_fail(c_data, "no such partition");
		return;
	}

	if (fb_ctx.download_received < 64) {
		fastboot_fail(c_data, "firmware too small");
		return;
	}

	/* Verify firmware header */
	rc = fb_ops->firmware_verify(fb_ctx.download_buf, fb_ctx.download_received, p.magic);
	if (rc != 0) {
		fastboot_fail(c_data, "bad firmware header");
		return;
	}

	LOG_INF("Erasing %s...", p.name);
	rc = fb_ops->flash_erase(p.addr, p.size);
	if (rc) {
		fastboot_fail(c_data, "erase failed");
		return;
	}

	LOG_INF("Writing %s...", p.name);
	rc = fb_ops->flash_write(p.addr, fb_ctx.download_buf, fb_ctx.download_received);
	if (rc) {
		fastboot_fail(c_data, "write failed");
		return;
	}

	if (fb_ops->on_flash_done) {
		fb_ops->on_flash_done(arg);
	}

	fb_ctx.state = FASTBOOT_STATE_IDLE;
	k_free(fb_ctx.download_buf);
	fb_ctx.download_buf = NULL;

	fastboot_okay(c_data);
}

static void cmd_erase(struct usbd_class_data *c_data, const char *arg)
{
	struct fastboot_part p;

	if (!fb_ops || !fb_ops->get_partition || !fb_ops->flash_erase) {
		fastboot_fail(c_data, "flash ops not registered");
		return;
	}

	if (fb_ops->get_partition(arg, &p) != 0) {
		fastboot_fail(c_data, "no such partition");
		return;
	}

	int rc = fb_ops->flash_erase(p.addr, p.size);

	if (rc) {
		fastboot_fail(c_data, "erase failed");
	} else {
		fastboot_okay(c_data);
	}
}

/*
 * Command Dispatcher
 */

static void handle_cmd(struct usbd_class_data *c_data, const char *cmd)
{
	LOG_DBG("cmd '%s'", cmd);

	if (strncmp(cmd, "getvar:", 7) == 0) {
		cmd_getvar(c_data, cmd + 7);
	} else if (strncmp(cmd, "download:", 9) == 0) {
		cmd_download(c_data, cmd + 9);
	} else if (strncmp(cmd, "flash:", 6) == 0) {
		cmd_flash(c_data, cmd + 6);
	} else if (strncmp(cmd, "erase:", 6) == 0) {
		cmd_erase(c_data, cmd + 6);
	} else if (strcmp(cmd, "reboot") == 0) {
		fastboot_okay(c_data);
	} else if (strcmp(cmd, "continue") == 0) {
		fastboot_okay(c_data);
	} else {
		fastboot_fail(c_data, "unknown command");
	}
}

/*
 * Data Processor
 */

static void fastboot_process_data(struct usbd_class_data *c_data, const uint8_t *data, size_t len)
{
	switch (fb_ctx.state) {
	case FASTBOOT_STATE_IDLE: {
		char cmd[64];
		size_t n = MIN(len, sizeof(cmd) - 1);

		memcpy(cmd, data, n);
		cmd[n] = '\0';
		while (n > 0 && (cmd[n - 1] == '\r' || cmd[n - 1] == '\n')) {
			cmd[--n] = '\0';
		}
		if (n > 0) {
			handle_cmd(c_data, cmd);
		}
		break;
	}
	case FASTBOOT_STATE_DOWNLOAD: {
		uint32_t copy = MIN(len, fb_ctx.download_size - fb_ctx.download_received);

		if (copy > 0) {
			memcpy(fb_ctx.download_buf + fb_ctx.download_received, data, copy);
			fb_ctx.download_received += copy;
		}
		if (fb_ctx.download_received >= fb_ctx.download_size) {
			fb_ctx.state = FASTBOOT_STATE_DOWNLOADED;
			fastboot_okay(c_data);
		}
		break;
	}
	default:
		fastboot_fail(c_data, "protocol error");
		fb_ctx.state = FASTBOOT_STATE_IDLE;
		break;
	}
}

/*
 * USB Class API Callbacks
 */

static int fastboot_request(struct usbd_class_data *const c_data, struct net_buf *buf, int err)
{
	struct usbd_context *ctx = usbd_class_get_ctx(c_data);
	uint8_t ep_out = get_ep_out();

	if (err != 0) {
		LOG_ERR("bulk err %d", err);
		usbd_ep_buf_free(ctx, buf);
		return err;
	}

	if (buf->len > 0) {
		fastboot_process_data(c_data, buf->data, buf->len);
	}

	usbd_ep_buf_free(ctx, buf);

	/* Re-submit bulk OUT to keep receiving commands */
	{
		struct net_buf *new_buf = usbd_ep_buf_alloc(c_data, ep_out, 64);

		if (new_buf) {
			usbd_ep_enqueue(c_data, new_buf);
		}
	}

	return 0;
}

static void fastboot_enable(struct usbd_class_data *const c_data)
{
	uint8_t ep_out = get_ep_out();
	struct net_buf *buf;

	LOG_INF("enabled");
	fb_c_data = c_data;
	fb_ctx.state = FASTBOOT_STATE_IDLE;

	buf = usbd_ep_buf_alloc(c_data, ep_out, 64);
	if (buf) {
		usbd_ep_enqueue(c_data, buf);
	}
}

static void fastboot_disable(struct usbd_class_data *const c_data)
{
	struct usbd_context *ctx = usbd_class_get_ctx(c_data);
	uint8_t ep_out = get_ep_out();
	uint8_t ep_in = get_ep_in();

	LOG_INF("disabled");
	fb_c_data = NULL;
	fb_ctx.state = FASTBOOT_STATE_OFFLINE;
	usbd_ep_dequeue(ctx, ep_out);
	usbd_ep_dequeue(ctx, ep_in);
}

static int fastboot_init_class(struct usbd_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	memset(&fb_ctx, 0, sizeof(fb_ctx));
	fb_ctx.state = FASTBOOT_STATE_OFFLINE;

	LOG_INF("class init");
	return 0;
}

static const struct usbd_class_api fastboot_class_api = {
	.get_desc = fastboot_get_desc,
	.request = fastboot_request,
	.enable = fastboot_enable,
	.disable = fastboot_disable,
	.init = fastboot_init_class,
};

USBD_DEFINE_CLASS(fastboot_class, &fastboot_class_api, &fb_ctx, NULL);

/*
 * Default Partition Table Support
 */

static const struct fastboot_part *fb_parts;
static size_t fb_num_parts;

static int default_get_partition(const char *name, struct fastboot_part *part)
{
	for (size_t i = 0; i < fb_num_parts; i++) {
		if (strcmp(fb_parts[i].name, name) == 0) {
			*part = fb_parts[i];
			return 0;
		}
	}
	return -ENOENT;
}

/* Default ops with built-in partition table search */
static struct fastboot_ops default_ops = {
	.get_partition = default_get_partition,
};

/*
 * Public API
 */

int fastboot_register_partitions(const struct fastboot_part *parts, size_t num_parts)
{
	if (parts == NULL || num_parts == 0) {
		return -EINVAL;
	}
	fb_parts = parts;
	fb_num_parts = num_parts;

	/* Use default ops if application hasn't registered custom ones */
	if (fb_ops == NULL) {
		fb_ops = &default_ops;
	} else {
		/* Preserve custom ops but fill in get_partition if not set */
		if (fb_ops->get_partition == NULL) {
			LOG_ERR("fb_ops->get_partition is NULL\n");
			return -EINVAL;
		}
	}

	return 0;
}

int fastboot_register_ops(const struct fastboot_ops *ops)
{
	if (ops == NULL) {
		return -EINVAL;
	}

	fb_ops = ops;

	return 0;
}

int fastboot_init_download_buf(void)
{
	if (fb_ctx.download_buf) {
		return 0;
	}
	fb_ctx.download_buf = k_malloc(FASTBOOT_DOWNLOAD_BUF);
	if (!fb_ctx.download_buf) {
		return -ENOMEM;
	}
	return 0;
}

const struct usbd_fastboot_config *usbd_fastboot_config_get(void)
{
	extern const struct usbd_fastboot_config usbd_fastboot_cfg;

	return &usbd_fastboot_cfg;
}
