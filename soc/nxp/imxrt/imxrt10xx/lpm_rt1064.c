/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Note: this file is linked to RAM. Any functions called while changing clocks
 * to the flexspi modules must be linked to RAM, or within this file
 */

#include <zephyr/init.h>
#include <power.h>
#include <zephyr/kernel.h>
#include <fsl_clock.h>

/*
 * Clock configuration structures populated at boot time. These structures are
 * used to reinitialize the PLLs after exiting low power mode.
 */
#ifdef CONFIG_INIT_ARM_PLL
static clock_arm_pll_config_t arm_pll_config;
#endif
#ifdef CONFIG_INIT_VIDEO_PLL
static clock_video_pll_config_t video_pll_config;
#endif
#ifdef CONFIG_INIT_ENET_PLL
static clock_enet_pll_config_t enet_pll_config;
#endif
static clock_sys_pll_config_t sys_pll_config;
static clock_usb_pll_config_t usb1_pll_config;

#define IMX_RT_SYS_PFD_FRAC(reg, pfd_num) \
	(((reg) >> (8U * pfd_num)) &\
	CCM_ANALOG_PFD_528_PFD0_FRAC_MASK)
#define IMX_RT_USB1_PFD_FRAC(reg, pfd_num) \
	(((reg) >> (8U * pfd_num)) &\
	CCM_ANALOG_PFD_480_PFD0_FRAC_MASK)

uint8_t sys_pll_pfd0_frac;
uint8_t sys_pll_pfd1_frac;
uint8_t sys_pll_pfd2_frac;
uint8_t sys_pll_pfd3_frac;

uint8_t usb1_pll_pfd1_frac;
uint8_t usb1_pll_pfd2_frac;
uint8_t usb1_pll_pfd3_frac;

uint32_t flexspi_div;

/*
 * Duplicate implementation of CLOCK_SetMux() provided by SDK. This function
 * must be linked to ITCM, as it will be used to change the clocks of the
 * FLEXSPI and SEMC peripherals.
 * Any function called from this function must also reside in ITCM
 */
static void clock_set_mux(clock_mux_t mux, uint32_t value)
{
	uint32_t busy_shift;

	busy_shift = (uint32_t)CCM_TUPLE_BUSY_SHIFT(mux);
	CCM_TUPLE_REG(CCM, mux) = (CCM_TUPLE_REG(CCM, mux) & (~CCM_TUPLE_MASK(mux))) |
		  (((uint32_t)((value) << CCM_TUPLE_SHIFT(mux))) & CCM_TUPLE_MASK(mux));

	/* Clock switch need Handshake? */
	if (busy_shift != CCM_NO_BUSY_WAIT) {
		/* Wait until CCM internal handshake finish. */
		while ((CCM->CDHIPR & ((1UL << busy_shift))) != 0UL) {
		}
	}
}

/*
 * Duplicate implementation of CLOCK_SetDiv() provided by SDK. This function
 * must be linked to ITCM, as it will be used to change the clocks of the
 * FLEXSPI and SEMC peripherals.
 * Any function called from this function must also reside in ITCM
 */
static void clock_set_div(clock_div_t divider, uint32_t value)
{
	uint32_t busy_shift;

	busy_shift = CCM_TUPLE_BUSY_SHIFT(divider);
	CCM_TUPLE_REG(CCM, divider) = (CCM_TUPLE_REG(CCM, divider) & (~CCM_TUPLE_MASK(divider))) |
		(((uint32_t)((value) << CCM_TUPLE_SHIFT(divider))) & CCM_TUPLE_MASK(divider));

	/* Clock switch need Handshake? */
	if (busy_shift != CCM_NO_BUSY_WAIT) {
		/* Wait until CCM internal handshake finish. */
		while ((CCM->CDHIPR & ((uint32_t)(1UL << busy_shift))) != 0UL) {
		}
	}
}

/*
 * Duplicate implementation of CLOCK_InitUsb1Pll() provided by SDK. This function
 * must be linked to ITCM, as it will be used to change the clocks of the
 * FLEXSPI and SEMC peripherals.
 * Any function called from this function must also reside in ITCM
 */
static void clock_init_usb1_pll(const clock_usb_pll_config_t *config)
{
	/* Bypass PLL first */
	CCM_ANALOG->PLL_USB1 = (CCM_ANALOG->PLL_USB1 & (~CCM_ANALOG_PLL_USB1_BYPASS_CLK_SRC_MASK)) |
		CCM_ANALOG_PLL_USB1_BYPASS_MASK | CCM_ANALOG_PLL_USB1_BYPASS_CLK_SRC(config->src);

	CCM_ANALOG->PLL_USB1 = (CCM_ANALOG->PLL_USB1 & (~CCM_ANALOG_PLL_USB1_DIV_SELECT_MASK)) |
		CCM_ANALOG_PLL_USB1_ENABLE_MASK | CCM_ANALOG_PLL_USB1_POWER_MASK |
		CCM_ANALOG_PLL_USB1_EN_USB_CLKS_MASK |
		CCM_ANALOG_PLL_USB1_DIV_SELECT(config->loopDivider);

	while ((CCM_ANALOG->PLL_USB1 & CCM_ANALOG_PLL_USB1_LOCK_MASK) == 0UL) {
		;
	}

	/* Disable Bypass */
	CCM_ANALOG->PLL_USB1 &= ~CCM_ANALOG_PLL_USB1_BYPASS_MASK;
}

static void flexspi_enter_critical(void)
{
#if DT_SAME_NODE(DT_NODELABEL(flexspi2), DT_PARENT(DT_CHOSEN(zephyr_flash)))
	/* Wait for flexspi to be inactive, and gate the clock */
	while (!((FLEXSPI2->STS0 & FLEXSPI_STS0_ARBIDLE_MASK) &&
			(FLEXSPI2->STS0 & FLEXSPI_STS0_SEQIDLE_MASK))) {
	}
	FLEXSPI2->MCR0 |= FLEXSPI_MCR0_MDIS_MASK;

	/* Disable clock gate of flexspi2. */
	CCM->CCGR7 &= (~CCM_CCGR7_CG1_MASK);
#endif

#if DT_SAME_NODE(DT_NODELABEL(flexspi), DT_PARENT(DT_CHOSEN(zephyr_flash)))
	/* Wait for flexspi to be inactive, and gate the clock */
	while (!((FLEXSPI->STS0 & FLEXSPI_STS0_ARBIDLE_MASK) &&
			(FLEXSPI->STS0 & FLEXSPI_STS0_SEQIDLE_MASK))) {
	}
	FLEXSPI->MCR0 |= FLEXSPI_MCR0_MDIS_MASK;

	/* Disable clock of flexspi. */
	CCM->CCGR6 &= (~CCM_CCGR6_CG5_MASK);
#endif
}

static void flexspi_exit_critical(void)
{
#if DT_SAME_NODE(DT_NODELABEL(flexspi2), DT_PARENT(DT_CHOSEN(zephyr_flash)))
	/* Enable clock gate of flexspi2. */
	CCM->CCGR7 |= (CCM_CCGR7_CG1_MASK);

	FLEXSPI2->MCR0 &= ~FLEXSPI_MCR0_MDIS_MASK;
	FLEXSPI2->MCR0 |= FLEXSPI_MCR0_SWRESET_MASK;
	while (FLEXSPI2->MCR0 & FLEXSPI_MCR0_SWRESET_MASK) {
	}
	while (!((FLEXSPI2->STS0 & FLEXSPI_STS0_ARBIDLE_MASK) &&
		(FLEXSPI2->STS0 & FLEXSPI_STS0_SEQIDLE_MASK))) {
	}
#elif DT_SAME_NODE(DT_NODELABEL(flexspi), DT_PARENT(DT_CHOSEN(zephyr_flash)))
	/* Enable clock of flexspi. */
	CCM->CCGR6 |= CCM_CCGR6_CG5_MASK;

	FLEXSPI->MCR0 &= ~FLEXSPI_MCR0_MDIS_MASK;
	FLEXSPI->MCR0 |= FLEXSPI_MCR0_SWRESET_MASK;
	while (FLEXSPI->MCR0 & FLEXSPI_MCR0_SWRESET_MASK) {
	}
	while (!((FLEXSPI->STS0 & FLEXSPI_STS0_ARBIDLE_MASK) &&
		(FLEXSPI->STS0 & FLEXSPI_STS0_SEQIDLE_MASK))) {
	}
#endif
	/* Invalidate I-cache after flexspi clock changed. */
	if (SCB_CCR_IC_Msk == (SCB_CCR_IC_Msk & SCB->CCR)) {
		SCB_InvalidateICache();
	}
}


void clock_full_power(void)
{
	/* Power up PLLS */
	/* Set arm PLL div to divide by 2*/
	clock_set_div(kCLOCK_ArmDiv, 1);
#ifdef CONFIG_INIT_ARM_PLL
	/* Reinit arm pll based on saved configuration */
	CLOCK_InitArmPll(&arm_pll_config);
#endif

#ifdef CONFIG_INIT_VIDEO_PLL
	/* Reinit video pll */
	CLOCK_InitVideoPll(&video_pll_config);
#endif

#ifdef CONFIG_INIT_ENET_PLL
	/* Reinit enet pll */
	CLOCK_InitEnetPll(&enet_pll_config);
#endif
	/* Init SYS PLL */
	CLOCK_InitSysPll(&sys_pll_config);

	/* Enable USB PLL PFD 1 2 3 */
	CLOCK_InitUsb1Pfd(kCLOCK_Pfd1, usb1_pll_pfd1_frac);
	CLOCK_InitUsb1Pfd(kCLOCK_Pfd2, usb1_pll_pfd2_frac);
	CLOCK_InitUsb1Pfd(kCLOCK_Pfd3, usb1_pll_pfd3_frac);

	/* Enable SYS PLL PFD0 1 2 3 */
	CLOCK_InitSysPfd(kCLOCK_Pfd0, sys_pll_pfd0_frac);
	CLOCK_InitSysPfd(kCLOCK_Pfd1, sys_pll_pfd1_frac);
	CLOCK_InitSysPfd(kCLOCK_Pfd2, sys_pll_pfd2_frac);
	CLOCK_InitSysPfd(kCLOCK_Pfd3, sys_pll_pfd3_frac);

	/* Switch to full speed clocks */
#if (defined(XIP_EXTERNAL_FLASH) && (XIP_EXTERNAL_FLASH == 1))
	flexspi_enter_critical();
#endif

	/* Set Flexspi divider before increasing frequency of PLL3 PDF0. */
#if DT_SAME_NODE(DT_NODELABEL(flexspi), DT_PARENT(DT_CHOSEN(zephyr_flash)))
	clock_set_div(kCLOCK_FlexspiDiv, flexspi_div);
	clock_set_mux(kCLOCK_FlexspiMux, 3);
#endif
#if DT_SAME_NODE(DT_NODELABEL(flexspi2), DT_PARENT(DT_CHOSEN(zephyr_flash)))
	clock_set_div(kCLOCK_Flexspi2Div, flexspi_div);
	clock_set_mux(kCLOCK_Flexspi2Mux, 1);
#endif
	/* Init USB1 PLL. This will disable the PLL3 bypass. */
	clock_init_usb1_pll(&usb1_pll_config);

	/* Switch SEMC clock to PLL2_PFD2 clock */
	clock_set_mux(kCLOCK_SemcMux, 1);

	/* CORE CLK to 600MHz, AHB, IPG to 150MHz, PERCLK to 75MHz */
	clock_set_div(kCLOCK_PerclkDiv, 1);
	clock_set_div(kCLOCK_IpgDiv, 3);
	clock_set_div(kCLOCK_AhbDiv, 0);
	/* PERCLK mux to IPG CLK */
	clock_set_mux(kCLOCK_PerclkMux, 0);
	/* MUX to ENET_500M (RT1010-1024) / ARM_PODF (RT1050-1064) */
	clock_set_mux(kCLOCK_PrePeriphMux, 3);
	/* PERIPH mux to periph clock 2 output */
	clock_set_mux(kCLOCK_PeriphMux, 0);

#if (defined(XIP_EXTERNAL_FLASH) && (XIP_EXTERNAL_FLASH == 1))
	flexspi_exit_critical();
#endif

}

void clock_low_power(void)
{
#if (defined(XIP_EXTERNAL_FLASH) && (XIP_EXTERNAL_FLASH == 1))
	flexspi_enter_critical();
#endif
	/* Switch to 24MHz core clock, so ARM PLL can power down */
	clock_set_div(kCLOCK_PeriphClk2Div, 0);
	/* Switch to OSC clock */
	clock_set_mux(kCLOCK_PeriphClk2Mux, 1);
	/* Switch peripheral mux to 24MHz source */
	clock_set_mux(kCLOCK_PeriphMux, 1);
	/* Set PLL3 to bypass mode, output 24M clock */
	CCM_ANALOG->PLL_USB1_SET = CCM_ANALOG_PLL_USB1_BYPASS_MASK;
	CCM_ANALOG->PLL_USB1_SET = CCM_ANALOG_PLL_USB1_ENABLE_MASK;
	CCM_ANALOG->PFD_480_CLR = CCM_ANALOG_PFD_480_PFD0_CLKGATE_MASK;
	/* Change flexspi to use PLL3 PFD0 with no divisor (24M flexspi clock) */
#if DT_SAME_NODE(DT_NODELABEL(flexspi), DT_PARENT(DT_CHOSEN(zephyr_flash)))
	clock_set_div(kCLOCK_FlexspiDiv, 0);
	/* FLEXSPI1 mux to PLL3 PFD0 BYPASS */
	clock_set_mux(kCLOCK_FlexspiMux, 3);
#endif
#if DT_SAME_NODE(DT_NODELABEL(flexspi2), DT_PARENT(DT_CHOSEN(zephyr_flash)))
	clock_set_div(kCLOCK_Flexspi2Div, 0);
	/* FLEXSPI2 mux to PLL3 PFD0 BYPASS */
	clock_set_mux(kCLOCK_Flexspi2Mux, 1);
#endif
	/* CORE CLK to 24MHz and AHB, IPG, PERCLK to 12MHz */
	clock_set_div(kCLOCK_PerclkDiv, 0);
	clock_set_div(kCLOCK_IpgDiv, 1);
	clock_set_div(kCLOCK_AhbDiv, 0);
	/* PERCLK mux to IPG CLK */
	clock_set_mux(kCLOCK_PerclkMux, 0);
	/* Switch SEMC clock to peripheral clock */
	clock_set_mux(kCLOCK_SemcMux, 0);
#if (defined(XIP_EXTERNAL_FLASH) && (XIP_EXTERNAL_FLASH == 1))
	flexspi_exit_critical();
#endif
	/* After switching clocks, it is safe to power down the PLLs */
#ifdef CONFIG_INIT_ARM_PLL
	/* Deinit ARM PLL */
	CLOCK_DeinitArmPll();
#endif
	/* Deinit SYS PLL */
	CLOCK_DeinitSysPll();

	/* Deinit SYS PLL PFD 0 1 2 3 */
	CLOCK_DeinitSysPfd(kCLOCK_Pfd0);
	CLOCK_DeinitSysPfd(kCLOCK_Pfd1);
	CLOCK_DeinitSysPfd(kCLOCK_Pfd2);
	CLOCK_DeinitSysPfd(kCLOCK_Pfd3);


	/* Deinit USB1 PLL PFD 1 2 3 */
	CLOCK_DeinitUsb1Pfd(kCLOCK_Pfd1);
	CLOCK_DeinitUsb1Pfd(kCLOCK_Pfd2);
	CLOCK_DeinitUsb1Pfd(kCLOCK_Pfd3);

	/* Deinit VIDEO PLL */
	CLOCK_DeinitVideoPll();

	/* Deinit ENET PLL */
	CLOCK_DeinitEnetPll();
}

void clock_lpm_init(void)
{
	uint32_t tmp_reg = 0;

	CLOCK_SetMode(kCLOCK_ModeRun);
	/* Enable RC OSC. It needs at least 4ms to be stable, so self tuning need to be enabled. */
	XTALOSC24M->LOWPWR_CTRL |= XTALOSC24M_LOWPWR_CTRL_RC_OSC_EN_MASK;
	/* Configure RC OSC */
	XTALOSC24M->OSC_CONFIG0 = XTALOSC24M_OSC_CONFIG0_RC_OSC_PROG_CUR(0x4) |
					XTALOSC24M_OSC_CONFIG0_SET_HYST_MINUS(0x2) |
					XTALOSC24M_OSC_CONFIG0_RC_OSC_PROG(0xA7) |
					XTALOSC24M_OSC_CONFIG0_START_MASK |
					XTALOSC24M_OSC_CONFIG0_ENABLE_MASK;
	XTALOSC24M->OSC_CONFIG1 = XTALOSC24M_OSC_CONFIG1_COUNT_RC_CUR(0x40) |
		XTALOSC24M_OSC_CONFIG1_COUNT_RC_TRG(0x2DC);
	/* Take some delay */
	SDK_DelayAtLeastUs(4000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
	/* Add some hysteresis */
	tmp_reg = XTALOSC24M->OSC_CONFIG0;
	tmp_reg &= ~(XTALOSC24M_OSC_CONFIG0_HYST_PLUS_MASK |
				XTALOSC24M_OSC_CONFIG0_HYST_MINUS_MASK);
	tmp_reg |= XTALOSC24M_OSC_CONFIG0_HYST_PLUS(3) | XTALOSC24M_OSC_CONFIG0_HYST_MINUS(3);
	XTALOSC24M->OSC_CONFIG0 = tmp_reg;
	/* Set COUNT_1M_TRG */
	tmp_reg = XTALOSC24M->OSC_CONFIG2;
	tmp_reg &= ~XTALOSC24M_OSC_CONFIG2_COUNT_1M_TRG_MASK;
	tmp_reg |= XTALOSC24M_OSC_CONFIG2_COUNT_1M_TRG(0x2d7);
	XTALOSC24M->OSC_CONFIG2 = tmp_reg;
	/* Hardware requires to read OSC_CONFIG0 or OSC_CONFIG1 to make OSC_CONFIG2 write work */
	tmp_reg = XTALOSC24M->OSC_CONFIG1;
	XTALOSC24M->OSC_CONFIG1 = tmp_reg;
}

static int imxrt_lpm_init(void)
{

	struct clock_callbacks callbacks;
	uint32_t usb1_pll_pfd0_frac;

	callbacks.clock_set_run = clock_full_power;
	callbacks.clock_set_low_power = clock_low_power;
	callbacks.clock_lpm_init = clock_lpm_init;

	/*
	 * Read the boot time configuration of all PLLs.
	 * This is required because not all PLLs preserve register state
	 * when powered down. Additionally, populating these configuration
	 * structures enables the rest of the code to use the fsl_clock HAL api.
	 */
#ifdef CONFIG_INIT_ARM_PLL
	/* Read configuration values for arm pll */
	arm_pll_config.src = ((CCM_ANALOG->PLL_ARM &
		CCM_ANALOG_PLL_ARM_BYPASS_CLK_SRC_MASK) >>
		CCM_ANALOG_PLL_ARM_BYPASS_CLK_SRC_SHIFT);
	arm_pll_config.loopDivider = ((CCM_ANALOG->PLL_ARM &
		CCM_ANALOG_PLL_ARM_DIV_SELECT_MASK) >>
		CCM_ANALOG_PLL_ARM_DIV_SELECT_SHIFT);
#endif
	/* Read configuration values for sys pll */
	sys_pll_config.src = ((CCM_ANALOG->PLL_SYS &
		CCM_ANALOG_PLL_SYS_BYPASS_CLK_SRC_MASK) >>
		CCM_ANALOG_PLL_SYS_BYPASS_CLK_SRC_SHIFT);
	sys_pll_config.loopDivider = ((CCM_ANALOG->PLL_SYS &
		CCM_ANALOG_PLL_SYS_DIV_SELECT_MASK) >>
		CCM_ANALOG_PLL_SYS_DIV_SELECT_SHIFT);
	sys_pll_config.numerator = CCM_ANALOG->PLL_SYS_NUM;
	sys_pll_config.denominator = CCM_ANALOG->PLL_SYS_DENOM;
	sys_pll_config.ss_step = ((CCM_ANALOG->PLL_SYS_SS &
			CCM_ANALOG_PLL_SYS_SS_STEP_MASK) >>
			CCM_ANALOG_PLL_SYS_SS_STEP_SHIFT);
	sys_pll_config.ss_enable = ((CCM_ANALOG->PLL_SYS_SS &
			CCM_ANALOG_PLL_SYS_SS_ENABLE_MASK) >>
			CCM_ANALOG_PLL_SYS_SS_ENABLE_SHIFT);
	sys_pll_config.ss_stop = ((CCM_ANALOG->PLL_SYS_SS &
			CCM_ANALOG_PLL_SYS_SS_STOP_MASK) >>
			CCM_ANALOG_PLL_SYS_SS_STOP_SHIFT);

	/* Read configuration values for usb1 pll */
	usb1_pll_config.src = ((CCM_ANALOG->PLL_USB1 &
		CCM_ANALOG_PLL_USB1_BYPASS_CLK_SRC_MASK) >>
		CCM_ANALOG_PLL_USB1_BYPASS_CLK_SRC_SHIFT);
	usb1_pll_config.loopDivider = ((CCM_ANALOG->PLL_USB1 &
		CCM_ANALOG_PLL_USB1_DIV_SELECT_MASK) >>
		CCM_ANALOG_PLL_USB1_DIV_SELECT_SHIFT);
#ifdef CONFIG_INIT_VIDEO_PLL
	/* Read configuration values for video pll */
	video_pll_config.src = ((CCM_ANALOG->PLL_VIDEO &
		CCM_ANALOG_PLL_VIDEO_BYPASS_CLK_SRC_MASK) >>
		CCM_ANALOG_PLL_VIDEO_BYPASS_CLK_SRC_SHIFT);
	video_pll_config.loopDivider = ((CCM_ANALOG->PLL_VIDEO &
		CCM_ANALOG_PLL_VIDEO_DIV_SELECT_MASK) >>
		CCM_ANALOG_PLL_VIDEO_DIV_SELECT_SHIFT);
	video_pll_config.numerator = CCM_ANALOG->PLL_VIDEO_NUM;
	video_pll_config.denominator = CCM_ANALOG->PLL_VIDEO_DENOM;
	switch ((CCM_ANALOG->PLL_VIDEO &
		CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT_MASK) >>
		CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT_SHIFT) {
		case 0:
			video_pll_config.postDivider = 16;
			break;
		case 1:
			if (CCM_ANALOG->MISC2 & CCM_ANALOG_MISC2_VIDEO_DIV(3)) {
				video_pll_config.postDivider = 8;
			} else {
				video_pll_config.postDivider = 2;
			}
			break;
		case 2:
			if (CCM_ANALOG->MISC2 & CCM_ANALOG_MISC2_VIDEO_DIV(3)) {
				video_pll_config.postDivider = 4;
			} else {
				video_pll_config.postDivider = 1;
			}
			break;
		default:
			video_pll_config.postDivider = 1;
	}
#endif
#if CONFIG_INIT_ENET_PLL
	enet_pll_config.src = ((CCM_ANALOG->PLL_ENET &
		CCM_ANALOG_PLL_ENET_BYPASS_CLK_SRC_MASK) >>
		CCM_ANALOG_PLL_ENET_BYPASS_CLK_SRC_SHIFT);
	enet_pll_config.loopDivider = ((CCM_ANALOG->PLL_ENET &
		CCM_ANALOG_PLL_ENET_DIV_SELECT_MASK) >>
		CCM_ANALOG_PLL_ENET_DIV_SELECT_SHIFT);
	enet_pll_config.loopDivider1 = ((CCM_ANALOG->PLL_ENET &
		CCM_ANALOG_PLL_ENET_ENET2_DIV_SELECT_MASK) >>
		CCM_ANALOG_PLL_ENET_ENET2_DIV_SELECT_SHIFT);
	enet_pll_config.enableClkOutput = (CCM_ANALOG->PLL_ENET &
		CCM_ANALOG_PLL_ENET_ENABLE_MASK);
	enet_pll_config.enableClkOutput1 = (CCM_ANALOG->PLL_ENET &
		CCM_ANALOG_PLL_ENET_ENET2_REF_EN_MASK);
	enet_pll_config.enableClkOutput25M = (CCM_ANALOG->PLL_ENET &
		CCM_ANALOG_PLL_ENET_ENET_25M_REF_EN_MASK);
#endif

	/* Record all pll PFD values that we intend to disable in low power mode */
	sys_pll_pfd0_frac = IMX_RT_SYS_PFD_FRAC(CCM_ANALOG->PFD_528, kCLOCK_Pfd0);
	sys_pll_pfd1_frac = IMX_RT_SYS_PFD_FRAC(CCM_ANALOG->PFD_528, kCLOCK_Pfd1);
	sys_pll_pfd2_frac = IMX_RT_SYS_PFD_FRAC(CCM_ANALOG->PFD_528, kCLOCK_Pfd2);
	sys_pll_pfd3_frac = IMX_RT_SYS_PFD_FRAC(CCM_ANALOG->PFD_528, kCLOCK_Pfd3);

	usb1_pll_pfd0_frac = IMX_RT_USB1_PFD_FRAC(CCM_ANALOG->PFD_480, kCLOCK_Pfd0);
	/* The target full power frequency for the flexspi clock is ~100MHz.
	 * Use the PFD0 value currently set to calculate the div we should use for
	 * the full power flexspi div
	 * PFD output frequency formula = (480 * 18) / pfd0_frac
	 * flexspi div formula = FLOOR((480*18) / (pfd0_frac * target_full_power_freq))
	 */
	flexspi_div = (480 * 18) / (usb1_pll_pfd0_frac * 100);


	usb1_pll_pfd1_frac = IMX_RT_USB1_PFD_FRAC(CCM_ANALOG->PFD_480, kCLOCK_Pfd1);
	usb1_pll_pfd2_frac = IMX_RT_USB1_PFD_FRAC(CCM_ANALOG->PFD_480, kCLOCK_Pfd2);
	usb1_pll_pfd3_frac = IMX_RT_USB1_PFD_FRAC(CCM_ANALOG->PFD_480, kCLOCK_Pfd3);

	/* Install LPM callbacks */
	imxrt_clock_pm_callbacks_register(&callbacks);
	return 0;
}


SYS_INIT(imxrt_lpm_init, PRE_KERNEL_1, 0);
