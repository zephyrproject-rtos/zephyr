/*
 * Copyright (c) 2026 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TODO: move HXTInit from system_M480.c to here using pinctrl or similar.
 */

#define DT_DRV_COMPAT nuvoton_numicro_m4_scc

#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numicro.h>
#include <zephyr/dt-bindings/clock/numicro_m48x_clock.h>
#include <zephyr/logging/log.h>
#include <soc.h>

#define DT_FREQ_K(x) ((x) * 1000)
#define DT_FREQ_M(x) (DT_FREQ_K(x) * 1000)

LOG_MODULE_REGISTER(clock_control_numicro_m4_scc, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct numicro_scc_config {
	CLK_T *regs;
	uint32_t pll_fin;
	uint8_t pllsrc;
	uint8_t hclkdiv;
	uint8_t hclksel;
	uint8_t pclk0_div;
	uint8_t pclk1_div;
};

struct numicro_scc_data {
	uint32_t pll_freq;
	uint32_t hclk_freq;
	uint8_t fmc_cycle;
};

/* similar to BSP CLK_EnableModuleClock but with regs from dev */
static void numicro_pcc_module_enable(const struct device *dev, uint32_t module_id, bool enable)
{
	const struct numicro_scc_config *const config = dev->config;
	uint32_t addr = (uint32_t)&config->regs->AHBCLK;
	uint32_t pos = NUMICRO_MODULE_IP_EN_Pos(module_id);

	addr += NUMICRO_MODULE_APBCLK(module_id) * 4;

	if (enable) {
		sys_set_bit(addr, pos);
	} else {
		sys_clear_bit(addr, pos);
	}
}

static int numicro_scc_on(const struct device *dev, clock_control_subsys_t subsys)
{
	struct numicro_scc_subsys *scc_subsys = (struct numicro_scc_subsys *)subsys;

	if (scc_subsys->subsys_id != NUMICRO_SCC_SUBSYS_ID_PCC) {
		return -EINVAL;
	}
	SYS_UnlockReg();
	numicro_pcc_module_enable(dev, scc_subsys->pcc.clk_mod, true);
	SYS_LockReg();

	return 0;
}

static int numicro_scc_off(const struct device *dev, clock_control_subsys_t subsys)
{
	struct numicro_scc_subsys *scc_subsys = (struct numicro_scc_subsys *)subsys;

	if (scc_subsys->subsys_id != NUMICRO_SCC_SUBSYS_ID_PCC) {
		return -EINVAL;
	}
	SYS_UnlockReg();
	numicro_pcc_module_enable(dev, scc_subsys->pcc.clk_mod, false);
	SYS_LockReg();

	return 0;
}

/* similar to BSP CLK_SetModuleClock but with regs from dev */
static void numicro_pcc_configure(const struct device *dev,
				  const struct numicro_scc_subsys_pcc *subsys)
{
	const struct numicro_scc_config *config = dev->config;

	volatile uint32_t *clk_reg;

	if (NUMICRO_MODULE_CLKDIV_Msk(subsys->clk_mod) != NUMICRO_MODULE_NoMsk) {
		/* Get clock divider control register address */
		if ((SYS->CSERVER & SYS_CSERVER_VERSION_Msk) == 0x01) { /* M480LD */
			if (NUMICRO_MODULE_CLKDIV(subsys->clk_mod) == 2 &&
			    NUMICRO_MODULE_IP_EN_Pos_ENC(subsys->clk_mod) == 31) { /* EADC1 */
				clk_reg = &config->regs->CLKDIV2;
			} else if (NUMICRO_MODULE_CLKDIV(subsys->clk_mod) == 2 &&
				   NUMICRO_MODULE_IP_EN_Pos_ENC(subsys->clk_mod) == 29) { /* I2S0 */
				clk_reg = &config->regs->CLKDIV2;
			} else if (NUMICRO_MODULE_CLKDIV(subsys->clk_mod) == 2) {
				clk_reg = &config->regs->CLKDIV3;
			} else if (NUMICRO_MODULE_CLKDIV(subsys->clk_mod) == 3) {
				clk_reg = &config->regs->CLKDIV4;
			} else {
				clk_reg = &config->regs->CLKDIV0 +
					  (NUMICRO_MODULE_CLKDIV(subsys->clk_mod)) * 4;
			}
		} else {
			if (NUMICRO_MODULE_CLKDIV(subsys->clk_mod) == 2) {
				clk_reg = &config->regs->CLKDIV3;
			} else if (NUMICRO_MODULE_CLKDIV(subsys->clk_mod) == 3) {
				clk_reg = &config->regs->CLKDIV4;
			} else {
				clk_reg = &config->regs->CLKDIV0 +
					  (NUMICRO_MODULE_CLKDIV(subsys->clk_mod)) * 4;
			}
		}

		/* Apply new divider */
		*clk_reg = (*clk_reg & (~(NUMICRO_MODULE_CLKDIV_Msk(subsys->clk_mod)
					  << NUMICRO_MODULE_CLKDIV_Pos(subsys->clk_mod)))) |
			   subsys->clk_div;
	}

	if (NUMICRO_MODULE_CLKSEL_Msk(subsys->clk_mod) != NUMICRO_MODULE_NoMsk) {
		clk_reg = &config->regs->CLKSEL0 + ((NUMICRO_MODULE_CLKSEL(subsys->clk_mod)) * 4);
		*clk_reg = (*clk_reg & (~(NUMICRO_MODULE_CLKSEL_Msk(subsys->clk_mod)
					  << NUMICRO_MODULE_CLKSEL_Pos(subsys->clk_mod)))) |
			   subsys->clk_src;
	}
}

static int numicro_scc_configure(const struct device *dev, clock_control_subsys_t subsys,
				 void *data)
{
	ARG_UNUSED(data);

	const struct numicro_scc_subsys *scc_subsys = (struct numicro_scc_subsys *)subsys;

	if (scc_subsys->subsys_id != NUMICRO_SCC_SUBSYS_ID_PCC) {
		return -EINVAL;
	}

	SYS_UnlockReg();
	numicro_pcc_configure(dev, &scc_subsys->pcc);
	SYS_LockReg();

	return 0;
}

/* System clock controller driver registration */
static DEVICE_API(clock_control, numicro_scc_api) = {
	.on = numicro_scc_on,
	.off = numicro_scc_off,
	.configure = numicro_scc_configure,
};

/* Calculate and set pll configuration and return frequency.
 * copied from the BSP CLK_EnablePLL function
 */
static uint32_t numicro_scc_set_pll_freq(const struct numicro_scc_config *config,
					 uint32_t target_freq)
{
	uint32_t nf, no, tmp, tmp2, tmp3;
	/* Find best solution */
	uint32_t pll_freq = target_freq;
	uint32_t min = (uint32_t)-1;
	uint32_t nr = 2UL;
	uint32_t min_nr = 0UL;
	uint32_t min_nf = 0UL;
	uint32_t min_no = 0UL;
	uint32_t base_freq = target_freq;
	uint32_t pll_src_clk = config->pll_fin;

	if (config->pllsrc == 1) {
		/* u32NR start from 4 when FIN = 22.1184MHz to avoid calculation overflow */
		nr = 4UL;
	}

	for (no = 1UL; no <= 4UL; no++) {
		/* Break when get good results */
		if (min == 0UL) {
			break;
		}

		if (no == 3) {
			continue;
		}

		if (no == 4UL) {
			pll_freq = base_freq << 2;
		} else if (no == 2UL) {
			pll_freq = base_freq << 1;
		}

		for (nr = 2UL; nr <= 32UL; nr++) {
			/* Break when get good results */
			if (min == 0UL) {
				break;
			}

			tmp = pll_src_clk / nr;
			if (tmp >= 4000000UL && tmp <= 8000000UL) {
				for (nf = 2UL; nf <= 513UL; nf++) {
					/* u32Tmp2 is shifted 2 bits to avoid overflow */
					tmp2 = ((tmp * 2UL) >> 2) * nf;

					if (!(tmp2 >= FREQ_50MHZ && tmp2 <= FREQ_125MHZ)) {
						continue;
					}

					tmp3 = (tmp2 > (pll_freq >> 2)) ? tmp2 - (pll_freq >> 2)
									: (pll_freq >> 2) - tmp2;
					if (tmp3 > min) {
						continue;
					}
					min = tmp3;
					min_nr = nr;
					min_nf = nf;
					min_no = no;

					/* Break when get good results */
					if (min == 0UL) {
						break;
					}
				}
			}
		}
	}

	/* Enable and apply new PLL setting. */
	config->regs->PLLCTL = ((uint32_t)config->pllsrc) << CLK_PLLCTL_PLLSRC_Pos |
			       (min_no - 1UL) << CLK_PLLCTL_OUTDIV_Pos |
			       (min_nr - 1UL) << CLK_PLLCTL_INDIV_Pos | (min_nf - 2UL);

	/* Wait for PLL clock stable */
	while (config->regs->STATUS & CLK_STATUS_PLLSTB_Msk) {
	}

	/* Actual PLL output clock frequency */
	return pll_src_clk / (min_no * min_nr) * min_nf * 2UL;
}

/* Enable a clock source and wait for it to be stable or return error */
static int numicro_scc_start_clock_source(const struct numicro_scc_config *const config,
					  uint8_t pwerctl_offset, uint8_t status_offset)
{
	config->regs->PWRCTL |= BIT(pwerctl_offset);

	/* Copied from the BSP CLK_WaitClockReady function */
	int32_t timeout_cnt = 2160000;

	while ((config->regs->STATUS & BIT(status_offset)) == 0) {

		if (timeout_cnt-- <= 0) {
			return -EAGAIN;
		}
	}

	return 0;
}

/* TODO: move this to a flash controller */
static void numicro_scc_set_flash_access_cycle(const struct device *dev)
{
	struct numicro_scc_data *const data = dev->data;

	if (data->fmc_cycle == 0xff) {
		if (data->hclk_freq < DT_FREQ_M(27)) {
			data->fmc_cycle = 1;
		} else if (data->hclk_freq < DT_FREQ_M(54)) {
			data->fmc_cycle = 2;
		} else if (data->hclk_freq < DT_FREQ_M(81)) {
			data->fmc_cycle = 3;
		} else if (data->hclk_freq < DT_FREQ_M(108)) {
			data->fmc_cycle = 4;
		} else if (data->hclk_freq < DT_FREQ_M(135)) {
			data->fmc_cycle = 5;
		} else if (data->hclk_freq < DT_FREQ_M(162)) {
			data->fmc_cycle = 6;
		} else if (data->hclk_freq < DT_FREQ_M(192)) {
			data->fmc_cycle = 7;
		} else {
			data->fmc_cycle = 8;
		}
	}

	LOG_DBG("Setting flash wait cycles to %d cycles.", data->fmc_cycle);

	FMC->CYCCTL = (uint32_t)data->fmc_cycle;
}

static int numicro_scc_init(const struct device *dev)
{
	const struct numicro_scc_config *const config = dev->config;
	struct numicro_scc_data *const data = dev->data;
	int ret;

	SYS_UnlockReg();

	ret = numicro_scc_start_clock_source(config, CLK_PWRCTL_HIRCEN_Pos, CLK_STATUS_HIRCSTB_Pos);
	if (ret < 0) {
		LOG_WRN("Failed to get HIRC stable");
	}

	/* Switch HCLK source to HIRC to be safe */
	config->regs->CLKSEL0 =
		(config->regs->CLKSEL0 & ~CLK_CLKSEL0_HCLKSEL_Msk) | CLK_CLKSEL0_HCLKSEL_HIRC;
	config->regs->CLKDIV0 &= ~CLK_CLKDIV0_HCLKDIV_Msk;

	/* set pclk0 and plck1 div */
	config->regs->PCLKDIV = config->pclk0_div << CLK_PCLKDIV_APB0DIV_Pos |
				config->pclk1_div << CLK_PCLKDIV_APB1DIV_Pos;

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_lirc))) {
		ret = numicro_scc_start_clock_source(config, CLK_PWRCTL_LIRCEN_Pos,
						     CLK_STATUS_LIRCSTB_Pos);
		if (ret < 0) {
			LOG_WRN("Failed to get LIRC stable");
		}
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hxt))) {
		ret = numicro_scc_start_clock_source(config, CLK_PWRCTL_HXTEN_Pos,
						     CLK_STATUS_HXTSTB_Pos);
		if (ret < 0) {
			LOG_WRN("Failed to get HXT stable");
		}
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_lxt))) {
		ret = numicro_scc_start_clock_source(config, CLK_PWRCTL_LXTEN_Pos,
						     CLK_PWRCTL_HIRCEN_Pos);
		if (ret < 0) {
			LOG_WRN("Failed to get LXT stable");
		}
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pll))) {
		if (data->pll_freq == 0) {
			data->pll_freq =
				numicro_scc_set_pll_freq(config, data->hclk_freq * config->hclkdiv);
			LOG_DBG("Set PLL to %d Hz", data->pll_freq);
			data->hclk_freq = data->pll_freq / config->hclkdiv;
		}
	}

	/* apply divider */
	config->regs->CLKDIV0 = (config->regs->CLKDIV0 & ~CLK_CLKDIV0_HCLKDIV_Msk) |
				(config->hclkdiv - 1) << CLK_CLKDIV0_HCLKDIV_Pos;

	/* switch to new clock */
	config->regs->CLKSEL0 = (config->regs->CLKSEL0 & ~CLK_CLKSEL0_HCLKSEL_Msk) |
				config->hclksel << CLK_CLKSEL0_HCLKSEL_Pos;

	/* disable HIRC if not requested */
	if (!DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hirc))) {
		config->regs->PWRCTL &= ~CLK_PWRCTL_HIRCEN_Msk;
	}

	numicro_scc_set_flash_access_cycle(dev);

	SYS_LockReg();
	return 0;
}

/* At most one compatible with status "okay" */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Requires at most one compatible with status \"okay\"");

#define DT_CLK_SRC(clk, src)                                                                       \
	DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(clk), 0), DT_NODELABEL(src))

/* get HCLKSEL from DTS */
#if DT_CLK_SRC(hclk, clk_hxt)
#if !DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hxt))
#error "Clock HXT has to be enabled to be used as hclk source"
#endif
#define HCLKSEL 0
#elif DT_CLK_SRC(hclk, clk_lxt)
#if !DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_lxt))
#error "Clock LXT has to be enabled to be used as hclk source"
#endif
#define HCLKSEL 1
#elif DT_CLK_SRC(hclk, pll)
#if !DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pll))
#error "CLock PLL has to be enabled to be used as hclk source"
#endif
#define HCLKSEL 2
#elif DT_CLK_SRC(hclk, clk_lirc)
#if !DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_lirc))
#error "Clock LIRC has to be enabled to be used as hclk source"
#endif
#define HCLKSEL 3
#elif DT_CLK_SRC(hclk, clk_hirc)
#if !DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hirc))
#error "Clock HIRC has to be enabled to be used as hclk source"
#endif
#define HCLKSEL 7
#else
#error "Invalid hclk source"
#endif

/* get PLL source from DTS */
#if DT_CLK_SRC(pll, clk_hxt)
#if !DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hxt))
#error "Clock HXT has to be enabled to be used as pll source"
#endif
#define PLL_FIN DT_PROP(DT_NODELABEL(clk_hxt), clock_frequency)
#define PLLSRC  0
#elif DT_CLK_SRC(pll, clk_hirc)
#if !DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hirc))
#error "Clock HIRC has to be enabled to be used as pll source"
#endif
#define PLL_FIN DT_PROP(DT_NODELABEL(clk_hirc), clock_frequency)
#define PLLSRC  1
#else
#error "Invalid PLL source"
#endif

static const struct numicro_scc_config numicro_scc_config = {
	.regs = (CLK_T *)DT_INST_REG_ADDR(0),

	.pll_fin = PLL_FIN,
	.pllsrc = PLLSRC,
	.hclkdiv = DT_INST_PROP_OR(0, div, 1),
	.hclksel = HCLKSEL,
	.pclk0_div = DT_ENUM_IDX(DT_NODELABEL(hclk), pclk0_div),
	.pclk1_div = DT_ENUM_IDX(DT_NODELABEL(hclk), pclk1_div),
};

static struct numicro_scc_data numicro_scc_data = {
	.hclk_freq = DT_PROP(DT_NODELABEL(hclk), clock_frequency),
	.pll_freq = 0, /* TODO: load override freq from dts. or warn when hclk div is set but pll
			* freq not
			*/
	.fmc_cycle = DT_INST_PROP_OR(0, flash_access_cycles, 0xff),
};

DEVICE_DT_INST_DEFINE(0, numicro_scc_init, NULL, &numicro_scc_data, &numicro_scc_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &numicro_scc_api);
