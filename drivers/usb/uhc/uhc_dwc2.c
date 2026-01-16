/*
 * Copyright (c) 2026 Roman Leonov <jam_roma@yahoo.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_dwc2

#include "uhc_common.h"
#include "uhc_dwc2.h"

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usb_ch9.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_dwc2, CONFIG_UHC_DRIVER_LOG_LEVEL);

K_THREAD_STACK_DEFINE(uhc_dwc2_stack, CONFIG_UHC_DWC2_STACK_SIZE);

#define UHC_DWC2_CHAN_REG(base, chan_idx)					\
	((struct usb_dwc2_host_chan *)(((mem_addr_t)(base)) + USB_DWC2_HCCHAR(chan_idx)))

struct uhc_dwc2_data {
	struct k_sem irq_sem;
	struct k_thread thread;
	struct k_event event;
};

static void uhc_dwc2_isr_handler(const struct device *dev)
{
	/* TODO: Interrupt handling */
}

static void uhc_dwc2_thread(void *arg0, void *arg1, void *arg2)
{
	const struct device *const dev = (const struct device *)arg0;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	uint32_t event;

	while (true) {
		event = k_event_wait_safe(&priv->event, UINT32_MAX, false, K_FOREVER);

		uhc_lock_internal(dev, K_FOREVER);

		/* TODO: handle port and channel events */
		(void) event;

		uhc_unlock_internal(dev);
	}
}

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
	struct uhc_data *const data = dev->data;

	/* Initialize the private data structure */
	memset(priv, 0, sizeof(struct uhc_dwc2_data));
	k_mutex_init(&data->mutex);
	k_event_init(&priv->event);

	uhc_dwc2_quirk_caps(dev);

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
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_enable(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_disable(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_shutdown(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

/*
 * Device Definition and Initialization
 */
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

#define UHC_DWC2_DT_INST_REG_ADDR(n)						\
	COND_CODE_1(DT_NUM_REGS(DT_DRV_INST(n)),				\
			(DT_INST_REG_ADDR(n)),					\
			(DT_INST_REG_ADDR_BY_NAME(n, core)))

#if !defined(UHC_DWC2_IRQ_DT_INST_DEFINE)
#define UHC_DWC2_IRQ_FLAGS_TYPE0(n)	0
#define UHC_DWC2_IRQ_FLAGS_TYPE1(n)	DT_INST_IRQ(n, type)
#define DW_IRQ_FLAGS(n)								\
	_CONCAT(UHC_DWC2_IRQ_FLAGS_TYPE, DT_INST_IRQ_HAS_CELL(n, type))(n)

#define UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
	static void uhc_dwc2_irq_enable_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    uhc_dwc2_isr_handler,				\
			    DEVICE_DT_INST_GET(n),				\
			    DW_IRQ_FLAGS(n));					\
										\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static void uhc_dwc2_irq_disable_func_##n(const struct device *dev)	\
	{									\
		irq_disable(DT_INST_IRQN(n));					\
	}
#endif

#define UHC_DWC2_PINCTRL_DT_INST_DEFINE(n)					\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),			\
			(PINCTRL_DT_INST_DEFINE(n)), ())

#define UHC_DWC2_PINCTRL_DT_INST_DEV_CONFIG_GET(n)				\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),			\
			((void *)PINCTRL_DT_INST_DEV_CONFIG_GET(n)),		\
			(NULL))

/* Multi-instance device definition for DWC2 host controller */
#define UHC_DWC2_DEVICE_DEFINE(n)						\
	UHC_DWC2_PINCTRL_DT_INST_DEFINE(n);					\
										\
	K_THREAD_STACK_DEFINE(uhc_dwc2_stack_##n,				\
		CONFIG_UHC_DWC2_STACK_SIZE);					\
										\
	UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
										\
	static struct uhc_dwc2_data uhc_dwc2_priv_##n = {			\
		.irq_sem = Z_SEM_INITIALIZER(uhc_dwc2_priv_##n.irq_sem, 0, 1),	\
	};									\
										\
	static struct uhc_data uhc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(uhc_data_##n.mutex),		\
		.priv = &uhc_dwc2_priv_##n,					\
	};									\
										\
	static const struct uhc_dwc2_config uhc_dwc2_config_##n = {		\
		.base = (struct usb_dwc2_reg *)UHC_DWC2_DT_INST_REG_ADDR(n),	\
		.quirks = UHC_DWC2_VENDOR_QUIRK_GET(n),				\
		.pcfg = UHC_DWC2_PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.stack = uhc_dwc2_stack_##n,					\
		.stack_size = K_THREAD_STACK_SIZEOF(uhc_dwc2_stack_##n),	\
		.irq_enable_func = uhc_dwc2_irq_enable_func_##n,		\
		.irq_disable_func = uhc_dwc2_irq_disable_func_##n,		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, uhc_dwc2_preinit, NULL,			\
		&uhc_data_##n, &uhc_dwc2_config_##n,				\
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
		&uhc_dwc2_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_DWC2_DEVICE_DEFINE)
