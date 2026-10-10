/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file clock_mchp_pic32cx_bz.c
 * @brief Clock control driver for pic32cx_bz family devices.
 */

#include <stdbool.h>
#include <string.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/drivers/clock_control/mchp_clock_control.h>

/******************************************************************************
 * @brief Macro definitions
 *****************************************************************************/
#define CLOCK_NODE DT_NODELABEL(clock)

#define CLOCK_ON 1U
#define CLOCK_OFF 0U
#define CLOCK_SUCCESS 0

/* fixed clock frequency values */
#define LPRC_FREQ 32768U
#define SOSC_FREQ 32768U
#define FRC_FREQ 8000000U
#define FRC_FREQ_BY_250 32000U
#define POSC_FREQ 16000000U
#define POSC_FREQ_BY_500 32000U
#if CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_BZ6
#define SPLL3_FREQ 128000000U
#define CLOCK_MCHP_PBCLK_ID_MAX 5U
#define CLOCK_MCHP_GCLKPERIPH_ID_MAX 37U
#define CLOCK_MCHP_PBCLK_OFFSET_MAX 0x1B0U
#define CLOCK_MCHP_REFCLK_OFFSET_MAX 0x140U
#define CLOCK_MCHP_GCLKPERIPH_OFFSET_MAX 0xB0U
#else
#error Unsupported PIC32CX_BZ family
#endif

/* number of refclk input pins, to store their frequencies */
#define REFI_IO_COUNT 6

/* Macros used to write source and enable bits, in gclkperiph registers */
#define CFGPCLKGEN_CLR_MASK 0xFU
#define CFGPCLKGEN_SRC_MASK 0x7U
#define CFGPCLKGEN_EN_POS 3U

/* Trim value bit capacity, used to calculate refclk frequency */
#define TRIM_VALUE_DIVIDER 512U

/* Offset of REFOxTRIM_REG register from respective REFOxCON register */
#define REFCLK_TRIM_OFFSET 0x10U

/* To configure WDT Clock, actual value to write in register, as it does not match enum value */
#define CLOCK_MCHP_WDTCLK_RUN_MOD_LPRC_VAL 3U

/* Difference between register offsets of REFOxCON registers */
#define CLOCK_REFOCON_REG_NEXT_OFST (0x20U)
#define DIV_BY_32(x)                (x / 32)
#define DIV_BY_31_25(x)             ((x * 100) / 3125)
#define DIV_BY_1_024(x)             ((x * 1000) / 1024)

/* register offset Not Applicable for a clock subsystem ID */
#define OFFSET_NA (0xfff)
/* bit position Not Applicable for a clock subsystem ID */
#define BITPOS_NA (0xff)

/* Maximum possible values, used to validate corresponding input arguments */
#define CLOCK_MCHP_POSC_ID_MAX       (0)
#define CLOCK_MCHP_SOSC_ID_MAX       (0)
#define CLOCK_MCHP_SYSCLK_ID_MAX     (0)
#define CLOCK_MCHP_SPLL1_ID_MAX      (0)
#define CLOCK_MCHP_SPLL2_ID_MAX      (0)
#define CLOCK_MCHP_REFCLK_ID_MAX     (5)


#define CLOCK_MCHP_GCLKPERIPH_BITPOS_MAX (28)

/* Clock subsystem Types */
#define SUBSYS_TYPE_POSC       (0)
#define SUBSYS_TYPE_SOSC       (1)
#define SUBSYS_TYPE_SYSCLK     (2)
#define SUBSYS_TYPE_SPLL1      (3)
#define SUBSYS_TYPE_SPLL2      (4)
#define SUBSYS_TYPE_PBCLK      (5)
#define SUBSYS_TYPE_REFCLK     (6)
#define SUBSYS_TYPE_GCLKPERIPH (7)
#define SUBSYS_TYPE_WDTCLK     (8)
#define SUBSYS_TYPE_VBKPCLK    (9)
#define SUBSYS_TYPE_DSWDTCLK   (10)
#define SUBSYS_TYPE_LPCLK      (11)
#define SUBSYS_TYPE_RTCCLK     (12)
#define SUBSYS_TYPE_FRC        (13)
#define SUBSYS_TYPE_LPRC       (14)
#define SUBSYS_TYPE_SPLL3      (15)
#define SUBSYS_TYPE_MAX        (15)

/******************************************************************************
 * @brief Data type definitions
 *****************************************************************************/
/** @brief Clock subsystem definition.
 *
 * Structure which can be used as a sys argument in the clock_control API.
 * Encode clock type, instance number, register offset and bit position
 * to clock subsystem.
 *
 * - 00..07 (8 bits): bit position
 * - 08..19 (12 bits): register offset
 * - 20..25 (6 bits): inst
 * - 26..31 (6 bits): type
 */
typedef union {
	uint32_t val;
	struct {
		uint32_t bitpos: 8;
		uint32_t offset: 12;
		uint32_t inst: 6;
		uint32_t type: 6;
	} bits;
} clock_mchp_subsys_t;

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP
/** @brief REFCLK initialization structure. */
typedef struct {
	clock_mchp_subsys_t subsys;
	uint16_t div;
	uint8_t stop_in_idle_en;
	uint8_t run_in_sleep_en;
	uint8_t pin_out_en;
	uint8_t src_sel;
	uint16_t trim_val;
	uint32_t refi_freq;
} clock_refclk_init_t;
#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP */

/** @brief clock driver configuration structure. */
typedef struct {
	cru_registers_t *cru_regs;
	cfg_registers_t *cfg_regs;

	uint32_t frc_freq;
	uint32_t posc_freq;

	/* Timeout in milliseconds to wait for clock to turn on */
	uint32_t on_timeout_ms;
} clock_mchp_config_t;

/** @brief clock driver data structure. */
typedef struct {
	uint32_t refi_freq[REFI_IO_COUNT];
} clock_mchp_data_t;

/******************************************************************************
 * @brief Function forward declarations
 *****************************************************************************/
static enum clock_control_status clock_mchp_get_status(const struct device *dev,
						       clock_control_subsys_t sys);

/******************************************************************************
 * @brief Helper functions
 *****************************************************************************/
static inline void clock_write_reg32(__IO uint32_t *reg, uint32_t mask, uint32_t val)
{
	uint32_t regval = *reg;

	regval &= ~mask;
	regval |= val;
	*reg = regval;
}

/**
 * @brief configure SYSCLK.
 */
void clock_sysclk_config(cru_registers_t *cru_regs, uint16_t frc_div, uint8_t new_osc)
{
	/* CRU_OSCCON register bits */
	clock_write_reg32(&cru_regs->CRU_OSCCON, CRU_OSCCON_FRCDIV_Msk | CRU_OSCCON_NOSC_Msk,
			  CRU_OSCCON_FRCDIV(frc_div) | CRU_OSCCON_NOSC(new_osc));

	cru_regs->CRU_OSCCONSET = CRU_OSCCON_OSWEN_Msk;

	/* wait for indication of successful clock change before proceeding */
	while ((cru_regs->CRU_OSCCON & CRU_OSCCON_OSWEN_Msk) != 0U) {
	}
}

/**
 * @brief configure SPLL1.
 */
void clock_spll1_config(cru_registers_t *cru_regs, uint8_t post_div)
{
	/* CRU_SPLLCON register bits */
	clock_write_reg32(&cru_regs->CRU_SPLLCON, CRU_SPLLCON_SPLLPOSTDIV1_Msk,
			  CRU_SPLLCON_SPLLPOSTDIV1(post_div));
}

/**
 * @brief configure SPLL2.
 */
void clock_spll2_config(cru_registers_t *cru_regs, uint8_t src, uint8_t post_div)
{
	/* CRU_SPLLCON register bits */
	clock_write_reg32(&cru_regs->CRU_SPLLCON,
			  CRU_SPLLCON_SPLL_BYP_Msk | CRU_SPLLCON_SPLLPOSTDIV2_Msk,
			  CRU_SPLLCON_SPLL_BYP(src) | CRU_SPLLCON_SPLLPOSTDIV2(post_div));
}

/**
 * @brief configure PBCLK.
 */
void clock_pbclk_config(cru_registers_t *cru_regs, uint32_t subsys_val, uint8_t div)
{
	__IO uint32_t *reg;
	clock_mchp_subsys_t subsys;

	subsys.val = subsys_val;
	reg = (__IO uint32_t *)((uint32_t)cru_regs + subsys.bits.offset);

	/* Wait till PB clock divisor logic is not switching divisors */
	while ((*reg & CRU_PB1DIV_PBDIVRDY_Msk) == 0) {
	}
	clock_write_reg32(reg, CRU_PB1DIV_PBDIV_Msk, CRU_PB1DIV_PBDIV(div));
}

/**
 * @brief configure REFCLK generator.
 */
void clock_refclk_config(cru_registers_t *cru_regs, uint32_t subsys_val,
			 struct clock_mchp_subsys_refclk_config *refclk_config)
{
	clock_mchp_subsys_t subsys;
	__IO uint32_t *reg;
	uint32_t regval;

	subsys.val = subsys_val;
	reg = (__IO uint32_t *)((uint32_t)cru_regs + subsys.bits.offset);
	regval = *reg;

	/* REFOCON.ROSEL must not be written while the REFOCON.ACTIVE bit is ‘1’
	 * REFOCON must not be written when REFOCON[ON] != REFOCON[ACTIVE]
	 */
	if ((regval & CRU_REFO1CON_ON_Msk) == 0) {
		while ((*reg & CRU_REFO1CON_ACTIVE_Msk) != 0) {
		}
	} else {
		while ((*reg & CRU_REFO1CON_ACTIVE_Msk) == 0) {
		}
	}

	clock_write_reg32(reg,
			  CRU_REFO1CON_RODIV_Msk | CRU_REFO1CON_SIDL_Msk | CRU_REFO1CON_RSLP_Msk |
				  CRU_REFO1CON_OE_Msk | CRU_REFO1CON_ROSEL_Msk,
			  CRU_REFO1CON_RODIV(refclk_config->div) |
				  CRU_REFO1CON_SIDL(refclk_config->stop_in_idle_en) |
				  CRU_REFO1CON_RSLP(refclk_config->run_in_sleep_en) |
				  CRU_REFO1CON_OE(refclk_config->pin_out_en) |
				  CRU_REFO1CON_ROSEL(refclk_config->src_sel));

	reg = (__IO uint32_t *)((uint32_t)cru_regs + subsys.bits.offset + REFCLK_TRIM_OFFSET);
	clock_write_reg32(reg, CRU_REFO1TRIM_ROTRIM_Msk,
			  CRU_REFO1TRIM_ROTRIM(refclk_config->trim_val));
}

/**
 * @brief save refi_freq.
 */
void clock_refclk_set_refi_freq(const struct device *dev, uint32_t subsys_val, uint32_t refi_freq)
{
	clock_mchp_data_t *data = dev->data;
	clock_mchp_subsys_t subsys;

	subsys.val = subsys_val;
	data->refi_freq[subsys.bits.inst] = refi_freq;
}

/**
 * @brief configure peripheral GCLKPERIPH.
 */
void clock_gclkperiph_config(cfg_registers_t *cfg_regs, uint32_t subsys_val, uint8_t src_sel)
{
	clock_mchp_subsys_t subsys;
	__IO uint32_t *reg;

	subsys.val = subsys_val;
	reg = (__IO uint32_t *)((uint32_t)cfg_regs + subsys.bits.offset);
	clock_write_reg32(reg, CFGPCLKGEN_CLR_MASK << subsys.bits.bitpos,
			  src_sel << subsys.bits.bitpos);
}

/**
 * @brief configure WDT Clock.
 */
void clock_wdtclk_config(cfg_registers_t *cfg_regs, uint8_t src_sel)
{
	if (src_sel == CLOCK_MCHP_WDTCLK_RUN_MOD_LPRC) {
		/* Get the actual value to write in register, as it does not match enum value */
		src_sel = CLOCK_MCHP_WDTCLK_RUN_MOD_LPRC_VAL;
	}
	clock_write_reg32(&cfg_regs->CFG_CFGCON2, CFG_CFGCON2_WDTRMCS_Msk,
			  CFG_CFGCON2_WDTRMCS(src_sel));
}

/**
 * @brief VDDBUKPCORE 32 KHz Clock Source Selection.
 */
void clock_vbkpclk_config(cfg_registers_t *cfg_regs, uint8_t src_sel)
{
	clock_write_reg32(&cfg_regs->CFG_CFGCON4, CFG_CFGCON4_VBKP_32KCSEL_Msk,
			  CFG_CFGCON4_VBKP_32KCSEL(src_sel));
}

/**
 * @brief Deep Sleep Watchdog Timer Reference Clock Select.
 */
void clock_dswdtclk_config(cfg_registers_t *cfg_regs, uint8_t src_sel)
{
	clock_write_reg32(&cfg_regs->CFG_CFGCON4, CFG_CFGCON4_DSWDTOSC_Msk,
			  CFG_CFGCON4_DSWDTOSC(src_sel));
}

/**
 * @brief configure LPCLK Modifier in Counter/Delay Mode.
 */
void clock_lpclk_config(cfg_registers_t *cfg_regs, uint8_t div_modifier)
{
	clock_write_reg32(&cfg_regs->CFG_CFGCON4, CFG_CFGCON4_LPCLK_MOD_Msk,
			  CFG_CFGCON4_LPCLK_MOD(div_modifier));
}

/**
 * @brief configure RTC LPCLK configuration.
 */
void clock_rtcclk_config(cfg_registers_t *cfg_regs, uint8_t mode_sel, uint8_t div_sel,
			 uint8_t sel_1k_32k)
{
	clock_write_reg32(&cfg_regs->CFG_CFGCON4,
			  CFG_CFGCON4_RTCNTM_CSEL_Msk | CFG_CFGCON4_VBKP_DIVSEL_Msk |
				  CFG_CFGCON4_VBKP_1KCSEL_Msk,
			  CFG_CFGCON4_RTCNTM_CSEL(mode_sel) | CFG_CFGCON4_VBKP_DIVSEL(div_sel) |
				  CFG_CFGCON4_VBKP_1KCSEL(sel_1k_32k));
}

/**
 * @brief on/off POSC clock.
 */
void clock_posc_on_off(cfg_registers_t *cfg_regs, cru_registers_t *cru_regs, uint8_t enable)
{
	__IO uint32_t *reg;

	/* CFG_CFGCON2 register bits. value 0 means HS oscillator mode is selected. */
	reg = (enable == 0) ? &cfg_regs->CFG_CFGCON2SET : &cfg_regs->CFG_CFGCON2CLR;
	*reg = CFG_CFGCON2_POSCMOD_Msk;

	/* Wait till clock is ready */
	while ((enable != 0) && ((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_POSCRDY_Msk) == 0)) {
	}
}

/**
 * @brief on/off SOSC clock.
 */
void clock_sosc_on_off(cru_registers_t *cru_regs, uint8_t enable)
{
	__IO uint32_t *reg;

	/* CRU_OSCCON register bits */
	reg = (enable == 0) ? &cru_regs->CRU_OSCCONCLR : &cru_regs->CRU_OSCCONSET;
	*reg = CRU_OSCCON_SOSCEN(1);

	/* Wait till clock is ready */
	while ((enable != 0) && ((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_SOSCRDY_Msk) == 0)) {
	}
}

/**
 * @brief on/off PBCLK clock.
 */
void clock_pbclk_on_off(cru_registers_t *cru_regs, uint32_t subsys_val, uint8_t enable)
{
	clock_mchp_subsys_t subsys;
	__IO uint32_t *reg;

	subsys.val = subsys_val;
	reg = (__IO uint32_t *)((uint32_t)cru_regs + subsys.bits.offset);
	clock_write_reg32(reg, CRU_PB1DIV_PBDIVON_Msk, (enable == 0) ? 0 : CRU_PB1DIV_PBDIVON(1));
}

/**
 * @brief on/off REFCLK generator.
 */
void clock_refclk_on_off(cru_registers_t *cru_regs, uint32_t subsys_val, uint8_t enable)
{
	clock_mchp_subsys_t subsys;
	__IO uint32_t *reg;
	uint32_t regval;

	subsys.val = subsys_val;
	reg = (__IO uint32_t *)((uint32_t)cru_regs + subsys.bits.offset);
	regval = *reg;

	/* REFOCON must not be written when REFOCON[ON] != REFOCON[ACTIVE] */
	if ((regval & CRU_REFO1CON_ON_Msk) == 0) {
		while ((*reg & CRU_REFO1CON_ACTIVE_Msk) != 0) {
		}
		clock_write_reg32(reg, CRU_REFO1CON_ON_Msk, CRU_REFO1CON_ON(enable));
	}
	while ((*reg & CRU_REFO1CON_ACTIVE_Msk) == 0) {
	}
}

/**
 * @brief on/off peripheral GCLKPERIPH.
 */
void clock_gclkperiph_on_off(cfg_registers_t *cfg_regs, uint32_t subsys_val, uint8_t enable)
{
	clock_mchp_subsys_t subsys;
	__IO uint32_t *reg;
	uint32_t mask;

	subsys.val = subsys_val;
	reg = (__IO uint32_t *)((uint32_t)cfg_regs + subsys.bits.offset);
	mask = 1 << (subsys.bits.bitpos + CFGPCLKGEN_EN_POS);
	clock_write_reg32(reg, mask, enable << (subsys.bits.bitpos + CFGPCLKGEN_EN_POS));
}

/**
 * @brief function to on/off clock subsystem.
 */
static inline int clock_on_off(const clock_mchp_config_t *config, const clock_mchp_subsys_t subsys,
			       uint8_t on_off)
{
	cru_registers_t *cru_regs = config->cru_regs;
	cfg_registers_t *cfg_regs = config->cfg_regs;
	int ret_val;

	ret_val = CLOCK_SUCCESS;
	switch (subsys.bits.type) {
	case SUBSYS_TYPE_POSC:
		clock_posc_on_off(cfg_regs, cru_regs, on_off);
		break;
	case SUBSYS_TYPE_SOSC:
		clock_sosc_on_off(cru_regs, on_off);
		break;
	case SUBSYS_TYPE_PBCLK:
		clock_pbclk_on_off(cru_regs, subsys.val, on_off);
		break;
	case SUBSYS_TYPE_REFCLK:
		clock_refclk_on_off(cru_regs, subsys.val, on_off);
		break;
	case SUBSYS_TYPE_GCLKPERIPH:
		clock_gclkperiph_on_off(cfg_regs, subsys.val, on_off);
		break;
	default:
		ret_val = -ENOTSUP;
	}
	return ret_val;
}

/**
 * @brief check if subsystem type and id are valid.
 */
static int clock_check_subsys(clock_mchp_subsys_t subsys)
{
	int ret_val;
	uint32_t inst_max, offset_max, bitpos_max;

	ret_val = -EINVAL;
	inst_max = 0;
	offset_max = OFFSET_NA;
	bitpos_max = BITPOS_NA;
	do {
		/* Check if turning on all clocks is requested. */
		if (subsys.val == (uint32_t)CLOCK_CONTROL_SUBSYS_ALL) {
			break;
		}

		/* Check if the specified subsystem was found. */
		if (subsys.bits.type > SUBSYS_TYPE_MAX) {
			break;
		}

		switch (subsys.bits.type) {
		case SUBSYS_TYPE_PBCLK:
			inst_max = CLOCK_MCHP_PBCLK_ID_MAX;
			offset_max = CLOCK_MCHP_PBCLK_OFFSET_MAX;
			break;
		case SUBSYS_TYPE_REFCLK:
			inst_max = CLOCK_MCHP_REFCLK_ID_MAX;
			offset_max = CLOCK_MCHP_REFCLK_OFFSET_MAX;
			break;
		case SUBSYS_TYPE_GCLKPERIPH:
			inst_max = CLOCK_MCHP_GCLKPERIPH_ID_MAX;
			offset_max = CLOCK_MCHP_GCLKPERIPH_OFFSET_MAX;
			bitpos_max = CLOCK_MCHP_GCLKPERIPH_BITPOS_MAX;
			break;
		default:
			inst_max = 0;
		}

		/* Check if the specified id is valid. */
		if ((subsys.bits.inst > inst_max) || (subsys.bits.offset > offset_max) ||
		    (subsys.bits.bitpos > bitpos_max)) {
			break;
		}

		ret_val = CLOCK_SUCCESS;
	} while (0);

	return ret_val;
}

#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
/**
 * @brief get rate of SPLL1 in Hz.
 */
static uint32_t clock_get_rate_spll1(cru_registers_t *cru_regs)
{
	uint32_t regval, src_freq, freq, spll1_div;

	if ((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_SPLL1RDY_Msk) == 0) {
		freq = 0;
	} else {
		regval = cru_regs->CRU_SPLLCON;
		src_freq = SPLL3_FREQ;
		spll1_div = (regval & CRU_SPLLCON_SPLLPOSTDIV1_Msk) >> CRU_SPLLCON_SPLLPOSTDIV1_Pos;
		/* SPLL1 post divide value is family-specific. */
		if (spll1_div == 0U) {
			spll1_div = 1U;
		}
		freq = src_freq / spll1_div;
	}
	return freq;
}

/**
 * @brief get rate of SYSCLK in Hz.
 */
static uint32_t clock_get_rate_sysclk(cru_registers_t *cru_regs)
{
	uint32_t regval, sysclk_cosc, sysclk_frcdiv, freq;

	regval = cru_regs->CRU_OSCCON;
	sysclk_cosc = (regval & CRU_OSCCON_COSC_Msk) >> CRU_OSCCON_COSC_Pos;
	switch (sysclk_cosc) {
	case CLOCK_MCHP_SYSCLK_NOSC_FRCDIV:
		sysclk_frcdiv = (regval & CRU_OSCCON_FRCDIV_Msk) >> CRU_OSCCON_FRCDIV_Pos;
		if (sysclk_frcdiv == CLOCK_MCHP_SYSCLK_FRC_DIV_256) {
			/* There is no divider for 128. So, need to increment for shifting */
			sysclk_frcdiv++;
		}
		freq = ((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_FRCRDY_Msk) == 0)
			       ? 0
			       : FRC_FREQ / (1 << sysclk_frcdiv);
		break;
	case CLOCK_MCHP_SYSCLK_NOSC_SPLL1:
		freq = clock_get_rate_spll1(cru_regs);
		break;
	case CLOCK_MCHP_SYSCLK_NOSC_POSC:
		freq = ((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_POSCRDY_Msk) == 0) ? 0 : POSC_FREQ;
		break;
	case CLOCK_MCHP_SYSCLK_NOSC_SOSC:
		freq = ((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_SOSCRDY_Msk) == 0) ? 0 : SOSC_FREQ;
		break;
	case CLOCK_MCHP_SYSCLK_NOSC_LPRC:
		freq = ((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_LPRCRDY_Msk) == 0) ? 0 : LPRC_FREQ;
		break;
	default:
		freq = 0;
	}
	return freq;
}

/**
 * @brief get rate of PBCLK in Hz.
 */
static uint32_t clock_get_rate_pbclk(cru_registers_t *cru_regs, clock_mchp_subsys_t subsys)
{
	uint32_t regval, sysclk_freq;
	uint8_t pbclk_div;

	sysclk_freq = clock_get_rate_sysclk(cru_regs);
	regval = *(__IO uint32_t *)((uint32_t)cru_regs + subsys.bits.offset);
	pbclk_div = 1 + ((regval & CRU_PB1DIV_PBDIV_Msk) >> CRU_PB1DIV_PBDIV_Pos);
	return (sysclk_freq / pbclk_div);
}

/**
 * @brief get rate of REFCLK in Hz.
 */
static uint32_t clock_get_rate_refclk(clock_mchp_data_t *data, cru_registers_t *cru_regs,
				      clock_mchp_subsys_t subsys)
{
	uint32_t regval, refclk_src, refclk_div, refclk_trim, refclk_src_freq, freq;

	regval = *(__IO uint32_t *)((uint32_t)cru_regs + subsys.bits.offset);
	refclk_src = (regval & CRU_REFO1CON_ROSEL_Msk) >> CRU_REFO1CON_ROSEL_Pos;
	refclk_div = (regval & CRU_REFO1CON_RODIV_Msk) >> CRU_REFO1CON_RODIV_Pos;
	regval = *(__IO uint32_t *)((uint32_t)cru_regs + subsys.bits.offset + REFCLK_TRIM_OFFSET);
	refclk_trim = (regval & CRU_REFO1TRIM_ROTRIM_Msk) >> CRU_REFO1TRIM_ROTRIM_Pos;

	switch (refclk_src) {
	case CLOCK_MCHP_REFCLK_SRC_FRC:
		refclk_src_freq =
			((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_FRCRDY_Msk) == 0) ? 0 : FRC_FREQ;
		break;
	case CLOCK_MCHP_REFCLK_SRC_SPLL1:
		refclk_src_freq = clock_get_rate_spll1(cru_regs);
		break;
	case CLOCK_MCHP_REFCLK_SRC_POSC:
		refclk_src_freq =
			((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_POSCRDY_Msk) == 0) ? 0 : POSC_FREQ;
		break;
	case CLOCK_MCHP_REFCLK_SRC_SOSC:
		refclk_src_freq =
			((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_SOSCRDY_Msk) == 0) ? 0 : SOSC_FREQ;
		break;
	case CLOCK_MCHP_REFCLK_SRC_LPRC:
		refclk_src_freq =
			((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_LPRCRDY_Msk) == 0) ? 0 : LPRC_FREQ;
		break;
	case CLOCK_MCHP_REFCLK_SRC_SPLL3:
		refclk_src_freq =
			#if CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_BZ6
			((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_SPLLRDY_Msk) == 0) ? 0 : SPLL3_FREQ;
#else
			((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_SPLL3RDY_Msk) == 0) ? 0 : SPLL3_FREQ;
#endif
		break;
	case CLOCK_MCHP_REFCLK_SRC_PBCLK1:
		clock_mchp_subsys_t src_subsys;

		src_subsys.val = CLOCK_MCHP_PBCLK_ID_1;
		refclk_src_freq = clock_get_rate_pbclk(cru_regs, src_subsys);
		break;
	case CLOCK_MCHP_REFCLK_SRC_SYSCLK:
		refclk_src_freq = clock_get_rate_sysclk(cru_regs);
		break;
	case CLOCK_MCHP_REFCLK_SRC_REFIPIN:
		refclk_src_freq = data->refi_freq[subsys.bits.inst];
		break;
	default:
		refclk_src_freq = 0;
	}

	/* if refclk_div is 0, REFOx clock is same frequency as Base clock (no divider) */
	freq = (refclk_div == 0) ? refclk_src_freq
				 : (refclk_src_freq * TRIM_VALUE_DIVIDER) /
					   (2 * (TRIM_VALUE_DIVIDER * refclk_div + refclk_trim));
	return freq;
}

/**
 * @brief get clock rate of VBKP low power clock in Hz.
 */
static uint32_t clock_get_rate_vbkpclk(cfg_registers_t *cfg_regs, cru_registers_t *cru_regs)
{
	uint32_t calc_vbkpclk_freq;

	/* Calculate VBKP clock frequency */
	uint32_t vbkpclk_src = (cfg_regs->CFG_CFGCON4 & CFG_CFGCON4_VBKP_32KCSEL_Msk) >>
			       CFG_CFGCON4_VBKP_32KCSEL_Pos;
	switch (vbkpclk_src) {
	case CLOCK_MCHP_VBKPCLK_SRC_FRC:
		calc_vbkpclk_freq = ((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_FRCRDY_Msk) == 0)
					    ? 0
					    : FRC_FREQ_BY_250;
		break;
	case CLOCK_MCHP_VBKPCLK_SRC_POSC:
		calc_vbkpclk_freq = ((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_POSCRDY_Msk) == 0)
					    ? 0
					    : POSC_FREQ_BY_500;
		break;
	case CLOCK_MCHP_VBKPCLK_SRC_SOSC:
		calc_vbkpclk_freq =
			((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_SOSCRDY_Msk) == 0) ? 0 : SOSC_FREQ;
		break;
	case CLOCK_MCHP_VBKPCLK_SRC_LPRC:
		calc_vbkpclk_freq =
			((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_LPRCRDY_Msk) == 0) ? 0 : LPRC_FREQ;
		break;
	}

	return calc_vbkpclk_freq;
}

/**
 * @brief get clock rate of RTC, LPCLK and Deep-Sleep Watchdog in Hz.
 */
static void clock_get_rate_rtc_lp_dswdt(cfg_registers_t *cfg_regs, cru_registers_t *cru_regs,
					uint32_t *rtc_freq, uint32_t *lpclk_freq,
					uint32_t *dswdt_freq)
{

	/* Calculate VBKP clock frequency */
	uint32_t calc_vbkpclk_freq = clock_get_rate_vbkpclk(cfg_regs, cru_regs);

	/* Calculate LPCLK frequency */
	uint32_t lpclk_div =
		(cfg_regs->CFG_CFGCON4 & CFG_CFGCON4_LPCLK_MOD_Msk) >> CFG_CFGCON4_LPCLK_MOD_Pos;
	uint32_t calc_lpclk_freq = (lpclk_div == CLOCK_MCHP_LPCLK_DIV_BY_1)
					   ? calc_vbkpclk_freq
					   : DIV_BY_1_024(calc_vbkpclk_freq);

	/* Calculate Deep Sleep WDT frequency */
	uint32_t dswdtosc_src =
		(cfg_regs->CFG_CFGCON4 & CFG_CFGCON4_DSWDTOSC_Msk) >> CFG_CFGCON4_DSWDTOSC_Pos;
	uint32_t calc_dswdt_freq = (dswdtosc_src == CLOCK_MCHP_DSWDTCLK_SRC_SOSC)
					   ? calc_vbkpclk_freq
					   : calc_lpclk_freq;

	/* Calculate RTC frequency */
	uint32_t vbkp_divsel = (cfg_regs->CFG_CFGCON4 & CFG_CFGCON4_VBKP_DIVSEL_Msk) >>
			       CFG_CFGCON4_VBKP_DIVSEL_Pos;
	uint32_t vbkp_divsel_outfreq = (vbkp_divsel == CLOCK_MCHP_RTCCLK_VBKP_DIV_BY_32)
					       ? DIV_BY_32(calc_vbkpclk_freq)
					       : DIV_BY_31_25(calc_vbkpclk_freq);

	uint32_t rtccntm_csel = (cfg_regs->CFG_CFGCON4 & CFG_CFGCON4_RTCNTM_CSEL_Msk) >>
				CFG_CFGCON4_RTCNTM_CSEL_Pos;
	uint32_t rtc_in_freq = (rtccntm_csel == CLOCK_MCHP_RTCCLK_MODE_PROCESSED_32K)
				       ? calc_lpclk_freq
				       : calc_vbkpclk_freq;

	uint32_t vbkp_1kcsel = (cfg_regs->CFG_CFGCON4 & CFG_CFGCON4_VBKP_1KCSEL_Msk) >>
			       CFG_CFGCON4_VBKP_1KCSEL_Pos;
	uint32_t calc_rtc_freq =
		(vbkp_1kcsel == CLOCK_MCHP_RTCCLK_VBKP_32KHZ) ? rtc_in_freq : vbkp_divsel_outfreq;

	if (rtc_freq != NULL) {
		*rtc_freq = calc_rtc_freq;
	}
	if (lpclk_freq != NULL) {
		*lpclk_freq = calc_lpclk_freq;
	}
	if (dswdt_freq != NULL) {
		*dswdt_freq = calc_dswdt_freq;
	}
}
#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */

/******************************************************************************
 * @brief API functions
 *****************************************************************************/

/**
 * @brief Turn on the clock for a specified subsystem, can be blocking.
 *
 * @param dev Pointer to the clock device structure
 * @param sys Clock subsystem
 *
 * @return 0 if the clock is successfully turned on
 * @return -ENOTSUP If the requested operation is not supported.
 * @return -ETIMEDOUT If the requested operation is timedout.
 */
static int clock_mchp_on(const struct device *dev, clock_control_subsys_t sys)
{
	const clock_mchp_config_t *config = dev->config;
	clock_mchp_subsys_t subsys;

	int ret_val;
	enum clock_control_status status;
	bool is_wait = false;
	uint32_t on_timeout_ms = 0;

	subsys.val = (uint32_t)sys;
	ret_val = -ENOTSUP;
	do {
		/* Validate subsystem. */
		if (0 != clock_check_subsys(subsys)) {
			break;
		}

		status = clock_mchp_get_status(dev, sys);
		if (status == CLOCK_CONTROL_STATUS_ON) {
			/* clock is already on. */
			ret_val = -EALREADY;
			break;
		}

		/* Check if the clock on operation is successful. */
		if (clock_on_off(config, subsys, CLOCK_ON) == 0) {
			is_wait = true;
		}
	} while (0);

	/* Wait until the clock state becomes ON. */
	while (is_wait == true) {
		status = clock_mchp_get_status(dev, sys);
		if (status == CLOCK_CONTROL_STATUS_ON) {
			/* Successfully turned on clock. */
			ret_val = CLOCK_SUCCESS;
			break;
		}
		if (on_timeout_ms < config->on_timeout_ms) {
			/* Thread is not available while booting. */
			if ((k_is_pre_kernel() == false) && (k_current_get() != NULL)) {
				/* Sleep before checking again. */
				k_sleep(K_MSEC(1));
				on_timeout_ms++;
			}
		} else {
			/* Clock on timeout occurred */
			ret_val = -ETIMEDOUT;
			break;
		}
	}

	return ret_val;
}

/**
 * @brief Turn off the clock for a specified subsystem.
 *
 * @param dev Pointer to the clock device structure
 * @param sys Clock subsystem
 *
 * @return 0 if the clock is successfully turned off
 * @return -ENOTSUP If the requested operation is not supported.
 */
static int clock_mchp_off(const struct device *dev, clock_control_subsys_t sys)
{
	const clock_mchp_config_t *config = dev->config;
	clock_mchp_subsys_t subsys;
	enum clock_control_status status;

	/* Return value for the operation status. */
	int ret_val;

	subsys.val = (uint32_t)sys;
	ret_val = -ENOTSUP;
	do {
		/* Validate subsystem. */
		if (0 != clock_check_subsys(subsys)) {
			break;
		}
		status = clock_mchp_get_status(dev, sys);
		if (status == CLOCK_CONTROL_STATUS_OFF) {
			/* clock is already off. */
			ret_val = -EALREADY;
			break;
		}
		ret_val = clock_on_off(config, subsys, CLOCK_OFF);
	} while (0);

	return ret_val;
}

/**
 * @brief Get the status of clock, for a specified subsystem.
 *
 * This function retrieves the current status of the clock for a given subsystem.
 *
 * @param dev Pointer to the clock device structure.
 * @param sys The clock subsystem.
 *
 * @return The current status of clock for the subsystem (e.g., off, on, starting, or unknown).
 */
static enum clock_control_status clock_mchp_get_status(const struct device *dev,
						       clock_control_subsys_t sys)
{
	const clock_mchp_config_t *config = dev->config;
	cru_registers_t *cru_regs = config->cru_regs;
	cfg_registers_t *cfg_regs = config->cfg_regs;
	clock_mchp_subsys_t subsys;
	__IO uint32_t *reg32;
	uint32_t clkstat_mask;

	subsys.val = (uint32_t)sys;

	/* Variable to store the returned status to indicate state is not yet known. */
	enum clock_control_status ret_status = CLOCK_CONTROL_STATUS_UNKNOWN;

	do {
		/* Validate subsystem. */
		if (0 != clock_check_subsys(subsys)) {
			break;
		}

		switch (subsys.bits.type) {
		case SUBSYS_TYPE_FRC:
			clkstat_mask = CRU_CLKSTAT_FRCRDY_Msk;
			break;
		case SUBSYS_TYPE_POSC:
			clkstat_mask = CRU_CLKSTAT_POSCRDY_Msk;
			break;
		case SUBSYS_TYPE_SPLL1:
			clkstat_mask = CRU_CLKSTAT_SPLL1RDY_Msk;
			break;
		case SUBSYS_TYPE_SPLL3:
			#if CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_BZ6
			clkstat_mask = CRU_CLKSTAT_SPLLRDY_Msk;
#else
			clkstat_mask = CRU_CLKSTAT_SPLL3RDY_Msk;
#endif
			break;
		case SUBSYS_TYPE_LPRC:
			clkstat_mask = CRU_CLKSTAT_LPRCRDY_Msk;
			break;
		default:
			clkstat_mask = 0;
		}
		if (clkstat_mask != 0) {
			ret_status = ((cru_regs->CRU_CLKSTAT & clkstat_mask) == 0)
					     ? CLOCK_CONTROL_STATUS_OFF
					     : CLOCK_CONTROL_STATUS_ON;
			break;
		}

		switch (subsys.bits.type) {
		case SUBSYS_TYPE_SOSC:
			/* Check if SOSC is enabled */
			if ((cru_regs->CRU_OSCCON & CRU_OSCCON_SOSCEN_Msk) != 0) {
				/* Check if ready bit is set */
				ret_status =
					((cru_regs->CRU_CLKSTAT & CRU_CLKSTAT_SOSCRDY_Msk) == 0)
						? CLOCK_CONTROL_STATUS_STARTING
						: CLOCK_CONTROL_STATUS_ON;
			} else {
				ret_status = CLOCK_CONTROL_STATUS_OFF;
			}
			break;
		case SUBSYS_TYPE_SPLL2:
			/* Check if Post Divide Value is 0 */
			ret_status = ((cru_regs->CRU_SPLLCON & CRU_SPLLCON_SPLLPOSTDIV2_Msk) == 0)
					     ? CLOCK_CONTROL_STATUS_OFF
					     : CLOCK_CONTROL_STATUS_ON;
			break;
		case SUBSYS_TYPE_PBCLK:
			/* Check if PBCLK is enabled */
			reg32 = (__IO uint32_t *)((uint32_t)cru_regs + subsys.bits.offset);
			ret_status = ((*reg32 & CRU_PB1DIV_PBDIVON_Msk) != 0)
					     ? CLOCK_CONTROL_STATUS_ON
					     : CLOCK_CONTROL_STATUS_OFF;
			break;
		case SUBSYS_TYPE_REFCLK:
			/* Check if REFCLK is enabled */
			reg32 = (__IO uint32_t *)((uint32_t)cru_regs + subsys.bits.offset);
			if ((*reg32 & CRU_REFO1CON_ON_Msk) != 0) {
				/* Check if active bit is set */
				ret_status = ((*reg32 & CRU_REFO1CON_ACTIVE_Msk) == 0)
						     ? CLOCK_CONTROL_STATUS_STARTING
						     : CLOCK_CONTROL_STATUS_ON;
			} else {
				ret_status = CLOCK_CONTROL_STATUS_OFF;
			}
			break;
		case SUBSYS_TYPE_GCLKPERIPH:
			/* Check if GCLKPERIPH is enabled */
			reg32 = (__IO uint32_t *)((uint32_t)cfg_regs + subsys.bits.offset);
			ret_status =
				((*reg32 & (1 << (subsys.bits.bitpos + CFGPCLKGEN_EN_POS))) != 0)
					? CLOCK_CONTROL_STATUS_ON
					: CLOCK_CONTROL_STATUS_OFF;
			break;
		case SUBSYS_TYPE_SYSCLK:
		case SUBSYS_TYPE_WDTCLK:
		case SUBSYS_TYPE_VBKPCLK:
		case SUBSYS_TYPE_DSWDTCLK:
		case SUBSYS_TYPE_LPCLK:
		case SUBSYS_TYPE_RTCCLK:
			ret_status = CLOCK_CONTROL_STATUS_ON;
			break;
		}
	} while (0);

	/* Return the status of the clock for the specified subsystem. */
	return ret_status;
}

#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
/**
 * @brief Get the rate of the clock for a specified subsystem.
 *
 * This function retrieves the clock frequency (Hz) for the given subsystem.
 *
 * @param dev Pointer to clock device structure.
 * @param sys The clock subsystem.
 * @param frequency Pointer to store the retrieved clock rate.
 *
 * @return 0 if the rate is successfully retrieved.
 * @return -ENOTSUP If the requested operation is not supported.
 */
static int clock_mchp_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *freq)
{
	const clock_mchp_config_t *config = dev->config;
	clock_mchp_data_t *data = dev->data;
	cru_registers_t *cru_regs = config->cru_regs;
	cfg_registers_t *cfg_regs = config->cfg_regs;
	clock_mchp_subsys_t subsys, src_subsys;
	uint32_t regval, spll2_src, spll2_div, spll2_src_freq, gclkperiph_src;
	int ret_val;

	subsys.val = (uint32_t)sys;
	ret_val = CLOCK_SUCCESS;
	do {
		/* Validate subsystem. */
		if (0 != clock_check_subsys(subsys)) {
			ret_val = -ENOTSUP;
			break;
		}

		/* Return rate as 0, if clock is not on */
		if (clock_mchp_get_status(dev, sys) != CLOCK_CONTROL_STATUS_ON) {
			*freq = 0;
			break;
		}

		switch (subsys.bits.type) {
		case SUBSYS_TYPE_POSC:
			*freq = POSC_FREQ;
			break;
		case SUBSYS_TYPE_SOSC:
			*freq = SOSC_FREQ;
			break;
		case SUBSYS_TYPE_SYSCLK:
			*freq = clock_get_rate_sysclk(cru_regs);
			break;
		case SUBSYS_TYPE_SPLL1:
			*freq = clock_get_rate_spll1(cru_regs);
			break;
		case SUBSYS_TYPE_SPLL2:
			regval = cru_regs->CRU_SPLLCON;
			spll2_src = (regval & CRU_SPLLCON_SPLL_BYP_Msk) >> CRU_SPLLCON_SPLL_BYP_Pos;
			spll2_div = (regval & CRU_SPLLCON_SPLLPOSTDIV2_Msk) >>
				    CRU_SPLLCON_SPLLPOSTDIV2_Pos;
			spll2_src_freq = 0;
			switch (spll2_src) {
			case CLOCK_MCHP_SPLL2_SRC_SPLL3:
				spll2_src_freq = SPLL3_FREQ;
				break;
			case CLOCK_MCHP_SPLL2_SRC_FRC:
				spll2_src_freq = FRC_FREQ;
				break;
			case CLOCK_MCHP_SPLL2_SRC_POSC:
				spll2_src_freq = POSC_FREQ;
				break;
			}
			*freq = (spll2_div == 0) ? 0 : spll2_src_freq / spll2_div;
			break;

		case SUBSYS_TYPE_PBCLK:
			*freq = clock_get_rate_pbclk(cru_regs, subsys);
			break;
		case SUBSYS_TYPE_REFCLK:
			*freq = clock_get_rate_refclk(data, cru_regs, subsys);
			break;
		case SUBSYS_TYPE_GCLKPERIPH:
			regval = *(__IO uint32_t *)((uint32_t)cfg_regs + subsys.bits.offset);
			gclkperiph_src = (regval & (CFGPCLKGEN_SRC_MASK << subsys.bits.bitpos)) >>
					 subsys.bits.bitpos;
			if (gclkperiph_src == CLOCK_MCHP_GCLKPERIPH_SRC_NOCLK) {
				*freq = 0;
			} else if (gclkperiph_src == CLOCK_MCHP_GCLKPERIPH_SRC_LPCLK) {
				clock_get_rate_rtc_lp_dswdt(cfg_regs, cru_regs, NULL, freq, NULL);
			} else {
				src_subsys.val = MCHP_CLOCK_DERIVE_ID(
					SUBSYS_TYPE_REFCLK, gclkperiph_src - 1,
					CRU_REFO1CON_REG_OFST + ((gclkperiph_src - 1) *
								 CLOCK_REFOCON_REG_NEXT_OFST),
					BITPOS_NA);
				*freq = clock_get_rate_refclk(data, cru_regs, src_subsys);
			}
			break;
		case SUBSYS_TYPE_WDTCLK:
			*freq = (((cfg_regs->CFG_CFGCON2 & CFG_CFGCON2_WDTRMCS_Msk) >>
				  CFG_CFGCON2_WDTRMCS_Pos) == CLOCK_MCHP_WDTCLK_RUN_MOD_PBCLK)
					? clock_get_rate_sysclk(cru_regs)
					: LPRC_FREQ;
			break;
		case SUBSYS_TYPE_VBKPCLK:
			*freq = clock_get_rate_vbkpclk(cfg_regs, cru_regs);
			break;

		case SUBSYS_TYPE_DSWDTCLK:
			clock_get_rate_rtc_lp_dswdt(cfg_regs, cru_regs, NULL, NULL, freq);
			break;
		case SUBSYS_TYPE_LPCLK:
			clock_get_rate_rtc_lp_dswdt(cfg_regs, cru_regs, NULL, freq, NULL);
			break;
		case SUBSYS_TYPE_RTCCLK:
			clock_get_rate_rtc_lp_dswdt(cfg_regs, cru_regs, freq, NULL, NULL);
			break;
		case CLOCK_MCHP_FRC_ID:
			*freq = FRC_FREQ;
			break;
		case CLOCK_MCHP_LPRC_ID:
			*freq = LPRC_FREQ;
			break;
		case CLOCK_MCHP_SPLL3_ID:
			*freq = SPLL3_FREQ;
			break;
		}
	} while (0);

	return ret_val;
}
#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_RUNTIME
/**
 * @brief Configure the clock for a specified subsystem.
 *
 * req_config is typecasted to corresponding structure type, according to the clock
 * subsystem.
 *
 * @param dev Pointer to clock device structure.
 * @param sys The clock subsystem.
 * @param req_config Pointer to the requested configuration for the clock.
 *
 * @return 0 if the configuration is successful.
 * @return -EINVAL if req_config is not a valid value.
 * @return -ENOTSUP If the requested operation is not supported.
 */
static int clock_mchp_configure(const struct device *dev, clock_control_subsys_t sys,
				void *req_config)
{
	const clock_mchp_config_t *config = dev->config;
	cru_registers_t *cru_regs = config->cru_regs;
	cfg_registers_t *cfg_regs = config->cfg_regs;
	clock_mchp_subsys_t subsys;
	int ret_val;

	subsys.val = (uint32_t)sys;
	ret_val = CLOCK_SUCCESS;
	do {
		if (req_config == NULL) {
			ret_val = -EINVAL;
			break;
		}
		/* Validate subsystem. */
		if (0 != clock_check_subsys(subsys)) {
			ret_val = -ENOTSUP;
			break;
		}

		switch (subsys.bits.type) {
		case SUBSYS_TYPE_SYSCLK:
			struct clock_mchp_subsys_sysclk_config *sysclk_config =
				(struct clock_mchp_subsys_sysclk_config *)req_config;
			clock_sysclk_config(cru_regs, sysclk_config->frc_div,
					    sysclk_config->new_osc);
			break;
		case SUBSYS_TYPE_SPLL1:
			struct clock_mchp_subsys_spll1_config *spll1_config =
				(struct clock_mchp_subsys_spll1_config *)req_config;
			clock_spll1_config(cru_regs, spll1_config->post_div);
			break;
		case SUBSYS_TYPE_SPLL2:
			struct clock_mchp_subsys_spll2_config *spll2_config =
				(struct clock_mchp_subsys_spll2_config *)req_config;
			clock_spll2_config(cru_regs, spll2_config->src, spll2_config->post_div);
			break;
		case SUBSYS_TYPE_PBCLK:
			struct clock_mchp_subsys_pbclk_config *pbclk_config =
				(struct clock_mchp_subsys_pbclk_config *)req_config;
			clock_pbclk_config(cru_regs, (uint32_t)sys, pbclk_config->div);
			break;
		case SUBSYS_TYPE_REFCLK:
			clock_refclk_config(cru_regs, (uint32_t)sys,
					    (struct clock_mchp_subsys_refclk_config *)req_config);
			break;
		case SUBSYS_TYPE_GCLKPERIPH:
			struct clock_mchp_subsys_gclkperiph_config *gclkperiph_config =
				(struct clock_mchp_subsys_gclkperiph_config *)req_config;
			clock_gclkperiph_config(cfg_regs, (uint32_t)sys,
						gclkperiph_config->src_sel);
			break;
		case SUBSYS_TYPE_WDTCLK:
			struct clock_mchp_subsys_wdtclk_config *wdtclk_config =
				(struct clock_mchp_subsys_wdtclk_config *)req_config;
			clock_wdtclk_config(cfg_regs, wdtclk_config->run_mode_clock_sel);
			break;
		case SUBSYS_TYPE_VBKPCLK:
			struct clock_mchp_subsys_vbkpclk_config *vbkpclk_config =
				(struct clock_mchp_subsys_vbkpclk_config *)req_config;
			clock_vbkpclk_config(cfg_regs, vbkpclk_config->src_sel);
			break;
		case SUBSYS_TYPE_DSWDTCLK:
			struct clock_mchp_subsys_dswdtclk_config *dswdtclk_config =
				(struct clock_mchp_subsys_dswdtclk_config *)req_config;
			clock_dswdtclk_config(cfg_regs, dswdtclk_config->src_sel);
			break;
		case SUBSYS_TYPE_LPCLK:
			struct clock_mchp_subsys_lpclk_config *lpclk_config =
				(struct clock_mchp_subsys_lpclk_config *)req_config;
			clock_lpclk_config(cfg_regs, lpclk_config->div_modifier);
			break;
		case SUBSYS_TYPE_RTCCLK:
			struct clock_mchp_subsys_rtcclk_config *rtcclk_config =
				(struct clock_mchp_subsys_rtcclk_config *)req_config;
			clock_rtcclk_config(cfg_regs, rtcclk_config->counter_mode_sel,
					    rtcclk_config->vbkp_div_sel,
					    rtcclk_config->vbkp_1k_32k_sel);
			break;

		default:
			ret_val = -ENOTSUP;
		}
	} while (0);

	return ret_val;
}
#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_RUNTIME */

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP

#define CLOCK_MCHP_ITERATE_PBCLK(child)                                                            \
	clock_pbclk_config(cru_regs, DT_PROP(child, subsystem), DT_PROP(child, pbclk_div));        \
	clock_pbclk_on_off(cru_regs, DT_PROP(child, subsystem), DT_PROP(child, pbclk_en));

#define CLOCK_MCHP_ITERATE_REFCLK(child)                                                           \
	refclk_config.div = DT_PROP(child, refclk_div);                                            \
	refclk_config.stop_in_idle_en = DT_PROP(child, refclk_stop_in_idle_en);                    \
	refclk_config.run_in_sleep_en = DT_PROP(child, refclk_run_in_sleep_en);                    \
	refclk_config.pin_out_en = DT_PROP(child, refclk_pin_out_en);                              \
	refclk_config.src_sel = DT_ENUM_IDX(child, refclk_src_sel);                                \
	refclk_config.trim_val = DT_PROP(child, refclk_trim_val);                                  \
	clock_refclk_config(cru_regs, DT_PROP(child, subsystem), &refclk_config);                  \
	clock_refclk_set_refi_freq(dev, DT_PROP(child, subsystem),                                 \
				   DT_PROP(child, refclk_refi_freq));                              \
	clock_refclk_on_off(cru_regs, DT_PROP(child, subsystem), DT_PROP(child, refclk_en));

#define CLOCK_MCHP_ITERATE_GCLKPERIPH(child)                                                       \
	clock_gclkperiph_config(cfg_regs, DT_PROP(child, subsystem),                               \
				DT_ENUM_IDX(child, gclkperiph_src_sel));                           \
	clock_gclkperiph_on_off(cfg_regs, DT_PROP(child, subsystem), DT_PROP(child, gclkperiph_en));

#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP */

/**
 * @brief clock driver initialization function.
 */
#if CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_BZ6
static void clock_rf_write_reg(uint8_t addr, uint16_t value)
{
	BLE_REGS->BLE_SPI_REG_ADDR = addr;
	BLE_REGS->BLE_SPI_WR_DATA = value;
	BLE_REGS->BLE_RFPWRMGMT |= 0x00100000U;
	while ((BLE_REGS->BLE_RFPWRMGMT & 0x00100000U) != 0U) {
		/* Nothing to do */
	}
}
#endif

static int clock_mchp_init(const struct device *dev)
{
#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP
	const clock_mchp_config_t *config = dev->config;
	cru_registers_t *cru_regs = config->cru_regs;
	cfg_registers_t *cfg_regs = config->cfg_regs;
	struct clock_mchp_subsys_refclk_config refclk_config;
#endif

	/* Check CLDO ready. */
	while ((CFG_REGS->CFG_MISCSTAT & CFG_MISCSTAT_CLDORDY_Msk) == 0U) {
		/* Nothing to do */
	}

	clock_rf_write_reg(0x27U, 0x2078U);

	if ((CRU_REGS->CRU_OSCCON & CRU_OSCCON_COSC_Msk) != CRU_OSCCON_COSC_SPLL) {
		clock_rf_write_reg(0x2EU, 0x4328U);

#if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 64000000)
		typedef void (*func_pche_setup)(uint32_t setup);
		(void)((func_pche_setup)(*(uint32_t *)0xF2D0U))(
			(PCHE_REGS->PCHE_CHECON &
			 (~(PCHE_CHECON_PFMWS_Msk | PCHE_CHECON_ADRWS_Msk |
			    PCHE_CHECON_PREFEN_Msk))) |
			(PCHE_CHECON_PFMWS(1) | PCHE_CHECON_PREFEN(1) | PCHE_CHECON_ADRWS(1)));
#elif (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 128000000)
		typedef void (*func_pche_setup)(uint32_t setup);
		(void)((func_pche_setup)(*(uint32_t *)0xF2D0U))(
			(PCHE_REGS->PCHE_CHECON &
			 (~(PCHE_CHECON_PFMWS_Msk | PCHE_CHECON_ADRWS_Msk |
			    PCHE_CHECON_PREFEN_Msk))) |
			(PCHE_CHECON_PFMWS(4) | PCHE_CHECON_PREFEN(1) | PCHE_CHECON_ADRWS(1)));
#else
#error "SYS_CLOCK_HW_CYCLES_PER_SEC value is not supported"
#endif
	}

	while ((BTZBSYS_REGS->BTZBSYS_SUBSYS_STATUS_REG1 &
		BTZBSYS_SUBSYS_STATUS_REG1_xtal_ready_out_Msk) !=
	       BTZBSYS_SUBSYS_STATUS_REG1_xtal_ready_out_Msk) {
		/* Nothing to do */
	}

	CFG_REGS->CFG_SYSKEY = 0x00000000U;
	CFG_REGS->CFG_SYSKEY = 0xAA996655U;
	CFG_REGS->CFG_SYSKEY = 0x556699AAU;
	CRU_REGS->CRU_OSCCON = 0x200U;
	CRU_REGS->CRU_OSCCONSET = CRU_OSCCON_OSWEN_Msk;
	while ((CRU_REGS->CRU_OSCCON & CRU_OSCCON_OSWEN_Msk) != 0U) {
		/* Nothing to do */
	}

	BLE_REGS->BLE_DPLL_RG2 |= 0x02U;
	BLE_REGS->BLE_DPLL_RG2 &= ((uint8_t)~(0x02U));
	CFG_REGS->CFG_MISCSTAT &= 0x00FFFFFFU;
	BTZBSYS_REGS->BTZBSYS_SUBSYS_CNTRL_REG3 =
		((BTZBSYS_REGS->BTZBSYS_SUBSYS_CNTRL_REG3 &
		  ~BTZBSYS_SUBSYS_CNTRL_REG3_subsys_pll_ready_delay_Msk) |
		 ((0x02UL) << BTZBSYS_SUBSYS_CNTRL_REG3_subsys_pll_ready_delay_Pos));

	CFG_REGS->CFG_SYSKEY = 0x00000000U;
	CFG_REGS->CFG_SYSKEY = 0xAA996655U;
	CFG_REGS->CFG_SYSKEY = 0x556699AAU;

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP
	clock_posc_on_off(cfg_regs, cru_regs, DT_PROP(DT_NODELABEL(posc), posc_en));
	clock_sosc_on_off(cru_regs, DT_PROP(DT_NODELABEL(sosc), sosc_en));
	clock_sysclk_config(cru_regs, DT_ENUM_IDX(DT_NODELABEL(sysclk), sysclk_frc_div),
			    DT_ENUM_IDX(DT_NODELABEL(sysclk), sysclk_new_osc));

#if CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_BZ6
#if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 64000000)
	clock_spll1_config(cru_regs, 2U);
	clock_spll2_config(cru_regs, CRU_SPLLCON_SPLL_BYP_FRC_Val,
			   CRU_SPLLCON_SPLLPOSTDIV2_DIV_1_Val);
#elif (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 128000000)
	clock_spll1_config(cru_regs, 1U);
	clock_spll2_config(cru_regs, CRU_SPLLCON_SPLL_BYP_FRC_Val,
			   CRU_SPLLCON_SPLLPOSTDIV2_DIV_1_Val);
#else
#error "SYS_CLOCK_HW_CYCLES_PER_SEC value is not supported"
#endif
#endif

	DT_FOREACH_CHILD(DT_NODELABEL(pbclk), CLOCK_MCHP_ITERATE_PBCLK);
	DT_FOREACH_CHILD(DT_NODELABEL(refclk), CLOCK_MCHP_ITERATE_REFCLK);
	DT_FOREACH_CHILD(DT_NODELABEL(gclkperiph), CLOCK_MCHP_ITERATE_GCLKPERIPH);

	clock_wdtclk_config(cfg_regs, DT_ENUM_IDX(DT_NODELABEL(wdtclk), wdtclk_run_mode_clock_sel));
	clock_vbkpclk_config(cfg_regs, DT_ENUM_IDX(DT_NODELABEL(vbkpclk), vbkpclk_32kc_src_sel));
	clock_dswdtclk_config(cfg_regs, DT_ENUM_IDX(DT_NODELABEL(dswdtclk), dswdtclk_src_sel));
	clock_lpclk_config(cfg_regs, DT_ENUM_IDX(DT_NODELABEL(lpclk), lpclk_div_modifier));
	clock_rtcclk_config(cfg_regs, DT_ENUM_IDX(DT_NODELABEL(rtcclk), rtcclk_counter_mode_sel),
			    DT_ENUM_IDX(DT_NODELABEL(rtcclk), rtcclk_vbkp_div_sel),
			    DT_ENUM_IDX(DT_NODELABEL(rtcclk), rtcclk_vbkp_1k_32k_sel));
#endif

	BTZBSYS_REGS->BTZBSYS_SUBSYS_CNTRL_REG1 |= 0x00000010U;
	BTZBSYS_REGS->BTZBSYS_SUBSYS_CNTRL_REG0 |= 0x01000000U;

#if CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_BZ6
	CRU_REGS->CRU_UPLLCON = CRU_UPLLCON_UPLLPWDN_Msk;
	CRU_REGS->CRU_EPLLCON = CRU_EPLLCON_EPLLPWDN_Msk;
#endif

	CFG_REGS->CFG_SYSKEY = 0x33333333U;

#if CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_BZ6 && (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 64000000)
	typedef void (*func_pche_setup)(uint32_t setup);
	(void)((func_pche_setup)(*(uint32_t *)0xF2D0U))(
		(PCHE_REGS->PCHE_CHECON &
		 (~(PCHE_CHECON_PFMWS_Msk | PCHE_CHECON_ADRWS_Msk | PCHE_CHECON_PREFEN_Msk))) |
		(PCHE_CHECON_PFMWS(1) | PCHE_CHECON_PREFEN(1) | PCHE_CHECON_ADRWS(1)));
#endif

	return CLOCK_SUCCESS;
}
/******************************************************************************
 * @brief Zephyr driver instance creation
 *****************************************************************************/
static DEVICE_API(clock_control, clock_mchp_driver_api) = {
	.on = clock_mchp_on,
	.off = clock_mchp_off,
	.get_status = clock_mchp_get_status,
#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
	.get_rate = clock_mchp_get_rate,
#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */
#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_RUNTIME
	.configure = clock_mchp_configure,
#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_RUNTIME */
};

#define CLOCK_MCHP_CONFIG_DEFN(n) \
	static const clock_mchp_config_t clock_mchp_config_##n = { \
		.on_timeout_ms = DT_PROP_OR(CLOCK_NODE, on_timeout_ms, 5), \
		.cru_regs = (cru_registers_t *)DT_REG_ADDR_BY_IDX(CLOCK_NODE, 1), \
		.cfg_regs = (cfg_registers_t *)DT_REG_ADDR_BY_IDX(CLOCK_NODE, 0) \
	}

#define CLOCK_MCHP_DATA_DEFN(n) static clock_mchp_data_t clock_mchp_data_##n;

#define CLOCK_MCHP_DEVICE_INIT(n) \
	CLOCK_MCHP_CONFIG_DEFN(n); \
	CLOCK_MCHP_DATA_DEFN(n); \
	DEVICE_DT_INST_DEFINE(n, clock_mchp_init, NULL, &clock_mchp_data_##n, \
			      &clock_mchp_config_##n, PRE_KERNEL_1, \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_mchp_driver_api);

#define DT_DRV_COMPAT microchip_pic32cx_bz6_clock
DT_INST_FOREACH_STATUS_OKAY(CLOCK_MCHP_DEVICE_INIT)
#undef DT_DRV_COMPAT
