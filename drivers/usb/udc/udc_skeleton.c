/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * USB device controller (UDC) driver skeleton
 *
 * This is a skeleton for a device controller driver using the UDC API.
 * Please use it as a starting point for a driver implementation for your
 * USB device controller. Maintaining a common style, terminology and
 * abbreviations will allow us to speed up reviews and reduce maintenance.
 * Copy UDC driver skeleton, remove all unrelated comments and replace the
 * copyright notice with your own.
 *
 * Typically, a driver implementation contains only a single source file,
 * but the large list of e.g. register definitions should be in a separate
 * .h file.
 *
 * If you want to define a helper macro, check if there is something similar
 * in include/zephyr/sys/util.h or include/zephyr/usb/usb_ch9.h that you can use.
 * Please keep all identifiers and logging messages concise and clear.
 */

#include "udc_common.h"

#include <string.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_skeleton, CONFIG_UDC_DRIVER_LOG_LEVEL);

/*
 * Structure for holding controller configuration items that can remain in
 * non-volatile memory. This is usually accessed as
 *   const struct udc_skeleton_config *config = dev->config;
 */
struct udc_skeleton_config {
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	void (*make_thread)(const struct device *dev);
	int speed_idx;
};

/*
 * Structure to hold driver private data.
 * Note that this is not accessible via dev->data, but as
 *   struct udc_skeleton_data *priv = udc_get_private(dev);
 */
struct udc_skeleton_data {
	struct k_thread thread_data;
};

/*
 * You can use one thread per driver instance model or UDC driver workqueue,
 * whichever model suits your needs best. If you decide to use the UDC workqueue,
 * enable Kconfig option UDC_WORKQUEUE and remove the handler below and
 * caller from the UDC_SKELETON_DEVICE_DEFINE macro.
 */
static ALWAYS_INLINE void skeleton_thread_handler(void *const arg)
{
	const struct device *dev = (const struct device *)arg;

	LOG_DBG("Driver %p thread started", dev);
	while (true) {
		k_msleep(1000);
	}
}

/*
 * This is called in the context of udc_ep_enqueue() and must
 * not block. The driver can immediately claim the buffer if the queue is empty,
 * but usually it is offloaded to a thread or workqueue to handle transfers
 * in a single location. Please refer to existing driver implementations
 * for examples.
 */
static int udc_skeleton_ep_enqueue(const struct device *dev,
				   struct udc_ep_config *const cfg,
				   struct net_buf *buf)
{
	LOG_DBG("%p enqueue %p", dev, buf);
	udc_buf_put(cfg, buf);

	if (cfg->stat.halted) {
		/*
		 * It is fine to enqueue a transfer for a halted endpoint,
		 * you need to make sure that transfers are retriggered when
		 * the halt is cleared.
		 *
		 * Always use the abbreviation 'ep' for the endpoint address
		 * and 'ep_idx' or 'ep_num' for the endpoint number identifiers.
		 * Although struct udc_ep_config uses address to be unambiguous
		 * in its context.
		 */
		LOG_DBG("ep 0x%02x halted", cfg->addr);
		return 0;
	}

	return 0;
}

/*
 * This is called in the context of udc_ep_dequeue()
 * and must remove all requests from an endpoint queue
 * Successful removal should be reported to the higher level with
 * ECONNABORTED as the request result.
 * It is up to the request owner to clean up or reuse the buffer.
 */
static int udc_skeleton_ep_dequeue(const struct device *dev,
				   struct udc_ep_config *const cfg)
{
	unsigned int lock_key;
	struct net_buf *buf;

	lock_key = irq_lock();

	buf = udc_buf_get_all(dev, cfg->addr);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	irq_unlock(lock_key);

	return 0;
}

/*
 * Configure and make an endpoint ready for use.
 * This is called in the context of udc_ep_enable() or udc_ep_enable_internal(),
 * the latter of which may be used by the driver to enable control endpoints.
 */
static int udc_skeleton_ep_enable(const struct device *dev,
				  struct udc_ep_config *const cfg)
{
	LOG_DBG("Enable ep 0x%02x", cfg->addr);

	return 0;
}

/*
 * Opposite function to udc_skeleton_ep_enable(). udc_ep_disable_internal()
 * may be used by the driver to disable control endpoints.
 */
static int udc_skeleton_ep_disable(const struct device *dev,
				   struct udc_ep_config *const cfg)
{
	LOG_DBG("Disable ep 0x%02x", cfg->addr);

	return 0;
}

/* Halt endpoint. Halted endpoint should respond with a STALL handshake. */
static int udc_skeleton_ep_set_halt(const struct device *dev,
				    struct udc_ep_config *const cfg)
{
	LOG_DBG("Set halt ep 0x%02x", cfg->addr);

	cfg->stat.halted = true;

	return 0;
}

/*
 * Opposite to halt endpoint. If there are requests in the endpoint queue,
 * the next transfer should be prepared.
 */
static int udc_skeleton_ep_clear_halt(const struct device *dev,
				      struct udc_ep_config *const cfg)
{
	LOG_DBG("Clear halt ep 0x%02x", cfg->addr);
	cfg->stat.halted = false;

	return 0;
}

static int udc_skeleton_set_address(const struct device *dev, const uint8_t addr)
{
	LOG_DBG("Set new address %u for %p", addr, dev);

	return 0;
}

static int udc_skeleton_host_wakeup(const struct device *dev)
{
	LOG_DBG("Remote wakeup from %p", dev);

	return 0;
}

/* Return actual USB device speed */
static enum udc_bus_speed udc_skeleton_device_speed(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return data->caps.hs ? UDC_BUS_SPEED_HS : UDC_BUS_SPEED_FS;
}

static int udc_skeleton_enable(const struct device *dev)
{
	LOG_DBG("Enable device %p", dev);

	return 0;
}

static int udc_skeleton_disable(const struct device *dev)
{
	LOG_DBG("Disable device %p", dev);

	return 0;
}

/*
 * Prepare and configure most of the parts, if the controller has a way
 * of detecting VBUS activity it should be enabled here.
 * Only udc_skeleton_enable() makes device visible to the host.
 */
static int udc_skeleton_init(const struct device *dev)
{
	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
				   USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN,
				   USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	return 0;
}

/* Shut down the controller completely */
static int udc_skeleton_shutdown(const struct device *dev)
{
	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	return 0;
}

/*
 * This is called once to initialize the controller and endpoints
 * capabilities, and register endpoint structures.
 */
static int udc_skeleton_driver_preinit(const struct device *dev)
{
	const struct udc_skeleton_config *config = dev->config;
	struct udc_data *data = dev->data;
	uint16_t mps = 1023;
	int err;

	/*
	 * You do not need to initialize it if your driver does not use
	 * udc_lock_internal() / udc_unlock_internal(), but implements its
	 * own mechanism.
	 */
	k_mutex_init(&data->mutex);

	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;
	if (config->speed_idx == 2) {
		data->caps.hs = true;
		mps = 1024;
	}

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			config->ep_cfg_out[i].caps.control = 1;
			config->ep_cfg_out[i].caps.mps = 64;
		} else {
			config->ep_cfg_out[i].caps.bulk = 1;
			config->ep_cfg_out[i].caps.interrupt = 1;
			config->ep_cfg_out[i].caps.iso = 1;
			config->ep_cfg_out[i].caps.mps = mps;
		}

		config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			config->ep_cfg_in[i].caps.control = 1;
			config->ep_cfg_in[i].caps.mps = 64;
		} else {
			config->ep_cfg_in[i].caps.bulk = 1;
			config->ep_cfg_in[i].caps.interrupt = 1;
			config->ep_cfg_in[i].caps.iso = 1;
			config->ep_cfg_in[i].caps.mps = mps;
		}

		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	config->make_thread(dev);
	LOG_INF("Device %p (max. speed %d)", dev, config->speed_idx);

	return 0;
}

static void udc_skeleton_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_skeleton_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

/*
 * UDC API structure.
 * Note, you do not need to implement basic checks, these are done by
 * the UDC common layer udc_common.c
 */
static const struct udc_api udc_skeleton_api = {
	.lock = udc_skeleton_lock,
	.unlock = udc_skeleton_unlock,
	.device_speed = udc_skeleton_device_speed,
	.init = udc_skeleton_init,
	.enable = udc_skeleton_enable,
	.disable = udc_skeleton_disable,
	.shutdown = udc_skeleton_shutdown,
	.set_address = udc_skeleton_set_address,
	.host_wakeup = udc_skeleton_host_wakeup,
	.ep_enable = udc_skeleton_ep_enable,
	.ep_disable = udc_skeleton_ep_disable,
	.ep_set_halt = udc_skeleton_ep_set_halt,
	.ep_clear_halt = udc_skeleton_ep_clear_halt,
	.ep_enqueue = udc_skeleton_ep_enqueue,
	.ep_dequeue = udc_skeleton_ep_dequeue,
};

#define DT_DRV_COMPAT zephyr_udc_skeleton

/*
 * A UDC driver should always be implemented as a multi-instance
 * driver, even if your platform does not require it.
 */
#define UDC_SKELETON_DEVICE_DEFINE(n)						\
	K_THREAD_STACK_DEFINE(udc_skeleton_stack_##n, CONFIG_UDC_SKELETON);	\
										\
	static void udc_skeleton_thread_##n(void *dev, void *arg1, void *arg2)	\
	{									\
		skeleton_thread_handler(dev);					\
	}									\
										\
	static void udc_skeleton_make_thread_##n(const struct device *dev)	\
	{									\
		struct udc_skeleton_data *priv = udc_get_private(dev);		\
										\
		k_thread_create(&priv->thread_data,				\
				udc_skeleton_stack_##n,				\
				K_THREAD_STACK_SIZEOF(udc_skeleton_stack_##n),	\
				udc_skeleton_thread_##n,			\
				(void *)dev, NULL, NULL,			\
				K_PRIO_COOP(CONFIG_UDC_SKELETON_THREAD_PRIORITY),\
				K_ESSENTIAL,					\
				K_NO_WAIT);					\
		k_thread_name_set(&priv->thread_data, dev->name);		\
	}									\
										\
	static struct udc_ep_config						\
		ep_cfg_out[DT_INST_PROP(n, num_bidir_endpoints)];		\
	static struct udc_ep_config						\
		ep_cfg_in[DT_INST_PROP(n, num_bidir_endpoints)];		\
										\
	static const struct udc_skeleton_config udc_skeleton_config_##n = {	\
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),		\
		.ep_cfg_in = ep_cfg_out,					\
		.ep_cfg_out = ep_cfg_in,					\
		.make_thread = udc_skeleton_make_thread_##n,			\
		.speed_idx = DT_ENUM_IDX(DT_DRV_INST(n), maximum_speed),	\
	};									\
										\
	static struct udc_skeleton_data udc_priv_##n = {			\
	};									\
										\
	static struct udc_data udc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),		\
		.priv = &udc_priv_##n,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, udc_skeleton_driver_preinit, NULL,		\
			      &udc_data_##n, &udc_skeleton_config_##n,		\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &udc_skeleton_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_SKELETON_DEVICE_DEFINE)
