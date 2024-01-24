/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <soc.h>

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memc_stm32, CONFIG_MEMC_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_fmc)
#define DT_DRV_COMPAT st_stm32_fmc
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_fmc)
#define DT_DRV_COMPAT st_stm32h7_fmc
#else
#error "No compatible FMC devicetree node found"
#endif

/* This symbol takes the value 1 if one of the device instances */
/* is configured in dts with a domain clock */
#if STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define STM32_FMC_DOMAIN_CLOCK_SUPPORT 1
#else
#define STM32_FMC_DOMAIN_CLOCK_SUPPORT 0
#endif

struct memc_stm32_config {
	uint32_t fmc;
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	const struct pinctrl_dev_config *pcfg;
};

static int memc_stm32_init(const struct device *dev)
{
	const struct memc_stm32_config *config = dev->config;

	int r;
	const struct device *clk;

	/* configure pinmux */
	r = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (r < 0) {
		LOG_ERR("FMC pinctrl setup failed (%d)", r);
		return r;
	}

	/* enable FMC peripheral clock */
	clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	r = clock_control_on(clk, (clock_control_subsys_t)&config->pclken[0]);
	if (r < 0) {
		LOG_ERR("Could not initialize FMC clock (%d)", r);
		return r;
	}

	if (IS_ENABLED(STM32_FMC_DOMAIN_CLOCK_SUPPORT) && (config->pclk_len > 1)) {
		/* Enable FMC clock source */
		r = clock_control_configure(clk, (clock_control_subsys_t)&config->pclken[1], NULL);
		if (r < 0) {
			LOG_ERR("Could not select FMC clock (%d)", r);
			return r;
		}
	}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_fmc)
#if (DT_ENUM_IDX(DT_DRV_INST(0), st_mem_swap) == 1)
	/* sdram-sram */
	MODIFY_REG(FMC_Bank1_R->BTCR[0], FMC_BCR1_BMAP, FMC_BCR1_BMAP_0);
#elif (DT_ENUM_IDX(DT_DRV_INST(0), st_mem_swap) == 2)
	/* sdramb2 */
	MODIFY_REG(FMC_Bank1_R->BTCR[0], FMC_BCR1_BMAP, FMC_BCR1_BMAP_1);
#endif
#endif

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);

static const struct stm32_pclken pclken[] = STM32_DT_INST_CLOCKS(0);

static const struct memc_stm32_config config = {
	.fmc = DT_INST_REG_ADDR(0),
	.pclken = pclken,
	.pclk_len = DT_INST_NUM_CLOCKS(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

DEVICE_DT_INST_DEFINE(0, memc_stm32_init, NULL, NULL,
	      &config, POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY, NULL);
