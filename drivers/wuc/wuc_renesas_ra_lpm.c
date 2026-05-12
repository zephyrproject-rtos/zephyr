/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_lpm_wuc

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include "wuc_renesas_ra_lpm_priv.h"

#include <zephyr/dt-bindings/wuc/wuc_renesas_ra_lpm.h>

LOG_MODULE_REGISTER(wuc_renesas_ra_lpm, CONFIG_WUC_LOG_LEVEL);

/* Maximum flat bit index across all DPSIER registers (6 regs × 8 bits) */
#define RA_WUC_MAX_BIT 48U

lpm_deep_standby_cancel_source_bits_t ra_wuc_get_cancel_source(const struct device *dev)
{
	struct ra_wuc_data *data = dev->data;
	lpm_deep_standby_cancel_source_bits_t src;

	K_SPINLOCK(&data->lock) {
		src = data->cancel_source;
	}
	return src;
}

lpm_deep_standby_cancel_edge_bits_t ra_wuc_get_cancel_edge(const struct device *dev)
{
	struct ra_wuc_data *data = dev->data;
	lpm_deep_standby_cancel_edge_bits_t edge;

	K_SPINLOCK(&data->lock) {
		edge = data->cancel_edge;
	}
	return edge;
}

static int ra_wuc_enable(const struct device *dev, uint32_t id)
{
	struct ra_wuc_data *data = dev->data;
	uint8_t bit  = RA_WUC_DPSS_GET_BIT(id);
	uint8_t edge = RA_WUC_DPSS_GET_EDGE(id);

	if (bit >= RA_WUC_MAX_BIT) {
		LOG_ERR("Invalid wakeup source id: %u (bit %u >= %u)", id, bit, RA_WUC_MAX_BIT);
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->cancel_source |= BIT64(bit);
		if (edge) {
			data->cancel_edge |= BIT64(bit);
		} else {
			data->cancel_edge &= ~BIT64(bit);
		}
	}

	LOG_DBG("Enabled deep standby source bit=%u edge=%u", bit, edge);
	return 0;
}

static int ra_wuc_disable(const struct device *dev, uint32_t id)
{
	struct ra_wuc_data *data = dev->data;
	uint8_t bit = RA_WUC_DPSS_GET_BIT(id);

	if (bit >= RA_WUC_MAX_BIT) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->cancel_source &= ~BIT64(bit);
		data->cancel_edge   &= ~BIT64(bit);
	}

	LOG_DBG("Disabled deep standby source bit=%u", bit);
	return 0;
}

static int ra_wuc_triggered(const struct device *dev, uint32_t id)
{
	struct ra_wuc_data *data = dev->data;
	uint8_t bit = RA_WUC_DPSS_GET_BIT(id);
	int ret = 0;

	if (bit >= RA_WUC_MAX_BIT) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		if (data->triggered_snapshot & BIT64(bit)) {
			ret = 1;
		}
	}
	return ret;
}

static int ra_wuc_clear(const struct device *dev, uint32_t id)
{
	struct ra_wuc_data *data = dev->data;
	uint8_t bit = RA_WUC_DPSS_GET_BIT(id);

	if (bit >= RA_WUC_MAX_BIT) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->triggered_snapshot &= ~BIT64(bit);
	}
	return 0;
}

static int ra_wuc_init(const struct device *dev)
{
	struct ra_wuc_data *data = dev->data;
	uint64_t snapshot = 0;

	/*
	 * Snapshot DPSIFR (Deep Software Standby Interrupt Flag) registers at boot.
	 *
	 * These registers are NOT reset by the Deep Software Standby cancel
	 * reset (per hardware manual), so they retain the wakeup cause.
	 *
	 * After the snapshot, clear the flags so that if the system boots
	 * for any reason other than deep standby wakeup, stale flags are
	 * not reported.
	 */
	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_OM_LPC_BATT);

	/* DPSIFR0–3: present on all RA8 variants */
	snapshot |= (uint64_t)R_SYSTEM->DPSIFR0;
	R_SYSTEM->DPSIFR0 = 0U;

	snapshot |= (uint64_t)R_SYSTEM->DPSIFR1 << 8U;
	R_SYSTEM->DPSIFR1 = 0U;

	snapshot |= (uint64_t)R_SYSTEM->DPSIFR2 << 16U;
	R_SYSTEM->DPSIFR2 = 0U;

	snapshot |= (uint64_t)R_SYSTEM->DPSIFR3 << 24U;
	R_SYSTEM->DPSIFR3 = 0U;

#if BSP_FEATURE_LPM_HAS_DPSIER4
	/* DPSIFR4–5: IRQ16–31, RA8P1 and similar */
	snapshot |= (uint64_t)R_SYSTEM->DPSIFR4 << 32U;
	R_SYSTEM->DPSIFR4 = 0U;
#endif

#if BSP_FEATURE_LPM_HAS_DPSIER5
	snapshot |= (uint64_t)R_SYSTEM->DPSIFR5 << 40U;
	R_SYSTEM->DPSIFR5 = 0U;
#endif

	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_OM_LPC_BATT);

	K_SPINLOCK(&data->lock) {
		data->triggered_snapshot = snapshot;
	}

	if (snapshot) {
		LOG_INF("Booted from Deep Software Standby, wakeup flags=0x%llx", snapshot);
	}

	return 0;
}

static struct ra_wuc_data wuc_data;

static const DEVICE_API(wuc, ra_wuc_api) = {
	.enable    = ra_wuc_enable,
	.disable   = ra_wuc_disable,
	.triggered = ra_wuc_triggered,
	.clear     = ra_wuc_clear,
};

DEVICE_DT_INST_DEFINE(0, ra_wuc_init, NULL, &wuc_data, NULL,
		      POST_KERNEL, CONFIG_WUC_INIT_PRIORITY, &ra_wuc_api);
