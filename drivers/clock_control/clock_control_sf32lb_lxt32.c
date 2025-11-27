/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_lxt32

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <register.h>

#define PMUC_LXT_CR               offsetof(PMUC_TypeDef, LXT_CR)
#define PMUC_LXT_CR_EN            PMUC_LXT_CR_EN_Msk
#define PMUC_LXT_READY_TIMEOUT_US 1000000U
#define PMUC_LXT_BM_VALUE         0x2U
#define PMUC_LXT_AMP_BM_VALUE     0x3U

struct sf32lb_lxt32_config {
	uint32_t rate;
	uintptr_t pmuc;
};

static int sf32lb_lxt32_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct sf32lb_lxt32_config *config = dev->config;
	uint32_t val;

	ARG_UNUSED(sys);

	/* Configure bias current and enable in a single write */
	val = sys_read32(config->pmuc + PMUC_LXT_CR);
	val &= ~(PMUC_LXT_CR_EN | PMUC_LXT_CR_RSN | PMUC_LXT_CR_CAP_SEL | PMUC_LXT_CR_BM_Msk |
		 PMUC_LXT_CR_AMP_BM_Msk);
	val |= FIELD_PREP(PMUC_LXT_CR_BM_Msk, PMUC_LXT_BM_VALUE) |
	       FIELD_PREP(PMUC_LXT_CR_AMP_BM_Msk, PMUC_LXT_AMP_BM_VALUE) | PMUC_LXT_CR_EN |
	       PMUC_LXT_CR_RSN;
	sys_write32(val, config->pmuc + PMUC_LXT_CR);

	while (sys_test_bit(config->pmuc + PMUC_LXT_CR, PMUC_LXT_CR_RDY_Pos) == 0U) {
	}

	return 0;
}

static int sf32lb_lxt32_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct sf32lb_lxt32_config *config = dev->config;

	ARG_UNUSED(sys);

	sys_clear_bits(config->pmuc + PMUC_LXT_CR, PMUC_LXT_CR_EN | PMUC_LXT_CR_RSN);

	return 0;
}

static enum clock_control_status sf32lb_lxt32_get_status(const struct device *dev,
							 clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);
	const struct sf32lb_lxt32_config *config = dev->config;

	if (sys_test_bit(config->pmuc + PMUC_LXT_CR, PMUC_LXT_CR_RDY_Pos) != 0U) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static int sf32lb_lxt32_get_rate(const struct device *dev, clock_control_subsys_t sys,
				 uint32_t *rate)
{
	ARG_UNUSED(sys);
	const struct sf32lb_lxt32_config *config = dev->config;

	*rate = config->rate;
	return 0;
}

static DEVICE_API(clock_control, sf32lb_lxt32_api) = {
	.on = sf32lb_lxt32_on,
	.off = sf32lb_lxt32_off,
	.get_status = sf32lb_lxt32_get_status,
	.get_rate = sf32lb_lxt32_get_rate,
};

static int sf32lb_lxt32_init(const struct device *dev)
{
	const struct sf32lb_lxt32_config *config = dev->config;

	return sf32lb_lxt32_on(dev, NULL);
}

#define SF32LB_LXT32_INIT(inst)                                                                    \
	static const struct sf32lb_lxt32_config sf32lb_lxt32_config_##inst = {                     \
		.rate = DT_INST_PROP(inst, clock_frequency),                                       \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                   \
		.pmuc = DT_REG_ADDR(DT_INST_PHANDLE(inst, sifli_pmuc)),                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, sf32lb_lxt32_init, NULL, NULL, &sf32lb_lxt32_config_##inst,    \
			      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                    \
			      &sf32lb_lxt32_api);

DT_INST_FOREACH_STATUS_OKAY(SF32LB_LXT32_INIT)
