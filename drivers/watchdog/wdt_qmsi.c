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
#ifdef CONFIG_WDT_QMSI_API_REENTRANCY
	struct nano_sem sem;
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
static const int reentrancy_protection = 1;
#define RP_GET(dev) (&((struct wdt_data *)(dev->driver_data))->sem)
#else
static const int reentrancy_protection;
#define RP_GET(dev) (NULL)
#endif

static void wdt_reentrancy_init(struct device *dev)
{
	if (!reentrancy_protection) {
		return;
	}

	nano_sem_init(RP_GET(dev));
	nano_sem_give(RP_GET(dev));
}

static void wdt_critical_region_start(struct device *dev)
{
	if (!reentrancy_protection) {
		return;
	}

	nano_sem_take(RP_GET(dev), TICKS_UNLIMITED);
}

static void wdt_critical_region_end(struct device *dev)
{
	if (!reentrancy_protection) {
		return;
	}

	nano_sem_give(RP_GET(dev));
}

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

static const struct wdt_driver_api api = {
	.enable = enable,
	.disable = disable,
	.get_config = get_config,
	.set_config = set_config,
	.reload = reload,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
struct wdt_ctx {
	uint32_t wdt_cr;
	uint32_t wdt_torr;
	uint32_t int_watchdog_mask;
};

static struct wdt_ctx wdt_ctx_save;


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


static int wdt_suspend_device(struct device *dev)
{
	wdt_ctx_save.wdt_torr = QM_WDT[QM_WDT_0].wdt_torr;
	wdt_ctx_save.wdt_cr = QM_WDT[QM_WDT_0].wdt_cr;
	wdt_ctx_save.int_watchdog_mask =
	QM_SCSS_INT->int_watchdog_mask;

	wdt_qmsi_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int wdt_resume_device_from_suspend(struct device *dev)
{
	/* TOP_INIT field has to be written before
	 * the Watchdog Timer is enabled.
	 */
	QM_WDT[QM_WDT_0].wdt_torr = wdt_ctx_save.wdt_torr;
	QM_WDT[QM_WDT_0].wdt_cr = wdt_ctx_save.wdt_cr;
	QM_SCSS_INT->int_watchdog_mask = wdt_ctx_save.int_watchdog_mask;

	wdt_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int wdt_qmsi_device_ctrl(struct device *dev, uint32_t ctrl_command,
				void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((uint32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return wdt_suspend_device(dev);
		} else if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return wdt_resume_device_from_suspend(dev);
		}
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
	wdt_reentrancy_init(dev);

	IRQ_CONNECT(QM_IRQ_WDT_0, CONFIG_WDT_0_IRQ_PRI,
		    qm_wdt_isr_0, 0, IOAPIC_EDGE | IOAPIC_HIGH);

	/* Unmask watchdog interrupt */
	irq_enable(QM_IRQ_WDT_0);

	/* Route watchdog interrupt to Lakemont */
	QM_SCSS_INT->int_watchdog_mask &= ~BIT(0);

	wdt_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

DEVICE_DEFINE(wdt, CONFIG_WDT_0_NAME, init, wdt_qmsi_device_ctrl, WDT_CONTEXT,
	      0, PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &api);
