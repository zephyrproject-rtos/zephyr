/*
 * Copyright (c) 2026 Roman Leonov <jam_roma@yahoo.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_dwc2

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usb_ch9.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_dwc2, CONFIG_UHC_DRIVER_LOG_LEVEL);

#include "uhc_common.h"
#include "uhc_dwc2.h"

#define DEBOUNCE_DELAY_MS	CONFIG_UHC_DWC2_DEBOUNCE_DELAY_MS
#define RESET_HOLD_MS		CONFIG_UHC_DWC2_RESET_HOLD_MS
#define RESET_RECOVERY_MS	CONFIG_UHC_DWC2_RESET_RECOVERY_MS
#define SET_ADDR_DELAY_MS	CONFIG_UHC_DWC2_SET_ADDR_DELAY_MS
#define MAX_CHANNELS		CONFIG_UHC_DWC2_MAX_CHANNELS

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

enum uhc_dwc2_channel_event {
	/* The channel has completed a transfer. Channel is now halted */
	UHC_DWC2_CHANNEL_EVENT_CPLT,
	/* The channel has completed a transfer. Request STALLed. Channel is now halted */
	UHC_DWC2_CHANNEL_EVENT_STALL,
	/* The channel has encountered an error. Channel is now halted */
	UHC_DWC2_CHANNEL_EVENT_ERROR,
	/* Need to release the channel for the next transfer */
	UHC_DWC2_CHANNEL_DO_RELEASE,
};

enum uhc_dwc2_speed {
	UHC_DWC2_SPEED_HIGH = 0,
	UHC_DWC2_SPEED_FULL = 1,
	UHC_DWC2_SPEED_LOW = 2,
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

#define UHC_DWC2_FIFODEPTH(config)						\
	((uint32_t)(((config)->ghwcfg3 & USB_DWC2_GHWCFG3_DFIFODEPTH_MASK) >>	\
		    USB_DWC2_GHWCFG3_DFIFODEPTH_POS))

#define UHC_DWC2_NUMHSTCHNL(config)						\
	((uint32_t)(((config)->ghwcfg2 & USB_DWC2_GHWCFG2_NUMHSTCHNL_MASK) >>	\
		    USB_DWC2_GHWCFG2_NUMHSTCHNL_POS))


/* Mask to clear HPRT register */
#define USB_DWC2_HPRT_W1C_MSK	(USB_DWC2_HPRT_PRTENA |				\
				 USB_DWC2_HPRT_PRTCONNDET |			\
				 USB_DWC2_HPRT_PRTENCHNG |			\
				 USB_DWC2_HPRT_PRTOVRCURRCHNG)

struct uhc_dwc2_channel {
	/* Index of the channel */
	uint8_t index;
	/* Cached pointer to channel registers */
	const struct usb_dwc2_host_chan *regs;
	/* Pointer to the transfer */
	struct uhc_transfer *xfer;
	/* Channel events */
	atomic_t events;

	/* Channel flags */
	/* Transfer stage is IN */
	uint8_t data_stg_in: 1;
	/* Transfer has no data stage */
	uint8_t data_stg_skip: 1;
	/* Transfer will change the device address */
	uint8_t set_address: 1;
};

struct uhc_dwc2_data {
	struct k_thread thread;
	/* Event bitmask the driver thread waits for */
	struct k_event event;
	/* Semaphore used to indicate that port is enabled */
	struct k_sem sem_port_enabled;
	/* Port channels */
	struct uhc_dwc2_channel channel[MAX_CHANNELS];
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

static inline void dwc2_hal_set_reset(struct usb_dwc2_reg *const dwc2, const bool reset_on)
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

static int dwc2_hal_flush_rx_fifo(struct usb_dwc2_reg *const dwc2)
{
	const k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(100));

	sys_write32(USB_DWC2_GRSTCTL_RXFFLSH, (mem_addr_t)&dwc2->grstctl);

	while (sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_RXFFLSH) {
		k_busy_wait(1);

		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for RXFFLSH timeout");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int dwc2_hal_flush_tx_fifo(struct usb_dwc2_reg *const dwc2, const uint8_t fnum)
{
	const k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(100));
	uint32_t grstctl;

	grstctl = usb_dwc2_set_grstctl_txfnum(fnum) | USB_DWC2_GRSTCTL_TXFFLSH;
	sys_write32(grstctl, (mem_addr_t)&dwc2->grstctl);

	while (sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_TXFFLSH) {
		k_busy_wait(1);

		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for TXFFLSH timeout");
			return -ETIMEDOUT;
		}
	}

	return 0;
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

static inline int dwc2_hal_init_gusbcfg(struct usb_dwc2_reg *const dwc2)
{
	uint32_t gusbcfg = sys_read32((mem_addr_t)&dwc2->gusbcfg);
	uint32_t ghwcfg2 = sys_read32((mem_addr_t)&dwc2->ghwcfg2);
	uint32_t ghwcfg4 = sys_read32((mem_addr_t)&dwc2->ghwcfg4);
	const k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(100));

	/* Enable Host mode */
	sys_set_bits((mem_addr_t)&dwc2->gusbcfg, USB_DWC2_GUSBCFG_FORCEHSTMODE);

	/* Wait until core is in host mode */
	while ((sys_read32((mem_addr_t)&dwc2->gintsts) & USB_DWC2_GINTSTS_CURMOD) == 0) {
		k_busy_wait(1);

		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for Host mode timeout, GINTSTS 0x%08x",
				sys_read32((mem_addr_t)&dwc2->gintsts));
			return -ETIMEDOUT;
		}
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
			/* Disable FS/LS ULPI and Suspend mode */
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

	return 0;
}

static inline int dwc2_hal_init_gahbcfg(struct usb_dwc2_reg *const dwc2)

{
	uint32_t ghwcfg2;
	uint32_t gahbcfg;

	/* Disable Global Interrupt */
	sys_clear_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);

	/* Configure AHB */
	gahbcfg = sys_read32((mem_addr_t)&dwc2->gahbcfg);
	gahbcfg &= ~USB_DWC2_GAHBCFG_HBSTLEN_MASK;
	gahbcfg |= usb_dwc2_set_gahbcfg_hbstlen(USB_DWC2_GAHBCFG_HBSTLEN_INCR16);
	sys_write32(gahbcfg, (mem_addr_t)&dwc2->gahbcfg);

	ghwcfg2 = sys_read32((mem_addr_t)&dwc2->ghwcfg2);
	if (usb_dwc2_get_ghwcfg2_otgarch(ghwcfg2) == USB_DWC2_GHWCFG2_OTGARCH_INTERNALDMA) {
		sys_set_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_DMAEN);
	} else {
		/* TODO: Implement Slave mode */
		LOG_ERR("DMA isn't supported by the hardware");
		return -ENXIO;
	}

	/* Enable Global Interrupt */
	sys_set_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);
	return 0;
}

static int dwc2_core_soft_reset(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	mem_addr_t grstctl_reg = (mem_addr_t)&dwc2->grstctl;
	mem_addr_t gsnpsid_reg = (mem_addr_t)&dwc2->gsnpsid;
	k_timepoint_t timepoint;
	uint32_t gsnpsid;
	uint32_t grstctl;
	uint32_t gsnpsid_rev;

	/* Check AHB master idle state before starting reset */
	timepoint = sys_timepoint_calc(K_MSEC(100));
	while (!(sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_AHBIDLE)) {
		k_busy_wait(1);

		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for AHB idle timeout, GRSTCTL 0x%08x",
				sys_read32(grstctl_reg));
			return -ETIMEDOUT;
		}
	}

	/*
	 * Load GSNPSID before reset. On some cores it may not be readable
	 * after reset is asserted.
	 */
	gsnpsid = sys_read32(gsnpsid_reg);
	gsnpsid_rev = usb_dwc2_get_gsnpsid_rev(gsnpsid);

	/* Apply Core Soft Reset */
	sys_set_bits(grstctl_reg, USB_DWC2_GRSTCTL_CSFTRST);

	if (gsnpsid_rev < usb_dwc2_get_gsnpsid_rev(USB_DWC2_GSNPSID_REV_4_20A)) {
		/*
		 * Before v4.20a, CSFTRST is self-clearing.
		 * Wait until hardware clears it.
		 */
		timepoint = sys_timepoint_calc(K_MSEC(100));
		while (sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_CSFTRST) {
			k_busy_wait(1);

			if (sys_timepoint_expired(timepoint)) {
				LOG_ERR("Wait for CSR clear timeout, GRSTCTL 0x%08x",
					sys_read32(grstctl_reg));
				return -ETIMEDOUT;
			}

			if (uhc_dwc2_quirk_is_phy_clk_off(dev)) {
				LOG_ERR("Core soft reset stuck, PHY clock is off");
				return -EIO;
			}
		}

		/*
		 * Hardware requires at least 3 PHY clocks after CSFTRST clears
		 * before accessing the PHY domain. Use a conservative delay.
		 */
		k_busy_wait(1);
	} else {
		/*
		 * From v4.20a, CSFTRST is write-only/reset-controlled.
		 * Wait for CSFTRSTDONE, then clear DONE using W1C.
		 */
		timepoint = sys_timepoint_calc(K_MSEC(100));
		while (!(sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_CSFTRSTDONE)) {
			k_busy_wait(1);

			if (sys_timepoint_expired(timepoint)) {
				LOG_ERR("Wait for CSR done timeout, GRSTCTL 0x%08x",
					sys_read32(grstctl_reg));
				return -ETIMEDOUT;
			}

			if (uhc_dwc2_quirk_is_phy_clk_off(dev)) {
				LOG_ERR("Core soft reset stuck, PHY clock is off");
				return -EIO;
			}
		}

		/*
		 * CSFTRSTDONE is W1C. Preserve normal bits, clear CSFTRST,
		 * and write 1 to CSFTRSTDONE to clear the done indication.
		 */
		grstctl = sys_read32(grstctl_reg);
		grstctl &= ~USB_DWC2_GRSTCTL_CSFTRST;
		grstctl |= USB_DWC2_GRSTCTL_CSFTRSTDONE;
		sys_write32(grstctl, grstctl_reg);
	}

	/* Wait for AHB master idle again after reset */
	timepoint = sys_timepoint_calc(K_MSEC(100));
	while (!(sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_AHBIDLE)) {
		k_busy_wait(1);

		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for AHB idle after reset timeout, GRSTCTL 0x%08x",
				sys_read32(grstctl_reg));
			return -ETIMEDOUT;
		}
	}

	LOG_DBG("DWC2 core reset done");

	return 0;
}

static int dwc2_hal_init_host(struct usb_dwc2_reg *const dwc2)
{
	uint32_t gintsts;
	int ret;

	/* Init GUSBCFG */
	ret = dwc2_hal_init_gusbcfg(dwc2);
	if (ret != 0) {
		LOG_ERR("Unable to configure USB global register");
		return ret;
	}

	/* Init GAHBCFG */
	ret = dwc2_hal_init_gahbcfg(dwc2);
	if (ret != 0) {
		/* TODO: Implement Slave Mode */
		LOG_WRN("DMA isn't supported, but Slave Mode isn't implemented yet");
		return ret;
	}

	/* Clear interrupts */
	sys_clear_bits((mem_addr_t)&dwc2->gintmsk, 0xFFFFFFFFUL);
	sys_set_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_DISCONNINT);

	/* Clear status */
	gintsts = sys_read32((mem_addr_t)&dwc2->gintsts);
	sys_write32(gintsts, (mem_addr_t)&dwc2->gintsts);

	return ret;
}

static int dwc2_hal_set_fifo_sizes(struct usb_dwc2_reg *const dwc2,
				       const uint32_t ghwcfg2,
				       const uint32_t ghwcfg3)
{
	const uint32_t nptx_largest = EPSIZE_BULK_FS / 4;
	const uint32_t ptx_largest = 256 / 4;
	const uint32_t dfifodepth = FIELD_GET(USB_DWC2_GHWCFG3_DFIFODEPTH_MASK, ghwcfg3);
	const uint32_t numhstchnl = FIELD_GET(USB_DWC2_GHWCFG2_NUMHSTCHNL_MASK, ghwcfg2);
	uint32_t fifo_available = dfifodepth - (numhstchnl + 1);
	/* TODO: support HS */
	const uint16_t fifo_nptxfdep = 2 * nptx_largest;
	const uint16_t fifo_rxfsiz = 2 * (ptx_largest + 2) + numhstchnl + 1;
	const uint16_t fifo_ptxfsiz = fifo_available - (fifo_nptxfdep + fifo_rxfsiz);
	uint32_t gdfifocfg;
	uint32_t grxfsiz;
	uint32_t gnptxfsiz;
	uint32_t hptxfsiz;
	int ret;

	LOG_DBG("Setting FIFO sizes: top=%u, nptx=%u, rx=%u, ptx=%u",
		fifo_available * 4, fifo_nptxfdep * 4, fifo_rxfsiz * 4, fifo_ptxfsiz * 4);

	/* FIFO config */
	gdfifocfg = usb_dwc2_set_gdfifocfg_epinfobaseaddr(fifo_available);
	gdfifocfg |= usb_dwc2_set_gdfifocfg_gdfifocfg(fifo_available);
	sys_write32(gdfifocfg, (mem_addr_t)&dwc2->gdfifocfg);

	/* RX FIFO size */
	fifo_available -= fifo_rxfsiz;
	grxfsiz = usb_dwc2_set_grxfsiz(fifo_rxfsiz);
	sys_write32(grxfsiz, (mem_addr_t)&dwc2->grxfsiz);

	/* Non-periodic TX FIFO size */
	fifo_available -= fifo_nptxfdep;
	gnptxfsiz = usb_dwc2_set_gnptxfsiz_nptxfdep(fifo_nptxfdep);
	gnptxfsiz |= usb_dwc2_set_gnptxfsiz_nptxfstaddr(fifo_available);
	sys_write32(gnptxfsiz, (mem_addr_t)&dwc2->gnptxfsiz);

	/* Periodic TX FIFO size */
	fifo_available -= fifo_ptxfsiz;
	hptxfsiz = usb_dwc2_set_hptxfsiz_ptxfsize(fifo_ptxfsiz);
	hptxfsiz |= fifo_available;
	sys_write32(hptxfsiz, (mem_addr_t)&dwc2->hptxfsiz);

	/* Flush TX and RX FIFOs */
	ret = dwc2_hal_flush_tx_fifo(dwc2, MAX_CHANNELS);
	if (ret != 0) {
		return ret;
	}

	return dwc2_hal_flush_rx_fifo(dwc2);
}

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

static inline void uhc_dwc2_channel_process_control(struct uhc_dwc2_channel *channel)
{
	struct uhc_transfer *const xfer = channel->xfer;
	bool next_dir_is_in;
	enum uhc_dwc2_channel_pid next_pid;
	uint16_t size = 0;
	uint16_t pkt_cnt;
	uint8_t *dma_addr = NULL;
	uint32_t hcchar;
	uint32_t hctsiz;

	if (xfer->stage == UHC_CONTROL_STAGE_SETUP) {
		/* Just finished UHC_CONTROL_STAGE_SETUP */
		if (channel->data_stg_skip) {
			/* No data stage. Go strait to status */
			next_dir_is_in = true;
			next_pid = UHC_DWC2_PID_DATA1;
			xfer->stage = UHC_CONTROL_STAGE_STATUS;
		} else {
			/* Data stage is present, go to data stage */
			next_dir_is_in = channel->data_stg_in;
			next_pid = UHC_DWC2_PID_DATA1;
			xfer->stage = UHC_CONTROL_STAGE_DATA;

			/*
			 * NOTE:
			 * For OUT - number of bytes host sends to device
			 * For IN - number of bytes host reserves to receive
			 */
			if (!next_dir_is_in) {
				size = xfer->buf->len;
			} else {
				/* TODO: Check the buffer is large enough for the next transfer? */
				size = net_buf_tailroom(xfer->buf);
			}

			/* TODO: Toggle PID? */

			if (xfer->buf != NULL) {
				/* Get the tail of the buffer to append data */
				dma_addr = net_buf_tail(xfer->buf);
			}
		}
	} else {
		/* Just finished UHC_CONTROL_STAGE_DATA */
		hctsiz = sys_read32((mem_addr_t)&channel->regs->hctsiz);
		/* Increase the net_buf for the actual transferred len */
		net_buf_add(xfer->buf, xfer->buf->size - usb_dwc2_get_hctsiz_xfersize(hctsiz));
		/* Status stage is always the opposite direction of data stage */
		next_dir_is_in = !channel->data_stg_in;
		/* Status stage always has a PID of DATA1 */
		next_pid = UHC_DWC2_PID_DATA1;
		xfer->stage = UHC_CONTROL_STAGE_STATUS;
	}

	/* Calculate new packet count */
	pkt_cnt  = calc_packet_count(size, xfer->mps);

	if (next_dir_is_in) {
		sys_set_bits((mem_addr_t)&channel->regs->hcchar, USB_DWC2_HCCHAR_EPDIR);
	} else {
		sys_clear_bits((mem_addr_t)&channel->regs->hcchar, USB_DWC2_HCCHAR_EPDIR);
	}

	hctsiz = usb_dwc2_set_hctsiz_pid(next_pid) |
		usb_dwc2_set_hctsiz_pktcnt(pkt_cnt) |
		usb_dwc2_set_hctsiz_xfersize(size);

	sys_write32(hctsiz, (mem_addr_t)&channel->regs->hctsiz);
	sys_write32((uint32_t)dma_addr, (mem_addr_t)&channel->regs->hcdma);

	/* TODO: Configure split transaction if needed */

	/* TODO: sync CACHE */

	hcchar = sys_read32((mem_addr_t)&channel->regs->hcchar);
	hcchar |= USB_DWC2_HCCHAR_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&channel->regs->hcchar);
}

static uint32_t uhc_dwc2_channel_irq_handle_events(struct uhc_dwc2_channel *channel)
{
	struct uhc_transfer *const xfer = channel->xfer;
	uint32_t hcint = sys_read32((mem_addr_t)&channel->regs->hcint);
	uint32_t channel_events = 0;
	/* Only control transfers need to be handled in stages */
	const bool xfer_is_done = xfer->type != USB_EP_TYPE_CONTROL ||
				  xfer->stage == UHC_CONTROL_STAGE_STATUS;

	/* Clear the interrupt bits by writing them back */
	sys_write32(hcint, (mem_addr_t)&channel->regs->hcint);

	/* TODO: Read hcchar and split paths for EP IN and OUT? */

	/* TODO: Read hcchar and split paths for BULK/CTRL/ISOC/INTR? */

	if (hcint & USB_DWC2_HCINT_CHHLTD) {
		if (hcint & USB_DWC2_HCINT_XFERCOMPL) {
			/* TODO: channel error count = 0 */
			if (xfer_is_done) {
				channel_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
				channel_events |= BIT(UHC_DWC2_CHANNEL_EVENT_CPLT);
			} else {
				if (xfer->type == USB_EP_TYPE_CONTROL) {
					uhc_dwc2_channel_process_control(channel);
				} else {
					LOG_ERR("Only control xfer need to be handled in stages");
				}
			}
		} else if (hcint & USB_DWC2_HCINT_STALL) {
			/* TODO: channel error count = 0 */

			/* Expecting ACK interrupt next */
			sys_set_bits((mem_addr_t)&channel->regs->hcintmsk, USB_DWC2_HCINT_ACK);

			/* Notify thread */
			channel_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
			channel_events |= BIT(UHC_DWC2_CHANNEL_EVENT_STALL);
		} else {
			/* TODO: Add handling for other cases */
			LOG_WRN("Channel halted, handle not implemented: HCINT=0x%08x", hcint);
		}
	} else if (hcint & USB_DWC2_HCINT_ACK) {
		/* TODO: channel error count = 1 */

		/* Not expecting ACK interrupt anymore */
		sys_clear_bits((mem_addr_t)&channel->regs->hcintmsk, USB_DWC2_HCINT_ACK);
	} else {
		/* TODO: Handle channel error */
		LOG_WRN("Channel not halted, handle not implemented: HCINT=0x%08x", hcint);
	}

	return channel_events;
}

static enum uhc_dwc2_event uhc_dwc2_port_get_event(const struct device *dev)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;

	/* Read and clear core interrupt status */
	uint32_t core_intrs = sys_read32((mem_addr_t)&dwc2->gintsts);

	sys_write32(core_intrs, (mem_addr_t)&dwc2->gintsts);

	uint32_t port_intrs = 0;

	if (core_intrs & USB_DWC2_GINTSTS_PRTINT) {
		/* Port interrupt occurred -> read and clear selected HPRT W1C bits */
		port_intrs = sys_read32((mem_addr_t)&dwc2->hprt);
		/* W1C changed bits except the PRTENA */
		sys_write32(port_intrs & ~USB_DWC2_HPRT_PRTENA, (mem_addr_t)&dwc2->hprt);
	}

	LOG_DBG("GINTSTS=%08Xh, HPRT=%08Xh", core_intrs, port_intrs);

	/* Handle disconnect first */
	if (core_intrs & USB_DWC2_GINTSTS_DISCONNINT) {
		/* Port disconnected */
		uhc_dwc2_port_debounce_lock(dev);
		return UHC_DWC2_EVENT_PORT_DISCONNECTION;
	}

	/* To have better throughput, handle channels right after disconnection */
	if (core_intrs & USB_DWC2_GINTSTS_HCHINT) {
		priv->pending_channels_msk = sys_read32((mem_addr_t)&dwc2->haint);
		return UHC_DWC2_EVENT_PORT_PEND_CHANNEL;
	}

	/* Handle port overcurrent as it is a failure state */
	if (port_intrs & USB_DWC2_HPRT_PRTOVRCURRCHNG) {
		/* TODO: Overcurrent or overcurrent clear? */
		return UHC_DWC2_EVENT_PORT_OVERCURRENT;
	}

	/* Handle port change bits */
	if (port_intrs & USB_DWC2_HPRT_PRTENCHNG) {
		if (port_intrs & USB_DWC2_HPRT_PRTENA) {
			/* Notify thread */
			k_sem_give(&priv->sem_port_enabled);
			/* Host port was enabled */
			return UHC_DWC2_EVENT_PORT_ENABLED;
		}
		/* Host port has been disabled */
		return UHC_DWC2_EVENT_PORT_DISABLED;
	}

	/* Handle port connection */
	if (port_intrs & USB_DWC2_HPRT_PRTCONNDET && !priv->debouncing) {
		/* Port connected */
		uhc_dwc2_port_debounce_lock(dev);
		return UHC_DWC2_EVENT_PORT_CONNECTION;
	}

	return UHC_DWC2_EVENT_NONE;
}

static inline bool uhc_dwc2_port_debounce(const struct device *dev,
					enum uhc_dwc2_event event)
{
	const struct uhc_dwc2_config *config = dev->config;
	struct usb_dwc2_reg *dwc2 = config->base;

	/* Perform the debounce delay outside of the global lock */
	uhc_unlock_internal(dev);

	k_msleep(DEBOUNCE_DELAY_MS);

	uhc_lock_internal(dev, K_FOREVER);

	bool connected = ((sys_read32((mem_addr_t)&dwc2->hprt) & USB_DWC2_HPRT_PRTCONNSTS) != 0);
	bool want_connected = (event == UHC_DWC2_EVENT_PORT_CONNECTION);

	uhc_dwc2_port_debounce_unlock(dev);

	/* True if stable state matches the event */
	return connected == want_connected;
}

static inline void uhc_dwc2_port_power_on(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;

	sys_clear_bits((mem_addr_t)&dwc2->haintmsk, 0xFFFFFFFFUL);
	sys_set_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_PRTINT |
							USB_DWC2_GINTSTS_HCHINT);
	dwc2_hal_set_power(dwc2, true);
}

static inline void uhc_dwc2_port_power_off(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;

	sys_clear_bits((mem_addr_t)&dwc2->haintmsk, 0xFFFFFFFFUL);
	sys_clear_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_PRTINT |
							USB_DWC2_GINTSTS_HCHINT);
	dwc2_hal_set_power(dwc2, false);
}

static inline int uhc_dwc2_soft_reset(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	int ret;

	/* TODO: Check that port has no ongoing transfers */

	/* Disable Global IRQ */
	config->irq_disable_func(dev);

	/* Reset the core */
	ret = dwc2_core_soft_reset(dev);
	if (ret != 0) {
		LOG_ERR("Failed to perform soft reset: %d", ret);
		return ret;
	}

	/* Re-config after reset */
	dwc2_hal_init_gahbcfg(dwc2);

	/* Enable Global IRQ */
	config->irq_enable_func(dev);

	return ret;
}

static int uhc_dwc2_channel_claim(const struct device *const dev,
				  struct uhc_transfer *const xfer,
				  struct uhc_dwc2_channel **const channel_p)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	/* TODO: select non-claimed channel, use channel 0 for now */
	uint8_t idx = 0;
	struct uhc_dwc2_channel *const channel = &priv->channel[idx];

	LOG_DBG("Claimed channel%d, configuring it with xfer %p, channel %p",
		idx, (void *)xfer, (void *)channel);

	/* Save channel characteristics of the underlying channel */
	channel->xfer = xfer;

	*channel_p = channel;

	return 0;
}

static int uhc_dwc2_channel_configure(const struct device *const dev,
				      struct uhc_dwc2_channel *const channel)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_transfer *const xfer = channel->xfer;
	struct usb_device *const udev = xfer->udev;
	uint32_t hcint;
	uint32_t hcintmsk;
	uint32_t hcchar;

	/* Clear the interrupt bits by writing them back */
	hcint = sys_read32((mem_addr_t)&channel->regs->hcint);
	sys_write32(hcint, (mem_addr_t)&channel->regs->hcint);

	/* Enable channel interrupt in the core */
	sys_set_bits((mem_addr_t)&dwc2->haintmsk, BIT(channel->index));

	hcintmsk = sys_read32((mem_addr_t)&channel->regs->hcintmsk);
	/* Enable transfer complete and channel halted interrupts */
	hcintmsk |= USB_DWC2_HCINT_XFERCOMPL;
	hcintmsk |= USB_DWC2_HCINT_CHHLTD;
	/* Enable error interrupts */
	hcintmsk |= USB_DWC2_HCINT_AHBERR;
	hcintmsk |= USB_DWC2_HCINT_STALL;
	hcintmsk |= USB_DWC2_HCINT_XACTERR;
	hcintmsk |= USB_DWC2_HCINT_BBLERR;
	hcintmsk |= USB_DWC2_HCINT_FRMOVRUN;
	hcintmsk |= USB_DWC2_HCINT_DTGERR;
	hcintmsk |= USB_DWC2_HCINT_XACTERR;
	sys_write32(hcintmsk, (mem_addr_t)&channel->regs->hcintmsk);

	/* Configure the channel main properties */
	hcchar = usb_dwc2_set_hcchar_mps(xfer->mps);
	hcchar |= usb_dwc2_set_hcchar_epnum(USB_EP_GET_IDX(xfer->ep));
	hcchar |= usb_dwc2_set_hcchar_eptype(xfer->type);
	hcchar |= usb_dwc2_set_hcchar_ec(1UL /* TODO: ep_config->mult */);
	hcchar |= usb_dwc2_set_hcchar_devaddr(udev->addr);

	if (USB_EP_DIR_IS_IN(xfer->ep)) {
		hcchar |= USB_DWC2_HCCHAR_EPDIR;
	}

	if (false /* TODO: Support Hubs channel->ls_via_fs_hub */) {
		hcchar |= USB_DWC2_HCCHAR_LSPDDEV;
	}

	if (xfer->type == USB_EP_TYPE_INTERRUPT) {
		hcchar |= USB_DWC2_HCCHAR_ODDFRM;
	}

	sys_write32(hcchar, (mem_addr_t)&channel->regs->hcchar);

	return 0;
}

static int uhc_dwc2_channel_release(const struct device *dev,
				struct uhc_dwc2_channel *channel)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;

	sys_clear_bits((mem_addr_t)&dwc2->haintmsk, (1 << channel->index));

	/* Release channel */
	channel->xfer = NULL;

	LOG_DBG("Released channel%d", channel->index);

	return 0;
}

static int uhc_dwc2_channel_start_transfer_ctrl(struct uhc_dwc2_channel *channel)
{
	struct uhc_transfer *const xfer = channel->xfer;
	const struct usb_setup_packet *setup_pkt = (const struct usb_setup_packet *)xfer->setup_pkt;
	uint16_t pkt_cnt;
	uint32_t hctsiz;
	uint32_t hcint;
	uint32_t hcchar;

	LOG_HEXDUMP_DBG(xfer->setup_pkt, 8, "setup");

	xfer->stage = UHC_CONTROL_STAGE_SETUP;

	/* Get information about the control transfer by analyzing the setup packet */
	channel->data_stg_in = usb_reqtype_is_to_host(setup_pkt);
	channel->data_stg_skip = (setup_pkt->wLength == 0);
	channel->set_address = (setup_pkt->bRequest == USB_SREQ_SET_ADDRESS) ? 1 : 0;

	LOG_DBG("data_in: %s, data_skip: %s",
			channel->data_stg_in ? "true" : "false",
			channel->data_stg_skip ? "true" : "false");

	/* TODO: Control transfer should always use EP0, make sense to assert */

	/* Control stage is always OUT */
	sys_clear_bits((mem_addr_t)&channel->regs->hcchar, USB_DWC2_HCCHAR_EPDIR);

	pkt_cnt  = calc_packet_count(sizeof(struct usb_setup_packet), xfer->mps);
	hctsiz = usb_dwc2_set_hctsiz_pid(UHC_DWC2_PID_MDATA_SETUP) |
		usb_dwc2_set_hctsiz_pktcnt(pkt_cnt) |
		usb_dwc2_set_hctsiz_xfersize(sizeof(struct usb_setup_packet));

	sys_write32(hctsiz, (mem_addr_t)&channel->regs->hctsiz);
	sys_write32((uint32_t)xfer->setup_pkt, (mem_addr_t)&channel->regs->hcdma);

	/* TODO: Configure split transaction if needed */

	hcint = sys_read32((mem_addr_t)&channel->regs->hcint);
	sys_write32(hcint, (mem_addr_t)&channel->regs->hcint);

	/* TODO: sync CACHE */

	hcchar = sys_read32((mem_addr_t)&channel->regs->hcchar);
	hcchar |= USB_DWC2_HCCHAR_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&channel->regs->hcchar);

	return 0;
}

struct uhc_dwc2_channel *uhc_dwc2_channel_get_pending(const struct device *dev)
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

static inline void uhc_dwc2_submit_new_device(const struct device *dev, enum uhc_dwc2_speed speed)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	static const char *const uhc_dwc2_speed_str[] = {"High", "Full", "Low"};
	enum uhc_event_type type;

	LOG_DBG("New device, %s-speed", uhc_dwc2_speed_str[speed]);

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

	priv->has_device = 1;
	uhc_submit_event(dev, type, 0);
}

static inline void uhc_dwc2_submit_dev_gone(const struct device *dev)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);

	if (!priv->has_device) {
		return;
	}

	LOG_DBG("Device removed");

	uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
	priv->has_device = 0;
}

static int uhc_dwc2_submit_xfer(const struct device *const dev, struct uhc_transfer *const xfer)
{
	struct uhc_dwc2_channel *channel = NULL;
	int ret;

	LOG_DBG("ep=%02Xh, mps=%d, int=%d, start_frame=%d, stage=%d, no_status=%d",
		xfer->ep, xfer->mps, xfer->interval,
		xfer->start_frame, xfer->stage, xfer->no_status);

	/* TODO: dma requirement, setup packet must be aligned 4 bytes */
	if (((uintptr_t)xfer->setup_pkt % 4)) {
		LOG_WRN("Setup packet address %p is not 4-byte aligned",
					xfer->setup_pkt);
	}

	/* TODO: dma requirement, buffer addr that will used as dma addr also should be aligned */
	if ((xfer->buf != NULL) && ((uintptr_t)net_buf_tail(xfer->buf) % 4)) {
		LOG_WRN("Buffer address %08lXh is not 4-byte aligned",
					(uintptr_t)net_buf_tail(xfer->buf));
	}

	if (xfer->type != USB_EP_TYPE_CONTROL) {
		LOG_WRN("Channel type %d isn't supported yet", xfer->type);
		return -EINVAL;
	}

	ret = uhc_dwc2_channel_claim(dev, xfer, &channel);
	if (ret != 0) {
		LOG_ERR("Failed to claim channel: %d", ret);
		return ret;
	}

	ret = uhc_dwc2_channel_configure(dev, channel);
	if (ret != 0) {
		LOG_ERR("Failed to configure channel: %d", ret);
		uhc_dwc2_channel_release(dev, channel);
		return ret;
	}

	return uhc_dwc2_channel_start_transfer_ctrl(channel);
}

static void uhc_dwc2_port_handle_events(const struct device *dev, uint32_t event_mask)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;

	LOG_DBG("Port events: %08Xh", event_mask);

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_CONNECTION)) {
		/* Port connected */
		LOG_DBG("Port connected");
		/* Debounce port connection */
		if (uhc_dwc2_port_debounce(dev, UHC_DWC2_EVENT_PORT_CONNECTION)) {
			/* Get port speed */
			enum uhc_dwc2_speed speed = dwc2_hal_get_speed(dwc2);
			/* Notify the higher logic about the new device */
			uhc_dwc2_submit_new_device(dev, speed);
		} else {
			/* TODO: Implement handling */
			LOG_ERR("Port changed during debouncing connect");
		}
	}

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_ENABLED)) {
		/* Port has a device connected and finish the first reset, we know the speed */
		LOG_DBG("Port enabled");
		/* Enable the rest of the port */
		enum uhc_dwc2_speed speed = dwc2_hal_get_speed(dwc2);

		dwc2_hal_enable_port(dwc2, speed);
	}

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_DISCONNECTION)) {
		/* Port disconnected */
		/* Debounce port disconnection */
		if (uhc_dwc2_port_debounce(dev, UHC_DWC2_EVENT_PORT_DISCONNECTION)) {
			LOG_DBG("Port disconnected");
			/* Notify upper layer */
			uhc_dwc2_submit_dev_gone(dev);
			/* Reset the controller to handle new connection */
			uhc_dwc2_soft_reset(dev);
			/* Prepare for device connection */
			uhc_dwc2_port_power_on(dev);
		} else {
			/* TODO: Implement handling */
			LOG_ERR("Port changed during debouncing disconnect");
		}
	}

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_ERROR)) {
		LOG_DBG("Port error");
		/* Notify upper layer */
		uhc_dwc2_submit_dev_gone(dev);
		/* TODO: recover from the error */

		/* Reset the controller to handle new connection */
		uhc_dwc2_soft_reset(dev);
		/* Prepare for device connection */
		uhc_dwc2_port_power_on(dev);
	}

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_OVERCURRENT)) {
		LOG_ERR("Port overcurrent");
		/* TODO: Handle overcurrent */
		LOG_WRN("Handle overcurrent is not implemented");
		/* Just power off the port via registers */
		dwc2_hal_set_power(dwc2, false);
	}
}

static void uhc_dwc2_channel_handle_events(const struct device *dev,
						struct uhc_dwc2_channel *channel)
{
	uint32_t events = (uint32_t)atomic_set(&channel->events, 0);
	struct uhc_transfer *const xfer = channel->xfer;
	int err = -EIO;

	__ASSERT_NO_MSG(xfer != NULL);

	LOG_DBG("Thread channel%d events: %08Xh", channel->index, events);

	if (events & BIT(UHC_DWC2_CHANNEL_EVENT_ERROR)) {
		LOG_ERR("Channel%d error", channel->index);
		LOG_WRN("Channel error handling has not been implemented yet");
		err = -EIO;
	} else if (events & BIT(UHC_DWC2_CHANNEL_EVENT_STALL)) {
		err = -EPIPE;
	} else if (events & BIT(UHC_DWC2_CHANNEL_EVENT_CPLT)) {
		if (channel->set_address) {
			/* When the device is processing new address, delay is required. */
			k_msleep(SET_ADDR_DELAY_MS);
		}

		err = 0;
	} else {
		LOG_WRN("Channel%d unhandled events %08Xh", channel->index, events);
	}

	if (events & BIT(UHC_DWC2_CHANNEL_DO_RELEASE)) {
		uhc_dwc2_channel_release(dev, channel);
		uhc_xfer_return(dev, xfer, err);
	}
}

static void uhc_dwc2_isr_handler(const struct device *dev)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct uhc_dwc2_channel *channel = NULL;
	enum uhc_dwc2_event port_event = uhc_dwc2_port_get_event(dev);
	uint32_t channel_events = 0;

	/* TODO: Simplify by merging event and channel handling */

	switch (port_event) {
	case UHC_DWC2_EVENT_NONE: {
		/* Port event occurred but should not be handled in higher logic */
		break;
	}
	case UHC_DWC2_EVENT_PORT_PEND_CHANNEL: {
		/* Handle pending channel event  */
		channel = uhc_dwc2_channel_get_pending(dev);
		while (channel != NULL) {
			channel_events = uhc_dwc2_channel_irq_handle_events(channel);
			if (channel_events) {
				/**
				 * The rest channel events be handled by the thread
				 */
				atomic_or(&channel->events, channel_events);
				k_event_set(&priv->event,
					BIT(UHC_DWC2_EVENT_PORT_PEND_CHANNEL + channel->index));
			}
			channel = uhc_dwc2_channel_get_pending(dev);
		}
		break;
	}
	default:
		/* Notify thread about port event */
		k_event_set(&priv->event, BIT(port_event));
		break;
	}

	(void) uhc_dwc2_quirk_irq_clear(dev);
}

static void uhc_dwc2_thread(void *arg0, void *arg1, void *arg2)
{
	const struct device *const dev = (const struct device *)arg0;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	uint32_t event_mask;

	while (true) {
		event_mask = k_event_wait_safe(&priv->event, UINT32_MAX, false, K_FOREVER);

		uhc_lock_internal(dev, K_FOREVER);

		/* Handle port events */
		uhc_dwc2_port_handle_events(dev, event_mask);

		/* Interate channels events */
		for (uint32_t index = 0; index < MAX_CHANNELS; index++) {
			if (event_mask & BIT(UHC_DWC2_EVENT_PORT_PEND_CHANNEL + index)) {
				uhc_dwc2_channel_handle_events(dev, &priv->channel[index]);
			}
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
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;
	int ret;

	/* Reset the port */
	dwc2_hal_set_reset(dwc2, true);

	/* Hold the bus in the reset state */
	k_msleep(RESET_HOLD_MS);

	/* Return the bus to the idle state. Port enabled event should occur */
	dwc2_hal_set_reset(dwc2, false);

	/* Give 10 ms to the port to become enabled */
	ret = k_sem_take(&priv->sem_port_enabled, K_MSEC(10));
	if (ret != 0) {
		/* Port didn't change its state to ENABLED */
		LOG_ERR("Port wasn't enabled after reset");
		return ret;
	}

	/* Give the device some extra time to recover after port reset */
	k_msleep(RESET_RECOVERY_MS);

	/* Finish the port config for the appeared device */
	/* TODO: set frame list for the ISOC/INTR xfer */
	/* TODO: enable periodic transfer */

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
	struct usb_dwc2_reg *const dwc2 = config->base;
	int ret;

	for (uint32_t idx = 0; idx < MAX_CHANNELS; idx++) {
		priv->channel[idx].index = idx;
		priv->channel[idx].regs =
			(struct usb_dwc2_host_chan *)((mem_addr_t)dwc2 + USB_DWC2_HCCHAR(idx));
	}

	k_thread_create(&priv->thread,
		config->stack,
		config->stack_size,
		uhc_dwc2_thread,
		(void *)dev, NULL, NULL,
		K_PRIO_COOP(CONFIG_UHC_DWC2_THREAD_PRIORITY),
		K_ESSENTIAL,
		K_NO_WAIT);
	k_thread_name_set(&priv->thread, dev->name);

	ret = uhc_dwc2_quirk_post_preinit(dev);
	if (ret != 0) {
		LOG_ERR("Quirk post preinit failed %d", ret);
		return ret;
	}

	return 0;
}

static int uhc_dwc2_init(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	uint32_t reg;
	int ret;

	ret = uhc_dwc2_quirk_pre_init(dev);
	if (ret != 0) {
		LOG_ERR("Quirk pre init failed %d", ret);
		return ret;
	}

	/* Read hardware configuration registers */

	reg = sys_read32((mem_addr_t)&dwc2->gsnpsid);
	if (reg != config->gsnpsid) {
		LOG_ERR("Unexpected GSNPSID 0x%08x instead of 0x%08x", reg, config->gsnpsid);
		return -ENOTSUP;
	}

	LOG_DBG("DWC2 Core %04x", usb_dwc2_get_gsnpsid_rev(reg));

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

	/* Core reset */

	ret = dwc2_core_soft_reset(dev);
	if (ret != 0) {
		LOG_ERR("DWC2 core reset failed after PHY init: %d", ret);
		return ret;
	}

	/* FIFO */

	ret = dwc2_hal_set_fifo_sizes(dwc2, config->ghwcfg2, config->ghwcfg3);
	if (ret != 0) {
		LOG_ERR("Unable to configure FIFO");
		return ret;
	}

	/* Init controller as a USB host */

	return dwc2_hal_init_host(dwc2);
}

static int uhc_dwc2_enable(const struct device *const dev)
{
	int ret;

	ret = uhc_dwc2_quirk_pre_enable(dev);
	if (ret != 0) {
		LOG_ERR("Quirk pre enable failed %d", ret);
		return ret;
	}

	uhc_dwc2_soft_reset(dev);

	uhc_dwc2_port_power_on(dev);

	ret = uhc_dwc2_quirk_post_enable(dev);
	if (ret != 0) {
		LOG_ERR("Quirk post enable failed %d", ret);
		return ret;
	}

	return 0;
}

static int uhc_dwc2_disable(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	int ret;
	/* TODO: Check ongoing transfer? */

	uhc_dwc2_submit_dev_gone(dev);

	config->irq_disable_func(dev);

	uhc_dwc2_port_power_off(dev);

	ret = uhc_dwc2_quirk_disable(dev);
	if (ret != 0) {
		LOG_ERR("Quirk disable failed %d", ret);
		return ret;
	}

	return 0;
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
			(DT_INST_REG_ADDR(n)),					\
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

/* Multi-instance device definition for DWC2 host controller */
#define UHC_DWC2_DEVICE_DEFINE(n)						\
										\
	K_THREAD_STACK_DEFINE(uhc_dwc2_stack_##n,				\
		CONFIG_UHC_DWC2_STACK_SIZE);					\
										\
	UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
										\
	static struct uhc_dwc2_data uhc_dwc2_priv_##n = {			\
		.event = Z_EVENT_INITIALIZER(uhc_dwc2_priv_##n.event),		\
		.sem_port_enabled =						\
			Z_SEM_INITIALIZER(uhc_dwc2_priv_##n.sem_port_enabled,	\
					  0,					\
					  1),					\
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
		.quirk_data = &uhc_dwc2_quirk_data_##n,				\
		.quirk_config = &uhc_dwc2_quirk_config_##n,			\
		.stack = uhc_dwc2_stack_##n,					\
		.stack_size = K_THREAD_STACK_SIZEOF(uhc_dwc2_stack_##n),	\
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
