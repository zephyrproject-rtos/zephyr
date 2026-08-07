/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <ethosu_driver.h>

#include "ethos_u_common.h"

/* Infineon HAL for clock and power control */
#include "cy_sysclk.h"
#include "cy_syspm_ppu.h"
#include "cy_syspm_pdcm.h"
#include "ppu_v1.h"
#include "cy_device.h"

#define DT_DRV_COMPAT infineon_edge_npu

LOG_MODULE_REGISTER(ethos_u_infineon, CONFIG_ETHOS_U_LOG_LEVEL);

/* PSE84 U55 NPU Clock Configuration */
#define U55_PERI_NR   1u
#define U55_GROUP_NR  2u
#define U55_SLAVE_NR  0u
#define U55_CLK_HF_NR 1u

/* DT helpers: use fast-memory-region if present; else NULL/0 */
#define ETHOSU_FAST_BASE(n)                                                                        \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, fast_memory_region),                                 \
		((void *)DT_REG_ADDR(DT_INST_PHANDLE(n, fast_memory_region))),                    \
		(NULL))

#define ETHOSU_FAST_SIZE(n)                                                                        \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, fast_memory_region),                                 \
		((size_t)DT_REG_SIZE(DT_INST_PHANDLE(n, fast_memory_region))),                    \
		(0u))

static void ethosu_infineon_irq_handler(const struct device *dev)
{
	struct ethosu_data *data = dev->data;
	struct ethosu_driver *drv = &data->drv;

	ethosu_irq_handler(drv);
}

/**
 * @brief Enable U55 NPU peripheral clock
 *
 * Initializes the peripheral clock for the U55 NPU using the Infineon HAL.
 * This enables the SCTL (Slave Clock Control) for the NPU peripheral.
 *
 * @return 0 on success, negative errno code on failure
 */
static int infineon_npu_clock_enable(void)
{
	LOG_DBG("Enabling U55 NPU peripheral clock (PERI=%u, GROUP=%u, SLAVE=%u, CLK_HF=%u)",
		U55_PERI_NR, U55_GROUP_NR, U55_SLAVE_NR, U55_CLK_HF_NR);

	/* Cy_SysClk_PeriGroupSlaveInit returns void - no error checking possible */
	Cy_SysClk_PeriGroupSlaveInit(U55_PERI_NR, U55_GROUP_NR, U55_SLAVE_NR, U55_CLK_HF_NR);

	LOG_DBG("U55 NPU clock enabled successfully");
	return 0;
}

/**
 * @brief Enable U55 NPU power via PPU
 *
 * Powers up the U55 NPU using the Power Policy Unit (PPU).
 * Sets the PPU to ON mode to enable the NPU hardware.
 *
 * @return 0 on success, negative errno code on failure
 */
static int infineon_npu_ppu_enable(void)
{
	cy_en_syspm_status_t status;

	LOG_DBG("Powering up U55 NPU via PPU (base=0x%08x)", CY_PPU_U55_BASE);

	status = cy_pd_ppu_set_power_mode((struct ppu_v1_reg *)CY_PPU_U55_BASE,
					  (uint32_t)PPU_V1_MODE_ON);

	if (status != CY_SYSPM_SUCCESS) {
		LOG_ERR("Failed to power up U55 PPU: %d", status);
		return -EIO;
	}

	LOG_DBG("U55 NPU PPU enabled successfully");
	return 0;
}

/**
 * @brief Set PDCM dependency for U55 NPU
 *
 * Configures the Power Dependency Control Matrix (PDCM) to ensure the U55
 * NPU remains powered as long as the current CPU is active.
 *
 * @return 0 on success, negative errno code on failure
 */
static int infineon_npu_pdcm_enable(void)
{
	cy_en_syspm_status_t status;

	LOG_DBG("Setting PDCM dependency: U55 depends on CPU PD %d", CY_PD_PDCM_APPCPU);

	status = cy_pd_pdcm_set_dependency(CY_PD_PDCM_U55, CY_PD_PDCM_APPCPU);

	if (status != CY_SYSPM_SUCCESS) {
		LOG_ERR("Failed to set PDCM dependency: %d", status);
		return -EIO;
	}

	LOG_DBG("U55 NPU PDCM dependency set successfully");
	return 0;
}

/**
 * @brief Enable Infineon PSE84 U55 NPU hardware
 *
 * This function performs the vendor-specific hardware initialization required
 * for the Infineon PSE84 before calling ethosu_init(). This is equivalent to
 * the ModusToolbox Cy_SysU55Enable() function.
 *
 * Hardware initialization steps:
 * 1. Enable peripheral clock via SCTL (Slave Clock Control)
 * 2. Configure PPU (Power Policy Unit) to active mode
 * 3. Set PDCM (Power Dependency Control Matrix) to maintain NPU power
 *
 * @return 0 on success, negative errno code on failure
 */
static int infineon_npu_hardware_enable(void)
{
	int ret;

	LOG_DBG("Enabling PSE84 NPU hardware");

	/* Step 1: Enable peripheral clock */
	ret = infineon_npu_clock_enable();
	if (ret < 0) {
		return ret;
	}

	/* Step 2: Power up NPU via PPU */
	ret = infineon_npu_ppu_enable();
	if (ret < 0) {
		return ret;
	}

	/* Step 3: Set PDCM dependency */
	ret = infineon_npu_pdcm_enable();
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("PSE84 NPU hardware enabled successfully");
	return 0;
}

static int ethosu_infineon_init(const struct device *dev)
{
	const struct ethosu_dts_info *config = dev->config;
	struct ethosu_data *data = dev->data;
	struct ethosu_driver *drv = &data->drv;
	struct ethosu_driver_version version;
	int ret;

	LOG_DBG("Ethos-U DTS info: base_addr=%p, fast_mem=%p, fast_size=%zu, "
		"secure_enable=%u, privilege_enable=%u",
		config->base_addr, config->fast_mem_base, config->fast_mem_size,
		config->secure_enable, config->privilege_enable);

	/* Infineon PSE84-specific: Enable NPU hardware before accessing registers.
	 * This performs clock enable, PPU power-up, and PDCM configuration.
	 */
	ret = infineon_npu_hardware_enable();
	if (ret < 0) {
		LOG_ERR("Failed to enable NPU hardware: %d", ret);
		return ret;
	}

	ethosu_get_driver_version(&version);
	LOG_DBG("Ethos-U Driver version: %u.%u.%u", version.major, version.minor, version.patch);

	if (ethosu_init(drv, config->base_addr, config->fast_mem_base, config->fast_mem_size,
			config->secure_enable, config->privilege_enable)) {
		LOG_ERR("Failed to initialize NPU with ethosu_init()");
		return -EINVAL;
	}

	config->irq_config();

	LOG_INF("Ethos-U NPU initialized successfully");

	return 0;
}

#define INFINEON_ETHOS_U_INIT(n)                                                                   \
	static struct ethosu_data ethosu_infineon_data_##n;                                        \
                                                                                                   \
	static void ethosu_infineon_irq_config_##n(void)                                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),                             \
			    ethosu_infineon_irq_handler, DEVICE_DT_INST_GET(n), 0);                \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct ethosu_dts_info ethosu_infineon_dts_info_##n = {                       \
		.base_addr = (void *)DT_INST_REG_ADDR(n),                                          \
		.secure_enable = DT_INST_PROP(n, secure_enable),                                   \
		.privilege_enable = DT_INST_PROP(n, privilege_enable),                             \
		.irq_config = &ethosu_infineon_irq_config_##n,                                     \
		.fast_mem_base = ETHOSU_FAST_BASE(n),                                              \
		.fast_mem_size = ETHOSU_FAST_SIZE(n),                                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ethosu_infineon_init, NULL, &ethosu_infineon_data_##n,            \
			      &ethosu_infineon_dts_info_##n, POST_KERNEL,                          \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_ETHOS_U_INIT);
