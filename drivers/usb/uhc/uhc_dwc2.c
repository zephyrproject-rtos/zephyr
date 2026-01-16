/*
 * Copyright (c) 2026 Roman Leonov <jam_roma@yahoo.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_dwc2

#include "uhc_common.h"
#include "uhc_dwc2.h"

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usb_ch9.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_dwc2, CONFIG_UHC_DRIVER_LOG_LEVEL);

#define DEBOUNCE_DELAY_MS CONFIG_UHC_DWC2_DEBOUNCE_DELAY_MS
#define RESET_HOLD_MS     CONFIG_UHC_DWC2_RESET_HOLD_MS
#define RESET_RECOVERY_MS CONFIG_UHC_DWC2_RESET_RECOVERY_MS

enum uhc_dwc2_event {
	/* No event has occurred or the event is no longer valid */
	UHC_DWC2_EVENT_NONE,
	/* A device has been connected to the port */
	UHC_DWC2_EVENT_PORT_CONNECTION,
	/* A device is connected to the port but has not been reset. */
	UHC_DWC2_EVENT_PORT_DISABLED,
	/* A device has completed reset and enabled on the port */
	UHC_DWC2_EVENT_PORT_ENABLED,
	/* A device disconnection has been detected */
	UHC_DWC2_EVENT_PORT_DISCONNECTION,
	/* Port error detected */
	UHC_DWC2_EVENT_PORT_ERROR,
	/* Overcurrent detected */
	UHC_DWC2_EVENT_PORT_OVERCURRENT,
	/* Port has pending channel event */
	UHC_DWC2_EVENT_PORT_PEND_CHANNEL
};

enum uhc_dwc2_speed {
	UHC_DWC2_SPEED_HIGH = 0,
	UHC_DWC2_SPEED_FULL = 1,
	UHC_DWC2_SPEED_LOW = 2,
};

K_THREAD_STACK_DEFINE(uhc_dwc2_stack, CONFIG_UHC_DWC2_STACK_SIZE);

#define UHC_DWC2_CHANNEL_REGS(base, chan_idx)					\
	((struct usb_dwc2_host_chan *)(((mem_addr_t)(base)) + USB_DWC2_HCCHAR(chan_idx)))

/* Mask to clear HPRT register */
#define USB_DWC2_HPRT_W1C_MSK	(USB_DWC2_HPRT_PRTENA |				\
				USB_DWC2_HPRT_PRTCONNDET |			\
				USB_DWC2_HPRT_PRTENCHNG |			\
				USB_DWC2_HPRT_PRTOVRCURRCHNG)

/* Interrupts that pertain to core events */
#define CORE_EVENTS_INTRS_MSK (USB_DWC2_GINTSTS_DISCONNINT | 			\
				USB_DWC2_GINTSTS_HCHINT)

/* Interrupt that pertain to host port events */
#define PORT_EVENTS_INTRS_MSK (USB_DWC2_HPRT_PRTCONNDET | 			\
				USB_DWC2_HPRT_PRTENCHNG | 			\
				USB_DWC2_HPRT_PRTOVRCURRCHNG)

struct uhc_dwc2_data {
	struct k_sem irq_sem;
	struct k_thread thread;
	/* Event bitmask the driver thread waits for */
	struct k_event event;
	/* Bit mask of channels with pending interrupts */
	uint32_t pending_channels_msk;
	/* Root Port flags */
	uint8_t debouncing: 1;
	uint8_t has_device: 1;
};

/*
 * DWC2 low-level HAL Functions,
 *
 * Never use device structs or other driver config/data structs, but only the registers,
 * directly provided as arguments.
 */
static inline void dwc2_hal_toggle_reset(struct usb_dwc2_reg *const dwc2, const bool reset_on)
{
	uint32_t hprt;

	hprt = sys_read32((mem_addr_t)&dwc2->hprt);
	if (reset_on) {
		hprt |= USB_DWC2_HPRT_PRTRST;
	} else {
		hprt &= ~USB_DWC2_HPRT_PRTRST;
	}
	sys_write32(hprt & (~USB_DWC2_HPRT_W1C_MSK), (mem_addr_t)&dwc2->hprt);
}

static inline void dwc2_hal_set_power(struct usb_dwc2_reg *const dwc2, const bool power_on)
{
	uint32_t hprt;

	hprt = sys_read32((mem_addr_t)&dwc2->hprt);
	if (power_on) {
		hprt |= USB_DWC2_HPRT_PRTPWR;
	} else {
		hprt &= ~USB_DWC2_HPRT_PRTPWR;
	}
	sys_write32(hprt & (~USB_DWC2_HPRT_W1C_MSK), (mem_addr_t)&dwc2->hprt);
}

static inline enum uhc_dwc2_speed dwc2_hal_get_speed(struct usb_dwc2_reg *const dwc2)
{
	uint32_t hprt = sys_read32((mem_addr_t)&dwc2->hprt);
	uint8_t speed = usb_dwc2_get_hprt_prtspd(hprt);
	switch (speed) {
		case USB_DWC2_HPRT_PRTSPD_HIGH:
			return UHC_DWC2_SPEED_HIGH;
		case USB_DWC2_HPRT_PRTSPD_FULL:
			return UHC_DWC2_SPEED_FULL;
		case USB_DWC2_HPRT_PRTSPD_LOW:
			return UHC_DWC2_SPEED_LOW;
		default:
			LOG_ERR("Unexpected speed, assign Full-speed");
			break;
		
	}
	return UHC_DWC2_SPEED_FULL;
}

static inline void dwc2_hal_enable_port(struct usb_dwc2_reg *const dwc2, 
					const enum uhc_dwc2_speed speed)
{
	uint32_t hcfg = sys_read32((mem_addr_t)&dwc2->hcfg);
	uint32_t hfir = sys_read32((mem_addr_t)&dwc2->hfir);
	uint32_t ghwcfg2 = sys_read32((mem_addr_t)&dwc2->ghwcfg2);
	uint8_t fslspclksel;
	uint16_t frint;

	/* We can select Buffer DMA of Scatter-Gather DMA mode here: Buffer DMA for now */
	hcfg &= ~USB_DWC2_HCFG_DESCDMA;

	/* Disable periodic scheduling, will enable later */
	hcfg &= ~USB_DWC2_HCFG_PERSCHEDENA;

	if (usb_dwc2_get_ghwcfg2_hsphytype(ghwcfg2) == 0) {
		/* Indicate to the OTG core what speed the PHY clock is at
		 * Note: FSLS PHY has an implicit 8 divider applied when in LS mode,
		 * so the values of FSLSPclkSel and FrInt have to be adjusted accordingly.
		 */
		fslspclksel = (speed == UHC_DWC2_SPEED_FULL) ? 1 : 2;
		hcfg &= ~USB_DWC2_HCFG_FSLSPCLKSEL_MASK;
		hcfg |= (fslspclksel << USB_DWC2_HCFG_FSLSPCLKSEL_POS);

		/* Disable dynamic loading */
		hfir &= ~USB_DWC2_HFIR_HFIRRLDCTRL;
		/* Set frame interval to be equal to 1ms
		 * Note: FSLS PHY has an implicit 8 divider applied when in LS mode,
		 * so the values of FSLSPclkSel and FrInt have to be adjusted accordingly.
		 */
		frint = (speed == UHC_DWC2_SPEED_FULL) ? 48000 : 6000;
		hfir &= ~USB_DWC2_HFIR_FRINT_MASK;
		hfir |= (frint << USB_DWC2_HFIR_FRINT_POS);

		sys_write32(hcfg, (mem_addr_t)&dwc2->hcfg);
		sys_write32(hfir, (mem_addr_t)&dwc2->hfir);
	} else {
		LOG_ERR("Configuring clocks for HS PHY is not implemented");
	}
}

static inline void dwc2_hal_init_gusbcfg(struct usb_dwc2_reg *const dwc2)
{
	uint32_t gusbcfg = sys_read32((mem_addr_t)&dwc2->gusbcfg);
	uint32_t ghwcfg2 = sys_read32((mem_addr_t)&dwc2->ghwcfg2);
	uint32_t ghwcfg4 = sys_read32((mem_addr_t)&dwc2->ghwcfg4);
	
	/* Enable Host mode */
	sys_set_bits((mem_addr_t)&dwc2->gusbcfg, USB_DWC2_GUSBCFG_FORCEHSTMODE);
	/* Wait until core is in host mode */
	while ((sys_read32((mem_addr_t)&dwc2->gintsts) & USB_DWC2_GINTSTS_CURMOD) != 1) {
	}

	if (usb_dwc2_get_ghwcfg2_hsphytype(ghwcfg2) != 0) {
		/* De-select FS PHY */
		gusbcfg &= ~USB_DWC2_GUSBCFG_PHYSEL_USB11;

		if (usb_dwc2_get_ghwcfg2_hsphytype(ghwcfg2) == USB_DWC2_GHWCFG2_HSPHYTYPE_ULPI) {
			LOG_DBG("Highspeed ULPI PHY init");
			/* Select ULPI PHY (external) */
			gusbcfg |= USB_DWC2_GUSBCFG_ULPI_UTMI_SEL_ULPI;
			/* ULPI is always 8-bit interface */
			gusbcfg &= ~USB_DWC2_GUSBCFG_PHYIF_16_BIT;
			/* ULPI select single data rate */
			gusbcfg &= ~USB_DWC2_GUSBCFG_DDR_DOUBLE;
			/* Default internal VBUS Indicator and Drive */
			gusbcfg &= ~(USB_DWC2_GUSBCFG_ULPIEVBUSD | USB_DWC2_GUSBCFG_ULPIEVBUSI);
			/* Disable FS/LS ULPI and Supend mode */
			gusbcfg &= ~(USB_DWC2_GUSBCFG_ULPIFSLS | USB_DWC2_GUSBCFG_ULPICLK_SUSM);
		} else {
			LOG_DBG("Highspeed UTMI+ PHY init");
			/* Select UTMI+ PHY (internal) */
			gusbcfg &= ~USB_DWC2_GUSBCFG_ULPI_UTMI_SEL_ULPI;
			/* Set 16-bit interface if supported */
			if (usb_dwc2_get_ghwcfg4_phydatawidth(ghwcfg4)) {
				gusbcfg |= USB_DWC2_GUSBCFG_PHYIF_16_BIT;
			} else {
				gusbcfg &= ~USB_DWC2_GUSBCFG_PHYIF_16_BIT;
			}
		}
		sys_write32(gusbcfg, (mem_addr_t)&dwc2->gusbcfg);
	} else {
		LOG_DBG("Fullspeed PHY init");
		sys_set_bits((mem_addr_t)&dwc2->gusbcfg, USB_DWC2_GUSBCFG_PHYSEL_USB11);
	}
}

static inline int dwc2_hal_init_gahbcfg(struct usb_dwc2_reg *const dwc2)
{
	uint32_t ghwcfg2 = sys_read32((mem_addr_t)&dwc2->ghwcfg2);

	/* Disable Global Interrupt */
	sys_clear_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);

	/* TODO: Set AHB burst mode for some ECO only for ESP32S2 */
	/* Make config quirk? */

	/* TODO: Disable HNP and SRP capabilities */
	/* Also move to quirk? */

	/* Configure AHB */
	uint32_t gahbcfg = USB_DWC2_GAHBCFG_NPTXFEMPLVL | 
				usb_dwc2_set_gahbcfg_hbstlen(USB_DWC2_GAHBCFG_HBSTLEN_INCR16);
	sys_write32(gahbcfg, (mem_addr_t)&dwc2->gahbcfg);

	if (usb_dwc2_get_ghwcfg2_otgarch(ghwcfg2) == USB_DWC2_GHWCFG2_OTGARCH_INTERNALDMA) {
		sys_set_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_DMAEN);
	} else {
		/* TODO: Implement Simple mode */
		LOG_ERR("DMA isn't supported by the hardware");
		return -ENXIO;
	}

	/* Enable Global Interrupt */
	sys_set_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);
	return 0;
}

static int dwc2_hal_core_reset(struct usb_dwc2_reg *const dwc2, const k_timeout_t timeout)
{
	const k_timepoint_t timepoint = sys_timepoint_calc(timeout);

	/* Check AHB master idle state */
	while ((sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_AHBIDLE) == 0) {
		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for AHB idle timeout, GRSTCTL 0x%08x",
				sys_read32((mem_addr_t)&dwc2->grstctl));
			return -EIO;
		}

		k_busy_wait(1);
	}

	/* Apply Core Soft Reset */
	sys_write32(USB_DWC2_GRSTCTL_CSFTRST, (mem_addr_t)&dwc2->grstctl);

	/* Wait for reset to complete */
	while ((sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_CSFTRST) != 0 &&
	       (sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_CSFTRSTDONE) == 0) {
		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for CSR done timeout, GRSTCTL 0x%08x",
				sys_read32((mem_addr_t)&dwc2->grstctl));
			return -EIO;
		}

		k_busy_wait(1);
	}

	/* CSFTRSTDONE is W1C so the write must have the bit set to clear it */
	sys_clear_bits((mem_addr_t)&dwc2->grstctl, USB_DWC2_GRSTCTL_CSFTRST);

	LOG_DBG("DWC2 core reset done");

	return 0;
}

static int dwc2_hal_init_host(struct usb_dwc2_reg *const dwc2)
{
	int ret;

	/* Init GUSBCFG */
	dwc2_hal_init_gusbcfg(dwc2);

	/* Init GAHBCFG */
	ret = dwc2_hal_init_gahbcfg(dwc2);

	/* Clear interrupts */
	sys_clear_bits((mem_addr_t)&dwc2->gintmsk, 0xFFFFFFFFUL);
	sys_set_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_DISCONNINT);

	/* Clear status */
	uint32_t core_intrs = sys_read32((mem_addr_t)&dwc2->gintsts);
	sys_write32(core_intrs, (mem_addr_t)&dwc2->gintsts);

	return ret;
}

/*
 * DWC2 Port Management
 *
 * Operation of the USB port and handling if events related to it, and helpers to query information
 * about their speed, occupancy, status...
 */

static void uhc_dwc2_port_debounce_lock(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	/* Disable connection and disconnection interrupts to prevent spurious events */
	sys_clear_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_PRTINT |
						USB_DWC2_GINTSTS_DISCONNINT);
	/* Set the debounce lock flag */
	priv->debouncing = 1;
}

static void uhc_dwc2_port_debounce_unlock(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	/* Clear the flag */
	priv->debouncing = 0;
	/* Clear Connection and disconnection interrupt in case it triggered again */
	sys_set_bits((mem_addr_t)&dwc2->gintsts, USB_DWC2_GINTSTS_DISCONNINT);
	/* Clear the PRTCONNDET interrupt by writing 1 to the corresponding bit (W1C logic) */
	sys_set_bits((mem_addr_t)&dwc2->hprt, USB_DWC2_HPRT_PRTCONNDET);
	/* Re-enable the HPRT (connection) and disconnection interrupts */
	sys_set_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_PRTINT |
						USB_DWC2_GINTSTS_DISCONNINT);
}

static enum uhc_dwc2_event uhc_dwc2_port_get_event(const struct device *dev)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;

	uint32_t gintsts = 0;
	uint32_t hprt = 0;

	/* Read and clear core interrupt status */
	gintsts = sys_read32((mem_addr_t)&dwc2->gintsts);
	sys_write32(gintsts, (mem_addr_t)&dwc2->gintsts);

	/* If port interruprt - read and clear */
	if (gintsts & USB_DWC2_GINTSTS_PRTINT) {
		hprt = sys_read32((mem_addr_t)&dwc2->hprt);
		/* Clear the interrupt status by writing 1 to the W1C bits, except the PRTENA bit */
		sys_write32(hprt & (~USB_DWC2_HPRT_PRTENA), (mem_addr_t)&dwc2->hprt);
	}

	enum uhc_dwc2_event port_event = UHC_DWC2_EVENT_NONE;
	/*
	 * Events priority:
	 * CHANNEL < CONN < ENABLE/DISABLE < OVRCUR < DISCONN
	 */
	if ((gintsts & CORE_EVENTS_INTRS_MSK) || (hprt & PORT_EVENTS_INTRS_MSK)) {
		if (gintsts & USB_DWC2_GINTSTS_DISCONNINT) {
			/* Port disconnected */
			port_event = UHC_DWC2_EVENT_PORT_DISCONNECTION;
			uhc_dwc2_port_debounce_lock(dev);
		} else {
			/* Port still connected, check port event */
			if (hprt & USB_DWC2_HPRT_PRTOVRCURRCHNG) {
				/* TODO: Overcurrent or overcurrent clear? */
				port_event = UHC_DWC2_EVENT_PORT_OVERCURRENT;
			} else if (hprt & USB_DWC2_HPRT_PRTENCHNG) {
				if (hprt & USB_DWC2_HPRT_PRTENA) {
					/* Host port was enabled */
					port_event = UHC_DWC2_EVENT_PORT_ENABLED;
				} else {
					/* Host port has been disabled */
					port_event = UHC_DWC2_EVENT_PORT_DISABLED;
				}
			} else if (hprt & USB_DWC2_HPRT_PRTCONNDET && !priv->debouncing) {
				port_event = UHC_DWC2_EVENT_PORT_CONNECTION;
				uhc_dwc2_port_debounce_lock(dev);
			} else {
				/* Should never happened, as port event masked */
				__ASSERT(false,
					"Unknown port interrupt, GINTSTS=%08Xh, HPRT=%08Xh",		
					gintsts, 
					hprt);
			}
		}
	}

	if (port_event == UHC_DWC2_EVENT_NONE && (gintsts & USB_DWC2_GINTSTS_HCHINT)) {
		/* One or more channels have pending interrupts. Store the mask of those channels */
		priv->pending_channels_msk = sys_read32((mem_addr_t)&dwc2->haint);
		port_event = UHC_DWC2_EVENT_PORT_PEND_CHANNEL;
	}

	return port_event;
}

static inline bool uhc_dwc2_port_debounce(const struct device *dev, 
						enum uhc_dwc2_event event)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	bool ret = false; 

	/* TODO: k_msleep() inside uhc_lock_internal(), good/bad? */
	/* Wait debounce time */
	k_msleep(DEBOUNCE_DELAY_MS);

	/* Check the post-debounce state (i.e., whether it's actually connected/disconnected) */
	if ((sys_read32((mem_addr_t)&dwc2->hprt) & USB_DWC2_HPRT_PRTCONNSTS)) {
		if (event == UHC_DWC2_EVENT_PORT_CONNECTION) {
			ret = true;
		}
	} else {
		if (event == UHC_DWC2_EVENT_PORT_DISCONNECTION) {
			ret = true;
		}
	}
	/* Disable debounce lock */
	uhc_dwc2_port_debounce_unlock(dev);
	/* Return true when port passed the debouce, false - when jitter */
	return ret;
}

static inline void uhc_dwc2_port_reset(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	// struct uhc_dwc2_data *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	/*
	Proceed to resetting the bus:
	- Update the port's state variable
	- Hold the bus in the reset state for RESET_HOLD_MS.
	- Return the bus to the idle state for RESET_RECOVERY_MS
	During this reset the port state should be set to RESETTING and do not change.
	*/
	// priv->dynamic.port_state = UHC_DWC2_PORT_STATE_RESETTING;
	dwc2_hal_toggle_reset(dwc2, true);

	/* Hold the bus in the reset state */
	k_msleep(RESET_HOLD_MS);

	/* Return the bus to the idle state. Port enabled event should occur */
	dwc2_hal_toggle_reset(dwc2, false);

	/* Give the port time to recover */
	k_msleep(RESET_RECOVERY_MS);

	/* TODO: verify the port state after reset */

	/* Finish the port config for the appeared device */
	/* TODO: apply FIFO settings */
	/* TODO: set frame list for the ISOC/INTR xfer */
	/* TODO: enable periodic transfer */
}

static inline int uhc_dwc2_soft_reset(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	int ret;

	/* TODO: Implement port checks */
	/* Port should have no ongoing transfers */
	/* Port flags should be 0 */

	/* Disable Global IRQ */
	config->irq_disable_func(dev);

	/* Reset the core */
	ret = dwc2_hal_core_reset(dwc2, K_MSEC(10));
	if (ret) {
		LOG_ERR("Failed to perform soft reset: %d", ret);
		return ret;
	}

	/* Re-config after reset */
	dwc2_hal_init_gahbcfg(dwc2);

	/* Enable Global IRQ */
	config->irq_enable_func(dev);

	/* Power on the port */
	sys_clear_bits((mem_addr_t)&dwc2->haintmsk, 0xFFFFFFFFUL);
	sys_set_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_PRTINT | USB_DWC2_GINTSTS_HCHINT);
	dwc2_hal_set_power(dwc2, true);

	// ret = uhc_dwc2_power_on(dev);
	// if (ret) {
	// 	LOG_ERR("Failed to power on root port: %d", ret);
	// 	return ret;
	// }

	return ret;
}

/*
 * Interaction with upeer layer (USBH)
 *
 * Notifies the upper layer about the events
 */
static inline void uhc_dwc2_submit_new_device(const struct device *dev, enum uhc_dwc2_speed speed)
{
	static const char *const uhc_dwc2_speed_str[] = {"High", "Full", "Low"};
	enum uhc_event_type type;

	LOG_WRN("New device, %s-speed", uhc_dwc2_speed_str[speed]);

	switch (speed) {
		case UHC_DWC2_SPEED_LOW:
			type = UHC_EVT_DEV_CONNECTED_LS;
			break;
		case UHC_DWC2_SPEED_FULL:
			type = UHC_EVT_DEV_CONNECTED_FS;
			break;
		case UHC_DWC2_SPEED_HIGH:
			type = UHC_EVT_DEV_CONNECTED_HS;
			break;
		default:
			LOG_ERR("Unsupported speed %d", speed);
			return;
	}

	/* TODO: uhc submit event UHC_EVT_DEV_CONNECTED_XX */
}

static inline void uhc_dwc2_submit_dev_gone(const struct device *dev)
{
	LOG_WRN("Device removed");

	/* TODO: uhc submit event UHC_EVT_DEV_REMOVED */
}

static void uhc_dwc2_port_handle_events(const struct device *dev, uint32_t event_mask)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_CONNECTION)) {
		/* Device connected to the port, but hasn't been reset */
		/* We don't have the device speed yet, but the device already in the port. */
		LOG_DBG("Port connected");
		/* Debounce port connection */
		if (uhc_dwc2_port_debounce(dev, UHC_DWC2_EVENT_PORT_CONNECTION)) {
			/* Reset the port to get the device speed */
			uhc_dwc2_port_reset(dev);
		}
	}

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_ENABLED)) {
		/* Port has a device connected and finish the first reset, we know the speed */
		LOG_DBG("Port enabled");
		/* Get the device speed */
		enum uhc_dwc2_speed speed = dwc2_hal_get_speed(dwc2);
		/* Enable the rest of the port */
		dwc2_hal_enable_port(dwc2, speed);
		/* Notify the higher logic about the new device */
		uhc_dwc2_submit_new_device(dev, speed);
		
		priv->has_device = 1;
	}


	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_DISCONNECTION)) { 
		/* Port disconnected */
		LOG_DBG("Port disconnected");
		/* Debounce port disconnection */
		if (uhc_dwc2_port_debounce(dev, UHC_DWC2_EVENT_PORT_DISCONNECTION)) {
			if (priv->has_device) {
				uhc_dwc2_submit_dev_gone(dev);
			}
			/* Reset the controller to handle new connection */
			uhc_dwc2_soft_reset(dev);
		}
	}

	if ((event_mask & BIT(UHC_DWC2_EVENT_PORT_ERROR)) ||
	    (event_mask & BIT(UHC_DWC2_EVENT_PORT_ERROR))) {
		LOG_DBG("Port error");
			
		if (priv->has_device) {
			uhc_dwc2_submit_dev_gone(dev);
		}

		/* TODO: recover from the error */

		/* Reset the controller to handle new connection */
		uhc_dwc2_soft_reset(dev);
	}

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_OVERCURRENT)) {
		LOG_ERR("Port overcurrent");
		/* TODO: Handle overcurrent */
		LOG_WRN("Handle overcurrent is not implemented");
		/* Just power off the port via registers */
	}
}

static void uhc_dwc2_channel_handle_events(const struct device *dev, void *channel)
{
	LOG_WRN("Channel event handle has not been implemented");
}

/*
 * Interrupt handler (ISR)
 *
 * Handle the interrupts being dispatched into events, as well as some immediate handling of
 * events directly from the IRQ handler.
 */
static void uhc_dwc2_isr_handler(const struct device *dev)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	enum uhc_dwc2_event port_event = uhc_dwc2_port_get_event(dev);

	switch (port_event)
	{
	case UHC_DWC2_EVENT_PORT_PEND_CHANNEL: {
		/* This is the one event, we do not propagate to the thread. */
		/* Instead, handle the channels and keep the event in associated channel object. */

		/* TODO: Cycle through each pending channel */
		k_event_set(&priv->event, BIT(port_event));
		break;
	}
	case UHC_DWC2_EVENT_PORT_ERROR:
	case UHC_DWC2_EVENT_PORT_OVERCURRENT:
	case UHC_DWC2_EVENT_PORT_CONNECTION:
	case UHC_DWC2_EVENT_PORT_DISABLED:
	case UHC_DWC2_EVENT_PORT_DISCONNECTION:
	case UHC_DWC2_EVENT_PORT_ENABLED: {
		k_event_set(&priv->event, BIT(port_event));
		break;
	}
	default:
		// Clear events without thread notification
		break;
	}

	(void) uhc_dwc2_quirk_irq_clear(dev);
}

static ALWAYS_INLINE void uhc_dwc2_thread_handler(void *const arg)
{
	const struct device *const dev = (const struct device *)arg;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	uint32_t event_mask;

	while (true) {
		event_mask = k_event_wait_safe(&priv->event, UINT32_MAX, false, K_FOREVER);

		uhc_lock_internal(dev, K_FOREVER);

		uhc_dwc2_port_handle_events(dev, event_mask);

		if (event_mask & BIT(UHC_DWC2_EVENT_PORT_PEND_CHANNEL)) {
			uhc_dwc2_channel_handle_events(dev, NULL /* &priv->chan[i] */);
		}

		uhc_unlock_internal(dev);
	}
}

static int uhc_dwc2_lock(const struct device *const dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_lock(&data->mutex, K_FOREVER);
}

static int uhc_dwc2_unlock(const struct device *const dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_unlock(&data->mutex);
}

static int uhc_dwc2_sof_enable(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_bus_suspend(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_bus_reset(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_bus_resume(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_enqueue(const struct device *const dev, struct uhc_transfer *const xfer)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_dequeue(const struct device *const dev, struct uhc_transfer *const xfer)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_preinit(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct uhc_data *const data = dev->data;

	/* Initialize the private data structure */
	memset(priv, 0, sizeof(struct uhc_dwc2_data));
	k_mutex_init(&data->mutex);
	k_event_init(&priv->event);

	uhc_dwc2_quirk_caps(dev);

	config->make_thread(dev);

	return 0;
}

static int uhc_dwc2_init(const struct device *const dev)
{
	// struct uhc_dwc2_data *priv = uhc_get_private(dev);
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	uint32_t reg;
	int ret;

	/* 1. Power-up dwc2 hardware controller */

	ret = uhc_dwc2_quirk_init(dev);
	if (ret) {
		LOG_ERR("Quirk init failed %d", ret);
		return ret;
	}

	/* 2. Read hardware configuration registers */

	reg = sys_read32((mem_addr_t)&dwc2->gsnpsid);
	if (reg != config->gsnpsid) {
		LOG_ERR("Unexpected GSNPSID 0x%08x instead of 0x%08x", reg, config->gsnpsid);
		return -ENOTSUP;
	}

	reg = sys_read32((mem_addr_t)&dwc2->ghwcfg1);
	if (reg != config->ghwcfg1) {
		LOG_ERR("Unexpected GHWCFG1 0x%08x instead of 0x%08x", reg, config->ghwcfg1);
		return -ENOTSUP;
	}

	reg = sys_read32((mem_addr_t)&dwc2->ghwcfg2);
	if (reg != config->ghwcfg2) {
		LOG_ERR("Unexpected GHWCFG2 0x%08x instead of 0x%08x", reg, config->ghwcfg2);
		return -ENOTSUP;
	}

	reg = sys_read32((mem_addr_t)&dwc2->ghwcfg3);
	if (reg != config->ghwcfg3) {
		LOG_ERR("Unexpected GHWCFG3 0x%08x instead of 0x%08x", reg, config->ghwcfg3);
		return -ENOTSUP;
	}

	reg = sys_read32((mem_addr_t)&dwc2->ghwcfg4);
	if (reg != config->ghwcfg4) {
		LOG_ERR("Unexpected GHWCFG4 0x%08x instead of 0x%08x", reg, config->ghwcfg4);
		return -ENOTSUP;
	}

	/* 3. Select PHY */

	ret = uhc_dwc2_quirk_phy_pre_select(dev);
	if (ret) {
		LOG_ERR("Quirk PHY pre select failed %d", ret);
		return ret;
	}

	if (uhc_dwc2_quirk_is_phy_clk_off(dev)) {
		LOG_ERR("PHY clock is  turned off, cannot reset");
		return -EIO;
	}

	ret = dwc2_hal_core_reset(dwc2, K_MSEC(10));
	if (ret) {
		LOG_ERR("DWC2 core reset failed after PHY init: %d", ret);
		return ret;
	}

	ret = uhc_dwc2_quirk_phy_post_select(dev);
	if (ret) {
		LOG_ERR("Quirk PHY post select failed %d", ret);
		return ret;
	}

	/* 4. Init DWC2 controller as a host */

	ret = dwc2_hal_init_host(dwc2);
	if (ret) {
		return ret;
	}

	return 0;
}

static int uhc_dwc2_enable(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;

	int ret;

	ret = uhc_dwc2_quirk_pre_enable(dev);
	if (ret) {
		LOG_ERR("Quirk pre enable failed %d", ret);
		return ret;
	}

	/* 1. Flush root port config */

	/* TODO: Pre-calculate FIFO configuration */

	/* TODO: Flush channels */

	/* 2. Enable IRQ */
	config->irq_enable_func(dev);

	/* 3. Power up root port */
	sys_clear_bits((mem_addr_t)&dwc2->haintmsk, 0xFFFFFFFFUL);
	sys_set_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_PRTINT | USB_DWC2_GINTSTS_HCHINT);

	dwc2_hal_set_power(dwc2, true);
	// ret = uhc_dwc2_power_on(dev);
	// if (ret) {
	// 	LOG_ERR("Failed to power on port: %d", ret);
	// 	return ret;
	// }

	return 0;
}

static int uhc_dwc2_disable(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	/* 0. TODO: Check ongoing transfer? */

	/* 1. Disable IRQ */

	/* 2. Power down root port */

	return -ENOSYS;
}

static int uhc_dwc2_shutdown(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

/*
 * Device Definition and Initialization
 */
static const struct uhc_api uhc_dwc2_api = {
	/* Common */
	.lock = uhc_dwc2_lock,
	.unlock = uhc_dwc2_unlock,
	.init = uhc_dwc2_init,
	.enable = uhc_dwc2_enable,
	.disable = uhc_dwc2_disable,
	.shutdown = uhc_dwc2_shutdown,
	/* Bus related */
	.bus_reset = uhc_dwc2_bus_reset,
	.sof_enable = uhc_dwc2_sof_enable,
	.bus_suspend = uhc_dwc2_bus_suspend,
	.bus_resume = uhc_dwc2_bus_resume,
	/* EP related */
	.ep_enqueue = uhc_dwc2_enqueue,
	.ep_dequeue = uhc_dwc2_dequeue,
};

#define UHC_DWC2_DT_INST_REG_ADDR(n)						\
	COND_CODE_1(DT_NUM_REGS(DT_DRV_INST(n)),				\
	(DT_INST_REG_ADDR(n)),							\
	(DT_INST_REG_ADDR_BY_NAME(n, core)))

#if !defined(UHC_DWC2_IRQ_DT_INST_DEFINE)
#define UHC_DWC2_IRQ_FLAGS_TYPE0(n)	0
#define UHC_DWC2_IRQ_FLAGS_TYPE1(n)	DT_INST_IRQ(n, type)
#define DW_IRQ_FLAGS(n)								\
	_CONCAT(UHC_DWC2_IRQ_FLAGS_TYPE, DT_INST_IRQ_HAS_CELL(n, type))(n)

#define UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
	static void uhc_dwc2_irq_enable_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    uhc_dwc2_isr_handler,				\
			    DEVICE_DT_INST_GET(n),				\
			    DW_IRQ_FLAGS(n));					\
										\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static void uhc_dwc2_irq_disable_func_##n(const struct device *dev)	\
	{									\
		irq_disable(DT_INST_IRQN(n));					\
	}
#endif

#define UHC_DWC2_PINCTRL_DT_INST_DEFINE(n)					\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),			\
			(PINCTRL_DT_INST_DEFINE(n)), ())

#define UHC_DWC2_PINCTRL_DT_INST_DEV_CONFIG_GET(n)				\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),			\
			((void *)PINCTRL_DT_INST_DEV_CONFIG_GET(n)),		\
			(NULL))

/* Multi-instance device definition for DWC2 host controller */
#define UHC_DWC2_DEVICE_DEFINE(n)						\
	UHC_DWC2_PINCTRL_DT_INST_DEFINE(n);					\
										\
	K_THREAD_STACK_DEFINE(uhc_dwc2_stack_##n,				\
		CONFIG_UHC_DWC2_STACK_SIZE);					\
										\
	static void uhc_dwc2_thread_##n(void *dev, void *arg1, void *arg2)	\
	{									\
		while (true) {							\
			uhc_dwc2_thread_handler(dev);				\
		}								\
	}									\
										\
	static void uhc_dwc2_make_thread_##n(const struct device *dev)		\
	{									\
		struct uhc_dwc2_data *priv = uhc_get_private(dev);		\
										\
		k_thread_create(&priv->thread,					\
				uhc_dwc2_stack_##n,				\
				K_THREAD_STACK_SIZEOF(uhc_dwc2_stack_##n),	\
				uhc_dwc2_thread_##n,				\
				(void *)dev, NULL, NULL,			\
				K_PRIO_COOP(CONFIG_UHC_DWC2_THREAD_PRIORITY),	\
				K_ESSENTIAL,					\
				K_NO_WAIT);					\
		k_thread_name_set(&priv->thread, dev->name);			\
	}									\
										\
	UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
										\
	static struct uhc_dwc2_data uhc_dwc2_priv_##n = {			\
		.irq_sem = Z_SEM_INITIALIZER(uhc_dwc2_priv_##n.irq_sem, 0, 1),	\
	};									\
										\
	static struct uhc_data uhc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(uhc_data_##n.mutex),		\
		.priv = &uhc_dwc2_priv_##n,					\
	};									\
										\
	static const struct uhc_dwc2_config uhc_dwc2_config_##n = {		\
		.base = (struct usb_dwc2_reg *)UHC_DWC2_DT_INST_REG_ADDR(n),	\
		.quirks = UHC_DWC2_VENDOR_QUIRK_GET(n),				\
		.pcfg = UHC_DWC2_PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.make_thread = uhc_dwc2_make_thread_##n,			\
		.irq_enable_func = uhc_dwc2_irq_enable_func_##n,		\
		.irq_disable_func = uhc_dwc2_irq_disable_func_##n,		\
		.gsnpsid = DT_INST_PROP(n, gsnpsid),				\
		.ghwcfg1 = DT_INST_PROP(n, ghwcfg1),				\
		.ghwcfg2 = DT_INST_PROP(n, ghwcfg2),				\
		.ghwcfg3 = DT_INST_PROP(n, ghwcfg3),				\
		.ghwcfg4 = DT_INST_PROP(n, ghwcfg4),				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, uhc_dwc2_preinit, NULL,			\
		&uhc_data_##n, &uhc_dwc2_config_##n,				\
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
		&uhc_dwc2_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_DWC2_DEVICE_DEFINE)
