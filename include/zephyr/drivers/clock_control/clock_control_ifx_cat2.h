/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_IFX_CAT2_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_IFX_CAT2_H_

#include <zephyr/drivers/clock_control.h>
#include <cy_sysclk.h>
#include <cy_systick.h>

#define IFX_CAT2_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(block) ((cy_en_divider_types_t)((block) & 0x03))

/* Converts the group/div pair into a unique block number. */
#define IFX_CAT2_PERIPHERAL_GROUP_ADJUST(group, div) (((group) << 2) | (div))

#define IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(gr)                                                        \
		IFX_CAT2_CLOCK_BLOCK_PERIPHERAL##gr##_16BIT = IFX_CAT2_PERIPHERAL_GROUP_ADJUST(    \
			(gr), CY_SYSCLK_DIV_16_BIT), /*!< 16bit Peripheral Divider Group */        \
		IFX_CAT2_CLOCK_BLOCK_PERIPHERAL##gr##_16_5BIT = IFX_CAT2_PERIPHERAL_GROUP_ADJUST(  \
			(gr), CY_SYSCLK_DIV_16_5_BIT), /*!< 16.5bit Peripheral Divider Group */    \
		IFX_CAT2_CLOCK_BLOCK_PERIPHERAL##gr##_24_5BIT = IFX_CAT2_PERIPHERAL_GROUP_ADJUST(  \
			(gr), CY_SYSCLK_DIV_24_5_BIT) /*!< 24.5bit Peripheral Divider Group */

enum ifx_cat2_resource {
	IFX_CAT2_RSC_ADC,     /*!< Analog to digital converter */
	IFX_CAT2_RSC_ADCMIC,  /*!< Analog to digital converter with Analog Mic support */
	IFX_CAT2_RSC_BLESS,   /*!< Bluetooth communications block */
	IFX_CAT2_RSC_CAN,     /*!< CAN communication block */
	IFX_CAT2_RSC_CLKPATH, /*!< Clock Path. DEPRECATED. */
	IFX_CAT2_RSC_CLOCK,   /*!< Clock */
	IFX_CAT2_RSC_CRYPTO,  /*!< Crypto hardware accelerator */
	IFX_CAT2_RSC_DAC,     /*!< Digital to analog converter */
	IFX_CAT2_RSC_DMA,     /*!< DMA controller */
	IFX_CAT2_RSC_DW,      /*!< Datawire DMA controller */
	IFX_CAT2_RSC_ETH,     /*!< Ethernet communications block */
	IFX_CAT2_RSC_GPIO,    /*!< General purpose I/O pin */
	IFX_CAT2_RSC_I2S,     /*!< I2S communications block */
	IFX_CAT2_RSC_I3C,     /*!< I3C communications block */
	IFX_CAT2_RSC_KEYSCAN, /*!< KeyScan block */
	IFX_CAT2_RSC_LCD,     /*!< Segment LCD controller */
	IFX_CAT2_RSC_LIN,     /*!< LIN communications block */
	IFX_CAT2_RSC_LPCOMP,  /*!< Low power comparator */
	IFX_CAT2_RSC_LPTIMER, /*!< Low power timer */
	IFX_CAT2_RSC_OPAMP,   /*!< Opamp */
	IFX_CAT2_RSC_PDM,     /*!< PCM/PDM communications block */
	IFX_CAT2_RSC_PTC,     /*!< Programmable Threshold comparator */
	IFX_CAT2_RSC_SMIF,    /*!< Quad-SPI communications block */
	IFX_CAT2_RSC_RTC,     /*!< Real time clock */
	IFX_CAT2_RSC_SCB,     /*!< Serial Communications Block */
	IFX_CAT2_RSC_SDHC,    /*!< SD Host Controller */
	IFX_CAT2_RSC_SDIODEV, /*!< SDIO Device Block */
	IFX_CAT2_RSC_TCPWM,   /*!< Timer/Counter/PWM block */
	IFX_CAT2_RSC_TDM,     /*!< TDM block */
	IFX_CAT2_RSC_UDB,     /*!< UDB Array */
	IFX_CAT2_RSC_USB,     /*!< USB communication block */
	IFX_CAT2_RSC_INVALID, /*!< Placeholder for invalid type */
};

enum ifx_cat2_clock_block {
	IFX_CAT2_CLOCK_BLOCK_PERIPHERAL_16BIT =
		CY_SYSCLK_DIV_16_BIT, /*!< Equivalent to IFX_CAT2_CLOCK_BLOCK_PERIPHERAL0_16_BIT */
	IFX_CAT2_CLOCK_BLOCK_PERIPHERAL_16_5BIT =
		CY_SYSCLK_DIV_16_5_BIT, /*!< Equivalent to IFX_CAT2_CLOCK_BLOCK_PERIPHERAL0_16_5_BIT
					 */
	IFX_CAT2_CLOCK_BLOCK_PERIPHERAL_24_5BIT =
		CY_SYSCLK_DIV_24_5_BIT, /*!< Equivalent to IFX_CAT2_CLOCK_BLOCK_PERIPHERAL0_24_5_BIT
					 */

/* The first four items are here for backwards compatibility with old clock APIs */
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 1)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(0),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 2)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(1),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 3)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(2),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 4)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(3),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 5)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(4),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 6)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(5),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 7)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(6),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 8)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(7),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 9)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(8),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 10)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(9),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 11)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(10),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 12)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(11),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 13)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(12),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 14)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(13),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 15)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(14),
#endif
#if (PERI_PERI_PCLK_PCLK_GROUP_NR >= 16)
	IFX_CAT2_CLOCK_BLOCK_PERI_GROUP(15),
#endif

	IFX_CAT2_CLOCK_BLOCK_IMO,   /*!< Internal Main Oscillator Input Clock */
	IFX_CAT2_CLOCK_BLOCK_ECO,   /*!< External Crystal Oscillator Input Clock */
	IFX_CAT2_CLOCK_BLOCK_EXT,   /*!< External Input Clock */
	IFX_CAT2_CLOCK_BLOCK_ALTHF, /*!< Alternate High Frequency Input Clock */
	IFX_CAT2_CLOCK_BLOCK_ALTLF, /*!< Alternate Low Frequency Input Clock */
	IFX_CAT2_CLOCK_BLOCK_ILO,   /*!< Internal Low Speed Oscillator Input Clock */
	IFX_CAT2_CLOCK_BLOCK_PILO,  /*!< Precision ILO Input Clock */
	IFX_CAT2_CLOCK_BLOCK_WCO,   /*!< Watch Crystal Oscillator Input Clock */
	IFX_CAT2_CLOCK_BLOCK_MFO,   /*!< Medium Frequency Oscillator Clock */

	IFX_CAT2_CLOCK_BLOCK_PATHMUX, /*!< Path selection mux for input to FLL/PLLs */

	IFX_CAT2_CLOCK_BLOCK_FLL,           /*!< Frequency-Locked Loop Clock */
	IFX_CAT2_CLOCK_BLOCK_PLL200,        /*!< 200MHz Phase-Locked Loop Clock */
	IFX_CAT2_CLOCK_BLOCK_PLL400,        /*!< 400MHz Phase-Locked Loop Clock */
	IFX_CAT2_CLOCK_BLOCK_ECO_PRESCALER, /*!< ECO Prescaler Divider */

	IFX_CAT2_CLOCK_BLOCK_LF, /*!< Low Frequency Clock */
	IFX_CAT2_CLOCK_BLOCK_MF, /*!< Medium Frequency Clock */
	IFX_CAT2_CLOCK_BLOCK_HF, /*!< High Frequency Clock */

	IFX_CAT2_CLOCK_BLOCK_PUMP,         /*!< Analog Pump Clock */
	IFX_CAT2_CLOCK_BLOCK_BAK,          /*!< Backup Power Domain Clock */
	IFX_CAT2_CLOCK_BLOCK_ALT_SYS_TICK, /*!< Alternative SysTick Clock */
	IFX_CAT2_CLOCK_BLOCK_PERI,         /*!< Peripheral Clock Group */

};

struct ifx_cat2_clock {
	enum ifx_cat2_clock_block block;
	uint8_t channel;
	bool reserved;
	const void *funcs;
};

struct ifx_cat2_resource_inst {
	enum ifx_cat2_resource type; /* !< The resource block type */
	uint8_t block_num;           /* !< The resource block index */
	/**
	 * The channel number, if the resource type defines multiple channels
	 * per block instance. Otherwise, 0
	 */
	uint8_t channel_num;
};

int ifx_cat2_clock_control_get_frequency(uint32_t dt_ord, uint32_t *frequency);

en_clk_dst_t ifx_cat2_scb_get_clock_index(uint32_t block_num);

static inline cy_rslt_t ifx_cat2_utils_peri_pclk_disable_divider(en_clk_dst_t clk_dest,
		const struct ifx_cat2_clock *_clock)
{
	CY_UNUSED_PARAMETER(clk_dest);
	return Cy_SysClk_PeriphDisableDivider(
			IFX_CAT2_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(_clock->block), _clock->channel);
}

static inline cy_rslt_t ifx_cat2_utils_peri_pclk_enable_divider(en_clk_dst_t clk_dest,
		const struct ifx_cat2_clock *_clock)
{
	CY_UNUSED_PARAMETER(clk_dest);
	return Cy_SysClk_PeriphEnableDivider(
			IFX_CAT2_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(_clock->block), _clock->channel);
}

static inline cy_rslt_t ifx_cat2_utils_peri_pclk_set_divider(en_clk_dst_t clk_dest,
		const struct ifx_cat2_clock *_clock,
		uint32_t div)
{
	CY_UNUSED_PARAMETER(clk_dest);
	return Cy_SysClk_PeriphSetDivider(IFX_CAT2_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(_clock->block),
			_clock->channel, div);
}

	static inline cy_rslt_t
ifx_cat2_utils_peri_pclk_set_frac_divider(en_clk_dst_t clk_dest,
		const struct ifx_cat2_clock *_clock, uint32_t div_int,
		uint32_t div_frac)
{
	CY_UNUSED_PARAMETER(clk_dest);
	return Cy_SysClk_PeriphSetFracDivider(
			IFX_CAT2_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(_clock->block), _clock->channel,
			div_int, div_frac);
}

static inline cy_rslt_t ifx_cat2_utils_peri_pclk_assign_divider(en_clk_dst_t clk_dest,
		const struct ifx_cat2_clock *_clock)
{
	return Cy_SysClk_PeriphAssignDivider(
			clk_dest, IFX_CAT2_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(_clock->block),
			_clock->channel);
}
#endif
