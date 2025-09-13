/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/fatal.h>
#include <zephyr/logging/log.h>
#include "soc.h"
#include "zephyr/sys/util_macro.h"
#include <zephyr/device.h>
#include <common/ctrl_partitions.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/tisci_clock_control.h>
LOG_MODULE_REGISTER(soc, LOG_LEVEL_DBG);

unsigned int z_soc_irq_get_active(void)
{
	return z_vim_irq_get_active();
}

void z_soc_irq_eoi(unsigned int irq)
{
	z_vim_irq_eoi(irq);
}

void z_soc_irq_init(void)
{
	z_vim_irq_init();
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	/* Configure interrupt type and priority */
	z_vim_irq_priority_set(irq, prio, flags);
}

void z_soc_irq_enable(unsigned int irq)
{
	/* Enable interrupt */
	z_vim_irq_enable(irq);
}

void z_soc_irq_disable(unsigned int irq)
{
	/* Disable interrupt */
	z_vim_irq_disable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	/* Check if interrupt is enabled */
	return z_vim_irq_is_enabled(irq);
}

void soc_early_init_hook(void)
{
	k3_unlock_all_ctrl_partitions();
}

#if defined(CONFIG_POWER_DOMAIN)
int soc_power_domain_init(void)
{
	LOG_INF("Starting power domain initialization\n");

	int error_count = 0;

#define CHECK_NODE_POWER_DOMAIN(child)                                                             \
	COND_CODE_1(DT_NODE_HAS_PROP(child, power_domains), ({		\
		const struct device *dev_##child =                                                 \
			DEVICE_DT_GET(DT_PHANDLE(child, power_domains));                   \
			LOG_INF("Turning on power domain: %s\n", dev_##child->name);               \
			int err = pm_device_runtime_get(dev_##child);                              \
			if (err < 0) {                                                             \
				LOG_INF("Failed to get power domain: %s\n", dev_##child->name);    \
				error_count++;                                                     \
			}                                                                          \
		}), (/* Do nothing */))

	DT_FOREACH_CHILD(DT_ROOT, CHECK_NODE_POWER_DOMAIN);

	return error_count;
}
SYS_INIT(soc_power_domain_init, POST_KERNEL, 0);
#endif /* CONFIG_POWER_DOMAIN  */

#if defined(CONFIG_CLOCK_CONTROL)
int soc_clock_init(void)
{
	LOG_INF("Starting clock initialization\n");

	int error_count = 0;

#define CHECK_NODE_CLOCK(child)                                                                    \
	COND_CODE_1(DT_NODE_HAS_PROP(child, clocks), \
		({ \
			const struct device *dev_##child = TISCI_GET_CLOCK(child); \
			struct tisci_clock_config req_##child = TISCI_GET_CLOCK_DETAILS(child); \
			uint64_t freq_##child = DT_PROP(child, clock_frequency);	\
			if (clock_control_set_rate(dev_##child, &req_##child, &freq_##child)) {	\
				printf("Failed to set clock rate for %s\n", dev_##child->name);	\
				error_count++; \
			} \
		}), \
		(/* Do nothing */) \
	)

	DT_FOREACH_CHILD(DT_ROOT, CHECK_NODE_CLOCK);

	return error_count;
}
SYS_INIT(soc_clock_init, POST_KERNEL, 0);
#endif /* CONFIG_CLOCK_CONTROL  */
