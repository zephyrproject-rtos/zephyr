/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Roman Leonov <jam_roma@yahoo.com>
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_dwc2

#include <stdint.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_ch9.h>

LOG_MODULE_REGISTER(uhc_dwc2, CONFIG_UHC_DRIVER_LOG_LEVEL);

#include <usb_dwc2_hw.h>

#include "uhc_common.h"

/* Mask to clear HPRT register */
#define USB_DWC2_HPRT_W1C_MSK	(USB_DWC2_HPRT_PRTENA |				\
				 USB_DWC2_HPRT_PRTCONNDET |			\
				 USB_DWC2_HPRT_PRTENCHNG |			\
				 USB_DWC2_HPRT_PRTOVRCURRCHNG)

/* Mask of errors received for host channels (STALL excluded) */
#define USB_DWC2_HCINT_ERRORS	(USB_DWC2_HCINT_AHBERR |			\
				 USB_DWC2_HCINT_XACTERR |			\
				 USB_DWC2_HCINT_BBLERR |			\
				 USB_DWC2_HCINT_FRMOVRUN |			\
				 USB_DWC2_HCINT_DTGERR |			\
				 USB_DWC2_HCINT_XACTERR)

#define EPSIZE_BULK_FS		64
#define EPSIZE_BULK_HS		512
#define EPSIZE_ISO_FS_MAX	1023
#define EPSIZE_ISO_HS_MAX	1024

#define UHC_DWC2_QUIRK_CONFIG(dev)						\
	(((const struct uhc_dwc2_config *)dev->config)->quirk_config)

#define UHC_DWC2_QUIRK_DATA(dev)						\
	(((const struct uhc_dwc2_config *)dev->config)->quirk_data)

enum uhc_dwc2_channel_event {
	/* The channel has completed execution of a transfer. Channel is now halted */
	UHC_DWC2_CHANNEL_EVENT_CPLT,
	/* The channel has encountered an error. Channel is now halted */
	UHC_DWC2_CHANNEL_EVENT_ERROR,
	/* Need to release the channel for the next transfer */
	UHC_DWC2_CHANNEL_DO_RELEASE,
	/* A halt has been requested on the channel */
	UHC_DWC2_CHANNEL_EVENT_HALT_REQ,
	/* Need to reinit a channel */
	UHC_DWC2_CHANNEL_DO_REINIT,
	/* Need to process the next CSPLIT */
	UHC_DWC2_CHANNEL_DO_NEXT_CSPLIT,
	/* Need to process the next CSPLIT */
	UHC_DWC2_CHANNEL_DO_NEXT_SSPLIT,
	/* Need to process the next transaction */
	UHC_DWC2_CHANNEL_DO_NEXT_TRANSACTION,
	/* Need to re-enable the channel */
	UHC_DWC2_CHANNEL_DO_REENABLE_CHANNEL,
	/* Need to re-enable the channel */
	UHC_DWC2_CHANNEL_DO_REENABLE,
	/* Need to retry the CSPLIT transaction */
	UHC_DWC2_CHANNEL_DO_RETRY_CSPLIT,
	/* Need to retry the SSPLIT transactio */
	UHC_DWC2_CHANNEL_DO_RETRY_SSPLIT,
	/* Need to rewind the buffer being transmitted on this channel */
	UHC_DWC2_CHANNEL_DO_REWIND,
	/* The channel has completed a transfer. Request STALLed. Channel is now halted */
	UHC_DWC2_CHANNEL_EVENT_STALL,
};

enum uhc_dwc2_port_event {
	/* No event has occurred or the event is no longer valid */
	UHC_DWC2_EVENT_NONE,
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
	/* Port has pending channel event, one bit per channel */
	UHC_DWC2_EVENT_PORT_PEND_CHANNEL
};

struct uhc_dwc2_vendor_quirks {
	int (*preinit)(const struct device *const dev);
	int (*init)(const struct device *const dev);
	int (*pre_enable)(const struct device *const dev);
	int (*post_enable)(const struct device *const dev);
	int (*disable)(const struct device *const dev);
	int (*shutdown)(const struct device *const dev);
	int (*irq_clear)(const struct device *const dev);
};

struct uhc_dwc2_config {
	/* Pointer to base address of DWC2 registers */
	struct usb_dwc2_reg *const base;
	/* Thread stack */
	k_thread_stack_t *stack;
	size_t stack_size;
	/* Vendor-specific implementation */
	const struct uhc_dwc2_vendor_quirks *quirk_api;
	void *quirk_data;
	const void *quirk_config;
	/* IRQ configuration */
	void (*irq_enable_func)(const struct device *const dev);
	void (*irq_disable_func)(const struct device *const dev);
};

struct uhc_dwc2_channel {
	/* Pointer to base address of a channel registers */
	struct usb_dwc2_host_chan *base;
	/* Pointer to the transfer */
	struct uhc_transfer *xfer;
	/* Channel events */
	atomic_t events;
	/* Index of the channel */
	uint8_t index;
	/* Only accessed in ISR. Number of error interrupt received. */
	uint8_t irq_error_count;
	/* Transfer stage is IN */
	uint8_t data_stg_in : 1;
	/* Transfer has no data stage */
	uint8_t data_stg_skip : 1;
	/* Transfer will change the device address */
	uint8_t set_address : 1;
};

struct uhc_dwc2_data {
	struct k_thread thread;
	/* Event bitmask the driver thread waits for */
	struct k_event events;
	/* Semaphore used in a different thread */
	struct k_sem sem_port_en;
	/* Port channels */
	struct uhc_dwc2_channel channels[CONFIG_UHC_DWC2_MAX_CHANNELS];
	/* Root Port flags */
	uint8_t debouncing : 1;
	uint8_t has_device : 1;
};

static int uhc_dwc2_soft_reset(const struct device *const dev);
static void uhc_dwc2_isr_handler(const struct device *const dev);

/*
 * Vendor quirks handling
 *
 * Definition of vendor-specific functions that can be overwritten on a per-SoC basis.
 */

static void uhc_dwc2_isr_handler(const struct device *const dev);

#if DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_usb_otg)
#include "uhc_dwc2_esp32_usb_otg.h"
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_usbhs_nrf54l)
#include "uhc_dwc2_nrf_usbhs_nrf54l.h"
#endif

/* Wrapper functions that fallback to returning 0 if no quirk is needed */
#define DWC2_QUIRK_FUNC_DEFINE(fn)						\
	static int uhc_dwc2_quirk_##fn(const struct device *const dev)		\
	{									\
		const struct uhc_dwc2_config *const config = dev->config;	\
										\
		if (config->quirk_api->fn != NULL) {				\
			return config->quirk_api->fn(dev);			\
		}								\
										\
		return 0;							\
	}

DWC2_QUIRK_FUNC_DEFINE(init);
DWC2_QUIRK_FUNC_DEFINE(pre_enable);
DWC2_QUIRK_FUNC_DEFINE(post_enable);
DWC2_QUIRK_FUNC_DEFINE(disable);
DWC2_QUIRK_FUNC_DEFINE(shutdown);
DWC2_QUIRK_FUNC_DEFINE(irq_clear);
DWC2_QUIRK_FUNC_DEFINE(preinit);

/*
 * Hardware Abstraction Layer
 *
 * Snippets of code repeated across the driver to set multiple registers.
 * Only low-level register I/O is done, using (struct usb_dwc2_reg) and other
 * parameters but never dev or dev->data/config.
 */

static uint32_t uhc_dwc2_hal_get_speed(struct usb_dwc2_reg *const dwc2)
{
	uint32_t hprt = sys_read32((mem_addr_t)&dwc2->hprt);

	return usb_dwc2_get_hprt_prtspd(hprt);
}

static void uhc_dwc2_hal_init_hfir(struct usb_dwc2_reg *const dwc2)
{
	uint32_t hfir = sys_read32((mem_addr_t)&dwc2->hfir);

	/* Disable dynamic loading */
	hfir &= ~USB_DWC2_HFIR_HFIRRLDCTRL;

	/* Set frame interval to be equal to 1ms
	 * Note: FSLS PHY has an implicit 8 divider applied when in LS mode,
	 * so the values of FSLSPclkSel and FrInt have to be adjusted accordingly.
	 */
	hfir &= ~USB_DWC2_HFIR_FRINT_MASK;
	if (uhc_dwc2_hal_get_speed(dwc2) == USB_DWC2_HPRT_PRTSPD_FULL) {
		hfir |= 48000 << USB_DWC2_HFIR_FRINT_POS;
	} else {
		hfir |= 6000 << USB_DWC2_HFIR_FRINT_POS;
	}

	sys_write32(hfir, (mem_addr_t)&dwc2->hfir);
}

static void uhc_dwc2_hal_init_hcfg(struct usb_dwc2_reg *const dwc2)
{
	uint32_t hcfg = sys_read32((mem_addr_t)&dwc2->hcfg);

	/* We can select Buffer DMA of Scatter-Gather DMA mode here: Buffer DMA for now */
	hcfg &= ~USB_DWC2_HCFG_DESCDMA;

	/* Disable periodic scheduling, will enable later */
	hcfg &= ~USB_DWC2_HCFG_PERSCHEDENA;

	/* Configure the PHY clock speed depending on max supported DWC2 speed */
	hcfg &= ~USB_DWC2_HCFG_FSLSPCLKSEL_MASK;
	if (IS_ENABLED(CONFIG_UDC_DRIVER_HIGH_SPEED_SUPPORT_ENABLED)) {
		hcfg |= USB_DWC2_HCFG_FSLSPCLKSEL_CLK3060 << USB_DWC2_HCFG_FSLSPCLKSEL_POS;
	} else {
		hcfg |= USB_DWC2_HCFG_FSLSPCLKSEL_CLK48 << USB_DWC2_HCFG_FSLSPCLKSEL_POS;
	}

	sys_write32(hcfg, (mem_addr_t)&dwc2->hcfg);
}

static void uhc_dwc2_hal_init_gusbcfg(struct usb_dwc2_reg *const dwc2)
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
}

static int uhc_dwc2_hal_init_gahbcfg(struct usb_dwc2_reg *const dwc2)

{
	uint32_t ghwcfg2 = sys_read32((mem_addr_t)&dwc2->ghwcfg2);
	uint32_t gahbcfg;

	/* Disable Global Interrupt */
	sys_clear_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);

	/* TODO: Set AHB burst mode for some ECO only for some devices */
	/* TODO: Disable HNP and SRP capabilities */

	/* Configure AHB */
	gahbcfg = USB_DWC2_GAHBCFG_NPTXFEMPLVL |
		  usb_dwc2_set_gahbcfg_hbstlen(USB_DWC2_GAHBCFG_HBSTLEN_INCR16);
	sys_write32(gahbcfg, (mem_addr_t)&dwc2->gahbcfg);

	if (usb_dwc2_get_ghwcfg2_otgarch(ghwcfg2) == USB_DWC2_GHWCFG2_OTGARCH_INTERNALDMA) {
		sys_set_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_DMAEN);
	} else {
		/* TODO: Implement Slave mode */
		LOG_WRN("DMA is not supported and Slave Mode is not implemented");
		return -ENXIO;
	}

	/* Enable Global Interrupt */
	sys_set_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);

	return 0;
}

static int uhc_dwc2_hal_core_reset(struct usb_dwc2_reg *const dwc2, const k_timeout_t timeout)
{
	const k_timepoint_t timepoint = sys_timepoint_calc(timeout);

	/* Check AHB master idle state */
	while ((sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_AHBIDLE) == 0) {
		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for AHB idle timeout, GRSTCTL 0x%08X",
				sys_read32((mem_addr_t)&dwc2->grstctl));
			return -EIO;
		}
	}

	/* Apply Core Soft Reset */
	sys_write32(USB_DWC2_GRSTCTL_CSFTRST, (mem_addr_t)&dwc2->grstctl);

	/* Wait for reset to complete */
	while ((sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_CSFTRST) != 0 &&
	       (sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_CSFTRSTDONE) == 0) {
		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for CSR done timeout, GRSTCTL 0x%08X",
				sys_read32((mem_addr_t)&dwc2->grstctl));
			return -EIO;
		}
	}

	/* CSFTRSTDONE is W1C so the write must have the bit set to clear it */
	sys_clear_bits((mem_addr_t)&dwc2->grstctl, USB_DWC2_GRSTCTL_CSFTRST);

	LOG_DBG("DWC2 core reset done");

	return 0;
}

static void uhc_dwc2_hal_port_set_power(struct usb_dwc2_reg *const dwc2, const bool power)
{
	uint32_t hprt;

	hprt = sys_read32((mem_addr_t)&dwc2->hprt);
	if (power) {
		hprt |= USB_DWC2_HPRT_PRTPWR;
	} else {
		hprt &= ~USB_DWC2_HPRT_PRTPWR;
	}
	sys_write32(hprt & ~USB_DWC2_HPRT_W1C_MSK, (mem_addr_t)&dwc2->hprt);
}

static void uhc_dwc2_hal_port_set_bus_reset(struct usb_dwc2_reg *const dwc2, const bool bus_reset)
{
	uint32_t hprt;

	hprt = sys_read32((mem_addr_t)&dwc2->hprt);
	if (bus_reset) {
		hprt |= USB_DWC2_HPRT_PRTRST;
	} else {
		hprt &= ~USB_DWC2_HPRT_PRTRST;
	}
	sys_write32(hprt & ~USB_DWC2_HPRT_W1C_MSK, (mem_addr_t)&dwc2->hprt);
}

static void uhc_dwc2_hal_set_fifo_sizes(struct usb_dwc2_reg *const dwc2)
{
	const uint32_t ghwcfg2 = sys_read32((mem_addr_t)&dwc2->ghwcfg2);
	const uint32_t ghwcfg3 = sys_read32((mem_addr_t)&dwc2->ghwcfg3);
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
	uint32_t grstctl;

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

	/* Flush TX FIFO and set number of TX FIFO */
	grstctl = usb_dwc2_set_grstctl_txfnum(CONFIG_UHC_DWC2_MAX_CHANNELS);
	grstctl |= USB_DWC2_GRSTCTL_TXFFLSH;
	sys_write32(grstctl, (mem_addr_t)&dwc2->grstctl);
	while (sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_TXFFLSH) {
		continue;
	}

	/* Flush RX FIFO */
	sys_write32(USB_DWC2_GRSTCTL_RXFFLSH, (mem_addr_t)&dwc2->grstctl);
	while (sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_RXFFLSH) {
		continue;
	}
}

static void uhc_dwc2_hal_init_hctsiz(struct usb_dwc2_host_chan *const base,
				     const uint32_t size, const uint32_t mps,
				     const enum uhc_control_stage stage)
{
	uint32_t packet_count = max(1, DIV_ROUND_UP(size, mps));
	uint32_t hctsiz = 0;

	if (stage == UHC_CONTROL_STAGE_SETUP) {
		hctsiz |= usb_dwc2_set_hctsiz_pid(USB_DWC2_HCTSIZ_PID_MDATA);
	} else {
		/* DATA or STATUS stages are always starting with DATA1 */
		hctsiz |= usb_dwc2_set_hctsiz_pid(USB_DWC2_HCTSIZ_PID_DATA1);
	}

	/* Transfer information */
	hctsiz |= usb_dwc2_set_hctsiz_pktcnt(packet_count);
	hctsiz |= usb_dwc2_set_hctsiz_xfersize(size);

	sys_write32(hctsiz, (mem_addr_t)&base->hctsiz);
}

/*
 * Port
 *
 * Event handling and debounce logic for DWC2 port
 */

static void uhc_dwc2_port_debounce_lock(const struct device *const dev)
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

static void uhc_dwc2_port_debounce_unlock(const struct device *const dev)
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

static void uhc_dwc2_channel_process_ctrl(struct uhc_dwc2_channel *const channel)
{
	struct uhc_transfer *const xfer = channel->xfer;
	uint8_t *dma_addr = NULL;
	uint16_t dma_size = 0;
	bool next_dir_is_in;
	uint32_t hcchar;
	uint32_t hctsiz;

	if (xfer->stage == UHC_CONTROL_STAGE_SETUP) {
		/* Just finished UHC_CONTROL_STAGE_SETUP */
		if (channel->data_stg_skip) {
			/* No data stage. Go strait to status */
			next_dir_is_in = true;
			xfer->stage = UHC_CONTROL_STAGE_STATUS;
		} else {
			/* Data stage is present, go to data stage */
			next_dir_is_in = channel->data_stg_in;
			xfer->stage = UHC_CONTROL_STAGE_DATA;

			if (next_dir_is_in) {
				/* For IN, number of bytes host reserves to receive */
				/* TODO: Check the buffer is large enough for the next transfer? */
				dma_size = net_buf_tailroom(xfer->buf);
			} else {
				/* For OUT, number of bytes host sends to device */
				dma_size = xfer->buf->len;
			}

			/* TODO: Toggle PID? */

			/* Get the tail of the buffer to append data */
			dma_addr = net_buf_tail(xfer->buf);
		}
	} else {
		uint32_t actual;

		/* Just finished DATA stage, actual is requested minus remaining */
		hctsiz = sys_read32((mem_addr_t)&channel->base->hctsiz);
		actual = xfer->buf->size - usb_dwc2_get_hctsiz_xfersize(hctsiz);

		/* Increase the net_buf for the actual transferred len */
		net_buf_add(xfer->buf, actual);

		/* Status stage is always the opposite direction of data stage */
		next_dir_is_in = !channel->data_stg_in;
		xfer->stage = UHC_CONTROL_STAGE_STATUS;
	}

	/* Transfer address */
	if (next_dir_is_in) {
		sys_set_bits((mem_addr_t)&channel->base->hcchar, USB_DWC2_HCCHAR_EPDIR);
	} else {
		sys_clear_bits((mem_addr_t)&channel->base->hcchar, USB_DWC2_HCCHAR_EPDIR);
	}

	/* Configure the transfer parameters */
	uhc_dwc2_hal_init_hctsiz(channel->base, dma_size, xfer->mps, xfer->stage);
	sys_write32((uintptr_t)dma_addr, (mem_addr_t)&channel->base->hcdma);

	/* TODO: Configure split transaction if needed */

	/* Sync CACHE for DMA to access it */
	sys_cache_data_flush_and_invd_range(dma_addr, dma_size);

	/* Enable the transfer */
	hcchar = sys_read32((mem_addr_t)&channel->base->hcchar);
	hcchar |= USB_DWC2_HCCHAR_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&channel->base->hcchar);
}

static void uhc_dwc2_channel_isr_handler(const struct device *const dev,
					 struct uhc_dwc2_channel *const channel)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	uint32_t chan_events = 0;
	uint32_t hcint;
	const bool xfer_is_done =
		channel->xfer->type != USB_EP_TYPE_CONTROL ||
		channel->xfer->stage == UHC_CONTROL_STAGE_STATUS;

	/* Clear the interrupt bits by writing them back */
	hcint = sys_read32((mem_addr_t)&channel->base->hcint);
	sys_write32(hcint, (mem_addr_t)&channel->base->hcint);

	/* Event decoding using a decision tree identical to the datasheet pseudocode */

	if (hcint & USB_DWC2_HCINT_ERRORS) {
		LOG_ERR("Channel error, hcint=0x%08x", hcint);
	}

	if (USB_EP_DIR_IS_IN(channel->xfer->ep) &&
	    (channel->xfer->type == USB_EP_TYPE_BULK ||
	     channel->xfer->type == USB_EP_TYPE_CONTROL)) {

		if (hcint & USB_DWC2_HCINT_CHHLTD ||
		    hcint & USB_DWC2_HCINT_XFERCOMPL) {
			if (hcint & USB_DWC2_HCINT_XFERCOMPL) {
				channel->irq_error_count = 0;
				/* Expecting ACK interrupt next */
				chan_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHANNEL_EVENT_CPLT);
			} else if (hcint & (USB_DWC2_HCINT_STALL | USB_DWC2_HCINT_BBLERR)) {
				channel->irq_error_count = 0;
				/* Expecting ACK interrupt next */
				chan_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
			} else if (hcint & USB_DWC2_HCINT_XACTERR) {
				if (channel->irq_error_count == 2) {
					chan_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHANNEL_EVENT_ERROR);
				} else {
					/* Expecting ACK, NAK, DTGERR interrupt next */
					channel->irq_error_count++;
					chan_events |= BIT(UHC_DWC2_CHANNEL_DO_REINIT);
				}
			}
		} else if (hcint & (USB_DWC2_HCINT_ACK | USB_DWC2_HCINT_NAK |
				    USB_DWC2_HCINT_DTGERR)) {
			channel->irq_error_count = 0;
			/* Not expecting ACK, NAK, DTGERR interrupt anymore */
		}
	}

	if (USB_EP_DIR_IS_OUT(channel->xfer->ep) &&
	    (channel->xfer->type == USB_EP_TYPE_BULK ||
	     channel->xfer->type == USB_EP_TYPE_CONTROL)) {

		if (hcint & USB_DWC2_HCINT_CHHLTD) {
			if (hcint & (USB_DWC2_HCINT_XFERCOMPL | USB_DWC2_HCINT_STALL)) {
				channel->irq_error_count = 1;
				/* Not expecting ACK interrupt anymore */
				chan_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHANNEL_EVENT_CPLT);
			} else if (hcint & USB_DWC2_HCINT_XACTERR) {
				if (hcint & (USB_DWC2_HCINT_NAK | USB_DWC2_HCINT_NYET |
					     USB_DWC2_HCINT_ACK)) {
					channel->irq_error_count = 1;
					chan_events |= BIT(UHC_DWC2_CHANNEL_DO_REINIT);
					chan_events |= BIT(UHC_DWC2_CHANNEL_DO_REWIND);
				} else {
					channel->irq_error_count++;
					if (channel->irq_error_count == 3) {
						chan_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
						chan_events |= BIT(UHC_DWC2_CHANNEL_EVENT_ERROR);
						chan_events |= BIT(UHC_DWC2_CHANNEL_DO_REWIND);
					}
				}
			}
		} else if (hcint & USB_DWC2_HCINT_ACK) {
			channel->irq_error_count = 1;
			/* Not expecting ACK interrupt anymore */
		}
	}

	if (USB_EP_DIR_IS_IN(channel->xfer->ep) &&
	    channel->xfer->type == USB_EP_TYPE_ISO) {

		const uint32_t hctsiz = sys_read32((mem_addr_t)&channel->base->hctsiz);

		if (hcint & USB_DWC2_HCINT_CHHLTD) {
			if (hcint & (USB_DWC2_HCINT_XFERCOMPL | USB_DWC2_HCINT_FRMOVRUN)) {
				if ((hcint & USB_DWC2_HCINT_XFERCOMPL) &&
				    (hctsiz & USB_DWC2_HCTSIZ_PKTCNT_MASK) == 0) {
					channel->irq_error_count = 0;
				}
				chan_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHANNEL_EVENT_ERROR);
			} else if (hcint & (USB_DWC2_HCINT_XACTERR | USB_DWC2_HCINT_BBLERR)) {
				if (channel->irq_error_count == 2) {
					chan_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHANNEL_EVENT_ERROR);
				} else {
					channel->irq_error_count++;
					chan_events |= BIT(UHC_DWC2_CHANNEL_DO_REENABLE_CHANNEL);
				}
			}
		}
	}

	if (USB_EP_DIR_IS_OUT(channel->xfer->ep) &&
	    channel->xfer->type == USB_EP_TYPE_ISO) {

		if (hcint & USB_DWC2_HCINT_CHHLTD) {
			if (hcint & (USB_DWC2_HCINT_XFERCOMPL | USB_DWC2_HCINT_FRMOVRUN)) {
				chan_events |= BIT(UHC_DWC2_CHANNEL_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHANNEL_EVENT_ERROR);
			}
		}
	}

	LOG_DBG("ISR: hcint=0x%08x events=0x%08x", hcint, chan_events);

	if ((chan_events & BIT(UHC_DWC2_CHANNEL_EVENT_CPLT)) && !xfer_is_done) {
		uhc_dwc2_channel_process_ctrl(channel);
	} else {
		/* Handle others in a thread */
		atomic_or(&channel->events, chan_events);
		k_event_post(&priv->events, BIT(UHC_DWC2_EVENT_PORT_PEND_CHANNEL + channel->index));
	}
}

static bool uhc_dwc2_port_debounce(const struct device *const dev,
				   enum uhc_dwc2_port_event event)
{
	const struct uhc_dwc2_config *config = dev->config;
	struct usb_dwc2_reg *dwc2 = config->base;
	bool connected;
	bool want_connected;

	/* Do the debounce delay outside of the global lock */
	uhc_unlock_internal(dev);

	k_msleep(CONFIG_UHC_DWC2_DEBOUNCE_DELAY_MS);

	uhc_lock_internal(dev, K_FOREVER);

	connected = ((sys_read32((mem_addr_t)&dwc2->hprt) & USB_DWC2_HPRT_PRTCONNSTS) != 0);
	want_connected = (event == UHC_DWC2_EVENT_PORT_CONNECTION);

	uhc_dwc2_port_debounce_unlock(dev);

	/* True if stable state matches the event */
	return connected == want_connected;
}

static void uhc_dwc2_port_handle_events(const struct device *const dev, uint32_t event_mask)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	const uint32_t speed = uhc_dwc2_hal_get_speed(dwc2);

	LOG_DBG("Port events: 0x%08X", event_mask);

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_CONNECTION)) {
		/* Port connected */
		LOG_DBG("Port connected");

		/* Debounce port connection */
		if (uhc_dwc2_port_debounce(dev, UHC_DWC2_EVENT_PORT_CONNECTION)) {
			/* Notify the higher logic about the new device */
			switch (speed) {
			case USB_DWC2_HPRT_PRTSPD_LOW:
				LOG_INF("New low-speed device");
				uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_LS, 0);
				break;
			case USB_DWC2_HPRT_PRTSPD_FULL:
				LOG_INF("New full-speed device");
				uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_FS, 0);
				break;
			case USB_DWC2_HPRT_PRTSPD_HIGH:
				LOG_INF("New high-speed device");
				uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_HS, 0);
				break;
			default:
				LOG_ERR("Unsupported speed %d", speed);
				uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_LS, -ENOTSUP);
				break;
			}

			/* Program the Host Frame Interval Register according to speed */
			uhc_dwc2_hal_init_hfir(dwc2);

			/* Mark that port has a device */
			priv->has_device = 1;

		} else {
			/* TODO: Implement handling */
			LOG_ERR("Port changed during debouncing connect");
		}

		/* Reset of the initialization in uhc_dwc2_bus_reset() */
	}

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_DISCONNECTION)) {
		/* Port disconnected */
		/* Debounce port disconnection */
		if (uhc_dwc2_port_debounce(dev, UHC_DWC2_EVENT_PORT_DISCONNECTION)) {
			LOG_DBG("Port disconnected");

			/* If port has a device - notify upper layer */
			if (priv->has_device) {
				uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
				/* Unmark that port has a device */
				priv->has_device = 0;
			}
			/* Reset the controller to handle new connection */
			uhc_dwc2_soft_reset(dev);
		} else {
			/* TODO: Implement handling */
			LOG_ERR("Port changed during debouncing disconnect");
		}
	}

	if (event_mask & BIT(UHC_DWC2_EVENT_PORT_ERROR)) {
		LOG_DBG("Port error");

		if (priv->has_device) {
			uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
			/* Unmark that port has a device */
			priv->has_device = 0;
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

/*
 * Channel
 *
 * Event decoding and channel management
 */

static int uhc_dwc2_channel_claim(const struct device *const dev,
				  struct uhc_transfer *const xfer,
				  struct uhc_dwc2_channel **const channel_p)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	/* TODO: select non-claimed channel, use channel 0 for now */
	uint8_t idx = 0;
	struct uhc_dwc2_channel *const channel = &priv->channels[idx];

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
	hcint = sys_read32((mem_addr_t)&channel->base->hcint);
	sys_write32(hcint, (mem_addr_t)&channel->base->hcint);

	/* Enable channel interrupt in the core */
	sys_set_bits((mem_addr_t)&dwc2->haintmsk, BIT(channel->index));

	hcintmsk = sys_read32((mem_addr_t)&channel->base->hcintmsk);
	/* Enable transfer complete and channel halted interrupts */
	hcintmsk |= USB_DWC2_HCINT_CHHLTD;
	/* Enable error interrupts */
	hcintmsk |= USB_DWC2_HCINT_ERRORS;
	hcintmsk |= USB_DWC2_HCINT_STALL;
	sys_write32(hcintmsk, (mem_addr_t)&channel->base->hcintmsk);

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

	sys_write32(hcchar, (mem_addr_t)&channel->base->hcchar);

	return 0;
}

static int uhc_dwc2_channel_release(const struct device *const dev,
				    struct uhc_dwc2_channel *const channel)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;

	LOG_DBG("Releasing channel%d", channel->index);

	sys_clear_bits((mem_addr_t)&dwc2->haintmsk, BIT(channel->index));

	/* Release channel */
	channel->xfer = NULL;

	return 0;
}

static int uhc_dwc2_channel_start_transfer_iso(struct uhc_dwc2_channel *const channel)
{
	struct uhc_transfer *const xfer = channel->xfer;
	char *xfer_data = net_buf_tail(xfer->buf);
	size_t xfer_size;
	uint32_t hcint;
	uint32_t hcchar;

	if (USB_EP_DIR_IS_IN(xfer->ep)) {
		xfer_size = net_buf_tailroom(xfer->buf);
	} else {
		xfer_size = xfer->buf->len;
		LOG_HEXDUMP_DBG(xfer_data, xfer_size, "ISOCHRONOUS OUT");
	}

	/* Configure the transfer parameters */
	uhc_dwc2_hal_init_hctsiz(channel->base, xfer_size, xfer->mps, xfer->stage);
	sys_write32((uintptr_t)xfer_data, (mem_addr_t)&channel->base->hcdma);

	/* Clear interrupts */
	hcint = sys_read32((mem_addr_t)&channel->base->hcint);
	sys_write32(hcint, (mem_addr_t)&channel->base->hcint);

	/* Prepare the transfer characteristics */
	hcchar = sys_read32((mem_addr_t)&channel->base->hcchar);
	hcchar |= USB_DWC2_HCCHAR_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&channel->base->hcchar);

	return 0;
}

static int uhc_dwc2_channel_start_xfer_ctrl(struct uhc_dwc2_channel *const channel)
{
	struct uhc_transfer *const xfer = channel->xfer;
	const struct usb_setup_packet *const setup_pkt =
		(const struct usb_setup_packet *)xfer->setup_pkt;
	uint32_t hcint;
	uint32_t hcchar;

	LOG_DBG("data_in: %s, data_skip: %s",
		usb_reqtype_is_to_host(setup_pkt) ? "true" : "false",
		(setup_pkt->wLength == 0) ? "true" : "false");

	LOG_HEXDUMP_DBG(xfer->setup_pkt, 8, "setup_pkt");

	/* Get information about the control transfer by analyzing the setup packet */
	channel->set_address = (setup_pkt->bRequest == USB_SREQ_SET_ADDRESS);
	channel->xfer->stage = UHC_CONTROL_STAGE_SETUP;
	channel->data_stg_in = usb_reqtype_is_to_host(setup_pkt);
	channel->data_stg_skip = (setup_pkt->wLength == 0);

	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		/* Control stage is always OUT */
		sys_clear_bits((mem_addr_t)&channel->base->hcchar, USB_DWC2_HCCHAR_EPDIR);
	}

	if (xfer->interval != 0) {
		LOG_WRN("Periodic transfer is not supported");
	}

	/* Configure the transfer parameters */
	uhc_dwc2_hal_init_hctsiz(channel->base, sizeof(*setup_pkt), xfer->mps, xfer->stage);
	sys_write32((uintptr_t)setup_pkt, (mem_addr_t)&channel->base->hcdma);

	/* TODO: Configure split transaction if needed */

	hcint = sys_read32((mem_addr_t)&channel->base->hcint);
	sys_write32(hcint, (mem_addr_t)&channel->base->hcint);

	/* TODO: sync CACHE */

	hcchar = sys_read32((mem_addr_t)&channel->base->hcchar);
	hcchar |= USB_DWC2_HCCHAR_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&channel->base->hcchar);

	return 0;
}

/*
 * Transfers
 *
 * Channels are allocated and assigned a transfer until it completes.
 */

static int uhc_dwc2_submit_xfer(const struct device *const dev, struct uhc_transfer *const xfer)
{
	struct uhc_dwc2_channel *channel = NULL;
	int ret;

	LOG_DBG("ep=0x%02x, mps=%d, int=%d, start_frame=%d, stage=%d, no_status=%d",
		xfer->ep, xfer->mps, xfer->interval,
		xfer->start_frame, xfer->stage, xfer->no_status);

	/* TODO: dma requirement, setup packet must be aligned 4 bytes */
	if ((uintptr_t)xfer->setup_pkt % 4 != 0) {
		LOG_WRN("Setup packet address %p is not 4-byte aligned",
			xfer->setup_pkt);
		return -EINVAL;
	}

	/* TODO: dma requirement, buffer addr that will used as dma addr also should be aligned */
	if (xfer->buf != NULL && (uintptr_t)net_buf_tail(xfer->buf) % 4 != 0) {
		LOG_WRN("Buffer address 0x%08lx is not 4-byte aligned",
			(uintptr_t)net_buf_tail(xfer->buf));
		return -EINVAL;
	}

	ret = uhc_dwc2_channel_claim(dev, xfer, &channel);
	if (ret != 0) {
		LOG_ERR("Failed to claim channel: %d", ret);
		return ret;
	}

	/* Init underlying channel registers */
	uhc_dwc2_channel_configure(dev, channel);

	switch (xfer->type) {
	case USB_EP_TYPE_CONTROL:
		return uhc_dwc2_channel_start_xfer_ctrl(channel);
	case USB_EP_TYPE_BULK:
	case USB_EP_TYPE_INTERRUPT:
	case USB_EP_TYPE_ISO:
		return uhc_dwc2_channel_start_transfer_iso(channel);
	default:
		LOG_WRN("Channel type %d isn't supported yet", xfer->type);
		uhc_dwc2_channel_release(dev, channel);
		return -EINVAL;
	}
}

static void uhc_dwc2_channel_handle_events(const struct device *const dev,
					   struct uhc_dwc2_channel *const channel)
{
	struct uhc_transfer *const xfer = channel->xfer;
	uint32_t events = (uint32_t)atomic_set(&channel->events, 0);
	int err = 0;

	if (events & BIT(UHC_DWC2_CHANNEL_EVENT_CPLT)) {
		/* When the device is processing SetAddress(), delay should be applied */
		if (channel->set_address) {
			k_msleep(CONFIG_UHC_DWC2_SET_ADDR_DELAY_MS);
		}
	}

	if (events & BIT(UHC_DWC2_CHANNEL_EVENT_STALL)) {
		/* Request STALLed */
		LOG_WRN("Request did stall");
		err = -EPIPE;
	}

	if (events & BIT(UHC_DWC2_CHANNEL_EVENT_ERROR)) {
		LOG_ERR("Channel%d error", channel->index);
		/* TODO: Channel has an error */
		LOG_WRN("Channel error handing has not been implemented yet");
		err = -EIO;
	}

	if (events & BIT(UHC_DWC2_CHANNEL_DO_RELEASE)) {
		uhc_dwc2_channel_release(dev, channel);
		uhc_xfer_return(dev, xfer, err);
	}

	__ASSERT_NO_MSG(xfer != NULL);
}

/*
 * Events
 *
 * Event decoding, ISR handler, and event loop thread.
 */

static void uhc_dwc2_isr_handler(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;
	uint32_t gintsts;
	uint32_t hprt = 0;

	/* Read and clear core interrupt status */
	gintsts = sys_read32((mem_addr_t)&dwc2->gintsts);
	sys_write32(gintsts, (mem_addr_t)&dwc2->gintsts);

	LOG_DBG("GINTSTS=0x%08X", gintsts);

	/* Handle disconnect first */
	if (gintsts & USB_DWC2_GINTSTS_DISCONNINT) {
		LOG_DBG("USB_DWC2_GINTSTS_DISCONNINT");

		/* Port disconnected */
		uhc_dwc2_port_debounce_lock(dev);
		k_event_post(&priv->events, BIT(UHC_DWC2_EVENT_PORT_DISCONNECTION));
	}

	if (gintsts & USB_DWC2_GINTSTS_HCHINT) {
		/* Handle pending channel event  */
		uint32_t haint = sys_read32((mem_addr_t)&dwc2->haint);

		LOG_DBG("USB_DWC2_GINTSTS_HCHINT 0x%08x", haint);

		for (int i = __builtin_ffs(haint); i != 0; i = __builtin_ffs(haint)) {
			uhc_dwc2_channel_isr_handler(dev, &priv->channels[i - 1]);
			haint &= ~BIT(i - 1);
		}
	}

	if (gintsts & USB_DWC2_GINTSTS_PRTINT) {
		/* Port interrupt occurred -> read and clear selected HPRT W1C bits */
		hprt = sys_read32((mem_addr_t)&dwc2->hprt);
		/* W1C changed bits except the PRTENA */
		sys_write32(hprt & ~USB_DWC2_HPRT_PRTENA, (mem_addr_t)&dwc2->hprt);

		LOG_DBG("USB_DWC2_GINTSTS_PRTINT 0x%08x", hprt);

		/* Handle port overcurrent as it is a failure state */
		if (hprt & USB_DWC2_HPRT_PRTOVRCURRCHNG) {
			/* TODO: Overcurrent or overcurrent clear? */
			k_event_post(&priv->events, BIT(UHC_DWC2_EVENT_PORT_OVERCURRENT));
		}

		/* Handle port change bits */
		if (hprt & USB_DWC2_HPRT_PRTENCHNG) {
			if (hprt & USB_DWC2_HPRT_PRTENA) {
				/* Host port was enabled */
				k_sem_give(&priv->sem_port_en);
			} else {
				/* Host port has been disabled */
				k_event_post(&priv->events, BIT(UHC_DWC2_EVENT_PORT_DISABLED));
			}
		}

		/* Handle port connection */
		if (hprt & USB_DWC2_HPRT_PRTCONNDET && !priv->debouncing) {
			/* Port connected */
			uhc_dwc2_port_debounce_lock(dev);
			k_event_post(&priv->events, BIT(UHC_DWC2_EVENT_PORT_CONNECTION));
		}
	}

	(void)uhc_dwc2_quirk_irq_clear(dev);
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
		uhc_dwc2_port_handle_events(dev, event_mask);

		/* Integrate channels events */
		for (uint32_t i = 0; i < CONFIG_UHC_DWC2_MAX_CHANNELS; i++) {
			if (event_mask & BIT(UHC_DWC2_EVENT_PORT_PEND_CHANNEL + i)) {
				uhc_dwc2_channel_handle_events(dev, &priv->channels[i]);
			}
		}

		uhc_unlock_internal(dev);
	}
}

/*
 * Device driver API
 */

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

	LOG_DBG("Applying a bus reset");

	/* Reset the port */
	uhc_dwc2_hal_port_set_bus_reset(dwc2, true);

	/* Hold the bus in the reset state */
	k_msleep(CONFIG_UHC_DWC2_RESET_HOLD_MS);

	/* Return the bus to the idle state. A "port enabled" event should occur */
	uhc_dwc2_hal_port_set_bus_reset(dwc2, false);

	/* Wait the port to become enabled again */
	k_sem_take(&priv->sem_port_en, K_FOREVER);

	/* Finish the port config for the appeared device */
	uhc_dwc2_hal_set_fifo_sizes(dwc2);
	/* TODO: set frame list for the ISOC/INTR xfer */
	/* TODO: enable periodic transfer */

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

	uhc_xfer_append(dev, xfer);

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

static int uhc_dwc2_init(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	uint32_t gintsts;
	uint32_t val;
	int ret;

	ret = uhc_dwc2_quirk_init(dev);
	if (ret != 0) {
		LOG_ERR("Quirk init failed %d", ret);
		return ret;
	}

	/* Read hardware configuration registers */

	val = sys_read32((mem_addr_t)&dwc2->gsnpsid);
	if (val == 0x00000000) {
		LOG_ERR("GSNPSID is zero");
		return -ENOTSUP;
	}

	/* Init DWC2 controller */

	ret = uhc_dwc2_hal_core_reset(dwc2, K_MSEC(10));
	if (ret != 0) {
		LOG_ERR("DWC2 core reset failed after PHY init: %d", ret);
		return ret;
	}

	ret = uhc_dwc2_hal_init_gahbcfg(dwc2);
	if (ret != 0) {
		return ret;
	}

	uhc_dwc2_hal_init_gusbcfg(dwc2);

	/* Configure DWC2 in host mode */

	/* Enable port interrupt */
	sys_set_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_PRTINT);

	uhc_dwc2_hal_init_hcfg(dwc2);

	/* Clear interrupts */
	gintsts = sys_read32((mem_addr_t)&dwc2->gintsts);
	sys_write32(gintsts, (mem_addr_t)&dwc2->gintsts);

	/* Reset of the initialization in uhc_dwc2_enable() */
	return 0;
}

static int uhc_dwc2_enable(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	int ret;

	ret = uhc_dwc2_quirk_pre_enable(dev);
	if (ret != 0) {
		LOG_ERR("Quirk pre enable failed %d", ret);
		return ret;
	}

	/* TODO: Pre-calculate FIFO configuration */

	/* TODO: Flush channels */

	uhc_dwc2_hal_port_set_power(dwc2, true);

	config->irq_enable_func(dev);

	sys_clear_bits((mem_addr_t)&dwc2->haintmsk, 0xFFFFFFFFUL);
	sys_set_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_PRTINT | USB_DWC2_GINTSTS_HCHINT);

	ret = uhc_dwc2_quirk_post_enable(dev);
	if (ret != 0) {
		LOG_ERR("Quirk post enable failed %d", ret);
		return ret;
	}

	/* Enable host all-channels interrupts */
	sys_write32(0xffffffffUL, (mem_addr_t)&dwc2->haintmsk);

	/* Enable port and channels interrupts */
	sys_set_bits((mem_addr_t)&dwc2->gintmsk,
		     USB_DWC2_GINTSTS_PRTINT | USB_DWC2_GINTSTS_HCHINT);

	/* Rest of the initialization in UHC_DWC2_EVENT_PORT_CONNECTION handler */
	return 0;
}

static int uhc_dwc2_disable(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	int ret;

	return 0;

	/* TODO: Check ongoing transfer? */

	config->irq_disable_func(dev);
	sys_clear_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);

	/* Power down root port */
	uhc_dwc2_hal_port_set_power(dwc2, false);

	ret = uhc_dwc2_quirk_disable(dev);
	if (ret != 0) {
		LOG_ERR("Quirk init failed: %d", ret);
		return ret;
	}

	return 0;
}

static int uhc_dwc2_shutdown(const struct device *const dev)
{
	int ret;

	return 0;

	ret = uhc_dwc2_quirk_shutdown(dev);
	if (ret != 0) {
		LOG_ERR("Quirk shutdown failed: %d", ret);
		return ret;
	}

	return 0;
}

static int uhc_dwc2_soft_reset(const struct device *const dev)
{
	int ret;

	LOG_DBG("Applying soft reset");

	ret = uhc_dwc2_disable(dev);
	if (ret != 0) {
		return ret;
	}

	ret = uhc_dwc2_shutdown(dev);
	if (ret != 0) {
		return ret;
	}

	ret = uhc_dwc2_init(dev);
	if (ret != 0) {
		return ret;
	}

	ret = uhc_dwc2_enable(dev);
	if (ret != 0) {
		return ret;
	}

	return ret;
}

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

static int uhc_dwc2_preinit(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;
	int ret;

	ret = uhc_dwc2_quirk_preinit(dev);
	if (ret != 0) {
		return ret;
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

	for (uint32_t i = 0; i < ARRAY_SIZE(priv->channels); i++) {
		priv->channels[i].index = i;
		priv->channels[i].base =
			(struct usb_dwc2_host_chan *)((mem_addr_t)dwc2 + USB_DWC2_HCCHAR(i));
	}

	return 0;
}

#ifndef UHC_DWC2_IRQ_DT_INST_DEFINE
#define UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
	static void uhc_dwc2_irq_enable_func_##n(const struct device *const dev)\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    uhc_dwc2_isr_handler,				\
			    DEVICE_DT_INST_GET(n),				\
			    0);							\
										\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static void uhc_dwc2_irq_disable_func_##n(const struct device *const dev)\
	{									\
		irq_disable(DT_INST_IRQN(n));					\
	}
#endif

#define UHC_DWC2_DEVICE_DEFINE(n)						\
	K_THREAD_STACK_DEFINE(uhc_dwc2_stack_##n, CONFIG_UHC_DWC2_STACK_SIZE);	\
										\
	UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
										\
	static struct uhc_dwc2_data uhc_dwc2_priv_##n = {			\
		.events = Z_EVENT_INITIALIZER(uhc_dwc2_priv_##n.events),	\
		.sem_port_en =							\
			Z_SEM_INITIALIZER(uhc_dwc2_priv_##n.sem_port_en, 0, 1),	\
	};									\
										\
	static struct uhc_data uhc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(uhc_data_##n.mutex),		\
		.priv = &uhc_dwc2_priv_##n,					\
	};									\
										\
	static const struct uhc_dwc2_config uhc_dwc2_config_##n = {		\
		.base = (struct usb_dwc2_reg *)DT_INST_REG_ADDR(n),		\
		.stack = uhc_dwc2_stack_##n,					\
		.stack_size = K_THREAD_STACK_SIZEOF(uhc_dwc2_stack_##n),	\
		.quirk_api = &uhc_dwc2_vendor_quirks_##n,			\
		.quirk_config = &uhc_dwc2_quirk_config_##n,			\
		.quirk_data = &uhc_dwc2_quirk_data_##n,				\
		.irq_enable_func = uhc_dwc2_irq_enable_func_##n,		\
		.irq_disable_func = uhc_dwc2_irq_disable_func_##n,		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, uhc_dwc2_preinit, NULL,			\
		&uhc_data_##n, &uhc_dwc2_config_##n,				\
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
		&uhc_dwc2_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_DWC2_DEVICE_DEFINE)
