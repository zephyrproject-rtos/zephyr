/*
 * Copyright (c) 2022,2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/usb/usbh.h>

#include "usbh_class.h"
#include "usbh_class_api.h"
#include "usbh_desc.h"
#include "usbh_device.h"
#include "usbh_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhs, CONFIG_USBH_LOG_LEVEL);

static K_KERNEL_STACK_DEFINE(usbh_stack, CONFIG_USBH_STACK_SIZE);
static struct k_thread usbh_thread_data;

static K_KERNEL_STACK_DEFINE(usbh_bus_stack, CONFIG_USBH_STACK_SIZE);
static struct k_thread usbh_bus_thread_data;

K_MSGQ_DEFINE(usbh_msgq, sizeof(struct uhc_event),
	      CONFIG_USBH_MAX_UHC_MSG, sizeof(uint32_t));

K_MSGQ_DEFINE(usbh_bus_msgq, sizeof(struct uhc_event),
	      CONFIG_USBH_MAX_UHC_MSG, sizeof(uint32_t));

static int usbh_event_carrier(const struct device *dev,
			      const struct uhc_event *const event)
{
	int err;

	if (event->type == UHC_EVT_EP_REQUEST) {
		err = k_msgq_put(&usbh_msgq, event, K_NO_WAIT);
	} else {
		err = k_msgq_put(&usbh_bus_msgq, event, K_NO_WAIT);
	}

	return err;
}

/**
 * @brief Enumerate device descriptors and match class drivers
 *
 * This function traverses the USB device descriptors, identifies class-specific
 * descriptor segments, and attempts to match them with registered class drivers.
 *
 * @param ctx USB host context
 * @param udev USB device to process
 * @return 0 on success, negative errno on failure
 */
static int usbh_match_classes(struct usbh_context *const ctx,
			      struct usb_device *udev)
{
	struct usb_cfg_descriptor *cfg_desc = udev->cfg_desc;

	if (!cfg_desc) {
		LOG_ERR("No configuration descriptor found");
		return -EINVAL;
	}

	uint8_t *desc_buf_base = (uint8_t *)cfg_desc;
	uint8_t *desc_buf_end = desc_buf_base + sys_le16_to_cpu(cfg_desc->wTotalLength);
	uint8_t *current_desc = desc_buf_base + cfg_desc->bLength;  /* Skip config descriptor */
	int matched_count = 0;

	LOG_DBG("Starting class enumeration for device (total desc length: %d)",
		sys_le16_to_cpu(cfg_desc->wTotalLength));

	/* Main descriptor traversal loop - traverse entire descriptor */
	while (current_desc < desc_buf_end) {
		uint8_t *start_addr = current_desc;   /* Initialize to current descriptor */
		uint8_t *end_addr = current_desc;     /* Initialize to current descriptor */
		uint8_t *next_iad_addr = NULL;
		struct usbh_class_filter device_info = {0};
		bool found_iad = false;
		bool found_interface = false;
		struct usb_desc_header *iad_desc = NULL;
		struct usb_desc_header *desc;
		uint32_t mask;

		/* Step 1: Find first IAD or interface descriptor from start_addr */
		mask = BIT(USB_DESC_INTERFACE) | BIT(USB_DESC_INTERFACE_ASSOC);

		desc = usbh_desc_get_by_type(start_addr, desc_buf_end, mask);
		if (desc == NULL) {
			LOG_ERR("No IAD or interface descriptor found - error condition");
			break;
		}
		start_addr = (uint8_t *)desc;

		if (desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
			found_iad = true;
			iad_desc = desc;
		}

		if (desc->bDescriptorType == USB_DESC_INTERFACE) {
			struct usb_if_descriptor *if_desc = (void *)desc;

			device_info.code_triple.dclass = if_desc->bInterfaceClass;
			device_info.code_triple.sub = if_desc->bInterfaceSubClass;
			device_info.code_triple.proto = if_desc->bInterfaceProtocol;
			found_interface = true;
		}

		/* Step 2: Continue searching for subsequent descriptors to determine end_addr */
		uint8_t *search_start = start_addr + ((struct usb_desc_header *)start_addr)->bLength;

		/* Find next IAD */
		mask = BIT(USB_DESC_INTERFACE_ASSOC);
		desc = usbh_desc_get_by_type(search_start, desc_buf_end, mask);
		next_iad_addr = (uint8_t *)desc;

		/* Handle different cases and determine end_addr and device_info. */
		if (!found_iad && next_iad_addr == NULL) {
			/* Case 2a: No IAD in step 1, no IAD in subsequent descriptors */
			end_addr = desc_buf_end;
			/* device_info already saved in step 1 */
		} else if (!found_iad && next_iad_addr) {
			/* Case 2b: No IAD in step 1, found new IAD in subsequent descriptors */
			end_addr = next_iad_addr;
			/* device_info already saved in step 1 */
		} else if (found_iad && next_iad_addr) {
			/* Case 2c: Found IAD in step 1, found new IAD in subsequent descriptors */
			end_addr = next_iad_addr;

			/* Get class code from first interface after IAD */
			mask = BIT(USB_DESC_INTERFACE);
			desc = usbh_desc_get_by_type(search_start, end_addr, mask);
			if (desc != NULL) {
				struct usb_if_descriptor *if_desc = (void *)desc;

				device_info.code_triple.dclass = if_desc->bInterfaceClass;
				device_info.code_triple.sub = if_desc->bInterfaceSubClass;
				device_info.code_triple.proto = if_desc->bInterfaceProtocol;
			}
		} else if (found_iad && !next_iad_addr) {
			/* Case 2d: Found IAD in step 1, no new IAD in subsequent descriptors */
			/* Get class code from first interface after IAD */
			mask = BIT(USB_DESC_INTERFACE);
			desc = usbh_desc_get_by_type(search_start, desc_buf_end, mask);
			if (desc != NULL) {
				struct usb_if_descriptor *if_desc = (void *)desc;

				device_info.code_triple.dclass = if_desc->bInterfaceClass;
				device_info.code_triple.sub = if_desc->bInterfaceSubClass;
				device_info.code_triple.proto = if_desc->bInterfaceProtocol;

				/* Search for interface descriptor with different class code after IAD */
				uint8_t *next_search = (uint8_t *)desc + desc->bLength;
				mask = BIT(USB_DESC_INTERFACE);
				desc = usbh_desc_get_by_type(next_search, desc_buf_end, mask);
				if (desc != NULL) {
					struct usb_if_descriptor *if_desc = (void *)desc;

					/* Only compare class code */
					if (if_desc->bInterfaceClass != device_info.code_triple.dclass) {
						end_addr = (uint8_t *)desc;
					} else {
						end_addr = desc_buf_end;
					}
				} else {
					end_addr = desc_buf_end;
				}
			} else {
				end_addr = desc_buf_end;
			}
		}

		LOG_DBG("Found class segment: class=0x%02x, sub=0x%02x, proto=0x%02x, start=%p, end=%p",
			device_info.code_triple.dclass, device_info.code_triple.sub,
			device_info.code_triple.proto, start_addr, end_addr);

		/* Step 3: Loop through registered class drivers and call usbh_match_class_driver */
		struct usbh_class_data *cdata;
		bool matched = false;

		SYS_SLIST_FOR_EACH_CONTAINER(&ctx->class_list, cdata, node) {
			/* Call usbh_match_class_driver with cdata and code */
			if (usbh_class_is_matching(cdata, &device_info)) {
				LOG_INF("Class driver %s matched for class 0x%02x",
					cdata->name, device_info.code_triple.dclass);

				/* Step 4: Call connected handler */
				int ret = usbh_class_connected(udev, cdata, start_addr, end_addr);
				if (ret == 0) {
					LOG_INF("Class driver %s successfully claimed device", cdata->name);
					matched = true;
					matched_count++;
				} else {
					LOG_WRN("Class driver %s failed to claim device: %d",
						cdata->name, ret);
				}
			}
		}

		if (!matched) {
			LOG_DBG("No class driver matched for class 0x%02x",
				device_info.code_triple.dclass);
		}

		/* Step 4: assign end_addr to current_desc and continue main loop */
		current_desc = end_addr;

		/* Ensure we advance to next valid descriptor */
		if (current_desc < desc_buf_end) {
			struct usb_desc_header *header = (struct usb_desc_header *)current_desc;
			if (header->bLength == 0) {
				break;
			}
		}
	}

	LOG_INF("Class enumeration completed: %d driver(s) matched", matched_count);
	return 0;
}

static void dev_connected_handler(struct usbh_context *const ctx,
				  const struct uhc_event *const event)
{

	LOG_DBG("Device connected event");
	if (ctx->root != NULL) {
		LOG_ERR("Device already connected");
		usbh_device_free(ctx->root);
		ctx->root = NULL;
	}

	ctx->root = usbh_device_alloc(ctx);
	if (ctx->root == NULL) {
		LOG_ERR("Failed allocate new device");
		return;
	}

	ctx->root->state = USB_STATE_DEFAULT;

	if (event->type == UHC_EVT_DEV_CONNECTED_HS) {
		ctx->root->speed = USB_SPEED_SPEED_HS;
	} else {
		ctx->root->speed = USB_SPEED_SPEED_FS;
	}

	if (usbh_device_init(ctx->root)) {
		LOG_ERR("Failed to reset new USB device");
	}

	/* Now only consider about one device connected (root device) */
	if (usbh_match_classes(ctx, ctx->root)) {
		LOG_ERR("Failed to match classes");
	}
}

static void dev_removed_handler(struct usbh_context *const ctx)
{
	if (ctx->root != NULL) {
		LOG_DBG("Device removed - notifying class drivers");

		/* Notify all relevant class drivers that device is disconnected */
		struct usbh_class_data *cdata;
		SYS_SLIST_FOR_EACH_CONTAINER(&ctx->class_list, cdata, node) {
			LOG_DBG("Calling disconnected for class driver: %s", cdata->name);
			int ret = usbh_class_removed(ctx->root, cdata);
			if (ret != 0) {
				LOG_WRN("Class driver %s disconnected callback failed: %d", 
					cdata->name, ret);
			} else {
				LOG_INF("Class driver %s successfully disconnected", cdata->name);
			}
		}

		/* Free device resources */
		usbh_device_free(ctx->root);
		ctx->root = NULL;
		LOG_DBG("Device removed");
	} else {
		LOG_DBG("Spurious device removed event");
	}
}

static int discard_ep_request(struct usbh_context *const ctx,
			      struct uhc_transfer *const xfer)
{
	const struct device *dev = ctx->dev;

	if (xfer->buf) {
		LOG_HEXDUMP_INF(xfer->buf->data, xfer->buf->len, "buf");
		uhc_xfer_buf_free(dev, xfer->buf);
	}

	return uhc_xfer_free(dev, xfer);
}

static ALWAYS_INLINE int usbh_event_handler(struct usbh_context *const ctx,
					    struct uhc_event *const event)
{
	int ret = 0;

	switch (event->type) {
	case UHC_EVT_DEV_CONNECTED_LS:
		LOG_ERR("Low speed device not supported (connected event)");
		break;
	case UHC_EVT_DEV_CONNECTED_FS:
	case UHC_EVT_DEV_CONNECTED_HS:
		dev_connected_handler(ctx, event);
		break;
	case UHC_EVT_DEV_REMOVED:
		dev_removed_handler(ctx);
		break;
	case UHC_EVT_RESETED:
		LOG_DBG("Bus reset");
		break;
	case UHC_EVT_SUSPENDED:
		LOG_DBG("Bus suspended");
		break;
	case UHC_EVT_RESUMED:
		LOG_DBG("Bus resumed");
		break;
	case UHC_EVT_RWUP:
		LOG_DBG("RWUP event");
		break;
	case UHC_EVT_ERROR:
		LOG_DBG("Error event %d", event->status);
		break;
	default:
		break;
	};

	return ret;
}

static void usbh_bus_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct usbh_context *uhs_ctx;
	struct uhc_event event;

	while (true) {
		k_msgq_get(&usbh_bus_msgq, &event, K_FOREVER);

		uhs_ctx = (void *)uhc_get_event_ctx(event.dev);
		usbh_event_handler(uhs_ctx, &event);
	}
}

static void usbh_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct usbh_context *uhs_ctx;
	struct uhc_event event;
	usbh_udev_cb_t cb;
	int ret;

	while (true) {
		k_msgq_get(&usbh_msgq, &event, K_FOREVER);

		__ASSERT(event.type == UHC_EVT_EP_REQUEST, "Wrong event type");
		uhs_ctx = (void *)uhc_get_event_ctx(event.dev);
		cb = event.xfer->cb;

		if (event.xfer->cb) {
			ret = cb(event.xfer->udev, event.xfer);
		} else {
			ret = discard_ep_request(uhs_ctx, event.xfer);
		}

		if (ret) {
			LOG_ERR("Failed to handle request completion callback");
		}
	}
}

/**
 * @brief Initialize USB host controller and class drivers
 */
int usbh_init_device_intl(struct usbh_context *const uhs_ctx)
{
	int ret;

	ret = uhc_init(uhs_ctx->dev, usbh_event_carrier, uhs_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to init device driver");
		return ret;
	}

	sys_dlist_init(&uhs_ctx->udevs);
	sys_slist_init(&uhs_ctx->class_list);

	ret = usbh_register_all_classes(uhs_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to auto-register class instances");
		return ret;
	}

	ret = usbh_init_registered_classes(uhs_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to initialize all registered class instances");
		return ret;
	}

	return 0;
}

static int uhs_pre_init(void)
{
	k_thread_create(&usbh_thread_data, usbh_stack,
			K_KERNEL_STACK_SIZEOF(usbh_stack),
			usbh_thread,
			NULL, NULL, NULL,
			K_PRIO_COOP(9), 0, K_NO_WAIT);

	k_thread_name_set(&usbh_thread_data, "usbh");

	k_thread_create(&usbh_bus_thread_data, usbh_bus_stack,
			K_KERNEL_STACK_SIZEOF(usbh_bus_stack),
			usbh_bus_thread,
			NULL, NULL, NULL,
			K_PRIO_COOP(9), 0, K_NO_WAIT);

	k_thread_name_set(&usbh_thread_data, "usbh_bus");

	return 0;
}

SYS_INIT(uhs_pre_init, POST_KERNEL, CONFIG_USBH_INIT_PRIO);
