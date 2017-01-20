/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <device.h>
#include <kernel.h>
#include <watchdog.h>
#include <ioapic.h>
#include <power.h>
#include <soc.h>

#include "clk.h"
#include "qm_isr.h"
#include "qm_wdt.h"

struct wdt_data {
#ifdef CONFIG_WDT_QMSI_API_REENTRANCY
	struct k_sem sem;
#endif
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uint32_t device_power_state;
#endif
};

#define WDT_HAS_CONTEXT_DATA \
	(CONFIG_WDT_QMSI_API_REENTRANCY || CONFIG_DEVICE_POWER_MANAGEMENT)

#if WDT_HAS_CONTEXT_DATA
static struct wdt_data wdt_context;
#define WDT_CONTEXT (&wdt_context)
#else
#define WDT_CONTEXT (NULL)
#endif /* WDT_HAS_CONTEXT_DATA */

#ifdef CONFIG_WDT_QMSI_API_REENTRANCY
#define RP_GET(dev) (&((struct wdt_data *)(dev->driver_data))->sem)
#else
#define RP_GET(dev) (NULL)
#endif

static void (*user_cb)(struct device *dev);

static void get_config(struct device *dev, struct wdt_config *cfg)
{
	cfg->timeout = QM_WDT[QM_WDT_0].wdt_torr;
	cfg->mode = ((QM_WDT[QM_WDT_0].wdt_cr & QM_WDT_CR_RMOD) >>
			QM_WDT_CR_RMOD_OFFSET);
	cfg->interrupt_fn = user_cb;
}

static int set_config(struct device *dev, struct wdt_config *cfg)
{
	int ret_val = 0;
	qm_wdt_config_t qm_cfg;

	user_cb = cfg->interrupt_fn;
	qm_cfg.timeout = cfg->timeout;
	qm_cfg.mode = (cfg->mode == WDT_MODE_RESET) ?
			QM_WDT_MODE_RESET : QM_WDT_MODE_INTERRUPT_RESET;
	qm_cfg.callback = (void *)user_cb;
	qm_cfg.callback_data = dev;

	if (IS_ENABLED(CONFIG_WDT_QMSI_API_REENTRANCY)) {
		k_sem_take(RP_GET(dev), K_FOREVER);
	}

	if (qm_wdt_set_config(QM_WDT_0, &qm_cfg)) {
		ret_val = -EIO;
		goto wdt_config_return;
	}

	if (qm_wdt_start(QM_WDT_0)) {
		ret_val = -EIO;
	}

wdt_config_return:
	if (IS_ENABLED(CONFIG_WDT_QMSI_API_REENTRANCY)) {
		k_sem_give(RP_GET(dev));
	}

	return ret_val;
}

static void reload(struct device *dev)
{
	qm_wdt_reload(QM_WDT_0);
}

static void enable(struct device *dev)
{
	clk_periph_enable(CLK_PERIPH_WDT_REGISTER | CLK_PERIPH_CLK);
}

static void disable(struct device *dev)
{
	clk_periph_disable(CLK_PERIPH_WDT_REGISTER);
}

static const struct wdt_driver_api api = {
	.enable = enable,
	.disable = disable,
	.get_config = get_config,
	.set_config = set_config,
	.reload = reload,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void wdt_qmsi_set_power_state(struct device *dev, uint32_t power_state)
{
	struct wdt_data *context = dev->driver_data;

	context->device_power_state = power_state;
}

static uint32_t wdt_qmsi_get_power_state(struct device *dev)
{
	struct wdt_data *context = dev->driver_data;

	return context->device_power_state;
}

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
static qm_wdt_context_t wdt_ctx;

static int wdt_suspend_device(struct device *dev)
{
	qm_wdt_save_context(QM_WDT_0, &wdt_ctx);

	wdt_qmsi_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int wdt_resume_device_from_suspend(struct device *dev)
{
	qm_wdt_restore_context(QM_WDT_0, &wdt_ctx);

	wdt_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}
#endif

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int wdt_qmsi_device_ctrl(struct device *dev, uint32_t ctrl_command,
				void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
		if (*((uint32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return wdt_suspend_device(dev);
		} else if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return wdt_resume_device_from_suspend(dev);
		}
#endif
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((uint32_t *)context) = wdt_qmsi_get_power_state(dev);
		return 0;
	}

	return 0;
}
#else
#define wdt_qmsi_set_power_state(...)
#endif

static int init(struct device *dev)
{
	if (IS_ENABLED(CONFIG_WDT_QMSI_API_REENTRANCY)) {
		k_sem_init(RP_GET(dev), 0, UINT_MAX);
		k_sem_give(RP_GET(dev));
	}

	IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_WDT_0_INT), CONFIG_WDT_0_IRQ_PRI,
		    qm_wdt_0_isr, 0, IOAPIC_EDGE | IOAPIC_HIGH);

	/* Unmask watchdog interrupt */
	irq_enable(IRQ_GET_NUMBER(QM_IRQ_WDT_0_INT));

	/* Route watchdog interrupt to the current core */
	QM_IR_UNMASK_INTERRUPTS(QM_INTERRUPT_ROUTER->wdt_0_int_mask);

	wdt_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

DEVICE_DEFINE(wdt, CONFIG_WDT_0_NAME, init, wdt_qmsi_device_ctrl, WDT_CONTEXT,
	      0, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &api);
