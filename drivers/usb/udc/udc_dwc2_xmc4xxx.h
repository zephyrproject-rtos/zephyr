/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_XMC4XXX_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_XMC4XXX_H

#include <zephyr/sys/sys_io.h>
#include <xmc_scu.h>
#include <usb_dwc2_hw.h>

/*
 * USB PLL divider settings for a 12 MHz OSC_HP, giving fUSBPLLVCO = 384 MHz
 * and fUSBPLL = 192 MHz.
 */
#define XMC4XXX_USB_PLL_PDIV 2U
#define XMC4XXX_USB_PLL_NDIV 64U

/* fUSBPLL (192 MHz) / 4 = fUSB (48 MHz) */
#define XMC4XXX_USB_CLK_DIV 4U

static inline int xmc4xxx_usb_clock_enable(const struct device *dev)
{
	ARG_UNUSED(dev);

	XMC_SCU_CLOCK_EnableUsbPll();
	XMC_SCU_CLOCK_StartUsbPll(XMC4XXX_USB_PLL_PDIV, XMC4XXX_USB_PLL_NDIV);

	XMC_SCU_CLOCK_SetUsbClockSource(XMC_SCU_CLOCK_USBCLKSRC_USBPLL);
	XMC_SCU_CLOCK_SetUsbClockDivider(XMC4XXX_USB_CLK_DIV);
	SCU_CLK->CLKSET = SCU_CLK_CLKSET_USBCEN_Msk;

	XMC_SCU_RESET_DeassertPeripheralReset(XMC_SCU_PERIPHERAL_RESET_USB0);
#if defined(CLOCK_GATING_SUPPORTED)
	XMC_SCU_CLOCK_UngatePeripheralClock(XMC_SCU_PERIPHERAL_CLOCK_USB0);
#endif
	/*
	 * Reference Manual, 17.18 Initialization and System Dependencies:
	 * "Remove the USB PHY from power-down by writing a 1 to the USBOTGEN
	 *  and USBPHYPDQ bits in SCU_PWRSET register."
	 */
	SCU_POWER->PWRSET = SCU_POWER_PWRSET_USBOTGEN_Msk | SCU_POWER_PWRSET_USBPHYPDQ_Msk;

	/*
	 * Reference Manual, 11.3.2 System States / Sleep State:
	 * "The state is entered via WFI or WFE instruction of the CPU. In this
	 *  state the clock to the CPU is stopped. The source of the system
	 *  clock may be altered. Peripherals clocks are gated according to the
	 *  SLEEPCR register."
	 *
	 * SLEEPCR.USBCR resets to 0 (disabled), so every idle entry gates the
	 * USB clock and every wake-up restores it, corrupting USB traffic.
	 * Keep the USB clock enabled through Sleep state.
	 */
	SCU_CLK->SLEEPCR |= SCU_CLK_SLEEPCR_USBCR_Msk;

	return 0;
}

static inline int xmc4xxx_usb_clock_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	SCU_CLK->SLEEPCR &= ~SCU_CLK_SLEEPCR_USBCR_Msk;

	XMC_SCU_RESET_AssertPeripheralReset(XMC_SCU_PERIPHERAL_RESET_USB0);
#if defined(CLOCK_GATING_SUPPORTED)
	XMC_SCU_CLOCK_GatePeripheralClock(XMC_SCU_PERIPHERAL_CLOCK_USB0);
#endif
	SCU_POWER->PWRCLR = SCU_POWER_PWRCLR_USBOTGEN_Msk | SCU_POWER_PWRCLR_USBPHYPDQ_Msk;

	SCU_CLK->CLKCLR = SCU_CLK_CLKCLR_USBCDI_Msk;
	XMC_SCU_CLOCK_DisableUsbPll();

	return 0;
}

#define QUIRK_XMC4XXX_USB_DEFINE(n)						\
	const struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {		\
		.pre_enable = xmc4xxx_usb_clock_enable,				\
		.disable = xmc4xxx_usb_clock_disable,				\
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_XMC4XXX_USB_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_XMC4XXX_H */
