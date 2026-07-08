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
#define MAX_CHANNELS		16

enum uhc_dwc2_event {
	/* A device has been connected to the port */
	UHC_DWC2_EVENT_PORT_CONNECTION,
	/* A device is connected to the port but has not been reset. */
	UHC_DWC2_EVENT_PORT_DISABLED,
	/* A device disconnection has been detected */
	UHC_DWC2_EVENT_PORT_DISCONNECTION,
	/* Port error detected */
	UHC_DWC2_EVENT_PORT_ERROR,
	/* Overcurrent detected */
	UHC_DWC2_EVENT_PORT_OVERCURRENT,
	/* A queued transfer has been marked for cancellation */
	UHC_DWC2_EVENT_DEQUEUE,
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
	/* Need to submit the transfer after channel error. Channel is now halted */
	UHC_DWC2_CHANNEL_DO_REINIT,
	/* Need to rewind the buffer being transmitted. Channel is now halted */
	UHC_DWC2_CHANNEL_DO_REWIND,
	/* The transfer was cancelled. Channel is now halted */
	UHC_DWC2_CHANNEL_EVENT_CANCELLED,
};

#define EPSIZE_BULK_FS			64U

/* Mask to clear HPRT register */
#define USB_DWC2_HPRT_W1C_MSK		(USB_DWC2_HPRT_PRTENA |			\
					 USB_DWC2_HPRT_PRTCONNDET |		\
					 USB_DWC2_HPRT_PRTENCHNG |		\
					 USB_DWC2_HPRT_PRTOVRCURRCHNG)

/* Mask of errors received for host channels (STALL excluded) */
#define USB_DWC2_HCINT_ERRORS		(USB_DWC2_HCINT_AHBERR |		\
					 USB_DWC2_HCINT_XACTERR |		\
					 USB_DWC2_HCINT_BBLERR |		\
					 USB_DWC2_HCINT_FRMOVRUN |		\
					 USB_DWC2_HCINT_DTGERR)

/* Mask of reason of transfer completion for host channels */
#define USB_DWC2_HCINT_CPLT_BITS	(USB_DWC2_HCINT_XFERCOMPL |		\
					 USB_DWC2_HCINT_STALL |			\
					 USB_DWC2_HCINT_BBLERR |		\
					 USB_DWC2_HCINT_XACTERR)

/* TODO: rename to pipe_data and expand with udev */
struct uhc_dwc2_channel_data {
	uint8_t next_pid;
};
struct uhc_dwc2_channel {
	/* Index of the channel */
	uint8_t index;
	/* Accessed in ISR. Number of error to track errors for transactions. */
	uint8_t error_count;
	/* Set while a halt was initiated to cancel the transfer. */
	bool halt_cancel;
	/* Used to save pending completion bits, while waiting channel to be halted. */
	uint32_t hcint_cplt_pending;
	/* Cached pointer to channel registers */
	const struct usb_dwc2_host_chan *regs;
	/* Pointer to the transfer */
	struct uhc_transfer *xfer;
	/* Channel events */
	atomic_t events;
	/* Channel transfer helpers */
	uint32_t length;
	/* Channel transfer data helpers */
	struct uhc_dwc2_channel_data *data;
};

struct uhc_dwc2_data {
	struct k_thread thread;
	/* Event bitmask the driver thread waits for */
	struct k_event events;
	/* Semaphore used to indicate that port is enabled */
	struct k_sem sem_port_enabled;
	/* Port channels */
	struct uhc_dwc2_channel ch[MAX_CHANNELS];
	/* Channels specific transfer related parameters */
	struct uhc_dwc2_channel_data ch_data[2][MAX_CHANNELS];
	/* Number of channels, available on the hardware */
	uint32_t numhstchnl;
	/* Number of channels currently not claimed */
	uint32_t free_chs;
	/* Root port does the debounce of connection/disconnection */
	bool debouncing;
	/* Root port has an attached device */
	bool has_device;
};

static inline uint32_t calc_packet_count(const uint32_t size, const uint16_t mps)
{
	if (size == 0) {
		return 1; /* in Buffer DMA mode Zero Length Packet still counts as 1 packet */
	} else {
		return DIV_ROUND_UP(size, mps);
	}
}

static inline uint8_t calc_next_pid(const uint8_t pid, const uint32_t pkt_cnt)
{
	/* If amount of packets are even - do not toggle */
	if ((pkt_cnt & BIT(0)) == 0) {
		return pid;
	}

	return (pid == USB_DWC2_HCTSIZ_PID_DATA0) ?
		USB_DWC2_HCTSIZ_PID_DATA1 :
		USB_DWC2_HCTSIZ_PID_DATA0;
}

static inline void dwc2_set_reset(struct usb_dwc2_reg *const base, const bool reset)
{
	uint32_t hprt;

	hprt = sys_read32((mem_addr_t)&base->hprt);
	if (reset) {
		hprt |= USB_DWC2_HPRT_PRTRST;
	} else {
		hprt &= ~USB_DWC2_HPRT_PRTRST;
	}
	sys_write32(hprt & (~USB_DWC2_HPRT_W1C_MSK), (mem_addr_t)&base->hprt);
}

static inline void dwc2_set_power(struct usb_dwc2_reg *const base, const bool power)
{
	uint32_t hprt;

	hprt = sys_read32((mem_addr_t)&base->hprt);
	if (power) {
		hprt |= USB_DWC2_HPRT_PRTPWR;
	} else {
		hprt &= ~USB_DWC2_HPRT_PRTPWR;
	}
	sys_write32(hprt & (~USB_DWC2_HPRT_W1C_MSK), (mem_addr_t)&base->hprt);
}

static int dwc2_flush_rx_fifo(struct usb_dwc2_reg *const base)
{
	const k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(100));

	sys_write32(USB_DWC2_GRSTCTL_RXFFLSH, (mem_addr_t)&base->grstctl);

	while (sys_read32((mem_addr_t)&base->grstctl) & USB_DWC2_GRSTCTL_RXFFLSH) {
		k_busy_wait(1);

		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for RXFFLSH timeout");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int dwc2_flush_tx_fifo(struct usb_dwc2_reg *const base, const uint8_t fnum)
{
	const k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(100));
	uint32_t grstctl;

	grstctl = usb_dwc2_set_grstctl_txfnum(fnum) | USB_DWC2_GRSTCTL_TXFFLSH;
	sys_write32(grstctl, (mem_addr_t)&base->grstctl);

	while (sys_read32((mem_addr_t)&base->grstctl) & USB_DWC2_GRSTCTL_TXFFLSH) {
		k_busy_wait(1);

		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for TXFFLSH timeout");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static void dwc2_config_timings(struct usb_dwc2_reg *const base)
{
	uint32_t gusbcfg = sys_read32((mem_addr_t)&base->gusbcfg);
	uint32_t ghwcfg2 = sys_read32((mem_addr_t)&base->ghwcfg2);
	uint32_t hcfg = sys_read32((mem_addr_t)&base->hcfg);
	uint32_t hfir = sys_read32((mem_addr_t)&base->hfir);
	uint32_t hprt = sys_read32((mem_addr_t)&base->hprt);
	uint32_t phy_clock_mhz = 0;
	uint32_t fslspclk = 0;
	uint32_t frint = 0;
	bool hs_phy = (usb_dwc2_get_ghwcfg2_hsphytype(ghwcfg2) != USB_DWC2_GHWCFG2_HSPHYTYPE_NO_HS);

	if (hs_phy) {
		fslspclk = USB_DWC2_HCFG_FSLSPCLKSEL_CLK3060;
		if (gusbcfg & USB_DWC2_GUSBCFG_PHYIF_16_BIT) {
			phy_clock_mhz = 30U;
		} else {
			phy_clock_mhz = 60U;
		}
	} else {
		fslspclk = USB_DWC2_HCFG_FSLSPCLKSEL_CLK48;
		phy_clock_mhz = 48U;
	}

	switch (usb_dwc2_get_hprt_prtspd(hprt)) {
	case USB_DWC2_HPRT_PRTSPD_HIGH:
		frint = 125U * phy_clock_mhz - 1;
		break;
	case USB_DWC2_HPRT_PRTSPD_FULL:
		frint = 1000U * phy_clock_mhz - 1U;
		break;
	case USB_DWC2_HPRT_PRTSPD_LOW:
		if (hs_phy) {
			gusbcfg |= USB_DWC2_GUSBCFG_PHYLPWRCLKSEL_LP;
			sys_write32(gusbcfg, (mem_addr_t)&base->gusbcfg);

			frint = 1000U * phy_clock_mhz - 1U;
		} else {
			fslspclk = USB_DWC2_HCFG_FSLSPCLKSEL_CLK6;
			frint = 1000U * 6U - 1U;
		}
		break;
	default:
		LOG_WRN("Port has unexpected speed, configure FrInt for full speed");
		frint = 1000U * phy_clock_mhz - 1U;
		break;
	}

	hcfg &= ~USB_DWC2_HCFG_FSLSPCLKSEL_MASK;
	hcfg |= fslspclk << USB_DWC2_HCFG_FSLSPCLKSEL_POS;

	hfir &= ~USB_DWC2_HFIR_FRINT_MASK;
	hfir |= usb_dwc2_set_hfir_frint(frint);

	sys_write32(hcfg, (mem_addr_t)&base->hcfg);
	sys_write32(hfir, (mem_addr_t)&base->hfir);

	LOG_DBG("Timings: speed=%u, phy_clk=%uMHz, fslspclk=%u, frint=%u",
		usb_dwc2_get_hprt_prtspd(hprt),
		phy_clock_mhz,
		fslspclk,
		frint);
}

static inline int dwc2_core_init_host_gusbcfg(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const data = uhc_get_private(dev);
	struct usb_dwc2_reg *const base = config->base;
	uint32_t gusbcfg = sys_read32((mem_addr_t)&base->gusbcfg);
	uint32_t ghwcfg2 = sys_read32((mem_addr_t)&base->ghwcfg2);
	uint32_t ghwcfg4 = sys_read32((mem_addr_t)&base->ghwcfg4);
	const k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(100));

	/* Enable Host mode */
	sys_set_bits((mem_addr_t)&base->gusbcfg, USB_DWC2_GUSBCFG_FORCEHSTMODE);

	/* Wait until core is in host mode */
	while ((sys_read32((mem_addr_t)&base->gintsts) & USB_DWC2_GINTSTS_CURMOD) == 0) {
		k_busy_wait(1);

		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for Host mode timeout, GINTSTS 0x%08x",
				sys_read32((mem_addr_t)&base->gintsts));
			return -ETIMEDOUT;
		}
	}

	/* Store the actual number of channels */
	data->numhstchnl = usb_dwc2_get_ghwcfg2_numhstchnl(ghwcfg2) + 1;
	LOG_DBG("Number of Host Channels %u", data->numhstchnl);
	if (data->numhstchnl > MAX_CHANNELS) {
		return -EIO;
	}

	if (usb_dwc2_get_ghwcfg2_hsphytype(ghwcfg2) == USB_DWC2_GHWCFG2_HSPHYTYPE_NO_HS) {
		/* Select FS PHY */
		LOG_DBG("Fullspeed PHY init");
		sys_set_bits((mem_addr_t)&base->gusbcfg, USB_DWC2_GUSBCFG_PHYSEL_USB11);
	} else {
		/* Deselect FS PHY */
		gusbcfg &= ~USB_DWC2_GUSBCFG_PHYSEL_USB11;

		if (usb_dwc2_get_ghwcfg2_hsphytype(ghwcfg2) == USB_DWC2_GHWCFG2_HSPHYTYPE_ULPI) {
			LOG_DBG("Highspeed ULPI PHY init");
			/* Select ULPI PHY */
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
			/* Select UTMI+ PHY */
			gusbcfg &= ~USB_DWC2_GUSBCFG_ULPI_UTMI_SEL_ULPI;
			/* Set 16-bit interface if supported */
			if (usb_dwc2_get_ghwcfg4_phydatawidth(ghwcfg4)) {
				gusbcfg |= USB_DWC2_GUSBCFG_PHYIF_16_BIT;
			} else {
				gusbcfg &= ~USB_DWC2_GUSBCFG_PHYIF_16_BIT;
			}
		}
		sys_write32(gusbcfg, (mem_addr_t)&base->gusbcfg);
	}

	return 0;
}

static inline int dwc2_core_init_gahbcfg(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	uint32_t ghwcfg2;
	uint32_t gahbcfg;

	/* Disable Global Interrupt */
	sys_clear_bits((mem_addr_t)&base->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);

	/* Configure AHB */
	gahbcfg = sys_read32((mem_addr_t)&base->gahbcfg);
	gahbcfg &= ~USB_DWC2_GAHBCFG_HBSTLEN_MASK;
	gahbcfg |= usb_dwc2_set_gahbcfg_hbstlen(USB_DWC2_GAHBCFG_HBSTLEN_INCR16);
	sys_write32(gahbcfg, (mem_addr_t)&base->gahbcfg);

	ghwcfg2 = sys_read32((mem_addr_t)&base->ghwcfg2);
	if (usb_dwc2_get_ghwcfg2_otgarch(ghwcfg2) == USB_DWC2_GHWCFG2_OTGARCH_INTERNALDMA) {
		sys_set_bits((mem_addr_t)&base->gahbcfg, USB_DWC2_GAHBCFG_DMAEN);
	} else {
		/* TODO: Implement Slave mode */
		LOG_ERR("DMA isn't supported by the hardware");
		return -ENXIO;
	}

	/* Enable Global Interrupt */
	sys_set_bits((mem_addr_t)&base->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);
	return 0;
}

static int dwc2_core_soft_reset(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	mem_addr_t grstctl_reg = (mem_addr_t)&base->grstctl;
	mem_addr_t gsnpsid_reg = (mem_addr_t)&base->gsnpsid;
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

		/* Hardware requires at least 3 PHY clocks after CSFTRST clears. */
		k_busy_wait(1);
	} else {
		/*
		 * From v4.20a, CSFTRST is write-only/reset-controlled.
		 * Wait for CSFTRSTDONE, then clear it.
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

		/* Clear CSFTRST, and write 1 to CSFTRSTDONE to clear the done indication. */
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

static int dwc2_set_fifo_sizes(struct usb_dwc2_reg *const base)
{
	const uint32_t ghwcfg2 = sys_read32((mem_addr_t)&base->ghwcfg2);
	const uint32_t ghwcfg3 = sys_read32((mem_addr_t)&base->ghwcfg3);
	/* TODO: Check the FIFO setting on hardware, that supports HS */
	const uint32_t nptx_largest = EPSIZE_BULK_FS / 4;
	const uint32_t ptx_largest = 256 / 4;
	const uint32_t dfifodepth = FIELD_GET(USB_DWC2_GHWCFG3_DFIFODEPTH_MASK, ghwcfg3);
	const uint32_t numhstchnl = FIELD_GET(USB_DWC2_GHWCFG2_NUMHSTCHNL_MASK, ghwcfg2);
	uint32_t fifo_available = dfifodepth - (numhstchnl + 1);
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
	sys_write32(gdfifocfg, (mem_addr_t)&base->gdfifocfg);

	/* RX FIFO size */
	fifo_available -= fifo_rxfsiz;
	grxfsiz = usb_dwc2_set_grxfsiz(fifo_rxfsiz);
	sys_write32(grxfsiz, (mem_addr_t)&base->grxfsiz);

	/* Non-periodic TX FIFO size */
	fifo_available -= fifo_nptxfdep;
	gnptxfsiz = usb_dwc2_set_gnptxfsiz_nptxfdep(fifo_nptxfdep);
	gnptxfsiz |= usb_dwc2_set_gnptxfsiz_nptxfstaddr(fifo_available);
	sys_write32(gnptxfsiz, (mem_addr_t)&base->gnptxfsiz);

	/* Periodic TX FIFO size */
	fifo_available -= fifo_ptxfsiz;
	hptxfsiz = usb_dwc2_set_hptxfsiz_ptxfsize(fifo_ptxfsiz);
	hptxfsiz |= fifo_available;
	sys_write32(hptxfsiz, (mem_addr_t)&base->hptxfsiz);

	/* Flush TX and RX FIFOs */
	ret = dwc2_flush_tx_fifo(base, MAX_CHANNELS);
	if (ret != 0) {
		return ret;
	}

	return dwc2_flush_rx_fifo(base);
}

static void port_debounce_lock(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);

	/* Disable connection and disconnection interrupts to prevent spurious events */
	sys_clear_bits((mem_addr_t)&base->gintmsk, USB_DWC2_GINTSTS_PRTINT |
						USB_DWC2_GINTSTS_DISCONNINT);
	/* Set the debounce lock flag */
	priv->debouncing = true;
}

static void port_debounce_unlock(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);

	/* Clear the flag */
	priv->debouncing = false;
	/* Clear Connection and disconnection interrupt in case it triggered again */
	sys_set_bits((mem_addr_t)&base->gintsts, USB_DWC2_GINTSTS_DISCONNINT);
	/* Clear the PRTCONNDET interrupt by writing 1 to the corresponding bit (W1C logic) */
	sys_set_bits((mem_addr_t)&base->hprt, USB_DWC2_HPRT_PRTCONNDET);
	/* Re-enable the HPRT (connection) and disconnection interrupts */
	sys_set_bits((mem_addr_t)&base->gintmsk, USB_DWC2_GINTSTS_PRTINT |
						USB_DWC2_GINTSTS_DISCONNINT);
}

static int port_reset(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const base = config->base;
	int ret;

	/* Reset the port */
	dwc2_set_reset(base, true);

	/* Hold the bus in the reset state */
	uhc_unlock_internal(dev);

	k_msleep(RESET_HOLD_MS);

	uhc_lock_internal(dev, K_FOREVER);

	/* Return the bus to the idle state. Port enabled event should occur */
	dwc2_set_reset(base, false);

	/* Give 10 ms to the port to become enabled */
	ret = k_sem_take(&priv->sem_port_enabled, K_MSEC(10));
	if (ret != 0) {
		/* Port didn't change its state to ENABLED */
		LOG_ERR("Port wasn't enabled after reset");
		return ret;
	}

	/* Give the device some extra time to recover after port reset */
	uhc_unlock_internal(dev);

	k_msleep(RESET_RECOVERY_MS);

	uhc_lock_internal(dev, K_FOREVER);

	/* FIFO */
	ret = dwc2_set_fifo_sizes(base);
	if (ret != 0) {
		LOG_ERR("Unable to set FIFO sizes");
		return ret;
	}

	/* TODO: set frame list for the ISOC/INTR xfer */
	/* TODO: enable periodic transfer */

	return 0;
}

static inline void ch_process_control(struct uhc_dwc2_channel *ch)
{
	struct uhc_transfer *const xfer = ch->xfer;
	const struct usb_setup_packet *setup = (const struct usb_setup_packet *)xfer->setup_pkt;
	bool next_dir_is_in;
	uint16_t size = 0;
	uint32_t pkt_cnt;
	uint8_t *dma_addr = NULL;
	uint32_t hcchar;
	uint32_t hctsiz;
	uint16_t remaining;
	uint16_t actual_len;

	if (xfer->stage == UHC_CONTROL_STAGE_SETUP) {
		/* Just finished UHC_CONTROL_STAGE_SETUP */
		if (setup->wLength == 0) {
			/* No data stage. Go strait to status */
			next_dir_is_in = true;
			xfer->stage = UHC_CONTROL_STAGE_STATUS;
		} else {
			/* Data stage is present, go to data stage */
			next_dir_is_in = usb_reqtype_is_to_host(setup);
			xfer->stage = UHC_CONTROL_STAGE_DATA;

			/*
			 * NOTE: Sizes
			 * for IN - wLength and net_buf_tailroom(xfer->buf)
			 * for OUT - xfer->buf->len
			 */
			if (next_dir_is_in) {
				size = sys_le16_to_cpu(setup->wLength);

				LOG_DBG("Control DATA IN prog=%u, tailroom=%u",
					size, net_buf_tailroom(xfer->buf));

				dma_addr = net_buf_tail(xfer->buf);
			} else {
				size = xfer->buf->len;

				LOG_DBG("Control DATA OUT len=%u, tailroom=%u",
					xfer->buf->len, net_buf_tailroom(xfer->buf));

				LOG_HEXDUMP_DBG(xfer->buf->data, xfer->buf->len,
						"Control DATA OUT:");

				dma_addr = xfer->buf->data;
			}
		}
	} else {
		/* Finished UHC_CONTROL_STAGE_DATA */
		hctsiz = sys_read32((mem_addr_t)&ch->regs->hctsiz);
		remaining = usb_dwc2_get_hctsiz_xfersize(hctsiz);
		actual_len = ch->length - remaining;

		if (usb_reqtype_is_to_host(setup)) {
			net_buf_add(xfer->buf, actual_len);

			LOG_DBG("Control DATA IN completed, prog=%u, rem=%u, act=%u, tailroom=%u",
				ch->length,
				remaining,
				actual_len,
				net_buf_tailroom(xfer->buf));

			LOG_HEXDUMP_DBG(xfer->buf->data, xfer->buf->len, "Control DATA IN:");
		} else {
			LOG_DBG("Control DATA OUT completed, prog=%u, rem=%u, act=%u",
				ch->length, remaining, actual_len);
		}
		/* Status stage is always the opposite direction of data stage */
		next_dir_is_in = !usb_reqtype_is_to_host(setup);
		xfer->stage = UHC_CONTROL_STAGE_STATUS;
	}

	/* Calculate new packet count */
	pkt_cnt  = calc_packet_count(size, xfer->mps);

	if (next_dir_is_in) {
		sys_set_bits((mem_addr_t)&ch->regs->hcchar, USB_DWC2_HCCHAR_EPDIR);
	} else {
		sys_clear_bits((mem_addr_t)&ch->regs->hcchar, USB_DWC2_HCCHAR_EPDIR);
	}

	hctsiz = usb_dwc2_set_hctsiz_pid(USB_DWC2_HCTSIZ_PID_DATA1) |
		usb_dwc2_set_hctsiz_pktcnt(pkt_cnt) |
		usb_dwc2_set_hctsiz_xfersize(size);

	sys_write32(hctsiz, (mem_addr_t)&ch->regs->hctsiz);
	sys_write32((uint32_t) dma_addr, (mem_addr_t)&ch->regs->hcdma);

	/* TODO: Configure split transaction if needed */

	/* TODO: sync CACHE */

	hcchar = sys_read32((mem_addr_t)&ch->regs->hcchar);
	hcchar |= USB_DWC2_HCCHAR_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&ch->regs->hcchar);
}

static bool xfer_is_done(const struct uhc_transfer *xfer)
{
	return xfer->type != USB_EP_TYPE_CONTROL ||
	       xfer->stage == UHC_CONTROL_STAGE_STATUS;
}

static uint32_t ch_handle_xfer_complete(struct uhc_dwc2_channel *ch, uint32_t ch_events)
{
	if ((ch_events & BIT(UHC_DWC2_CHANNEL_EVENT_CPLT)) && !xfer_is_done(ch->xfer)) {
		ch_process_control(ch);
		return 0;
	}

	return ch_events;
}

static uint32_t ch_handle_in_bulk_control(struct uhc_dwc2_channel *ch, uint32_t hcint)
{
	uint32_t ch_events = 0;

	if (hcint & USB_DWC2_HCINT_CHHLTD) {
		if (hcint & (USB_DWC2_HCINT_XFERCOMPL | USB_DWC2_HCINT_STALL |
			     USB_DWC2_HCINT_BBLERR)) {
			/* Transfer completed, STALLed or Babble Error */
			ch->error_count = 0;
			/* TODO: Mask ACK */

			if (hcint & USB_DWC2_HCINT_XFERCOMPL) {
				ch_events |= BIT(UHC_DWC2_CHANNEL_EVENT_CPLT);
			} else if (hcint & USB_DWC2_HCINT_STALL) {
				ch_events |= BIT(UHC_DWC2_CHANNEL_EVENT_STALL);
			} else {
				ch_events |= BIT(UHC_DWC2_CHANNEL_EVENT_ERROR);
			}
			ch_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
		} else if (hcint & USB_DWC2_HCINT_XACTERR) {
			ch->error_count++;
			if (ch->error_count >= 3) {
				LOG_ERR("IN channel%d error retry limit, HCINT 0x%08x",
					ch->index, hcint);
				ch_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
				ch_events |= BIT(UHC_DWC2_CHANNEL_EVENT_ERROR);
			} else {
				/* TODO: Unmask ACK, NAK, DTGERR */
				LOG_DBG("IN channel%d error, HCINT 0x%08x, retry %u",
					ch->index, hcint, ch->error_count);
				ch_events |= BIT(UHC_DWC2_CHANNEL_DO_REINIT);
			}
		} else {
			/* TODO: Add handling for other cases */
			LOG_WRN("IN halted, unhandled HCINT 0x%08x", hcint);
		}
	} else if (hcint & (USB_DWC2_HCINT_ACK | USB_DWC2_HCINT_NAK | USB_DWC2_HCINT_DTGERR)) {
		ch->error_count = 0;
		/* TODO: Mask ACK, NAK, DTGERR */
		LOG_WRN("ACK, NAK or DTG Error");
	} else {
		/* TODO: Add handling for other cases */
		LOG_WRN("IN not halted, unhandled HCINT 0x%08x", hcint);
	}

	return ch_handle_xfer_complete(ch, ch_events);
}

static inline uint32_t ch_handle_out_bulk_control(struct uhc_dwc2_channel *ch, uint32_t hcint)
{
	uint32_t ch_events = 0;

	if (hcint & USB_DWC2_HCINT_CHHLTD) {
		if (hcint & (USB_DWC2_HCINT_XFERCOMPL | USB_DWC2_HCINT_STALL)) {
			/* Transfer completed or STALLed*/
			ch->error_count = 1;
			/* TODO: Mask ACK */

			if (hcint & USB_DWC2_HCINT_XFERCOMPL) {
				ch_events |= BIT(UHC_DWC2_CHANNEL_EVENT_CPLT);
			} else {
				ch_events |= BIT(UHC_DWC2_CHANNEL_EVENT_STALL);
			}
			ch_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
		} else if (hcint & USB_DWC2_HCINT_XACTERR) {
			/* Transfer Error */
			if (hcint & (USB_DWC2_HCINT_NAK | USB_DWC2_HCINT_NYET |
				     USB_DWC2_HCINT_ACK)) {
				ch->error_count = 1;
				LOG_DBG("OUT channel%d NAK/NYET/ACK, HCINT 0x%08x, reint/rewind",
						ch->index, hcint);
				ch_events |= BIT(UHC_DWC2_CHANNEL_DO_REINIT);
				ch_events |= BIT(UHC_DWC2_CHANNEL_DO_REWIND);
			} else {
				ch->error_count++;
				if (ch->error_count >= 3) {
					LOG_ERR("OUT channel%d error retry limit, HCINT 0x%08x",
						ch->index, hcint);
					ch_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
					ch_events |= BIT(UHC_DWC2_CHANNEL_EVENT_ERROR);
				} else {
					LOG_DBG("OUT channel%d error, HCINT 0x%08x, rewind %u",
						ch->index, hcint, ch->error_count);
					ch_events |= BIT(UHC_DWC2_CHANNEL_DO_REINIT);
					ch_events |= BIT(UHC_DWC2_CHANNEL_DO_REWIND);
				}
			}
		} else {
			/* TODO: Add handling for other cases */
			LOG_WRN("OUT halted, unhandled HCINT 0x%08x", hcint);
		}
	} else if (hcint & USB_DWC2_HCINT_ACK) {
		ch->error_count = 1;
		/* TODO: Unmask ACK */
		LOG_WRN("OUT Acked");
	} else {
		/* TODO: Expand handling */
		LOG_WRN("OUT not halted, unhandled HCINT 0x%08x", hcint);
	}

	return ch_handle_xfer_complete(ch, ch_events);
}

static uint32_t ch_handle_irq_events(struct uhc_dwc2_channel *ch)
{
	struct uhc_transfer *const xfer = ch->xfer;
	uint32_t ch_events;
	uint32_t hcint;

	hcint = sys_read32((mem_addr_t)&ch->regs->hcint);
	sys_write32(hcint, (mem_addr_t)&ch->regs->hcint);

	LOG_DBG("HCINT 0x%08x", hcint);

	if (ch->halt_cancel) {
		/* A halt was initiated to cancel this transfer. */
		if (hcint & USB_DWC2_HCINT_CHHLTD) {
			return BIT(UHC_DWC2_CHANNEL_EVENT_CANCELLED) |
			       BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
		}

		return 0U;
	}

	if ((hcint & USB_DWC2_HCINT_CPLT_BITS) != 0U ||
	    ch->hcint_cplt_pending != 0) {
		/*
		 * Completion bits might come earlier, than channel halted interrupt.
		 * Save the hcint complete mask as pending and wait for channel halted.
		 */
		if ((hcint & USB_DWC2_HCINT_CHHLTD) == 0U) {
			/* Channel not halted yet, wait for channel halted */
			ch->hcint_cplt_pending |= hcint & USB_DWC2_HCINT_CPLT_BITS;
			return 0U;
		}

		/* Channel halted, restore pending bits */
		hcint |= ch->hcint_cplt_pending;
		ch->hcint_cplt_pending = 0U;
	}

	if (xfer->type == USB_EP_TYPE_BULK || xfer->type == USB_EP_TYPE_CONTROL) {
		/* Bulk & Control */
		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			ch_events = ch_handle_in_bulk_control(ch, hcint);
		} else {
			ch_events = ch_handle_out_bulk_control(ch, hcint);
		}
	} else {
		LOG_ERR("Unhandled transfer type %u, HCINT 0x%08x", xfer->type, hcint);
		ch_events = 0;
	}

	return ch_events;
}

static inline bool port_debounce(const struct device *dev, const enum uhc_dwc2_event event)
{
	const bool want_connected = (event == UHC_DWC2_EVENT_PORT_CONNECTION);
	const struct uhc_dwc2_config *config = dev->config;
	struct usb_dwc2_reg *base = config->base;
	bool connected;

	/* Perform the debounce delay outside of the global lock */
	uhc_unlock_internal(dev);

	k_msleep(DEBOUNCE_DELAY_MS);

	uhc_lock_internal(dev, K_FOREVER);

	connected = ((sys_read32((mem_addr_t)&base->hprt) & USB_DWC2_HPRT_PRTCONNSTS) != 0);
	port_debounce_unlock(dev);

	/* True if stable state matches the event */
	return connected == want_connected;
}

static inline void port_enable(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;

	sys_clear_bits((mem_addr_t)&base->haintmsk, 0xFFFFFFFFUL);
	sys_set_bits((mem_addr_t)&base->gintmsk, USB_DWC2_GINTSTS_PRTINT |
							USB_DWC2_GINTSTS_HCHINT);
	dwc2_set_power(base, true);
}

static inline void port_disable(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;

	sys_clear_bits((mem_addr_t)&base->haintmsk, 0xFFFFFFFFUL);
	sys_clear_bits((mem_addr_t)&base->gintmsk, USB_DWC2_GINTSTS_PRTINT |
							USB_DWC2_GINTSTS_HCHINT);
	dwc2_set_power(base, false);
}

static inline int soft_reset(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
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
	dwc2_core_init_gahbcfg(dev);

	/* Enable Global IRQ */
	config->irq_enable_func(dev);

	return ret;
}

static int ch_claim(const struct device *const dev,
		    struct uhc_transfer *const xfer,
		    struct uhc_dwc2_channel **const ch_p)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	uint8_t ep_dir_idx = USB_EP_DIR_IS_IN(xfer->ep) ? 1 : 0;
	uint8_t ep_num = USB_EP_GET_IDX(xfer->ep);
	struct uhc_dwc2_channel *ch = NULL;

	/*
	 * A non-NULL xfer marks the channel as busy. Scan all channels:
	 * remember the first free one, but only after making sure the endpoint
	 * is not already in flight.
	 */
	for (uint8_t idx = 0; idx < priv->numhstchnl; idx++) {
		struct uhc_dwc2_channel *const cand = &priv->ch[idx];

		if (cand->xfer == NULL) {
			if (ch == NULL) {
				ch = cand;
			}
			continue;
		}

		if (cand->xfer->udev->addr == xfer->udev->addr &&
		    cand->xfer->ep == xfer->ep) {
			LOG_DBG("Endpoint 0x%02x on addr %u already busy",
				xfer->ep, xfer->udev->addr);
			return -EBUSY;
		}
	}

	if (ch == NULL) {
		LOG_DBG("No free channel for ep 0x%02x", xfer->ep);
		return -EBUSY;
	}

	LOG_DBG("Claimed channel%d for ep 0x%02x, xfer=%p, channel=%p",
		ch->index, xfer->ep, (void *)xfer, (void *)ch);

	/* Save channel characteristics of the underlying channel */
	ch->xfer = xfer;
	ch->data = &priv->ch_data[ep_dir_idx][ep_num];
	priv->free_chs--;

	*ch_p = ch;

	return 0;
}

static int ch_configure(const struct device *const dev, struct uhc_dwc2_channel *const ch)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	struct uhc_transfer *const xfer = ch->xfer;
	struct usb_device *const udev = xfer->udev;
	uint32_t hcint;
	uint32_t hcintmsk;
	uint32_t hcchar;

	/* Clear the interrupt bits by writing them back */
	hcint = sys_read32((mem_addr_t)&ch->regs->hcint);
	sys_write32(hcint, (mem_addr_t)&ch->regs->hcint);

	/* Enable channel interrupt in the core */
	sys_set_bits((mem_addr_t)&base->haintmsk, BIT(ch->index));

	hcintmsk = sys_read32((mem_addr_t)&ch->regs->hcintmsk);

	/* Enable transfer complete and channel halted interrupts */
	hcintmsk |= USB_DWC2_HCINT_XFERCOMPL;
	hcintmsk |= USB_DWC2_HCINT_CHHLTD;
	/* Enable error interrupts */
	hcintmsk |= USB_DWC2_HCINT_ERRORS;
	hcintmsk |= USB_DWC2_HCINT_STALL;
	sys_write32(hcintmsk, (mem_addr_t)&ch->regs->hcintmsk);

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

	sys_write32(hcchar, (mem_addr_t)&ch->regs->hcchar);

	return 0;
}

static void ch_complete_bulk(const struct device *dev, struct uhc_dwc2_channel *const ch)
{
	struct uhc_transfer *const xfer = ch->xfer;
	uint32_t actual_len = ch->length;
	uint32_t pkt_cnt;

	/* Add net buffer after IN stage */
	if (USB_EP_DIR_IS_IN(xfer->ep)) {
		uint32_t hctsiz = sys_read32((mem_addr_t)&ch->regs->hctsiz);
		uint16_t remaining = usb_dwc2_get_hctsiz_xfersize(hctsiz);

		/* Device may send a short packet, use the actual length */
		actual_len = ch->length - remaining;
		net_buf_add(xfer->buf, actual_len);
	}

	/* Precalculate next pid based on the packets actually transferred */
	pkt_cnt = calc_packet_count(actual_len, xfer->mps);
	ch->data->next_pid = calc_next_pid(ch->data->next_pid, pkt_cnt);

	LOG_DBG("Release channel%u, prog=%u, act=%u, len=%u, mps=%u, next_pid=%u",
		ch->index, ch->length, actual_len, xfer->buf->len,
		xfer->mps, ch->data->next_pid);
}

static void ch_complete(const struct device *dev, struct uhc_dwc2_channel *ch)
{
	struct uhc_transfer *const xfer = ch->xfer;
	const struct usb_setup_packet *setup;

	switch (xfer->type) {
	case USB_EP_TYPE_CONTROL:
		/* Apply a delay if request was Set Address */
		setup = (const struct usb_setup_packet *)xfer->setup_pkt;
		if (setup->bRequest == USB_SREQ_SET_ADDRESS) {
			k_msleep(SET_ADDR_DELAY_MS);
		}
		break;
	case USB_EP_TYPE_BULK:
		ch_complete_bulk(dev, ch);
		break;
	default:
		/* Nothing special before release */
		break;
	}
}

static void ch_release(const struct device *dev, struct uhc_dwc2_channel *ch)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const base = config->base;

	sys_clear_bits((mem_addr_t)&base->haintmsk, (1 << ch->index));

	/* Release channel */
	ch->xfer = NULL;
	ch->data = NULL;
	ch->halt_cancel = false;
	priv->free_chs++;

	LOG_DBG("Released channel%d", ch->index);
}

/*
 * Initiate halting an enabled channel to cancel its transfer.
 *
 * Returns true if a halt was started and the CHHLTD interrupt is expected, or
 * false if the channel was not running and the caller must finish the
 * cancellation itself.
 */
static bool ch_halt_initiate(struct uhc_dwc2_channel *ch)
{
	uint32_t hcchar;

	/* Halt already initiated for this channel */
	if (ch->halt_cancel) {
		return true;
	}

	/*
	 * Leave only Channel Halted enabled and clear any other pending
	 * channel interrupt, so a cancelled transfer's status bits cannot take
	 * effect.
	 */
	sys_write32(USB_DWC2_HCINT_CHHLTD, (mem_addr_t)&ch->regs->hcintmsk);
	sys_write32((uint32_t)~USB_DWC2_HCINT_CHHLTD, (mem_addr_t)&ch->regs->hcint);

	hcchar = sys_read32((mem_addr_t)&ch->regs->hcchar);
	if (!(hcchar & USB_DWC2_HCCHAR_CHENA)) {
		/* Channel is not running, no Channel Halted interrupt will be raised */
		return false;
	}

	ch->halt_cancel = true;

	hcchar |= USB_DWC2_HCCHAR_CHENA | USB_DWC2_HCCHAR_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&ch->regs->hcchar);

	return true;
}

static void ch_start_control(struct uhc_dwc2_channel *ch)
{
	struct uhc_transfer *const xfer = ch->xfer;
	const struct usb_setup_packet *setup = (const struct usb_setup_packet *)xfer->setup_pkt;
	uint32_t pkt_cnt;
	uint32_t hctsiz;
	uint32_t hcint;
	uint32_t hcchar;

	xfer->stage = UHC_CONTROL_STAGE_SETUP;

	/* Save information about control transfer by analyzing the setup packet */
	ch->length = setup->wLength;

	/* Control stage is always OUT */
	sys_clear_bits((mem_addr_t)&ch->regs->hcchar, USB_DWC2_HCCHAR_EPDIR);

	pkt_cnt  = calc_packet_count(sizeof(struct usb_setup_packet), xfer->mps);
	hctsiz = usb_dwc2_set_hctsiz_pid(USB_DWC2_HCTSIZ_PID_SETUP) |
		usb_dwc2_set_hctsiz_pktcnt(pkt_cnt) |
		usb_dwc2_set_hctsiz_xfersize(sizeof(struct usb_setup_packet));

	LOG_HEXDUMP_DBG(setup, 8, "SETUP");

	sys_write32(hctsiz, (mem_addr_t)&ch->regs->hctsiz);
	sys_write32((uint32_t)xfer->setup_pkt, (mem_addr_t)&ch->regs->hcdma);

	/* TODO: Do Split */

	hcint = sys_read32((mem_addr_t)&ch->regs->hcint);
	sys_write32(hcint, (mem_addr_t)&ch->regs->hcint);

	/* TODO: Sync CACHE */

	/* Start transfer */
	hcchar = sys_read32((mem_addr_t)&ch->regs->hcchar);
	hcchar |= USB_DWC2_HCCHAR_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&ch->regs->hcchar);
}

static void ch_start_bulk(struct uhc_dwc2_channel *ch)
{
	struct uhc_transfer *const xfer = ch->xfer;
	uint8_t *dma_addr;
	uint32_t pkt_cnt;
	uint32_t hctsiz;
	uint32_t hcchar;

	/* TODO: Do split */

	if (USB_EP_DIR_IS_IN(xfer->ep)) {
		/* For IN, receive into the buffer tailroom */
		ch->length = net_buf_tailroom(xfer->buf);
		dma_addr = net_buf_tail(xfer->buf);

		LOG_DBG("BULK IN, tailroom=%u", ch->length);
	} else {
		/* For Out, data size is the size of the data in buffer */
		ch->length = xfer->buf->len;
		dma_addr = xfer->buf->data;

		LOG_HEXDUMP_DBG(xfer->buf->data, ch->length, "BULK OUT");
	}

	pkt_cnt = calc_packet_count(ch->length, xfer->mps);

	hctsiz = usb_dwc2_set_hctsiz_pid(ch->data->next_pid) |
		usb_dwc2_set_hctsiz_pktcnt(pkt_cnt) |
		usb_dwc2_set_hctsiz_xfersize(ch->length);

	sys_write32(hctsiz, (mem_addr_t)&ch->regs->hctsiz);
	sys_write32((uint32_t)dma_addr, (mem_addr_t)&ch->regs->hcdma);

	/* Start transfer */
	hcchar = sys_read32((mem_addr_t)&ch->regs->hcchar);
	hcchar |= USB_DWC2_HCCHAR_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&ch->regs->hcchar);
}

static void ch_reinit(struct uhc_dwc2_channel *ch)
{
	uint32_t hcchar;

	/* Clear old channel interrupts before re-enabling. */
	sys_write32(0xFFFFFFFFU, (mem_addr_t)&ch->regs->hcint);

	/* TODO: check, if it might be a good this to do anything before the re-init */

	switch (ch->xfer->type) {
	case USB_EP_TYPE_CONTROL:
		/* Re-enable the channel using the already-programmed HCTSIZ/HCDMA. */
		hcchar = sys_read32((mem_addr_t)&ch->regs->hcchar);
		hcchar |= USB_DWC2_HCCHAR_CHENA;
		hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
		sys_write32(hcchar, (mem_addr_t)&ch->regs->hcchar);
		break;
	case USB_EP_TYPE_BULK:
		ch_start_bulk(ch);
		break;
	default:
		LOG_ERR("Reinit channel with type=%u isn't supported yet", ch->xfer->type);
		break;
	}
}

static inline void submit_new_device(const struct device *dev)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;
	uint32_t hprt = sys_read32((mem_addr_t)&base->hprt);

	switch (usb_dwc2_get_hprt_prtspd(hprt)) {
	case USB_DWC2_HPRT_PRTSPD_HIGH:
		uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_HS, 0);
		break;
	case USB_DWC2_HPRT_PRTSPD_FULL:
		uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_FS, 0);
		break;
	case USB_DWC2_HPRT_PRTSPD_LOW:
		uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_LS, 0);
		break;
	default:
		LOG_WRN("Port has unexpected speed, submit as full speed");
		uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_FS, 0);
		break;
	}

	priv->has_device = true;
}

static inline void submit_dev_gone(const struct device *dev)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);

	if (!priv->has_device) {
		return;
	}

	uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
	priv->has_device = false;
}

static int validate_control_xfer(const struct uhc_transfer *xfer)
{
	const struct usb_setup_packet *setup;
	uint16_t wLength;

	/* Only via EP0 */
	if (USB_EP_GET_IDX(xfer->ep) != 0) {
		LOG_ERR("Control transfer must use endpoint 0");
		return -EINVAL;
	}

	/* Has correct MPS */
	if ((xfer->mps != 8) && (xfer->mps != 16) &&
	    (xfer->mps != 32) && (xfer->mps != 64)) {
		LOG_ERR("Invalid control MPS %u", xfer->mps);
		return -EINVAL;
	}

	/* For DMA, ponter should be DMA aligned */
	if (!IS_ALIGNED(xfer->setup_pkt, 4)) {
		LOG_ERR("Setup packet address %p is not 4-byte aligned",
			xfer->setup_pkt);
		return -EINVAL;
	}

	setup = (const struct usb_setup_packet *)xfer->setup_pkt;
	wLength = sys_le16_to_cpu(setup->wLength);

	/* For data stage, xfer object should hold the buffer */
	if ((wLength != 0) && (xfer->buf == NULL)) {
		LOG_ERR("Control transfer with data stage requires buffer");
		return -EINVAL;
	}

	if (usb_reqtype_is_to_host(setup)) {
		/* Transfer has DATA IN step, so the buffer size should be enough */
		if ((xfer->buf != NULL) &&
		     wLength > net_buf_tailroom(xfer->buf)) {
			LOG_ERR("Control IN buffer is too small: wLength=%u tailroom=%u",
			wLength, net_buf_tailroom(xfer->buf));
			return -EINVAL;
		}
		/* For DMA, buffer pointer should be also aligned */
		if ((xfer->buf != NULL) && !IS_ALIGNED(net_buf_tail(xfer->buf), 4)) {
			LOG_WRN("Control IN buffer tail %p is not 4-byte aligned",
				net_buf_tail(xfer->buf));
			return -EINVAL;
		}
	} else {
		/* Transfer has DATA OUT step, wLength != 0 but buffer is absent or too small */
		if ((wLength != 0) &&
		    (xfer->buf != NULL) &&
		    (wLength > xfer->buf->len)) {
			LOG_ERR("Control OUT buffer is too small or absent: wLength=%u", wLength);
			return -EINVAL;
		}
		/* For DMA, buffer pointer should be also aligned */
		if ((xfer->buf != NULL) && !IS_ALIGNED(xfer->buf->data, 4)) {
			LOG_WRN("Control OUT buffer data %p is not 4-byte aligned",
				xfer->buf->data);
			return -EINVAL;
		}
	}

	return 0;
}

static int validate_bulk_xfer(const struct uhc_transfer *xfer)
{
	uint32_t size;

	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		LOG_ERR("Bulk transfer cannot use endpoint 0");
		return -EINVAL;
	}

	if (xfer->mps == 0) {
		LOG_ERR("Bulk transfer MPS is zero");
		return -EINVAL;
	}

	if ((xfer->mps != 8) && (xfer->mps != 16) &&
	    (xfer->mps != 32) && (xfer->mps != 64) &&
	    (xfer->mps != 512)) {
		LOG_ERR("Invalid bulk MPS %u", xfer->mps);
		return -EINVAL;
	}

	if (xfer->buf == NULL) {
		LOG_ERR("Bulk transfer requires buffer");
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(xfer->ep)) {
		size = net_buf_tailroom(xfer->buf);

		if (size == 0) {
			LOG_ERR("Bulk IN transfer has no tailroom");
			return -ENOMEM;
		}

		if (!IS_ALIGNED(net_buf_tail(xfer->buf), 4)) {
			LOG_ERR("Bulk IN buffer tail %p is not 4-byte aligned",
				net_buf_tail(xfer->buf));
			return -EINVAL;
		}
	} else {
		size = xfer->buf->len;

		/* TODO: Support Bulk ZLP if needed [required in CDC ACM] */
		if (size == 0) {
			LOG_ERR("Bulk OUT ZLP is not supported yet");
			return -ENOTSUP;
		}

		if (!IS_ALIGNED(xfer->buf->data, 4)) {
			LOG_ERR("Bulk OUT buffer data %p is not 4-byte aligned",
				xfer->buf->data);
			return -EINVAL;
		}
	}

	return 0;
}

static int submit_xfer(const struct device *const dev, struct uhc_transfer *const xfer)
{
	struct uhc_dwc2_channel *ch = NULL;
	int ret;

	LOG_DBG("addr=%u, ep=%02Xh, mps=%d, int=%d, start_frame=%d, stage=%d, no_status=%d",
		xfer->udev->addr, xfer->ep, xfer->mps, xfer->interval,
		xfer->start_frame, xfer->stage, xfer->no_status);

	switch (xfer->type) {
	case USB_EP_TYPE_CONTROL:
		ret = validate_control_xfer(xfer);
		break;
	case USB_EP_TYPE_BULK:
		ret = validate_bulk_xfer(xfer);
		break;
	default:
		LOG_ERR("Submit xfer with type=%u isn't supported yet", xfer->type);
		ret = -EINVAL;
		break;
	}

	if (ret != 0) {
		LOG_ERR("Invalid xfer for transfer: type=%u, err=%d", xfer->type, ret);
		return ret;
	}

	ret = ch_claim(dev, xfer, &ch);
	if (ret != 0) {
		if (ret != -EBUSY) {
			LOG_ERR("Failed to claim channel: %d", ret);
		}
		return ret;
	}

	ret = ch_configure(dev, ch);
	if (ret != 0) {
		LOG_ERR("Failed to configure channel: %d", ret);
		ch_release(dev, ch);
		return ret;
	}

	switch (xfer->type) {
	case USB_EP_TYPE_CONTROL:
		ch_start_control(ch);
		break;
	case USB_EP_TYPE_BULK:
		ch_start_bulk(ch);
		break;
	default:
		LOG_ERR("Start channel with type %d isn't supported yet", xfer->type);
		ch_release(dev, ch);
		return -EINVAL;
	}

	return ret;
}

static bool xfer_is_active(struct uhc_dwc2_data *const priv,
			   const struct uhc_transfer *const xfer)
{
	for (uint32_t idx = 0; idx < priv->numhstchnl; idx++) {
		if (priv->ch[idx].xfer == xfer) {
			return true;
		}
	}

	return false;
}

/*
 * Walk the transfer list and start any pending transfer that can be served
 * right now. Must be called with the internal lock (uhc_lock_internal()) held.
 */
static void submit_pending(const struct device *const dev)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct uhc_data *const data = dev->data;
	struct uhc_transfer *xfer, *tmp;
	int ret;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&data->ctrl_xfers, xfer, tmp, node) {
		/* No channel can be claimed, stop walking the queue */
		if (priv->free_chs == 0) {
			break;
		}

		/* Skip transfers already assigned to a channel */
		if (xfer_is_active(priv, xfer)) {
			continue;
		}

		ret = submit_xfer(dev, xfer);
		if (ret == -EBUSY) {
			/* Endpoint busy, retry on next completion */
			continue;
		}

		if (ret != 0) {
			/* Error, return the transfer to the higher layer */
			LOG_ERR("Dropping xfer %p, submit failed: %d", (void *)xfer, ret);
			uhc_xfer_return(dev, xfer, ret);
		}
	}
}

static void port_handle_events(const struct device *dev, uint32_t event_mask)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = config->base;

	LOG_DBG("Port events: %08Xh", event_mask);

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_CONNECTION)) {
		/* Port connected */
		LOG_DBG("Port connected");
		/* Debounce port connection */
		if (port_debounce(dev, UHC_DWC2_EVENT_PORT_CONNECTION)) {
			/* Do first reset in the driver */
			port_reset(dev);
			/* Notify the higher logic about the new device */
			submit_new_device(dev);
		} else {
			/* TODO: Implement handling */
			LOG_ERR("Port changed during debouncing connect");
		}
	}

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_DISCONNECTION)) {
		/* Port disconnected */
		/* Debounce port disconnection */
		if (port_debounce(dev, UHC_DWC2_EVENT_PORT_DISCONNECTION)) {
			LOG_DBG("Port disconnected");
			/* Notify upper layer */
			submit_dev_gone(dev);
			/* Reset the controller to handle new connection */
			soft_reset(dev);
			/* Prepare for device connection */
			port_enable(dev);
		} else {
			/* TODO: Implement handling */
			LOG_ERR("Port changed during debouncing disconnect");
		}
	}

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_ERROR)) {
		LOG_DBG("Port error");
		/* Notify upper layer */
		submit_dev_gone(dev);
		/* TODO: recover from the error */

		/* Reset the controller to handle new connection */
		soft_reset(dev);
		/* Prepare for device connection */
		port_enable(dev);
	}

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_OVERCURRENT)) {
		LOG_ERR("Port overcurrent");
		/* TODO: Handle overcurrent */
		LOG_WRN("Handle overcurrent is not implemented");
		/* Just power off the port via registers */
		dwc2_set_power(base, false);
	}
}

static void ch_handle_events(const struct device *dev, struct uhc_dwc2_channel *ch)
{
	uint32_t events = (uint32_t)atomic_set(&ch->events, 0);
	struct uhc_transfer *const xfer = ch->xfer;
	int err = -EIO;

	if (xfer == NULL) {
		/*
		 * The channel was released while its event was already raised,
		 * for example the transfer was dequeued.
		 */
		return;
	}

	LOG_DBG("Thread channel%d events: %08Xh", ch->index, events);

	if (events & BIT(UHC_DWC2_CHANNEL_DO_RELEASE)) {
		/* CANCELLED, ERROR, STALL and COMPLETE are mutually exclusive */
		if (events & BIT(UHC_DWC2_CHANNEL_EVENT_CANCELLED)) {
			err = -ECONNRESET;
		} else if (events & BIT(UHC_DWC2_CHANNEL_EVENT_ERROR)) {
			err = -EIO;
		} else if (events & BIT(UHC_DWC2_CHANNEL_EVENT_STALL)) {
			err = -EPIPE;
		} else if (events & BIT(UHC_DWC2_CHANNEL_EVENT_CPLT)) {
			err = 0;
			ch_complete(dev, ch);
		} else {
			LOG_ERR("Channel%d has unhandled events", ch->index);
		}

		ch_release(dev, ch);

		uhc_xfer_return(dev, xfer, err);
	} else {
		/* Reinit/rewind transfer */
		if (events & BIT(UHC_DWC2_CHANNEL_DO_REWIND)) {
			/* TODO: Implement rewind xfer */
			LOG_WRN("DO_REWIND not implemented yet");
		}

		if (events & BIT(UHC_DWC2_CHANNEL_DO_REINIT)) {
			ch_reinit(ch);
		}
	}
}

static void uhc_dwc2_isr_handler(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const base = config->base;
	uint32_t gintsts;
	uint32_t hprt = 0;
	uint32_t ch_events = 0;
	unsigned int ch_index;

	/* Read and clear core interrupt status */
	gintsts = sys_read32((mem_addr_t)&base->gintsts);
	sys_write32(gintsts, (mem_addr_t)&base->gintsts);

	LOG_DBG("GINTSTS=0x%08X", gintsts);

	/* Handle disconnect first */
	if (gintsts & USB_DWC2_GINTSTS_DISCONNINT) {
		/* Port disconnected */
		port_debounce_lock(dev);
		k_event_post(&priv->events, BIT(UHC_DWC2_EVENT_PORT_DISCONNECTION));
	}

	/* To have better throughput, handle channels right after disconnection */
	if (gintsts & USB_DWC2_GINTSTS_HCHINT) {
		/* Handle pending channel event  */
		uint32_t haint = sys_read32((mem_addr_t)&base->haint);

		LOG_DBG("HAINT 0x%08x", haint);

		/* Channel count might be configured via menuconfig, trim it if needed */
		if (haint & ~BIT_MASK(MAX_CHANNELS)) {
			haint &= BIT_MASK(MAX_CHANNELS);
			LOG_WRN("HAINT trimmed to valid channel mask 0x%08X", haint);
		}

		while (haint != 0U) {
			/* Channel index = channel number - 1 */
			ch_index = __builtin_ffs(haint) - 1U;

			__ASSERT_NO_MSG(priv->ch[ch_index].index == ch_index);

			ch_events = ch_handle_irq_events(&priv->ch[ch_index]);
			if (ch_events) {
				atomic_or(&priv->ch[ch_index].events, ch_events);
				k_event_post(&priv->events,
					BIT(UHC_DWC2_EVENT_PORT_PEND_CHANNEL + ch_index));
			}
			haint &= ~BIT(ch_index);
		}
	}

	if (gintsts & USB_DWC2_GINTSTS_PRTINT) {
		/* Port interrupt, read and clear all, except port enable */
		hprt = sys_read32((mem_addr_t)&base->hprt);
		sys_write32(hprt & ~USB_DWC2_HPRT_PRTENA, (mem_addr_t)&base->hprt);

		LOG_DBG("HPRT 0x%08x", hprt);

		/* Handle port overcurrent as it is a failure state */
		if (hprt & USB_DWC2_HPRT_PRTOVRCURRCHNG) {
			/* TODO: Overcurrent or overcurrent clear? */
			k_event_post(&priv->events, BIT(UHC_DWC2_EVENT_PORT_OVERCURRENT));
		}

		/* Handle port change */
		if (hprt & USB_DWC2_HPRT_PRTENCHNG) {
			if (hprt & USB_DWC2_HPRT_PRTENA) {
				/* Configure Host timings */
				dwc2_config_timings(base);
				/* Give port enable semaphore to unlock bus reset sequence */
				k_sem_give(&priv->sem_port_enabled);
			} else {
				/* Host port has been disabled */
				k_event_post(&priv->events, BIT(UHC_DWC2_EVENT_PORT_DISABLED));
			}
		}

		/* Handle port connection */
		if (hprt & USB_DWC2_HPRT_PRTCONNDET && !priv->debouncing) {
			/* Port connected */
			port_debounce_lock(dev);
			k_event_post(&priv->events, BIT(UHC_DWC2_EVENT_PORT_CONNECTION));
		}
	}

	(void)uhc_dwc2_quirk_irq_clear(dev);
}

/*
 * Process transfers marked for cancellation by uhc_dwc2_dequeue(). Runs in the
 * driver thread, so it owns all channel state. The transfer's channel is
 * halted and released with the controller interrupt disabled, so the ISR
 * cannot read or post an event for a channel being torn down.
 */
static void dequeue_cancelled(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct uhc_data *const data = dev->data;
	struct uhc_transfer *xfer, *tmp;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&data->ctrl_xfers, xfer, tmp, node) {
		struct uhc_dwc2_channel *ch = NULL;
		bool halting = false;

		if (xfer->err != -ECONNRESET) {
			continue;
		}

		/* Find the channel the transfer is assigned to, if any */
		for (uint32_t idx = 0; idx < priv->numhstchnl; idx++) {
			if (priv->ch[idx].xfer == xfer) {
				ch = &priv->ch[idx];
				break;
			}
		}

		if (ch != NULL) {
			/*
			 * Initiate halting with the controller interrupt
			 * disabled, so the ISR cannot run for this channel
			 * concurrently.
			 */
			config->irq_disable_func(dev);

			halting = ch_halt_initiate(ch);
			if (!halting) {
				/*
				 * Channel was no longer running, finish the
				 * teardown here.
				 */
				atomic_set(&ch->events, 0);
				k_event_clear(&priv->events,
					BIT(UHC_DWC2_EVENT_PORT_PEND_CHANNEL + ch->index));
				ch_release(dev, ch);
			}

			config->irq_enable_func(dev);
		}

		if (halting) {
			/*
			 * Halt initiated and will be finished in
			 * ch_handle_events():
			 */
			continue;
		}

		/* Return pending or already-stopped transfer */
		uhc_xfer_return(dev, xfer, -ECONNRESET);
	}
}

static void uhc_dwc2_thread(void *arg0, void *arg1, void *arg2)
{
	const struct device *const dev = (const struct device *)arg0;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	uint32_t event_mask;

	while (true) {
		event_mask = k_event_wait_safe(&priv->events, UINT32_MAX, false, K_FOREVER);

		uhc_lock_internal(dev, K_FOREVER);

		/* Handle port events */
		port_handle_events(dev, event_mask);

		/* Interate channels events */
		for (uint32_t index = 0; index < MAX_CHANNELS; index++) {
			if (event_mask & BIT(UHC_DWC2_EVENT_PORT_PEND_CHANNEL + index)) {
				ch_handle_events(dev, &priv->ch[index]);
			}
		}

		/* Cancel transfers marked for dequeue */
		if (event_mask & BIT(UHC_DWC2_EVENT_DEQUEUE)) {
			dequeue_cancelled(dev);
		}

		/* Check if a new transfer can be started */
		submit_pending(dev);

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
	int ret;

	ret = port_reset(dev);
	if (ret != 0) {
		LOG_ERR("Unable to reset root port");
		return ret;
	}

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
	uhc_lock_internal(dev, K_FOREVER);

	(void)uhc_xfer_append(dev, xfer);

	/*
	 * Try to start a new transfer. If no channel/endpoint is free it stays
	 * in the list and is started later from the completion path.
	 */
	submit_pending(dev);

	uhc_unlock_internal(dev);

	return 0;
}

static int uhc_dwc2_dequeue(const struct device *const dev, struct uhc_transfer *const xfer)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct uhc_data *const data = dev->data;
	struct uhc_transfer *tmp;

	uhc_lock_internal(dev, K_FOREVER);

	/*
	 * Mark the queued transfer as cancelled. The dequeue process itself
	 * will take place in the driver's thread.
	 */
	SYS_DLIST_FOR_EACH_CONTAINER(&data->ctrl_xfers, tmp, node) {
		if (tmp == xfer) {
			tmp->err = -ECONNRESET;
			k_event_post(&priv->events, BIT(UHC_DWC2_EVENT_DEQUEUE));
			break;
		}
	}

	uhc_unlock_internal(dev);

	return 0;
}

static int uhc_dwc2_preinit(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const base = config->base;
	int ret;

	for (uint32_t idx = 0; idx < MAX_CHANNELS; idx++) {
		priv->ch[idx].index = idx;
		priv->ch[idx].regs =
			(struct usb_dwc2_host_chan *)((mem_addr_t)base + USB_DWC2_HCCHAR(idx));
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
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const base = config->base;
	uint32_t hcfg;
	uint32_t hfir;
	uint32_t gsnpsid;
	uint32_t gintsts;
	int ret;

	for (uint32_t idx = 0; idx < MAX_CHANNELS; idx++) {
		priv->ch[idx].length = 0;
		priv->ch_data[0][idx].next_pid = USB_DWC2_HCTSIZ_PID_DATA0;
		priv->ch_data[1][idx].next_pid = USB_DWC2_HCTSIZ_PID_DATA0;
	}

	ret = uhc_dwc2_quirk_pre_init(dev);
	if (ret != 0) {
		LOG_ERR("Quirk pre init failed %d", ret);
		return ret;
	}

	gsnpsid = sys_read32((mem_addr_t)&base->gsnpsid);
	if (gsnpsid == 0) {
		LOG_ERR("Unexpected GSNPSID 0x%08x", gsnpsid);
		return -ENOTSUP;
	}

	LOG_DBG("DWC2 Core rev. %04x", usb_dwc2_get_gsnpsid_rev(gsnpsid));

	ret = dwc2_core_soft_reset(dev);
	if (ret != 0) {
		LOG_ERR("DWC2 core reset failed after PHY init: %d", ret);
		return ret;
	}

	ret = dwc2_core_init_host_gusbcfg(dev);
	if (ret != 0) {
		LOG_ERR("Unable to configure USB global register");
		return ret;
	}

	/* All channels are free after (re-)initialization */
	priv->free_chs = priv->numhstchnl;

	ret = dwc2_core_init_gahbcfg(dev);
	if (ret != 0) {
		/* TODO: Implement Slave Mode */
		LOG_WRN("DMA isn't supported, but Slave Mode isn't implemented yet");
		return ret;
	}

	hcfg = sys_read32((mem_addr_t)&base->hcfg);
	/* Only Buffer DMA for now */
	hcfg &= ~USB_DWC2_HCFG_DESCDMA;
	/* Work on maximum supported speed */
	hcfg &= ~USB_DWC2_HCFG_FSLSSUPP;
	/* Disable periodic scheduling, will enable after port is enabled */
	hcfg &= ~USB_DWC2_HCFG_PERSCHEDENA;

	sys_write32(hcfg, (mem_addr_t)&base->hcfg);

	hfir = sys_read32((mem_addr_t)&base->hfir);
	/* Enable dynamic loading if needed */
	if (usb_dwc2_get_gsnpsid_rev(gsnpsid) >
	    usb_dwc2_get_gsnpsid_rev(USB_DWC2_GSNPSID_REV_2_92A)) {
		hfir |= USB_DWC2_HFIR_HFIRRLDCTRL;
	} else {
		hfir &= ~USB_DWC2_HFIR_HFIRRLDCTRL;
	}
	sys_write32(hfir, (mem_addr_t)&base->hfir);

	/* Clear status */
	gintsts = sys_read32((mem_addr_t)&base->gintsts);
	sys_write32(gintsts, (mem_addr_t)&base->gintsts);

	return ret;
}

static int uhc_dwc2_enable(const struct device *const dev)
{
	int ret;

	ret = uhc_dwc2_quirk_pre_enable(dev);
	if (ret != 0) {
		LOG_ERR("Quirk pre enable failed %d", ret);
		return ret;
	}

	soft_reset(dev);

	port_enable(dev);

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

	submit_dev_gone(dev);

	config->irq_disable_func(dev);

	port_disable(dev);

	ret = uhc_dwc2_quirk_disable(dev);
	if (ret != 0) {
		LOG_ERR("Quirk disable failed %d", ret);
		return ret;
	}

	return 0;
}

static int uhc_dwc2_shutdown(const struct device *const dev)
{
	int ret;

	ret = uhc_dwc2_quirk_shutdown(dev);
	if (ret) {
		LOG_ERR("Quirk shutdown failed %d", ret);
		return ret;
	}

	return 0;
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

#ifndef UHC_DWC2_IRQ_DT_INST_DEFINE
#define UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
	static void uhc_dwc2_irq_enable_func_##n(const struct device *dev)	\
	{									\
		ARG_UNUSED(dev);						\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    uhc_dwc2_isr_handler,				\
			    DEVICE_DT_INST_GET(n),				\
			    0);							\
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
		.events = Z_EVENT_INITIALIZER(uhc_dwc2_priv_##n.events),	\
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
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, uhc_dwc2_preinit, NULL,			\
		&uhc_data_##n, &uhc_dwc2_config_##n,				\
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
		&uhc_dwc2_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_DWC2_DEVICE_DEFINE)
