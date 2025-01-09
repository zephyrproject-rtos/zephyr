/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_pcc

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/npcm_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_npcm, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

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

/* CDCG register fields */
#define NPCM_HFCGCTRL_LOAD     0
#define NPCM_HFCGCTRL_LOCK     2
#define NPCM_HFCGCTRL_CLK_CHNG 7

/* Clock settings from pcc node */
#define NPCM_NODE_PCC_LABEL DT_NODELABEL(pcc)
/* Target OFMCLK freq */
#define OFMCLK      DT_PROP(NPCM_NODE_PCC_LABEL, clock_frequency)
/* Core clock prescaler */
#define FPRED_VAL   (DT_PROP(NPCM_NODE_PCC_LABEL, core_prescaler) - 1)
/* APB1 clock divider */
#define APB1DIV_VAL (DT_PROP(NPCM_NODE_PCC_LABEL, apb1_prescaler) - 1)
/* APB2 clock divider */
#define APB2DIV_VAL (DT_PROP(NPCM_NODE_PCC_LABEL, apb2_prescaler) - 1)
/* APB3 clock divider */
#define APB3DIV_VAL (DT_PROP(NPCM_NODE_PCC_LABEL, apb3_prescaler) - 1)
/* AHB6 clock divider*/
#define AHB6DIV_VAL (DT_PROP(NPCM_NODE_PCC_LABEL, ahb6_prescaler) - 1)
/* FIU clock divider */
#define FIUDIV_VAL  (DT_PROP(NPCM_NODE_PCC_LABEL, fiu_prescaler) - 1)
/* I3C clock divider */
#define I3C_TC_DIV_VAL  (DT_PROP(NPCM_NODE_PCC_LABEL, i3c_tc_prescaler) - 1)

/* Core domain clock */
#define CORE_CLK           (OFMCLK / DT_PROP(NPCM_NODE_PCC_LABEL, core_prescaler))
/* Low Frequency clock */
#define LFCLK              32768
/* FMUL clock */
#if (OFMCLK > MHZ(50))
#define FMCLK              (OFMCLK / 2) /* FMUL clock = OFMCLK/2 */
#else
#define FMCLK              OFMCLK /* FMUL clock = OFMCLK */
#endif
/* MCLK clock */
#define MCLK               OFMCLK /* MCLK clock = OFMCLK */
/* APBs source clock */
#define APBSRC_CLK         MCLK
/* USB2.0 clock */
#define USB20_CLK          MHZ(12)
/* SIO clock */
#define SIO_CLK            MHZ(48)
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

/* PMC multi-registers */
#define NPCM_PWDWN_CTL_OFFSET(n) (((n) < 7) ? (0x07 + n) : (0x15 + (n - 7)))
#define NPCM_PWDWN_CTL(base, n)  (*(volatile uint8_t *)(base + NPCM_PWDWN_CTL_OFFSET(n)))
#define NPCM_CLOCK_REG_OFFSET(n)                                                                   \
	(((n) >> NPCM_CLOCK_PWDWN_GROUP_OFFSET) & NPCM_CLOCK_PWDWN_GROUP_MASK)
#define NPCM_CLOCK_REG_BIT_OFFSET(n)                                                               \
	(((n) >> NPCM_CLOCK_PWDWN_BIT_OFFSET) & NPCM_CLOCK_PWDWN_BIT_MASK)

#define DRV_CONFIG(dev) ((const struct npcm_pcc_config *)(dev)->config)

static inline int npcm_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t clk_cfg = (uint32_t)sub_system;
	const uint32_t pmc_base = DRV_CONFIG(dev)->base_pmc;

	if ((clk_cfg & BIT(NPCM_CLOCK_PWDWN_VALID_OFFSET)) == 0) {
		LOG_ERR("Unsupported clock cfg %d", clk_cfg);
		return -EINVAL;
	}

	/* Clear related PD (Power-Down) bit of module to turn on clock */
	NPCM_PWDWN_CTL(pmc_base, NPCM_CLOCK_REG_OFFSET(clk_cfg)) &=
		~(BIT(NPCM_CLOCK_REG_BIT_OFFSET(clk_cfg)));
	return 0;
}

static inline int npcm_clock_control_off(const struct device *dev,
					 clock_control_subsys_t sub_system)
{
	uint32_t clk_cfg = (uint32_t)sub_system;
	const uint32_t pmc_base = DRV_CONFIG(dev)->base_pmc;

	if ((clk_cfg & BIT(NPCM_CLOCK_PWDWN_VALID_OFFSET)) == 0) {
		LOG_ERR("Unsupported clock cfg %d", clk_cfg);
		return -EINVAL;
	}

	/* Set related PD (Power-Down) bit of module to turn off clock */
	NPCM_PWDWN_CTL(pmc_base, NPCM_CLOCK_REG_OFFSET(clk_cfg)) |=
		BIT(NPCM_CLOCK_REG_BIT_OFFSET(clk_cfg));
	return 0;
}

static int npcm_clock_control_get_subsys_rate(const struct device *dev,
					      clock_control_subsys_t sub_system, uint32_t *rate)
{
	ARG_UNUSED(dev);
	uint32_t clk_cfg = (uint32_t)sub_system;
	uint8_t bus_id = ((clk_cfg >> NPCM_CLOCK_BUS_OFFSET) & NPCM_CLOCK_BUS_MASK);

	switch (bus_id) {
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
	case NPCM_CLOCK_BUS_I3C_TC:
		*rate = NPCM_APB_CLOCK(3) / (I3C_TC_DIV_VAL + 1);
		break;
	case NPCM_CLOCK_BUS_CORE:
		*rate = CORE_CLK;
		break;
	case NPCM_CLOCK_BUS_OSC:
		*rate = OFMCLK;
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
static struct clock_control_driver_api npcm_clock_control_api = {
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
	priv->hfcbcd1 = (I3C_TC_DIV_VAL << 2) | FIUDIV_VAL;
	priv->hfcbcd2 = APB3DIV_VAL;

	return 0;
}

const struct npcm_pcc_config pcc_config = {
	.base_cdcg = DT_INST_REG_ADDR_BY_NAME(0, cdcg),
	.base_pmc = DT_INST_REG_ADDR_BY_NAME(0, pmc),
};

DEVICE_DT_INST_DEFINE(0, &npcm_clock_control_init, NULL, NULL, &pcc_config, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &npcm_clock_control_api);
