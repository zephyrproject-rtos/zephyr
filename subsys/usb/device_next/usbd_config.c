/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/usb/udc.h>
#include <zephyr/usb/usbd.h>

#include "usbd_device.h"
#include "usbd_config.h"
#include "usbd_interface.h"
#include "usbd_ch9.h"
#include "usbd_class_api.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_cfg, CONFIG_USBD_LOG_LEVEL);

struct usbd_config_node *usbd_config_get(struct usbd_contex *const uds_ctx,
					 const uint8_t cfg)
{
	struct usbd_config_node *cfg_nd;

	SYS_SLIST_FOR_EACH_CONTAINER(&uds_ctx->configs, cfg_nd, node) {
		if (usbd_config_get_value(cfg_nd) == cfg) {
			return cfg_nd;
		}
	}

	return NULL;
}

struct usbd_config_node *
usbd_config_get_current(struct usbd_contex *const uds_ctx)
{
	if (!usbd_state_is_configured(uds_ctx)) {
		LOG_INF("No configuration set (Address state?)");
		return NULL;
	}

	return usbd_config_get(uds_ctx, usbd_get_config_value(uds_ctx));
}

static void usbd_config_classes_enable(struct usbd_config_node *const cfg_nd,
				       const bool enable)
{
	struct usbd_class_iter *iter;

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, iter, node) {
		if (enable) {
			usbd_class_enable(iter->c_nd);
		} else {
			usbd_class_disable(iter->c_nd);
		}
	}
}

/* Reset configuration to addressed state, shutdown all endpoints */
static int usbd_config_reset(struct usbd_contex *const uds_ctx)
{
	struct usbd_config_node *cfg_nd;
	int ret = 0;

	cfg_nd = usbd_config_get_current(uds_ctx);
	if (cfg_nd == NULL) {
		return -ENODATA;
	}

	ret = usbd_interface_shutdown(uds_ctx, cfg_nd);

	memset(&uds_ctx->ch9_data.alternate, 0,
	       USBD_NUMOF_INTERFACES_MAX);

	usbd_set_config_value(uds_ctx, 0);
	usbd_config_classes_enable(cfg_nd, false);

	return ret;
}

bool usbd_config_exist(struct usbd_contex *const uds_ctx,
		       const uint8_t cfg)
{
	struct usbd_config_node *config;

	config = usbd_config_get(uds_ctx, cfg);

	return (config != NULL) ? true : false;
}

int usbd_config_set(struct usbd_contex *const uds_ctx,
		    const uint8_t new_cfg)
{
	struct usbd_config_node *cfg_nd;
	int ret;

	if (usbd_get_config_value(uds_ctx) != 0) {
		ret = usbd_config_reset(uds_ctx);
		if (ret) {
			LOG_ERR("Failed to reset configuration");
			return ret;
		}
	}

	if (new_cfg == 0) {
		usbd_set_config_value(uds_ctx, new_cfg);
		return 0;
	}

	cfg_nd = usbd_config_get(uds_ctx, new_cfg);
	if (cfg_nd == NULL) {
		return -ENODATA;
	}

	ret = usbd_interface_default(uds_ctx, cfg_nd);
	if (ret) {
		return ret;
	}

	usbd_set_config_value(uds_ctx, new_cfg);
	usbd_config_classes_enable(cfg_nd, true);

	return 0;
}

/*
 * All the functions below are part of public USB device support API.
 */

int usbd_config_attrib_rwup(struct usbd_contex *const uds_ctx,
			    const uint8_t cfg, const bool enable)
{
	struct usbd_config_node *cfg_nd;
	struct usb_cfg_descriptor *desc;
	struct udc_device_caps caps;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto attrib_rwup_exit;
	}

	caps = udc_caps(uds_ctx->dev);
	if (!caps.rwup) {
		LOG_ERR("Feature not supported by controller");
		ret = -ENOTSUP;
		goto attrib_rwup_exit;
	}

	cfg_nd = usbd_config_get(uds_ctx, cfg);
	if (cfg_nd == NULL) {
		LOG_INF("Configuration %u not found", cfg);
		ret = -ENODATA;
		goto attrib_rwup_exit;
	}

	desc = cfg_nd->desc;
	if (enable) {
		desc->bmAttributes |= USB_SCD_REMOTE_WAKEUP;
	} else {
		desc->bmAttributes &= ~USB_SCD_REMOTE_WAKEUP;
	}

attrib_rwup_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_config_attrib_self(struct usbd_contex *const uds_ctx,
			    const uint8_t cfg, const bool enable)
{
	struct usbd_config_node *cfg_nd;
	struct usb_cfg_descriptor *desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto attrib_self_exit;
	}

	cfg_nd = usbd_config_get(uds_ctx, cfg);
	if (cfg_nd == NULL) {
		LOG_INF("Configuration %u not found", cfg);
		ret = -ENODATA;
		goto attrib_self_exit;
	}

	desc = cfg_nd->desc;
	if (enable) {
		desc->bmAttributes |= USB_SCD_SELF_POWERED;
	} else {
		desc->bmAttributes &= ~USB_SCD_SELF_POWERED;
	}

attrib_self_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_config_maxpower(struct usbd_contex *const uds_ctx,
			 const uint8_t cfg, const uint8_t power)
{
	struct usbd_config_node *cfg_nd;
	struct usb_cfg_descriptor *desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto maxpower_exit;
	}

	cfg_nd = usbd_config_get(uds_ctx, cfg);
	if (cfg_nd == NULL) {
		LOG_INF("Configuration %u not found", cfg);
		ret = -ENODATA;
		goto maxpower_exit;
	}

	desc = cfg_nd->desc;
	desc->bMaxPower = power;

maxpower_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_add_configuration(struct usbd_contex *const uds_ctx,
			   struct usbd_config_node *const cfg_nd)
{
	struct usb_cfg_descriptor *desc = cfg_nd->desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_initialized(uds_ctx)) {
		LOG_ERR("USB device support is initialized");
		ret = -EBUSY;
		goto add_configuration_exit;
	}

	if (desc->bmAttributes & USB_SCD_REMOTE_WAKEUP) {
		struct udc_device_caps caps = udc_caps(uds_ctx->dev);

		if (!caps.rwup) {
			LOG_ERR("Feature not supported by controller");
			ret = -ENOTSUP;
			goto add_configuration_exit;
		}
	}

	if (sys_slist_find_and_remove(&uds_ctx->configs, &cfg_nd->node)) {
		LOG_WRN("Configuration %u re-inserted",
			usbd_config_get_value(cfg_nd));
	} else {
		usbd_config_set_value(cfg_nd, usbd_get_num_configs(uds_ctx) + 1);
		usbd_set_num_configs(uds_ctx, usbd_get_num_configs(uds_ctx) + 1);
	}

	sys_slist_append(&uds_ctx->configs, &cfg_nd->node);

	usbd_device_unlock(uds_ctx);

add_configuration_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}
