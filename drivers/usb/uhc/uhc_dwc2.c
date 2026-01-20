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

#define DEBOUNCE_DELAY_MS CONFIG_UHC_DWC2_PORT_DEBOUNCE_DELAY_MS
#define RESET_HOLD_MS     CONFIG_UHC_DWC2_PORT_RESET_HOLD_MS
#define RESET_RECOVERY_MS CONFIG_UHC_DWC2_PORT_RESET_RECOVERY_MS
#define SET_ADDR_DELAY_MS CONFIG_UHC_DWC2_PORT_SET_ADDR_DELAY_MS

/* TODO: Configurable? */
#define UHC_DWC2_MAX_CHAN 16

enum uhc_dwc2_event {
	/* Root port event */
	UHC_DWC2_EVENT_PORT,
	/* Root channel event */
	UHC_DWC2_EVENT_CHANNEL,
};

enum uhc_dwc2_port_event {
	/* No event has occurred or the event is no longer valid */
	UHC_DWC2_PORT_EVENT_NONE,
	/* A device has been connected to the port */
	UHC_DWC2_PORT_EVENT_CONNECTION,
	/* A device is connected to the port but has not been reset. */
	UHC_DWC2_PORT_EVENT_DISABLED,
	/* A device has completed reset and enabled on the port */
	UHC_DWC2_PORT_EVENT_ENABLED,
	/* A device disconnection has been detected */
	UHC_DWC2_PORT_EVENT_DISCONNECTION,
	/* Port error detected */
	UHC_DWC2_PORT_EVENT_ERROR,
	/* Overcurrent detected */
	UHC_DWC2_PORT_EVENT_OVERCURRENT,
	/* Port has pending channel event */
	UHC_DWC2_PORT_EVENT_CHANNEL
};

enum uhc_dwc2_channel_event {
	UHC_DWC2_CHANNEL_EVENT_NONE = 0,
	UHC_DWC2_CHANNEL_EVENT_XFER_DONE,
	UHC_DWC2_CHANNEL_EVENT_ERROR,
	UHC_DWC2_CHANNEL_EVENT_HALTED,
};

enum uhc_dwc2_speed {
	UHC_DWC2_SPEED_HIGH = 0,
	UHC_DWC2_SPEED_FULL = 1,
	UHC_DWC2_SPEED_LOW = 2,
};

enum uhc_dwc2_xfer_type {
	UHC_DWC2_XFER_TYPE_CTRL = 0,
	UHC_DWC2_XFER_TYPE_ISOCHRONOUS = 1,
	UHC_DWC2_XFER_TYPE_BULK = 2,
	UHC_DWC2_XFER_TYPE_INTR = 3,
};

enum uhc_dwc2_channel_pid {
  UHC_DWC2_PID_DATA0 = 0,
  UHC_DWC2_PID_DATA2 = 1,
  UHC_DWC2_PID_DATA1 = 2,
  UHC_DWC2_PID_MDATA_SETUP = 3,
};

enum {
	EPSIZE_BULK_FS = 64,
	EPSIZE_BULK_HS = 512,

	EPSIZE_ISO_FS_MAX = 1023,
	EPSIZE_ISO_HS_MAX = 1024,
};

K_THREAD_STACK_DEFINE(uhc_dwc2_stack, CONFIG_UHC_DWC2_STACK_SIZE);

#define UHC_DWC2_CHAN_REG(base, chan_idx)                                                          \
	((struct usb_dwc2_host_chan *)(((mem_addr_t)(base)) + USB_DWC2_HCCHARN(chan_idx)))

#define UHC_DWC2_FIFODEPTH(config)                                                                 \
	((uint32_t)(((config)->ghwcfg3 & USB_DWC2_GHWCFG3_DFIFODEPTH_MASK) >>                      \
		    USB_DWC2_GHWCFG3_DFIFODEPTH_POS))

#define UHC_DWC2_NUMHSTCHNL(config)                                                                \
	((uint32_t)(((config)->ghwcfg2 & USB_DWC2_GHWCFG2_NUMHSTCHNL_MASK) >>                      \
		    USB_DWC2_GHWCFG2_NUMHSTCHNL_POS))

/* Mask to clear HPRT register */
#define USB_DWC2_HPRT_W1C_MSK	(USB_DWC2_HPRT_PRTENA |		\
								USB_DWC2_HPRT_PRTCONNDET |	\
								USB_DWC2_HPRT_PRTENCHNG |	\
	 							USB_DWC2_HPRT_PRTOVRCURRCHNG)


/* Interrupts that pertain to core events */
#define CORE_EVENTS_INTRS_MSK (USB_DWC2_GINTSTS_DISCONNINT | \
							   USB_DWC2_GINTSTS_HCHINT)

/* Interrupt that pertain to host port events */
#define PORT_EVENTS_INTRS_MSK (USB_DWC2_HPRT_PRTCONNDET | \
							   USB_DWC2_HPRT_PRTENCHNG | \
							   USB_DWC2_HPRT_PRTOVRCURRCHNG)

struct uhc_dwc2_ep_conf {
	enum uhc_dwc2_xfer_type type;
	uint8_t ep_addr;
	uint8_t mps;
	uint8_t dev_addr;
	unsigned int interval;
	uint32_t offset;
	/* Flags */
	uint8_t ls_via_fs_hub: 1;
	uint8_t is_hs: 1;
};

struct uhc_dwc2_channel {
	/* Cached channel regs */
	const struct usb_dwc2_host_chan *regs;
	/* Pointer to the transfer */
	struct uhc_transfer *xfer;
	/* The index of the channel */
	uint8_t index;
	/* Stage index */
	enum uhc_control_stage ctrl_stg;
	/* Last channel event */
	enum uhc_dwc2_channel_event last_event;
	
	/* Associated endpoint characteristics */
	/* Type of endpoint */
	enum uhc_dwc2_xfer_type type;
	/* Address */
	uint8_t ep_addr;
	/* Device Address */
	uint8_t dev_addr;
	/* Maximum Packet Size */
	uint16_t ep_mps;

	/* Channel flags */
	/* Channel is claimed and has associated endpoint */
	uint8_t claimed: 1;
	/* Transfer stage is IN */
	uint8_t data_stg_in: 1;
	/* Transfer has no data stage */
	uint8_t data_stg_skip: 1;
	/* Transfer will change the device address */
	uint8_t set_address: 1;
	/* Channel executing transfer */
	uint8_t executing: 1;
};
struct uhc_dwc2_data {
	struct k_sem irq_sem;
	struct k_thread thread;
	/* Event the driver thread waits for */
	struct k_event event;
	/* Port channels */
	struct uhc_dwc2_channel channel[UHC_DWC2_MAX_CHAN];
	/* Port last event */
	enum uhc_dwc2_port_event last_event;
	/* Port FIFO configuration */
	uint16_t fifo_top;
	uint16_t fifo_nptxfsiz;
	uint16_t fifo_rxfsiz;
	uint16_t fifo_ptxfsiz;
	/* Bit mask of channels with pending interrupts */
	uint32_t pending_channels_msk;
	/* Root Port flags */
	uint8_t debouncing: 1;
	uint8_t has_device: 1;
};

static inline uint16_t calc_packet_count(const uint16_t size, const uint8_t mps)
{
	if (size == 0) {
		return 1; /* in Buffer DMA mode Zero Length Packet still counts as 1 packet */
	} else {
		return DIV_ROUND_UP(size, mps);
	}
}

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
	uint32_t hprt;

	hprt = sys_read32((mem_addr_t)&dwc2->hprt);
	return (hprt & USB_DWC2_HPRT_PRTSPD_MASK) >> USB_DWC2_HPRT_PRTSPD_POS;
}

static void dwc2_hal_flush_rx_fifo(struct usb_dwc2_reg *const dwc2)
{
	sys_write32(USB_DWC2_GRSTCTL_RXFFLSH, (mem_addr_t)&dwc2->grstctl);

	while (sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_RXFFLSH) {
		continue;
	}
}

static void dwc2_hal_flush_tx_fifo(struct usb_dwc2_reg *const dwc2, const uint8_t fnum)
{
	uint32_t grstctl;

	grstctl = usb_dwc2_set_grstctl_txfnum(fnum) | USB_DWC2_GRSTCTL_TXFFLSH;
	sys_write32(grstctl, (mem_addr_t)&dwc2->grstctl);

	while (sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_TXFFLSH) {
		continue;
	}
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
	uint32_t gahbcfg = sys_read32((mem_addr_t)&dwc2->gahbcfg);
	gahbcfg |= USB_DWC2_GAHBCFG_NPTXFEMPLVL;
	gahbcfg &= ~USB_DWC2_GAHBCFG_HBSTLEN_MASK;
	gahbcfg |= (USB_DWC2_GAHBCFG_HBSTLEN_INCR16 << USB_DWC2_GAHBCFG_HBSTLEN_POS);
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

static void dwc2_hal_config_fifo(struct usb_dwc2_reg *const dwc2, uint16_t top, 
								 uint16_t nptxfsiz, uint8_t ptxfsiz, uint16_t rxfsiz)
{
	uint16_t fifo_available = top;

	sys_write32(fifo_available << USB_DWC2_GDFIFOCFG_EPINFOBASEADDR_POS | fifo_available,
		    (mem_addr_t)&dwc2->gdfifocfg);

	fifo_available -= rxfsiz;

	sys_write32(rxfsiz << USB_DWC2_GRXFSIZ_RXFDEP_POS, (mem_addr_t)&dwc2->grxfsiz);

	fifo_available -= nptxfsiz;

	sys_write32(nptxfsiz << USB_DWC2_GNPTXFSIZ_NPTXFDEP_POS | fifo_available,
		    (mem_addr_t)&dwc2->gnptxfsiz);

	fifo_available -= ptxfsiz;

	sys_write32(ptxfsiz << USB_DWC2_HPTXFSIZ_PTXFSIZE_POS | fifo_available,
		    (mem_addr_t)&dwc2->hptxfsiz);

	dwc2_hal_flush_tx_fifo(dwc2, 0x10UL);
	dwc2_hal_flush_rx_fifo(dwc2);

	LOG_DBG("FIFO configured: nptx=%u, rx=%u, ptx=%u, available=%u",
		nptxfsiz * 4, rxfsiz * 4, ptxfsiz * 4, fifo_available * 4);
}

enum uhc_dwc2_channel_event dwc2_hal_get_channel_event(const struct usb_dwc2_host_chan *chan_regs)
{
	uint32_t hcint = sys_read32((mem_addr_t)&chan_regs->hcint);
	/* Clear the interrupt bits by writing them back */
	sys_write32(hcint, (mem_addr_t)&chan_regs->hcint);

	enum uhc_dwc2_channel_event event;

	/*
	 * Note:
	 * Do not change order of checks as some events take precedence over others.
	 * Errors > Channel Halt Request > Transfer completed
	*/
	if (hcint & (USB_DWC2_HCINT_STALL | USB_DWC2_HCINT_BBLERR | USB_DWC2_HCINT_XACTERR)) {
		__ASSERT(hcint & USB_DWC2_HCINT_CHHLTD,
			 "uhc_dwc2_hal_chan_decode_intr: Channel error without channel halted interrupt");

		LOG_ERR("Error on channel: HCINT=0x%08x", hcint);
		event = UHC_DWC2_CHANNEL_EVENT_ERROR;
	} else if (hcint & USB_DWC2_HCINT_CHHLTD) {
		// if (chan_obj->flags.halt_requested) {
			// chan_obj->flags.halt_requested = 0;
			// chan_event = DWC2_CHAN_EVENT_HALT_REQ;
		// } else {
			event = UHC_DWC2_CHANNEL_EVENT_XFER_DONE;
		// }
		// chan_obj->flags.active = 0;
	} else {
		__ASSERT(false,
				"Unknown channel interrupt, HCINT=%08Xh", hcint);
		event = UHC_DWC2_CHANNEL_EVENT_NONE;
	}
	return event;
}
/* ---------------------------- */

static void uhc_dwc2_port_debounce_lock(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	/* Disable Connection and disconnection interrupts to prevent spurious events */
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

static inline bool uhc_dwc2_channel_xfer_is_done(struct uhc_dwc2_channel *channel)
{
	/* Only control transfers need to be handled in stages */
	if (channel->type != UHC_DWC2_XFER_TYPE_CTRL) {
		return true;
	}

	/* Is done when we finished the UHC_CONTROL_STAGE_STATUS */
	return (channel->ctrl_stg == UHC_CONTROL_STAGE_STATUS);
}

static void IRAM_ATTR uhc_dwc2_channel_ctrl_procees_next_stage(struct uhc_dwc2_channel *channel)
{
	struct uhc_transfer *const xfer = channel->xfer;
	bool next_dir_is_in;
	enum uhc_dwc2_channel_pid next_pid;
	uint16_t size = 0;
	uint8_t *dma_addr = NULL;

	if (channel->ctrl_stg == UHC_CONTROL_STAGE_SETUP) {
		/* Just finished UHC_CONTROL_STAGE_SETUP */
		if (channel->data_stg_skip) {
			/* No data stage. Go strait to status */
			next_dir_is_in = true;		
			next_pid = UHC_DWC2_PID_DATA1;
			channel->ctrl_stg = UHC_CONTROL_STAGE_STATUS;
		} else {
			/* Data stage is present, go to data stage */
			next_dir_is_in = channel->data_stg_in;
			next_pid = UHC_DWC2_PID_DATA1;	
			channel->ctrl_stg = UHC_CONTROL_STAGE_DATA;

			/* NOTE:
			* For OUT - number of bytes host sends to device
			* For IN - number of bytes host reserves to receive
			*/
			size = xfer->buf->size;

			/* TODO: Toggle PID? */

			/* TODO: Check if the buffer is large enough for the next transfer? */

			/* TODO: Check that the buffer is DMA and CACHE aligned and compatible with the DMA */
			/* (better to do this on enqueue) */

			if (xfer->buf != NULL) {
				/* Get the tail of the buffer to append data */
				dma_addr = net_buf_tail(xfer->buf);
				/* TODO: Ensure the buffer has enough space? */
				net_buf_add(xfer->buf, size);
			}
		}
	} else {        
		/* Just finished UHC_CONTROL_STAGE_DATA */
		/* Status stage is always the opposite direction of data stage */
		next_dir_is_in = !channel->data_stg_in;
		next_pid = UHC_DWC2_PID_DATA1;	/* Status stage always has a PID of DATA1 */
		channel->ctrl_stg = UHC_CONTROL_STAGE_STATUS;
	}

	/* Calculate new packet count */
	const uint16_t pkt_cnt = calc_packet_count(size, channel->ep_mps);


	if (next_dir_is_in) {
		sys_set_bits((mem_addr_t)&channel->regs->hcchar, USB_DWC2_HCCHAR_EPDIR);
	} else {
		sys_clear_bits((mem_addr_t)&channel->regs->hcchar, USB_DWC2_HCCHAR_EPDIR);
	}

	uint32_t hctsiz = ((next_pid << USB_DWC2_HCTSIZ_PID_POS) & USB_DWC2_HCTSIZ_PID_MASK) |
					((pkt_cnt << USB_DWC2_HCTSIZ_PKTCNT_POS) & USB_DWC2_HCTSIZ_PKTCNT_MASK) |
					((size << USB_DWC2_HCTSIZ_XFERSIZE_POS) & USB_DWC2_HCTSIZ_XFERSIZE_MASK);

	sys_write32(hctsiz, (mem_addr_t)&channel->regs->hctsiz);
	sys_write32((uint32_t)dma_addr, (mem_addr_t)&channel->regs->hcdma);

	/* TODO: Configure split transaction if needed */

	/* TODO: sync CACHE */

	uint32_t hcchar = sys_read32((mem_addr_t)&channel->regs->hcchar);

	// LOG_WRN("%d: %08Xh, %08Xh", channel->cur_stg, hctsiz, hcchar);

	hcchar |= USB_DWC2_HCCHAR_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&channel->regs->hcchar);
}

static enum uhc_dwc2_channel_event uhc_dwc2_get_channel_event(struct uhc_dwc2_channel *channel)
{
	enum uhc_dwc2_channel_event event = dwc2_hal_get_channel_event(channel->regs);

	switch (event) {
	case UHC_DWC2_CHANNEL_EVENT_NONE: {
		/* No event, nothing to do */
		break;
	}
	case UHC_DWC2_CHANNEL_EVENT_XFER_DONE: {
		if (!uhc_dwc2_channel_xfer_is_done(channel)) {
			/* Control transfer isn't finished yet */
			uhc_dwc2_channel_ctrl_procees_next_stage(channel);
			event = UHC_DWC2_CHANNEL_EVENT_NONE; 
			break;
		}
		/* Transfer finished, pass the event higher */
		break;
	}
	case UHC_DWC2_CHANNEL_EVENT_ERROR: {
		LOG_ERR("Channel error handling not implemented yet");
		/* TODO: get channel error, halt the pipe */
		break;
	}
	case UHC_DWC2_CHANNEL_EVENT_HALTED: {
		LOG_ERR("Channel halt request handling not implemented yet");
		/* TODO: Implement halting the ongoing transfer */
		break;
	}
	default:
		/* Should never happen */
		LOG_WRN("Unknown channel event %d", event);
		break;
	}

	return event;
}

static enum uhc_dwc2_port_event uhc_dwc2_port_get_event(const struct device *dev)
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

	LOG_DBG("GINTSTS=%08Xh, HPRT=%08Xh", gintsts, hprt);

	enum uhc_dwc2_port_event port_event = UHC_DWC2_PORT_EVENT_NONE;
	/*
	 * Events priority:
	 * CONN < ENABLE/DISABLE < OVRCUR < DISCONN
	 */
	if ((gintsts & CORE_EVENTS_INTRS_MSK) || (hprt & PORT_EVENTS_INTRS_MSK)) {
		if (gintsts & USB_DWC2_GINTSTS_DISCONNINT) {
			/* Port disconnected */
			port_event = UHC_DWC2_PORT_EVENT_DISCONNECTION;
			uhc_dwc2_port_debounce_lock(dev);
		} else {
			/* Port still connected, check port event */
			if (hprt & USB_DWC2_HPRT_PRTOVRCURRCHNG) {
				/* TODO: Overcurrent or overcurrent clear? */
				port_event = UHC_DWC2_PORT_EVENT_OVERCURRENT;
			} else if (hprt & USB_DWC2_HPRT_PRTENCHNG) {
				if (hprt & USB_DWC2_HPRT_PRTENA) {
					/* Host port was enabled */
					port_event = UHC_DWC2_PORT_EVENT_ENABLED;
				} else {
					/* Host port has been disabled */
					port_event = UHC_DWC2_PORT_EVENT_DISABLED;
				}
			} else if (hprt & USB_DWC2_HPRT_PRTCONNDET && !priv->debouncing) {
				port_event = UHC_DWC2_PORT_EVENT_CONNECTION;
				uhc_dwc2_port_debounce_lock(dev);
			} else {
				/* Should never happened, as port event masked with PORT_EVENTS_INTRS_MSK */
				__ASSERT(false,
					 "uhc_dwc2_decode_intr: Unknown port interrupt, GINTSTS=%08Xh, HPRT=%08Xh",
					 						gintsts, hprt);
			}
		}
	}

	if (port_event == UHC_DWC2_PORT_EVENT_NONE && (gintsts & USB_DWC2_GINTSTS_HCHINT)) {
		/* One or more channels have pending interrupts. Store the mask of those channels */
		priv->pending_channels_msk = sys_read32((mem_addr_t)&dwc2->haint);
		port_event = UHC_DWC2_PORT_EVENT_CHANNEL;
	}

	return port_event;
}

static inline enum uhc_dwc2_port_event uhc_dwc2_port_debounce(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	enum uhc_dwc2_port_event ret; 

	/* TODO: k_msleep() inside uhc_lock_internal(), good/bad? */
	/* Wait debounce time */
	k_msleep(DEBOUNCE_DELAY_MS);

	/* Check the post-debounce state (i.e., whether it's actually connected/disconnected) */
	if ((sys_read32((mem_addr_t)&dwc2->hprt) & USB_DWC2_HPRT_PRTCONNSTS)) {
		ret = UHC_DWC2_PORT_EVENT_CONNECTION;
	} else {
		ret = UHC_DWC2_PORT_EVENT_DISCONNECTION;
	}
	/* Disable debounce lock */
	uhc_dwc2_port_debounce_unlock(dev);
	return ret;
}

static enum uhc_dwc2_port_event uhc_dwc2_port_debounce_event(const struct device *dev)
{
	struct uhc_dwc2_data *priv = uhc_get_private(dev);

	enum uhc_dwc2_port_event event = priv->last_event;

	switch (event) {
		case UHC_DWC2_PORT_EVENT_CONNECTION: 
		case UHC_DWC2_PORT_EVENT_DISCONNECTION: {
			/* Don't update state immediately, we need to debounce. */
			event = uhc_dwc2_port_debounce(dev); 
			break;
		}
		default: {
			/* Do not need to debounce */
			break;
		}
	}

	return event;
}

static inline void uhc_dwc2_port_reset(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *priv = uhc_get_private(dev);
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
	dwc2_hal_config_fifo(dwc2, priv->fifo_top, priv->fifo_nptxfsiz,
								priv->fifo_ptxfsiz, priv->fifo_rxfsiz);
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

static void uhc_dwc2_port_fifo_precalc_dma(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);

	const uint32_t nptx_largest = EPSIZE_BULK_FS / 4;
	const uint32_t ptx_largest = 256 / 4;

	LOG_DBG("Init FIFO sizes");

	/* TODO: support HS */

	priv->fifo_top = UHC_DWC2_FIFODEPTH(config) - (UHC_DWC2_NUMHSTCHNL(config) + 1);
	priv->fifo_nptxfsiz = 2 * nptx_largest;
	priv->fifo_rxfsiz = 2 * (ptx_largest + 2) + UHC_DWC2_NUMHSTCHNL(config) + 1;
	priv->fifo_ptxfsiz = priv->fifo_top - (priv->fifo_nptxfsiz + priv->fifo_rxfsiz);

	/* TODO: verify ptxfsiz is overflowed */

	LOG_DBG("FIFO sizes: top=%u, nptx=%u, rx=%u, ptx=%u", priv->fifo_top * 4,
		priv->fifo_nptxfsiz * 4, priv->fifo_rxfsiz * 4, priv->fifo_ptxfsiz * 4);
}

static int uhc_dwc2_port_channel_claim(const struct device *dev, 
							struct uhc_transfer *const xfer, uint8_t *chan_idx)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;
	const struct usb_device *udev = xfer->udev;
	
	/* TODO: select non-claimed channel, use channel 0 for now */
	uint8_t idx = 0;

	struct uhc_dwc2_channel *channel = &priv->channel[idx];

	/* TODO: assign type */
	channel->type = UHC_DWC2_XFER_TYPE_CTRL;

	/* Save channel characteristics underlying channel */
	channel->xfer = xfer;
	channel->ep_addr = xfer->ep;
	channel->dev_addr = udev->addr;
	channel->ep_mps = xfer->mps;
	/* Claim channel */
	channel->claimed = 1;
	
	/* Init underlying channel registers */

	/* Clear the interrupt bits by writing them back */
	uint32_t hcint = sys_read32((mem_addr_t)&channel->regs->hcint);
	sys_write32(hcint, (mem_addr_t)&channel->regs->hcint);

	/* Enable channel interrupt in the core */
	sys_set_bits((mem_addr_t)&dwc2->haintmsk, (1 << idx));

	/* Enable transfer complete and channel halted interrupts */
	sys_set_bits((mem_addr_t)&channel->regs->hcintmsk,
						USB_DWC2_HCINT_XFERCOMPL | USB_DWC2_HCINT_CHHLTD);

	uint32_t hcchar = ((uint32_t)channel->ep_mps << USB_DWC2_HCCHAR_MPS_POS) |
			((uint32_t)USB_EP_GET_IDX(channel->ep_addr) << USB_DWC2_HCCHAR_EPNUM_POS) |
			((uint32_t)channel->type << USB_DWC2_HCCHAR_EPTYPE_POS) |
			((uint32_t)1UL /* TODO: ep_config->mult */ << USB_DWC2_HCCHAR_EC_POS)  |
			((uint32_t)channel->dev_addr << USB_DWC2_HCCHAR_DEVADDR_POS);

	if (USB_EP_DIR_IS_IN(channel->ep_addr)) {
		hcchar |= USB_DWC2_HCCHAR_EPDIR;
	}

	if (false /* channel->ls_via_fs_hub */) {
		hcchar |= USB_DWC2_HCCHAR_LSPDDEV;
	}

	if (channel->type == UHC_DWC2_XFER_TYPE_INTR) {
		hcchar |= USB_DWC2_HCCHAR_ODDFRM;
	}

	sys_write32(hcchar, (mem_addr_t)&channel->regs->hcchar);

	LOG_DBG("Claimed channel%d", idx);

	*chan_idx = idx;
	return 0;
}

static int uhc_dwc2_port_channel_release(const struct device *dev, uint8_t chan_idx)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_channel *channel = &priv->channel[chan_idx];

	sys_clear_bits((mem_addr_t)&dwc2->haintmsk, (1 << chan_idx));

	/* Release channel */
	channel->xfer = NULL;
	channel->claimed = 0;

	LOG_DBG("Released channel%d", chan_idx);

	return 0;
}

static int uhc_dwc2_port_channel_submit_ctrl(const struct device *dev, uint8_t chan_idx)
{
	struct uhc_dwc2_data *priv = uhc_get_private(dev);
	struct uhc_dwc2_channel *channel = &priv->channel[chan_idx];
	/* TODO: Configure channel assosiated registers and submit transfer */
	
	/* Get information about the control transfer by analyzing the setup packet */
	struct uhc_transfer *const xfer = channel->xfer;
	const struct usb_setup_packet *setup_pkt = (const struct usb_setup_packet *)xfer->setup_pkt;
	
	LOG_HEXDUMP_WRN(xfer->setup_pkt, 8, "setup");

	channel->ctrl_stg = UHC_CONTROL_STAGE_SETUP;
	channel->data_stg_in = usb_reqtype_is_to_host(setup_pkt);
	channel->data_stg_skip = (setup_pkt->wLength == 0);
	channel->set_address = (setup_pkt->bRequest == USB_SREQ_SET_ADDRESS)? 1 : 0;

	LOG_DBG("data_in: %s, data_skip: %s",
			channel->data_stg_in ? "true" : "false",
			channel->data_stg_skip ? "true" : "false");

	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		/* Control stage is always OUT */
		sys_clear_bits((mem_addr_t)&channel->regs->hcchar, USB_DWC2_HCCHAR_EPDIR);
	}

	if (xfer->interval != 0) {
		LOG_WRN("Periodic transfer is not supported");
	}

	const uint16_t pkt_cnt = calc_packet_count(sizeof(struct usb_setup_packet), channel->ep_mps);
	uint16_t size = sizeof(struct usb_setup_packet);

	uint32_t hctsiz = ((UHC_DWC2_PID_MDATA_SETUP << USB_DWC2_HCTSIZ_PID_POS) & USB_DWC2_HCTSIZ_PID_MASK) |
					((pkt_cnt << USB_DWC2_HCTSIZ_PKTCNT_POS) & USB_DWC2_HCTSIZ_PKTCNT_MASK) |
					((size << USB_DWC2_HCTSIZ_XFERSIZE_POS) & USB_DWC2_HCTSIZ_XFERSIZE_MASK);

	sys_write32(hctsiz, (mem_addr_t)&channel->regs->hctsiz);

	sys_write32((uint32_t)xfer->setup_pkt, (mem_addr_t)&channel->regs->hcdma);

	/* TODO: Configure split transaction if needed */

	uint32_t hcint = sys_read32((mem_addr_t)&channel->regs->hcint);
	sys_write32(hcint, (mem_addr_t)&channel->regs->hcint);

	/* TODO: sync CACHE */

	uint32_t hcchar = sys_read32((mem_addr_t)&channel->regs->hcchar);

	hcchar |= USB_DWC2_HCCHAR_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&channel->regs->hcchar);

	channel->executing = 1;

	return 0;
}

struct uhc_dwc2_channel *uhc_dwc2_port_get_pending_channel(const struct device *dev)
{
	struct uhc_dwc2_data *priv = uhc_get_private(dev);

	int chan_num = __builtin_ffs(priv->pending_channels_msk);
	if (chan_num) {
		/* Clear the pending bit for that channel */
		priv->pending_channels_msk &= ~(1 << (chan_num - 1));
		return &priv->channel[chan_num - 1];
	} else {
		return NULL;
	}
}

/* ---------------------------- */

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

	uhc_submit_event(dev, type, 0);
}

static inline void uhc_dwc2_submit_dev_gone(const struct device *dev)
{
	LOG_WRN("Device removed");

	uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
}

static int uhc_dwc2_submit_xfer(const struct device *const dev, struct uhc_transfer *const xfer)
{
	const struct usb_device *udev = xfer->udev;
	uint8_t chan_idx;
	int ret; 

	LOG_DBG("dev_addr=%d, ep=%02Xh, mps=%d, interval=%d, start_frame=%d, stage=%d, no_status=%d",
		udev->addr, xfer->ep, xfer->mps, xfer->interval,
		xfer->start_frame, xfer->stage, xfer->no_status);

	/* TODO: dma requirement, setup packet must be aligned 4 bytes */
	if (((uintptr_t)xfer->setup_pkt % 4)) {
		LOG_WRN("Setup packet address %p is not 4-byte aligned", 
						xfer->setup_pkt);
	}

	/* TODO: dma requirement, buffer addr that will used as dma addr also should be aligned */
	if((xfer->buf != NULL) && ((uintptr_t)net_buf_tail(xfer->buf) % 4)) {
		LOG_WRN("Buffer address %08lXh is not 4-byte aligned", 
						(uintptr_t)net_buf_tail(xfer->buf));
	}
		
	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		/* Control transfer */

		ret = uhc_dwc2_port_channel_claim(dev, xfer, &chan_idx);
		if (ret) {
			LOG_ERR("Failed to claim channel: %d", ret);
			return ret;
		}

		ret = uhc_dwc2_port_channel_submit_ctrl(dev, chan_idx);
		if (ret) {
			LOG_ERR("Failed to submit ctrl channel%d: %d", chan_idx, ret);
			return ret;
		}
	} else {
		/* Other */
		LOG_ERR("Non-control endpoint submit not implemented yet");
		return -ENOSYS;
	}

	return 0;
}


/* ---------------------------- */

static void uhc_dwc2_port_handle_events(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	enum uhc_dwc2_port_event event = uhc_dwc2_port_debounce_event(dev);

	switch (event) {
		case UHC_DWC2_PORT_EVENT_NONE:
			/* No event, nothing to do */
			break;
		case UHC_DWC2_PORT_EVENT_CONNECTION: {
			/* Device connected to the port, but hasn't been reset */
			/* We don't have the device speed yet, but the device already in the port. */
			LOG_DBG("Port connected");

			/* Reset the port to get the device speed */
			uhc_dwc2_port_reset(dev);
			break;
		}
		case UHC_DWC2_PORT_EVENT_ENABLED: {
			/* Port has a device connected and finish the first reset, we know the speed */
			LOG_DBG("Port enabled");

			if (priv->has_device == 0) {
				/* Port new connection */
				enum uhc_dwc2_speed speed = dwc2_hal_get_speed(dwc2);
				/* Initialize remaining host port registers */
				dwc2_hal_enable_port(dwc2, speed);

				priv->has_device = 1;
				/* Notify the higher logic about the new device */
				uhc_dwc2_submit_new_device(dev, speed);
			}

			break;
		}
		case UHC_DWC2_PORT_EVENT_DISCONNECTION:
		case UHC_DWC2_PORT_EVENT_ERROR:
		case UHC_DWC2_PORT_EVENT_OVERCURRENT: {
			LOG_DBG("Port disconnected / error");

			if (priv->has_device) {
				priv->has_device = 0;
				uhc_dwc2_submit_dev_gone(dev);
			}

			/* TODO: recover from the error */

			/* Reset the controller to handle new connection */
			uhc_dwc2_soft_reset(dev);
			break;
		}
		default:
			break;
	}
}

static void uhc_dwc2_channel_handle_events(const struct device *dev)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);	
	/* TODO: support more channels  */
	uint8_t chan_idx = 0;

	struct uhc_dwc2_channel *channel = &priv->channel[chan_idx];

	LOG_DBG("New event on channel%d, %d", chan_idx, channel->last_event);

	switch (channel->last_event) {
		case UHC_DWC2_CHANNEL_EVENT_XFER_DONE: {
			struct uhc_transfer *const xfer = channel->xfer;
			/* XFER transfer is done, process the transfer and release the pipe buffer */
			channel->executing = 0;

			if (xfer->buf != NULL && xfer->buf->len) {
				LOG_HEXDUMP_WRN(xfer->buf->data, xfer->buf->len, "data");
			}

			if (channel->set_address) {
				/* Device address needs time to be applied */
				k_msleep(SET_ADDR_DELAY_MS);
			}

			/* Release channel */
			uhc_dwc2_port_channel_release(dev, chan_idx);

			uhc_xfer_return(dev, xfer, 0);
			break;
		}
		default: {
			LOG_WRN("Unhandled channel%d event: %d", chan_idx, channel->last_event);
			break;
		}
	}
}

static void uhc_dwc2_isr_handler(const struct device *dev)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	enum uhc_dwc2_port_event port_event = uhc_dwc2_port_get_event(dev);

	switch (port_event)
	{
	case UHC_DWC2_PORT_EVENT_CHANNEL: {
		/* This is the one event, we do not propagate to the thread. */
		/* Instead, we handle the channels and keep the last event in associated channel object. */
		struct uhc_dwc2_channel *channel = uhc_dwc2_port_get_pending_channel(dev);
		while (channel != NULL) {
			enum uhc_dwc2_channel_event channel_event = uhc_dwc2_get_channel_event(channel);
			if (channel_event != UHC_DWC2_CHANNEL_EVENT_NONE) {
				channel->last_event = channel_event;
				/* TODO: Carefully here, we might start the handling in thread already */
				k_event_set(&priv->event, BIT(UHC_DWC2_EVENT_CHANNEL));
			}
			/* Check for more channels with pending interrupts. Returns NULL if there are no more */
			channel = uhc_dwc2_port_get_pending_channel(dev);
		}
		break;
	}
	case UHC_DWC2_PORT_EVENT_ERROR:
	case UHC_DWC2_PORT_EVENT_OVERCURRENT:
	case UHC_DWC2_PORT_EVENT_CONNECTION:
	case UHC_DWC2_PORT_EVENT_DISABLED:
	case UHC_DWC2_PORT_EVENT_DISCONNECTION:
	case UHC_DWC2_PORT_EVENT_ENABLED: {
		priv->last_event = port_event;
		k_event_set(&priv->event, BIT(UHC_DWC2_EVENT_PORT));
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
	uint32_t event;

	while (true) {
		event = k_event_wait_safe(&priv->event, UINT32_MAX, false, K_FOREVER);

		uhc_lock_internal(dev, K_FOREVER);

		if (event & BIT(UHC_DWC2_EVENT_PORT)) {
			uhc_dwc2_port_handle_events(dev);
		}

		if (event & BIT(UHC_DWC2_EVENT_CHANNEL)) {
			uhc_dwc2_channel_handle_events(dev);
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
	LOG_DBG("Bus reset");
	/* Reset port */
	uhc_dwc2_port_reset(dev);
	
	/* Hint: First reset is done by the uhc dwc2 driver, so we don't need to do anything here */
	uhc_submit_event(dev, UHC_EVT_RESETED, 0);
	return 0;
}

static int uhc_dwc2_bus_resume(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_enqueue(const struct device *const dev, struct uhc_transfer *const xfer)
{
	int ret;

	(void)uhc_xfer_append(dev, xfer);

	uhc_lock_internal(dev, K_FOREVER);

	ret = uhc_dwc2_submit_xfer(dev, xfer);

	uhc_unlock_internal(dev);

	return ret;
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

	/*
	 * TODO:
	 * use devicetree to get GHWCFGn values and use them to determine the
	 * number and type of configured endpoints in the hardware as in udc?
	 */

	config->make_thread(dev);

	return 0;
}

static int uhc_dwc2_init(const struct device *const dev)
{
	struct uhc_dwc2_data *priv = uhc_get_private(dev);
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

	/* Pre-calculate FIFO settings */
	uhc_dwc2_port_fifo_precalc_dma(dev);

	/* 4. Init DWC2 controller as a host */

	ret = dwc2_hal_init_host(dwc2);
	if (ret) {
		return ret;
	}

	/* Init the channels constant values */
	for (uint32_t n = 0; n < UHC_DWC2_MAX_CHAN; n++) {
		priv->channel[n].regs = UHC_DWC2_CHAN_REG(dwc2, n);
		priv->channel[n].index = n;
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

#define UHC_DWC2_DT_INST_REG_ADDR(n)				\
	COND_CODE_1(DT_NUM_REGS(DT_DRV_INST(n)),		\
	(DT_INST_REG_ADDR(n)),							\
	(DT_INST_REG_ADDR_BY_NAME(n, core)))

#if !defined(UHC_DWC2_IRQ_DT_INST_DEFINE)
#define UHC_DWC2_IRQ_FLAGS_TYPE0(n)	0
#define UHC_DWC2_IRQ_FLAGS_TYPE1(n)	DT_INST_IRQ(n, type)
#define DW_IRQ_FLAGS(n) \
	_CONCAT(UHC_DWC2_IRQ_FLAGS_TYPE, DT_INST_IRQ_HAS_CELL(n, type))(n)

#define UHC_DWC2_IRQ_DT_INST_DEFINE(n)	\
	static void uhc_dwc2_irq_enable_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    uhc_dwc2_isr_handler,				\
			    DEVICE_DT_INST_GET(n),				\
			    DW_IRQ_FLAGS(n));					\
										\
		irq_enable(DT_INST_IRQN(n));	\
	}									\
										\
	static void uhc_dwc2_irq_disable_func_##n(const struct device *dev)	\
	{									\
		irq_disable(DT_INST_IRQN(n));	\
	}
#endif

#define UHC_DWC2_PINCTRL_DT_INST_DEFINE(n)	\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),	\
			(PINCTRL_DT_INST_DEFINE(n)), ())

#define UHC_DWC2_PINCTRL_DT_INST_DEV_CONFIG_GET(n)		\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),	\
			((void *)PINCTRL_DT_INST_DEV_CONFIG_GET(n)),\
			(NULL))

/* Multi-instance device definition for DWC2 host controller */
#define UHC_DWC2_DEVICE_DEFINE(n)						\
	UHC_DWC2_PINCTRL_DT_INST_DEFINE(n);					\
										\
	K_THREAD_STACK_DEFINE(uhc_dwc2_stack_##n,	\
		CONFIG_UHC_DWC2_STACK_SIZE);	\
										\
	static void uhc_dwc2_thread_##n(void *dev, void *arg1, void *arg2)		\
	{									\
		while (true) {								\
			uhc_dwc2_thread_handler(dev);			\
		}								\
	}									\
										\
	static void uhc_dwc2_make_thread_##n(const struct device *dev)		\
	{									\
		struct uhc_dwc2_data *priv = uhc_get_private(dev);		\
										\
		k_thread_create(&priv->thread,		\
				uhc_dwc2_stack_##n,			\
				K_THREAD_STACK_SIZEOF(uhc_dwc2_stack_##n),	\
				uhc_dwc2_thread_##n,	\
				(void *)dev, NULL, NULL,	\
				K_PRIO_COOP(CONFIG_UHC_DWC2_THREAD_PRIORITY),	\
				K_ESSENTIAL,	\
				K_NO_WAIT);	\
		k_thread_name_set(&priv->thread, dev->name);			\
	}									\
										\
	UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
										\
	static struct uhc_dwc2_data uhc_dwc2_priv_##n = {	\
		.irq_sem = Z_SEM_INITIALIZER(uhc_dwc2_priv_##n.irq_sem, 0, 1),	\
	};									\
										\
	static struct uhc_data uhc_data_##n = {		\
		.mutex = Z_MUTEX_INITIALIZER(uhc_data_##n.mutex),\
		.priv = &uhc_dwc2_priv_##n,		\
		};								\
										\
	static const struct uhc_dwc2_config uhc_dwc2_config_##n = {	\
		.base = (struct usb_dwc2_reg *)UHC_DWC2_DT_INST_REG_ADDR(n),	\
		.quirks = UHC_DWC2_VENDOR_QUIRK_GET(n),	\
		.pcfg = UHC_DWC2_PINCTRL_DT_INST_DEV_CONFIG_GET(n),	\
		.make_thread = uhc_dwc2_make_thread_##n,	\
		.irq_enable_func = uhc_dwc2_irq_enable_func_##n,	\
		.irq_disable_func = uhc_dwc2_irq_disable_func_##n,	\
		.gsnpsid = DT_INST_PROP(n, gsnpsid),	\
		.ghwcfg1 = DT_INST_PROP(n, ghwcfg1),	\
		.ghwcfg2 = DT_INST_PROP(n, ghwcfg2),	\
		.ghwcfg3 = DT_INST_PROP(n, ghwcfg3),	\
		.ghwcfg4 = DT_INST_PROP(n, ghwcfg4),	\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, uhc_dwc2_preinit, NULL,	\
		&uhc_data_##n, &uhc_dwc2_config_##n,	\
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		&uhc_dwc2_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_DWC2_DEVICE_DEFINE)
