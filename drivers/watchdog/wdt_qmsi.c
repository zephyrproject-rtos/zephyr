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

#include "clk.h"
#include "qm_interrupt.h"
#include "qm_isr.h"
#include "qm_wdt.h"

static void (*user_cb)(struct device *dev);

/* global variable to track qmsi wdt conf */
static qm_wdt_config_t qm_cfg;

static void get_config(struct device *dev, struct wdt_config *cfg)
{
	cfg->timeout = qm_cfg.timeout;
	cfg->mode = (qm_cfg.mode == QM_WDT_MODE_RESET) ?
			WDT_MODE_RESET : WDT_MODE_INTERRUPT_RESET;
	cfg->interrupt_fn = user_cb;
}

static int set_config(struct device *dev, struct wdt_config *cfg)
{
	user_cb = cfg->interrupt_fn;

	qm_cfg.timeout = cfg->timeout;
	qm_cfg.mode = (cfg->mode == WDT_MODE_RESET) ?
			QM_WDT_MODE_RESET : QM_WDT_MODE_INTERRUPT_RESET;
	qm_cfg.callback = (void *)user_cb;
	qm_cfg.callback_data = dev;
	if (qm_wdt_set_config(QM_WDT_0, &qm_cfg) != 0) {
		return -EIO;
	}

	return qm_wdt_start(QM_WDT_0) == 0 ? 0 : -EIO;
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
	IRQ_CONNECT(QM_IRQ_WDT_0, CONFIG_WDT_0_IRQ_PRI,
		    qm_wdt_isr_0, 0, IOAPIC_EDGE | IOAPIC_HIGH);

	/* Unmask watchdog interrupt */
	irq_enable(QM_IRQ_WDT_0);

	/* Route watchdog interrupt to Lakemont */
	QM_SCSS_INT->int_watchdog_mask &= ~BIT(0);

	return 0;
}

DEVICE_AND_API_INIT(wdt, CONFIG_WDT_0_NAME, init, 0, 0,
		    PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&api);
