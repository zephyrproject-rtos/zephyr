/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Roman Leonov <jam_roma@yahoo.com>
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_dwc2

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usb_ch9.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_dwc2, CONFIG_UHC_DRIVER_LOG_LEVEL);

#include <usb_dwc2_hw.h>

#include "uhc_common.h"

#define UHC_DWC2_CHAN_REG(base, chan_idx)					\
	((struct usb_dwc2_host_chan *)(((mem_addr_t)(base)) + USB_DWC2_HCCHAR(chan_idx)))

struct uhc_dwc2_config {
	/* Pointer to base address of DWC_OTG registers */
	struct usb_dwc2_reg *const base;
	/* Pointer to the stack used by the driver thread */
	k_thread_stack_t *stack;
	/* Size of the stack used by the driver thread */
	size_t stack_size;
	/* Sendor-specific structure */
	struct uhc_dwc2_quirk_data *quirk_data;

	void (*irq_enable_func)(const struct device *const dev);
	void (*irq_disable_func)(const struct device *const dev);
};

struct uhc_dwc2_data {
	struct k_thread thread;
	struct k_event events;
};

/*
 * Vendor quirks handling
 *
 * Definition of vendor-specific functions that can be overwritten on a per-SoC basis
 * by defining the associated macro to inhibit the default no-op alias.
 */

#if DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_usb_otg)
#include "uhc_dwc2_esp32.h"
#endif

/* Fallback no-op implementations */
#ifndef UHC_DWC2_HAS_QUIRK_INIT
static int uhc_dwc2_quirk_init(const struct device *const dev)
{
	LOG_DBG("Fallback quirk called for %s", __func__);
	return 0;
}
#endif
#ifndef UHC_DWC2_HAS_QUIRK_PRE_ENABLE
static int uhc_dwc2_quirk_pre_enable(const struct device *const dev)
{
	LOG_DBG("Fallback quirk called for %s", __func__);
	return 0;
}
#endif
#ifndef UHC_DWC2_HAS_QUIRK_POST_ENABLE
static int uhc_dwc2_quirk_post_enable(const struct device *const dev)
{
	LOG_DBG("Fallback quirk called for %s", __func__);
	return 0;
}
#endif
#ifndef UHC_DWC2_HAS_QUIRK_DISABLE
static int uhc_dwc2_quirk_disable(const struct device *const dev)
{
	LOG_DBG("Fallback quirk called for %s", __func__);
	return 0;
}
#endif
#ifndef UHC_DWC2_HAS_QUIRK_SHUTDOWN
static int uhc_dwc2_quirk_shutdown(const struct device *const dev)
{
	LOG_DBG("Fallback quirk called for %s", __func__);
	return 0;
}
#endif
#ifndef UHC_DWC2_HAS_QUIRK_IRQ_CLEAR
static int uhc_dwc2_quirk_irq_clear(const struct device *const dev)
{
	LOG_DBG("Fallback quirk called for %s", __func__);
	return 0;
}
#endif
#ifndef UHC_DWC2_HAS_QUIRK_PREINIT
static int uhc_dwc2_quirk_preinit(const struct device *const dev)
{
	LOG_DBG("Fallback quirk called for %s", __func__);
	return 0;
}
#endif

/*
 * Event handling
 */

static void uhc_dwc2_isr_handler(const struct device *dev)
{
	/* TODO: Interrupt handling */

	(void)uhc_dwc2_quirk_irq_clear(dev);
}

static void uhc_dwc2_thread(void *arg0, void *arg1, void *arg2)
{
	const struct device *const dev = (const struct device *)arg0;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	uint32_t events;

	while (true) {
		events = k_event_wait_safe(&priv->events, UINT32_MAX, false, K_FOREVER);

		uhc_lock_internal(dev, K_FOREVER);

		/* TODO: handle port and channel events */
		(void)events;

		uhc_unlock_internal(dev);
	}
}

/*
 * Device driver API
 */

static int uhc_dwc2_lock(const struct device *const dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_lock(&data->mutex, K_FOREVER);
}

static int uhc_dwc2_unlock(const struct device *const dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_unlock(&data->mutex);
}

static int uhc_dwc2_sof_enable(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_bus_suspend(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_bus_reset(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_bus_resume(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_enqueue(const struct device *const dev, struct uhc_transfer *const xfer)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_dequeue(const struct device *const dev, struct uhc_transfer *const xfer)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_preinit(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	int ret;

	ret = uhc_dwc2_quirk_preinit(dev);
	if (ret != 0) {
		return ret;
	}

	/*
	 * TODO:
	 * use devicetree to get GHWCFGn values and use them to determine the
	 * number and type of configured endpoints in the hardware as in udc?
	 */

	k_thread_create(&priv->thread,
			config->stack,
			config->stack_size,
			uhc_dwc2_thread,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_UHC_DWC2_THREAD_PRIORITY),
			K_ESSENTIAL,
			K_NO_WAIT);
	k_thread_name_set(&priv->thread, dev->name);

	return 0;
}

static int uhc_dwc2_init(const struct device *const dev)
{
	int ret;

	LOG_WRN("%s has not been implemented", __func__);

	ret = uhc_dwc2_quirk_init(dev);
	if (ret != 0) {
		return ret;
	}

	return -ENOSYS;
}

static int uhc_dwc2_enable(const struct device *const dev)
{
	int ret;

	LOG_WRN("%s has not been implemented", __func__);

	ret = uhc_dwc2_quirk_pre_enable(dev);
	if (ret != 0) {
		return ret;
	}

	ret = uhc_dwc2_quirk_post_enable(dev);
	if (ret != 0) {
		return ret;
	}

	return -ENOSYS;
}

static int uhc_dwc2_disable(const struct device *const dev)
{
	int ret;

	LOG_WRN("%s has not been implemented", __func__);

	ret = uhc_dwc2_quirk_disable(dev);
	if (ret != 0) {
		return ret;
	}

	return -ENOSYS;
}

static int uhc_dwc2_shutdown(const struct device *const dev)
{
	int ret;

	LOG_WRN("%s has not been implemented", __func__);

	ret = uhc_dwc2_quirk_shutdown(dev);
	if (ret != 0) {
		return ret;
	}

	return -ENOSYS;
}

static const struct uhc_api uhc_dwc2_api = {
	/* Common */
	.lock = uhc_dwc2_lock,
	.unlock = uhc_dwc2_unlock,
	.init = uhc_dwc2_init,
	.enable = uhc_dwc2_enable,
	.disable = uhc_dwc2_disable,
	.shutdown = uhc_dwc2_shutdown,
	/* Bus related */
	.bus_reset = uhc_dwc2_bus_reset,
	.sof_enable = uhc_dwc2_sof_enable,
	.bus_suspend = uhc_dwc2_bus_suspend,
	.bus_resume = uhc_dwc2_bus_resume,
	/* EP related */
	.ep_enqueue = uhc_dwc2_enqueue,
	.ep_dequeue = uhc_dwc2_dequeue,
};

/* Default IRQ enable/disable functions */
#ifndef UHC_DWC2_IRQ_DT_INST_DEFINE
#define UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
	static void uhc_dwc2_irq_enable_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			uhc_dwc2_isr_handler, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static void uhc_dwc2_irq_disable_func_##n(const struct device *dev)	\
	{									\
		irq_disable(DT_INST_IRQN(n));					\
	}
#endif

#define UHC_DWC2_DEVICE_DEFINE(n)						\
	K_THREAD_STACK_DEFINE(uhc_dwc2_stack_##n, CONFIG_UHC_DWC2_STACK_SIZE);	\
	UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
	UHC_DWC2_QUIRK_DEFINE(n)						\
										\
	static struct uhc_dwc2_data uhc_dwc2_priv_##n = {			\
		.events = Z_EVENT_INITIALIZER(uhc_dwc2_priv_##n.events),	\
	};									\
										\
	static struct uhc_data uhc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(uhc_data_##n.mutex),		\
		.priv = &uhc_dwc2_priv_##n,					\
	};									\
										\
	static const struct uhc_dwc2_config uhc_dwc2_config_##n = {		\
		.base = (struct usb_dwc2_reg *)DT_INST_REG_ADDR(n),		\
		.stack = uhc_dwc2_stack_##n,					\
		.stack_size = K_THREAD_STACK_SIZEOF(uhc_dwc2_stack_##n),	\
		.quirk_data = &uhc_dwc2_quirk_data_##n,				\
		.irq_enable_func = uhc_dwc2_irq_enable_func_##n,		\
		.irq_disable_func = uhc_dwc2_irq_disable_func_##n,		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, uhc_dwc2_preinit, NULL, &uhc_data_##n,		\
			      &uhc_dwc2_config_##n, POST_KERNEL,		\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &uhc_dwc2_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_DWC2_DEVICE_DEFINE)
