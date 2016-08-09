/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <init.h>
#include <device.h>
#include <watchdog.h>
#include <ioapic.h>
#include <power.h>

#include "clk.h"
#include "qm_interrupt.h"
#include "qm_isr.h"
#include "qm_wdt.h"

struct wdt_data {
	struct nano_sem sem;
};

#ifdef CONFIG_WDT_QMSI_API_REENTRANCY
static struct wdt_data wdt_context;
#define WDT_CONTEXT (&wdt_context)
static const int reentrancy_protection = 1;
#else
#define WDT_CONTEXT (NULL)
static const int reentrancy_protection;
#endif /* CONFIG_WDT_QMSI_API_REENTRANCY */

static void wdt_reentrancy_init(struct device *dev)
{
	struct wdt_data *context = dev->driver_data;

	if (!reentrancy_protection) {
		return;
	}

	nano_sem_init(&context->sem);
	nano_sem_give(&context->sem);
}

static void wdt_critical_region_start(struct device *dev)
{
	struct wdt_data *context = dev->driver_data;

	if (!reentrancy_protection) {
		return;
	}

	nano_sem_take(&context->sem, TICKS_UNLIMITED);
}

static void wdt_critical_region_end(struct device *dev)
{
	struct wdt_data *context = dev->driver_data;

	if (!reentrancy_protection) {
		return;
	}

	nano_sem_give(&context->sem);
}

static void (*user_cb)(struct device *dev);

static void get_config(struct device *dev, struct wdt_config *cfg)
{
	cfg->timeout = QM_WDT[QM_WDT_0].wdt_torr;
	cfg->mode = ((QM_WDT[QM_WDT_0].wdt_cr & QM_WDT_MODE) >>
			QM_WDT_MODE_OFFSET);
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

	wdt_critical_region_start(dev);

	if (qm_wdt_set_config(QM_WDT_0, &qm_cfg)) {
		ret_val = -EIO;
		goto wdt_config_return;
	}

	if (qm_wdt_start(QM_WDT_0)) {
		ret_val = -EIO;
	}

wdt_config_return:
	wdt_critical_region_end(dev);

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

static struct wdt_driver_api api = {
	.enable = enable,
	.disable = disable,
	.get_config = get_config,
	.set_config = set_config,
	.reload = reload,
};

static int init(struct device *dev)
{
	wdt_reentrancy_init(dev);

	IRQ_CONNECT(QM_IRQ_WDT_0, CONFIG_WDT_0_IRQ_PRI,
		    qm_wdt_isr_0, 0, IOAPIC_EDGE | IOAPIC_HIGH);

	/* Unmask watchdog interrupt */
	irq_enable(QM_IRQ_WDT_0);

	/* Route watchdog interrupt to Lakemont */
	QM_SCSS_INT->int_watchdog_mask &= ~BIT(0);

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
struct wdt_ctx {
	uint32_t wdt_cr;
	uint32_t wdt_torr;
	uint32_t int_watchdog_mask;
};

static struct wdt_ctx wdt_ctx_save;

static int wdt_suspend_device(struct device *dev, int pm_policy)
{
	if (pm_policy == SYS_PM_DEEP_SLEEP) {
		wdt_ctx_save.wdt_torr = QM_WDT[QM_WDT_0].wdt_torr;
		wdt_ctx_save.wdt_cr = QM_WDT[QM_WDT_0].wdt_cr;
		wdt_ctx_save.int_watchdog_mask =
			QM_SCSS_INT->int_watchdog_mask;
	}

	return 0;
}

static int wdt_resume_device(struct device *dev, int pm_policy)
{
	if (pm_policy == SYS_PM_DEEP_SLEEP) {
		/* TOP_INIT field has to be written before
		 * the Watchdog Timer is enabled.
		 */
		QM_WDT[QM_WDT_0].wdt_torr = wdt_ctx_save.wdt_torr;
		QM_WDT[QM_WDT_0].wdt_cr = wdt_ctx_save.wdt_cr;
		QM_SCSS_INT->int_watchdog_mask =
			wdt_ctx_save.int_watchdog_mask;
	}

	return 0;
}
#endif

DEFINE_DEVICE_PM_OPS(wdt, wdt_suspend_device, wdt_resume_device);

DEVICE_AND_API_INIT_PM(wdt, CONFIG_WDT_0_NAME, init, DEVICE_PM_OPS_GET(wdt),
		    WDT_CONTEXT, 0, PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&api);
