/*
 * Copyright (c) 2022 Andrei-Edward Popa
 * Copyright (c) 2023 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_clock_controller

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#if defined(CONFIG_SOC_SERIES_RP2040)
#include <zephyr/dt-bindings/clock/rpi_pico_rp2040_clock.h>
#else
#include <zephyr/dt-bindings/clock/rpi_pico_rp2350_clock.h>
#endif

#include <hardware/clocks.h>
#include <hardware/xosc.h>
#include <hardware/structs/rosc.h>
#include <hardware/pll.h>
#include <hardware/watchdog.h>
#include <hardware/resets.h>
#include <hardware/ticks.h>

/* Undefine to prevent conflicts with header definitions */
#undef pll_sys
#undef pll_usb

#define CTRL_SRC_LSB     CLOCKS_CLK_REF_CTRL_SRC_LSB
#define CTRL_SRC_BITS    CLOCKS_CLK_REF_CTRL_SRC_BITS
#define CTRL_AUXSRC_LSB  CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB
#define CTRL_AUXSRC_BITS CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_BITS
#define CTRL_ENABLE_BITS CLOCKS_CLK_GPOUT0_CTRL_ENABLE_BITS
#define DIV_FRAC_BITS    CLOCKS_CLK_GPOUT0_DIV_FRAC_BITS
#define DIV_INT_BITS     CLOCKS_CLK_GPOUT0_DIV_INT_BITS
#define DIV_INT_LSB      CLOCKS_CLK_GPOUT0_DIV_INT_LSB

#define PLL_VCO_FREQ_MIN 750000000
#define PLL_VCO_FREQ_MAX 1600000000
#define PLL_FB_DIV_MIN   16
#define PLL_FB_DIV_MAX   320
#define PLL_POST_DIV_MIN 1
#define PLL_POST_DIV_MAX 7

#define ROSC_PHASE_PASSWD_VALUE_PASS _u(0xAA)

#define STAGE_DS(n)                                                                                \
	(COND_CODE_1(                                                                              \
		 DT_PROP_HAS_IDX(DT_INST_CLOCKS_CTLR_BY_NAME(0, rosc), stage_drive_strength, n),   \
		 (DT_PROP_BY_IDX(DT_INST_CLOCKS_CTLR_BY_NAME(0, rosc), stage_drive_strength, n) &  \
		  ROSC_FREQA_DS0_BITS),                                                            \
		 (0))                                                                              \
	 << (n * 3))

#define CLK_SRC_IS(clk, src)                                                                       \
	DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk), 0),                \
		     DT_INST_CLOCKS_CTLR_BY_NAME(0, src))

#define REF_DIV(pll)   DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll), clock_div)
#define FB_DIV(pll)    DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll), fb_div)
#define POST_DIV1(pll) DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll), post_div1)
#define POST_DIV2(pll) DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll), post_div2)
#define VCO_FREQ(pll)  ((CLOCK_FREQ_xosc / REF_DIV(pll)) * FB_DIV(pll))

/*
 * Using the 'clock-names[0]' for expanding macro to frequency value.
 * The 'clock-names[0]' is set same as label value that given to the node itself.
 * Use it for traverse clock tree to find root of clock source.
 */
#define CLOCK_FREQ(clk)     _CONCAT(CLOCK_FREQ_, clk)
#define SRC_CLOCK(clk)      DT_STRING_TOKEN_BY_IDX(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk),            \
						   clock_names, 0)
#define SRC_CLOCK_FREQ(clk) _CONCAT(CLOCK_FREQ_, SRC_CLOCK(clk))

#define PLL_FREQ(pll)                                                                              \
	(DT_PROP(DT_CLOCKS_CTLR_BY_IDX(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll), 0), clock_frequency) / \
	 REF_DIV(pll) * FB_DIV(pll) / POST_DIV1(pll) / POST_DIV2(pll))

#define CLOCK_FREQ_clk_gpout0 DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_gpout0), clock_frequency)
#define CLOCK_FREQ_clk_gpout1 DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_gpout1), clock_frequency)
#define CLOCK_FREQ_clk_gpout2 DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_gpout2), clock_frequency)
#define CLOCK_FREQ_clk_gpout3 DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_gpout3), clock_frequency)
#define CLOCK_FREQ_clk_hstx   DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_hstx), clock_frequency)
#define CLOCK_FREQ_clk_ref    DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_ref), clock_frequency)
#define CLOCK_FREQ_clk_sys    DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_sys), clock_frequency)
#define CLOCK_FREQ_clk_usb    DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_usb), clock_frequency)
#define CLOCK_FREQ_clk_adc    DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_adc), clock_frequency)
#define CLOCK_FREQ_clk_rtc    DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_rtc), clock_frequency)
#define CLOCK_FREQ_clk_peri   DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_peri), clock_frequency)
#define CLOCK_FREQ_xosc       DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, xosc), clock_frequency)
#define CLOCK_FREQ_rosc       DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, rosc), clock_frequency)
#define CLOCK_FREQ_rosc_ph    DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, rosc), clock_frequency)
#define CLOCK_FREQ_gpin0      DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, gpin0), clock_frequency)
#define CLOCK_FREQ_gpin1      DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, gpin1), clock_frequency)
#define CLOCK_FREQ_pll_sys    PLL_FREQ(pll_sys)
#define CLOCK_FREQ_pll_usb    PLL_FREQ(pll_usb)

#define CLOCK_AUX_SOURCE(clk) _CONCAT(_CONCAT(AUXSTEM_, clk), _CONCAT(AUXSRC_, SRC_CLOCK(clk)))

#define AUXSRC_xosc      XOSC_CLKSRC
#define AUXSRC_rosc      ROSC_CLKSRC
#define AUXSRC_rosc_ph   ROSC_CLKSRC_PH
#define AUXSRC_pll_sys   CLKSRC_PLL_SYS
#define AUXSRC_pll_usb   CLKSRC_PLL_USB
#define AUXSRC_clk_ref   CLK_REF
#define AUXSRC_clk_sys   CLK_SYS
#define AUXSRC_clk_usb   CLK_USB
#define AUXSRC_clk_adc   CLK_ADC
#define AUXSRC_clk_gpin0 CLKSRC_GPIN0
#define AUXSRC_clk_gpin1 CLKSRC_GPIN1
#if defined(CONFIG_SOC_SERIES_RP2040)
#define AUXSRC_clk_rtc CLK_RTC
#else
#define AUXSRC_pll_usb_primary_ref_opcg CLKSRC_PLL_PLL_USB_PRIMARY_REF_OPCG
#define AUXSRC_lposc LPOSC_CLKSRC
#define AUXSRC_clk_hstx CLK_HSTX
#define AUXSRC_otp_clk2fc OTP_CLK2FC
#endif

#define AUXSTEM_clk_gpout0 CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_
#define AUXSTEM_clk_gpout1 CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_
#define AUXSTEM_clk_gpout2 CLOCKS_CLK_GPOUT2_CTRL_AUXSRC_VALUE_
#define AUXSTEM_clk_gpout3 CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_
#define AUXSTEM_clk_ref    CLOCKS_CLK_REF_CTRL_AUXSRC_VALUE_
#define AUXSTEM_clk_sys    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_
#define AUXSTEM_clk_usb    CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_
#define AUXSTEM_clk_adc    CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_
#define AUXSTEM_clk_peri CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_
#if defined(CONFIG_SOC_SERIES_RP2040)
#define AUXSTEM_clk_rtc CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_
#else
#define AUXSTEM_clk_hstx CLOCKS_CLK_HSTX_CTRL_AUXSRC_VALUE_
#endif

#define TUPLE_ENTRY(n, p, i)                                                                       \
	{                                                                                          \
		_CONCAT(RPI_PICO_CLKID_, DT_INST_STRING_UPPER_TOKEN_BY_IDX(0, clock_names, i)),    \
			COND_CODE_1(                                                               \
				DT_PROP_HAS_IDX(DT_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(clocks), i),    \
						clocks, 0),                                        \
				(_CONCAT(RPI_PICO_CLKID_,                                          \
					 DT_STRING_UPPER_TOKEN_BY_IDX(                             \
						 DT_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(clocks), i),   \
						 clock_names, 0))),                                \
				(-1))                                                              \
	}

enum rpi_pico_clkid {
	rpi_pico_clkid_none = -1,
	rpi_pico_clkid_clk_gpout0 = RPI_PICO_CLKID_CLK_GPOUT0,
	rpi_pico_clkid_clk_gpout1 = RPI_PICO_CLKID_CLK_GPOUT1,
	rpi_pico_clkid_clk_gpout2 = RPI_PICO_CLKID_CLK_GPOUT2,
	rpi_pico_clkid_clk_gpout3 = RPI_PICO_CLKID_CLK_GPOUT3,
	rpi_pico_clkid_clk_ref = RPI_PICO_CLKID_CLK_REF,
	rpi_pico_clkid_clk_sys = RPI_PICO_CLKID_CLK_SYS,
	rpi_pico_clkid_clk_peri = RPI_PICO_CLKID_CLK_PERI,
	rpi_pico_clkid_clk_usb = RPI_PICO_CLKID_CLK_USB,
	rpi_pico_clkid_clk_adc = RPI_PICO_CLKID_CLK_ADC,
#if defined(RPI_PICO_CLKID_CLK_RTC)
	rpi_pico_clkid_clk_rtc = RPI_PICO_CLKID_CLK_RTC,
#endif
	rpi_pico_clkid_pll_sys = RPI_PICO_CLKID_PLL_SYS,
	rpi_pico_clkid_pll_usb = RPI_PICO_CLKID_PLL_USB,
	rpi_pico_clkid_xosc = RPI_PICO_CLKID_XOSC,
	rpi_pico_clkid_rosc = RPI_PICO_CLKID_ROSC,
	rpi_pico_clkid_rosc_ph = RPI_PICO_CLKID_ROSC_PH,
	rpi_pico_clkid_gpin0 = RPI_PICO_CLKID_GPIN0,
	rpi_pico_clkid_gpin1 = RPI_PICO_CLKID_GPIN1,
	END_OF_RPI_PICO_CLKID,
};

struct rpi_pico_clkid_tuple {
	enum rpi_pico_clkid clk;
	enum rpi_pico_clkid parent;
};

struct rpi_pico_clk_config {
	uint32_t source;
	uint32_t aux_source;
	uint32_t rate;
	uint32_t source_rate;
};

struct rpi_pico_pll_config {
	uint32_t ref_div;
	uint32_t fb_div;
	uint32_t post_div1;
	uint32_t post_div2;
};

struct rpi_pico_rosc_config {
	uint32_t phase;
	uint32_t range;
	uint32_t div;
	uint32_t code;
};

struct rpi_pico_gpin_config {
	uint32_t frequency;
};

struct clock_control_rpi_pico_config {
	clocks_hw_t *const clocks_regs;
	xosc_hw_t *const xosc_regs;
	pll_hw_t *const pll_sys_regs;
	pll_hw_t *const pll_usb_regs;
	rosc_hw_t *const rosc_regs;
	const struct pinctrl_dev_config *pcfg;
	struct rpi_pico_pll_config plls_data[RPI_PICO_PLL_COUNT];
	struct rpi_pico_clk_config clocks_data[RPI_PICO_CLOCK_COUNT];
	struct rpi_pico_rosc_config rosc_data;
	struct rpi_pico_gpin_config gpin_data[RPI_PICO_GPIN_COUNT];
};

struct clock_control_rpi_pico_data {
	uint32_t rosc_freq;
	uint32_t rosc_ph_freq;
};

uint64_t rpi_pico_frequency_count(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_rpi_pico_config *config = dev->config;
	enum rpi_pico_clkid clkid = (enum rpi_pico_clkid)sys;
	fc_hw_t *fc0 = &config->clocks_regs->fc0;
	uint32_t fc0_id;

	switch (clkid) {
	case rpi_pico_clkid_clk_ref:
		fc0_id = CLOCKS_FC0_SRC_VALUE_CLK_REF;
		break;
	case rpi_pico_clkid_clk_sys:
		fc0_id = CLOCKS_FC0_SRC_VALUE_CLK_SYS;
		break;
	case rpi_pico_clkid_clk_peri:
		fc0_id = CLOCKS_FC0_SRC_VALUE_CLK_PERI;
		break;
	case rpi_pico_clkid_clk_usb:
		fc0_id = CLOCKS_FC0_SRC_VALUE_CLK_USB;
		break;
	case rpi_pico_clkid_clk_adc:
		fc0_id = CLOCKS_FC0_SRC_VALUE_CLK_ADC;
		break;
#if defined(CONFIG_SOC_SERIES_RP2040)
	case rpi_pico_clkid_clk_rtc:
		fc0_id = CLOCKS_FC0_SRC_VALUE_CLK_RTC;
		break;
#endif
	case rpi_pico_clkid_pll_sys:
		fc0_id = CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY;
		break;
	case rpi_pico_clkid_pll_usb:
		fc0_id = CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY;
		break;
	case rpi_pico_clkid_xosc:
		fc0_id = CLOCKS_FC0_SRC_VALUE_XOSC_CLKSRC;
		break;
	case rpi_pico_clkid_rosc:
		fc0_id = CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC;
		break;
	case rpi_pico_clkid_rosc_ph:
		fc0_id = CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC_PH;
		break;
	case rpi_pico_clkid_gpin0:
		fc0_id = CLOCKS_FC0_SRC_VALUE_CLKSRC_GPIN0;
		break;
	case rpi_pico_clkid_gpin1:
		fc0_id = CLOCKS_FC0_SRC_VALUE_CLKSRC_GPIN0;
		break;
	default:
		return -1;
	}

	(void)frequency_count_khz(fc0_id);

	return ((fc0->result >> CLOCKS_FC0_RESULT_KHZ_LSB) * 1000) +
	       ((fc0->result & CLOCKS_FC0_RESULT_FRAC_BITS) * 1000 / CLOCKS_FC0_RESULT_FRAC_BITS);
}

static int rpi_pico_rosc_write(const struct device *dev, io_rw_32 *addr, uint32_t value)
{
	hw_clear_bits(&rosc_hw->status, ROSC_STATUS_BADWRITE_BITS);

	if (rosc_hw->status & ROSC_STATUS_BADWRITE_BITS) {
		return -EINVAL;
	}

	*addr = value;

	if (rosc_hw->status & ROSC_STATUS_BADWRITE_BITS) {
		return -EINVAL;
	}

	return 0;
}

/**
 * Get source clock id of this clock
 *
 * @param dev pointer to clock device
 * @param id id of this clock
 * @return parent clock id
 */
static enum rpi_pico_clkid rpi_pico_get_clock_src(const struct device *dev, enum rpi_pico_clkid id)
{
	const struct clock_control_rpi_pico_config *config = dev->config;
	enum rpi_pico_clkid srcid;

	switch (id) {
	case rpi_pico_clkid_clk_gpout0:
	case rpi_pico_clkid_clk_gpout1:
	case rpi_pico_clkid_clk_gpout2:
	case rpi_pico_clkid_clk_gpout3: {
		const static enum rpi_pico_clkid table[] = {
			rpi_pico_clkid_pll_sys,
			rpi_pico_clkid_gpin0,
			rpi_pico_clkid_gpin1,
			rpi_pico_clkid_pll_usb,
			rpi_pico_clkid_rosc_ph,
			rpi_pico_clkid_xosc,
			rpi_pico_clkid_clk_sys,
			rpi_pico_clkid_clk_usb,
			rpi_pico_clkid_clk_adc,
#if defined(CONFIG_SOC_SERIES_RP2040)
			rpi_pico_clkid_clk_rtc,
#endif
			rpi_pico_clkid_clk_ref,
		};

		clock_hw_t *clock_hw = &config->clocks_regs->clk[id];
		uint32_t aux = ((clock_hw->ctrl & CTRL_AUXSRC_BITS) >> CTRL_AUXSRC_LSB);

		srcid = table[aux];
		break;
	}
	case rpi_pico_clkid_clk_ref: {
		const static enum rpi_pico_clkid table[] = {
			rpi_pico_clkid_pll_usb,
			rpi_pico_clkid_gpin0,
			rpi_pico_clkid_gpin1,
		};

		clock_hw_t *clock_hw = &clocks_hw->clk[id];
		uint32_t aux = ((clock_hw->ctrl & CTRL_AUXSRC_BITS) >> CTRL_AUXSRC_LSB);
		uint32_t src = ((clock_hw->ctrl >> CTRL_SRC_LSB) & CTRL_SRC_BITS);

		if (src == CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH) {
			srcid = rpi_pico_clkid_rosc_ph;
		} else if (src == CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC) {
			srcid = rpi_pico_clkid_xosc;
		} else {
			srcid = table[aux];
		}
		break;
	}
	case rpi_pico_clkid_clk_sys: {
		const static enum rpi_pico_clkid table[] = {
			rpi_pico_clkid_pll_sys,
			rpi_pico_clkid_pll_usb,
			rpi_pico_clkid_rosc,
			rpi_pico_clkid_xosc,
			rpi_pico_clkid_gpin0,
			rpi_pico_clkid_gpin1,
		};

		clock_hw_t *clock_hw = &clocks_hw->clk[id];
		uint32_t aux = ((clock_hw->ctrl & CTRL_AUXSRC_BITS) >> CTRL_AUXSRC_LSB);
		uint32_t src = ((clock_hw->ctrl >> CTRL_SRC_LSB) & CTRL_SRC_BITS);

		if (src == CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF) {
			srcid = rpi_pico_clkid_clk_ref;
		} else {
			srcid = table[aux];
		}
		break;
	}
	case rpi_pico_clkid_clk_peri: {
		const static enum rpi_pico_clkid table[] = {
			rpi_pico_clkid_clk_sys,
			rpi_pico_clkid_pll_sys,
			rpi_pico_clkid_pll_usb,
			rpi_pico_clkid_rosc_ph,
			rpi_pico_clkid_xosc,
			rpi_pico_clkid_gpin0,
			rpi_pico_clkid_gpin1,
		};

		clock_hw_t *clock_hw = &clocks_hw->clk[id];
		uint32_t aux = ((clock_hw->ctrl & CTRL_AUXSRC_BITS) >> CTRL_AUXSRC_LSB);

		srcid = table[aux];
		break;
	}
	case rpi_pico_clkid_clk_usb:
	case rpi_pico_clkid_clk_adc:
#if defined(RPI_PICO_CLKID_CLK_RTC)
	case rpi_pico_clkid_clk_rtc:
#else

#endif
	{
		const static enum rpi_pico_clkid table[] = {
			rpi_pico_clkid_pll_usb,
			rpi_pico_clkid_pll_sys,
			rpi_pico_clkid_rosc_ph,
			rpi_pico_clkid_xosc,
			rpi_pico_clkid_gpin0,
			rpi_pico_clkid_gpin1,
		};

		clock_hw_t *clock_hw = &clocks_hw->clk[id];
		uint32_t aux = ((clock_hw->ctrl & CTRL_AUXSRC_BITS) >> CTRL_AUXSRC_LSB);

		srcid = table[aux];
		break;
	}
	case rpi_pico_clkid_pll_sys:
	case rpi_pico_clkid_pll_usb: {
		srcid = rpi_pico_clkid_xosc;
		break;
	}
	default:
		srcid = rpi_pico_clkid_none;
		break;
	}

	return srcid;
}

/**
 * Query clock is enabled or not
 *
 * @param dev pointer to clock device
 * @param id id of clock
 * @return true if the clock enabled, otherwith false
 */
static bool rpi_pico_is_clock_enabled(const struct device *dev, enum rpi_pico_clkid id)
{
	const struct clock_control_rpi_pico_config *config = dev->config;

	if (id == rpi_pico_clkid_clk_sys || id == rpi_pico_clkid_clk_ref) {
		return true;
	} else if (id == rpi_pico_clkid_clk_usb ||
		   id == rpi_pico_clkid_clk_peri ||
		   id == rpi_pico_clkid_clk_adc ||
#if defined(RPI_PICO_CLKID_CLK_RTC)
		   id == rpi_pico_clkid_clk_rtc ||
#endif
		   id == rpi_pico_clkid_clk_gpout0 ||
		   id == rpi_pico_clkid_clk_gpout1 ||
		   id == rpi_pico_clkid_clk_gpout2 ||
		   id == rpi_pico_clkid_clk_gpout3) {
		clock_hw_t *clock_hw = &config->clocks_regs->clk[id];

		if (clock_hw->ctrl & CTRL_ENABLE_BITS) {
			return true;
		}
	} else if (id == rpi_pico_clkid_pll_sys || id == rpi_pico_clkid_pll_usb) {
		pll_hw_t *pll = (id == rpi_pico_clkid_pll_sys) ? config->pll_sys_regs
							       : config->pll_usb_regs;

		if (!(pll->pwr & (PLL_PWR_VCOPD_BITS | PLL_PWR_POSTDIVPD_BITS | PLL_PWR_PD_BITS))) {
			return true;
		}
	} else if (id == rpi_pico_clkid_xosc) {
		if (config->xosc_regs->status & XOSC_STATUS_ENABLED_BITS) {
			return true;
		}
	} else if (id == rpi_pico_clkid_rosc || id == rpi_pico_clkid_rosc_ph) {
		return true;
	}

	return false;
}

/**
 * Calculate clock frequency with traversing clock tree.
 *
 * @param dev pointer to clock device
 * @param id id of clock
 * @return frequency value or 0 if disabled
 */
static float rpi_pico_calc_clock_freq(const struct device *dev, enum rpi_pico_clkid id)
{
	const struct clock_control_rpi_pico_config *config = dev->config;
	struct clock_control_rpi_pico_data *data = dev->data;
	float freq = 0.f;

	if (!rpi_pico_is_clock_enabled(dev, id)) {
		return freq;
	}

	if (id == rpi_pico_clkid_clk_sys ||
	    id == rpi_pico_clkid_clk_usb ||
	    id == rpi_pico_clkid_clk_adc ||
#if defined(RPI_PICO_CLKID_CLK_RTC)
	    id == rpi_pico_clkid_clk_rtc ||
#endif
	    id == rpi_pico_clkid_clk_ref ||
	    id == rpi_pico_clkid_clk_gpout0 ||
	    id == rpi_pico_clkid_clk_gpout1 ||
	    id == rpi_pico_clkid_clk_gpout2 ||
	    id == rpi_pico_clkid_clk_gpout3) {
		clock_hw_t *clock_hw = &config->clocks_regs->clk[id];

		freq = rpi_pico_calc_clock_freq(dev, rpi_pico_get_clock_src(dev, id)) /
		       (((clock_hw->div & DIV_INT_BITS) >> DIV_INT_LSB) +
			((clock_hw->div & DIV_FRAC_BITS) / (float)DIV_FRAC_BITS));
	} else if (id == rpi_pico_clkid_clk_peri) {
		freq = rpi_pico_calc_clock_freq(dev, rpi_pico_get_clock_src(dev, id));
	} else if (id == rpi_pico_clkid_pll_sys || id == rpi_pico_clkid_pll_usb) {
		pll_hw_t *pll = (id == rpi_pico_clkid_pll_sys) ? config->pll_sys_regs
							       : config->pll_usb_regs;
		freq = rpi_pico_calc_clock_freq(dev, rpi_pico_get_clock_src(dev, id)) *
		       (pll->fbdiv_int) / (pll->cs & PLL_CS_REFDIV_BITS) /
		       ((pll->prim & PLL_PRIM_POSTDIV1_BITS) >> PLL_PRIM_POSTDIV1_LSB) /
		       ((pll->prim & PLL_PRIM_POSTDIV2_BITS) >> PLL_PRIM_POSTDIV2_LSB);
	} else if (id == rpi_pico_clkid_xosc) {
		freq = CLOCK_FREQ_xosc;
	} else if (id == rpi_pico_clkid_rosc) {
		freq = data->rosc_freq;
	} else if (id == rpi_pico_clkid_rosc_ph) {
		freq = data->rosc_ph_freq;
	}

	return freq;
}

static int rpi_pico_is_valid_clock_index(enum rpi_pico_clkid index)
{
	if (index >= END_OF_RPI_PICO_CLKID) {
		return -EINVAL;
	}

	return 0;
}

static int clock_control_rpi_pico_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_rpi_pico_config *config = dev->config;
	enum rpi_pico_clkid clkid = (enum rpi_pico_clkid)sys;

	if (rpi_pico_is_valid_clock_index(clkid) < 0) {
		return -EINVAL;
	}

	switch (clkid) {
	case rpi_pico_clkid_pll_sys:
		hw_clear_bits(&config->pll_sys_regs->pwr, PLL_PWR_BITS);
		break;
	case rpi_pico_clkid_pll_usb:
		hw_clear_bits(&config->pll_usb_regs->pwr, PLL_PWR_BITS);
		break;
	default:
		hw_set_bits(&config->clocks_regs->clk[clkid].ctrl, CTRL_ENABLE_BITS);
	}

	return 0;
}

static int clock_control_rpi_pico_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_rpi_pico_config *config = dev->config;
	enum rpi_pico_clkid clkid = (enum rpi_pico_clkid)sys;

	if (rpi_pico_is_valid_clock_index(clkid) < 0) {
		return -EINVAL;
	}

	switch (clkid) {
	case rpi_pico_clkid_pll_sys:
		hw_set_bits(&config->pll_sys_regs->pwr, PLL_PWR_BITS);
		break;
	case rpi_pico_clkid_pll_usb:
		hw_set_bits(&config->pll_usb_regs->pwr, PLL_PWR_BITS);
		break;
	default:
		hw_clear_bits(&config->clocks_regs->clk[clkid].ctrl, CTRL_ENABLE_BITS);
	}

	return 0;
}

static enum clock_control_status clock_control_rpi_pico_get_status(const struct device *dev,
								   clock_control_subsys_t sys)
{
	enum rpi_pico_clkid clkid = (enum rpi_pico_clkid)sys;

	if (rpi_pico_is_valid_clock_index(clkid) < 0) {
		return -EINVAL;
	}

	if (rpi_pico_is_clock_enabled(dev, clkid)) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static int clock_control_rpi_pico_get_rate(const struct device *dev, clock_control_subsys_t sys,
					   uint32_t *rate)
{
	struct clock_control_rpi_pico_data *data = dev->data;
	enum rpi_pico_clkid clkid = (enum rpi_pico_clkid)sys;

	if (rpi_pico_is_valid_clock_index(clkid) < 0) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_RPI_PICO_ROSC_USE_MEASURED_FREQ)) {
		if (clkid == rpi_pico_clkid_rosc) {
			data->rosc_freq = rpi_pico_frequency_count(dev, sys);
		} else if (clkid == rpi_pico_clkid_rosc_ph) {
			data->rosc_ph_freq = rpi_pico_frequency_count(dev, sys);
		}
	}

	*rate = (int)rpi_pico_calc_clock_freq(dev, clkid);

	return 0;
}

void rpi_pico_clkid_tuple_swap(struct rpi_pico_clkid_tuple *lhs, struct rpi_pico_clkid_tuple *rhs)
{
	struct rpi_pico_clkid_tuple tmp = *lhs;
	*lhs = *rhs;
	*rhs = tmp;
}

void rpi_pico_clkid_tuple_reorder_by_dependencies(struct rpi_pico_clkid_tuple *tuples, size_t len)
{
	uint32_t sorted_idx = 0;
	uint32_t checked_idx = 0;
	uint32_t target = -1;

	while (sorted_idx < len) {
		for (uint32_t i = sorted_idx; i < len; i++) {
			if (tuples[i].parent == target) {
				rpi_pico_clkid_tuple_swap(&tuples[sorted_idx], &tuples[i]);
				sorted_idx++;
			}
		}
		target = tuples[checked_idx++].clk;
	}
}

static int clock_control_rpi_pico_init(const struct device *dev)
{
	const uint32_t cycles_per_tick = CLOCK_FREQ_xosc / 1000000;
	const struct clock_control_rpi_pico_config *config = dev->config;
	struct clock_control_rpi_pico_data *data = dev->data;
	clocks_hw_t *clocks_regs = config->clocks_regs;
	rosc_hw_t *rosc_regs = config->rosc_regs;
	pll_hw_t *plls[] = {config->pll_sys_regs, config->pll_usb_regs};
	struct rpi_pico_clkid_tuple tuples[] = {
		DT_INST_FOREACH_PROP_ELEM_SEP(0, clock_names, TUPLE_ENTRY, (,))};
	uint32_t rosc_div;
	int ret;

	/* Reset all function before clock configuring */
	reset_block(~(RESETS_RESET_IO_QSPI_BITS | RESETS_RESET_PADS_QSPI_BITS |
		      RESETS_RESET_PLL_USB_BITS | RESETS_RESET_USBCTRL_BITS |
		      RESETS_RESET_SYSCFG_BITS | RESETS_RESET_PLL_SYS_BITS));

	unreset_block_wait(RESETS_RESET_BITS &
			   ~(RESETS_RESET_ADC_BITS |
#if defined(RESETS_RESET_RTC_BITS)
			     RESETS_RESET_RTC_BITS |
#endif
#if defined(RESETS_RESET_HSTX_BITS)
			     RESETS_RESET_HSTX_BITS |
#endif
			     RESETS_RESET_SPI0_BITS | RESETS_RESET_SPI1_BITS |
			     RESETS_RESET_UART0_BITS | RESETS_RESET_UART1_BITS |
			     RESETS_RESET_USBCTRL_BITS));

	/* Start tick in watchdog */
	tick_start(TICK_WATCHDOG, cycles_per_tick);
#if defined(CONFIG_SOC_SERIES_RP2350)
	tick_start(TICK_TIMER0, cycles_per_tick);
	tick_start(TICK_TIMER1, cycles_per_tick);
#endif

	clocks_regs->resus.ctrl = 0;

	/* Configure xosc */
	xosc_init();

	/* Before we touch PLLs, switch sys and ref cleanly away from their aux sources. */
	clocks_hw->clk[RPI_PICO_CLKID_CLK_SYS].ctrl &= ~CTRL_SRC_BITS;
	while (clocks_hw->clk[RPI_PICO_CLKID_CLK_SYS].selected != 0x1) {
		;
	}
	clocks_hw->clk[RPI_PICO_CLKID_CLK_REF].ctrl &= ~CTRL_SRC_BITS;
	while (clocks_hw->clk[RPI_PICO_CLKID_CLK_REF].selected != 0x1) {
		;
	}

	/* Configure pll */
	for (uint32_t i = 0; i < RPI_PICO_PLL_COUNT; i++) {
		pll_init(plls[i], config->plls_data[i].ref_div,
			 CLOCK_FREQ_xosc * config->plls_data[i].fb_div,
			 config->plls_data[i].post_div1, config->plls_data[i].post_div2);
	}

	/* Configure clocks */
	rpi_pico_clkid_tuple_reorder_by_dependencies(tuples, ARRAY_SIZE(tuples));
	for (uint32_t i = 0; i < ARRAY_SIZE(tuples); i++) {
		if (tuples[i].clk < 0 || tuples[i].clk >= RPI_PICO_CLOCK_COUNT) {
			continue;
		}

		if (!(clock_configure(tuples[i].clk, config->clocks_data[tuples[i].clk].source,
				      config->clocks_data[tuples[i].clk].aux_source,
				      config->clocks_data[tuples[i].clk].source_rate,
				      config->clocks_data[tuples[i].clk].rate))) {
			return -EINVAL;
		}
	}

	hw_clear_bits(&clocks_regs->clk[rpi_pico_clkid_clk_gpout0].ctrl, CTRL_ENABLE_BITS);
	hw_clear_bits(&clocks_regs->clk[rpi_pico_clkid_clk_gpout1].ctrl, CTRL_ENABLE_BITS);
	hw_clear_bits(&clocks_regs->clk[rpi_pico_clkid_clk_gpout2].ctrl, CTRL_ENABLE_BITS);
	hw_clear_bits(&clocks_regs->clk[rpi_pico_clkid_clk_gpout3].ctrl, CTRL_ENABLE_BITS);

	/* Configure rosc */
	ret = rpi_pico_rosc_write(dev, &rosc_regs->phase,
				  (ROSC_PHASE_PASSWD_VALUE_PASS << ROSC_PHASE_PASSWD_LSB) |
					  config->rosc_data.phase);
	if (ret < 0) {
		return ret;
	}

	ret = rpi_pico_rosc_write(dev, &rosc_regs->ctrl,
				  (ROSC_CTRL_ENABLE_VALUE_ENABLE << ROSC_CTRL_ENABLE_LSB) |
					  config->rosc_data.range);
	if (ret < 0) {
		return ret;
	}

	if (config->rosc_data.div <= 0) {
		rosc_div = ROSC_DIV_VALUE_PASS + 1;
	} else if (config->rosc_data.div > 31) {
		rosc_div = ROSC_DIV_VALUE_PASS;
	} else {
		rosc_div = ROSC_DIV_VALUE_PASS + config->rosc_data.div;
	}

	ret = rpi_pico_rosc_write(dev, &rosc_regs->div, rosc_div);
	if (ret < 0) {
		return ret;
	}

	ret = rpi_pico_rosc_write(dev, &rosc_regs->freqa,
				  (ROSC_FREQA_PASSWD_VALUE_PASS << ROSC_FREQA_PASSWD_LSB) |
					  (config->rosc_data.code & UINT16_MAX));
	if (ret < 0) {
		return ret;
	}

	ret = rpi_pico_rosc_write(dev, &rosc_regs->freqb,
				  (ROSC_FREQA_PASSWD_VALUE_PASS << ROSC_FREQA_PASSWD_LSB) |
					  (config->rosc_data.code >> 16));
	if (ret < 0) {
		return ret;
	}

	unreset_block_wait(RESETS_RESET_BITS);

	if (IS_ENABLED(CONFIG_RPI_PICO_ROSC_USE_MEASURED_FREQ)) {
		data->rosc_freq =
			rpi_pico_frequency_count(dev, (clock_control_subsys_t)rpi_pico_clkid_rosc);
		data->rosc_ph_freq = rpi_pico_frequency_count(
			dev, (clock_control_subsys_t)rpi_pico_clkid_rosc_ph);
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	return 0;
}

static DEVICE_API(clock_control, clock_control_rpi_pico_api) = {
	.on = clock_control_rpi_pico_on,
	.off = clock_control_rpi_pico_off,
	.get_rate = clock_control_rpi_pico_get_rate,
	.get_status = clock_control_rpi_pico_get_status,
};

BUILD_ASSERT((VCO_FREQ(pll_sys) >= PLL_VCO_FREQ_MIN) && (VCO_FREQ(pll_sys) <= PLL_VCO_FREQ_MAX) &&
		     (VCO_FREQ(pll_sys) >= (CLOCK_FREQ_xosc / REF_DIV(pll_sys) * 16)),
	     "pll_sys: vco_freq is out of range");
BUILD_ASSERT((FB_DIV(pll_sys) >= PLL_FB_DIV_MIN) && (FB_DIV(pll_sys) <= PLL_FB_DIV_MAX),
	     "pll_sys: fb-div is out of range");
BUILD_ASSERT((POST_DIV1(pll_sys) >= PLL_POST_DIV_MIN) && (POST_DIV1(pll_sys) <= PLL_POST_DIV_MAX),
	     "pll_sys: post-div1 is out of range");
BUILD_ASSERT((POST_DIV2(pll_sys) >= PLL_POST_DIV_MIN) && (POST_DIV2(pll_sys) <= PLL_POST_DIV_MAX),
	     "pll_sys: post-div2 is out of range");

BUILD_ASSERT((VCO_FREQ(pll_usb) >= PLL_VCO_FREQ_MIN) && (VCO_FREQ(pll_usb) <= PLL_VCO_FREQ_MAX) &&
		     (VCO_FREQ(pll_usb) >= (CLOCK_FREQ_xosc / REF_DIV(pll_usb) * 16)),
	     "pll_usb: vco_freq is out of range");
BUILD_ASSERT((FB_DIV(pll_usb) >= PLL_FB_DIV_MIN) && (FB_DIV(pll_usb) <= PLL_FB_DIV_MAX),
	     "pll_usb: fb-div is out of range");
BUILD_ASSERT((POST_DIV1(pll_usb) >= PLL_POST_DIV_MIN) && (POST_DIV1(pll_usb) <= PLL_POST_DIV_MAX),
	     "pll_usb: post-div is out of range");
BUILD_ASSERT((POST_DIV2(pll_usb) >= PLL_POST_DIV_MIN) && (POST_DIV2(pll_usb) <= PLL_POST_DIV_MAX),
	     "pll_usb: post-div is out of range");

BUILD_ASSERT(SRC_CLOCK_FREQ(clk_ref) >= CLOCK_FREQ_clk_ref,
	     "clk_ref: clock divider is out of range");
BUILD_ASSERT(SRC_CLOCK_FREQ(clk_sys) >= CLOCK_FREQ_clk_sys,
	     "clk_sys: clock divider is out of range");
BUILD_ASSERT(SRC_CLOCK_FREQ(clk_usb) >= CLOCK_FREQ_clk_usb,
	     "clk_usb: clock divider is out of range");
BUILD_ASSERT(SRC_CLOCK_FREQ(clk_adc) >= CLOCK_FREQ_clk_adc,
	     "clk_adc: clock divider is out of range");
#if defined(CONFIG_SOC_SERIES_RP2040)
BUILD_ASSERT(SRC_CLOCK_FREQ(clk_rtc) >= CLOCK_FREQ_clk_rtc,
	     "clk_rtc: clock divider is out of range");
#endif
BUILD_ASSERT(SRC_CLOCK_FREQ(clk_peri) >= CLOCK_FREQ_clk_peri,
	     "clk_peri: clock divider is out of range");

BUILD_ASSERT(CLOCK_FREQ(rosc_ph) == CLOCK_FREQ(rosc), "rosc_ph: frequency must be equal to rosc");

PINCTRL_DT_INST_DEFINE(0);

static const struct clock_control_rpi_pico_config clock_control_rpi_pico_config = {
	.clocks_regs = (clocks_hw_t *)DT_INST_REG_ADDR_BY_NAME(0, clocks),
	.xosc_regs = (xosc_hw_t *)DT_INST_REG_ADDR_BY_NAME(0, xosc),
	.pll_sys_regs = (pll_hw_t *)DT_INST_REG_ADDR_BY_NAME(0, pll_sys),
	.pll_usb_regs = (pll_hw_t *)DT_INST_REG_ADDR_BY_NAME(0, pll_usb),
	.rosc_regs = (rosc_hw_t *)DT_INST_REG_ADDR_BY_NAME(0, rosc),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.clocks_data = {
		[RPI_PICO_CLKID_CLK_GPOUT0] = {
			.source = 0,
			.aux_source = CLOCK_AUX_SOURCE(clk_gpout0),
			.source_rate = SRC_CLOCK_FREQ(clk_gpout0),
			.rate = CLOCK_FREQ(clk_gpout0),
		},
		[RPI_PICO_CLKID_CLK_GPOUT1] = {
			.source = 0,
			.aux_source = CLOCK_AUX_SOURCE(clk_gpout1),
			.source_rate = SRC_CLOCK_FREQ(clk_gpout1),
			.rate = CLOCK_FREQ(clk_gpout1),
		},
		[RPI_PICO_CLKID_CLK_GPOUT2] = {
			.source = 0,
			.aux_source = CLOCK_AUX_SOURCE(clk_gpout2),
			.source_rate = SRC_CLOCK_FREQ(clk_gpout2),
			.rate = CLOCK_FREQ(clk_gpout2),
		},
		[RPI_PICO_CLKID_CLK_GPOUT3] = {
			.source = 0,
			.aux_source = CLOCK_AUX_SOURCE(clk_gpout3),
			.source_rate = SRC_CLOCK_FREQ(clk_gpout3),
			.rate = CLOCK_FREQ(clk_gpout3),
		},
		[RPI_PICO_CLKID_CLK_REF] = {
#if CLK_SRC_IS(clk_ref, rosc_ph)
			.source = CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH,
			.aux_source = 0,
#elif CLK_SRC_IS(clk_ref, xosc)
			.source = CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC,
			.aux_source = 0,
#else
			.source = CLOCKS_CLK_REF_CTRL_SRC_VALUE_CLKSRC_CLK_REF_AUX,
#endif
			.source_rate = SRC_CLOCK_FREQ(clk_ref),
			.rate = CLOCK_FREQ(clk_ref),
		},
		[RPI_PICO_CLKID_CLK_SYS] = {
#if CLK_SRC_IS(clk_sys, clk_ref)
			.source = CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF,
			.aux_source = 0,
#else
			.source = CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
			.aux_source = CLOCK_AUX_SOURCE(clk_sys),
#endif
			.source_rate = SRC_CLOCK_FREQ(clk_sys),
			.rate = CLOCK_FREQ(clk_sys),
		},
		[RPI_PICO_CLKID_CLK_PERI] = {
			.source = 0,
			.aux_source = CLOCK_AUX_SOURCE(clk_peri),
			.source_rate = SRC_CLOCK_FREQ(clk_peri),
			.rate = CLOCK_FREQ(clk_peri),
		},
		[RPI_PICO_CLKID_CLK_USB] = {
			.source = 0,
			.aux_source = CLOCK_AUX_SOURCE(clk_usb),
			.source_rate = SRC_CLOCK_FREQ(clk_usb),
			.rate = CLOCK_FREQ(clk_usb),
		},
		[RPI_PICO_CLKID_CLK_ADC] = {
			.source = 0,
			.aux_source = CLOCK_AUX_SOURCE(clk_adc),
			.source_rate = SRC_CLOCK_FREQ(clk_adc),
			.rate = CLOCK_FREQ(clk_adc),
		},
#if defined(RPI_PICO_CLKID_CLK_RTC)
		[RPI_PICO_CLKID_CLK_RTC] = {
			.source = 0,
			.aux_source = CLOCK_AUX_SOURCE(clk_rtc),
			.source_rate = SRC_CLOCK_FREQ(clk_rtc),
			.rate = CLOCK_FREQ(clk_rtc),
		},
#elif defined(RPI_PICO_CLKID_CLK_HSTX)
		[RPI_PICO_CLKID_CLK_HSTX] = {
			.source = 0,
			.aux_source = CLOCK_AUX_SOURCE(clk_hstx),
			.source_rate = SRC_CLOCK_FREQ(clk_hstx),
			.rate = CLOCK_FREQ(clk_hstx),
		},
#endif
	},
	.plls_data = {
		[RPI_PICO_PLL_SYS] = {
			.ref_div = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_sys), clock_div),
			.fb_div = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_sys), fb_div),
			.post_div1 = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_sys), post_div1),
			.post_div2 = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_sys), post_div2),
		},
		[RPI_PICO_PLL_USB] = {
			.ref_div = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_usb), clock_div),
			.fb_div = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_usb), fb_div),
			.post_div1 = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_usb), post_div1),
			.post_div2 = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_usb), post_div2),
		},
	},
	.rosc_data = {
		.phase = (COND_CODE_1(DT_NODE_HAS_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, rosc),
						       phase_flip),
				      (ROSC_PHASE_FLIP_BITS), (0x0)) |
			  COND_CODE_1(DT_NODE_HAS_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, rosc),
						       phase),
				      ((DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, rosc), phase) &
					ROSC_PHASE_SHIFT_BITS) | ROSC_PHASE_ENABLE_BITS), (0x0))),
		.div = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, rosc), clock_div),
		.range = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, rosc), range),
		.code = (STAGE_DS(0) | STAGE_DS(1) | STAGE_DS(2) | STAGE_DS(3) |
			 STAGE_DS(4) | STAGE_DS(5) | STAGE_DS(6) | STAGE_DS(7)),
	},
	.gpin_data = {
		[RPI_PICO_GPIN_0] = {
			COND_CODE_1(DT_NODE_HAS_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, gpin0),
						 clock_frequency),
				    (.frequency = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, gpin0),
							  clock_frequency),),
				    (.frequency = 0,))
		},
		[RPI_PICO_GPIN_1] = {
			COND_CODE_1(DT_NODE_HAS_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, gpin1),
						     clock_frequency),
				    (.frequency = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, gpin1),
							  clock_frequency),),
				    (.frequency = 0,))
		},
	},
};

static struct clock_control_rpi_pico_data clock_control_rpi_pico_data = {
	.rosc_freq = CLOCK_FREQ(rosc),
	.rosc_ph_freq = CLOCK_FREQ(rosc_ph),
};

DEVICE_DT_INST_DEFINE(0, clock_control_rpi_pico_init, NULL, &clock_control_rpi_pico_data,
		      &clock_control_rpi_pico_config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_rpi_pico_api);
