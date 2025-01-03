/*
 * Copyright 2024  NXP
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <fsl_clock.h>
#include <fsl_spc.h>
#include <soc.h>
#if CONFIG_USB_DC_NXP_EHCI
#include "usb_phy.h"
#include "usb.h"

/* USB PHY condfiguration */
#define BOARD_USB_PHY_D_CAL     (0x04U)
#define BOARD_USB_PHY_TXCAL45DP (0x07U)
#define BOARD_USB_PHY_TXCAL45DM (0x07U)

usb_phy_config_struct_t usbPhyConfig = {
	BOARD_USB_PHY_D_CAL, BOARD_USB_PHY_TXCAL45DP, BOARD_USB_PHY_TXCAL45DM,
};
#endif

/* Board xtal frequency in Hz */
#define BOARD_XTAL0_CLK_HZ                        24000000U
/* Core clock frequency: 150MHz */
#define CLOCK_INIT_CORE_CLOCK                     150000000U
/* System clock frequency. */
extern uint32_t SystemCoreClock;

/* Update Active mode voltage for OverDrive mode. */
void power_mode_od(void)
{
	/* Set the DCDC VDD regulator to 1.2 V voltage level */
	spc_active_mode_dcdc_option_t opt = {
		.DCDCVoltage       = kSPC_DCDC_OverdriveVoltage,
		.DCDCDriveStrength = kSPC_DCDC_NormalDriveStrength,
	};
	SPC_SetActiveModeDCDCRegulatorConfig(SPC0, &opt);

	/* Set the LDO_CORE VDD regulator to 1.2 V voltage level */
	spc_active_mode_core_ldo_option_t ldo_opt = {
		.CoreLDOVoltage       = kSPC_CoreLDO_OverDriveVoltage,
		.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength,
	};
	SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldo_opt);

	/* Specifies the 1.2V operating voltage for the SRAM's read/write timing margin */
	spc_sram_voltage_config_t cfg = {
		.operateVoltage       = kSPC_sramOperateAt1P2V,
		.requestVoltageUpdate = true,
	};
	SPC_SetSRAMOperateVoltage(SPC0, &cfg);
}

#if CONFIG_FLASH_MCUX_FLEXSPI_NOR || CONFIG_FLASH_MCUX_FLEXSPI_XIP
__ramfunc static void enable_cache64(void)
{
	/* Make sure the FlexSPI clock is enabled before configuring the FlexSPI cache. */
	SYSCON->AHBCLKCTRLSET[0] |= SYSCON_AHBCLKCTRL0_FLEXSPI_MASK;

	/* Set command to invalidate all ways and write GO bit to initiate command */
	CACHE64_CTRL0->CCR = CACHE64_CTRL_CCR_INVW1_MASK | CACHE64_CTRL_CCR_INVW0_MASK;
	CACHE64_CTRL0->CCR |= CACHE64_CTRL_CCR_GO_MASK;
	/* Wait until the command completes */
	while ((CACHE64_CTRL0->CCR & CACHE64_CTRL_CCR_GO_MASK) != 0U) {
	}
	/* Enable cache, enable write buffer */
	CACHE64_CTRL0->CCR = (CACHE64_CTRL_CCR_ENWRBUF_MASK | CACHE64_CTRL_CCR_ENCACHE_MASK);

	/* configure reg0, reg1 to cover the whole FlexSPI
	 * reg 0 covers the space where Zephyr resides in case of XIP from FlexSPI
	 * reg 1 covers the storage space in case of XIP from FlexSPI
	 */
	CACHE64_POLSEL0->REG0_TOP = 0x7FFC00;
	CACHE64_POLSEL0->REG1_TOP = 0x0;
	CACHE64_POLSEL0->POLSEL =
		(CACHE64_POLSEL_POLSEL_REG0_POLICY(1) | CACHE64_POLSEL_POLSEL_REG1_POLICY(0) |
		 CACHE64_POLSEL_POLSEL_REG2_POLICY(0));

	__ISB();
	__DSB();
}
#endif

static int frdm_mcxn947_init(void)
{
	power_mode_od();

	/* Enable SCG clock */
	CLOCK_EnableClock(kCLOCK_Scg);

	/* FRO OSC setup - begin, enable the FRO for safety switching */

	/* Switch to FRO 12M first to ensure we can change the clock setting */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/* Configure Flash wait-states to support 1.2V voltage level and 150000000Hz frequency */
	FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x3U));

	/* Enable FRO HF(48MHz) output */
	CLOCK_SetupFROHFClocking(48000000U);

#ifdef CONFIG_FLASH_MCUX_FLEXSPI_XIP
	/* Call function flexspi_clock_safe_config() to move FleXSPI clock to a stable
	 * clock source when updating the PLL if in XIP (execute code from FlexSPI memory
	 */
	flexspi_clock_safe_config();
#endif

	/* Set up PLL0 */
	const pll_setup_t pll0Setup = {
		.pllctrl = SCG_APLLCTRL_SOURCE(1U) | SCG_APLLCTRL_SELI(27U) |
			   SCG_APLLCTRL_SELP(13U),
		.pllndiv = SCG_APLLNDIV_NDIV(8U),
		.pllpdiv = SCG_APLLPDIV_PDIV(1U),
		.pllmdiv = SCG_APLLMDIV_MDIV(50U),
		.pllRate = 150000000U
	};
	/* Configure PLL0 to the desired values */
	CLOCK_SetPLL0Freq(&pll0Setup);
	/* PLL0 Monitor is disabled */
	CLOCK_SetPll0MonitorMode(kSCG_Pll0MonitorDisable);

	/* Switch MAIN_CLK to PLL0 */
	CLOCK_AttachClk(kPLL0_to_MAIN_CLK);

	/* Set AHBCLKDIV divider to value 1 */
	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U);

	CLOCK_SetupExtClocking(BOARD_XTAL0_CLK_HZ);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(sai0)) || DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(sai1))
	/* < Set up PLL1 */
	const pll_setup_t pll1_Setup = {
		.pllctrl = SCG_SPLLCTRL_SOURCE(1U) | SCG_SPLLCTRL_SELI(3U) |
				 SCG_SPLLCTRL_SELP(1U),
		.pllndiv = SCG_SPLLNDIV_NDIV(25U),
		.pllpdiv = SCG_SPLLPDIV_PDIV(10U),
		.pllmdiv = SCG_SPLLMDIV_MDIV(256U),
		.pllRate = 24576000U};

	/* Configure PLL1 to the desired values */
	CLOCK_SetPLL1Freq(&pll1_Setup);
	/* Set PLL1 CLK0 divider to value 1 */
	CLOCK_SetClkDiv(kCLOCK_DivPLL1Clk0, 1U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcomm1))
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom1Clk, 1u);
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcomm2))
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom2Clk, 1u);
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcomm4))
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom4Clk, 1u);
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM4);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcomm7))
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom7Clk, 1u);
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM7);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(os_timer))
	CLOCK_AttachClk(kCLK_1M_to_OSTIMER);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio0))
	CLOCK_EnableClock(kCLOCK_Gpio0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio1))
	CLOCK_EnableClock(kCLOCK_Gpio1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio2))
	CLOCK_EnableClock(kCLOCK_Gpio2);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio3))
	CLOCK_EnableClock(kCLOCK_Gpio3);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio4))
	CLOCK_EnableClock(kCLOCK_Gpio4);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dac0))
	SPC_EnableActiveModeAnalogModules(SPC0, kSPC_controlDac0);
	CLOCK_SetClkDiv(kCLOCK_DivDac0Clk, 1u);
	CLOCK_AttachClk(kFRO_HF_to_DAC0);

	CLOCK_EnableClock(kCLOCK_Dac0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dac1))
	SPC_EnableActiveModeAnalogModules(SPC0, kSPC_controlDac1);
	CLOCK_SetClkDiv(kCLOCK_DivDac1Clk, 1u);
	CLOCK_AttachClk(kFRO_HF_to_DAC1);

	CLOCK_EnableClock(kCLOCK_Dac1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(enet))
	CLOCK_AttachClk(kNONE_to_ENETRMII);
	CLOCK_EnableClock(kCLOCK_Enet);
	SYSCON0->PRESETCTRL2 = SYSCON_PRESETCTRL2_ENET_RST_MASK;
	SYSCON0->PRESETCTRL2 &= ~SYSCON_PRESETCTRL2_ENET_RST_MASK;
	/* rmii selection for this board */
	SYSCON->ENET_PHY_INTF_SEL = SYSCON_ENET_PHY_INTF_SEL_PHY_SEL(1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(wwdt0))
	CLOCK_SetClkDiv(kCLOCK_DivWdt0Clk, 1u);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ctimer0))
	CLOCK_SetClkDiv(kCLOCK_DivCtimer0Clk, 1U);
	CLOCK_AttachClk(kPLL0_to_CTIMER0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ctimer1))
	CLOCK_SetClkDiv(kCLOCK_DivCtimer1Clk, 1U);
	CLOCK_AttachClk(kPLL0_to_CTIMER1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ctimer2))
	CLOCK_SetClkDiv(kCLOCK_DivCtimer2Clk, 1U);
	CLOCK_AttachClk(kPLL0_to_CTIMER2);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ctimer3))
	CLOCK_SetClkDiv(kCLOCK_DivCtimer3Clk, 1U);
	CLOCK_AttachClk(kPLL0_to_CTIMER3);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ctimer4))
	CLOCK_SetClkDiv(kCLOCK_DivCtimer4Clk, 1U);
	CLOCK_AttachClk(kPLL0_to_CTIMER4);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan0))
	CLOCK_SetClkDiv(kCLOCK_DivFlexcan0Clk, 1U);
	CLOCK_AttachClk(kFRO_HF_to_FLEXCAN0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usdhc0))
	CLOCK_SetClkDiv(kCLOCK_DivUSdhcClk, 1u);
	CLOCK_AttachClk(kFRO_HF_to_USDHC);
#endif

#if CONFIG_FLASH_MCUX_FLEXSPI_NOR || CONFIG_FLASH_MCUX_FLEXSPI_XIP
	/* Setup the FlexSPI clock */
	flexspi_clock_set_freq(MCUX_FLEXSPI_CLK,
			       DT_PROP(DT_NODELABEL(w25q64jvssiq), spi_max_frequency));
	enable_cache64();
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(smartdma))
	CLOCK_EnableClock(kCLOCK_Smartdma);
	RESET_PeripheralReset(kSMART_DMA_RST_SHIFT_RSTn);
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(video_sdma))
	/* Drive CLKOUT from main clock, divided by 25 to yield 6MHz clock
	 * The camera will use this clock signal to generate
	 * PCLK, HSYNC, and VSYNC
	 */
	CLOCK_AttachClk(kMAIN_CLK_to_CLKOUT);
	CLOCK_SetClkDiv(kCLOCK_DivClkOut, 25U);
#endif
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(vref))
	CLOCK_EnableClock(kCLOCK_Vref);
	SPC_EnableActiveModeAnalogModules(SPC0, kSPC_controlVref);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpadc0))
	CLOCK_SetClkDiv(kCLOCK_DivAdc0Clk, 1U);
	CLOCK_AttachClk(kFRO_HF_to_ADC0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usb1)) && (CONFIG_USB_DC_NXP_EHCI || CONFIG_UDC_NXP_EHCI)
	SPC0->ACTIVE_VDELAY = 0x0500;
	/* Change the power DCDC to 1.8v (By default, DCDC is 1.8V), CORELDO to 1.1v (By default,
	 * CORELDO is 1.0V)
	 */
	SPC0->ACTIVE_CFG &= ~SPC_ACTIVE_CFG_CORELDO_VDD_DS_MASK;
	SPC0->ACTIVE_CFG |= SPC_ACTIVE_CFG_DCDC_VDD_LVL(0x3) | SPC_ACTIVE_CFG_CORELDO_VDD_LVL(0x3) |
			    SPC_ACTIVE_CFG_SYSLDO_VDD_DS_MASK | SPC_ACTIVE_CFG_DCDC_VDD_DS(0x2u);
	/* Wait until it is done */
	while (SPC0->SC & SPC_SC_BUSY_MASK) {
	};
	if (0u == (SCG0->LDOCSR & SCG_LDOCSR_LDOEN_MASK)) {
		SCG0->TRIM_LOCK = 0x5a5a0001U;
		SCG0->LDOCSR |= SCG_LDOCSR_LDOEN_MASK;
		/* wait LDO ready */
		while (0U == (SCG0->LDOCSR & SCG_LDOCSR_VOUT_OK_MASK)) {
		};
	}
	SYSCON->AHBCLKCTRLSET[2] |= SYSCON_AHBCLKCTRL2_USB_HS_MASK |
				    SYSCON_AHBCLKCTRL2_USB_HS_PHY_MASK;
	SCG0->SOSCCFG &= ~(SCG_SOSCCFG_RANGE_MASK | SCG_SOSCCFG_EREFS_MASK);
	/* xtal = 20 ~ 30MHz */
	SCG0->SOSCCFG = (1U << SCG_SOSCCFG_RANGE_SHIFT) | (1U << SCG_SOSCCFG_EREFS_SHIFT);
	SCG0->SOSCCSR |= SCG_SOSCCSR_SOSCEN_MASK;
	while (1) {
		if (SCG0->SOSCCSR & SCG_SOSCCSR_SOSCVLD_MASK) {
			break;
		}
	}
	SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_CLKIN_ENA_MASK |
			      SYSCON_CLOCK_CTRL_CLKIN_ENA_FM_USBH_LPT_MASK;
	CLOCK_EnableClock(kCLOCK_UsbHs);
	CLOCK_EnableClock(kCLOCK_UsbHsPhy);
	CLOCK_EnableUsbhsPhyPllClock(kCLOCK_Usbphy480M, BOARD_XTAL0_CLK_HZ);
	CLOCK_EnableUsbhsClock();
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usb1)) && CONFIG_USB_DC_NXP_EHCI
	USB_EhciPhyInit(kUSB_ControllerEhci0, BOARD_XTAL0_CLK_HZ, &usbPhyConfig);
#endif
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpcmp0))
	CLOCK_SetClkDiv(kCLOCK_DivCmp0FClk, 1U);
	CLOCK_AttachClk(kFRO12M_to_CMP0F);
	SPC_EnableActiveModeAnalogModules(SPC0, (kSPC_controlCmp0 | kSPC_controlCmp0Dac));
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lptmr0))

/*
 * Clock Select Decides what input source the lptmr will clock from
 *
 * 0 <- 12MHz FRO
 * 1 <- 16K FRO
 * 2 <- 32K OSC
 * 3 <- Output from the OSC_SYS
 */
#if DT_PROP(DT_NODELABEL(lptmr0), clk_source) == 0x0
	CLOCK_SetupClockCtrl(kCLOCK_FRO12MHZ_ENA);
#elif DT_PROP(DT_NODELABEL(lptmr0), clk_source) == 0x1
	CLOCK_SetupClk16KClocking(kCLOCK_Clk16KToVsys);
#elif DT_PROP(DT_NODELABEL(lptmr0), clk_source) == 0x2
	CLOCK_SetupOsc32KClocking(kCLOCK_Osc32kToVsys);
#elif DT_PROP(DT_NODELABEL(lptmr0), clk_source) == 0x3
	/* Value here should not exceed 25MHZ when using lptmr */
	CLOCK_SetupExtClocking(MHZ(24));
	CLOCK_SetupClockCtrl(kCLOCK_CLKIN_ENA_FM_USBH_LPT);
#endif /* DT_PROP(DT_NODELABEL(lptmr0), clk_source) */

#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lptmr0)) */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexio0))
	CLOCK_SetClkDiv(kCLOCK_DivFlexioClk, 1u);
	CLOCK_AttachClk(kPLL0_to_FLEXIO);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i3c1), okay)
	/* Enable 1MHz clock. */
	SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK;

	CLOCK_SetClkDiv(kCLOCK_DivI3c1FClk, DT_PROP(DT_NODELABEL(i3c1), clk_divider));
	CLOCK_SetClkDiv(kCLOCK_DivI3c1FClkS, DT_PROP(DT_NODELABEL(i3c1), clk_divider_slow));
	CLOCK_SetClkDiv(kCLOCK_DivI3c1FClkStc, DT_PROP(DT_NODELABEL(i3c1), clk_divider_tc));

	/* Attach PLL0 clock to I3C, 150MHz / 6 = 25MHz. */
	CLOCK_AttachClk(kPLL0_to_I3C1FCLK);
	CLOCK_AttachClk(kCLK_1M_to_I3C1FCLKS);
	CLOCK_AttachClk(kI3C1FCLK_to_I3C1FCLKSTC);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sc_timer), okay)
	/* attach FRO HF to SCT */
	CLOCK_SetClkDiv(kCLOCK_DivSctClk, 1u);
	CLOCK_AttachClk(kFRO_HF_to_SCT);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(sai0))
	CLOCK_SetClkDiv(kCLOCK_DivSai0Clk, 1u);
	CLOCK_AttachClk(kPLL1_CLK0_to_SAI0);
	CLOCK_EnableClock(kCLOCK_Sai0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(sai1))
	CLOCK_SetClkDiv(kCLOCK_DivSai1Clk, 1u);
	CLOCK_AttachClk(kPLL1_CLK0_to_SAI1);
	CLOCK_EnableClock(kCLOCK_Sai1);
#endif

	/* Set SystemCoreClock variable. */
	SystemCoreClock = CLOCK_INIT_CORE_CLOCK;

	return 0;
}

SYS_INIT(frdm_mcxn947_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
