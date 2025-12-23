/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <ethosu_driver.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <soc.h>

#include "ethos_u_common.h"

#define DT_DRV_COMPAT renesas_ra_npu
LOG_MODULE_REGISTER(renesas_ra_npu, CONFIG_ETHOS_U_LOG_LEVEL);

struct ethos_u_renesas_config {
	const struct ethosu_dts_info ethosu_dts_info;
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_subsys;
};

void ethos_u_renesas_ra_irq_handler(const struct device *dev)
{
	struct ethosu_data *data = dev->data;
	struct ethosu_driver *drv = &data->drv;
	IRQn_Type irq = R_FSP_CurrentIrqGet();

	ethosu_irq_handler(drv);

	R_BSP_IrqStatusClear(irq);
}

static int ethos_u_renesas_ra_init(const struct device *dev)
{
	const struct ethos_u_renesas_config *config = dev->config;
	const struct ethosu_dts_info ethosu_dts_info = config->ethosu_dts_info;
	struct ethosu_data *data = dev->data;
	struct ethosu_driver *drv = &data->drv;
	struct ethosu_driver_version version;
	int err;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_subsys);
	if (err < 0) {
		LOG_ERR("Could not initialize clock (%d)", err);
		return err;
	}

	if ((((0 == R_SYSTEM->PGCSAR_b.NONSEC2) && FSP_PRIV_TZ_USE_SECURE_REGS) ||
	     ((1 == R_SYSTEM->PGCSAR_b.NONSEC2) && BSP_TZ_NONSECURE_BUILD)) &&
	    (0 != R_SYSTEM->PDCTRNPU)) {
		/* Turn on NPU power domain */
		R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_OM_LPC_BATT);
		FSP_HARDWARE_REGISTER_WAIT((R_SYSTEM->PDCTRNPU & (R_SYSTEM_PDCTRNPU_PDCSF_Msk |
								  R_SYSTEM_PDCTRGD_PDPGSF_Msk)),
					   R_SYSTEM_PDCTRGD_PDPGSF_Msk);
		R_SYSTEM->PDCTRNPU = 0;
		FSP_HARDWARE_REGISTER_WAIT((R_SYSTEM->PDCTRNPU & (R_SYSTEM_PDCTRNPU_PDCSF_Msk |
								  R_SYSTEM_PDCTRGD_PDPGSF_Msk)),
					   0);
		R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_OM_LPC_BATT);
	}

	LOG_DBG("Ethos-U DTS info. base_address=0x%p, secure_enable=%u, privilege_enable=%u",
		ethosu_dts_info.base_addr, ethosu_dts_info.secure_enable,
		ethosu_dts_info.privilege_enable);

	ethosu_get_driver_version(&version);

	LOG_DBG("Version. major=%u, minor=%u, patch=%u", version.major, version.minor,
		version.patch);

	if (ethosu_init(drv, ethosu_dts_info.base_addr, NULL, 0, ethosu_dts_info.secure_enable,
			ethosu_dts_info.privilege_enable)) {
		LOG_ERR("Failed to initialize NPU with ethosu_init().");
		return -EINVAL;
	}

	ethosu_dts_info.irq_config();

	return 0;
}

#define ETHOSU_RENESAS_RA_DEVICE_INIT(idx)                                                         \
	static struct ethosu_data ethosu_data_##idx;                                               \
                                                                                                   \
	static void ethosu_zephyr_irq_config_##idx(void)                                           \
	{                                                                                          \
		R_ICU->IELSR_b[DT_INST_IRQ_BY_NAME(idx, npu_irq, irq)].IELS =                      \
			BSP_PRV_IELS_ENUM(EVENT_NPU_IRQ);                                          \
                                                                                                   \
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(BSP_PRV_IELS_ENUM(EVENT_NPU_IRQ));                \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority),                         \
			    ethos_u_renesas_ra_irq_handler, DEVICE_DT_INST_GET(idx), 0);           \
                                                                                                   \
		irq_enable(DT_INST_IRQN(idx));                                                     \
	}                                                                                          \
                                                                                                   \
	static const struct ethos_u_renesas_config ethos_u_renesas_config##idx = {                 \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_IDX(idx, 0, mstp),        \
				.stop_bit = DT_INST_CLOCKS_CELL_BY_IDX(idx, 0, stop_bit),          \
			},                                                                         \
		.ethosu_dts_info =                                                                 \
			{                                                                          \
				.base_addr = (void *)DT_INST_REG_ADDR(idx),                        \
				.secure_enable = DT_INST_PROP(idx, secure_enable),                 \
				.privilege_enable = DT_INST_PROP(idx, privilege_enable),           \
				.irq_config = &ethosu_zephyr_irq_config_##idx,                     \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, ethos_u_renesas_ra_init, NULL, &ethosu_data_##idx,              \
			      &ethos_u_renesas_config##idx, POST_KERNEL,                           \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(ETHOSU_RENESAS_RA_DEVICE_INIT);
