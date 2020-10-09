/*
 * Copyright (c) 2017-2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBD_INTERNAL_H_
#define ZEPHYR_INCLUDE_USBD_INTERNAL_H_

#define USBD_DESC_MANUFACTURER_IDX		1
#define USBD_DESC_PRODUCT_IDX			2
#define USBD_DESC_SERIAL_NUMBER_IDX		3

/*
 * The USB Unicode bString is encoded in UTF16LE, which means it takes up
 * twice the amount of bytes than the same string encoded in ASCII7.
 * Use this macro to determine the length of the bString array.
 *
 * bString length without null character:
 *   bString_length = (sizeof(initializer_string) - 1) * 2
 * or:
 *   bString_length = sizeof(initializer_string) * 2 - 2
 */
#define USB_BSTRING_LENGTH(s)		(sizeof(s) * 2 - 2)

/*
 * The length of the string descriptor (bLength) is calculated from the
 * size of the two octets bLength and bDescriptorType plus the
 * length of the UTF16LE string:
 *
 *   bLength = 2 + bString_length
 *   bLength = 2 + sizeof(initializer_string) * 2 - 2
 *   bLength = sizeof(initializer_string) * 2
 * Use this macro to determine the bLength of the string descriptor.
 */
#define USB_STRING_DESCRIPTOR_LENGTH(s)	(sizeof(s) * 2)

#define USBD_CTRL_SEQ_SETUP		0
#define USBD_CTRL_SEQ_DATA_OUT		1
#define USBD_CTRL_SEQ_DATA_IN		2
#define USBD_CTRL_SEQ_NO_DATA		3
#define USBD_CTRL_SEQ_STATUS_OUT	4
#define USBD_CTRL_SEQ_STATUS_IN		5
#define USBD_CTRL_SEQ_ERROR		6

/** Set if USB device stack has been enabled */
#define USBD_STATE_ENABLED		0
/** Set if USB device stack has been configured */
#define USBD_STATE_CONFIGURED		1

struct usbd_contex {
	/** Setup packet, up-to-date for the respective control transaction. */
	struct usb_setup_packet setup;
	/** USB device stack status callback */
	usb_dc_status_callback status_cb;
	/** USB device stack user status callback */
	usb_dc_status_callback user_status_cb;
	/** Control Sequence Stage */
	uint8_t ctrl_stage;
	/** State of the USB device stack */
	atomic_t state;
	/** USB device stack selected configuration */
	uint8_t configuration;
	/** Remote wakeup feature status */
	bool remote_wakeup;
	/** List of registered classes or functions */
	sys_slist_t class_list;
	/** Endpoints bitmap */
	uint32_t ep_bm;

	/** Table to track alternative interfaces */
	uint8_t alternate[10];

	/** USB Device Descriptor */
	struct usb_device_descriptor dev_desc;

	/** USB Configuration Descriptor */
	struct usb_cfg_descriptor cfg_desc;

	/** Language String Descriptor */
	struct usb_string_descriptor lang_desc;

	/** Manufacturer String Descriptor */
	struct usb_mfr_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bString[USB_BSTRING_LENGTH(
				CONFIG_USBD_DEVICE_MANUFACTURER)];
	} __packed mfr_desc;

	/* Product String Descriptor */
	struct usb_product_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bString[USB_BSTRING_LENGTH(CONFIG_USBD_DEVICE_PRODUCT)];
	} __packed product_desc;

	/* Serial Number String Descriptor */
	struct usb_sn_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bString[USB_BSTRING_LENGTH(CONFIG_USBD_DEVICE_SN)];
	} __packed sn_desc;
};

int usbd_init_control_ep(void);

int usbd_init_desc(void);

int usbd_cctx_cfg_eps(uint8_t i_n, bool enable);

size_t usbd_cctx_desc_len(struct usbd_class_ctx *cctx);

struct usbd_class_ctx *usbd_cctx_get_by_iface(uint8_t i_n);

struct usbd_class_ctx *usbd_cctx_get_by_ep(uint8_t ep);

struct usbd_class_ctx *usbd_cctx_get_by_req(uint8_t request);

int cctx_restart_out_eps(struct usbd_class_ctx *cctx, uint8_t i_n,
			 bool force_all);

/** USBD buffer management */
struct usbd_buf_ud {
	/** Endpoint associated to the transfer */
	uint8_t ep;
	/** Transfer status */
	uint8_t status;
	/** Endpoint type */
	uint8_t type;
	/** Transfer flags */
	uint8_t flags;
	/** Allow to find back buffer based on endpoint address */
	sys_snode_t node;
} __packed;

BUILD_ASSERT(sizeof(struct usbd_buf_ud) == CONFIG_NET_BUF_USER_DATA_SIZE,
	     "sizeof usbd_buf_ud mismatch");

/* USBD tbuf flags */
#define USBD_TRANS_RD		BIT(0) /** Read transfer flag */
#define USBD_TRANS_WR		BIT(1) /** Write transfer flag */
#define USBD_TRANS_ZLP		BIT(2) /** Handle zero-length packet flag */

/**
 * @brief Initialize USB buffer management
 *
 * @return 0 on success, negative errno code on fail.
 */
int usbd_tbuf_init(void);

void usbd_tbuf_ep_cb(uint8_t ep, enum usb_dc_ep_cb_status_code status);

struct net_buf *usbd_tbuf_alloc(uint8_t ep, size_t size);

int usbd_tbuf_submit(struct net_buf *buf, bool handle_zlp);

int usbd_tbuf_cancel(uint8_t ep);

#endif /* ZEPHYR_INCLUDE_USBD_INTERNAL_H_ */
