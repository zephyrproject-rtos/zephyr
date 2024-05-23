/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_pcc

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/npcm_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_npcm, LOG_LEVEL_ERR);

/* Driver config */
struct npcm_pcc_config {
	/* cdcg device base address */
	uintptr_t base_cdcg;
	/* pmc device base address */
	uintptr_t base_pmc;
};

/* Driver convenience defines */
#define DRV_CONFIG(dev) \
	((const struct npcm_pcc_config *)(dev)->config)

#define HAL_CDCG_INST(dev) \
	(struct cdcg_reg *)(DRV_CONFIG(dev)->base_cdcg)

/* Clock controller local functions */
static inline int npcm_clock_control_on(const struct device *dev,
					 clock_control_subsys_t sub_system)
{
	ARG_UNUSED(dev);
	struct npcm_clk_cfg *clk_cfg = (struct npcm_clk_cfg *)(sub_system);
	const uint32_t pmc_base = DRV_CONFIG(dev)->base_pmc;

	if (clk_cfg->ctrl >= NPCM_PWDWN_CTL_COUNT)
		return -EINVAL;

	/* Clear related PD (Power-Down) bit of module to turn on clock */
	NPCM_PWDWN_CTL(pmc_base, clk_cfg->ctrl) &= ~(BIT(clk_cfg->bit));
	return 0;
}

static inline int npcm_clock_control_off(const struct device *dev,
					  clock_control_subsys_t sub_system)
{
	ARG_UNUSED(dev);
	struct npcm_clk_cfg *clk_cfg = (struct npcm_clk_cfg *)(sub_system);
	const uint32_t pmc_base = DRV_CONFIG(dev)->base_pmc;

	if (clk_cfg->ctrl >= NPCM_PWDWN_CTL_COUNT) {
		return -EINVAL;
	}

	/* Set related PD (Power-Down) bit of module to turn off clock */
	NPCM_PWDWN_CTL(pmc_base, clk_cfg->ctrl) |= BIT(clk_cfg->bit);
	return 0;
}

static int npcm_clock_control_get_subsys_rate(const struct device *dev,
					      clock_control_subsys_t sub_system,
					      uint32_t *rate)
{
	ARG_UNUSED(dev);
	struct npcm_clk_cfg *clk_cfg = (struct npcm_clk_cfg *)(sub_system);

	switch (clk_cfg->bus) {
	case NPCM_CLOCK_BUS_APB1:
		*rate = NPCM_APB_CLOCK(1);
		break;
	case NPCM_CLOCK_BUS_APB2:
		*rate = NPCM_APB_CLOCK(2);
		break;
	case NPCM_CLOCK_BUS_APB3:
		*rate = NPCM_APB_CLOCK(3);
		break;
	case NPCM_CLOCK_BUS_AHB6:
		*rate = CORE_CLK/(AHB6DIV_VAL + 1);
		break;
	case NPCM_CLOCK_BUS_FIU:
		*rate = CORE_CLK/(FIUDIV_VAL + 1);
		break;
	case NPCM_CLOCK_BUS_CORE:
		*rate = CORE_CLK;
		break;
	case NPCM_CLOCK_BUS_LFCLK:
		*rate = LFCLK;
		break;
	case NPCM_CLOCK_BUS_FMCLK:
		*rate = FMCLK;
		break;
	default:
		*rate = 0U;
		/* Invalid parameters */
		return -EINVAL;
	}

	return 0;
}

/* Clock controller driver registration */
static struct clock_control_driver_api npcm_clock_control_api = {
	.on = npcm_clock_control_on,
	.off = npcm_clock_control_off,
	.get_rate = npcm_clock_control_get_subsys_rate,
};

static int npcm_clock_control_init(const struct device *dev)
{
	struct cdcg_reg *const inst_cdcg = HAL_CDCG_INST(dev);

	/*
	 * Resetting the OFMCLK (even to the same value) will make the clock
	 * unstable for a little which can affect peripheral communication like
	 * eSPI. Skip this if not needed.
	 */
	if (inst_cdcg->HFCGN != HFCGN_VAL || inst_cdcg->HFCGML != HFCGML_VAL
			|| inst_cdcg->HFCGMH != HFCGMH_VAL) {
		/*
		 * Configure frequency multiplier M/N values according to
		 * the requested OFMCLK (Unit:Hz).
		 */
		inst_cdcg->HFCGN  = HFCGN_VAL;
		inst_cdcg->HFCGML = HFCGML_VAL;
		inst_cdcg->HFCGMH = HFCGMH_VAL;

		/* Load M and N values into the frequency multiplier */
		inst_cdcg->HFCGCTRL |= BIT(NPCM_HFCGCTRL_LOAD);
		/* Wait for stable */
		while (IS_BIT_SET(inst_cdcg->HFCGCTRL, NPCM_HFCGCTRL_CLK_CHNG))
			;
	}

	/* Set all clock prescalers of core and peripherals. */
	inst_cdcg->HFCGP = ((FPRED_VAL << 4) | AHB6DIV_VAL);
	inst_cdcg->HFCBCD = (APB1DIV_VAL | (APB2DIV_VAL << 4));
	inst_cdcg->HFCBCD1 = FIUDIV_VAL;
	inst_cdcg->HFCBCD2 = APB3DIV_VAL;

	return 0;
}

const struct npcm_pcc_config pcc_config = {
	.base_cdcg = DT_INST_REG_ADDR_BY_NAME(0, cdcg),
	.base_pmc  = DT_INST_REG_ADDR_BY_NAME(0, pmc),
};

DEVICE_DT_INST_DEFINE(0,
		    &npcm_clock_control_init,
		    NULL,
		    NULL, &pcc_config,
		    PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,
		    &npcm_clock_control_api);
