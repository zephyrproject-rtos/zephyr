/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cy_sysclk.h>
#include <cy_systick.h>

#define IFX_CAT1_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(block) ((cy_en_divider_types_t)((block) & 0x03))

/* Converts the group/div pair into a unique block number. */
#define IFX_CAT1_PERIPHERAL_GROUP_ADJUST(group, div) (((group) << 2) | (div))

#define IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(gr)                                                        \
	IFX_CAT1_CLOCK_BLOCK_PERIPHERAL##gr##_8BIT = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(             \
		(gr), CY_SYSCLK_DIV_8_BIT), /*!< 8bit Peripheral Divider Group */                  \
		IFX_CAT1_CLOCK_BLOCK_PERIPHERAL##gr##_16BIT = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(    \
			(gr), CY_SYSCLK_DIV_16_BIT), /*!< 16bit Peripheral Divider Group */        \
		IFX_CAT1_CLOCK_BLOCK_PERIPHERAL##gr##_16_5BIT = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(  \
			(gr), CY_SYSCLK_DIV_16_5_BIT), /*!< 16.5bit Peripheral Divider Group */    \
		IFX_CAT1_CLOCK_BLOCK_PERIPHERAL##gr##_24_5BIT = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(  \
			(gr), CY_SYSCLK_DIV_24_5_BIT) /*!< 24.5bit Peripheral Divider Group */

enum ifx_cat1_resource {
	IFX_CAT1_RSC_ADC,     /*!< Analog to digital converter */
	IFX_CAT1_RSC_ADCMIC,  /*!< Analog to digital converter with Analog Mic support */
	IFX_CAT1_RSC_BLESS,   /*!< Bluetooth communications block */
	IFX_CAT1_RSC_CAN,     /*!< CAN communication block */
	IFX_CAT1_RSC_CLKPATH, /*!< Clock Path. DEPRECATED. */
	IFX_CAT1_RSC_CLOCK,   /*!< Clock */
	IFX_CAT1_RSC_CRYPTO,  /*!< Crypto hardware accelerator */
	IFX_CAT1_RSC_DAC,     /*!< Digital to analog converter */
	IFX_CAT1_RSC_DMA,     /*!< DMA controller */
	IFX_CAT1_RSC_DW,      /*!< Datawire DMA controller */
	IFX_CAT1_RSC_ETH,     /*!< Ethernet communications block */
	IFX_CAT1_RSC_GPIO,    /*!< General purpose I/O pin */
	IFX_CAT1_RSC_I2S,     /*!< I2S communications block */
	IFX_CAT1_RSC_I3C,     /*!< I3C communications block */
	IFX_CAT1_RSC_KEYSCAN, /*!< KeyScan block */
	IFX_CAT1_RSC_LCD,     /*!< Segment LCD controller */
	IFX_CAT1_RSC_LIN,     /*!< LIN communications block */
	IFX_CAT1_RSC_LPCOMP,  /*!< Low power comparator */
	IFX_CAT1_RSC_LPTIMER, /*!< Low power timer */
	IFX_CAT1_RSC_OPAMP,   /*!< Opamp */
	IFX_CAT1_RSC_PDM,     /*!< PCM/PDM communications block */
	IFX_CAT1_RSC_PTC,     /*!< Programmable Threshold comparator */
	IFX_CAT1_RSC_SMIF,    /*!< Quad-SPI communications block */
	IFX_CAT1_RSC_RTC,     /*!< Real time clock */
	IFX_CAT1_RSC_SCB,     /*!< Serial Communications Block */
	IFX_CAT1_RSC_SDHC,    /*!< SD Host Controller */
	IFX_CAT1_RSC_SDIODEV, /*!< SDIO Device Block */
	IFX_CAT1_RSC_TCPWM,   /*!< Timer/Counter/PWM block */
	IFX_CAT1_RSC_TDM,     /*!< TDM block */
	IFX_CAT1_RSC_UDB,     /*!< UDB Array */
	IFX_CAT1_RSC_USB,     /*!< USB communication block */
	IFX_CAT1_RSC_INVALID, /*!< Placeholder for invalid type */
};

enum ifx_cat1_clock_block {
#if defined(CONFIG_SOC_FAMILY_INFINEON_CAT1A)
	/* The first four items are here for backwards compatibility with old clock APIs */
	IFX_CAT1_CLOCK_BLOCK_PERIPHERAL_8BIT = CY_SYSCLK_DIV_8_BIT, /*!< 8bit Peripheral Divider */
	IFX_CAT1_CLOCK_BLOCK_PERIPHERAL_16BIT =
		CY_SYSCLK_DIV_16_BIT, /*!< 16bit Peripheral Divider */
	IFX_CAT1_CLOCK_BLOCK_PERIPHERAL_16_5BIT =
		CY_SYSCLK_DIV_16_5_BIT, /*!< 16.5bit Peripheral Divider */
	IFX_CAT1_CLOCK_BLOCK_PERIPHERAL_24_5BIT =
		CY_SYSCLK_DIV_24_5_BIT, /*!< 24.5bit Peripheral Divider */

	IFX_CAT1_CLOCK_BLOCK_IMO,   /*!< Internal Main Oscillator Input Clock */
	IFX_CAT1_CLOCK_BLOCK_ECO,   /*!< External Crystal Oscillator Input Clock */
	IFX_CAT1_CLOCK_BLOCK_EXT,   /*!< External Input Clock */
	IFX_CAT1_CLOCK_BLOCK_ALTHF, /*!< Alternate High Frequency Input Clock */
	IFX_CAT1_CLOCK_BLOCK_ALTLF, /*!< Alternate Low Frequency Input Clock */
	IFX_CAT1_CLOCK_BLOCK_ILO,   /*!< Internal Low Speed Oscillator Input Clock */
#if !(defined(SRSS_HT_VARIANT) && (SRSS_HT_VARIANT > 0))
	IFX_CAT1_CLOCK_BLOCK_PILO, /*!< Precision ILO Input Clock */
#endif

	IFX_CAT1_CLOCK_BLOCK_WCO, /*!< Watch Crystal Oscillator Input Clock */
	IFX_CAT1_CLOCK_BLOCK_MFO, /*!< Medium Frequency Oscillator Clock */

	IFX_CAT1_CLOCK_BLOCK_PATHMUX, /*!< Path selection mux for input to FLL/PLLs */

	IFX_CAT1_CLOCK_BLOCK_FLL, /*!< Frequency-Locked Loop Clock */
#if defined(CY_IP_MXS40SRSS) && (CY_IP_MXS40SRSS_VERSION >= 3)
	IFX_CAT1_CLOCK_BLOCK_PLL200, /*!< 200MHz Phase-Locked Loop Clock */
	IFX_CAT1_CLOCK_BLOCK_PLL400, /*!< 400MHz Phase-Locked Loop Clock */
#else
	IFX_CAT1_CLOCK_BLOCK_PLL, /*!< Phase-Locked Loop Clock */
#endif

	IFX_CAT1_CLOCK_BLOCK_LF, /*!< Low Frequency Clock */
	IFX_CAT1_CLOCK_BLOCK_MF, /*!< Medium Frequency Clock */
	IFX_CAT1_CLOCK_BLOCK_HF, /*!< High Frequency Clock */

	IFX_CAT1_CLOCK_BLOCK_PUMP,         /*!< Analog Pump Clock */
	IFX_CAT1_CLOCK_BLOCK_BAK,          /*!< Backup Power Domain Clock */
	IFX_CAT1_CLOCK_BLOCK_TIMER,        /*!< Timer Clock */
	IFX_CAT1_CLOCK_BLOCK_ALT_SYS_TICK, /*!< Alternative SysTick Clock */

	IFX_CAT1_CLOCK_BLOCK_FAST, /*!< Fast Clock for CM4 */
	IFX_CAT1_CLOCK_BLOCK_PERI, /*!< Peripheral Clock */
	IFX_CAT1_CLOCK_BLOCK_SLOW, /*!< Slow Clock for CM0+ */

#elif defined(COMPONENT_CAT1B)

	IFX_CAT1_CLOCK_BLOCK_PERIPHERAL_8BIT =
		CY_SYSCLK_DIV_8_BIT, /*!< Equivalent to IFX_CAT1_CLOCK_BLOCK_PERIPHERAL0_8_BIT */
	IFX_CAT1_CLOCK_BLOCK_PERIPHERAL_16BIT =
		CY_SYSCLK_DIV_16_BIT, /*!< Equivalent to IFX_CAT1_CLOCK_BLOCK_PERIPHERAL0_16_BIT */
	IFX_CAT1_CLOCK_BLOCK_PERIPHERAL_16_5BIT =
		CY_SYSCLK_DIV_16_5_BIT, /*!< Equivalent to IFX_CAT1_CLOCK_BLOCK_PERIPHERAL0_16_5_BIT
					 */
	IFX_CAT1_CLOCK_BLOCK_PERIPHERAL_24_5BIT =
		CY_SYSCLK_DIV_24_5_BIT, /*!< Equivalent to IFX_CAT1_CLOCK_BLOCK_PERIPHERAL0_24_5_BIT
					 */

/* The first four items are here for backwards compatibility with old clock APIs */
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 1)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(0),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 2)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(1),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 3)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(2),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 4)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(3),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 5)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(4),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 6)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(5),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 7)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(6),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 8)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(7),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 9)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(8),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 10)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(9),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 11)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(10),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 12)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(11),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 13)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(12),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 14)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(13),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 15)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(14),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 16)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(15),
#endif

	IFX_CAT1_CLOCK_BLOCK_IHO,   /*!< Internal High Speed Oscillator Input Clock */
	IFX_CAT1_CLOCK_BLOCK_IMO,   /*!< Internal Main Oscillator Input Clock */
	IFX_CAT1_CLOCK_BLOCK_ECO,   /*!< External Crystal Oscillator Input Clock */
	IFX_CAT1_CLOCK_BLOCK_EXT,   /*!< External Input Clock */
	IFX_CAT1_CLOCK_BLOCK_ALTHF, /*!< Alternate High Frequency Input Clock */
	IFX_CAT1_CLOCK_BLOCK_ALTLF, /*!< Alternate Low Frequency Input Clock */
	IFX_CAT1_CLOCK_BLOCK_ILO,   /*!< Internal Low Speed Oscillator Input Clock */
	IFX_CAT1_CLOCK_BLOCK_PILO,  /*!< Precision ILO Input Clock */
	IFX_CAT1_CLOCK_BLOCK_WCO,   /*!< Watch Crystal Oscillator Input Clock */
	IFX_CAT1_CLOCK_BLOCK_MFO,   /*!< Medium Frequency Oscillator Clock */

	IFX_CAT1_CLOCK_BLOCK_PATHMUX, /*!< Path selection mux for input to FLL/PLLs */

	IFX_CAT1_CLOCK_BLOCK_FLL,           /*!< Frequency-Locked Loop Clock */
	IFX_CAT1_CLOCK_BLOCK_PLL200,        /*!< 200MHz Phase-Locked Loop Clock */
	IFX_CAT1_CLOCK_BLOCK_PLL400,        /*!< 400MHz Phase-Locked Loop Clock */
	IFX_CAT1_CLOCK_BLOCK_ECO_PRESCALER, /*!< ECO Prescaler Divider */

	IFX_CAT1_CLOCK_BLOCK_LF, /*!< Low Frequency Clock */
	IFX_CAT1_CLOCK_BLOCK_MF, /*!< Medium Frequency Clock */
	IFX_CAT1_CLOCK_BLOCK_HF, /*!< High Frequency Clock */

	IFX_CAT1_CLOCK_BLOCK_PUMP,         /*!< Analog Pump Clock */
	IFX_CAT1_CLOCK_BLOCK_BAK,          /*!< Backup Power Domain Clock */
	IFX_CAT1_CLOCK_BLOCK_ALT_SYS_TICK, /*!< Alternative SysTick Clock */
	IFX_CAT1_CLOCK_BLOCK_PERI,         /*!< Peripheral Clock Group */

#elif defined(COMPONENT_CAT1C)

	IFX_CAT1_CLOCK_BLOCK_PERIPHERAL_8BIT =
		CY_SYSCLK_DIV_8_BIT, /*!< Equivalent to IFX_CAT1_CLOCK_BLOCK_PERIPHERAL0_8_BIT */
	IFX_CAT1_CLOCK_BLOCK_PERIPHERAL_16BIT =
		CY_SYSCLK_DIV_16_BIT, /*!< Equivalent to IFX_CAT1_CLOCK_BLOCK_PERIPHERAL0_16_BIT */
	IFX_CAT1_CLOCK_BLOCK_PERIPHERAL_16_5BIT =
		CY_SYSCLK_DIV_16_5_BIT, /*!< Equivalent to IFX_CAT1_CLOCK_BLOCK_PERIPHERAL0_16_5_BIT
					 */
	IFX_CAT1_CLOCK_BLOCK_PERIPHERAL_24_5BIT =
		CY_SYSCLK_DIV_24_5_BIT, /*!< Equivalent to IFX_CAT1_CLOCK_BLOCK_PERIPHERAL0_24_5_BIT
					 */

/* The first four items are here for backwards compatibility with old clock APIs */
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 1)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(0),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 2)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(1),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 3)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(2),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 4)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(3),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 5)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(4),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 6)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(5),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 7)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(6),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 8)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(7),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 9)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(8),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 10)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(9),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 11)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(10),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 12)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(11),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 13)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(12),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 14)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(13),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 15)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(14),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 16)
	IFX_CAT1_CLOCK_BLOCK_PERI_GROUP(15),
#endif

	IFX_CAT1_CLOCK_BLOCK_IMO, /*!< Internal Main Oscillator Input Clock */
	IFX_CAT1_CLOCK_BLOCK_ECO, /*!< External Crystal Oscillator Input Clock */
	IFX_CAT1_CLOCK_BLOCK_EXT, /*!< External Input Clock */
	IFX_CAT1_CLOCK_BLOCK_ILO, /*!< Internal Low Speed Oscillator Input Clock */
	IFX_CAT1_CLOCK_BLOCK_WCO, /*!< Watch Crystal Oscillator Input Clock */

	IFX_CAT1_CLOCK_BLOCK_PATHMUX, /*!< Path selection mux for input to FLL/PLLs */

	IFX_CAT1_CLOCK_BLOCK_FLL,    /*!< Frequency-Locked Loop Clock */
	IFX_CAT1_CLOCK_BLOCK_PLL200, /*!< 200MHz Phase-Locked Loop Clock */
	IFX_CAT1_CLOCK_BLOCK_PLL400, /*!< 400MHz Phase-Locked Loop Clock */

	IFX_CAT1_CLOCK_BLOCK_LF,           /*!< Low Frequency Clock */
	IFX_CAT1_CLOCK_BLOCK_HF,           /*!< High Frequency Clock */
	IFX_CAT1_CLOCK_BLOCK_BAK,          /*!< Backup Power Domain Clock */
	IFX_CAT1_CLOCK_BLOCK_ALT_SYS_TICK, /*!< Alternative SysTick Clock */

	IFX_CAT1_CLOCK_BLOCK_PERI,  /*!< Peripheral Clock Group */
	IFX_CAT1_CLOCK_BLOCK_FAST,  /*!< Fast Clock for CM7 */
	IFX_CAT1_CLOCK_BLOCK_SLOW,  /*!< Slow Clock for CM0+ */
	IFX_CAT1_CLOCK_BLOCK_MEM,   /*!< CLK MEM */
	IFX_CAT1_CLOCK_BLOCK_TIMER, /*!< CLK Timer */
#endif
};

struct ifx_cat1_clock {
	enum ifx_cat1_clock_block block;
	uint8_t channel;
	bool reserved;
	const void *funcs;
};

struct ifx_cat1_resource_inst {
	enum ifx_cat1_resource type; /* !< The resource block type */
	uint8_t block_num;           /* !< The resource block index */
	/**
	 * The channel number, if the resource type defines multiple channels
	 * per block instance. Otherwise, 0
	 */
	uint8_t channel_num;
};

int ifx_cat1_clock_control_get_frequency(uint32_t dt_ord, uint32_t *frequency);

en_clk_dst_t ifx_cat1_scb_get_clock_index(uint32_t block_num);

static inline cy_rslt_t ifx_cat1_utils_peri_pclk_enable_divider(en_clk_dst_t clk_dest,
								const struct ifx_cat1_clock *_clock)
{
#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C)
	return Cy_SysClk_PeriPclkEnableDivider(
		clk_dest, IFX_CAT1_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(_clock->block),
		_clock->channel);
#else
	CY_UNUSED_PARAMETER(clk_dest);
	return Cy_SysClk_PeriphEnableDivider(
		IFX_CAT1_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(_clock->block), _clock->channel);
#endif
}

static inline cy_rslt_t ifx_cat1_utils_peri_pclk_set_divider(en_clk_dst_t clk_dest,
							     const struct ifx_cat1_clock *_clock,
							     uint32_t div)
{
#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C)
	return Cy_SysClk_PeriPclkSetDivider(
		clk_dest, IFX_CAT1_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(_clock->block),
		_clock->channel, div);
#else
	CY_UNUSED_PARAMETER(clk_dest);
	return Cy_SysClk_PeriphSetDivider(IFX_CAT1_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(_clock->block),
					  _clock->channel, div);
#endif
}

static inline cy_rslt_t
ifx_cat1_utils_peri_pclk_set_frac_divider(en_clk_dst_t clk_dest,
					  const struct ifx_cat1_clock *_clock, uint32_t div_int,
					  uint32_t div_frac)
{
#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C)
	return Cy_SysClk_PeriPclkSetFracDivider(
		clk_dest, IFX_CAT1_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(_clock->block),
		_clock->channel, div_int, div_frac);
#else
	CY_UNUSED_PARAMETER(clk_dest);
	return Cy_SysClk_PeriphSetFracDivider(
		IFX_CAT1_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(_clock->block), _clock->channel, div_int,
		div_frac);
#endif
}

static inline cy_rslt_t ifx_cat1_utils_peri_pclk_assign_divider(en_clk_dst_t clk_dest,
								const struct ifx_cat1_clock *_clock)
{
#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C) || defined(COMPONENT_CAT1D)
	return Cy_SysClk_PeriPclkAssignDivider(
		clk_dest, IFX_CAT1_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(_clock->block),
		_clock->channel);
#else
	return Cy_SysClk_PeriphAssignDivider(
		clk_dest, IFX_CAT1_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(_clock->block),
		_clock->channel);
#endif
}
