/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <tc_util.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>
#include <usb_descriptor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_main, LOG_LEVEL_DBG);

#ifdef CONFIG_USB_COMPOSITE_DEVICE
#error Do not use composite configuration
#endif

/* Linker-defined symbols bound the USB descriptor structs */
extern struct usb_desc_header __usb_descriptor_start[];
extern struct usb_desc_header __usb_descriptor_end[];
extern struct usb_cfg_data _usb_cfg_data_list_start[];
extern struct usb_cfg_data _usb_cfg_data_list_end[];

uint8_t *usb_get_device_descriptor(void);

struct usb_test_config {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in1_ep;
	struct usb_ep_descriptor if0_in2_ep;
} __packed;

#if IS_ENABLED(CONFIG_USB_DC_HAS_HS_SUPPORT)
#define TEST_BULK_EP_MPS		512
#else
#define TEST_BULK_EP_MPS		64
#endif

#define TEST_DESCRIPTOR_TABLE_SPAN	157

#define INITIALIZER_IF							\
	{								\
		.bLength = sizeof(struct usb_if_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE,			\
		.bInterfaceNumber = 0,					\
		.bAlternateSetting = 0,					\
		.bNumEndpoints = 3,					\
		.bInterfaceClass = USB_BCC_VENDOR,			\
		.bInterfaceSubClass = 0,				\
		.bInterfaceProtocol = 0,				\
		.iInterface = 0,					\
	}

#define INITIALIZER_IF_EP(addr, attr, mps)				\
	{								\
		.bLength = sizeof(struct usb_ep_descriptor),		\
		.bDescriptorType = USB_DESC_ENDPOINT,			\
		.bEndpointAddress = addr,				\
		.bmAttributes = attr,					\
		.wMaxPacketSize = sys_cpu_to_le16(mps),			\
		.bInterval = 0x00,					\
	}


#define DEFINE_TEST_DESC(x, _)						\
	USBD_CLASS_DESCR_DEFINE(primary, x)				\
	struct usb_test_config test_cfg_##x = {				\
	.if0 = INITIALIZER_IF,						\
	.if0_out_ep = INITIALIZER_IF_EP(AUTO_EP_OUT,			\
					USB_DC_EP_BULK,			\
					TEST_BULK_EP_MPS),		\
	.if0_in1_ep = INITIALIZER_IF_EP(AUTO_EP_IN,			\
					USB_DC_EP_BULK,			\
					TEST_BULK_EP_MPS),		\
	.if0_in2_ep = INITIALIZER_IF_EP(AUTO_EP_IN,			\
					USB_DC_EP_BULK,			\
					TEST_BULK_EP_MPS),		\
	}

#define INITIALIZER_EP_DATA(cb, addr)					\
	{								\
		.ep_cb = cb,						\
		.ep_addr = addr,					\
	}

#define DEFINE_TEST_EP_CFG(x, _)				\
	static struct usb_ep_cfg_data ep_cfg_##x[] = {		\
		INITIALIZER_EP_DATA(NULL, AUTO_EP_OUT),		\
		INITIALIZER_EP_DATA(NULL, AUTO_EP_IN),		\
		INITIALIZER_EP_DATA(NULL, AUTO_EP_IN),		\
	}

#define DEFINE_TEST_CFG_DATA(x, _)				\
	USBD_DEFINE_CFG_DATA(test_config_##x) = {		\
	.usb_device_description = NULL,				\
	.interface_config = interface_config,			\
	.interface_descriptor = &test_cfg_##x.if0,		\
	.cb_usb_status = NULL,					\
	.interface = {						\
		.class_handler = NULL,				\
		.custom_handler = NULL,				\
		.vendor_handler = NULL,				\
	},							\
	.num_endpoints = ARRAY_SIZE(ep_cfg_##x),		\
	.endpoint = ep_cfg_##x,					\
	}

#define NUM_INSTANCES 2

static void interface_config(struct usb_desc_header *head,
			     uint8_t iface_num)
{
	struct usb_if_descriptor *if_desc = (struct usb_if_descriptor *)head;

	LOG_DBG("head %p iface_num %u", head, iface_num);

	if_desc->bInterfaceNumber = iface_num;
}

LISTIFY(NUM_INSTANCES, DEFINE_TEST_DESC, (;), _);
LISTIFY(NUM_INSTANCES, DEFINE_TEST_EP_CFG, (;), _);
LISTIFY(NUM_INSTANCES, DEFINE_TEST_CFG_DATA, (;), _);

static struct usb_cfg_data *usb_get_cfg_data(struct usb_if_descriptor *iface)
{
	STRUCT_SECTION_FOREACH(usb_cfg_data, cfg_data) {
		if (cfg_data->interface_descriptor == iface) {
			return cfg_data;
		}
	}

	return NULL;
}

static bool find_cfg_data_ep(const struct usb_ep_descriptor * const ep_descr,
			     const struct usb_cfg_data * const cfg_data,
			     uint8_t ep_count)
{
	for (int i = 0; i < cfg_data->num_endpoints; i++) {
		if (cfg_data->endpoint[i].ep_addr ==
				ep_descr->bEndpointAddress) {
			LOG_DBG("found ep[%d] %x", i,
				ep_descr->bEndpointAddress);

			if (ep_count != i) {
				LOG_ERR("EPs are assigned in wrong order");
				return false;
			}

			return true;
		}
	}

	return false;
}

static void check_endpoint_allocation(struct usb_desc_header *head)
{
	struct usb_cfg_data *cfg_data = NULL;
	static uint8_t interfaces;
	uint8_t ep_count = 0;

	while (head->bLength != 0) {
		if (head->bDescriptorType == USB_DESC_INTERFACE) {
			struct usb_if_descriptor *if_descr = (void *)head;

			ep_count = 0;

			LOG_DBG("iface %u", if_descr->bInterfaceNumber);

			/* Check that interfaces get correct numbers */
			zassert_equal(if_descr->bInterfaceNumber, interfaces,
				      "Interfaces numbering failed");

			interfaces++;

			cfg_data = usb_get_cfg_data(if_descr);
			zassert_not_null(cfg_data, "Check available cfg data");
		}

		if (head->bDescriptorType == USB_DESC_ENDPOINT) {
			struct usb_ep_descriptor *ep_descr =
				(struct usb_ep_descriptor *)head;

			/* Check that we get iface desc before */
			zassert_not_null(cfg_data, "Check available cfg data");

			zassert_true(find_cfg_data_ep(ep_descr, cfg_data,
						      ep_count),
				     "Check endpoint config in cfg_data");
			ep_count++;
		}

		head = (struct usb_desc_header *)((uint8_t *)head + head->bLength);
	}
}

/* Determine the number of bytes spanned between two linker-defined
 * symbols that are normally interpreted as pointers.
 */
#define SYMBOL_SPAN(_ep, _sp) (int)(intptr_t)((uintptr_t)(_ep) - (uintptr_t)(_sp))

static void test_desc_sections(void)
{
	struct usb_desc_header *head;

	TC_PRINT("__usb_descriptor_start %p\n", __usb_descriptor_start);
	TC_PRINT("__usb_descriptor_end %p\n",  __usb_descriptor_end);
	TC_PRINT("USB Descriptor table span %d\n",
		 SYMBOL_SPAN(__usb_descriptor_end, __usb_descriptor_start));

	TC_PRINT("_usb_cfg_data_list_start %p\n", _usb_cfg_data_list_start);
	TC_PRINT("_usb_cfg_data_list_end %p\n", _usb_cfg_data_list_end);
	TC_PRINT("USB Configuration data span %d\n",
		 SYMBOL_SPAN(_usb_cfg_data_list_end, _usb_cfg_data_list_start));

	TC_PRINT("sizeof usb_cfg_data %zu\n", sizeof(struct usb_cfg_data));

	LOG_DBG("Starting logs");

	LOG_HEXDUMP_DBG((uint8_t *)__usb_descriptor_start,
			SYMBOL_SPAN(__usb_descriptor_end, __usb_descriptor_start),
			"USB Descriptor table section");

	LOG_HEXDUMP_DBG((uint8_t *)_usb_cfg_data_list_start,
			SYMBOL_SPAN(_usb_cfg_data_list_end, _usb_cfg_data_list_start),
			"USB Configuration structures section");

	head = (struct usb_desc_header *)__usb_descriptor_start;
	zassert_not_null(head, NULL);

	zassert_equal(SYMBOL_SPAN(__usb_descriptor_end, __usb_descriptor_start),
		      TEST_DESCRIPTOR_TABLE_SPAN, NULL);

	/* Calculate number of structures */
	zassert_equal(_usb_cfg_data_list_end - _usb_cfg_data_list_start,
		      NUM_INSTANCES, NULL);
	zassert_equal(SYMBOL_SPAN(_usb_cfg_data_list_end, _usb_cfg_data_list_start),
		      NUM_INSTANCES * sizeof(struct usb_cfg_data), NULL);

	check_endpoint_allocation(head);
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_desc,
			 ztest_unit_test(test_desc_sections));
	ztest_run_test_suite(test_desc);
}
