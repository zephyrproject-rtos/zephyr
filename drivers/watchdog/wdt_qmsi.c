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
#include <watchdog.h>

struct wdt_data {
#ifdef CONFIG_WDT_QMSI_API_REENTRANCY
	struct k_sem sem;
#endif
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t device_power_state;
#endif
	qm_wdt_config_t qm_cfg;
	wdt_callback_t callback;
};

static struct wdt_data wdt_context;
#define WDT_CONTEXT (&wdt_context)

#ifdef CONFIG_WDT_QMSI_API_REENTRANCY
#define RP_GET(dev) (&((struct wdt_data *)(dev->driver_data))->sem)
#else
#define RP_GET(dev) (NULL)
#endif

void wdt_handler(void *data)
{
	struct device *dev = data;
	struct wdt_data *wdt_context = dev->driver_data;

	wdt_context->callback(dev, 0);
}

static int wdt_qmsi_setup(struct device *dev, u8_t options)
{
	int ret_val = 0;

	struct wdt_data *wdt_context = dev->driver_data;
	qm_wdt_config_t *qm_cfg = &(wdt_context->qm_cfg);

	qm_cfg->callback = wdt_handler;
	qm_cfg->callback_data = dev;

	if (IS_ENABLED(CONFIG_WDT_QMSI_API_REENTRANCY)) {
		k_sem_take(RP_GET(dev), K_FOREVER);
	}

	if (qm_wdt_set_config(QM_WDT_0, qm_cfg)) {
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

static __attribute__((noinline)) u32_t next_pow2(u32_t x)
{
	if (x <= 2)
		return x;

	return (1ULL << 32) >> __builtin_clz(x - 1);
}

static u32_t get_timeout(u32_t  timeout)
{
	u32_t val = timeout / 2;
	u32_t count = 0U;

	if (val & (val - 1))
		val = next_pow2(val);

	while (val) {
		val = val >> 1;
		count++;
	}

	return ((count > 1U) ? (count - 1U) : 0);
}

static int wdt_qmsi_install_timeout(struct device *dev,
				    const struct wdt_timeout_cfg *cfg)
{
	struct wdt_data *wdt_context = dev->driver_data;
	qm_wdt_config_t *qm_cfg = &(wdt_context->qm_cfg);


	ARG_UNUSED(dev);

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		return -ENOTSUP;
	}

	if (cfg->window.min != 0 || cfg->window.max == 0) {
		return -EINVAL;
	}

	qm_cfg->timeout =  get_timeout(cfg->window.max);

	qm_cfg->mode = (cfg->callback == NULL) ?
			QM_WDT_MODE_RESET : QM_WDT_MODE_INTERRUPT_RESET;

	wdt_context->callback = cfg->callback;

	return 0;
}

static int reload(struct device *dev, int channel_id)
{
	qm_wdt_reload(QM_WDT_0);

	return 0;
}

static void enable(struct device *dev)
{
	clk_periph_enable(CLK_PERIPH_WDT_REGISTER | CLK_PERIPH_CLK);
}

static int disable(struct device *dev)
{
	clk_periph_disable(CLK_PERIPH_WDT_REGISTER);

	return 0;
}

static const struct wdt_driver_api api = {
	.setup = wdt_qmsi_setup,
	.disable = disable,
	.install_timeout = wdt_qmsi_install_timeout,
	.feed = reload,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static qm_wdt_context_t wdt_ctx;

static void wdt_qmsi_set_power_state(struct device *dev, u32_t power_state)
{
	struct wdt_data *context = dev->driver_data;

	context->device_power_state = power_state;
}

static u32_t wdt_qmsi_get_power_state(struct device *dev)
{
	struct wdt_data *context = dev->driver_data;

	return context->device_power_state;
}

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

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int wdt_qmsi_device_ctrl(struct device *dev, u32_t ctrl_command,
				void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return wdt_suspend_device(dev);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return wdt_resume_device_from_suspend(dev);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = wdt_qmsi_get_power_state(dev);
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
		k_sem_init(RP_GET(dev), 1, UINT_MAX);
	}

	IRQ_CONNECT(DT_WDT_0_IRQ, DT_WDT_0_IRQ_PRI,
		    qm_wdt_0_isr, 0, DT_WDT_0_IRQ_FLAGS);

	/* Unmask watchdog interrupt */
	irq_enable(IRQ_GET_NUMBER(QM_IRQ_WDT_0_INT));

	/* Route watchdog interrupt to the current core */
	QM_IR_UNMASK_INTERRUPTS(QM_INTERRUPT_ROUTER->wdt_0_int_mask);

	wdt_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	enable(dev);

	return 0;
}

DEVICE_DEFINE(wdt, CONFIG_WDT_0_NAME, init, wdt_qmsi_device_ctrl, WDT_CONTEXT,
	      0, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &api);
