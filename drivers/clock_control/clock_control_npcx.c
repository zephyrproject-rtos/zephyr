/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_pcc

#include <soc.h>
#include <drivers/clock_control.h>
#include <dt-bindings/clock/npcx_clock.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(clock_control_npcx, LOG_LEVEL_ERR);

/* Driver config */
struct npcx_pcc_config {
	/* cdcg device base address */
	uint32_t base_cdcg;
	/* pmc device base address */
	uint32_t base_pmc;
};

/* Driver convenience defines */
#define DRV_CONFIG(dev) \
	((const struct npcx_pcc_config *)(dev)->config)

#define HAL_CDCG_INST(dev) \
	(struct cdcg_reg_t *)(DRV_CONFIG(dev)->base_cdcg)

#define HAL_PMC_INST(dev) \
	(struct pmc_reg_t *)(DRV_CONFIG(dev)->base_pmc)

/* Clock controller local functions */
static inline int npcx_clock_control_on(struct device *dev,
					 clock_control_subsys_t sub_system)
{
	ARG_UNUSED(dev);
	struct npcx_clk_cfg *clk_cfg = (struct npcx_clk_cfg *)(sub_system);
	uint32_t pmc_base = DRV_CONFIG(dev)->base_pmc;

	/* Clear related PD (Power-Down) bit of module to turn on clock */
	NPCX_PWDWN_CTL(pmc_base, clk_cfg->ctrl) &= ~(BIT(clk_cfg->bit));
	return 0;
}

static inline int npcx_clock_control_off(struct device *dev,
					  clock_control_subsys_t sub_system)
{
	ARG_UNUSED(dev);
	struct npcx_clk_cfg *clk_cfg = (struct npcx_clk_cfg *)(sub_system);
	uint32_t pmc_base = DRV_CONFIG(dev)->base_pmc;

	/* Set related PD (Power-Down) bit of module to turn off clock */
	NPCX_PWDWN_CTL(pmc_base, clk_cfg->ctrl) |= BIT(clk_cfg->bit);
	return 0;
}

static int npcx_clock_control_get_subsys_rate(struct device *dev,
					 clock_control_subsys_t sub_system,
					 uint32_t *rate)
{
	ARG_UNUSED(dev);
	struct npcx_clk_cfg *clk_cfg = (struct npcx_clk_cfg *)(sub_system);

	switch (clk_cfg->bus) {
	case NPCX_CLOCK_BUS_APB1:
		*rate = NPCX_APB_CLOCK(1);
		break;
	case NPCX_CLOCK_BUS_APB2:
		*rate = NPCX_APB_CLOCK(2);
		break;
	case NPCX_CLOCK_BUS_APB3:
		*rate = NPCX_APB_CLOCK(3);
		break;
	case NPCX_CLOCK_BUS_AHB6:
		*rate = CORE_CLK/(AHB6DIV_VAL + 1);
		break;
	case NPCX_CLOCK_BUS_FIU:
		*rate = CORE_CLK/(FIUDIV_VAL + 1);
		break;
	case NPCX_CLOCK_BUS_CORE:
		*rate = CORE_CLK;
		break;
	case NPCX_CLOCK_BUS_LFCLK:
		*rate = LFCLK;
		break;
	default:
		*rate = 0U;
		/* Invalid parameters */
		return -EINVAL;
	};

	return 0;
}

/* Clock controller driver registration */
static struct clock_control_driver_api npcx_clock_control_api = {
	.on = npcx_clock_control_on,
	.off = npcx_clock_control_off,
	.get_rate = npcx_clock_control_get_subsys_rate,
};

static int npcx_clock_control_init(struct device *dev)
{
	struct cdcg_reg_t *inst_cdcg = HAL_CDCG_INST(dev);
	uint32_t pmc_base = DRV_CONFIG(dev)->base_pmc;

	/*
	 * Resetting the OSC_CLK (even to the same value) will make the clock
	 * unstable for a little which can affect peripheral communication like
	 * eSPI. Skip this if not needed.
	 */
	if (inst_cdcg->HFCGN != HFCGN_VAL || inst_cdcg->HFCGML != HFCGML_VAL
			|| inst_cdcg->HFCGMH != HFCGMH_VAL) {
		/*
		 * Configure frequency multiplier M/N values according to
		 * the requested OSC_CLK (Unit:Hz).
		 */
		inst_cdcg->HFCGN  = HFCGN_VAL;
		inst_cdcg->HFCGML = HFCGML_VAL;
		inst_cdcg->HFCGMH = HFCGMH_VAL;

		/* Load M and N values into the frequency multiplier */
		inst_cdcg->HFCGCTRL |= BIT(NPCX_HFCGCTRL_LOAD);
		/* Wait for stable */
		while (IS_BIT_SET(inst_cdcg->HFCGCTRL, NPCX_HFCGCTRL_CLK_CHNG))
			;
	}

	/* Set all clock prescalers of core and peripherals. */
	inst_cdcg->HFCGP   = ((FPRED_VAL << 4) | AHB6DIV_VAL);
	inst_cdcg->HFCBCD  = (FIUDIV_VAL << 4);
	inst_cdcg->HFCBCD1 = (APB1DIV_VAL | (APB2DIV_VAL << 4));
	inst_cdcg->HFCBCD2 = APB3DIV_VAL;

	/*
	 * Power-down (turn off clock) the modules initially for better
	 * power consumption.
	 */
	NPCX_PWDWN_CTL(pmc_base, NPCX_PWDWN_CTL1) = 0xF9; /* No SDP_PD/FIU_PD */
	NPCX_PWDWN_CTL(pmc_base, NPCX_PWDWN_CTL2) = 0xFF;
	NPCX_PWDWN_CTL(pmc_base, NPCX_PWDWN_CTL3) = 0x1F; /* No GDMA_PD */
	NPCX_PWDWN_CTL(pmc_base, NPCX_PWDWN_CTL4) = 0xFF;
	NPCX_PWDWN_CTL(pmc_base, NPCX_PWDWN_CTL5) = 0xFA;
	NPCX_PWDWN_CTL(pmc_base, NPCX_PWDWN_CTL6) = 0xFF;
	NPCX_PWDWN_CTL(pmc_base, NPCX_PWDWN_CTL7) = 0xE7;

	return 0;
}

const struct npcx_pcc_config pcc_config = {
	.base_cdcg = DT_INST_REG_ADDR_BY_NAME(0, cdcg),
	.base_pmc  = DT_INST_REG_ADDR_BY_NAME(0, pmc),
};

DEVICE_AND_API_INIT(npcx_cdcg, NPCX_CLOCK_CONTROL_NAME,
		    &npcx_clock_control_init,
		    NULL, &pcc_config,
		    PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,
		    &npcx_clock_control_api);
