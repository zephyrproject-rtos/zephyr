/*
 * Copyright (c) 2017-2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <linker/sections.h>
#include <linker/linker-defs.h>
#include <fsl_clock.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <fsl_flexspi_nor_boot.h>
#include <clock_config.h>
#if CONFIG_USB_DC_NXP_EHCI
#include "usb_phy.h"
#include "usb_dc_mcux.h"
#endif

#if CONFIG_USB_DC_NXP_EHCI
/* USB PHY condfiguration */
#define BOARD_USB_PHY_D_CAL (0x0CU)
#define BOARD_USB_PHY_TXCAL45DP (0x06U)
#define BOARD_USB_PHY_TXCAL45DM (0x06U)
#endif

#if CONFIG_USB_DC_NXP_EHCI
	usb_phy_config_struct_t usbPhyConfig = {
		BOARD_USB_PHY_D_CAL, BOARD_USB_PHY_TXCAL45DP, BOARD_USB_PHY_TXCAL45DM,
	};
#endif

#ifdef CONFIG_NXP_IMX_RT_BOOT_HEADER
const __imx_boot_data_section BOOT_DATA_T boot_data = {
	.start = CONFIG_FLASH_BASE_ADDRESS,
	.size = KB(CONFIG_FLASH_SIZE),
	.plugin = PLUGIN_FLAG,
	.placeholder = 0xFFFFFFFF,
};

const __imx_boot_ivt_section ivt image_vector_table = {
	.hdr = IVT_HEADER,
	.entry = (uint32_t) _vector_start,
	.reserved1 = IVT_RSVD,
#ifdef CONFIG_DEVICE_CONFIGURATION_DATA
	.dcd = (uint32_t) dcd_data,
#else
	.dcd = (uint32_t) NULL,
#endif
	.boot_data = (uint32_t) &boot_data,
	.self = (uint32_t) &image_vector_table,
	.csf = (uint32_t)CSF_ADDRESS,
	.reserved2 = IVT_RSVD,
};
#endif

/**
 * @brief Initialize the system clock
 */
static ALWAYS_INLINE void clock_post_init(void)
{
#if defined(CONFIG_HAS_ENET_PLL) && !defined(CONFIG_USE_ENET_PLL_AS_ARM_CLK)
#ifndef CONFIG_ETH_MCUX
	/* Turn off the ENET PLL as Ethernet is not used */
	CLOCK_DeinitEnetPll();
#endif
#endif

#ifdef CONFIG_HAS_VIDEO_PLL
#ifndef CONFIG_DISPLAY
	/* Turn off the Video PLL as Display is not used */
	CLOCK_DeinitVideoPll();
#endif
#endif

#if CONFIG_USB_DC_NXP_EHCI
	CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usb480M,
		DT_PROP_BY_PHANDLE(DT_INST(0, nxp_mcux_usbd), clocks, clock_frequency));
	CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M,
		DT_PROP_BY_PHANDLE(DT_INST(0, nxp_mcux_usbd), clocks, clock_frequency));
	USB_EhciPhyInit(kUSB_ControllerEhci0, CPU_XTAL_CLK_HZ, &usbPhyConfig);
#else /* CONFIG_USB_DC_NXP_EHCI */
	CLOCK_DisableUsbhs0PhyPllClock();
#endif /* CONFIG_USB_DC_NXP_EHCI */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc1), okay) && CONFIG_DISK_DRIVER_SDMMC
	CLOCK_EnableClock(kCLOCK_Usdhc1);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc2), okay) && CONFIG_DISK_DRIVER_SDMMC
	CLOCK_EnableClock(kCLOCK_Usdhc2);
#endif

#ifdef CONFIG_VIDEO_MCUX_CSI
	CLOCK_EnableClock(kCLOCK_Csi); /* Disable CSI clock gate */
#endif

	/* Keep the system clock running so SYSTICK can wake up the system from
	 * wfi.
	 */
	CLOCK_SetMode(kCLOCK_ModeRun);
}

#if (DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc1), okay) && CONFIG_DISK_DRIVER_SDMMC)

/* Usdhc driver needs to re-configure pinmux
 * Pinmux depends on board design.
 * From the perspective of Usdhc driver,
 * it can't access board specific function.
 * So SoC provides this for board to register
 * its usdhc pinmux and for usdhc to access
 * pinmux.
 */

static usdhc_pin_cfg_cb g_usdhc_pin_cfg_cb;

void imxrt_usdhc_pinmux_cb_register(usdhc_pin_cfg_cb cb)
{
	g_usdhc_pin_cfg_cb = cb;
}

void imxrt_usdhc_pinmux(uint16_t nusdhc, bool init,
	uint32_t speed, uint32_t strength)
{
	if (g_usdhc_pin_cfg_cb)
		g_usdhc_pin_cfg_cb(nusdhc, init,
			speed, strength);
}

/* Usdhc driver needs to reconfigure the dat3 line to a pullup in order to
 * detect an SD card on the bus. Expose a callback to do that here. The board
 * must register this callback in its init function.
 */
static usdhc_dat3_cfg_cb g_usdhc_dat3_cfg_cb;

void imxrt_usdhc_dat3_cb_register(usdhc_dat3_cfg_cb cb)
{
	g_usdhc_dat3_cfg_cb = cb;
}

void imxrt_usdhc_dat3_pull(bool pullup)
{
	if (g_usdhc_dat3_cfg_cb) {
		g_usdhc_dat3_cfg_cb(pullup);
	}
}

#endif

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */

static int imxrt_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	unsigned int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Watchdog disable */
	if ((WDOG1->WCR & WDOG_WCR_WDE_MASK) != 0) {
		WDOG1->WCR &= ~WDOG_WCR_WDE_MASK;
	}

	if ((WDOG2->WCR & WDOG_WCR_WDE_MASK) != 0) {
		WDOG2->WCR &= ~WDOG_WCR_WDE_MASK;
	}

	RTWDOG->CNT = 0xD928C520U; /* 0xD928C520U is the update key */
	RTWDOG->TOVAL = 0xFFFF;
	RTWDOG->CS = (uint32_t) ((RTWDOG->CS) & ~RTWDOG_CS_EN_MASK)
		| RTWDOG_CS_UPDATE_MASK;

	/* Disable Systick which might be enabled by bootrom */
	if ((SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) != 0) {
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	}

	SCB_EnableICache();
	if ((SCB->CCR & SCB_CCR_DC_Msk) == 0) {
		SCB_EnableDCache();
	}

	/* Initialize clocks with tool generated code */
	clock_init();
	/* Additional clock init after generated code */
	clock_post_init();

	/*
	 * install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);
	return 0;
}

SYS_INIT(imxrt_init, PRE_KERNEL_1, 0);
