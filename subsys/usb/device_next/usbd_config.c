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

static sys_slist_t *usbd_configs(struct usbd_context *uds_ctx,
				 const enum usbd_speed speed)
{
	switch (speed) {
	case USBD_SPEED_FS:
		return &uds_ctx->fs_configs;
	case USBD_SPEED_HS:
		return &uds_ctx->hs_configs;
	default:
		return NULL;
	}
}

struct usbd_config_node *usbd_config_get(struct usbd_context *const uds_ctx,
					 const enum usbd_speed speed,
					 const uint8_t cfg)
{
	struct usbd_config_node *cfg_nd;

	SYS_SLIST_FOR_EACH_CONTAINER(usbd_configs(uds_ctx, speed), cfg_nd, node) {
		if (usbd_config_get_value(cfg_nd) == cfg) {
			return cfg_nd;
		}
	}

	return NULL;
}

struct usbd_config_node *
usbd_config_get_current(struct usbd_context *const uds_ctx)
{
	if (!usbd_state_is_configured(uds_ctx)) {
		LOG_INF("No configuration set (Address state?)");
		return NULL;
	}

	return usbd_config_get(uds_ctx, usbd_bus_speed(uds_ctx),
			       usbd_get_config_value(uds_ctx));
}

static void usbd_config_classes_enable(struct usbd_config_node *const cfg_nd,
				       const bool enable)
{
	struct usbd_class_node *c_nd;

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		if (enable) {
			usbd_class_enable(c_nd->c_data);
		} else {
			usbd_class_disable(c_nd->c_data);
		}
	}
}

/* Reset configuration to addressed state, shutdown all endpoints */
static int usbd_config_reset(struct usbd_context *const uds_ctx)
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

bool usbd_config_exist(struct usbd_context *const uds_ctx,
		       const enum usbd_speed speed,
		       const uint8_t cfg)
{
	struct usbd_config_node *config;

	config = usbd_config_get(uds_ctx, speed, cfg);

	return (config != NULL) ? true : false;
}

int usbd_config_set(struct usbd_context *const uds_ctx,
		    const uint8_t new_cfg)
{
	struct usbd_config_node *cfg_nd;
	const enum usbd_speed speed = usbd_bus_speed(uds_ctx);
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

	cfg_nd = usbd_config_get(uds_ctx, speed, new_cfg);
	if (cfg_nd == NULL) {
		return -ENODATA;
	}

	ret = usbd_interface_default(uds_ctx, speed, cfg_nd);
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

int usbd_config_attrib_rwup(struct usbd_context *const uds_ctx,
			    const enum usbd_speed speed,
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

	cfg_nd = usbd_config_get(uds_ctx, speed, cfg);
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

int usbd_config_attrib_self(struct usbd_context *const uds_ctx,
			    const enum usbd_speed speed,
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

	cfg_nd = usbd_config_get(uds_ctx, speed, cfg);
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

int usbd_config_maxpower(struct usbd_context *const uds_ctx,
			 const enum usbd_speed speed,
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

	cfg_nd = usbd_config_get(uds_ctx, speed, cfg);
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

int usbd_add_configuration(struct usbd_context *const uds_ctx,
			   const enum usbd_speed speed,
			   struct usbd_config_node *const cfg_nd)
{
	struct usb_cfg_descriptor *desc = cfg_nd->desc;
	sys_slist_t *configs;
	sys_snode_t *node;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_initialized(uds_ctx)) {
		LOG_ERR("USB device support is initialized");
		ret = -EBUSY;
		goto add_configuration_exit;
	}

	if (speed == USBD_SPEED_HS && !USBD_SUPPORTS_HIGH_SPEED) {
		LOG_ERR("Stack was compiled without High-Speed support");
		ret = -ENOTSUP;
		goto add_configuration_exit;
	}

	if (speed == USBD_SPEED_HS &&
	    usbd_caps_speed(uds_ctx) == USBD_SPEED_FS) {
		LOG_ERR("Controller doesn't support HS");
		ret = -ENOTSUP;
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

	configs = usbd_configs(uds_ctx, speed);
	switch (speed) {
	case USBD_SPEED_HS:
		SYS_SLIST_FOR_EACH_NODE(&uds_ctx->fs_configs, node) {
			if (node == &cfg_nd->node) {
				LOG_ERR("HS config already on FS list");
				ret = -EINVAL;
				goto add_configuration_exit;
			}
		}
		break;
	case USBD_SPEED_FS:
		SYS_SLIST_FOR_EACH_NODE(&uds_ctx->hs_configs, node) {
			if (node == &cfg_nd->node) {
				LOG_ERR("FS config already on HS list");
				ret = -EINVAL;
				goto add_configuration_exit;
			}
		}
		break;
	default:
		LOG_ERR("Unsupported configuration speed");
		ret = -ENOTSUP;
		goto add_configuration_exit;
	}

	if (sys_slist_find_and_remove(configs, &cfg_nd->node)) {
		LOG_WRN("Configuration %u re-inserted",
			usbd_config_get_value(cfg_nd));
	} else {
		uint8_t num = usbd_get_num_configs(uds_ctx, speed) + 1;

		usbd_config_set_value(cfg_nd, num);
		usbd_set_num_configs(uds_ctx, speed, num);
	}

	if (cfg_nd->str_desc_nd != NULL) {
		ret = usbd_add_descriptor(uds_ctx, cfg_nd->str_desc_nd);
		if (ret != 0) {
			LOG_ERR("Failed to add configuration string descriptor");
			goto add_configuration_exit;
		}

		desc->iConfiguration = usbd_str_desc_get_idx(cfg_nd->str_desc_nd);
	}

	sys_slist_append(configs, &cfg_nd->node);

add_configuration_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}
