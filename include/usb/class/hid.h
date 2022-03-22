/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018,2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Human Interface Device (HID) common definitions header
 *
 * Header follows Device Class Definition for Human Interface Devices (HID)
 * Version 1.11 document (HID1_11-1.pdf).
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_HID_H_
#define ZEPHYR_INCLUDE_USB_CLASS_HID_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief hid.h API
 * @defgroup usb_hid_definitions USB HID common definitions
 * @{
 * @}
 */

/**
 * @defgroup usb_hid_types USB HID types and values
 * @ingroup usb_hid_definitions
 * @{
 */

/** USB HID Class HID descriptor type */
#define USB_DESC_HID			0x21
/** USB HID Class Report descriptor type */
#define USB_DESC_HID_REPORT		0x22
/** USB HID Class physical descriptor type */
#define USB_DESC_HID_PHYSICAL		0x23

/** USB HID Class GetReport bRequest value */
#define USB_HID_GET_REPORT		0x01
/** USB HID Class GetIdle bRequest value */
#define USB_HID_GET_IDLE		0x02
/** USB HID Class GetProtocol bRequest value */
#define USB_HID_GET_PROTOCOL		0x03
/** USB HID Class SetReport bRequest value */
#define USB_HID_SET_REPORT		0x09
/** USB HID Class SetIdle bRequest value */
#define USB_HID_SET_IDLE		0x0A
/** USB HID Class SetProtocol bRequest value */
#define USB_HID_SET_PROTOCOL		0x0B

/** USB HID Boot Interface Protocol (bInterfaceProtocol) Code None */
#define HID_BOOT_IFACE_CODE_NONE	0
/** USB HID Boot Interface Protocol (bInterfaceProtocol) Code Keyboard */
#define HID_BOOT_IFACE_CODE_KEYBOARD	1
/** USB HID Boot Interface Protocol (bInterfaceProtocol) Code Mouse */
#define HID_BOOT_IFACE_CODE_MOUSE	2

/** USB HID Class Boot protocol code */
#define HID_PROTOCOL_BOOT		0
/** USB HID Class Report protocol code */
#define HID_PROTOCOL_REPORT		1

/** HID Main item type */
#define HID_ITEM_TYPE_MAIN		0x0
/** HID Global item type */
#define HID_ITEM_TYPE_GLOBAL		0x1
/** HID Local item type */
#define HID_ITEM_TYPE_LOCAL		0x2

/** HID Input item tag */
#define HID_ITEM_TAG_INPUT		0x8
/** HID Output item tag */
#define HID_ITEM_TAG_OUTPUT		0x9
/** HID Collection item tag */
#define HID_ITEM_TAG_COLLECTION		0xA
/** HID Feature item tag */
#define HID_ITEM_TAG_FEATURE		0xB
/** HID End Collection item tag */
#define HID_ITEM_TAG_COLLECTION_END	0xC

/** HID Usage Page item tag */
#define HID_ITEM_TAG_USAGE_PAGE		0x0
/** HID Logical Minimum item tag */
#define HID_ITEM_TAG_LOGICAL_MIN	0x1
/** HID Logical Maximum item tag */
#define HID_ITEM_TAG_LOGICAL_MAX	0x2
/** HID Physical Minimum item tag */
#define HID_ITEM_TAG_PHYSICAL_MIN	0x3
/** HID Physical Maximum item tag */
#define HID_ITEM_TAG_PHYSICAL_MAX	0x4
/** HID Unit Exponent item tag */
#define HID_ITEM_TAG_UNIT_EXPONENT	0x5
/** HID Unit item tag */
#define HID_ITEM_TAG_UNIT		0x6
/** HID Report Size item tag */
#define HID_ITEM_TAG_REPORT_SIZE	0x7
/** HID Report ID item tag */
#define HID_ITEM_TAG_REPORT_ID		0x8
/** HID Report count item tag */
#define HID_ITEM_TAG_REPORT_COUNT	0x9

/** HID Usage item tag */
#define HID_ITEM_TAG_USAGE		0x0
/** HID Usage Minimum item tag */
#define HID_ITEM_TAG_USAGE_MIN		0x1
/** HID Usage Maximum item tag */
#define HID_ITEM_TAG_USAGE_MAX		0x2

/** Physical collection type */
#define HID_COLLECTION_PHYSICAL		0x00
/** Application collection type */
#define HID_COLLECTION_APPLICATION	0x01
/** Logical collection type */
#define HID_COLLECTION_LOGICAL		0x02
/** Report collection type */
#define HID_COLLECTION_REPORT		0x03
/** Named Array collection type */
#define HID_COLLECTION_NAMED_ARRAY	0x04
/** Usage Switch collection type */
#define HID_COLLECTION_USAGE_SWITCH	0x05
/** Modifier collection type */
#define HID_COLLECTION_MODIFIER		0x06


/* Usage page and IDs from Universal Serial Bus HID Usage Tables */

/** HID Generic Desktop Controls Usage page */
#define HID_USAGE_GEN_DESKTOP		0x01
/** HID Keyboard Usage page */
#define HID_USAGE_GEN_KEYBOARD		0x07
/** HID LEDs Usage page */
#define HID_USAGE_GEN_LEDS		0x08
/** HID Button Usage page */
#define HID_USAGE_GEN_BUTTON		0x09

/** HID Generic Desktop Undefined Usage ID */
#define HID_USAGE_GEN_DESKTOP_UNDEFINED	0x00
/** HID Generic Desktop Pointer Usage ID */
#define HID_USAGE_GEN_DESKTOP_POINTER	0x01
/** HID Generic Desktop Mouse Usage ID */
#define HID_USAGE_GEN_DESKTOP_MOUSE	0x02
/** HID Generic Desktop Joystick Usage ID */
#define HID_USAGE_GEN_DESKTOP_JOYSTICK	0x04
/** HID Generic Desktop Gamepad Usage ID */
#define HID_USAGE_GEN_DESKTOP_GAMEPAD	0x05
/** HID Generic Desktop Keyboard Usage ID */
#define HID_USAGE_GEN_DESKTOP_KEYBOARD	0x06
/** HID Generic Desktop Keypad Usage ID */
#define HID_USAGE_GEN_DESKTOP_KEYPAD	0x07
/** HID Generic Desktop X Usage ID */
#define HID_USAGE_GEN_DESKTOP_X		0x30
/** HID Generic Desktop Y Usage ID */
#define HID_USAGE_GEN_DESKTOP_Y		0x31
/** HID Generic Desktop Wheel Usage ID */
#define HID_USAGE_GEN_DESKTOP_WHEEL	0x38

/**
 * @}
 */

/**
 * @defgroup usb_hid_items USB HID Item helpers
 * @ingroup usb_hid_definitions
 * @{
 */

/**
 * @brief Define HID short item.
 *
 * @param bTag  Item tag
 * @param bType Item type
 * @param bSize Item data size
 * @return      HID Input item
 */
#define HID_ITEM(bTag, bType, bSize)	(((bTag & 0xF) << 4) | \
					 ((bType & 0x3) << 2) | (bSize & 0x3))

/**
 * @brief  Define HID Input item with the data length of one byte.
 *
 * For usage examples, see @ref HID_MOUSE_REPORT_DESC(),
 * @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @param a Input item data
 * @return  HID Input item
 */
#define HID_INPUT(a)			\
	HID_ITEM(HID_ITEM_TAG_INPUT, HID_ITEM_TYPE_MAIN, 1), a

/**
 * @brief Define HID Output item with the data length of one byte.
 *
 * For usage examples, see @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @param a Output item data
 * @return  HID Output item
 */
#define HID_OUTPUT(a)			\
	HID_ITEM(HID_ITEM_TAG_OUTPUT, HID_ITEM_TYPE_MAIN, 1), a

/**
 * @brief Define HID Feature item with the data length of one byte.
 *
 * @param a Feature item data
 * @return  HID Feature item
 */
#define HID_FEATURE(a)			\
	HID_ITEM(HID_ITEM_TAG_FEATURE, HID_ITEM_TYPE_MAIN, 1), a

/**
 * @brief Define HID Collection item with the data length of one byte.
 *
 * For usage examples, see @ref HID_MOUSE_REPORT_DESC(),
 * @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @param a Collection item data
 * @return  HID Collection item
 */
#define HID_COLLECTION(a)		\
	HID_ITEM(HID_ITEM_TAG_COLLECTION, HID_ITEM_TYPE_MAIN, 1), a

/**
 * @brief Define HID End Collection (non-data) item.
 *
 * For usage examples, see @ref HID_MOUSE_REPORT_DESC(),
 * @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @return  HID End Collection item
 */
#define HID_END_COLLECTION		\
	HID_ITEM(HID_ITEM_TAG_COLLECTION_END, HID_ITEM_TYPE_MAIN, 0)

/**
 * @brief Define HID Usage Page item.
 *
 * For usage examples, see @ref HID_MOUSE_REPORT_DESC(),
 * @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @param page Usage Page
 * @return     HID Usage Page item
 */
#define HID_USAGE_PAGE(page)		\
	HID_ITEM(HID_ITEM_TAG_USAGE_PAGE, HID_ITEM_TYPE_GLOBAL, 1), page

/**
 * @brief Define HID Logical Minimum item with the data length of one byte.
 *
 * For usage examples, see @ref HID_MOUSE_REPORT_DESC(),
 * @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @param a Minimum value in logical units
 * @return  HID Logical Minimum item
 */
#define HID_LOGICAL_MIN8(a)		\
	HID_ITEM(HID_ITEM_TAG_LOGICAL_MIN, HID_ITEM_TYPE_GLOBAL, 1), a

/**
 * @brief Define HID Logical Maximum item with the data length of one byte.
 *
 * For usage examples, see @ref HID_MOUSE_REPORT_DESC(),
 * @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @param a Maximum value in logical units
 * @return  HID Logical Maximum item
 */
#define HID_LOGICAL_MAX8(a)		\
	HID_ITEM(HID_ITEM_TAG_LOGICAL_MAX, HID_ITEM_TYPE_GLOBAL, 1), a

/**
 * @brief Define HID Logical Minimum item with the data length of two bytes.
 *
 * @param a Minimum value lower byte
 * @param b Minimum value higher byte
 * @return  HID Logical Minimum item
 */
#define HID_LOGICAL_MIN16(a, b)		\
	HID_ITEM(HID_ITEM_TAG_LOGICAL_MIN, HID_ITEM_TYPE_GLOBAL, 2), a, b

/**
 * @brief Define HID Logical Maximum item with the data length of two bytes.
 *
 * @param a Minimum value lower byte
 * @param b Minimum value higher byte
 * @return  HID Logical Maximum item
 */
#define HID_LOGICAL_MAX16(a, b)		\
	HID_ITEM(HID_ITEM_TAG_LOGICAL_MAX, HID_ITEM_TYPE_GLOBAL, 2), a, b

/**
 * @brief Define HID Logical Minimum item with the data length of four bytes.
 *
 * @param a Minimum value lower byte
 * @param b Minimum value low middle byte
 * @param c Minimum value high middle byte
 * @param d Minimum value higher byte
 * @return  HID Logical Minimum item
 */
#define HID_LOGICAL_MIN32(a, b, c, d)	\
	HID_ITEM(HID_ITEM_TAG_LOGICAL_MIN, HID_ITEM_TYPE_GLOBAL, 3), a, b, c, d

/**
 * @brief Define HID Logical Maximum item with the data length of four bytes.
 *
 * @param a Minimum value lower byte
 * @param b Minimum value low middle byte
 * @param c Minimum value high middle byte
 * @param d Minimum value higher byte
 * @return  HID Logical Maximum item
 */
#define HID_LOGICAL_MAX32(a, b, c, d)	\
	HID_ITEM(HID_ITEM_TAG_LOGICAL_MAX, HID_ITEM_TYPE_GLOBAL, 3), a, b, c, d

/**
 * @brief Define HID Report Size item with the data length of one byte.
 *
 * For usage examples, see @ref HID_MOUSE_REPORT_DESC(),
 * @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @param size Report field size in bits
 * @return     HID Report Size item
 */
#define HID_REPORT_SIZE(size)		\
	HID_ITEM(HID_ITEM_TAG_REPORT_SIZE, HID_ITEM_TYPE_GLOBAL, 1), size

/**
 * @brief Define HID Report ID item with the data length of one byte.
 *
 * @param id Report ID
 * @return   HID Report ID item
 */
#define HID_REPORT_ID(id)		\
	HID_ITEM(HID_ITEM_TAG_REPORT_ID, HID_ITEM_TYPE_GLOBAL, 1), id

/**
 * @brief Define HID Report Count item with the data length of one byte.
 *
 * For usage examples, see @ref HID_MOUSE_REPORT_DESC(),
 * @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @param count Number of data fields included in the report
 * @return      HID Report Count item
 */
#define HID_REPORT_COUNT(count)		\
	HID_ITEM(HID_ITEM_TAG_REPORT_COUNT, HID_ITEM_TYPE_GLOBAL, 1), count

/**
 * @brief Define HID Usage Index item with the data length of one byte.
 *
 * For usage examples, see @ref HID_MOUSE_REPORT_DESC(),
 * @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @param idx Number of data fields included in the report
 * @return    HID Usage Index item
 */
#define HID_USAGE(idx)			\
	HID_ITEM(HID_ITEM_TAG_USAGE, HID_ITEM_TYPE_LOCAL, 1), idx

/**
 * @brief Define HID Usage Minimum item with the data length of one byte.
 *
 * For usage examples, see @ref HID_MOUSE_REPORT_DESC(),
 * @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @param a Starting Usage
 * @return  HID Usage Minimum item
 */
#define HID_USAGE_MIN8(a)		\
	HID_ITEM(HID_ITEM_TAG_USAGE_MIN, HID_ITEM_TYPE_LOCAL, 1), a

/**
 * @brief Define HID Usage Maximum item with the data length of one byte.
 *
 * For usage examples, see @ref HID_MOUSE_REPORT_DESC(),
 * @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @param a Ending Usage
 * @return  HID Usage Maximum item
 */
#define HID_USAGE_MAX8(a)		\
	HID_ITEM(HID_ITEM_TAG_USAGE_MAX, HID_ITEM_TYPE_LOCAL, 1), a

/**
 * @brief Define HID Usage Minimum item with the data length of two bytes.
 *
 * For usage examples, see @ref HID_MOUSE_REPORT_DESC(),
 * @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @param a Starting Usage lower byte
 * @param b Starting Usage higher byte
 * @return  HID Usage Minimum item
 */
#define HID_USAGE_MIN16(a, b)		\
	HID_ITEM(HID_ITEM_TAG_USAGE_MIN, HID_ITEM_TYPE_LOCAL, 2), a, b

/**
 * @brief Define HID Usage Maximum item with the data length of two bytes.
 *
 * For usage examples, see @ref HID_MOUSE_REPORT_DESC(),
 * @ref HID_KEYBOARD_REPORT_DESC()
 *
 * @param a Ending Usage lower byte
 * @param b Ending Usage higher byte
 * @return  HID Usage Maximum item
 */
#define HID_USAGE_MAX16(a, b)		\
	HID_ITEM(HID_ITEM_TAG_USAGE_MAX, HID_ITEM_TYPE_LOCAL, 2), a, b

/**
 * @}
 */

/**
 * @defgroup usb_hid_mk_report_desc Mouse and keyboard report descriptors
 * @ingroup usb_hid_definitions
 * @{
 */

/**
 * @brief Simple HID mouse report descriptor for n button mouse.
 *
 * @param bcnt Button count. Allowed values from 1 to 8.
 */
#define HID_MOUSE_REPORT_DESC(bcnt) {				\
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),			\
	HID_USAGE(HID_USAGE_GEN_DESKTOP_MOUSE),			\
	HID_COLLECTION(HID_COLLECTION_APPLICATION),		\
		HID_USAGE(HID_USAGE_GEN_DESKTOP_POINTER),	\
		HID_COLLECTION(HID_COLLECTION_PHYSICAL),	\
			/* Bits used for button signalling */	\
			HID_USAGE_PAGE(HID_USAGE_GEN_BUTTON),	\
			HID_USAGE_MIN8(1),			\
			HID_USAGE_MAX8(bcnt),			\
			HID_LOGICAL_MIN8(0),			\
			HID_LOGICAL_MAX8(1),			\
			HID_REPORT_SIZE(1),			\
			HID_REPORT_COUNT(bcnt),			\
			/* HID_INPUT (Data,Var,Abs) */		\
			HID_INPUT(0x02),			\
			/* Unused bits */			\
			HID_REPORT_SIZE(8 - bcnt),		\
			HID_REPORT_COUNT(1),			\
			/* HID_INPUT (Cnst,Ary,Abs) */		\
			HID_INPUT(1),				\
			/* X and Y axis, scroll */		\
			HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),	\
			HID_USAGE(HID_USAGE_GEN_DESKTOP_X),	\
			HID_USAGE(HID_USAGE_GEN_DESKTOP_Y),	\
			HID_USAGE(HID_USAGE_GEN_DESKTOP_WHEEL),	\
			HID_LOGICAL_MIN8(-127),			\
			HID_LOGICAL_MAX8(127),			\
			HID_REPORT_SIZE(8),			\
			HID_REPORT_COUNT(3),			\
			/* HID_INPUT (Data,Var,Rel) */		\
			HID_INPUT(0x06),			\
		HID_END_COLLECTION,				\
	HID_END_COLLECTION,					\
}

/**
 * @brief Simple HID keyboard report descriptor.
 */
#define HID_KEYBOARD_REPORT_DESC() {				\
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),			\
	HID_USAGE(HID_USAGE_GEN_DESKTOP_KEYBOARD),		\
	HID_COLLECTION(HID_COLLECTION_APPLICATION),		\
		HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP_KEYPAD),	\
		/* HID_USAGE_MINIMUM(Keyboard LeftControl) */	\
		HID_USAGE_MIN8(0xE0),				\
		/* HID_USAGE_MAXIMUM(Keyboard Right GUI) */	\
		HID_USAGE_MAX8(0xE7),				\
		HID_LOGICAL_MIN8(0),				\
		HID_LOGICAL_MAX8(1),				\
		HID_REPORT_SIZE(1),				\
		HID_REPORT_COUNT(8),				\
		/* HID_INPUT(Data,Var,Abs) */			\
		HID_INPUT(0x02),				\
		HID_REPORT_SIZE(8),				\
		HID_REPORT_COUNT(1),				\
		/* HID_INPUT(Cnst,Var,Abs) */			\
		HID_INPUT(0x03),				\
		HID_REPORT_SIZE(1),				\
		HID_REPORT_COUNT(5),				\
		HID_USAGE_PAGE(HID_USAGE_GEN_LEDS),		\
		/* HID_USAGE_MINIMUM(Num Lock) */		\
		HID_USAGE_MIN8(1),				\
		/* HID_USAGE_MAXIMUM(Kana) */			\
		HID_USAGE_MAX8(5),				\
		/* HID_OUTPUT(Data,Var,Abs) */			\
		HID_OUTPUT(0x02),				\
		HID_REPORT_SIZE(3),				\
		HID_REPORT_COUNT(1),				\
		/* HID_OUTPUT(Cnst,Var,Abs) */			\
		HID_OUTPUT(0x03),				\
		HID_REPORT_SIZE(8),				\
		HID_REPORT_COUNT(6),				\
		HID_LOGICAL_MIN8(0),				\
		HID_LOGICAL_MAX8(101),				\
		HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP_KEYPAD),	\
		/* HID_USAGE_MIN8(Reserved) */			\
		HID_USAGE_MIN8(0),				\
		/* HID_USAGE_MAX8(Keyboard Application) */	\
		HID_USAGE_MAX8(101),				\
		/* HID_INPUT (Data,Ary,Abs) */			\
		HID_INPUT(0x00),				\
	HID_END_COLLECTION,					\
}

/**
 * @brief HID keyboard button codes.
 */
enum hid_kbd_code {
	HID_KEY_A		= 4,
	HID_KEY_B		= 5,
	HID_KEY_C		= 6,
	HID_KEY_D		= 7,
	HID_KEY_E		= 8,
	HID_KEY_F		= 9,
	HID_KEY_G		= 10,
	HID_KEY_H		= 11,
	HID_KEY_I		= 12,
	HID_KEY_J		= 13,
	HID_KEY_K		= 14,
	HID_KEY_L		= 15,
	HID_KEY_M		= 16,
	HID_KEY_N		= 17,
	HID_KEY_O		= 18,
	HID_KEY_P		= 19,
	HID_KEY_Q		= 20,
	HID_KEY_R		= 21,
	HID_KEY_S		= 22,
	HID_KEY_T		= 23,
	HID_KEY_U		= 24,
	HID_KEY_V		= 25,
	HID_KEY_W		= 26,
	HID_KEY_X		= 27,
	HID_KEY_Y		= 28,
	HID_KEY_Z		= 29,
	HID_KEY_1		= 30,
	HID_KEY_2		= 31,
	HID_KEY_3		= 32,
	HID_KEY_4		= 33,
	HID_KEY_5		= 34,
	HID_KEY_6		= 35,
	HID_KEY_7		= 36,
	HID_KEY_8		= 37,
	HID_KEY_9		= 38,
	HID_KEY_0		= 39,
	HID_KEY_ENTER		= 40,
	HID_KEY_ESC		= 41,
	HID_KEY_BACKSPACE	= 42,
	HID_KEY_TAB		= 43,
	HID_KEY_SPACE		= 44,
	HID_KEY_MINUS		= 45,
	HID_KEY_EQUAL		= 46,
	HID_KEY_LEFTBRACE	= 47,
	HID_KEY_RIGHTBRACE	= 48,
	HID_KEY_BACKSLASH	= 49,
	HID_KEY_HASH		= 50, /* Non-US # and ~ */
	HID_KEY_SEMICOLON	= 51,
	HID_KEY_APOSTROPHE	= 52,
	HID_KEY_GRAVE		= 53,
	HID_KEY_COMMA		= 54,
	HID_KEY_DOT		= 55,
	HID_KEY_SLASH		= 56,
	HID_KEY_CAPSLOCK	= 57,
	HID_KEY_F1		= 58,
	HID_KEY_F2		= 59,
	HID_KEY_F3		= 60,
	HID_KEY_F4		= 61,
	HID_KEY_F5		= 62,
	HID_KEY_F6		= 63,
	HID_KEY_F7		= 64,
	HID_KEY_F8		= 65,
	HID_KEY_F9		= 66,
	HID_KEY_F10		= 67,
	HID_KEY_F11		= 68,
	HID_KEY_F12		= 69,
	HID_KEY_SYSRQ		= 70, /* PRINTSCREEN */
	HID_KEY_SCROLLLOCK	= 71,
	HID_KEY_PAUSE		= 72,
	HID_KEY_INSERT		= 73,
	HID_KEY_HOME		= 74,
	HID_KEY_PAGEUP		= 75,
	HID_KEY_DELETE		= 76,
	HID_KEY_END		= 77,
	HID_KEY_PAGEDOWN	= 78,
	HID_KEY_RIGHT		= 79,
	HID_KEY_LEFT		= 80,
	HID_KEY_DOWN		= 81,
	HID_KEY_UP		= 82,
	HID_KEY_NUMLOCK		= 83,
	HID_KEY_KPSLASH		= 84, /* NUMPAD DIVIDE */
	HID_KEY_KPASTERISK	= 85, /* NUMPAD MULTIPLY */
	HID_KEY_KPMINUS		= 86,
	HID_KEY_KPPLUS		= 87,
	HID_KEY_KPENTER		= 88,
	HID_KEY_KP_1		= 89,
	HID_KEY_KP_2		= 90,
	HID_KEY_KP_3		= 91,
	HID_KEY_KP_4		= 92,
	HID_KEY_KP_5		= 93,
	HID_KEY_KP_6		= 94,
	HID_KEY_KP_7		= 95,
	HID_KEY_KP_8		= 96,
	HID_KEY_KP_9		= 97,
	HID_KEY_KP_0		= 98,
};

/**
 * @brief HID keyboard modifiers.
 */
enum hid_kbd_modifier {
	HID_KBD_MODIFIER_NONE		= 0x00,
	HID_KBD_MODIFIER_LEFT_CTRL	= 0x01,
	HID_KBD_MODIFIER_LEFT_SHIFT	= 0x02,
	HID_KBD_MODIFIER_LEFT_ALT	= 0x04,
	HID_KBD_MODIFIER_LEFT_UI	= 0x08,
	HID_KBD_MODIFIER_RIGHT_CTRL	= 0x10,
	HID_KBD_MODIFIER_RIGHT_SHIFT	= 0x20,
	HID_KBD_MODIFIER_RIGHT_ALT	= 0x40,
	HID_KBD_MODIFIER_RIGHT_UI	= 0x80,
};

/**
 * @brief HID keyboard LEDs.
 */
enum hid_kbd_led {
	HID_KBD_LED_NUM_LOCK	= 0x01,
	HID_KBD_LED_CAPS_LOCK	= 0x02,
	HID_KBD_LED_SCROLL_LOCK	= 0x04,
	HID_KBD_LED_COMPOSE	= 0x08,
	HID_KBD_LED_KANA	= 0x10,
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CLASS_HID_H_ */
