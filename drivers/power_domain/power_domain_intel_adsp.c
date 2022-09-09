/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(power_domain_intel_adsp, LOG_LEVEL_INF);

#define PWRCTL_OFFSET 0xD0
#define PWRSTS_OFFSET 0xD2

struct pg_registers {
	uint32_t wr_address;
	uint32_t rd_address;
	uint32_t SPA_bit;
	uint32_t CPA_bit;
};

static int pd_intel_adsp_set_power_enable(struct pg_registers *reg, bool power_enable)
{
	uint32_t SPA_bit_mask = BIT(reg->SPA_bit);

	if (power_enable) {
		sys_write32(sys_read32(reg->wr_address) | SPA_bit_mask, reg->wr_address);
	} else {
		sys_write32(sys_read32(reg->wr_address) & ~(SPA_bit_mask), reg->wr_address);
	}

	return 0;
}

static int pd_intel_adsp_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct pg_registers *reg_data = (struct pg_registers *)dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_ON, NULL);
		pd_intel_adsp_set_power_enable(reg_data, true);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_OFF, NULL);
		pd_intel_adsp_set_power_enable(reg_data, false);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
static int pd_intel_adsp_init(const struct device *dev)
{
	pm_device_init_suspended(dev);
	pm_device_runtime_enable(dev);
	return 0;
}

#define DT_DRV_COMPAT intel_adsp_power_domain

#define POWER_DOMAIN_DEVICE(id)                                                                    \
	static struct pg_registers pd_pg_reg##id = {                                               \
		.wr_address = DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(id), lps)) + PWRCTL_OFFSET,       \
		.rd_address = DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(id), lps)) + PWRSTS_OFFSET,       \
		.SPA_bit = DT_INST_PROP(id, bit_position),                                         \
		.CPA_bit = DT_INST_PROP(id, bit_position),                                         \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(id, pd_intel_adsp_pm_action);                                     \
	DEVICE_DT_INST_DEFINE(id, pd_intel_adsp_init, PM_DEVICE_DT_INST_GET(id),                   \
			      &pd_pg_reg##id, NULL, POST_KERNEL, 75, NULL);

DT_INST_FOREACH_STATUS_OKAY(POWER_DOMAIN_DEVICE)
