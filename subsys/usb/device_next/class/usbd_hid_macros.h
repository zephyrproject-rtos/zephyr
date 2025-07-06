/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The macros in this file are not for public use, but only for HID driver
 * instantiation.
 */

#include <zephyr/sys/util_macro.h>
#include <zephyr/usb/usb_ch9.h>

#ifndef ZEPHYR_USB_DEVICE_CLASS_HID_MACROS_H_
#define ZEPHYR_USB_DEVICE_CLASS_HID_MACROS_H_

/*
 * This long list of definitions is used in HID_MPS_LESS_65 macro to determine
 * whether an endpoint MPS is equal to or less than 64 bytes.
 */
#define HID_MPS_LESS_65_0 1
#define HID_MPS_LESS_65_1 1
#define HID_MPS_LESS_65_2 1
#define HID_MPS_LESS_65_3 1
#define HID_MPS_LESS_65_4 1
#define HID_MPS_LESS_65_5 1
#define HID_MPS_LESS_65_6 1
#define HID_MPS_LESS_65_7 1
#define HID_MPS_LESS_65_8 1
#define HID_MPS_LESS_65_9 1
#define HID_MPS_LESS_65_10 1
#define HID_MPS_LESS_65_11 1
#define HID_MPS_LESS_65_12 1
#define HID_MPS_LESS_65_13 1
#define HID_MPS_LESS_65_14 1
#define HID_MPS_LESS_65_15 1
#define HID_MPS_LESS_65_16 1
#define HID_MPS_LESS_65_17 1
#define HID_MPS_LESS_65_18 1
#define HID_MPS_LESS_65_19 1
#define HID_MPS_LESS_65_20 1
#define HID_MPS_LESS_65_21 1
#define HID_MPS_LESS_65_22 1
#define HID_MPS_LESS_65_23 1
#define HID_MPS_LESS_65_24 1
#define HID_MPS_LESS_65_25 1
#define HID_MPS_LESS_65_26 1
#define HID_MPS_LESS_65_27 1
#define HID_MPS_LESS_65_28 1
#define HID_MPS_LESS_65_29 1
#define HID_MPS_LESS_65_30 1
#define HID_MPS_LESS_65_31 1
#define HID_MPS_LESS_65_32 1
#define HID_MPS_LESS_65_33 1
#define HID_MPS_LESS_65_34 1
#define HID_MPS_LESS_65_35 1
#define HID_MPS_LESS_65_36 1
#define HID_MPS_LESS_65_37 1
#define HID_MPS_LESS_65_38 1
#define HID_MPS_LESS_65_39 1
#define HID_MPS_LESS_65_40 1
#define HID_MPS_LESS_65_41 1
#define HID_MPS_LESS_65_42 1
#define HID_MPS_LESS_65_43 1
#define HID_MPS_LESS_65_44 1
#define HID_MPS_LESS_65_45 1
#define HID_MPS_LESS_65_46 1
#define HID_MPS_LESS_65_47 1
#define HID_MPS_LESS_65_48 1
#define HID_MPS_LESS_65_49 1
#define HID_MPS_LESS_65_50 1
#define HID_MPS_LESS_65_51 1
#define HID_MPS_LESS_65_52 1
#define HID_MPS_LESS_65_53 1
#define HID_MPS_LESS_65_54 1
#define HID_MPS_LESS_65_55 1
#define HID_MPS_LESS_65_56 1
#define HID_MPS_LESS_65_57 1
#define HID_MPS_LESS_65_58 1
#define HID_MPS_LESS_65_59 1
#define HID_MPS_LESS_65_60 1
#define HID_MPS_LESS_65_61 1
#define HID_MPS_LESS_65_62 1
#define HID_MPS_LESS_65_63 1
#define HID_MPS_LESS_65_64 1

#define HID_MPS_LESS_65(x) UTIL_PRIMITIVE_CAT(HID_MPS_LESS_65_, x)

/*
 * If all the endpoint MPS are less than 65 bytes, we do not need to define and
 * configure an alternate interface.
 */
#define HID_ALL_MPS_LESS_65(n)							\
	COND_CODE_1(HID_MPS_LESS_65(DT_INST_PROP_OR(n, out_report_size, 0)),	\
		(HID_MPS_LESS_65(DT_INST_PROP(n, in_report_size))), (0))

/* Get IN endpoint polling rate based on the desired speed. */
#define HID_IN_EP_INTERVAL(n, hs)							\
	COND_CODE_1(hs,									\
		    (USB_HS_INT_EP_INTERVAL(DT_INST_PROP(n, in_polling_period_us))),	\
		    (USB_FS_INT_EP_INTERVAL(DT_INST_PROP(n, in_polling_period_us))))

/* Get OUT endpoint polling rate based on the desired speed. */
#define HID_OUT_EP_INTERVAL(n, hs)							\
	COND_CODE_1(hs,									\
		    (USB_HS_INT_EP_INTERVAL(DT_INST_PROP(n, out_polling_period_us))),	\
		    (USB_FS_INT_EP_INTERVAL(DT_INST_PROP(n, out_polling_period_us))))

/* Get the number of endpoints, which can be either 1 or 2 */
#define HID_NUM_ENDPOINTS(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, out_report_size), (2), (1))

/*
 * Either the device does not support a boot protocol, or it supports the
 * keyboard or mouse boot protocol.
 */
#define HID_INTERFACE_PROTOCOL(n) DT_INST_ENUM_IDX_OR(n, protocol_code, 0)

/* bInterfaceSubClass must be set to 1 if a boot device protocol is supported */
#define HID_INTERFACE_SUBCLASS(n)						\
	COND_CODE_0(HID_INTERFACE_PROTOCOL(n), (0), (1))

#define HID_INTERFACE_DEFINE(n, alt)						\
	{									\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = alt,					\
		.bNumEndpoints = HID_NUM_ENDPOINTS(n),				\
		.bInterfaceClass = USB_BCC_HID,					\
		.bInterfaceSubClass = HID_INTERFACE_SUBCLASS(n),		\
		.bInterfaceProtocol = HID_INTERFACE_PROTOCOL(n),		\
		.iInterface = 0,						\
	}

#define HID_DESCRIPTOR_DEFINE(n)						\
	{									\
		.bLength = sizeof(struct hid_descriptor),			\
		.bDescriptorType = USB_DESC_HID,				\
		.bcdHID = sys_cpu_to_le16(USB_HID_VERSION),			\
		.bCountryCode = 0,						\
		.bNumDescriptors = HID_SUBORDINATE_DESC_NUM,			\
		.sub[0] = {							\
			.bDescriptorType = USB_DESC_HID_REPORT,			\
			.wDescriptorLength = 0,					\
		},								\
	}									\

/*
 * OUT endpoint MPS for either default or alternate interface.
 * MPS for the default interface is always limited to 64 bytes.
 */
#define HID_OUT_EP_MPS(n, alt)							\
	COND_CODE_1(alt,							\
	(sys_cpu_to_le16(USB_TPL_TO_MPS(DT_INST_PROP(n, out_report_size)))),	\
	(sys_cpu_to_le16(MIN(DT_INST_PROP(n, out_report_size), 64U))))

/*
 * IN endpoint MPS for either default or alternate interface.
 * MPS for the default interface is always limited to 64 bytes.
 */
#define HID_IN_EP_MPS(n, alt)							\
	COND_CODE_1(alt,							\
	(sys_cpu_to_le16(USB_TPL_TO_MPS(DT_INST_PROP(n, in_report_size)))),	\
	(sys_cpu_to_le16(MIN(DT_INST_PROP(n, in_report_size), 64U))))

#define HID_OUT_EP_DEFINE(n, hs, alt)						\
	{									\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = HID_OUT_EP_MPS(n, alt),			\
		.bInterval = HID_OUT_EP_INTERVAL(n, hs),			\
	}

#define HID_IN_EP_DEFINE(n, hs, alt)						\
	{									\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = HID_IN_EP_MPS(n, alt),			\
		.bInterval = HID_IN_EP_INTERVAL(n, hs),				\
	}

/*
 * Both the optional OUT endpoint and the associated pool are only defined if
 * there is an out-report-size property.
 */
#define HID_OUT_EP_DEFINE_OR_ZERO(n, hs, alt)					\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, out_report_size),			\
		    (HID_OUT_EP_DEFINE(n, hs, alt)),				\
		    ({0}))

#define HID_OUT_POOL_DEFINE(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, out_report_size),			\
		    (UDC_BUF_POOL_DEFINE(hid_buf_pool_out_##n,			\
					 CONFIG_USBD_HID_OUT_BUF_COUNT,		\
					 DT_INST_PROP(n, out_report_size),	\
					 sizeof(struct udc_buf_info), NULL)),	\
		    ())

#define HID_OUT_POOL_ADDR(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, out_report_size),			\
		    (&hid_buf_pool_out_##n), (NULL))

#define HID_VERIFY_REPORT_SIZES(n)						\
	BUILD_ASSERT(USB_TPL_IS_VALID(DT_INST_PROP_OR(n, out_report_size, 0)),	\
		"out-report-size must be valid Total Packet Length");		\
	BUILD_ASSERT(USB_TPL_IS_VALID(DT_INST_PROP_OR(n, in_report_size, 0)),	\
		"in-report-size must be valid Total Packet Length");

#endif /* ZEPHYR_USB_DEVICE_CLASS_HID_MACROS_H_ */
