/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_power_controller

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bflb_power_controller, CONFIG_KERNEL_LOG_LEVEL);

#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#include <pds_reg.h>
#include <aon_reg.h>
#include <extra_defines.h>

#define E24_VALID_BO_THRES_0	2000000
#define E24_VALID_BO_THRES_1	2400000
#define E907_VALID_BO_THRES_MIN	2050000

struct power_bflb_config_s {
	uintptr_t base_hbn;
	uintptr_t base_aon;
	bool brown_out_reset;
	int bo_threshold;
};

static void power_bflb_isr(const struct device *dev)
{
	LOG_ERR("Unexpected Power Management Interrupt");
}

static void power_bflb_setup_irqs(void)
{
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, bor, irq), DT_INST_IRQ_BY_NAME(0, bor, priority),
		    power_bflb_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, bor, irq));
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, hbn0, irq), DT_INST_IRQ_BY_NAME(0, hbn0, priority),
		    power_bflb_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, hbn0, irq));
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, hbn1, irq), DT_INST_IRQ_BY_NAME(0, hbn1, priority),
		    power_bflb_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, hbn1, irq));
}

#ifdef CONFIG_SOC_SERIES_BL61X

static void power_bflb_setup_bor(const struct device *dev)
{
	const struct power_bflb_config_s *config = dev->config;
	uint32_t tmp;

	if (config->brown_out_reset) {
		/* disable BOD interrupt */
		tmp = sys_read32(config->base_hbn + HBN_IRQ_MODE_OFFSET);
		tmp &= ~HBN_IRQ_BOR_EN_MSK;
		sys_write32(tmp, config->base_hbn + HBN_IRQ_MODE_OFFSET);
	} else {
		tmp = sys_read32(config->base_hbn + HBN_IRQ_MODE_OFFSET);
		tmp |= HBN_IRQ_BOR_EN_MSK;
		sys_write32(tmp, config->base_hbn + HBN_IRQ_MODE_OFFSET);
	}

	tmp = sys_read32(config->base_hbn + HBN_BOR_CFG_OFFSET);
	if (config->brown_out_reset) {
		/* when brownout threshold, restart*/
		tmp |= HBN_BOD_SEL_MSK;
	} else {
		tmp &= ~HBN_BOD_SEL_MSK;
	}
	tmp &= ~HBN_BOD_VTH_MSK;
	tmp |= ((config->bo_threshold - 1U) << HBN_BOD_VTH_POS) & HBN_BOD_VTH_MSK;
	/* enable BOD */
	tmp |= HBN_PU_BOD_MSK;
	sys_write32(tmp, config->base_hbn + HBN_BOR_CFG_OFFSET);
}

#elif defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X)

#ifdef CONFIG_SOC_SERIES_BL70X
#define BOR_CFG_OFFSET HBN_MISC_OFFSET
#else
#define BOR_CFG_OFFSET HBN_BOR_CFG_OFFSET
#endif

static void power_bflb_setup_bor(const struct device *dev)
{
	const struct power_bflb_config_s *config = dev->config;
	uint32_t tmp;

	tmp = sys_read32(config->base_hbn + BOR_CFG_OFFSET);
	if (config->bo_threshold > 0) {
		/* 2400000 uV */
		tmp |= HBN_BOR_VTH_MSK;
	} else {
		/* 2000000 uV */
		tmp &= HBN_BOR_VTH_UMSK;
	}
	tmp &= HBN_BOR_SEL_UMSK;
	if (config->brown_out_reset) {
		tmp |= HBN_BOR_SEL_MSK;
	}
	/* enable BOR */
	tmp |= HBN_PU_BOR_MSK;
	sys_write32(tmp, config->base_hbn + BOR_CFG_OFFSET);

	tmp = sys_read32(config->base_hbn + HBN_IRQ_MODE_OFFSET);
	if (config->brown_out_reset) {
		tmp &= HBN_IRQ_BOR_EN_UMSK;
	} else {
		tmp |= HBN_IRQ_BOR_EN_MSK;
	}
	sys_write32(tmp, config->base_hbn + HBN_IRQ_MODE_OFFSET);
}

#else
#error Unknown Platform
#endif

static int power_bflb_init(const struct device *dev)
{
	power_bflb_setup_irqs();
	power_bflb_setup_bor(dev);

	return 0;
}

const struct power_bflb_config_s power_bflb_config = {
		.base_hbn = DT_INST_REG_ADDR_BY_NAME(0, hbn),
		.base_aon = DT_INST_REG_ADDR_BY_NAME(0, aon),
		.brown_out_reset = !DT_INST_PROP(0, brown_out_reset_disable),
		.bo_threshold = DT_INST_ENUM_IDX(0, brown_out_threshold_microvolt),
};

DEVICE_DT_INST_DEFINE(0, power_bflb_init, NULL, NULL, &power_bflb_config, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

#ifdef CONFIG_SOC_SERIES_BL61X
BUILD_ASSERT(DT_INST_PROP(0, brown_out_threshold_microvolt) > E907_VALID_BO_THRES_MIN,
	     "BL61x Brown Out threshold must be > 2050000 uV");
#elif defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X)
BUILD_ASSERT(DT_INST_PROP(0, brown_out_threshold_microvolt) == E24_VALID_BO_THRES_0
	|| DT_INST_PROP(0, brown_out_threshold_microvolt) == E24_VALID_BO_THRES_1,
	     "BL60x and BL70x only support 2.0v and 2.4v Brown Out thresholds");
#endif
