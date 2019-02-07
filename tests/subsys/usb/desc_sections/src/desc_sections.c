/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <tc_util.h>

#include <misc/byteorder.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>
LOG_MODULE_REGISTER(test_main);

#ifdef CONFIG_USB_COMPOSITE_DEVICE
#error Do not use composite configuration
#endif

/* Linker-defined symbols bound the USB descriptor structs */
extern struct usb_desc_header __usb_descriptor_start[];
extern struct usb_desc_header __usb_descriptor_end[];
extern struct usb_cfg_data __usb_data_start[];
extern struct usb_cfg_data __usb_data_end[];

u8_t *usb_get_device_descriptor(void);

struct usb_loopback_config {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
} __packed;

#define LOOPBACK_OUT_EP_ADDR		0x01
#define LOOPBACK_IN_EP_ADDR		0x81

#define LOOPBACK_BULK_EP_MPS		64

#define INITIALIZER_IF							\
	{								\
		.bLength = sizeof(struct usb_if_descriptor),		\
		.bDescriptorType = USB_INTERFACE_DESC,			\
		.bInterfaceNumber = 0,					\
		.bAlternateSetting = 0,					\
		.bNumEndpoints = 2,					\
		.bInterfaceClass = CUSTOM_CLASS,			\
		.bInterfaceSubClass = 0,				\
		.bInterfaceProtocol = 0,				\
		.iInterface = 0,					\
	}

#define INITIALIZER_IF_EP(addr, attr, mps)				\
	{								\
		.bLength = sizeof(struct usb_ep_descriptor),		\
		.bDescriptorType = USB_ENDPOINT_DESC,			\
		.bEndpointAddress = addr,				\
		.bmAttributes = attr,					\
		.wMaxPacketSize = sys_cpu_to_le16(mps),			\
		.bInterval = 0x00,					\
	}


#define DEFINE_LOOPBACK_DESC(x)						\
	USBD_CLASS_DESCR_DEFINE(primary, x)				\
	struct usb_loopback_config loopback_cfg_##x = {			\
	.if0 = INITIALIZER_IF,						\
	.if0_out_ep = INITIALIZER_IF_EP(LOOPBACK_OUT_EP_ADDR,		\
					USB_DC_EP_BULK,			\
					LOOPBACK_BULK_EP_MPS),		\
	.if0_in_ep = INITIALIZER_IF_EP(LOOPBACK_IN_EP_ADDR,		\
				       USB_DC_EP_BULK,			\
				       LOOPBACK_BULK_EP_MPS),		\
	}

#define DEFINE_LOOPBACK_EP_CFG(x)				\
	static struct usb_ep_cfg_data ep_cfg_##x[] = {		\
		{						\
			.ep_cb = NULL,				\
			.ep_addr = LOOPBACK_OUT_EP_ADDR,	\
		},						\
		{						\
			.ep_cb = NULL,				\
			.ep_addr = LOOPBACK_IN_EP_ADDR,		\
		},						\
	}

#define DEFINE_LOOPBACK_CFG_DATA(x) \
	USBD_CFG_DATA_DEFINE(loopback_##x) \
	struct usb_cfg_data loopback_config_##x = { \
	.usb_device_description = NULL, \
	.interface_config = NULL, \
	.interface_descriptor = &loopback_cfg_##x.if0, \
	.cb_usb_status = NULL, \
	.interface = { \
		.class_handler = NULL, \
		.custom_handler = NULL, \
		.vendor_handler = NULL, \
		.vendor_data = NULL, \
		.payload_data = NULL, \
	}, \
	.num_endpoints = ARRAY_SIZE(ep_cfg_##x), \
	.endpoint = ep_cfg_##x, \
}

DEFINE_LOOPBACK_DESC(0);
DEFINE_LOOPBACK_EP_CFG(0);
DEFINE_LOOPBACK_CFG_DATA(0);

DEFINE_LOOPBACK_DESC(1);
DEFINE_LOOPBACK_EP_CFG(1);
DEFINE_LOOPBACK_CFG_DATA(1);

#define NUM_INSTANCES	2

static void test_desc_sections(void)
{
	u8_t *head;

	TC_PRINT("__usb_descriptor_start %p\n", __usb_descriptor_start);
	TC_PRINT("__usb_descriptor_end %p\n",  __usb_descriptor_end);
	TC_PRINT("USB Descriptor table size %d\n",
		 (int)__usb_descriptor_end - (int)__usb_descriptor_start);

	TC_PRINT("__usb_data_start %p\n", __usb_data_start);
	TC_PRINT("__usb_data_end %p\n", __usb_data_end);
	TC_PRINT("USB Configuration data size %d\n",
		 (int)__usb_data_end - (int)__usb_data_start);

	TC_PRINT("sizeof usb_cfg_data %u\n", sizeof(struct usb_cfg_data));

	LOG_DBG("Starting logs");

	LOG_HEXDUMP_DBG((u8_t *)__usb_descriptor_start,
			(int)__usb_descriptor_end - (int)__usb_descriptor_start,
			"USB Descriptor table section");

	LOG_HEXDUMP_DBG((u8_t *)__usb_data_start,
			(int)__usb_data_end - (int)__usb_data_start,
			"USB Configuratio structures section");

	head = usb_get_device_descriptor();
	zassert_not_null(head, NULL);

	zassert_equal((int)__usb_descriptor_end - (int)__usb_descriptor_start,
		      119, NULL);

	/* Calculate number of structures */
	zassert_equal(__usb_data_end - __usb_data_start, NUM_INSTANCES, NULL);
	zassert_equal((int)__usb_data_end - (int)__usb_data_start,
		      NUM_INSTANCES * sizeof(struct usb_cfg_data), NULL);
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_desc,
			 ztest_unit_test(test_desc_sections));
	ztest_run_test_suite(test_desc);
}
