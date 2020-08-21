/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
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

#include <drivers/usb/usb_dc.h>
#include <usb/usb_ch9.h>
#include <net/buf.h>
#include <logging/log.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB Device Core Layer API
 * @defgroup _usb_device_core_api USB Device Core API
 * @{
 */

enum usbd_pme_code {
	USBD_PME_DETACHED,
	USBD_PME_SUSPEND,
	USBD_PME_RESUME,
};

struct usbd_class_ctx;

typedef void (*usbd_ep_event_cb)(struct usbd_class_ctx *cctx,
				 struct net_buf *buf, int err);

/**
 * @brief Request handler
 *
 * Request handler
 *
 * @param[in] cctx Class context struct of the class instance
 * @param[in] setup Pointer to USB Setup Packet
 * @param[in] buf Control Request Data buffer
 *
 * @return 0 on success, other values on fail.
 */
typedef int (*usbd_req_handler)(struct usbd_class_ctx *cctx,
				struct usb_setup_packet *setup,
				struct net_buf *buf);

/**
 * @brief Configuration update handler
 *
 * Configuration update handler
 *
 * @param[in] cctx Class context struct of the class instance
 * @param[in] setup Pointer to USB Setup Packet
 *
 * @return 0 on success, other values on fail.
 */
typedef void (*usbd_cfg_update)(struct usbd_class_ctx *cctx,
				struct usb_setup_packet *setup);

/**
 * @brief USB power management handler
 *
 * USB power management handler
 *
 * @param[in] cctx Class context struct of the class instance
 * @param[in] event Power management event
 *
 * @return 0 on success, other values on fail.
 */
typedef void (*usbd_pm_event)(struct usbd_class_ctx *cctx,
			      enum usbd_pme_code event);

/**
 * @brief Initialization of the class implementation
 *
 * Initialization of the class implementation
 *
 * @param[in] cctx Class context struct of the class instance
 * @param[in] setup Pointer to USB Setup Packet
 * @param[in] buf Control Request Data buffer
 *
 * @return 0 on success, other values on fail.
 */
typedef int (*usbd_cctx_init)(struct usbd_class_ctx *cctx);

/**
 * @brief Vendor Requests Table
 */
struct usbd_cctx_vendor_req {
	/** */
	const uint8_t *reqs;
	/** */
	uint8_t len;
};

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


#define USBD_CCTX_REGISTERED		0

/**
 * @brief Common USB Class instance operations
 */
struct usbd_class_ops {
	/** */
	usbd_req_handler req_handler;
	/** */
	usbd_cfg_update cfg_update;
	/** */
	usbd_ep_event_cb ep_cb;
	/** */
	usbd_pm_event pm_event;
	/** */
	usbd_cctx_init init;
	/** */
	usbd_cctx_init terminate;
};

/**
 * @brief USB Class Context per instance
 */
struct usbd_class_ctx {
	/** Class Contex operations */
	struct usbd_class_ops ops;
	/** Terminated interfaces descriptor for Class implementation */
	struct usb_desc_header *class_desc;
	/** Terminated string descriptor for Class implementation */
	struct usb_desc_header *string_desc;
	/** Supported vendor request table, can be NULL */
	const struct usbd_cctx_vendor_req *v_reqs;
	/** Node information for the slist. */
	sys_snode_t node;
	/** Variable to store the state of the class instance */
	atomic_t state;
	/** Bitmap of the endpoints reserved for the instance.
	 *  The IN endpoints are mapped in the upper nibble.
	 *  Can be used for fast back access to the specific instance.
	 */
	uint32_t ep_bm;
};

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
 * Registered instances are initialized at startup
 * of the USB device stack, and the interface descriptors
 * of each instance are adapted to the whole context.
 *
 * @param[in] dev    Device struct of the class instance
 *
 * @return 0 on success, other values on fail.
 */
int usbd_cctx_register(const struct device *dev);

/**
 * @brief Unregister an USB class instance
 *
 * USB class instance will be removed and will not appear
 * on the next start of the stack. Instance can only be unregistered
 * when the USB device stack is disabled.
 *
 * @param[in] dev    Device struct of the class instance
 *
 * @return 0 on success, other values on fail.
 */
int usbd_cctx_unregister(const struct device *dev);

/**
 * @brief Enable the USB device support and registered class instances
 *
 * This function initializes the USB device support core and
 * all registered class instances.
 * Class instances should be registered before they are involved.
 * However, the stack should also start without registered instances,
 * even if the host would complain about missing interfaces.
 *
 * @param[in] status_cb Callback registered by user to notify
 *                      about USB device controller state.
 *
 * @return 0 on success, other values on fail.
 */
int usbd_enable(usb_dc_status_callback status_cb);

/**
 * @brief Disable the USB device support and registered class instances
 *
 * This function disables the USB device support core and
 * terminate all registered class instances.
 *
 * @return 0 on success, other values on fail.
 */
int usbd_disable(void);

/**
 * @brief Stall specified endpoint
 *
 * Internal wrapper to stall specified endpoint.
 *
 * @param[in] ep Endpoint address
 *
 * @return 0 on success, other values on fail.
 */
int usbd_ep_set_stall(uint8_t ep);

/**
 * @brief Clear stall for specified endpoint
 *
 * Internal wrapper to clear stall for specified endpoint.
 *
 * @param[in] ep Endpoint address
 *
 * @return 0 on success, other values on fail.
 */
int usbd_ep_clear_stall(uint8_t ep);

/**
 * @brief Initiate the USB remote wakeup
 *
 * @return 0 on success, other values on fail.
 */
int usbd_wakeup_request(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USBD_H_ */
