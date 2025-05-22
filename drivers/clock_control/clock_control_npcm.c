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

/*
 * Core Domain Clock Generator (CDCG) device registers
 */
struct cdcg_reg {
	/* High Frequency Clock Generator (HFCG) registers */
	/* 0x000: HFCG Control */
	volatile uint8_t hfcgctrl;
	volatile uint8_t reserved1;
	/* 0x002: HFCG M Low Byte Value */
	volatile uint8_t hfcgml;
	volatile uint8_t reserved2;
	/* 0x004: HFCG M High Byte Value */
	volatile uint8_t hfcgmh;
	volatile uint8_t reserved3;
	/* 0x006: HFCG N Value */
	volatile uint8_t hfcgn;
	volatile uint8_t reserved4;
	/* 0x008: HFCG Prescaler */
	volatile uint8_t hfcgp;
	volatile uint8_t reserved5[7];
	/* 0x010: HFCG Bus Clock Dividers */
	volatile uint8_t hfcbcd;
	volatile uint8_t reserved6;
	/* 0x012: HFCG Bus Clock Dividers */
	volatile uint8_t hfcbcd1;
	volatile uint8_t reserved7;
	/* 0x014: HFCG Bus Clock Dividers */
	volatile uint8_t hfcbcd2;
	volatile uint8_t reserved12[8];
	/* 0x01d: HFCG Bus Clock Dividers */
	volatile uint8_t hfcbcd3;
};

/* clock bus references */
#define NPCM_CLOCK_BUS_LFCLK     0
#define NPCM_CLOCK_BUS_OSC       1
#define NPCM_CLOCK_BUS_FIU       2
#define NPCM_CLOCK_BUS_I3C       3
#define NPCM_CLOCK_BUS_CORE      4
#define NPCM_CLOCK_BUS_APB1      5
#define NPCM_CLOCK_BUS_APB2      6
#define NPCM_CLOCK_BUS_APB3      7
#define NPCM_CLOCK_BUS_APB4      8
#define NPCM_CLOCK_BUS_AHB6      9
#define NPCM_CLOCK_BUS_FMCLK     10
#define NPCM_CLOCK_BUS_USB20_CLK 11
#define NPCM_CLOCK_BUS_SIO_CLK   12

/* clock enable/disable references */
#define NPCM_PWDWN_CTL0 0
#define NPCM_PWDWN_CTL1 1
#define NPCM_PWDWN_CTL2 2
#define NPCM_PWDWN_CTL3 3
#define NPCM_PWDWN_CTL4 4
#define NPCM_PWDWN_CTL5 5
#define NPCM_PWDWN_CTL6 6
#define NPCM_PWDWN_CTL7 7

/* CDCG register fields */
#define NPCM_HFCGCTRL_LOAD     0
#define NPCM_HFCGCTRL_LOCK     2
#define NPCM_HFCGCTRL_CLK_CHNG 7

/* Clock settings from pcc node */
/* Target OFMCLK freq */
#define OFMCLK      DT_PROP(DT_NODELABEL(pcc), clock_frequency)
/* Core clock prescaler */
#define FPRED_VAL   (DT_PROP(DT_NODELABEL(pcc), core_prescaler) - 1)
/* APB1 clock divider */
#define APB1DIV_VAL (DT_PROP(DT_NODELABEL(pcc), apb1_prescaler) - 1)
/* APB2 clock divider */
#define APB2DIV_VAL (DT_PROP(DT_NODELABEL(pcc), apb2_prescaler) - 1)
/* APB3 clock divider */
#define APB3DIV_VAL (DT_PROP(DT_NODELABEL(pcc), apb3_prescaler) - 1)
/* AHB6 clock divider*/
#define AHB6DIV_VAL (DT_PROP(DT_NODELABEL(pcc), ahb6_prescaler) - 1)
/* FIU clock divider */
#define FIUDIV_VAL  (DT_PROP(DT_NODELABEL(pcc), fiu_prescaler) - 1)
/* I3C clock divider */
#define I3CDIV_VAL  (DT_PROP(DT_NODELABEL(pcc), i3c_prescaler) - 1)

/* Core domain clock */
#define CORE_CLK           (OFMCLK / DT_PROP(DT_NODELABEL(pcc), core_prescaler))
/* Low Frequency clock */
#define LFCLK              32768
/* FMUL clock */
#define FMCLK              OFMCLK /* FMUL clock = OFMCLK */
/* APBs source clock */
#define APBSRC_CLK         OFMCLK
/* USB2.0 clock */
#define USB20_CLK          12000000
/* SIO clock */
#define SIO_CLK            48000000
/* Get APB clock freq */
#define NPCM_APB_CLOCK(no) (APBSRC_CLK / (APB##no##DIV_VAL + 1))

struct freq_multiplier_t {
	uint32_t ofmclk;
	uint8_t hfcgn;
	uint8_t hfcgmh;
	uint8_t hfcgml;
};

static struct freq_multiplier_t freq_multiplier[] = {
	{.ofmclk = 100000000, .hfcgn = 0x82, .hfcgmh = 0x0B, .hfcgml = 0xEC},
	{.ofmclk = 96000000, .hfcgn = 0x82, .hfcgmh = 0x0B, .hfcgml = 0x72},
	{.ofmclk = 80000000, .hfcgn = 0x82, .hfcgmh = 0x09, .hfcgml = 0x89},
	{.ofmclk = 66000000, .hfcgn = 0x82, .hfcgmh = 0x07, .hfcgml = 0xDE},
	{.ofmclk = 50000000, .hfcgn = 0x02, .hfcgmh = 0x0B, .hfcgml = 0xEC},
	{.ofmclk = 48000000, .hfcgn = 0x02, .hfcgmh = 0x0B, .hfcgml = 0x72},
	{.ofmclk = 40000000, .hfcgn = 0x02, .hfcgmh = 0x09, .hfcgml = 0x89},
	{.ofmclk = 33000000, .hfcgn = 0x02, .hfcgmh = 0x07, .hfcgml = 0xDE}};

struct clk_cfg_t {
	uint32_t clock_id;
	uint16_t bus;
};

static struct clk_cfg_t clk_cfg[] = {
	{.clock_id = NPCM_CLOCK_PWM_I, .bus = NPCM_CLOCK_BUS_LFCLK},
	{.clock_id = NPCM_CLOCK_PWM_J, .bus = NPCM_CLOCK_BUS_LFCLK},
	{.clock_id = NPCM_CLOCK_I3CI, .bus = NPCM_CLOCK_BUS_APB3},
	{.clock_id = NPCM_CLOCK_UART3, .bus = NPCM_CLOCK_BUS_APB2},
	{.clock_id = NPCM_CLOCK_UART2, .bus = NPCM_CLOCK_BUS_APB2},

	{.clock_id = NPCM_CLOCK_FIU, .bus = NPCM_CLOCK_BUS_FIU},
	{.clock_id = NPCM_CLOCK_USB20, .bus = NPCM_CLOCK_BUS_USB20_CLK},
	{.clock_id = NPCM_CLOCK_UART, .bus = NPCM_CLOCK_BUS_APB2},

	{.clock_id = NPCM_CLOCK_PWM_A, .bus = NPCM_CLOCK_BUS_LFCLK},
	{.clock_id = NPCM_CLOCK_PWM_B, .bus = NPCM_CLOCK_BUS_LFCLK},
	{.clock_id = NPCM_CLOCK_PWM_C, .bus = NPCM_CLOCK_BUS_LFCLK},
	{.clock_id = NPCM_CLOCK_PWM_D, .bus = NPCM_CLOCK_BUS_LFCLK},
	{.clock_id = NPCM_CLOCK_PWM_E, .bus = NPCM_CLOCK_BUS_LFCLK},
	{.clock_id = NPCM_CLOCK_PWM_F, .bus = NPCM_CLOCK_BUS_LFCLK},
	{.clock_id = NPCM_CLOCK_PWM_G, .bus = NPCM_CLOCK_BUS_LFCLK},
	{.clock_id = NPCM_CLOCK_PWM_H, .bus = NPCM_CLOCK_BUS_LFCLK},

	{.clock_id = NPCM_CLOCK_SMB1, .bus = NPCM_CLOCK_BUS_APB3},
	{.clock_id = NPCM_CLOCK_SMB2, .bus = NPCM_CLOCK_BUS_APB3},
	{.clock_id = NPCM_CLOCK_SMB3, .bus = NPCM_CLOCK_BUS_APB3},
	{.clock_id = NPCM_CLOCK_SMB4, .bus = NPCM_CLOCK_BUS_APB3},
	{.clock_id = NPCM_CLOCK_SMB5, .bus = NPCM_CLOCK_BUS_APB3},
	{.clock_id = NPCM_CLOCK_SMB6, .bus = NPCM_CLOCK_BUS_APB3},

	{.clock_id = NPCM_CLOCK_ITIM1, .bus = NPCM_CLOCK_BUS_LFCLK},
	{.clock_id = NPCM_CLOCK_ITIM2, .bus = NPCM_CLOCK_BUS_LFCLK},
	{.clock_id = NPCM_CLOCK_ITIM3, .bus = NPCM_CLOCK_BUS_LFCLK},
	{.clock_id = NPCM_CLOCK_ADC, .bus = NPCM_CLOCK_BUS_APB1},
	{.clock_id = NPCM_CLOCK_PECI, .bus = NPCM_CLOCK_BUS_FMCLK},

	{.clock_id = NPCM_CLOCK_UART4, .bus = NPCM_CLOCK_BUS_APB2},

	{.clock_id = NPCM_CLOCK_ESPI, .bus = NPCM_CLOCK_BUS_APB3},

	{.clock_id = NPCM_CLOCK_SMB7, .bus = NPCM_CLOCK_BUS_APB3},
	{.clock_id = NPCM_CLOCK_SMB8, .bus = NPCM_CLOCK_BUS_APB3},
	{.clock_id = NPCM_CLOCK_SMB9, .bus = NPCM_CLOCK_BUS_APB3},
	{.clock_id = NPCM_CLOCK_SMB10, .bus = NPCM_CLOCK_BUS_APB3},
	{.clock_id = NPCM_CLOCK_SMB11, .bus = NPCM_CLOCK_BUS_APB3},
	{.clock_id = NPCM_CLOCK_SMB12, .bus = NPCM_CLOCK_BUS_APB3},
};

/* PMC multi-registers */
#define NPCM_PWDWN_CTL_OFFSET(n)     (((n) < 7) ? (0x07 + n) : (0x15 + (n - 7)))
#define NPCM_PWDWN_CTL(base, n)      (*(volatile uint8_t *)(base + NPCM_PWDWN_CTL_OFFSET(n)))
#define NPCM_CLOCK_REG_OFFSET(n)     ((n) >> 3)
#define NPCM_CLOCK_REG_BIT_OFFSET(n) ((n) & 0x7)

#define DRV_CONFIG(dev) ((const struct npcm_pcc_config *)(dev)->config)

/* Clock controller local functions */
static struct clk_cfg_t *npcm_get_cfg(clock_control_subsys_t sub_system)
{
	uint32_t clk_id = (uint32_t)sub_system;
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(clk_cfg); i++) {
		if (clk_cfg[i].clock_id == clk_id) {
			return &clk_cfg[i];
		}
	}

	return NULL;
}

static inline int npcm_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t clk_id = (uint32_t)sub_system;
	struct clk_cfg_t *priv;
	const uint32_t pmc_base = DRV_CONFIG(dev)->base_pmc;

	priv = npcm_get_cfg(sub_system);
	if (!priv) {
		LOG_ERR("Unsupported clock id %d", clk_id);
		return -EINVAL;
	}

	/* Clear related PD (Power-Down) bit of module to turn on clock */
	NPCM_PWDWN_CTL(pmc_base, NPCM_CLOCK_REG_OFFSET(priv->clock_id)) &=
		~(BIT(NPCM_CLOCK_REG_BIT_OFFSET(priv->clock_id)));
	return 0;
}

static inline int npcm_clock_control_off(const struct device *dev,
					 clock_control_subsys_t sub_system)
{
	uint32_t clk_id = (uint32_t)sub_system;
	struct clk_cfg_t *priv;
	const uint32_t pmc_base = DRV_CONFIG(dev)->base_pmc;

	priv = npcm_get_cfg(sub_system);
	if (!priv) {
		LOG_ERR("Unsupported clock id %d", clk_id);
		return -EINVAL;
	}

	/* Set related PD (Power-Down) bit of module to turn off clock */
	NPCM_PWDWN_CTL(pmc_base, NPCM_CLOCK_REG_OFFSET(priv->clock_id)) |=
		~(BIT(NPCM_CLOCK_REG_BIT_OFFSET(priv->clock_id)));
	return 0;
}

static int npcm_clock_control_get_subsys_rate(const struct device *dev,
					      clock_control_subsys_t sub_system, uint32_t *rate)
{
	ARG_UNUSED(dev);
	uint32_t clk_id = (uint32_t)sub_system;
	struct clk_cfg_t *priv;

	priv = npcm_get_cfg(sub_system);
	if (!priv) {
		LOG_ERR("Unsupported clock id %d", clk_id);
		return -EINVAL;
	}

	switch (priv->bus) {
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
		*rate = CORE_CLK / (AHB6DIV_VAL + 1);
		break;
	case NPCM_CLOCK_BUS_FIU:
		*rate = CORE_CLK / (FIUDIV_VAL + 1);
		break;
	case NPCM_CLOCK_BUS_I3C:
		*rate = CORE_CLK / (I3CDIV_VAL + 1);
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
	case NPCM_CLOCK_BUS_USB20_CLK:
		*rate = USB20_CLK;
		break;
	case NPCM_CLOCK_BUS_SIO_CLK:
		*rate = SIO_CLK;
		break;
	default:
		*rate = 0U;
		/* Invalid parameters */
		return -EINVAL;
	}

	return 0;
}

/* Clock controller driver registration */
static DEVICE_API(clock_control, npcm_clock_control_api) = {
	.on = npcm_clock_control_on,
	.off = npcm_clock_control_off,
	.get_rate = npcm_clock_control_get_subsys_rate,
};

static int npcm_clock_control_init(const struct device *dev)
{
	struct cdcg_reg *const priv = (struct cdcg_reg *)(DRV_CONFIG(dev)->base_cdcg);
	struct freq_multiplier_t *freq_p;
	int i;

	for (i = 0; i < ARRAY_SIZE(freq_multiplier); i++) {
		if (freq_multiplier[i].ofmclk == OFMCLK) {
			freq_p = &freq_multiplier[i];
			break;
		}
	}

	if (i >= ARRAY_SIZE(freq_multiplier)) {
		LOG_ERR("Unsupported OFMCLK frequency %d", OFMCLK);
		return -EINVAL;
	}

	/*
	 * Resetting the OFMCLK (even to the same value) will make the clock
	 * unstable for a little which can affect peripheral communication like
	 * eSPI. Skip this if not needed.
	 */
	if (priv->hfcgn != freq_p->hfcgn || priv->hfcgml != freq_p->hfcgml ||
	    priv->hfcgmh != freq_p->hfcgmh) {
		/*
		 * Configure frequency multiplier M/N values according to
		 * the requested OFMCLK (Unit:Hz).
		 */
		priv->hfcgn = freq_p->hfcgn;
		priv->hfcgml = freq_p->hfcgml;
		priv->hfcgmh = freq_p->hfcgmh;

		/* Load M and N values into the frequency multiplier */
		priv->hfcgctrl |= BIT(NPCM_HFCGCTRL_LOAD);
		/* Wait for stable */
		while (sys_test_bit(priv->hfcgctrl, NPCM_HFCGCTRL_CLK_CHNG))
			;
	}

	/* Set all clock prescalers of core and peripherals. */
	priv->hfcgp = (FPRED_VAL << 4) | AHB6DIV_VAL;
	priv->hfcbcd = APB1DIV_VAL | (APB2DIV_VAL << 4);
	priv->hfcbcd1 = (I3CDIV_VAL << 2) | FIUDIV_VAL;
	priv->hfcbcd2 = APB3DIV_VAL;

	return 0;
}

const struct npcm_pcc_config pcc_config = {
	.base_cdcg = DT_INST_REG_ADDR_BY_NAME(0, cdcg),
	.base_pmc = DT_INST_REG_ADDR_BY_NAME(0, pmc),
};

DEVICE_DT_INST_DEFINE(0, &npcm_clock_control_init, NULL, NULL, &pcc_config, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &npcm_clock_control_api);
