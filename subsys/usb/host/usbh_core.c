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

#include "usbh_internal.h"
#include "usbh_device.h"

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
 * @brief Check if USB device matches class driver
 * 
 * @param cdata Pointer to class driver data
 * @param code Pointer to USB class code triple
 * @return true if matched, false otherwise
 */
static bool usbh_match_class_driver(struct usbh_class_data *cdata, 
				    struct usbh_code_triple *code)
{
	if (!cdata || !code || !cdata->device_code_table) {
		return false;
	}

	/* Traverse device code table (cdata->device_code_table) */
	for (int i = 0; i < cdata->table_items_count; i++) {
		struct usbh_device_code_table *table_entry = &cdata->device_code_table[i];
		/* TODO: match device code */

		if (table_entry->match_type & USBH_MATCH_INTFACE) {
			/* Match interface class code */
			if (table_entry->interface_class_code == code->dclass &&
			    (table_entry->interface_subclass_code == 0xFF || 
			     table_entry->interface_subclass_code == code->sub) &&
			    (table_entry->interface_protocol_code == 0x00 || 
			     table_entry->interface_protocol_code == code->proto)) {
				return true;
			}
		}
	}
	
	return false;
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
static int usbh_match_classes(struct usbh_contex *const ctx,
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
		uint8_t *search_ptr;
		struct usbh_code_triple class_code = {0};
		bool found_iad = false;
		bool found_interface = false;
		struct usb_desc_header *iad_desc = NULL;
		
		/* Step 1: Find first IAD or interface descriptor from start_addr */
		search_ptr = start_addr;
		while (search_ptr < desc_buf_end) {
			struct usb_desc_header *header = (struct usb_desc_header *)search_ptr;
			
			if (header->bLength == 0) {
				goto exit_loop;
			}
			
			if (header->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
				start_addr = search_ptr;
				found_iad = true;
				iad_desc = search_ptr;
				break;
			} else if (header->bDescriptorType == USB_DESC_INTERFACE) {
				start_addr = search_ptr;
				found_interface = true;
				/* Save class code for interface */
				struct usb_if_descriptor *if_desc = (struct usb_if_descriptor *)search_ptr;
				class_code.dclass = if_desc->bInterfaceClass;
				class_code.sub = if_desc->bInterfaceSubClass;
				class_code.proto = if_desc->bInterfaceProtocol;
				break;
			}
			
			search_ptr += header->bLength;
		}
		
		/* If no IAD or interface found, exit main loop (error condition) */
		if (!found_iad && !found_interface) {
			LOG_ERR("No IAD or interface descriptor found - error condition");
			break;
		}
		
		/* Step 2: Continue searching for subsequent descriptors to determine end_addr */
		search_ptr = start_addr + ((struct usb_desc_header *)start_addr)->bLength;
		uint8_t *next_iad_addr = NULL;
		
		/* Find next IAD */
		while (search_ptr < desc_buf_end) {
			struct usb_desc_header *header = (struct usb_desc_header *)search_ptr;
			
			if (header->bLength == 0) {
				break;
			}
			
			if (header->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
				next_iad_addr = search_ptr;
				break;
			}
			
			search_ptr += header->bLength;
		}
		
		/* Handle different cases and determine end_addr and class_code */
		if (!found_iad && !next_iad_addr) {
			/* Case 2a: No IAD in step 1, no IAD in subsequent descriptors */
			end_addr = desc_buf_end;
			/* class_code already saved in step 1 */
		} else if (!found_iad && next_iad_addr) {
			/* Case 2b: No IAD in step 1, found new IAD in subsequent descriptors */
			end_addr = next_iad_addr;
			/* class_code already saved in step 1 */
		} else if (found_iad && next_iad_addr) {
			/* Case 2c: Found IAD in step 1, found new IAD in subsequent descriptors */
			end_addr = next_iad_addr;
			/* Get class code from first interface after IAD */
			search_ptr = start_addr + iad_desc->bLength;
			while (search_ptr < end_addr) {
				struct usb_desc_header *header = (struct usb_desc_header *)search_ptr;
				if (header->bLength == 0) {
					break;
				}
				if (header->bDescriptorType == USB_DESC_INTERFACE) {
					struct usb_if_descriptor *if_desc = (struct usb_if_descriptor *)search_ptr;
					class_code.dclass = if_desc->bInterfaceClass;
					class_code.sub = if_desc->bInterfaceSubClass;
					class_code.proto = if_desc->bInterfaceProtocol;
					break;
				}
				search_ptr += header->bLength;
			}
		} else if (found_iad && !next_iad_addr) {
			/* Case 2d: Found IAD in step 1, no new IAD in subsequent descriptors */
			/* Get class code from first interface after IAD */
			search_ptr = start_addr + iad_desc->bLength;
			while (search_ptr < desc_buf_end) {
				struct usb_desc_header *header = (struct usb_desc_header *)search_ptr;
				if (header->bLength == 0) {
					break;
				}
				if (header->bDescriptorType == USB_DESC_INTERFACE) {
					struct usb_if_descriptor *if_desc = (struct usb_if_descriptor *)search_ptr;
					class_code.dclass = if_desc->bInterfaceClass;
					class_code.sub = if_desc->bInterfaceSubClass;
					class_code.proto = if_desc->bInterfaceProtocol;
					break;
				}
				search_ptr += header->bLength;
			}

			/* Search for interface descriptor with different class code after IAD */
			search_ptr = start_addr + iad_desc->bLength;
			bool found_different_interface = false;

			while (search_ptr < desc_buf_end) {
				struct usb_desc_header *header = (struct usb_desc_header *)search_ptr;
				if (header->bLength == 0) {
					break;
				}
				
				if (header->bDescriptorType == USB_DESC_INTERFACE) {
					struct usb_if_descriptor *if_desc = (struct usb_if_descriptor *)search_ptr;
					/* Only compare class code */
					if (if_desc->bInterfaceClass != class_code.dclass) {
						end_addr = search_ptr;
						found_different_interface = true;
						break;
					}
				}
				search_ptr += header->bLength;
			}

			if (!found_different_interface) {
				end_addr = desc_buf_end;
			}
		}
		
		LOG_DBG("Found class segment: class=0x%02x, sub=0x%02x, proto=0x%02x, start=%p, end=%p",
			class_code.dclass, class_code.sub, class_code.proto,
			start_addr, end_addr);
		
		/* Step 3: Loop through registered class drivers and call usbh_match_class_driver */
		struct usbh_class_data *cdata;
		bool matched = false;
		
		SYS_SLIST_FOR_EACH_CONTAINER(&ctx->registered_classes, cdata, node) {
			if (!cdata->api || !cdata->api->connected) {
				continue;
			}
			
			/* Call usbh_match_class_driver with cdata and code */
			if (usbh_match_class_driver(cdata, &class_code)) {
				LOG_INF("Class driver %s matched for class 0x%02x", 
					cdata->name, class_code.dclass);
				
				/* Step 4: Call connected handler */
				int ret = cdata->api->connected(udev, start_addr, end_addr, (void *)cdata);
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
			LOG_DBG("No class driver matched for class 0x%02x", class_code.dclass);
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

exit_loop:
	LOG_INF("Class enumeration completed: %d driver(s) matched", matched_count);
	return 0;
}

static void dev_connected_handler(struct usbh_contex *const ctx,
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

static void dev_removed_handler(struct usbh_contex *const ctx)
{
	if (ctx->root != NULL) {
		usbh_device_free(ctx->root);
		ctx->root = NULL;
		LOG_DBG("Device removed");
	} else {
		LOG_DBG("Spurious device removed event");
	}
}

static int discard_ep_request(struct usbh_contex *const ctx,
			      struct uhc_transfer *const xfer)
{
	const struct device *dev = ctx->dev;

	if (xfer->buf) {
		LOG_HEXDUMP_INF(xfer->buf->data, xfer->buf->len, "buf");
		uhc_xfer_buf_free(dev, xfer->buf);
	}

	return uhc_xfer_free(dev, xfer);
}

static ALWAYS_INLINE int usbh_event_handler(struct usbh_contex *const ctx,
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

	struct usbh_contex *uhs_ctx;
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

	struct usbh_contex *uhs_ctx;
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
 * @brief Auto-register all compile-time defined class drivers
 */
static int usbh_register_all_classes(struct usbh_contex *uhs_ctx)
{
	int registered_count = 0;
	
	STRUCT_SECTION_FOREACH(usbh_class_data, cdata) {
		/* Check if already registered */
		struct usbh_class_data *existing;
		bool already_registered = false;
		
		SYS_SLIST_FOR_EACH_CONTAINER(&uhs_ctx->registered_classes, existing, node) {
			if (existing == cdata) {
				already_registered = true;
				break;
			}
		}
		
		if (!already_registered) {
			sys_slist_append(&uhs_ctx->registered_classes, &cdata->node);
			registered_count++;
			LOG_DBG("Auto-registered class: %s", cdata->name);
		}
	}
	
	LOG_INF("Auto-registered %d classes to controller %s", 
		registered_count, uhs_ctx->name);
	
	return 0;
}

/**
 * @brief Initialize USB host controller and class drivers
 */
int usbh_init_device_intl(struct usbh_contex *const uhs_ctx)
{
	int ret;

	ret = uhc_init(uhs_ctx->dev, usbh_event_carrier, uhs_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to init device driver");
		return ret;
	}

	sys_dlist_init(&uhs_ctx->udevs);
	sys_slist_init(&uhs_ctx->registered_classes);

	/* Register all class drivers */
	ret = usbh_register_all_classes(uhs_ctx);
	if (ret != 0) {
		LOG_WRN("Failed to auto-register classes");
	}

	/* Initialize registered class drivers */
	struct usbh_class_data *cdata;
	SYS_SLIST_FOR_EACH_CONTAINER(&uhs_ctx->registered_classes, cdata, node) {
		if (cdata->api && cdata->api->init) {
			ret = cdata->api->init(uhs_ctx, cdata);
			if (ret != 0) {
				LOG_WRN("Failed to init class driver %s", cdata->name);
			} else {
				LOG_INF("Class driver %s initialized", cdata->name);
			}
		}
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