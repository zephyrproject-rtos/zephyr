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

size_t usbd_class_desc_len(struct usbd_class_node *const c_nd,
			   const enum usbd_speed speed)
{
	struct usb_desc_header **dhp;
	size_t len = 0;

	dhp = usbd_class_get_desc(c_nd, speed);
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
usbd_class_get_by_config(struct usbd_contex *const uds_ctx,
			 const uint8_t cnum,
			 const uint8_t inum)
{
	struct usbd_class_node *c_nd;
	struct usbd_config_node *cfg_nd;

	cfg_nd = usbd_config_get(uds_ctx, cnum);
	if (cfg_nd == NULL) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		if (c_nd->data->iface_bm & BIT(inum)) {
			return c_nd;
		}
	}

	return NULL;
}

struct usbd_class_node *
usbd_class_get_by_iface(struct usbd_contex *const uds_ctx,
			const uint8_t inum)
{
	struct usbd_class_node *c_nd;
	struct usbd_config_node *cfg_nd;

	cfg_nd = usbd_config_get_current(uds_ctx);
	if (cfg_nd == NULL) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		if (c_nd->data->iface_bm & BIT(inum)) {
			return c_nd;
		}
	}

	return NULL;
}

static bool xfer_owner_exist(struct usbd_contex *const uds_ctx,
			     struct usbd_config_node *const cfg_nd,
			     struct net_buf *const buf)
{
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	struct usbd_class_node *c_nd;

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		if (bi->owner == c_nd) {
			uint32_t ep_active = c_nd->data->ep_active;
			uint32_t ep_assigned = c_nd->data->ep_assigned;

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

int usbd_class_handle_xfer(struct usbd_contex *const uds_ctx,
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
usbd_class_get_by_ep(struct usbd_contex *const uds_ctx,
		     const uint8_t ep)
{
	struct usbd_class_node *c_nd;
	struct usbd_config_node *cfg_nd;
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
	cfg_nd = usbd_config_get(uds_ctx, cfg);
	if (cfg_nd == NULL) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		if (c_nd->data->ep_assigned & ep_bm) {
			return c_nd;
		}
	}

	return NULL;
}

struct usbd_class_node *
usbd_class_get_by_req(struct usbd_contex *const uds_ctx,
		      const uint8_t request)
{
	struct usbd_config_node *cfg_nd;
	struct usbd_class_node *c_nd;

	cfg_nd = usbd_config_get_current(uds_ctx);
	if (cfg_nd == NULL) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		if (c_nd->data->v_reqs == NULL) {
			continue;
		}

		for (int i = 0; i < c_nd->data->v_reqs->len; i++) {
			/*
			 * First instance always wins.
			 * There is no other way to determine the recipient.
			 */
			if (c_nd->data->v_reqs->reqs[i] == request) {
				return c_nd;
			}
		}
	}

	return NULL;
}

static struct usbd_class_node *usbd_class_node_get(const char *name)
{
	STRUCT_SECTION_FOREACH(usbd_class_node, c_nd) {
		if (strcmp(name, c_nd->name) == 0) {
			return c_nd;
		}
	}

	LOG_ERR("USB device class %s not found", name);

	return NULL;
}

static int usbd_class_append(struct usbd_contex *const uds_ctx,
			     struct usbd_class_node *const c_nd,
			     const uint8_t cfg)
{
	struct usbd_config_node *cfg_nd;

	cfg_nd = usbd_config_get(uds_ctx, cfg);
	if (cfg_nd == NULL) {
		return -ENODATA;
	}

	sys_slist_append(&cfg_nd->class_list, &c_nd->node);

	return 0;
}

static int usbd_class_remove(struct usbd_contex *const uds_ctx,
			     struct usbd_class_node *const c_nd,
			     const uint8_t cfg)
{
	struct usbd_config_node *cfg_nd;

	cfg_nd = usbd_config_get(uds_ctx, cfg);
	if (cfg_nd == NULL) {
		return -ENODATA;
	}

	if (!sys_slist_find_and_remove(&cfg_nd->class_list, &c_nd->node)) {
		return -ENODATA;
	}

	return 0;
}

int usbd_class_remove_all(struct usbd_contex *const uds_ctx,
			  const uint8_t cfg)
{
	struct usbd_config_node *cfg_nd;
	struct usbd_class_node *c_nd;
	sys_snode_t *node;

	cfg_nd = usbd_config_get(uds_ctx, cfg);
	if (cfg_nd == NULL) {
		return -ENODATA;
	}

	while ((node = sys_slist_get(&cfg_nd->class_list))) {
		c_nd = CONTAINER_OF(node, struct usbd_class_node, node);
		atomic_clear_bit(&c_nd->data->state, USBD_CCTX_REGISTERED);
		usbd_class_shutdown(c_nd);
		LOG_DBG("Remove class node %p from configuration %u", c_nd, cfg);
	}

	return 0;
}

/*
 * All the functions below are part of public USB device support API.
 */

int usbd_register_class(struct usbd_contex *const uds_ctx,
			const char *name,
			const uint8_t cfg)
{
	struct usbd_class_node *c_nd;
	struct usbd_class_data *data;
	int ret;

	c_nd = usbd_class_node_get(name);
	if (c_nd == NULL) {
		return -ENODEV;
	}

	usbd_device_lock(uds_ctx);

	if (usbd_is_initialized(uds_ctx)) {
		LOG_ERR("USB device support is initialized");
		ret = -EBUSY;
		goto register_class_error;
	}

	data = c_nd->data;

	/* TODO: does it still need to be atomic ? */
	if (atomic_test_bit(&data->state, USBD_CCTX_REGISTERED)) {
		LOG_WRN("Class instance already registered");
		ret = -EBUSY;
		goto register_class_error;
	}

	ret = usbd_class_append(uds_ctx, c_nd, cfg);
	if (ret == 0) {
		/* Initialize pointer back to the device struct */
		atomic_set_bit(&data->state, USBD_CCTX_REGISTERED);
		data->uds_ctx = uds_ctx;
	}

register_class_error:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_unregister_class(struct usbd_contex *const uds_ctx,
			  const char *name,
			  const uint8_t cfg)
{
	struct usbd_class_node *c_nd;
	struct usbd_class_data *data;
	int ret;

	c_nd = usbd_class_node_get(name);
	if (c_nd == NULL) {
		return -ENODEV;
	}

	usbd_device_lock(uds_ctx);

	if (usbd_is_initialized(uds_ctx)) {
		LOG_ERR("USB device support is initialized");
		ret = -EBUSY;
		goto unregister_class_error;
	}

	data = c_nd->data;
	/* TODO: does it still need to be atomic ? */
	if (!atomic_test_bit(&data->state, USBD_CCTX_REGISTERED)) {
		LOG_WRN("Class instance not registered");
		ret = -EBUSY;
		goto unregister_class_error;
	}

	ret = usbd_class_remove(uds_ctx, c_nd, cfg);
	if (ret == 0) {
		atomic_clear_bit(&data->state, USBD_CCTX_REGISTERED);
		usbd_class_shutdown(c_nd);
		data->uds_ctx = NULL;
	}

unregister_class_error:
	usbd_device_unlock(uds_ctx);
	return ret;
}
