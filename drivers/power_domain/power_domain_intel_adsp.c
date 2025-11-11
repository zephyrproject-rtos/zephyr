/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <adsp_shim.h>
#include <adsp_power.h>

#if CONFIG_SOC_INTEL_ACE15_MTPM
#include <adsp_power.h>
#endif /* CONFIG_SOC_INTEL_ACE15_MTPM */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(power_domain_intel_adsp, LOG_LEVEL_INF);

struct pg_bits {
	uint32_t SPA_bit;
	uint32_t CPA_bit;
};

#ifdef CONFIG_PM_DEVICE
static int pd_intel_adsp_set_power_enable(struct pg_bits *bits, bool power_enable)
{
	uint16_t SPA_bit_mask = BIT(bits->SPA_bit);

	if (power_enable) {
		sys_write16(sys_read16((mem_addr_t)ACE_PWRCTL) | SPA_bit_mask,
			    (mem_addr_t)ACE_PWRCTL);

		if (!WAIT_FOR(sys_read16((mem_addr_t)ACE_PWRSTS) & BIT(bits->CPA_bit),
		    10000, k_busy_wait(1))) {
			return -EIO;
		}
	} else {
#if CONFIG_SOC_INTEL_ACE15_MTPM
		extern uint32_t adsp_pending_buffer;

		if (bits->SPA_bit == INTEL_ADSP_HST_DOMAIN_BIT) {
			volatile uint32_t *key_read_ptr = &adsp_pending_buffer;
			uint32_t key_value = *key_read_ptr;

			if (key_value != INTEL_ADSP_ACE15_MAGIC_KEY) {
				return -EINVAL;
			}
		}
#endif
		sys_write16(sys_read16((mem_addr_t)ACE_PWRCTL) & ~(SPA_bit_mask),
			    (mem_addr_t)ACE_PWRCTL);
	}

	return 0;
}

static int pd_intel_adsp_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct pg_bits *reg_bits = (struct pg_bits *)dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pd_intel_adsp_set_power_enable(reg_bits, true);

		if (ret == 0) {
			pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_ON, NULL);
		}

		break;
	case PM_DEVICE_ACTION_SUSPEND:
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_OFF, NULL);
		ret = pd_intel_adsp_set_power_enable(reg_bits, false);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int pd_intel_adsp_init(const struct device *dev)
{
	pm_device_init_suspended(dev);
	return pm_device_runtime_enable(dev);
}

#define DT_DRV_COMPAT intel_adsp_power_domain

#define POWER_DOMAIN_DEVICE(id)								\
	static struct pg_bits pd_pg_reg##id = {						\
		.SPA_bit = DT_INST_PROP(id, bit_position),				\
		.CPA_bit = DT_INST_PROP(id, bit_position),				\
	};										\
	PM_DEVICE_DT_INST_DEFINE(id, pd_intel_adsp_pm_action);				\
	DEVICE_DT_INST_DEFINE(id, pd_intel_adsp_init, PM_DEVICE_DT_INST_GET(id),	\
			      &pd_pg_reg##id, NULL, POST_KERNEL,			\
			      CONFIG_POWER_DOMAIN_INTEL_ADSP_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(POWER_DOMAIN_DEVICE)
