/* usb_descriptor.c - USB common device descriptor definition */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 * Copyright (c) 2017, 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <usb/usbstruct.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include "usb_descriptor.h"

#define LOG_LEVEL CONFIG_USB_DEVICE_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_descriptor);

/*
 * The last index of the initializer_string without null character is:
 *   ascii_idx_max = bLength / 2 - 2
 * Use this macro to determine the last index of ASCII7 string.
 */
#define USB_BSTRING_ASCII_IDX_MAX(n)	(n / 2 - 2)

/*
 * The last index of the bString is:
 *   utf16le_idx_max = sizeof(initializer_string) * 2 - 2 - 1
 *   utf16le_idx_max = bLength - 2 - 1
 * Use this macro to determine the last index of UTF16LE string.
 */
#define USB_BSTRING_UTF16LE_IDX_MAX(n)	(n - 3)

/* Linker-defined symbols bound the USB descriptor structs */
extern struct usb_desc_header __usb_descriptor_start[];
extern struct usb_desc_header __usb_descriptor_end[];
extern struct usb_cfg_data __usb_data_start[];
extern struct usb_cfg_data __usb_data_end[];

/* Structure representing the global USB description */
struct common_descriptor {
	struct usb_device_descriptor device_descriptor;
	struct usb_cfg_descriptor cfg_descr;
} __packed;

#define USB_DESC_MANUFACTURER_IDX			1
#define USB_DESC_PRODUCT_IDX				2
#define USB_DESC_SERIAL_NUMBER_IDX			3

/*
 * Device and configuration descriptor placed in the device section,
 * no additional descriptor may be placed there.
 */
USBD_DEVICE_DESCR_DEFINE(primary) struct common_descriptor common_desc = {
	/* Device descriptor */
	.device_descriptor = {
		.bLength = sizeof(struct usb_device_descriptor),
		.bDescriptorType = USB_DEVICE_DESC,
		.bcdUSB = sys_cpu_to_le16(USB_2_0),
#ifdef CONFIG_USB_COMPOSITE_DEVICE
		.bDeviceClass = MISC_CLASS,
		.bDeviceSubClass = 0x02,
		.bDeviceProtocol = 0x01,
#else
		.bDeviceClass = 0,
		.bDeviceSubClass = 0,
		.bDeviceProtocol = 0,
#endif
		.bMaxPacketSize0 = USB_MAX_CTRL_MPS,
		.idVendor = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_VID),
		.idProduct = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_PID),
		.bcdDevice = sys_cpu_to_le16(BCDDEVICE_RELNUM),
		.iManufacturer = USB_DESC_MANUFACTURER_IDX,
		.iProduct = USB_DESC_PRODUCT_IDX,
		.iSerialNumber = USB_DESC_SERIAL_NUMBER_IDX,
		.bNumConfigurations = 1,
	},
	/* Configuration descriptor */
	.cfg_descr = {
		.bLength = sizeof(struct usb_cfg_descriptor),
		.bDescriptorType = USB_CONFIGURATION_DESC,
		/*wTotalLength will be fixed in usb_fix_descriptor() */
		.wTotalLength = 0,
		.bNumInterfaces = 0,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_CONFIGURATION_ATTRIBUTES,
		.bMaxPower = MAX_LOW_POWER,
	},
};

struct usb_string_desription {
	struct usb_string_descriptor lang_descr;
	struct usb_mfr_descriptor {
		u8_t bLength;
		u8_t bDescriptorType;
		u8_t bString[USB_BSTRING_LENGTH(
				CONFIG_USB_DEVICE_MANUFACTURER)];
	} __packed utf16le_mfr;

	struct usb_product_descriptor {
		u8_t bLength;
		u8_t bDescriptorType;
		u8_t bString[USB_BSTRING_LENGTH(CONFIG_USB_DEVICE_PRODUCT)];
	} __packed utf16le_product;

	struct usb_sn_descriptor {
		u8_t bLength;
		u8_t bDescriptorType;
		u8_t bString[USB_BSTRING_LENGTH(CONFIG_USB_DEVICE_SN)];
	} __packed utf16le_sn;
} __packed;

/*
 * Language, Manufacturer, Product and Serial string descriptors,
 * placed in the string section.
 * FIXME: These should be sorted additionally.
 */
USBD_STRING_DESCR_DEFINE(primary) struct usb_string_desription string_descr = {
	.lang_descr = {
		.bLength = sizeof(struct usb_string_descriptor),
		.bDescriptorType = USB_STRING_DESC,
		.bString = sys_cpu_to_le16(0x0409),
	},
	/* Manufacturer String Descriptor */
	.utf16le_mfr = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				CONFIG_USB_DEVICE_MANUFACTURER),
		.bDescriptorType = USB_STRING_DESC,
		.bString = CONFIG_USB_DEVICE_MANUFACTURER,
	},
	/* Product String Descriptor */
	.utf16le_product = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				CONFIG_USB_DEVICE_PRODUCT),
		.bDescriptorType = USB_STRING_DESC,
		.bString = CONFIG_USB_DEVICE_PRODUCT,
	},
	/* Serial Number String Descriptor */
	.utf16le_sn = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(CONFIG_USB_DEVICE_SN),
		.bDescriptorType = USB_STRING_DESC,
		.bString = CONFIG_USB_DEVICE_SN,
	},
};

/* This element marks the end of the entire descriptor. */
USBD_TERM_DESCR_DEFINE(primary) struct usb_desc_header term_descr = {
	.bLength = 0,
	.bDescriptorType = 0,
};

/*
 * This function fixes bString by transforming the ASCII-7 string
 * into a UTF16-LE during runtime.
 */
static void ascii7_to_utf16le(void *descriptor)
{
	struct usb_string_descriptor *str_descr = descriptor;
	int idx_max = USB_BSTRING_UTF16LE_IDX_MAX(str_descr->bLength);
	int ascii_idx_max = USB_BSTRING_ASCII_IDX_MAX(str_descr->bLength);
	u8_t *buf = (u8_t *)&str_descr->bString;

	LOG_DBG("idx_max %d, ascii_idx_max %d, buf %p",
		idx_max, ascii_idx_max, buf);

	for (int i = idx_max; i >= 0; i -= 2) {
		LOG_DBG("char %c : %x, idx %d -> %d",
			buf[ascii_idx_max],
			buf[ascii_idx_max],
			ascii_idx_max, i);
		__ASSERT(buf[ascii_idx_max] > 0x1F && buf[ascii_idx_max] < 0x7F,
			 "Only printable ascii-7 characters are allowed in USB "
			 "string descriptors");
		buf[i] = 0U;
		buf[i - 1] = buf[ascii_idx_max--];
	}
}

/*
 * Look for the bString that has the address equal to the ptr and
 * return its index. Use it to determine the index of the bString and
 * assign it to the interfaces iInterface variable.
 */
int usb_get_str_descriptor_idx(void *ptr)
{
	struct usb_desc_header *head = __usb_descriptor_start;
	struct usb_string_descriptor *str = ptr;
	int str_descr_idx = 0;

	while (head->bLength != 0U) {
		switch (head->bDescriptorType) {
		case USB_STRING_DESC:
			if (head == (struct usb_desc_header *)str) {
				return str_descr_idx;
			}

			str_descr_idx += 1;
			break;
		default:
			break;
		}

		/* move to next descriptor */
		head = (struct usb_desc_header *)((u8_t *)head + head->bLength);
	}

	return 0;
}

/*
 * Validate endpoint address and Update the endpoint descriptors at runtime,
 * the result depends on the capabilities of the driver and the number and
 * type of endpoints.
 * The default endpoint address is stored in endpoint descriptor and
 * usb_ep_cfg_data, so both variables bEndpointAddress and ep_addr need
 * to be updated.
 */
static int usb_validate_ep_cfg_data(struct usb_ep_descriptor * const ep_descr,
				    struct usb_cfg_data * const cfg_data,
				    u32_t *requested_ep)
{
	for (int i = 0; i < cfg_data->num_endpoints; i++) {
		struct usb_ep_cfg_data *ep_data = cfg_data->endpoint;

		/*
		 * Trying to find the right entry in the usb_ep_cfg_data.
		 */
		if (ep_descr->bEndpointAddress != ep_data[i].ep_addr) {
			continue;
		}

		for (u8_t idx = 1; idx < 16; idx++) {
			struct usb_dc_ep_cfg_data ep_cfg;

			ep_cfg.ep_type = ep_descr->bmAttributes;
			ep_cfg.ep_mps = ep_descr->wMaxPacketSize;
			ep_cfg.ep_addr = ep_descr->bEndpointAddress;
			if (ep_cfg.ep_addr & USB_EP_DIR_IN) {
				if ((*requested_ep & (1 << (idx + 16)))) {
					continue;
				}

				ep_cfg.ep_addr = (USB_EP_DIR_IN | idx);
			} else {
				if ((*requested_ep & (1 << (idx)))) {
					continue;
				}

				ep_cfg.ep_addr = idx;
			}
			if (!usb_dc_ep_check_cap(&ep_cfg)) {
				LOG_DBG("Fixing EP address %x -> %x",
					ep_descr->bEndpointAddress,
					ep_cfg.ep_addr);
				ep_descr->bEndpointAddress = ep_cfg.ep_addr;
				ep_data[i].ep_addr = ep_cfg.ep_addr;
				if (ep_cfg.ep_addr & USB_EP_DIR_IN) {
					*requested_ep |= (1 << (idx + 16));
				} else {
					*requested_ep |= (1 << idx);
				}
				LOG_DBG("endpoint 0x%x", ep_data[i].ep_addr);
				return 0;
			}
		}
	}
	return -1;
}

/*
 * The interface descriptor of a USB function must be assigned to the
 * usb_cfg_data so that usb_ep_cfg_data and matching endpoint descriptor
 * can be found.
 */
static struct usb_cfg_data *usb_get_cfg_data(struct usb_if_descriptor *iface)
{
	size_t length = (__usb_data_end - __usb_data_start);

	for (size_t i = 0; i < length; i++) {
		if (__usb_data_start[i].interface_descriptor == iface) {
			return &__usb_data_start[i];
		}
	}

	return NULL;
}

/*
 * Default USB SN string descriptor is CONFIG_USB_DEVICE_SN, but sometimes
 * we want use another string as SN descriptor such as the chip's unique
 * ID. So user can implement this function return a string thats will be
 * replaced the default SN.
 * Please note that the new SN descriptor you changed must has same length
 * as CONFIG_USB_DEVICE_SN.
 */
__weak u8_t *usb_update_sn_string_descriptor(void)
{
	return NULL;
}

static void usb_fix_ascii_sn_string_descriptor(struct usb_sn_descriptor *sn)
{
	u8_t *runtime_sn =  usb_update_sn_string_descriptor();
	int runtime_sn_len, default_sn_len;

	if (!runtime_sn) {
		return;
	}

	runtime_sn_len = strlen(runtime_sn);
	default_sn_len = strlen(CONFIG_USB_DEVICE_SN);

	if (runtime_sn_len != default_sn_len) {
		LOG_ERR("the new SN descriptor doesn't has the same "
			"length as CONFIG_USB_DEVICE_SN");
		return;
	}

	memcpy(sn->bString, runtime_sn, runtime_sn_len);
}

/*
 * The entire descriptor, placed in the .usb.descriptor section,
 * needs to be fixed before use. Currently, only the length of the
 * entire device configuration (with all interfaces and endpoints)
 * and the string descriptors will be corrected.
 *
 * Restrictions:
 * - just one device configuration (there is only one)
 * - string descriptor must be present
 */
static int usb_fix_descriptor(struct usb_desc_header *head)
{
	struct usb_cfg_descriptor *cfg_descr = NULL;
	struct usb_if_descriptor *if_descr = NULL;
	struct usb_cfg_data *cfg_data = NULL;
	struct usb_ep_descriptor *ep_descr = NULL;
	u8_t numof_ifaces = 0U;
	u8_t str_descr_idx = 0U;
	u32_t requested_ep = BIT(16) | BIT(0);

	while (head->bLength != 0U) {
		switch (head->bDescriptorType) {
		case USB_CONFIGURATION_DESC:
			cfg_descr = (struct usb_cfg_descriptor *)head;
			LOG_DBG("Configuration descriptor %p", head);
			break;
		case USB_ASSOCIATION_DESC:
			LOG_DBG("Association descriptor %p", head);
			break;
		case USB_INTERFACE_DESC:
			if_descr = (struct usb_if_descriptor *)head;
			LOG_DBG("Interface descriptor %p", head);
			if (if_descr->bAlternateSetting) {
				LOG_DBG("Skip alternate interface");
				break;
			}

			if (if_descr->bInterfaceNumber == 0U) {
				cfg_data = usb_get_cfg_data(if_descr);
				if (!cfg_data) {
					LOG_ERR("There is no usb_cfg_data "
						"for %p", head);
					return -1;
				}

				if (cfg_data->interface_config) {
					cfg_data->interface_config(head,
							numof_ifaces);
				}
			}

			numof_ifaces++;
			break;
		case USB_ENDPOINT_DESC:
			if (!cfg_data) {
				LOG_ERR("Uninitialized usb_cfg_data pointer, "
					"corrupted device descriptor?");
				return -1;
			}

			LOG_DBG("Endpoint descriptor %p", head);
			ep_descr = (struct usb_ep_descriptor *)head;
			if (usb_validate_ep_cfg_data(ep_descr,
						     cfg_data,
						     &requested_ep)) {
				LOG_ERR("Failed to validate endpoints");
				return -1;
			}

			break;
		case 0:
		case USB_STRING_DESC:
			/*
			 * Copy runtime SN string descriptor first, if has
			 */
			if (str_descr_idx == USB_DESC_SERIAL_NUMBER_IDX) {
				struct usb_sn_descriptor *sn =
					(struct usb_sn_descriptor *)head;
				usb_fix_ascii_sn_string_descriptor(sn);
			}
			/*
			 * Skip language descriptor but correct
			 * wTotalLength and bNumInterfaces once.
			 */
			if (str_descr_idx) {
				ascii7_to_utf16le(head);
			} else {
				if (!cfg_descr) {
					LOG_ERR("Incomplete device descriptor");
					return -1;
				}

				LOG_DBG("Now the wTotalLength is %zd",
					(u8_t *)head - (u8_t *)cfg_descr);
				sys_put_le16((u8_t *)head - (u8_t *)cfg_descr,
					     (u8_t *)&cfg_descr->wTotalLength);
				cfg_descr->bNumInterfaces = numof_ifaces;
			}

			str_descr_idx += 1U;

			break;
		default:
			break;
		}

		/* Move to next descriptor */
		head = (struct usb_desc_header *)((u8_t *)head + head->bLength);
	}

	if ((head + 1) != __usb_descriptor_end) {
		LOG_DBG("try to fix next descriptor at %p", head + 1);
		return usb_fix_descriptor(head + 1);
	}

	return 0;
}


u8_t *usb_get_device_descriptor(void)
{
	LOG_DBG("__usb_descriptor_start %p", __usb_descriptor_start);
	LOG_DBG("__usb_descriptor_end %p", __usb_descriptor_end);

	if (usb_fix_descriptor(__usb_descriptor_start)) {
		LOG_ERR("Failed to fixup USB descriptor");
		return NULL;
	}

	return (u8_t *) __usb_descriptor_start;
}

struct usb_dev_data *usb_get_dev_data_by_cfg(sys_slist_t *list,
					     struct usb_cfg_data *cfg)
{
	struct usb_dev_data *dev_data;

	SYS_SLIST_FOR_EACH_CONTAINER(list, dev_data, node) {
		struct device *dev = dev_data->dev;
		const struct usb_cfg_data *cfg_cur = dev->config->config_info;

		if (cfg_cur == cfg) {
			return dev_data;
		}
	}

	LOG_DBG("Device data not found for cfg %p", cfg);

	return NULL;
}

struct usb_dev_data *usb_get_dev_data_by_iface(sys_slist_t *list,
					       u8_t iface_num)
{
	struct usb_dev_data *dev_data;

	SYS_SLIST_FOR_EACH_CONTAINER(list, dev_data, node) {
		struct device *dev = dev_data->dev;
		const struct usb_cfg_data *cfg = dev->config->config_info;
		const struct usb_if_descriptor *if_desc =
						cfg->interface_descriptor;

		if (if_desc->bInterfaceNumber == iface_num) {
			return dev_data;
		}
	}

	LOG_DBG("Device data not found for iface number %u", iface_num);

	return NULL;
}

struct usb_dev_data *usb_get_dev_data_by_ep(sys_slist_t *list, u8_t ep)
{
	struct usb_dev_data *dev_data;

	SYS_SLIST_FOR_EACH_CONTAINER(list, dev_data, node) {
		struct device *dev = dev_data->dev;
		const struct usb_cfg_data *cfg = dev->config->config_info;
		const struct usb_ep_cfg_data *ep_data = cfg->endpoint;

		for (u8_t i = 0; i < cfg->num_endpoints; i++) {
			if (ep_data[i].ep_addr == ep) {
				return dev_data;
			}
		}
	}

	LOG_DBG("Device data not found for ep %u", ep);

	return NULL;
}
