/*
 * Copyright (c) 2022 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT artery_at32_cctl

#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/at32_clock_control.h>

#include <at32_regs.h>
#include <at32_crm.h>
#include <at32_flash.h>
#include <at32_pwc.h>

/** offset (from id cell) */
#define AT32_CLOCK_ID_OFFSET(id) (((id) >> 6U) & 0xFFU)
/** configuration bit (from id cell) */
#define AT32_CLOCK_ID_BIT(id)    ((id)&0x1FU)

#define CPU_FREQ CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC

/** AHB prescaler exponents */
static const uint8_t ahb_exp[16] = {
	0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U,
};
/** APB1 prescaler exponents */
static const uint8_t apb1_exp[8] = {
	0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U,
};
/** APB2 prescaler exponents */
static const uint8_t apb2_exp[8] = {
	0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U,
};

struct clock_control_at32_config {
	uint32_t base;
};

#if defined(FLASH_WAIT_CYCLE)
static int clock_wait_cycle_config(void)
{
	if (CPU_FREQ > 192000000) {
		flash_psr_set(FLASH_WAIT_CYCLE_6);
	} else if (CPU_FREQ > 160000000) {
		flash_psr_set(FLASH_WAIT_CYCLE_5);
	} else if (CPU_FREQ > 128000000) {
		flash_psr_set(FLASH_WAIT_CYCLE_4);
	} else if (CPU_FREQ > 96000000) {
		flash_psr_set(FLASH_WAIT_CYCLE_3);
	} else if (CPU_FREQ > 64000000) {
		flash_psr_set(FLASH_WAIT_CYCLE_2);
	} else {
		flash_psr_set(FLASH_WAIT_CYCLE_1);
	}

	return 0;
}
#endif

static int clock_enable_hext(void)
{
	if (IS_ENABLED(AT32_HEXT_ENABLED)) {
		crm_clock_source_enable(CRM_CLOCK_SOURCE_HEXT, TRUE);

		/* wait till hext is ready */
		while (crm_hext_stable_wait() == ERROR) {
		}
	}

	return 0;
}

static int clock_enable_pll(void)
{
	if (IS_ENABLED(AT32_SYSCLK_SRC_PLL)) {
		/* config pll clock resource */
		if (IS_ENABLED(AT32_PLL_SRC_HEXT)) {
			crm_pll_config(CRM_PLL_SOURCE_HEXT, DT_PROP(DT_NODELABEL(pll), mul_ns),
				       DT_PROP(DT_NODELABEL(pll), div_ms),
				       DT_PROP(DT_NODELABEL(pll), div_fp));
		} else {
			crm_pll_config(CRM_PLL_SOURCE_HICK, DT_PROP(DT_NODELABEL(pll), mul_ns),
				       DT_PROP(DT_NODELABEL(pll), div_ms),
				       DT_PROP(DT_NODELABEL(pll), div_fp));
		}
#if defined(CRM_PLL_FU)
		crm_pllu_div_set(DT_PROP(DT_NODELABEL(pll), div_fu));
		crm_pllu_output_set(TRUE);
#endif
		/* enable pll */
		crm_clock_source_enable(CRM_CLOCK_SOURCE_PLL, TRUE);

		/* wait till pll is ready */
		while (crm_flag_get(CRM_PLL_STABLE_FLAG) != SET) {
		}
	}
	return 0;
}

static int clock_control_at32_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_at32_config *config = dev->config;
	uint16_t id = *(uint16_t *)sys;

	sys_set_bit(config->base + AT32_CLOCK_ID_OFFSET(id), AT32_CLOCK_ID_BIT(id));

	return 0;
}

static int clock_control_at32_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_at32_config *config = dev->config;
	uint16_t id = *(uint16_t *)sys;

	sys_clear_bit(config->base + AT32_CLOCK_ID_OFFSET(id), AT32_CLOCK_ID_BIT(id));

	return 0;
}

static int clock_control_at32_get_rate(const struct device *dev, clock_control_subsys_t sys,
				       uint32_t *rate)
{
	const struct clock_control_at32_config *config = dev->config;
	uint16_t id = *(uint16_t *)sys;
	uint32_t cfg;
	uint8_t psc;

	cfg = sys_read32(config->base + CRM_CFG_OFFSET);

	switch (AT32_CLOCK_ID_OFFSET(id)) {
	case CRM_AHB1EN_OFFSET:
	case CRM_AHB2EN_OFFSET:
	case CRM_AHB3EN_OFFSET:
		psc = (cfg & CRM_CFG_AHBDIV_MSK) >> CRM_CFG_AHBDIV_POS;
		*rate = CPU_FREQ >> ahb_exp[psc];
		break;
	case CRM_APB1EN_OFFSET:
		psc = (cfg & CRM_CFG_APB1DIV_MSK) >> CRM_CFG_APB1DIV_POS;
		*rate = CPU_FREQ >> apb1_exp[psc];
		break;
	case CRM_APB2EN_OFFSET:
		psc = (cfg & CRM_CFG_APB2DIV_MSK) >> CRM_CFG_APB2DIV_POS;
		*rate = CPU_FREQ >> apb2_exp[psc];
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static enum clock_control_status clock_control_at32_get_status(const struct device *dev,
							       clock_control_subsys_t sys)
{
	const struct clock_control_at32_config *config = dev->config;
	uint16_t id = *(uint16_t *)sys;

	if (sys_test_bit(config->base + AT32_CLOCK_ID_OFFSET(id), AT32_CLOCK_ID_BIT(id)) != 0) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, clock_control_at32_api) = {
	.on = clock_control_at32_on,
	.off = clock_control_at32_off,
	.get_rate = clock_control_at32_get_rate,
	.get_status = clock_control_at32_get_status,
};

/**
 * @brief Initialize clocks for the at32
 *
 * This routine is called to enable and configure the clocks and PLL
 * of the soc on the board. It depends on the board definition.
 * This function is called on the startup and also to restore the config
 * when exiting for low power mode.
 *
 * @param dev clock device struct
 *
 * @return 0
 */
int at32_clock_control_init(const struct device *dev)
{
	int clk_div = 0;
	/* set hick as system clock */
	crm_sysclk_switch(CRM_SCLK_HICK);

	/* wait till pll is used as system clock source */
	while (crm_sysclk_switch_status_get() != CRM_SCLK_HICK) {
	}

	crm_periph_clock_enable(CRM_PWC_PERIPH_CLOCK, TRUE);
#if defined(PWC_LDO_OUTPUT)
	pwc_ldo_output_voltage_set(PWC_LDO_OUTPUT_MAX);
#endif
	/* disable pll */
	crm_clock_source_enable(CRM_CLOCK_SOURCE_PLL, FALSE);
#if defined(FLASH_WAIT_CYCLE)
	clock_wait_cycle_config();
#endif
	/* config hext */
	clock_enable_hext();

	/* config pll */
	clock_enable_pll();

	/* config ahbclk */
	clk_div = DT_PROP(DT_NODELABEL(crm), ahb_prescaler);
	if (clk_div == 1) {
		clk_div = CRM_AHB_DIV_1;
	} else {
		clk_div = (clk_div - 2) + 8;
	}
	crm_ahb_div_set(clk_div);

	/* config apb2clk */
	clk_div = DT_PROP(DT_NODELABEL(crm), apb2_prescaler);
	if (clk_div == 1) {
		clk_div = CRM_APB2_DIV_1;
	} else {
		clk_div = (clk_div - 2) + 4;
	}
	crm_apb2_div_set(clk_div);

	/* config apb1clk */
	clk_div = DT_PROP(DT_NODELABEL(crm), apb1_prescaler);
	if (clk_div == 1) {
		clk_div = CRM_APB1_DIV_1;
	} else {
		clk_div = (clk_div - 2) + 4;
	}

	crm_apb1_div_set(clk_div);

	/* enable auto step mode */
	crm_auto_step_mode_enable(TRUE);

	if (IS_ENABLED(AT32_SYSCLK_SRC_PLL)) {
		/* select pll as system clock source */
		crm_sysclk_switch(CRM_SCLK_PLL);

		/* wait till pll is used as system clock source */
		while (crm_sysclk_switch_status_get() != CRM_SCLK_PLL) {
		}
	} else if (IS_ENABLED(AT32_SYSCLK_SRC_HEXT)) {
		/* select hext as system clock source */
		crm_sysclk_switch(CRM_SCLK_HEXT);

		/* wait till hext is used as system clock source */
		while (crm_sysclk_switch_status_get() != CRM_SCLK_HEXT) {
		}
	} else {
		/* select hick as system clock source */
		crm_sysclk_switch(CRM_SCLK_HICK);

		/* wait till hick is used as system clock source */
		while (crm_sysclk_switch_status_get() != CRM_SCLK_HICK) {
		}
	}

	/* disable auto step mode */
	crm_auto_step_mode_enable(FALSE);
	return 0;
}

static const struct clock_control_at32_config config = {
	.base = DT_REG_ADDR(DT_INST_PARENT(0)),
};

DEVICE_DT_INST_DEFINE(0, at32_clock_control_init, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_at32_api);
