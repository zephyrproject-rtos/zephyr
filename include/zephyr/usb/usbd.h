/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief New experimental USB device stack APIs and structures
 *
 * This file contains the USB device stack APIs and structures.
 */

#ifndef ZEPHYR_INCLUDE_USBD_H_
#define ZEPHYR_INCLUDE_USBD_H_

#include <zephyr/device.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usbd_msg.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief New USB device stack core API
 * @defgroup usbd_api USB device core API
 * @ingroup usb
 * @{
 */

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

/* Used internally to keep descriptors in order */
enum usbd_desc_usage_type {
	USBD_DUT_STRING_LANG,
	USBD_DUT_STRING_MANUFACTURER,
	USBD_DUT_STRING_PRODUCT,
	USBD_DUT_STRING_SERIAL_NUMBER,
	USBD_DUT_STRING_INTERFACE,
};

/**
 * Descriptor node
 *
 * Descriptor node is used to manage descriptors that are not
 * directly part of a structure, such as string or bos descriptors.
 */
struct usbd_desc_node {
	/** slist node struct */
	sys_dnode_t node;
	/** Descriptor index, required for string descriptors */
	unsigned int idx : 8;
	/** Descriptor usage type (not bDescriptorType) */
	unsigned int utype : 8;
	/** If not set, string descriptor must be converted to UTF16LE */
	unsigned int utf16le : 1;
	/** If not set, device stack obtains SN using the hwinfo API */
	unsigned int custom_sn : 1;
	/** Pointer to a descriptor */
	void *desc;
};

/**
 * Device configuration node
 *
 * Configuration node is used to manage device configurations,
 * at least one configuration is required. It does not have an index,
 * instead bConfigurationValue of the descriptor is used for
 * identification.
 */
struct usbd_config_node {
	/** slist node struct */
	sys_snode_t node;
	/** Pointer to configuration descriptor */
	void *desc;
	/** List of registered classes (functions) */
	sys_slist_t class_list;
};

/* TODO: Kconfig option USBD_NUMOF_INTERFACES_MAX? */
#define USBD_NUMOF_INTERFACES_MAX	16U

/**
 * USB device support middle layer runtime state
 *
 * Part of USB device states without suspended and powered
 * states, as it is better to track them separately.
 */
enum usbd_ch9_state {
	USBD_STATE_DEFAULT = 0,
	USBD_STATE_ADDRESS,
	USBD_STATE_CONFIGURED,
};


/**
 * USB device support middle layer runtime data
 */
struct usbd_ch9_data {
	/** Setup packet, up-to-date for the respective control request */
	struct usb_setup_packet setup;
	/** Control type, internally used for stage verification */
	int ctrl_type;
	/** Protocol state of the USB device stack */
	enum usbd_ch9_state state;
	/** Halted endpoints bitmap */
	uint32_t ep_halt;
	/** USB device stack selected configuration */
	uint8_t configuration;
	/** Post status stage work required, e.g. set new device address */
	bool post_status;
	/** Array to track interfaces alternate settings */
	uint8_t alternate[USBD_NUMOF_INTERFACES_MAX];
};

/**
 * @brief USB device speed
 */
enum usbd_speed {
	/** Device supports or is connected to a full speed bus */
	USBD_SPEED_FS,
	/** Device supports or is connected to a high speed bus  */
	USBD_SPEED_HS,
	/** Device supports or is connected to a super speed bus */
	USBD_SPEED_SS,
};

/**
 * USB device support status
 */
struct usbd_status {
	/** USB device support is initialized */
	unsigned int initialized : 1;
	/** USB device support is enabled */
	unsigned int enabled : 1;
	/** USB device is suspended */
	unsigned int suspended : 1;
	/** USB remote wake-up feature is enabled */
	unsigned int rwup : 1;
	/** USB device speed */
	enum usbd_speed speed : 2;
};

/**
 * USB device support runtime context
 *
 * Main structure that organizes all descriptors, configuration,
 * and interfaces. An UDC device must be assigned to this structure.
 */
struct usbd_contex {
	/** Name of the USB device */
	const char *name;
	/** Access mutex */
	struct k_mutex mutex;
	/** Pointer to UDC device */
	const struct device *dev;
	/** Notification message recipient callback */
	usbd_msg_cb_t msg_cb;
	/** Middle layer runtime data */
	struct usbd_ch9_data ch9_data;
	/** slist to manage descriptors like string, bos */
	sys_dlist_t descriptors;
	/** slist to manage Full-Speed device configurations */
	sys_slist_t fs_configs;
	/** slist to manage High-Speed device configurations */
	sys_slist_t hs_configs;
	/** Status of the USB device support */
	struct usbd_status status;
	/** Pointer to Full-Speed device descriptor */
	void *fs_desc;
	/** Pointer to High-Speed device descriptor */
	void *hs_desc;
};

/**
 * @brief Vendor Requests Table
 */
struct usbd_cctx_vendor_req {
	/** Array of vendor requests supported by the class */
	const uint8_t *reqs;
	/** Length of the array */
	uint8_t len;
};

/** USB Class instance registered flag */
#define USBD_CCTX_REGISTERED		0

struct usbd_class_node;

/**
 * @brief USB device support class instance API
 */
struct usbd_class_api {
	/** Feature halt state update handler */
	void (*feature_halt)(struct usbd_class_node *const node,
			     uint8_t ep, bool halted);

	/** Configuration update handler */
	void (*update)(struct usbd_class_node *const node,
		       uint8_t iface, uint8_t alternate);

	/** USB control request handler to device */
	int (*control_to_dev)(struct usbd_class_node *const node,
			      const struct usb_setup_packet *const setup,
			      const struct net_buf *const buf);

	/** USB control request handler to host */
	int (*control_to_host)(struct usbd_class_node *const node,
			       const struct usb_setup_packet *const setup,
			       struct net_buf *const buf);

	/** Endpoint request completion event handler */
	int (*request)(struct usbd_class_node *const node,
		       struct net_buf *buf, int err);

	/** USB power management handler suspended */
	void (*suspended)(struct usbd_class_node *const node);

	/** USB power management handler resumed */
	void (*resumed)(struct usbd_class_node *const node);

	/** Start of Frame */
	void (*sof)(struct usbd_class_node *const node);

	/** Class associated configuration is selected */
	void (*enable)(struct usbd_class_node *const node);

	/** Class associated configuration is disabled */
	void (*disable)(struct usbd_class_node *const node);

	/** Initialization of the class implementation */
	int (*init)(struct usbd_class_node *const node);

	/** Shutdown of the class implementation */
	void (*shutdown)(struct usbd_class_node *const node);

	/** Get function descriptor based on speed parameter */
	void *(*get_desc)(struct usbd_class_node *const node,
			  const enum usbd_speed speed);
};

/**
 * @brief USB device support class data
 */
struct usbd_class_data {
	/** Pointer to USB device stack context structure */
	struct usbd_contex *uds_ctx;
	/** Supported vendor request table, can be NULL */
	const struct usbd_cctx_vendor_req *v_reqs;
	/** Pointer to private data */
	void *priv;
};

struct usbd_class_node {
	/** Name of the USB device class instance */
	const char *name;
	/** Pointer to device support class API */
	const struct usbd_class_api *api;
	/** Pointer to USB device support class data */
	struct usbd_class_data *data;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Variables necessary for per speed class management. For each speed (Full,
 * High) there is separate `struct usbd_class_iter` pointing to the same
 * `struct usbd_class_node` (because the class can only operate at one speed
 * at a time).
 */
struct usbd_class_iter {
	/** Node information for the slist. */
	sys_snode_t node;
	/** Pointer to public class node instance. */
	struct usbd_class_node *const c_nd;
	/** Bitmap of all endpoints assigned to the instance.
	 *  The IN endpoints are mapped in the upper halfword.
	 */
	uint32_t ep_assigned;
	/** Bitmap of the enabled endpoints of the instance.
	 *  The IN endpoints are mapped in the upper halfword.
	 */
	uint32_t ep_active;
	/** Bitmap of the bInterfaceNumbers of the class instance */
	uint32_t iface_bm;
	/** Variable to store the state of the class instance */
	atomic_t state;
};

/** @endcond */

/**
 * @brief Get the USB device runtime context under which the class is registered
 *
 * The class implementation must use this function and not access the members
 * of the struct directly.
 *
 * @param[in] node Pointer to USB device class node
 *
 * @return Pointer to USB device runtime context
 */
static inline struct usbd_contex *usbd_class_get_ctx(const struct usbd_class_node *const c_nd)
{
	struct usbd_class_data *const c_data = c_nd->data;

	return c_data->uds_ctx;
}

/**
 * @brief Get class implementation private data
 *
 * The class implementation must use this function and not access the members
 * of the struct directly.
 *
 * @param[in] node Pointer to USB device class node
 *
 * @return Pointer to class implementation private data
 */
static inline void *usbd_class_get_private(const struct usbd_class_node *const c_nd)
{
	struct usbd_class_data *const c_data = c_nd->data;

	return c_data->priv;
}

#define USBD_DEVICE_DEFINE(device_name, uhc_dev, vid, pid)		\
	static struct usb_device_descriptor				\
	fs_desc_##device_name = {					\
		.bLength = sizeof(struct usb_device_descriptor),	\
		.bDescriptorType = USB_DESC_DEVICE,			\
		.bcdUSB = sys_cpu_to_le16(USB_SRN_2_0),			\
		.bDeviceClass = USB_BCC_MISCELLANEOUS,			\
		.bDeviceSubClass = 2,					\
		.bDeviceProtocol = 1,					\
		.bMaxPacketSize0 = USB_CONTROL_EP_MPS,			\
		.idVendor = vid,					\
		.idProduct = pid,					\
		.bcdDevice = sys_cpu_to_le16(USB_BCD_DRN),		\
		.iManufacturer = 0,					\
		.iProduct = 0,						\
		.iSerialNumber = 0,					\
		.bNumConfigurations = 0,				\
	};								\
	static struct usb_device_descriptor				\
	hs_desc_##device_name = {					\
		.bLength = sizeof(struct usb_device_descriptor),	\
		.bDescriptorType = USB_DESC_DEVICE,			\
		.bcdUSB = sys_cpu_to_le16(USB_SRN_2_0),			\
		.bDeviceClass = USB_BCC_MISCELLANEOUS,			\
		.bDeviceSubClass = 2,					\
		.bDeviceProtocol = 1,					\
		.bMaxPacketSize0 = 64,					\
		.idVendor = vid,					\
		.idProduct = pid,					\
		.bcdDevice = sys_cpu_to_le16(USB_BCD_DRN),		\
		.iManufacturer = 0,					\
		.iProduct = 0,						\
		.iSerialNumber = 0,					\
		.bNumConfigurations = 0,				\
	};								\
	static STRUCT_SECTION_ITERABLE(usbd_contex, device_name) = {	\
		.name = STRINGIFY(device_name),				\
		.dev = uhc_dev,						\
		.fs_desc = &fs_desc_##device_name,			\
		.hs_desc = &hs_desc_##device_name,			\
	}

#define USBD_CONFIGURATION_DEFINE(name, attrib, power)			\
	static struct usb_cfg_descriptor				\
	cfg_desc_##name = {						\
		.bLength = sizeof(struct usb_cfg_descriptor),		\
		.bDescriptorType = USB_DESC_CONFIGURATION,		\
		.wTotalLength = 0,					\
		.bNumInterfaces = 0,					\
		.bConfigurationValue = 1,				\
		.iConfiguration = 0,					\
		.bmAttributes = USB_SCD_RESERVED | (attrib),		\
		.bMaxPower = (power),					\
	};								\
	BUILD_ASSERT((power) < 256, "Too much power");			\
	static struct usbd_config_node name = {				\
		.desc = &cfg_desc_##name,				\
	}

/**
 * @brief Create a string descriptor node and language string descriptor
 *
 * This macro defines a descriptor node and a string descriptor that,
 * when added to the device context, is automatically used as the language
 * string descriptor zero. Both descriptor node and descriptor are defined with
 * static-storage-class specifier. Default and currently only supported
 * language ID is 0x0409 English (United States).
 * If string descriptors are used, it is necessary to add this descriptor
 * as the first one to the USB device context.
 *
 * @param name Language string descriptor node identifier.
 */
#define USBD_DESC_LANG_DEFINE(name)					\
	static struct usb_string_descriptor				\
	string_desc_##name = {						\
		.bLength = sizeof(struct usb_string_descriptor),	\
		.bDescriptorType = USB_DESC_STRING,			\
		.bString = sys_cpu_to_le16(0x0409),			\
	};								\
	static struct usbd_desc_node name = {				\
		.idx = 0,						\
		.utype = USBD_DUT_STRING_LANG,				\
		.desc = &string_desc_##name,				\
	}

#define USBD_DESC_STRING_DEFINE(d_name, d_string, d_utype)		\
	struct usb_string_descriptor_##d_name {				\
		uint8_t bLength;					\
		uint8_t bDescriptorType;				\
		uint8_t bString[USB_BSTRING_LENGTH(d_string)];		\
	} __packed;							\
	static struct usb_string_descriptor_##d_name			\
	string_desc_##d_name = {					\
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(d_string),	\
		.bDescriptorType = USB_DESC_STRING,			\
		.bString = d_string,					\
	};								\
	static struct usbd_desc_node d_name = {				\
		.utype = d_utype,					\
		.desc = &string_desc_##d_name,				\
	}

/**
 * @brief Create a string descriptor node and manufacturer string descriptor
 *
 * This macro defines a descriptor node and a string descriptor that,
 * when added to the device context, is automatically used as the manufacturer
 * string descriptor. Both descriptor node and descriptor are defined with
 * static-storage-class specifier.
 *
 * @param d_name   String descriptor node identifier.
 * @param d_string ASCII7 encoded manufacturer string literal
 */
#define USBD_DESC_MANUFACTURER_DEFINE(d_name, d_string)			\
	USBD_DESC_STRING_DEFINE(d_name, d_string, USBD_DUT_STRING_MANUFACTURER)

/**
 * @brief Create a string descriptor node and product string descriptor
 *
 * This macro defines a descriptor node and a string descriptor that,
 * when added to the device context, is automatically used as the product
 * string descriptor. Both descriptor node and descriptor are defined with
 * static-storage-class specifier.
 *
 * @param d_name   String descriptor node identifier.
 * @param d_string ASCII7 encoded product string literal
 */
#define USBD_DESC_PRODUCT_DEFINE(d_name, d_string)			\
	USBD_DESC_STRING_DEFINE(d_name, d_string, USBD_DUT_STRING_PRODUCT)

/**
 * @brief Create a string descriptor node and serial number string descriptor
 *
 * This macro defines a descriptor node and a string descriptor that,
 * when added to the device context, is automatically used as the serial number
 * string descriptor. The string literal parameter is used as a placeholder,
 * the unique number is obtained from hwinfo. Both descriptor node and descriptor
 * are defined with static-storage-class specifier.
 *
 * @param d_name   String descriptor node identifier.
 * @param d_string ASCII7 encoded serial number string literal placeholder
 */
#define USBD_DESC_SERIAL_NUMBER_DEFINE(d_name, d_string)		\
	USBD_DESC_STRING_DEFINE(d_name, d_string, USBD_DUT_STRING_SERIAL_NUMBER)

#define USBD_DEFINE_CLASS(class_name, class_api, class_data)		\
	static struct usbd_class_node class_name = {			\
		.name = STRINGIFY(class_name),				\
		.api = class_api,					\
		.data = class_data,					\
	};								\
	static STRUCT_SECTION_ITERABLE_ALTERNATE(			\
		usbd_class_fs, usbd_class_iter, class_name##_fs) = {	\
		.c_nd = &class_name,					\
	};								\
	static STRUCT_SECTION_ITERABLE_ALTERNATE(			\
		usbd_class_hs, usbd_class_iter, class_name##_hs) = {	\
		.c_nd = &class_name,					\
	}

/** @brief Helper to declare request table of usbd_cctx_vendor_req
 *
 *  @param _reqs Pointer to the vendor request field
 *  @param _len  Number of supported vendor requests
 */
#define VENDOR_REQ_DEFINE(_reqs, _len) \
	{ \
		.reqs = (const uint8_t *)(_reqs), \
		.len = (_len), \
	}

/** @brief Helper to declare supported vendor requests
 *
 *  @param _reqs Variable number of vendor requests
 */
#define USBD_VENDOR_REQ(_reqs...) \
	VENDOR_REQ_DEFINE(((uint8_t []) { _reqs }), \
			  sizeof((uint8_t []) { _reqs }))


/**
 * @brief Add common USB descriptor
 *
 * Add common descriptor like string or bos.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] dn      Pointer to USB descriptor node
 *
 * @return 0 on success, other values on fail.
 */
int usbd_add_descriptor(struct usbd_contex *uds_ctx,
			struct usbd_desc_node *dn);

/**
 * @brief Add a USB device configuration
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] speed   Speed at which this configuration operates
 * @param[in] cd      Pointer to USB configuration node
 *
 * @return 0 on success, other values on fail.
 */
int usbd_add_configuration(struct usbd_contex *uds_ctx,
			   const enum usbd_speed speed,
			   struct usbd_config_node *cd);

/**
 * @brief Register an USB class instance
 *
 * An USB class implementation can have one or more instances.
 * To identify the instances we use device drivers API.
 * Device names have a prefix derived from the name of the class,
 * for example CDC_ACM for CDC ACM class instance,
 * and can also be easily identified in the shell.
 * Class instance can only be registered when the USB device stack
 * is disabled.
 * Registered instances are initialized at initialization
 * of the USB device stack, and the interface descriptors
 * of each instance are adapted to the whole context.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] name    Class instance name
 * @param[in] speed   Configuration speed
 * @param[in] cfg     Configuration value (similar to bConfigurationValue)
 *
 * @return 0 on success, other values on fail.
 */
int usbd_register_class(struct usbd_contex *uds_ctx,
			const char *name,
			const enum usbd_speed speed, uint8_t cfg);

/**
 * @brief Unregister an USB class instance
 *
 * USB class instance will be removed and will not appear
 * on the next start of the stack. Instance can only be unregistered
 * when the USB device stack is disabled.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] name    Class instance name
 * @param[in] speed   Configuration speed
 * @param[in] cfg     Configuration value (similar to bConfigurationValue)
 *
 * @return 0 on success, other values on fail.
 */
int usbd_unregister_class(struct usbd_contex *uds_ctx,
			  const char *name,
			  const enum usbd_speed speed, uint8_t cfg);

/**
 * @brief Register USB notification message callback
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] cb      Pointer to message callback function
 *
 * @return 0 on success, other values on fail.
 */
int usbd_msg_register_cb(struct usbd_contex *const uds_ctx,
			 const usbd_msg_cb_t cb);

/**
 * @brief Initialize USB device
 *
 * Initialize USB device descriptors and configuration,
 * initialize USB device controller.
 * Class instances should be registered before they are involved.
 * However, the stack should also initialize without registered instances,
 * even if the host would complain about missing interfaces.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 *
 * @return 0 on success, other values on fail.
 */
int usbd_init(struct usbd_contex *uds_ctx);

/**
 * @brief Enable the USB device support and registered class instances
 *
 * This function enables the USB device support.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 *
 * @return 0 on success, other values on fail.
 */
int usbd_enable(struct usbd_contex *uds_ctx);

/**
 * @brief Disable the USB device support
 *
 * This function disables the USB device support.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 *
 * @return 0 on success, other values on fail.
 */
int usbd_disable(struct usbd_contex *uds_ctx);

/**
 * @brief Shutdown the USB device support
 *
 * This function completely disables the USB device support.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 *
 * @return 0 on success, other values on fail.
 */
int usbd_shutdown(struct usbd_contex *const uds_ctx);

/**
 * @brief Halt endpoint
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] ep      Endpoint address
 *
 * @return 0 on success, or error from udc_ep_set_halt()
 */
int usbd_ep_set_halt(struct usbd_contex *uds_ctx, uint8_t ep);

/**
 * @brief Clear endpoint halt
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] ep      Endpoint address
 *
 * @return 0 on success, or error from udc_ep_clear_halt()
 */
int usbd_ep_clear_halt(struct usbd_contex *uds_ctx, uint8_t ep);

/**
 * @brief Checks whether the endpoint is halted.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] ep      Endpoint address
 *
 * @return true if endpoint is halted, false otherwise
 */
bool usbd_ep_is_halted(struct usbd_contex *uds_ctx, uint8_t ep);

/**
 * @brief Allocate buffer for USB device control request
 *
 * Allocate a new buffer from controller's driver buffer pool.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] ep      Endpoint address
 * @param[in] size    Size of the request buffer
 *
 * @return pointer to allocated request or NULL on error.
 */
struct net_buf *usbd_ep_ctrl_buf_alloc(struct usbd_contex *const uds_ctx,
				       const uint8_t ep, const size_t size);

/**
 * @brief Allocate buffer for USB device request
 *
 * Allocate a new buffer from controller's driver buffer pool.
 *
 * @param[in] c_nd   Pointer to USB device class node
 * @param[in] ep     Endpoint address
 * @param[in] size   Size of the request buffer
 *
 * @return pointer to allocated request or NULL on error.
 */
struct net_buf *usbd_ep_buf_alloc(const struct usbd_class_node *const c_nd,
				  const uint8_t ep, const size_t size);

/**
 * @brief Queue USB device control request
 *
 * Add control request to the queue.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] buf     Pointer to UDC request buffer
 *
 * @return 0 on success, all other values should be treated as error.
 */
int usbd_ep_ctrl_enqueue(struct usbd_contex *const uds_ctx,
			 struct net_buf *const buf);

/**
 * @brief Queue USB device request
 *
 * Add request to the queue.
 *
 * @param[in] c_nd   Pointer to USB device class node
 * @param[in] buf    Pointer to UDC request buffer
 *
 * @return 0 on success, or error from udc_ep_enqueue()
 */
int usbd_ep_enqueue(const struct usbd_class_node *const c_nd,
		    struct net_buf *const buf);

/**
 * @brief Remove all USB device controller requests from endpoint queue
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] ep      Endpoint address
 *
 * @return 0 on success, or error from udc_ep_dequeue()
 */
int usbd_ep_dequeue(struct usbd_contex *uds_ctx, const uint8_t ep);

/**
 * @brief Free USB device request buffer
 *
 * Put the buffer back into the request buffer pool.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] buf     Pointer to UDC request buffer
 *
 * @return 0 on success, all other values should be treated as error.
 */
int usbd_ep_buf_free(struct usbd_contex *uds_ctx, struct net_buf *buf);

/**
 * @brief Checks whether the USB device controller is suspended.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 *
 * @return true if endpoint is halted, false otherwise
 */
bool usbd_is_suspended(struct usbd_contex *uds_ctx);

/**
 * @brief Initiate the USB remote wakeup (TBD)
 *
 * @return 0 on success, other values on fail.
 */
int usbd_wakeup_request(struct usbd_contex *uds_ctx);

/**
 * @brief Get actual device speed
 *
 * @param[in] uds_ctx Pointer to a device context
 *
 * @return Actual device speed
 */
enum usbd_speed usbd_bus_speed(const struct usbd_contex *const uds_ctx);

/**
 * @brief Get highest speed supported by the controller
 *
 * @param[in] uds_ctx Pointer to a device context
 *
 * @return Highest supported speed
 */
enum usbd_speed usbd_caps_speed(const struct usbd_contex *const uds_ctx);

/**
 * @brief Set USB device descriptor value bcdUSB
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] speed   Speed for which the bcdUSB should be set
 * @param[in] bcd     bcdUSB value
 *
 * @return 0 on success, other values on fail.
 */
int usbd_device_set_bcd(struct usbd_contex *const uds_ctx,
			const enum usbd_speed speed, const uint16_t bcd);

/**
 * @brief Set USB device descriptor value idVendor
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] vid     idVendor value
 *
 * @return 0 on success, other values on fail.
 */
int usbd_device_set_vid(struct usbd_contex *const uds_ctx,
			 const uint16_t vid);

/**
 * @brief Set USB device descriptor value idProduct
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] pid     idProduct value
 *
 * @return 0 on success, other values on fail.
 */
int usbd_device_set_pid(struct usbd_contex *const uds_ctx,
			const uint16_t pid);

/**
 * @brief Set USB device descriptor code triple Base Class, SubClass, and Protocol
 *
 * @param[in] uds_ctx    Pointer to USB device support context
 * @param[in] speed      Speed for which the code triple should be set
 * @param[in] base_class bDeviceClass value
 * @param[in] subclass   bDeviceSubClass value
 * @param[in] protocol   bDeviceProtocol value
 *
 * @return 0 on success, other values on fail.
 */
int usbd_device_set_code_triple(struct usbd_contex *const uds_ctx,
				const enum usbd_speed speed,
				const uint8_t base_class,
				const uint8_t subclass, const uint8_t protocol);

/**
 * @brief Setup USB device configuration attribute Remote Wakeup
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] speed   Configuration speed
 * @param[in] cfg     Configuration number
 * @param[in] enable  Sets attribute if true, clears it otherwise
 *
 * @return 0 on success, other values on fail.
 */
int usbd_config_attrib_rwup(struct usbd_contex *const uds_ctx,
			    const enum usbd_speed speed,
			    const uint8_t cfg, const bool enable);

/**
 * @brief Setup USB device configuration attribute Self-powered
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] speed   Configuration speed
 * @param[in] cfg     Configuration number
 * @param[in] enable  Sets attribute if true, clears it otherwise
 *
 * @return 0 on success, other values on fail.
 */
int usbd_config_attrib_self(struct usbd_contex *const uds_ctx,
			    const enum usbd_speed speed,
			    const uint8_t cfg, const bool enable);

/**
 * @brief Setup USB device configuration power consumption
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] speed   Configuration speed
 * @param[in] cfg     Configuration number
 * @param[in] power   Maximum power consumption value (bMaxPower)
 *
 * @return 0 on success, other values on fail.
 */
int usbd_config_maxpower(struct usbd_contex *const uds_ctx,
			 const enum usbd_speed speed,
			 const uint8_t cfg, const uint8_t power);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USBD_H_ */
