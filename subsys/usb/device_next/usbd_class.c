/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbd.h>
#include <zephyr/toolchain.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/iterable_sections.h>

#include "usbd_device.h"
#include "usbd_class_api.h"
#include "usbd_config.h"
#include "usbd_endpoint.h"
#include "usbd_ch9.h"

#include <zephyr/logging/log.h>
#if defined(CONFIG_USBD_LOG_LEVEL)
#define USBD_CLASS_LOG_LEVEL CONFIG_USBD_LOG_LEVEL
#else
#define USBD_CLASS_LOG_LEVEL LOG_LEVEL_NONE
#endif
LOG_MODULE_REGISTER(usbd_class, CONFIG_USBD_LOG_LEVEL);

size_t usbd_class_desc_len(struct usbd_class_data *const c_data,
			   const enum usbd_speed speed)
{
	struct usb_desc_header **dhp;
	size_t len = 0;

	dhp = usbd_class_get_desc(c_data, speed);
	/*
	 * If the desired descriptor is available, count to the last element,
	 * which must be a pointer to a nil descriptor.
	 */
	while (dhp != NULL && (*dhp != NULL) && (*dhp)->bLength != 0U) {
		len += (*dhp)->bLength;
		dhp++;
	}

	return len;
}

struct usbd_class_node *
usbd_class_get_by_config(struct usbd_context *const uds_ctx,
			 const enum usbd_speed speed,
			 const uint8_t cnum,
			 const uint8_t inum)
{
	struct usbd_class_node *c_nd;
	struct usbd_config_node *cfg_nd;

	cfg_nd = usbd_config_get(uds_ctx, speed, cnum);
	if (cfg_nd == NULL) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		if (c_nd->iface_bm & BIT(inum)) {
			return c_nd;
		}
	}

	return NULL;
}

struct usbd_class_node *
usbd_class_get_by_iface(struct usbd_context *const uds_ctx,
			const uint8_t inum)
{
	struct usbd_class_node *c_nd;
	struct usbd_config_node *cfg_nd;

	cfg_nd = usbd_config_get_current(uds_ctx);
	if (cfg_nd == NULL) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		if (c_nd->iface_bm & BIT(inum)) {
			return c_nd;
		}
	}

	return NULL;
}

static bool xfer_owner_exist(struct usbd_context *const uds_ctx,
			     struct usbd_config_node *const cfg_nd,
			     struct net_buf *const buf)
{
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	struct usbd_class_node *c_nd;

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		if (bi->owner == c_nd->c_data) {
			uint32_t ep_active = c_nd->ep_active;
			uint32_t ep_assigned = c_nd->ep_assigned;

			if (!usbd_ep_bm_is_set(&ep_active, bi->ep)) {
				LOG_DBG("ep 0x%02x is not active", bi->ep);
			}

			if (!usbd_ep_bm_is_set(&ep_assigned, bi->ep)) {
				LOG_DBG("ep 0x%02x is not assigned", bi->ep);
			}

			return true;
		}
	}

	return false;
}

int usbd_class_handle_xfer(struct usbd_context *const uds_ctx,
			   struct net_buf *const buf,
			   const int err)
{
	struct udc_buf_info *bi = udc_get_buf_info(buf);

	if (unlikely(USBD_CLASS_LOG_LEVEL == LOG_LEVEL_DBG)) {
		struct usbd_config_node *cfg_nd;

		if (usbd_state_is_configured(uds_ctx)) {
			cfg_nd = usbd_config_get_current(uds_ctx);
			if (!xfer_owner_exist(uds_ctx, cfg_nd, buf)) {
				LOG_DBG("Class request without owner");
			}
		} else {
			LOG_DBG("Class request on not configured device");
		}
	}

	return usbd_class_request(bi->owner, buf, err);
}

struct usbd_class_node *
usbd_class_get_by_ep(struct usbd_context *const uds_ctx,
		     const uint8_t ep)
{
	struct usbd_class_node *c_nd;
	struct usbd_config_node *cfg_nd;
	enum usbd_speed speed;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t cfg;
	uint32_t ep_bm;

	if (USB_EP_DIR_IS_IN(ep)) {
		ep_bm = BIT(ep_idx + 16);
	} else {
		ep_bm = BIT(ep_idx);
	}

	if (!usbd_state_is_configured(uds_ctx)) {
		LOG_ERR("No configuration set (Address state)");
		return NULL;
	}

	cfg = usbd_get_config_value(uds_ctx);
	speed = usbd_bus_speed(uds_ctx);
	cfg_nd = usbd_config_get(uds_ctx, speed, cfg);
	if (cfg_nd == NULL) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		if (c_nd->ep_assigned & ep_bm) {
			return c_nd;
		}
	}

	return NULL;
}

struct usbd_class_node *
usbd_class_get_by_req(struct usbd_context *const uds_ctx,
		      const uint8_t request)
{
	struct usbd_config_node *cfg_nd;
	struct usbd_class_node *c_nd;

	cfg_nd = usbd_config_get_current(uds_ctx);
	if (cfg_nd == NULL) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		if (c_nd->c_data->v_reqs == NULL) {
			continue;
		}

		for (int i = 0; i < c_nd->c_data->v_reqs->len; i++) {
			/*
			 * First instance always wins.
			 * There is no other way to determine the recipient.
			 */
			if (c_nd->c_data->v_reqs->reqs[i] == request) {
				return c_nd;
			}
		}
	}

	return NULL;
}

static struct usbd_class_node *
usbd_class_node_get(const char *name, const enum usbd_speed speed)
{
	if (speed == USBD_SPEED_FS) {
		STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_fs,
						 usbd_class_node, c_nd) {
			if (strcmp(name, c_nd->c_data->name) == 0) {
				return c_nd;
			}
		}
	} else if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_hs,
						 usbd_class_node, c_nd) {
			if (strcmp(name, c_nd->c_data->name) == 0) {
				return c_nd;
			}
		}
	}

	LOG_ERR("USB device class %s not found", name);

	return NULL;
}

static int usbd_class_append(struct usbd_context *const uds_ctx,
			     struct usbd_class_node *const c_nd,
			     const enum usbd_speed speed,
			     const uint8_t cfg)
{
	struct usbd_config_node *cfg_nd;

	cfg_nd = usbd_config_get(uds_ctx, speed, cfg);
	if (cfg_nd == NULL) {
		return -ENODATA;
	}

	sys_slist_append(&cfg_nd->class_list, &c_nd->node);

	return 0;
}

static int usbd_class_remove(struct usbd_context *const uds_ctx,
			     struct usbd_class_node *const c_nd,
			     const enum usbd_speed speed,
			     const uint8_t cfg)
{
	struct usbd_config_node *cfg_nd;

	cfg_nd = usbd_config_get(uds_ctx, speed, cfg);
	if (cfg_nd == NULL) {
		return -ENODATA;
	}

	if (!sys_slist_find_and_remove(&cfg_nd->class_list, &c_nd->node)) {
		return -ENODATA;
	}

	return 0;
}

int usbd_class_remove_all(struct usbd_context *const uds_ctx,
			  const enum usbd_speed speed,
			  const uint8_t cfg)
{
	struct usbd_config_node *cfg_nd;
	struct usbd_class_node *c_nd;
	sys_snode_t *node;

	cfg_nd = usbd_config_get(uds_ctx, speed, cfg);
	if (cfg_nd == NULL) {
		return -ENODATA;
	}

	while ((node = sys_slist_get(&cfg_nd->class_list))) {
		c_nd = CONTAINER_OF(node, struct usbd_class_node, node);
		atomic_clear_bit(&c_nd->state, USBD_CCTX_REGISTERED);
		usbd_class_shutdown(c_nd->c_data);
		LOG_DBG("Remove class node %p from configuration %u", c_nd, cfg);
	}

	return 0;
}

/*
 * All the functions below are part of public USB device support API.
 */

int usbd_register_class(struct usbd_context *const uds_ctx,
			const char *name,
			const enum usbd_speed speed, const uint8_t cfg)
{
	struct usbd_class_node *c_nd;
	struct usbd_class_data *c_data;
	int ret;

	c_nd = usbd_class_node_get(name, speed);
	if (c_nd == NULL) {
		return -ENODEV;
	}

	usbd_device_lock(uds_ctx);

	if (usbd_is_initialized(uds_ctx)) {
		LOG_ERR("USB device support is initialized");
		ret = -EBUSY;
		goto register_class_error;
	}

	c_data = c_nd->c_data;

	/* TODO: does it still need to be atomic ? */
	if (atomic_test_bit(&c_nd->state, USBD_CCTX_REGISTERED)) {
		LOG_WRN("Class instance already registered");
		ret = -EBUSY;
		goto register_class_error;
	}

	if ((c_data->uds_ctx != NULL) && (c_data->uds_ctx != uds_ctx)) {
		LOG_ERR("Class registered to other context at different speed");
		ret = -EBUSY;
		goto register_class_error;
	}

	ret = usbd_class_append(uds_ctx, c_nd, speed, cfg);
	if (ret == 0) {
		/* Initialize pointer back to the device struct */
		atomic_set_bit(&c_nd->state, USBD_CCTX_REGISTERED);
		c_data->uds_ctx = uds_ctx;
	}

register_class_error:
	usbd_device_unlock(uds_ctx);
	return ret;
}

static bool is_blocklisted(const struct usbd_class_node *const c_nd,
			   const char *const list[])
{
	for (int i = 0; list[i] != NULL; i++) {
		if (strcmp(c_nd->c_data->name, list[i]) == 0) {
			return true;
		}
	}

	return false;
}

int usbd_register_all_classes(struct usbd_context *const uds_ctx,
			      const enum usbd_speed speed, const uint8_t cfg,
			      const char *const blocklist[])
{
	int ret;

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_hs, usbd_class_node, c_nd) {
			if (blocklist != NULL && is_blocklisted(c_nd, blocklist)) {
				continue;
			}

			ret = usbd_register_class(uds_ctx, c_nd->c_data->name,
						  speed, cfg);
			if (ret) {
				LOG_ERR("Failed to register %s to HS configuration %u",
					c_nd->c_data->name, cfg);
				return ret;
			}
		}

		return 0;
	}

	if (speed == USBD_SPEED_FS) {
		STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_fs, usbd_class_node, c_nd) {
			if (blocklist != NULL && is_blocklisted(c_nd, blocklist)) {
				continue;
			}

			ret = usbd_register_class(uds_ctx, c_nd->c_data->name,
						  speed, cfg);
			if (ret) {
				LOG_ERR("Failed to register %s to FS configuration %u",
					c_nd->c_data->name, cfg);
				return ret;
			}
		}

		return 0;
	}

	return -ENOTSUP;
}

int usbd_unregister_class(struct usbd_context *const uds_ctx,
			  const char *name,
			  const enum usbd_speed speed, const uint8_t cfg)
{
	struct usbd_class_node *c_nd;
	struct usbd_class_data *c_data;
	bool can_release_data = true;
	int ret;

	c_nd = usbd_class_node_get(name, speed);
	if (c_nd == NULL) {
		return -ENODEV;
	}

	usbd_device_lock(uds_ctx);

	if (usbd_is_initialized(uds_ctx)) {
		LOG_ERR("USB device support is initialized");
		ret = -EBUSY;
		goto unregister_class_error;
	}

	c_data = c_nd->c_data;
	/* TODO: does it still need to be atomic ? */
	if (!atomic_test_bit(&c_nd->state, USBD_CCTX_REGISTERED)) {
		LOG_WRN("Class instance not registered");
		ret = -EBUSY;
		goto unregister_class_error;
	}

	/* TODO: The use of atomic here does not make this code thread safe.
	 * The atomic should be changed to something else.
	 */
	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_hs,
						 usbd_class_node, i) {
			if ((i->c_data == c_nd->c_data) &&
			    atomic_test_bit(&i->state, USBD_CCTX_REGISTERED)) {
				can_release_data = false;
				break;
			}
		}
	} else {
		STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_fs,
						 usbd_class_node, i) {
			if ((i->c_data == c_nd->c_data) &&
			    atomic_test_bit(&i->state, USBD_CCTX_REGISTERED)) {
				can_release_data = false;
				break;
			}
		}
	}

	ret = usbd_class_remove(uds_ctx, c_nd, speed, cfg);
	if (ret == 0) {
		atomic_clear_bit(&c_nd->state, USBD_CCTX_REGISTERED);
		usbd_class_shutdown(c_nd->c_data);

		if (can_release_data) {
			c_data->uds_ctx = NULL;
		}
	}

unregister_class_error:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_unregister_all_classes(struct usbd_context *const uds_ctx,
				const enum usbd_speed speed, const uint8_t cfg)
{
	int ret;

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_hs, usbd_class_node, c_nd) {
			ret = usbd_unregister_class(uds_ctx, c_nd->c_data->name,
						    speed, cfg);
			if (ret) {
				LOG_ERR("Failed to unregister %s to HS configuration %u",
					c_nd->c_data->name, cfg);
				return ret;
			}
		}

		return 0;
	}

	if (speed == USBD_SPEED_FS) {
		STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_fs, usbd_class_node, c_nd) {
			ret = usbd_unregister_class(uds_ctx, c_nd->c_data->name,
						    speed, cfg);
			if (ret) {
				LOG_ERR("Failed to unregister %s to FS configuration %u",
					c_nd->c_data->name, cfg);
				return ret;
			}
		}

		return 0;
	}

	return -ENOTSUP;
}
