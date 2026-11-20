/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_rw_pmu_wuc

#include <zephyr/device.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/kernel.h>

#include <fsl_power.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "RW6xx has one PMU wakeup controller");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_PARENT(DT_DRV_INST(0)), nxp_rw_pmu),
	     "The wakeup controller must be a child of the RW6xx PMU");

struct nxp_rw_pmu_wuc_data {
	struct k_spinlock lock;
};

/*
 * Consumer IDs are IRQn_Type values supplied by the SoC devicetree:
 * RTC_IRQn is 32 and PIN1_INT_IRQn is 101.
 * They are neither NVIC vector-table indices (IRQn + 16) nor bit
 * numbers in WAKEUP_MASK. Forward them unchanged to the Power driver,
 * which owns supported sources and their register mapping.
 */
static int nxp_rw_pmu_wuc_enable(const struct device *dev, uint32_t id)
{
	struct nxp_rw_pmu_wuc_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		POWER_EnableWakeup((IRQn_Type)id);
	}

	return 0;
}

static int nxp_rw_pmu_wuc_disable(const struct device *dev, uint32_t id)
{
	struct nxp_rw_pmu_wuc_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		POWER_DisableWakeup((IRQn_Type)id);
	}

	return 0;
}

static int nxp_rw_pmu_wuc_triggered(const struct device *dev, uint32_t id)
{
	ARG_UNUSED(dev);

	return POWER_GetWakeupStatus((IRQn_Type)id) ? 1 : 0;
}

static int nxp_rw_pmu_wuc_clear(const struct device *dev, uint32_t id)
{
	struct nxp_rw_pmu_wuc_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		POWER_ClearWakeupStatus((IRQn_Type)id);
	}

	return 0;
}

static DEVICE_API(wuc, nxp_rw_pmu_wuc_api) = {
	.enable = nxp_rw_pmu_wuc_enable,
	.disable = nxp_rw_pmu_wuc_disable,
	.triggered = nxp_rw_pmu_wuc_triggered,
	.clear = nxp_rw_pmu_wuc_clear,
};

static struct nxp_rw_pmu_wuc_data nxp_rw_pmu_wuc_data_0;

/*
 * No hardware initialization is needed. Initialize before peripheral
 * drivers and the PRE_KERNEL_2 SoC wakeup setup. All state is static;
 * no peripheral clocks are required.
 */
DEVICE_DT_INST_DEFINE(0, NULL, NULL, &nxp_rw_pmu_wuc_data_0, NULL, PRE_KERNEL_1,
		      CONFIG_WUC_INIT_PRIORITY, &nxp_rw_pmu_wuc_api);
