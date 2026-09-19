/*
 * Copyright 2023,2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_clock

#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <Clock_Ip.h>

LOG_MODULE_REGISTER(clock_control_nxp_s32, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#if defined(CLOCK_IP_HAS_FIRC_CLK) && (CLOCK_IP_HAS_FIRC_CLK == 0)
/*  Support newer platforms in which CLOCK_IS_OFF is undefined */
#define CLOCK_IS_OFF -1
#endif

#define NXP_S32_CLOCK_CONFIG_IDX CONFIG_CLOCK_CONTROL_NXP_S32_CLOCK_CONFIG_IDX

BUILD_ASSERT(CLOCK_IP_GET_FREQUENCY_API == STD_ON,
	     "Clock Get Frequency API must be enabled");

#define NXP_S32_EMAC_NODE DT_NODELABEL(emac0)

/*
 * The EMAC RX/TX/TS source muxes have to be attached before the MAC leaves
 * reset, and which receive and transmit sources are correct depends on the PHY
 * interface selected in devicetree. Only set them up on SoCs that have an EMAC
 * and only when it is actually enabled.
 */
#if defined(CLOCK_IP_HAS_EMAC_RX_CLK) && DT_NODE_HAS_STATUS_OKAY(NXP_S32_EMAC_NODE)
#define NXP_S32_HAS_EMAC 1

/*
 * Clock rates the PHY drives into the SoC, both fixed by IEEE 802.3: RMII
 * supplies a single 50 MHz reference, MII separate transmit and receive clocks
 * that run at 25 MHz for 100 Mbps.
 */
#define NXP_S32_EMAC_RMII_REF_CLK_HZ 50000000U
#define NXP_S32_EMAC_MII_CLK_HZ      25000000U

#if DT_ENUM_HAS_VALUE(NXP_S32_EMAC_NODE, phy_connection_type, rmii)
/*
 * The one reference clock feeds the receive and transmit domains, and the MAC
 * clocks its MII-side logic at half the RMII rate.
 */
#define NXP_S32_EMAC_TXPAD_CLK_HZ NXP_S32_EMAC_RMII_REF_CLK_HZ
#define NXP_S32_EMAC_RXPAD_CLK_HZ 0U
#define NXP_S32_EMAC_RX_SRC       EMAC_MII_RMII_TX_CLK
#define NXP_S32_EMAC_CLK_DIV      2U
#elif DT_ENUM_HAS_VALUE(NXP_S32_EMAC_NODE, phy_connection_type, mii)
/*
 * The PHY drives the transmit and receive clocks on separate pads, already at
 * the MII-side rate.
 */
#define NXP_S32_EMAC_TXPAD_CLK_HZ NXP_S32_EMAC_MII_CLK_HZ
#define NXP_S32_EMAC_RXPAD_CLK_HZ NXP_S32_EMAC_MII_CLK_HZ
#define NXP_S32_EMAC_RX_SRC       EMAC_MII_RX_CLK
#define NXP_S32_EMAC_CLK_DIV      1U
#else
#error "Unsupported PHY connection type for the S32 Ethernet MAC"
#endif

/*
 * SELCTL and SELSTAT sit in the same bits of every mux control and status
 * register, so one mask each covers all three. Assert it rather than leaving
 * it to chance.
 */
#define NXP_S32_EMAC_SELCTL_MASK  MC_CGM_MUX_7_CSC_SELCTL_MASK
#define NXP_S32_EMAC_SELSTAT_MASK MC_CGM_MUX_7_CSS_SELSTAT_MASK
BUILD_ASSERT(MC_CGM_MUX_8_CSC_SELCTL_MASK == NXP_S32_EMAC_SELCTL_MASK);
BUILD_ASSERT(MC_CGM_MUX_9_CSC_SELCTL_MASK == NXP_S32_EMAC_SELCTL_MASK);
BUILD_ASSERT(MC_CGM_MUX_8_CSS_SELSTAT_MASK == NXP_S32_EMAC_SELSTAT_MASK);
BUILD_ASSERT(MC_CGM_MUX_9_CSS_SELSTAT_MASK == NXP_S32_EMAC_SELSTAT_MASK);

/*
 * The source mux of an EMAC clock domain is attached when the gate of the
 * domain is turned on. The receive and transmit domains are derived from
 * clocks the PHY drives into the SoC, so this must happen once the pads are
 * muxed: the glitchless MC_CGM mux refuses to switch to a source that is not
 * toggling and leaves the domain on FIRC. Clock_Ip_Init() reports success
 * either way, so check the status register.
 */
struct nxp_s32_emac_clk {
	const Clock_Ip_SelectorConfigType selector[1];
	const Clock_Ip_DividerConfigType divider[1];
	volatile const uint32_t *csc;
	volatile const uint32_t *css;
	Clock_Ip_NameType gate;
};

static const struct nxp_s32_emac_clk nxp_s32_emac_clks[] = {
	{
		.gate = EMAC0_RX_CLK,
		.selector = {{.Name = EMAC_RX_CLK, .Value = NXP_S32_EMAC_RX_SRC}},
		.divider = {{.Name = EMAC_RX_CLK, .Value = NXP_S32_EMAC_CLK_DIV}},
		.csc = &IP_MC_CGM->MUX_7_CSC,
		.css = &IP_MC_CGM->MUX_7_CSS,
	},
	/* The transmit clock always comes off the same pad, in either mode. */
	{
		.gate = EMAC0_TX_CLK,
		.selector = {{.Name = EMAC_TX_CLK, .Value = EMAC_MII_RMII_TX_CLK}},
		.divider = {{.Name = EMAC_TX_CLK, .Value = NXP_S32_EMAC_CLK_DIV}},
		.csc = &IP_MC_CGM->MUX_8_CSC,
		.css = &IP_MC_CGM->MUX_8_CSS,
	},
	/*
	 * The timestamp unit runs from PLL_PHI0 instead: it is locked to the
	 * crystal, keeps running regardless of the PHY and the link speed, and
	 * undivided gives the finest time resolution. Self-test of the EMAC
	 * timestamp memory also needs this clock at 1.5 times AIPS_SLOW_CLK or
	 * more, which the PHY clocks cannot provide.
	 */
	{
		.gate = EMAC0_TS_CLK,
		.selector = {{.Name = EMAC_TS_CLK, .Value = PLL_PHI0_CLK}},
		.divider = {{.Name = EMAC_TS_CLK, .Value = 1U}},
		.csc = &IP_MC_CGM->MUX_9_CSC,
		.css = &IP_MC_CGM->MUX_9_CSS,
	},
};

/* Tell the HAL what the PHY drives into the pads; software state only. */
static const Clock_Ip_ExtClkConfigType nxp_s32_emac_ext_clks[] = {
	{.Name = EMAC_MII_RX_CLK, .Value = NXP_S32_EMAC_RXPAD_CLK_HZ},
	{.Name = EMAC_MII_RMII_TX_CLK, .Value = NXP_S32_EMAC_TXPAD_CLK_HZ},
};

/*
 * The HAL keeps a pointer to the configuration applied last and reads the
 * configured core and bus frequencies through it on later updates, so this
 * has to outlive the call and carry those of the boot configuration.
 */
static Clock_Ip_ClockConfigType nxp_s32_emac_clk_config = {
	.SelectorsCount = 1U,
	.DividersCount = 1U,
	.ExtClksCount = ARRAY_SIZE(nxp_s32_emac_ext_clks),
	.ExtClks = &nxp_s32_emac_ext_clks,
};

static int nxp_s32_emac_attach(const struct nxp_s32_emac_clk *clk)
{
	const Clock_Ip_ClockConfigType *boot_config =
		&Clock_Ip_aClockConfig[NXP_S32_CLOCK_CONFIG_IDX];

	nxp_s32_emac_clk_config.ConfigureFrequenciesCount = boot_config->ConfigureFrequenciesCount;
	nxp_s32_emac_clk_config.ConfiguredFrequencies = boot_config->ConfiguredFrequencies;
	nxp_s32_emac_clk_config.Selectors = &clk->selector;
	nxp_s32_emac_clk_config.Dividers = &clk->divider;

	if (Clock_Ip_Init(&nxp_s32_emac_clk_config) != CLOCK_IP_SUCCESS) {
		return -EIO;
	}

	if (FIELD_GET(NXP_S32_EMAC_SELSTAT_MASK, *clk->css) !=
	    FIELD_GET(NXP_S32_EMAC_SELCTL_MASK, *clk->csc)) {
		LOG_ERR("EMAC clock did not switch to its source; "
			"is the pin muxed and is the PHY driving it?");
		return -EIO;
	}

	return 0;
}
#endif /* defined(NXP_S32_HAS_EMAC) */

static int nxp_s32_clock_on(const struct device *dev,
			    clock_control_subsys_t sub_system)
{
	Clock_Ip_NameType clock_name = (Clock_Ip_NameType)sub_system;

	if ((clock_name <= CLOCK_IS_OFF) || (clock_name >= RESERVED_CLK)) {
		return -EINVAL;
	}

#if defined(NXP_S32_HAS_EMAC)
	ARRAY_FOR_EACH_PTR(nxp_s32_emac_clks, clk) {
		if (clk->gate == clock_name) {
			int ret = nxp_s32_emac_attach(clk);

			if (ret != 0) {
				return ret;
			}

			break;
		}
	}
#endif

	Clock_Ip_EnableModuleClock(clock_name);

	return 0;
}

static int nxp_s32_clock_off(const struct device *dev,
			     clock_control_subsys_t sub_system)
{
	Clock_Ip_NameType clock_name = (Clock_Ip_NameType)sub_system;

	if ((clock_name <= CLOCK_IS_OFF) || (clock_name >= RESERVED_CLK)) {
		return -EINVAL;
	}

	Clock_Ip_DisableModuleClock(clock_name);

	return 0;
}

static int nxp_s32_clock_get_rate(const struct device *dev,
				  clock_control_subsys_t sub_system,
				  uint32_t *rate)
{
	Clock_Ip_NameType clock_name = (Clock_Ip_NameType)sub_system;

	if ((clock_name <= CLOCK_IS_OFF) || (clock_name >= RESERVED_CLK)) {
		return -EINVAL;
	}

	*rate = Clock_Ip_GetClockFrequency(clock_name);

	return 0;
}

static int nxp_s32_clock_init(const struct device *dev)
{
	Clock_Ip_StatusType status;

	status = Clock_Ip_Init(&Clock_Ip_aClockConfig[NXP_S32_CLOCK_CONFIG_IDX]);

	return (status == CLOCK_IP_SUCCESS ? 0 : -EIO);
}

static DEVICE_API(clock_control, nxp_s32_clock_driver_api) = {
	.on = nxp_s32_clock_on,
	.off = nxp_s32_clock_off,
	.get_rate = nxp_s32_clock_get_rate,
};

DEVICE_DT_INST_DEFINE(0,
		      nxp_s32_clock_init,
		      NULL, NULL, NULL,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &nxp_s32_clock_driver_api);
