/*
 * Copyright 2025, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mc_cgm

#include <errno.h>
#include <zephyr/drivers/clock_control/nxp_clock_controller_sources.h>
#include <zephyr/dt-bindings/clock/nxp_mc_cgm.h>
#include <zephyr/sys/util.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control);

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(fxosc), nxp_fxosc, okay)
const fxosc_config_t fxosc_config = {.freqHz = NXP_FXOSC_FREQ,
				     .workMode = NXP_FXOSC_WORKMODE,
				     .startupDelay = NXP_FXOSC_DELAY,
				     .overdriveProtect = NXP_FXOSC_OVERDRIVE};
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), nxp_plldig, okay)
const pll_config_t pll_config = {.workMode = NXP_PLL_WORKMODE,
				 .preDiv = NXP_PLL_PREDIV, /* PLL input clock predivider: 2 */
				 .postDiv = NXP_PLL_POSTDIV,
				 .multiplier = NXP_PLL_MULTIPLIER,
				 .fracLoopDiv = NXP_PLL_FRACLOOPDIV,
				 .stepSize = NXP_PLL_STEPSIZE,
				 .stepNum = NXP_PLL_STEPNUM,
				 .accuracy = NXP_PLL_ACCURACY,
				 .outDiv = NXP_PLL_OUTDIV_POINTER};
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(mc_cgm), nxp_mc_cgm, okay)
const clock_pcfs_config_t pcfs_config = {.maxAllowableIDDchange = NXP_PLL_MAXIDOCHANGE,
					 .stepDuration = NXP_PLL_STEPDURATION,
					 .clkSrcFreq = NXP_PLL_CLKSRCFREQ};
#endif

#define MC_CGM_EMAC_NODE DT_NODELABEL(emac)

/*
 * The EMAC RX/TX/TS source muxes have to be attached before the MAC leaves
 * reset, and which source is correct depends on the PHY interface selected in
 * devicetree. Only set them up on SoCs that have an EMAC and only when it is
 * actually enabled.
 */
#if defined(FSL_FEATURE_CLOCK_HAS_EMAC) && (FSL_FEATURE_CLOCK_HAS_EMAC != 0U) && \
	DT_NODE_HAS_STATUS_OKAY(MC_CGM_EMAC_NODE)
#define MC_CGM_HAS_EMAC 1

/*
 * Clock rates the PHY drives into the SoC, both fixed by IEEE 802.3: RMII
 * supplies a single 50 MHz reference, MII separate transmit and receive clocks
 * that run at 25 MHz for 100 Mbps.
 */
#define MC_CGM_EMAC_RMII_REF_CLK_HZ 50000000U
#define MC_CGM_EMAC_MII_CLK_HZ      25000000U

#if DT_ENUM_HAS_VALUE(MC_CGM_EMAC_NODE, phy_connection_type, rmii)
/*
 * The one reference clock feeds all three domains, and the MAC clocks its
 * MII-side logic at half the RMII rate.
 */
#define MC_CGM_EMAC_TXPAD_CLK_HZ MC_CGM_EMAC_RMII_REF_CLK_HZ
#define MC_CGM_EMAC_RXPAD_CLK_HZ 0U
#define MC_CGM_EMAC_RX_ATTACH    kEMAC_RMII_TX_CLK_to_EMAC_RX
#define MC_CGM_EMAC_RX_SRC       CLOCK_EMAC_RMII_TX_CLK
#define MC_CGM_EMAC_TS_ATTACH    kEMAC_RMII_TX_CLK_to_EMAC_TS
#define MC_CGM_EMAC_TS_SRC       CLOCK_EMAC_RMII_TX_CLK
#define MC_CGM_EMAC_CLK_DIV      2U
#elif DT_ENUM_HAS_VALUE(MC_CGM_EMAC_NODE, phy_connection_type, mii)
/*
 * The PHY drives the transmit and receive clocks on separate pads, already at
 * the MII-side rate. The timestamp unit is fed from the transmit clock, which
 * means it follows the link speed - fine at 100 Mbps, ten times slower at
 * 10 Mbps.
 */
#define MC_CGM_EMAC_TXPAD_CLK_HZ MC_CGM_EMAC_MII_CLK_HZ
#define MC_CGM_EMAC_RXPAD_CLK_HZ MC_CGM_EMAC_MII_CLK_HZ
#define MC_CGM_EMAC_RX_ATTACH    kEMAC_RX_CLK_to_EMAC_RX
#define MC_CGM_EMAC_RX_SRC       CLOCK_EMAC_RX_CLK
#define MC_CGM_EMAC_TS_ATTACH    kEMAC_RMII_TX_CLK_to_EMAC_TS
#define MC_CGM_EMAC_TS_SRC       CLOCK_EMAC_RMII_TX_CLK
#define MC_CGM_EMAC_CLK_DIV      1U
#else
#error "Unsupported PHY connection type for the MCXE Ethernet MAC"
#endif

/* The transmit clock always comes off the same pad, in either mode. */
#define MC_CGM_EMAC_TX_ATTACH kEMAC_RMII_TX_CLK_to_EMAC_TX
#define MC_CGM_EMAC_TX_SRC    CLOCK_EMAC_RMII_TX_CLK
#endif

/*
 * SDK defines FSL_FEATURE_SOC_<IP>_COUNT as `(N)` with parentheses, which
 * breaks Zephyr's LISTIFY (it token-pastes LEN into a macro name and needs
 * a bare integer). Strip the parens before passing to LISTIFY.
 */
#define MC_CGM_UNWRAP(...) __VA_ARGS__
#define MC_CGM_COUNT(n)   MC_CGM_UNWRAP n

/*
 * The SDK exposes peripheral clocks via two distinct enum namespaces:
 *   - clock_ip_name_t (kCLOCK_Lpuart0)    feeds CLOCK_EnableClock() /
 *                                         CLOCK_DisableClock() — the gate
 *                                         path used by _on()/_off().
 *   - clock_name_t   (kCLOCK_Lpuart0Clk)  feeds CLOCK_GetFreq() — the
 *                                         clock-tree query used by
 *                                         _get_rate().
 * Keep two parallel tables so each entry carries the right typed enum,
 * and let _on()/_off() share the gate table.
 *
 * `dt` is the DT-binding prefix (uppercase, e.g. LPUART -> MCUX_LPUART0_CLK);
 * `sdk` is the SDK enum prefix (CamelCase, e.g. Lpuart -> kCLOCK_Lpuart0).
 * LISTIFY iterates 0..(N-1) where N is FSL_FEATURE_SOC_<IP>_COUNT, so
 * undefined SDK identifiers on derivative SoCs are never referenced.
 */
struct mc_cgm_gate_entry {
	uint32_t subsys;
	clock_ip_name_t sdk_enum;
};

struct mc_cgm_rate_entry {
	uint32_t subsys;
	clock_name_t sdk_enum;
};

#define MC_CGM_GATE_ENTRY(i, dt, sdk) \
	{ MCUX_##dt##i##_CLK, kCLOCK_##sdk##i }

#define MC_CGM_RATE_ENTRY(i, dt, sdk) \
	{ MCUX_##dt##i##_CLK, kCLOCK_##sdk##i##Clk }

static const struct mc_cgm_gate_entry mc_cgm_gate_map[] = {
#if defined(CONFIG_CAN_MCUX_FLEXCAN) && defined(FSL_FEATURE_SOC_FLEXCAN_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_FLEXCAN_COUNT),
		MC_CGM_GATE_ENTRY, (,), FLEXCAN, Flexcan),
#endif
#if defined(CONFIG_UART_MCUX_LPUART) && defined(FSL_FEATURE_SOC_LPUART_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPUART_COUNT),
		MC_CGM_GATE_ENTRY, (,), LPUART, Lpuart),
#endif
#if defined(CONFIG_SPI_NXP_LPSPI) && defined(FSL_FEATURE_SOC_LPSPI_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPSPI_COUNT),
		MC_CGM_GATE_ENTRY, (,), LPSPI, Lpspi),
#endif
#if defined(CONFIG_I2C_MCUX_LPI2C) && defined(FSL_FEATURE_SOC_LPI2C_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPI2C_COUNT),
		MC_CGM_GATE_ENTRY, (,), LPI2C, Lpi2c),
#endif
#if (defined(CONFIG_COUNTER_MCUX_STM) || defined(CONFIG_MCUX_STM_TIMER)) && \
	defined(FSL_FEATURE_SOC_STM_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_STM_COUNT),
		MC_CGM_GATE_ENTRY, (,), STM, Stm),
#endif
#if defined(CONFIG_COUNTER_NXP_PIT) && defined(FSL_FEATURE_SOC_PIT_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_PIT_COUNT),
		MC_CGM_GATE_ENTRY, (,), PIT, Pit),
#endif
#if defined(CONFIG_COMPARATOR_NXP_LPCMP) && defined(FSL_FEATURE_SOC_LPCMP_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPCMP_COUNT),
		MC_CGM_GATE_ENTRY, (,), CMP, Lpcmp),
#endif
#if defined(CONFIG_ADC_NXP_SAR_ADC) && defined(FSL_FEATURE_SOC_ADC_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_ADC_COUNT),
		MC_CGM_GATE_ENTRY, (,), ADC, Adc),
#endif
#if defined(CONFIG_I2S_MCUX_SAI) && defined(FSL_FEATURE_SOC_I2S_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_I2S_COUNT),
		MC_CGM_GATE_ENTRY, (,), SAI, Sai),
#endif
#if defined(MC_CGM_HAS_EMAC)
	{ MCUX_EMAC_CLK, kCLOCK_Emac },
#endif
};

/*
 * LPCMP: SDK naming is inconsistent between the two namespaces —
 * clock_ip_name_t uses "Lpcmp" (kCLOCK_Lpcmp0), but clock_name_t
 * drops the "Lp" (kCLOCK_Cmp0Clk). Pass "Cmp" as the SDK prefix
 * here so CLOCK_GetFreq receives the right identifier.
 */
static const struct mc_cgm_rate_entry mc_cgm_rate_map[] = {
#if defined(CONFIG_CAN_MCUX_FLEXCAN) && defined(FSL_FEATURE_SOC_FLEXCAN_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_FLEXCAN_COUNT),
		MC_CGM_RATE_ENTRY, (,), FLEXCAN, Flexcan),
#endif
#if defined(CONFIG_UART_MCUX_LPUART) && defined(FSL_FEATURE_SOC_LPUART_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPUART_COUNT),
		MC_CGM_RATE_ENTRY, (,), LPUART, Lpuart),
#endif
#if defined(CONFIG_SPI_NXP_LPSPI) && defined(FSL_FEATURE_SOC_LPSPI_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPSPI_COUNT),
		MC_CGM_RATE_ENTRY, (,), LPSPI, Lpspi),
#endif
#if defined(CONFIG_I2C_MCUX_LPI2C) && defined(FSL_FEATURE_SOC_LPI2C_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPI2C_COUNT),
		MC_CGM_RATE_ENTRY, (,), LPI2C, Lpi2c),
#endif
#if (defined(CONFIG_COUNTER_MCUX_STM) || defined(CONFIG_MCUX_STM_TIMER)) && \
	defined(FSL_FEATURE_SOC_STM_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_STM_COUNT),
		MC_CGM_RATE_ENTRY, (,), STM, Stm),
#endif
#if defined(CONFIG_COUNTER_NXP_PIT) && defined(FSL_FEATURE_SOC_PIT_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_PIT_COUNT),
		MC_CGM_RATE_ENTRY, (,), PIT, Pit),
#endif
#if defined(CONFIG_COMPARATOR_NXP_LPCMP) && defined(FSL_FEATURE_SOC_LPCMP_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPCMP_COUNT),
		MC_CGM_RATE_ENTRY, (,), CMP, Cmp),
#endif
#if defined(CONFIG_ADC_NXP_SAR_ADC) && defined(FSL_FEATURE_SOC_ADC_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_ADC_COUNT),
		MC_CGM_RATE_ENTRY, (,), ADC, Adc),
#endif
#if defined(CONFIG_I2S_MCUX_SAI) && defined(FSL_FEATURE_SOC_I2S_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_I2S_COUNT),
		MC_CGM_RATE_ENTRY, (,), SAI, Sai),
#endif
};

static const struct mc_cgm_gate_entry *mc_cgm_lookup_gate(uint32_t subsys)
{
	for (size_t i = 0; i < ARRAY_SIZE(mc_cgm_gate_map); i++) {
		if (mc_cgm_gate_map[i].subsys == subsys) {
			return &mc_cgm_gate_map[i];
		}
	}

	return NULL;
}

static const struct mc_cgm_rate_entry *mc_cgm_lookup_rate(uint32_t subsys)
{
	for (size_t i = 0; i < ARRAY_SIZE(mc_cgm_rate_map); i++) {
		if (mc_cgm_rate_map[i].subsys == subsys) {
			return &mc_cgm_rate_map[i];
		}
	}

	return NULL;
}

#if defined(MC_CGM_HAS_EMAC)
/*
 * Every EMAC clock domain is derived from a clock the PHY drives into the SoC,
 * so these must run only once the pads are muxed: the glitchless MC_CGM mux
 * refuses to switch to a source that is not toggling and silently leaves the
 * domain on FIRC, which is close enough to keep framing packets but far enough
 * off to corrupt every one of them. CLOCK_AttachClk() reports success either
 * way, so check the status register.
 */
struct mc_cgm_emac_clk {
	volatile const uint32_t *css;
	uint32_t src;
	clock_attach_id_t attach;
	clock_div_name_t div_name;
};

static const struct mc_cgm_emac_clk mc_cgm_emac_rx_clk = {
	.attach = MC_CGM_EMAC_RX_ATTACH,
	.div_name = kCLOCK_DivEmacRxClk,
	.css = &MC_CGM->MUX_7_CSS,
	.src = MC_CGM_EMAC_RX_SRC,
};

static const struct mc_cgm_emac_clk mc_cgm_emac_tx_clk = {
	.attach = MC_CGM_EMAC_TX_ATTACH,
	.div_name = kCLOCK_DivEmacTxClk,
	.css = &MC_CGM->MUX_8_CSS,
	.src = MC_CGM_EMAC_TX_SRC,
};

static const struct mc_cgm_emac_clk mc_cgm_emac_ts_clk = {
	.attach = MC_CGM_EMAC_TS_ATTACH,
	.div_name = kCLOCK_DivEmacTsClk,
	.css = &MC_CGM->MUX_9_CSS,
	.src = MC_CGM_EMAC_TS_SRC,
};

static int mc_cgm_emac_attach(const struct mc_cgm_emac_clk *clk)
{
	/* Tell the SDK what the PHY drives into the pads; software state only. */
	CLOCK_SetEmacRmiiTxClkFreq(MC_CGM_EMAC_TXPAD_CLK_HZ);
	if (MC_CGM_EMAC_RXPAD_CLK_HZ != 0U) {
		CLOCK_SetEmacRxClkFreq(MC_CGM_EMAC_RXPAD_CLK_HZ);
	}

	if (CLOCK_AttachClk(clk->attach) != kStatus_Success) {
		return -EIO;
	}

	if (FIELD_GET(MC_CGM_MUX_7_CSS_SELSTAT_MASK, *clk->css) != clk->src) {
		LOG_ERR("EMAC clock did not switch to source %u; "
			"is the pin muxed and is the PHY driving it?", clk->src);
		return -EIO;
	}

	if (CLOCK_SetClkDiv(clk->div_name, MC_CGM_EMAC_CLK_DIV) != kStatus_Success) {
		return -EIO;
	}

	return 0;
}
#endif /* defined(MC_CGM_HAS_EMAC) */

static int mc_cgm_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t clock_name = (uint32_t)sub_system;
	const struct mc_cgm_gate_entry *entry = mc_cgm_lookup_gate(clock_name);

	if (entry != NULL) {
		CLOCK_EnableClock(entry->sdk_enum);
		return 0;
	}

	switch (clock_name) {
#if defined(CONFIG_MCUX_FLEXIO)
	case MCUX_FLEXIO_CLK:
		CLOCK_EnableClock(kCLOCK_Flexio);
		return 0;
#endif
#if defined(CONFIG_NXP_TEMPSENSE)
	case MCUX_TEMPSENSE_CLK:
		CLOCK_EnableClock(kCLOCK_TempSensor);
		return 0;
#endif
#if defined(MC_CGM_HAS_EMAC)
	case MCUX_EMACRX_CLK:
		return mc_cgm_emac_attach(&mc_cgm_emac_rx_clk);
	case MCUX_EMACTX_CLK:
		return mc_cgm_emac_attach(&mc_cgm_emac_tx_clk);
	case MCUX_EMACTS_CLK:
		return mc_cgm_emac_attach(&mc_cgm_emac_ts_clk);
#endif
	case MCUX_SIRC_CLK:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mc_cgm_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t clock_name = (uint32_t)sub_system;
	const struct mc_cgm_gate_entry *entry = mc_cgm_lookup_gate(clock_name);

	if (entry != NULL) {
		CLOCK_DisableClock(entry->sdk_enum);
		return 0;
	}

	switch (clock_name) {
#if defined(CONFIG_MCUX_FLEXIO)
	case MCUX_FLEXIO_CLK:
		CLOCK_DisableClock(kCLOCK_Flexio);
		return 0;
#endif
#if defined(CONFIG_NXP_TEMPSENSE)
	case MCUX_TEMPSENSE_CLK:
		CLOCK_DisableClock(kCLOCK_TempSensor);
		return 0;
#endif
#if defined(MC_CGM_HAS_EMAC)
	case MCUX_EMACRX_CLK:
	case MCUX_EMACTX_CLK:
	case MCUX_EMACTS_CLK:
		return 0;
#endif
	case MCUX_SIRC_CLK:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mc_cgm_get_subsys_rate(const struct device *dev, clock_control_subsys_t sub_system,
				  uint32_t *rate)
{
	uint32_t clock_name = (uint32_t)sub_system;
	const struct mc_cgm_rate_entry *entry = mc_cgm_lookup_rate(clock_name);

	if (entry != NULL) {
		*rate = CLOCK_GetFreq(entry->sdk_enum);
		return 0;
	}

	switch (clock_name) {
	case MCUX_SIRC_CLK:
		*rate = CLOCK_SIRC_CLK_FREQ;
		return 0;
	case MCUX_FIRC_CLK:
		*rate = CLOCK_GetFircClkFreq();
		return 0;
	case MCUX_FXOSC_CLK:
		*rate = CLOCK_GetFxoscFreq();
		return 0;
	case MCUX_CORESYS_CLK:
#if defined(CONFIG_MCUX_FLEXIO)
	case MCUX_FLEXIO_CLK:
#endif
		*rate = CLOCK_GetCoreClkFreq();
		return 0;
	case MCUX_AIPSPLAT_CLK:
#if defined(MC_CGM_HAS_EMAC)
	/* The EMAC CSR (register) interface is clocked from AIPS_PLAT_CLK. */
	case MCUX_EMAC_CLK:
#endif
		*rate = CLOCK_GetAipsPlatClkFreq();
		return 0;
#if defined(MC_CGM_HAS_EMAC)
	case MCUX_EMACRX_CLK:
		*rate = CLOCK_GetEmacRxClkFreq();
		return 0;
	case MCUX_EMACTX_CLK:
		*rate = CLOCK_GetEmacTxClkFreq();
		return 0;
	case MCUX_EMACTS_CLK:
		*rate = CLOCK_GetEmacTsClkFreq();
		return 0;
#endif
	case MCUX_HSE_CLK:
		*rate = CLOCK_GetHseClkFreq();
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mc_cgm_init(const struct device *dev)
{
#if defined(FSL_FEATURE_PMC_HAS_LAST_MILE_REGULATOR) && (FSL_FEATURE_PMC_HAS_LAST_MILE_REGULATOR)
	/* Enables PMC last mile regulator before enable PLL.  */
	if ((PMC->LVSC & PMC_LVSC_LVD15S_MASK) != 0U) {
		/* External bipolar junction transistor is connected between external voltage and
		 * V15 input pin.
		 */
		PMC->CONFIG |= PMC_CONFIG_LMBCTLEN_MASK;
	}
	while ((PMC->LVSC & PMC_LVSC_LVD15S_MASK) != 0U) {
	}
	PMC->CONFIG |= PMC_CONFIG_LMEN_MASK;
	while ((PMC->CONFIG & PMC_CONFIG_LMSTAT_MASK) == 0u) {
	}
#endif /* FSL_FEATURE_PMC_HAS_LAST_MILE_REGULATOR */

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(firc), nxp_firc, okay)
	/* Switch the FIRC_DIV_SEL to the desired diveder. */
	CLOCK_SetFircDiv(NXP_FIRC_DIV);
	/* Disable FIRC in standby mode. */
	CLOCK_DisableFircInStandbyMode();
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(sirc), nxp_sirc, okay)
	/* Disable SIRC in standby mode. */
	CLOCK_DisableSircInStandbyMode();
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(fxosc), nxp_fxosc, okay)
	/* Enable FXOSC. */
	CLOCK_InitFxosc(&fxosc_config);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), nxp_plldig, okay)
	/* Enable PLL. */
	CLOCK_InitPll(&pll_config);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(mc_cgm), nxp_mc_cgm, okay)
	CLOCK_SelectSafeClock(kFIRC_CLK_to_MUX0);
	/* Configure MUX_0_CSC dividers */
	CLOCK_SetClkMux0DivTriggerType(KCLOCK_CommonTriggerUpdate);
	CLOCK_SetClkDiv(kCLOCK_DivCoreClk, NXP_PLL_MUX_0_DC_0_DIV);
	CLOCK_SetClkDiv(kCLOCK_DivAipsPlatClk, NXP_PLL_MUX_0_DC_1_DIV);
	CLOCK_SetClkDiv(kCLOCK_DivAipsSlowClk, NXP_PLL_MUX_0_DC_2_DIV);
	CLOCK_SetClkDiv(kCLOCK_DivHseClk, NXP_PLL_MUX_0_DC_3_DIV);
	CLOCK_SetClkDiv(kCLOCK_DivDcmClk, NXP_PLL_MUX_0_DC_4_DIV);
#ifdef MC_CGM_MUX_0_DC_5_DIV_MASK
	CLOCK_SetClkDiv(kCLOCK_DivLbistClk, NXP_PLL_MUX_0_DC_5_DIV);
#endif
#ifdef MC_CGM_MUX_0_DC_6_DIV_MASK
	CLOCK_SetClkDiv(kCLOCK_DivQspiClk, NXP_PLL_MUX_0_DC_6_DIV);
#endif
	CLOCK_CommonTriggerClkMux0DivUpdate();
	CLOCK_ProgressiveClockFrequencySwitch(kPLL_PHI0_CLK_to_MUX0, &pcfs_config);

	/*
	 * TODO(ZEP-5899): The peripheral clock source attachments below (STM,
	 * FlexCAN) are hardcoded. A follow-up change will move them to
	 * devicetree so customers can override clock sources from board
	 * overlays without modifying the driver.
	 */
#if defined(CONFIG_COUNTER_MCUX_STM) || defined(CONFIG_MCUX_STM_TIMER)
	CLOCK_SetClkDiv(kCLOCK_DivStm0Clk, NXP_PLL_MUX_1_DC_0_DIV);
	CLOCK_AttachClk(kAIPS_PLAT_CLK_to_STM0);
#if defined(FSL_FEATURE_SOC_STM_COUNT) && (FSL_FEATURE_SOC_STM_COUNT == 2U)
	CLOCK_SetClkDiv(kCLOCK_DivStm1Clk, NXP_PLL_MUX_2_DC_0_DIV);
	CLOCK_AttachClk(kAIPS_PLAT_CLK_to_STM1);
#endif /* FSL_FEATURE_SOC_STM_COUNT == 2U */
#endif /* defined(CONFIG_COUNTER_MCUX_STM) || defined(CONFIG_MCUX_STM_TIMER) */
#endif
#if defined(CONFIG_CAN_MCUX_FLEXCAN)
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan_0)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan_1)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan_2))
		CLOCK_SetClkDiv(kCLOCK_DivFlexcan012PeClk, 1U);
		CLOCK_AttachClk(kAIPS_PLAT_CLK_to_FLEXCAN012_PE);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan_3)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan_4)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan_5))
		CLOCK_SelectSafeClock(kFIRC_CLK_to_FLEXCAN345_PE);
		CLOCK_SetClkDiv(kCLOCK_DivFlexcan345PeClk, 1U);
#endif
#endif /* defined(CONFIG_CAN_MCUX_FLEXCAN) */

	/* Set SystemCoreClock variable. */
	SystemCoreClockUpdate();

	return 0;
}

static DEVICE_API(clock_control, mcux_mcxe31x_clock_api) = {
	.on = mc_cgm_clock_control_on,
	.off = mc_cgm_clock_control_off,
	.get_rate = mc_cgm_get_subsys_rate,
};

DEVICE_DT_INST_DEFINE(0, mc_cgm_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &mcux_mcxe31x_clock_api);
