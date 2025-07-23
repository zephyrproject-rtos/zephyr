/*
 * Copyright (c) 2026 Roman Leonov <jam_roma@yahoo.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_dwc2

#include "uhc_common.h"
#include "uhc_dwc2.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usb_ch9.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_dwc2, CONFIG_UHC_DRIVER_LOG_LEVEL);

#define DEBOUNCE_DELAY_MS CONFIG_UHC_DWC2_PORT_DEBOUNCE_DELAY_MS
#define RESET_HOLD_MS     CONFIG_UHC_DWC2_PORT_RESET_HOLD_MS
#define RESET_RECOVERY_MS CONFIG_UHC_DWC2_PORT_RESET_RECOVERY_MS
#define SET_ADDR_DELAY_MS CONFIG_UHC_DWC2_PORT_SET_ADDR_DELAY_MS

#define CTRL_EP_MAX_MPS_LS   8U
#define CTRL_EP_MAX_MPS_HSFS 64U
#define UHC_DWC2_MAX_CHAN    16

enum uhc_dwc2_event {
	/* Root port event */
	UHC_DWC2_EVENT_PORT,
	/* The host port has been enabled (i.e., connected device has been reset. Send SOFs) */
	UHC_DWC2_EVENT_ENABLED,
	/* The host port has been disabled (no more SOFs)  */
	UHC_DWC2_EVENT_DISABLED,
	/* Overcurrent detected. Port is now UHC_PORT_STATE_RECOVERY */
	UHC_DWC2_EVENT_OVERCURRENT,
	/* The host port has been cleared of the overcurrent condition */
	UHC_DWC2_EVENT_OVERCURRENT_CLEAR,
	/* A device has been connected to the port */
	UHC_DWC2_EVENT_CONNECTION,
	/* A device disconnection has been detected */
	UHC_DWC2_EVENT_DISCONNECTION,
	/* Port error detected. Port is now UHC_PORT_STATE_RECOVERY */
	UHC_DWC2_EVENT_ERROR,
	/* Event on a channel 0. Use +N for channel N */
	UHC_DWC2_EVENT_CHAN0,
};

enum uhc_dwc2_chan_event {
	/* The channel has completed execution of a transfer. Channel is now halted */
	UHC_DWC2_CHAN_EVENT_CPLT,
	/* The channel has encountered an error. Channel is now halted */
	UHC_DWC2_CHAN_EVENT_ERROR,
	/* Need to release the channel for the next transfer */
	UHC_DWC2_CHAN_DO_RELEASE,
	/* A halt has been requested on the channel */
	UHC_DWC2_CHAN_EVENT_HALT_REQ,
	/* Need to reinit a channel */
	UHC_DWC2_CHAN_DO_REINIT,
	/* Need to process the next CSPLIT */
	UHC_DWC2_CHAN_DO_NEXT_CSPLIT,
	/* Need to process the next CSPLIT */
	UHC_DWC2_CHAN_DO_NEXT_SSPLIT,
	/* Need to process the next transaction */
	UHC_DWC2_CHAN_DO_NEXT_TRANSACTION,
	/* Need to re-enable the channel */
	UHC_DWC2_CHAN_DO_REENABLE_CHANNEL,
	/* Need to retry the CSPLIT transaction */
	UHC_DWC2_CHAN_DO_RETRY_CSPLIT,
	/* Need to retry the SSPLIT transactio */
	UHC_DWC2_CHAN_DO_RETRY_SSPLIT,
	/* Need to rewind the buffer being transmitted on this channel */
	UHC_DWC2_CHAN_DO_REWIND_BUFFER,
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

enum uhc_port_state {
	/* The port is not powered */
	UHC_PORT_STATE_NOT_POWERED,
	/* The port is powered but no device is connected */
	UHC_PORT_STATE_DISCONNECTED,
	/* A device is connected to the port but has not been reset. */
	/* SOF/keep alive aren't being sent */
	UHC_PORT_STATE_DISABLED,
	/* The port is issuing a reset condition */
	UHC_PORT_STATE_RESETTING,
	/* The port has been suspended. */
	UHC_PORT_STATE_SUSPENDED,
	/* The port is issuing a resume condition */
	UHC_PORT_STATE_RESUMING,
	/* The port has been enabled. SOF/keep alive are being sent */
	UHC_PORT_STATE_ENABLED,
	/* Port needs to be recovered from a fatal error (error, overcurrent, or disconnection) */
	UHC_PORT_STATE_RECOVERY,
};

enum uhc_dwc2_ctrl_stage {
	UHC_DWC2_CTRL_STAGE_DATA0 = 0,
	UHC_DWC2_CTRL_STAGE_DATA2 = 1,
	UHC_DWC2_CTRL_STAGE_DATA1 = 2,
	UHC_DWC2_CTRL_STAGE_SETUP = 3,
};

struct uhc_dwc2_chan {
	/* Pointer to the transfer associated with the buffer */
	struct uhc_transfer *xfer;
	/* Interval in frames (FS) or microframes (HS) */
	unsigned int interval;
	/* Offset in the periodic scheduler */
	uint32_t offset;
	/* Type of endpoint */
	enum uhc_dwc2_xfer_type type;
	/* Registers cached */
	atomic_t events;
	/* The index of the channel */
	uint8_t chan_idx;
	/* Maximum Packet Size */
	uint16_t ep_mps;
	/* Endpoint address */
	uint8_t ep_addr;
	/* Endpoint type (bulk, control, isochronous or interrupt) */
	uint8_t ep_type;
	/* Device Address */
	uint8_t dev_addr;
	/* Stage index */
	uint8_t cur_stg;
	/* New address */
	uint8_t new_addr;
	/* Only accessed in ISR. Number of error interrupt received. */
	uint8_t irq_error_count;
	/* Only accessed in ISR. Whether it is time to pursue with CSPLIT. */
	uint8_t irq_do_csplit : 1;
	/* Set address request */
	uint8_t is_setting_addr: 1;
	/* Data stage is IN */
	uint8_t data_stg_in: 1;
	/* Has no data stage */
	uint8_t data_stg_skip: 1;
	/* High-speed flag */
	uint8_t is_hs: 1;
	/* Support for Low-Speed is via a Full-Speed HUB */
	uint8_t ls_via_fs_hub: 1;
	uint8_t chan_cmd_processing: 1;
	/* Halt has been requested */
	uint8_t halt_requested: 1;
	/* TODO: Lists of pending and done? */
	/* TODO: Add channel error? */
};

struct uhc_dwc2_data {
	struct k_sem irq_sem;
	struct k_thread thread;
	/* Main events the driver thread waits for */
	struct k_event event;
	struct uhc_dwc2_chan chan[UHC_DWC2_MAX_CHAN];
	/* Data, that is used in multiple threads */
	enum uhc_port_state port_state;
	/* FIFO */
	uint16_t fifo_top;
	uint16_t fifo_nptxfsiz;
	uint16_t fifo_rxfsiz;
	uint16_t fifo_ptxfsiz;
	/* TODO: Port context and callback? */
	/* TODO: FRAME LIST? */
	/* TODO: Pipes/channels LIST? */
	/* TODO: spinlock? */
};

K_THREAD_STACK_DEFINE(uhc_dwc2_stack, CONFIG_UHC_DWC2_STACK_SIZE);

#define UHC_DWC2_CHAN_REG(base, chan_idx)                                                          \
	((struct usb_dwc2_host_chan *)(((mem_addr_t)(base)) + 0x500UL + ((chan_idx) * 0x20UL)))

/*
 * DWC2 low-level HAL Functions,
 *
 * Never use device structs or other driver config/data structs, but only the registers,
 * directly provided as arguments.
 */

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

static inline void dwc2_hal_set_frame_list(struct usb_dwc2_reg *const dwc2, void *const frame_list)
{
	LOG_ERR("Setting frame list not implemented yet");
}

static inline void dwc2_hal_periodic_enable(struct usb_dwc2_reg *const dwc2)
{
	LOG_ERR("Enabling periodic scheduling not implemented yet");
}

static inline void dwc2_hal_port_init(struct usb_dwc2_reg *const dwc2)
{
	sys_clear_bits((mem_addr_t)&dwc2->haintmsk, 0xFFFFFFFFUL);
	sys_set_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_PRTINT | USB_DWC2_GINTSTS_HCHINT);
}

#define USB_DWC2_HPRT_W1C_MSK                                                                      \
	(USB_DWC2_HPRT_PRTENA | USB_DWC2_HPRT_PRTCONNDET | USB_DWC2_HPRT_PRTENCHNG |               \
	 USB_DWC2_HPRT_PRTOVRCURRCHNG)

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

static inline void dwc2_hal_toggle_power(struct usb_dwc2_reg *const dwc2, const bool power_on)
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

static inline enum uhc_dwc2_speed dwc2_hal_get_port_speed(struct usb_dwc2_reg *const dwc2)
{
	uint32_t hprt;

	hprt = sys_read32((mem_addr_t)&dwc2->hprt);
	return (hprt & USB_DWC2_HPRT_PRTSPD_MASK) >> USB_DWC2_HPRT_PRTSPD_POS;
}

/*
 * DWC2 FIFO Management
 *
 * Programming Guide 2.1.2 FIFO RAM allocation
 *
 * RX:
 * - Largest-EPsize/4 + 2 (status info). recommended x2 if high bandwidth or multiple ISO are used.
 * - 2 for transfer complete and channel halted status
 * - 1 for each Control/Bulk out endpoint to Handle NAK/NYET (i.e max is number of host channel)
 *
 * TX non-periodic (NPTX):
 * - At least largest-EPsize/4, recommended x2
 *
 * TX periodic (PTX):
 * - At least largest-EPsize*MulCount/4 (MulCount up to 3 for high-bandwidth ISO/interrupt)
 */

enum {
	EPSIZE_BULK_LS = 64,
	EPSIZE_BULK_FS = 64,
	EPSIZE_BULK_HS = 512,

	EPSIZE_ISO_FS_MAX = 1023,
	EPSIZE_ISO_HS_MAX = 1024,
};

static inline void uhc_dwc2_config_fifo_fixed_dma(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;
	const enum uhc_dwc2_speed speed = dwc2_hal_get_port_speed(dwc2);
	uint32_t nptx_largest;
	const uint32_t ptx_largest = 256 / 4;

	LOG_DBG("Configuring FIFO sizes");

	switch (speed) {
	case UHC_DWC2_SPEED_LOW:
		nptx_largest = EPSIZE_BULK_LS / 4;
		break;
	case UHC_DWC2_SPEED_FULL:
		nptx_largest = EPSIZE_BULK_FS / 4;
		break;
	case UHC_DWC2_SPEED_HIGH:
		nptx_largest = EPSIZE_BULK_HS / 4;
		break;
	default:
		CODE_UNREACHABLE;
	}

	priv->fifo_top = FIELD_GET(USB_DWC2_GHWCFG3_DFIFODEPTH_MASK, config->ghwcfg3) -
			 (FIELD_GET(USB_DWC2_GHWCFG2_NUMHSTCHNL_MASK, config->ghwcfg2) + 1);
	priv->fifo_nptxfsiz = 2 * nptx_largest;
	priv->fifo_rxfsiz = 2 * (ptx_largest + 2) +
			    (FIELD_GET(USB_DWC2_GHWCFG2_NUMHSTCHNL_MASK, config->ghwcfg2) + 1);
	priv->fifo_ptxfsiz = priv->fifo_top - (priv->fifo_nptxfsiz + priv->fifo_rxfsiz);

	/* TODO: verify ptxfsiz is overflowed */

	LOG_DBG("FIFO sizes: top=%u, nptx=%u, rx=%u, ptx=%u", priv->fifo_top * 4,
		priv->fifo_nptxfsiz * 4, priv->fifo_rxfsiz * 4, priv->fifo_ptxfsiz * 4);
}

static inline void dwc2_apply_fifo_config(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	uint16_t fifo_available = priv->fifo_top;

	sys_write32(fifo_available << USB_DWC2_GDFIFOCFG_EPINFOBASEADDR_POS | fifo_available,
		    (mem_addr_t)&dwc2->gdfifocfg);

	fifo_available -= priv->fifo_rxfsiz;

	sys_write32(priv->fifo_rxfsiz << USB_DWC2_GRXFSIZ_RXFDEP_POS, (mem_addr_t)&dwc2->grxfsiz);

	fifo_available -= priv->fifo_nptxfsiz;

	sys_write32(priv->fifo_nptxfsiz << USB_DWC2_GNPTXFSIZ_NPTXFDEP_POS | fifo_available,
		    (mem_addr_t)&dwc2->gnptxfsiz);

	fifo_available -= priv->fifo_ptxfsiz;

	sys_write32(priv->fifo_ptxfsiz << USB_DWC2_HPTXFSIZ_PTXFSIZE_POS | fifo_available,
		    (mem_addr_t)&dwc2->hptxfsiz);

	dwc2_hal_flush_tx_fifo(dwc2, 0x10UL);
	dwc2_hal_flush_rx_fifo(dwc2);

	LOG_DBG("FIFO configuration applied nptx=%u, rx=%u, ptx=%u",
		priv->fifo_nptxfsiz * 4, priv->fifo_rxfsiz * 4, priv->fifo_ptxfsiz * 4);
}

/*
 * DWC2 Port Management
 *
 * Operation of the USB port and handling if events related to it, and helpers to query information
 * about their speed, occupancy, status...
 */

#define CORE_INTRS_EN_MSK (USB_DWC2_GINTSTS_DISCONNINT)

/* Interrupts that pertain to core events */
#define CORE_EVENTS_INTRS_MSK (USB_DWC2_GINTSTS_DISCONNINT | USB_DWC2_GINTSTS_HCHINT)

/* Interrupt that pertain to host port events */
#define PORT_EVENTS_INTRS_MSK                                                                      \
	(USB_DWC2_HPRT_PRTCONNDET | USB_DWC2_HPRT_PRTENCHNG | USB_DWC2_HPRT_PRTOVRCURRCHNG)

static inline void uhc_dwc2_init_hfir(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	const enum uhc_dwc2_speed speed = dwc2_hal_get_port_speed(dwc2);
	uint32_t hfir;

	hfir = sys_read32((mem_addr_t)&dwc2->hfir);

	/* Disable dynamic loading */
	hfir &= ~USB_DWC2_HFIR_HFIRRLDCTRL;

	/* Disable dynamic loading */
	hfir &= ~USB_DWC2_HFIR_HFIRRLDCTRL;
	/* Set frame interval to be equal to 1ms
	 * Note: FSLS PHY has an implicit 8 divider applied when in LS mode,
	 * so the values of FSLSPclkSel and FrInt have to be adjusted accordingly.
	 */
	hfir &= ~USB_DWC2_HFIR_FRINT_MASK;
	switch (speed) {
	case UHC_DWC2_SPEED_LOW:
		hfir |= (6 * 1000) << USB_DWC2_HFIR_FRINT_POS;
		break;
	case UHC_DWC2_SPEED_FULL:
		hfir |= (48 * 1000) << USB_DWC2_HFIR_FRINT_POS;
		break;
	case UHC_DWC2_SPEED_HIGH:
		hfir |= (60 * 125) << USB_DWC2_HFIR_FRINT_POS;
		break;
	}

	sys_write32(hfir, (mem_addr_t)&dwc2->hfir);
}

static int uhc_dwc2_power_on(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	/* Port can only be powered on if it's currently unpowered */
	if (priv->port_state == UHC_PORT_STATE_NOT_POWERED) {
		priv->port_state = UHC_PORT_STATE_DISCONNECTED;
		dwc2_hal_port_init(dwc2);
		dwc2_hal_toggle_power(dwc2, true);
		return 0;
	}

	return -EINVAL;
}

static inline int uhc_dwc2_port_reset(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;
	int ret;

	/* TODO: implement port checks */

	/* Hint:
	 * Port can only a reset when it is in the enabled or disabled (in the case of a new
	 * connection) states. priv->port_state == UHC_PORT_STATE_ENABLED;
	 * priv->port_state == UHC_PORT_STATE_DISABLED;
	 */

	/* Proceed to resetting the bus:
	 * - Update the port's state variable
	 * - Hold the bus in the reset state for RESET_HOLD_MS.
	 * - Return the bus to the idle state for RESET_RECOVERY_MS
	 * During this reset the port state should be set to RESETTING and do not change.
	 */
	priv->port_state = UHC_PORT_STATE_RESETTING;
	dwc2_hal_toggle_reset(dwc2, true);

	/* Hold the bus in the reset state */
	k_msleep(RESET_HOLD_MS);

	if (priv->port_state != UHC_PORT_STATE_RESETTING) {
		/* The port state has unexpectedly changed */
		LOG_ERR("Port state changed during reset");
		ret = -EIO;
		goto bailout;
	}

	/* Return the bus to the idle state. Port enabled event should occur */
	dwc2_hal_toggle_reset(dwc2, false);

	/* Give the port time to recover */
	k_msleep(RESET_RECOVERY_MS);

	if (priv->port_state != UHC_PORT_STATE_RESETTING) {
		/* The port state has unexpectedly changed */
		LOG_ERR("Port state changed during reset");
		ret = -EIO;
		goto bailout;
	}

	ret = 0;
bailout:
	/* TODO: For each chan, reinitialize the channel with EP characteristics */
	/* TODO: Sync CACHE */
	return ret;
}

static int uhc_dwc2_init(const struct device *const dev);

/*
 * Port recovery is necessary when the port is in an error state and needs to be reset.
 */
static inline int uhc_dwc2_port_recovery(const struct device *const dev)
{
	int ret;

	/* TODO: Implement port checks */

	/* Port should be in recovery state and no ongoing transfers
	 * Port flags should be 0
	 */

	ret = uhc_dwc2_quirk_irq_disable_func(dev);
	if (ret) {
		LOG_ERR("Quirk IRQ disable failed %d", ret);
		return ret;
	}

	/* Init controller */
	ret = uhc_dwc2_init(dev);
	if (ret) {
		LOG_ERR("Failed to init controller: %d", ret);
		return ret;
	}

	ret = uhc_dwc2_quirk_irq_enable_func(dev);
	if (ret) {
		LOG_ERR("Quirk IRQ enable failed %d", ret);
		return ret;
	}

	ret = uhc_dwc2_power_on(dev);
	if (ret) {
		LOG_ERR("Failed to power on root port: %d", ret);
		return ret;
	}

	return ret;
}

/*
 * Buffer management
 *
 * Functions handling the operation of buffers: loading-unloading of the data, filling the
 * content, allocate and pass them between USB stack transfers and USB hardware.
 */

static inline bool uhc_dwc2_buffer_is_done(struct uhc_dwc2_chan *const chan)
{
	/* Only control transfers need to be continued */
	if (chan->type != UHC_DWC2_XFER_TYPE_CTRL) {
		return true;
	}

	return (chan->cur_stg == 2);
}

static inline void uhc_dwc2_buffer_fill_ctrl(struct uhc_dwc2_chan *const chan,
					     struct uhc_transfer *const xfer)
{
}

static inline uint16_t calc_packet_count(const uint16_t size, const uint8_t mps)
{
	if (size == 0) {
		return 1; /* in Buffer DMA mode Zero Length Packet still counts as 1 packet */
	} else {
		return DIV_ROUND_UP(size, mps);
	}
}

static void uhc_dwc2_buffer_continue(const struct device *const dev,
					 struct uhc_dwc2_chan *const chan)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_transfer *const xfer = chan->xfer;
	const struct usb_dwc2_host_chan *const chan_regs = UHC_DWC2_CHAN_REG(dwc2, chan->chan_idx);
	bool next_dir_is_in;
	enum uhc_dwc2_ctrl_stage next_pid;
	uint16_t size = 0;
	uint8_t *dma_addr = NULL;
	uint16_t pkt_cnt;
	uint32_t hctsiz;
	uint32_t hcchar;

	__ASSERT(xfer != NULL, "No transfer assigned to buffer");
	__ASSERT(chan->cur_stg != 2, "Invalid control stage: %d", chan->cur_stg);

	if (chan->cur_stg == 0) { /* Just finished control stage */
		if (chan->data_stg_skip) {
			/* No data stage. Go straight to status stage */
			next_dir_is_in = true; /* With no data stage, status stage must be IN */
			next_pid = UHC_DWC2_CTRL_STAGE_DATA1; /* Status stage always has a PID of DATA1 */
			chan->cur_stg = 2; /* Skip over */
		} else {
			/* Go to data stage */
			next_dir_is_in = chan->data_stg_in;
			/* Data stage always starts with a PID of DATA1 */
			next_pid = UHC_DWC2_CTRL_STAGE_DATA1;
			chan->cur_stg = 1;

			/* NOTE:
			 * For OUT - number of bytes host sends to device
			 * For IN - number of bytes host reserves to receive
			 */
			size = xfer->buf->size;

			/* TODO: Toggle PID? */

			/* TODO: Check if the buffer is large enough for the next transfer? */

			/* TODO: Check that the buffer is DMA and CACHE aligned and compatible with
			 * the DMA (better to do this on enqueue)
			 */

			if (xfer->buf != NULL) {
				/* Get the tail of the buffer to append data */
				dma_addr = net_buf_tail(xfer->buf);
				/* TODO: Ensure the buffer has enough space? */
				net_buf_add(xfer->buf, size);
			}
		}
	} else {
		/* cur_stg == 1. Just finished data stage. Go to status stage
		 * Status stage is always the opposite direction of data stage
		 */
		next_dir_is_in = !chan->data_stg_in;
		/* Status stage always has a PID of DATA1 */
		next_pid = UHC_DWC2_CTRL_STAGE_DATA1;
		chan->cur_stg = 2;
	}

	/* Calculate new packet count */
	pkt_cnt = calc_packet_count(size, chan->ep_mps);

	hctsiz = (next_pid << USB_DWC2_HCTSIZ_PID_POS) & USB_DWC2_HCTSIZ_PID_MASK;
	hctsiz |= (pkt_cnt << USB_DWC2_HCTSIZ_PKTCNT_POS) & USB_DWC2_HCTSIZ_PKTCNT_MASK;
	hctsiz |= (size << USB_DWC2_HCTSIZ_XFERSIZE_POS) & USB_DWC2_HCTSIZ_XFERSIZE_MASK;
	sys_write32(hctsiz, (mem_addr_t)&chan_regs->hctsiz);

	sys_write32((uint32_t)dma_addr, (mem_addr_t)&chan_regs->hcdma);

	/* TODO: Configure split transaction if needed */

	/* TODO: sync CACHE */
	hcchar = sys_read32((mem_addr_t)&chan_regs->hcchar);
	hcchar |= USB_DWC2_HCCHAR_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
	if (next_dir_is_in) {
		hcchar |= USB_DWC2_HCCHAR_EPDIR;
	} else {
		hcchar &= ~USB_DWC2_HCCHAR_EPDIR;
	}
	sys_write32(hcchar, (mem_addr_t)&chan_regs->hcchar);
}

static void uhc_dwc2_isr_chan_handler(const struct device *const dev,
				      struct uhc_dwc2_chan *const chan)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;
	const struct usb_dwc2_host_chan *const chan_regs = UHC_DWC2_CHAN_REG(dwc2, chan->chan_idx);
	const uint32_t hctsiz = sys_read32((mem_addr_t)&chan_regs->hctsiz);
	const uint32_t hcchar = sys_read32((mem_addr_t)&chan_regs->hcchar);
	const uint32_t hcint = sys_read32((mem_addr_t)&chan_regs->hcint);
	uint32_t chan_events = 0;
	bool is_split = false; /* TODO: support */
	bool is_transfer_done = uhc_dwc2_buffer_is_done(chan);

	/* Clear the interrupt bits by writing them back */
	sys_write32(hcint, (mem_addr_t)&chan_regs->hcint);

	/* Event decoding using a decision tree identical to the datasheet pseudocode */

	if (!is_split && USB_EP_DIR_IS_IN(chan->ep_addr) &&
	    (chan->ep_type == USB_EP_TYPE_BULK || chan->ep_type == USB_EP_TYPE_CONTROL)) {
		/* BULK/CONTROL IN */

		if (hcint & USB_DWC2_HCINT_CHHLTD) {
			if (hcint & USB_DWC2_HCINT_XFERCOMPL) {
				chan->irq_error_count = 0;
				/* Expecting ACK interrupt next */
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_CPLT);
			} else if (hcint & (USB_DWC2_HCINT_STALL | USB_DWC2_HCINT_BBLERR)) {
				chan->irq_error_count = 0;
				/* Expecting ACK interrupt next */
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
			} else if (hcint & USB_DWC2_HCINT_XACTERR) {
				if (chan->irq_error_count == 2) {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
				} else {
					/* Expecting ACK, NAK, DTGERR interrupt next */
					chan->irq_error_count++;
					chan_events |= BIT(UHC_DWC2_CHAN_DO_REINIT);
				}
			}
		} else if (hcint & (USB_DWC2_HCINT_ACK | USB_DWC2_HCINT_NAK |
				    USB_DWC2_HCINT_DTGERR)) {
			chan->irq_error_count = 0;
			/* Not expecting ACK, NAK, DTGERR interrupt anymore */
		}

	} else if (!is_split && USB_EP_DIR_IS_OUT(chan->ep_addr) &&
		   (chan->ep_type == USB_EP_TYPE_BULK || chan->ep_type == USB_EP_TYPE_CONTROL)) {
		/* BULK/CONTROL OUT */

		if (hcint & USB_DWC2_HCINT_CHHLTD) {
			if (hcint & (USB_DWC2_HCINT_XFERCOMPL | USB_DWC2_HCINT_STALL)) {
				chan->irq_error_count = 1;
				/* Not expecting ACK interrupt anymore */
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_CPLT);
			} else if (hcint & USB_DWC2_HCINT_XACTERR) {
				if (hcint & (USB_DWC2_HCINT_NAK | USB_DWC2_HCINT_NYET |
					     USB_DWC2_HCINT_ACK)) {
					chan->irq_error_count = 1;
					chan_events |= BIT(UHC_DWC2_CHAN_DO_REINIT);
					chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
				} else {
					chan->irq_error_count++;
					if (chan->irq_error_count == 3) {
						chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
						chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
						chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
					}
				}
			}
		} else if (hcint & USB_DWC2_HCINT_ACK) {
			chan->irq_error_count = 1;
			/* Not expecting ACK interrupt anymore */
		}

	} else if (is_split && USB_EP_DIR_IS_IN(chan->ep_addr) &&
		   (chan->ep_type == USB_EP_TYPE_BULK || chan->ep_type == USB_EP_TYPE_CONTROL)) {
		/* BULK/CONTROL IN (split) */

		if (((hcint & USB_DWC2_HCINT_CHHLTD)) == 0) {
			/* Nothing to do */
		} else if (!chan->irq_do_csplit) {
			/* Start split transaction (SSPLIT) */
			if (hcint & USB_DWC2_HCINT_ACK) {
				chan->irq_error_count = 0;
				chan->irq_do_csplit = true;
			} else if (hcint & USB_DWC2_HCINT_NAK) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
			} else if (hcint & USB_DWC2_HCINT_XACTERR) {
				chan->irq_error_count++;
				if (chan->irq_error_count < 3) {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
				} else {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
				}
			}
		} else {
			/* Complete split transaction (CSPLIT) */
			if (hcint & USB_DWC2_HCINT_XFERCOMPL) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_CPLT);
			} else if (hcint & USB_DWC2_HCINT_NAK) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
			} else if (hcint & USB_DWC2_HCINT_NYET) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_CSPLIT);
			} else if (hcint & (USB_DWC2_HCINT_STALL | USB_DWC2_HCINT_BBLERR)) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
			} else if (hcint & USB_DWC2_HCINT_XACTERR) {
				chan->irq_error_count++;
				if (chan->irq_error_count < 3) {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
				} else {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
				}
			}
		}

	} else if (is_split && USB_EP_DIR_IS_OUT(chan->ep_addr) &&
		   (chan->ep_type == USB_EP_TYPE_BULK || chan->ep_type == USB_EP_TYPE_CONTROL)) {
		/* BULK/CONTROL OUT (split) */

		if ((hcint & USB_DWC2_HCINT_CHHLTD) == 0) {
			/* Nothing to do */
		} else if (!chan->irq_do_csplit) {
			/* Start split transaction (SSPLIT) */
			if (hcint & USB_DWC2_HCINT_ACK) {
				chan->irq_error_count = 0;
				chan->irq_do_csplit = true;
			} else if (hcint & USB_DWC2_HCINT_NAK) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
			} else if (hcint & USB_DWC2_HCINT_XACTERR) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
				chan->irq_error_count++;
				if (chan->irq_error_count < 3) {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
				} else {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
				}
			}
		} else {
			/* Complete split transaction (CSPLIT) */
			if (hcint & USB_DWC2_HCINT_XFERCOMPL) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_CPLT);
			} else if (hcint & USB_DWC2_HCINT_NAK) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
			} else if (hcint & USB_DWC2_HCINT_NYET) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_CSPLIT);
			} else if (hcint & USB_DWC2_HCINT_STALL) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
			} else if (hcint & USB_DWC2_HCINT_XACTERR) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
				chan->irq_error_count++;
				if (chan->irq_error_count < 3) {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
				} else {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
				}
			}
		}

	} else if (!is_split && USB_EP_DIR_IS_IN(chan->ep_addr) &&
		   chan->ep_type == USB_EP_TYPE_INTERRUPT) {
		/* BULK/CONTROL IN */

		if (hcint & USB_DWC2_HCINT_CHHLTD) {
			if (hcint & USB_DWC2_HCINT_XFERCOMPL) {
				chan->irq_error_count = 0;
				/* Not expecting ACK interrupt anymore */
				if (is_transfer_done) {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				} else {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_REINIT);
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_CPLT);
				}
			} else if (hcint & (USB_DWC2_HCINT_STALL | USB_DWC2_HCINT_BBLERR)) {
				chan->irq_error_count = 0;
				/* Not expecting ACK interrupt anymore */
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
			} else if (hcint & (USB_DWC2_HCINT_NAK | USB_DWC2_HCINT_DTGERR |
					    USB_DWC2_HCINT_FRMOVRUN)) {
				/* Not expecting ACK interrupt anymore */
				chan_events |= BIT(UHC_DWC2_CHAN_DO_REINIT);
				/* DTGERR is "data toggle error" */
				if (hcint & (USB_DWC2_HCINT_DTGERR | USB_DWC2_HCINT_NAK)) {
					chan->irq_error_count = 0;
				}
			} else if (hcint & USB_DWC2_HCINT_XACTERR) {
				if (chan->irq_error_count == 2) {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
				} else {
					chan->irq_error_count++;
					/* Expecting ACK interrupt next */
					chan_events |= BIT(UHC_DWC2_CHAN_DO_REINIT);
				}
			}
		} else if (hcint & USB_DWC2_HCINT_ACK) {
			chan->irq_error_count = 0;
			/* Not expecting ACK interrupt anymore */
		}

	} else if (!is_split && USB_EP_DIR_IS_OUT(chan->ep_addr) &&
		   chan->ep_type == USB_EP_TYPE_INTERRUPT) {
		/* BULK/CONTROL OUT */

		if (hcint & USB_DWC2_HCINT_CHHLTD) {
			if (hcint & USB_DWC2_HCINT_XFERCOMPL) {
				chan->irq_error_count = 0;
				/* Not expecting ACK interrupt anymore */
				if (is_transfer_done) {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_CPLT);
				} else {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_REINIT);
				}
			} else if (hcint & USB_DWC2_HCINT_STALL) {
				is_transfer_done = true; /* TODO */
				chan->irq_error_count = 0;
				/* Not expecting ACK interrupt anymore */
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
			} else if (hcint & (USB_DWC2_HCINT_NAK | USB_DWC2_HCINT_FRMOVRUN)) {
				/* Not expecting ACK interrupt anymore */
				chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
				chan_events |= BIT(UHC_DWC2_CHAN_DO_REINIT);
				if (hcint & USB_DWC2_HCINT_NAK) {
					chan->irq_error_count = 0;
				}
			} else if (hcint & USB_DWC2_HCINT_XACTERR) {
				if (chan->irq_error_count == 2) {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
				} else {
					chan->irq_error_count++;
					chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
					/* Expecting ACK interrupt next */
					chan_events |= BIT(UHC_DWC2_CHAN_DO_REINIT);
				}
			}
		} else if (hcint & USB_DWC2_HCINT_ACK) {
			chan->irq_error_count = 0;
			/* Not expecting ACK interrupt anymore */
		}

	} else if (is_split && USB_EP_DIR_IS_IN(chan->ep_addr) &&
		   chan->ep_type == USB_EP_TYPE_INTERRUPT) {
		/* BULK/CONTROL IN (split) */

		if ((hcint & USB_DWC2_HCINT_CHHLTD) == 0) {
			/* Nothing to do */
		} else if (!chan->irq_do_csplit) {
			/* Start split transaction (SSPLIT) */
			if (hcint & USB_DWC2_HCINT_ACK) {
				chan->irq_do_csplit = true;
			} else if (hcint & USB_DWC2_HCINT_FRMOVRUN) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
			}
		} else {
			/* Complete split transaction (CSPLIT) */
			if (hcint & USB_DWC2_HCINT_XFERCOMPL) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_CPLT);
			} else if (hcint & USB_DWC2_HCINT_NAK) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
			} else if (hcint & USB_DWC2_HCINT_NYET) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_CSPLIT);
			} else if (hcint & (USB_DWC2_HCINT_STALL | USB_DWC2_HCINT_FRMOVRUN |
					    USB_DWC2_HCINT_BBLERR)) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
			} else if (hcint & USB_DWC2_HCINT_XACTERR) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
				if (FIELD_GET(hcchar, USB_DWC2_HCCHAR_EC_MASK) == 3) {
					/* ERR response received */
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
				} else {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
				}
			}
		}

	} else if (is_split && USB_EP_DIR_IS_OUT(chan->ep_addr) &&
		   chan->ep_type == USB_EP_TYPE_INTERRUPT) {
		/* BULK/CONTROL OUT (split) */

		if ((hcint & USB_DWC2_HCINT_CHHLTD) == 0) {
			/* Nothing to do */
		} else if (!chan->irq_do_csplit) {
			/* Start split transaction (SSPLIT) */
			if (hcint & USB_DWC2_HCINT_ACK) {
				chan->irq_do_csplit = true;
			} else if (hcint & USB_DWC2_HCINT_FRMOVRUN) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
			}
		} else {
			/* Complete split transaction (CSPLIT) */
			if (hcint & USB_DWC2_HCINT_XFERCOMPL) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_CPLT);
			} else if (hcint & USB_DWC2_HCINT_NAK) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
			} else if (hcint & USB_DWC2_HCINT_NYET) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_CSPLIT);
			} else if (hcint & (USB_DWC2_HCINT_STALL | USB_DWC2_HCINT_FRMOVRUN)) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
			} else if (hcint & USB_DWC2_HCINT_XACTERR) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
				if (FIELD_GET(hcchar, USB_DWC2_HCCHAR_EC_MASK) == 3) {
					/* ERR response received */
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
				} else {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
				}
			}
		}

	} else if (!is_split && USB_EP_DIR_IS_IN(chan->ep_addr) &&
		   chan->ep_type == USB_EP_TYPE_ISO) {
		/* BULK/CONTROL IN */

		if (hcint & USB_DWC2_HCINT_CHHLTD) {
			if (hcint & (USB_DWC2_HCINT_XFERCOMPL | USB_DWC2_HCINT_FRMOVRUN)) {
				if ((hcint & USB_DWC2_HCINT_XFERCOMPL) &&
				    (hctsiz & USB_DWC2_HCTSIZ_PKTCNT_MASK) == 0) {
					chan->irq_error_count = 0;
				}
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
			} else if (hcint & (USB_DWC2_HCINT_XACTERR | USB_DWC2_HCINT_BBLERR)) {
				if (chan->irq_error_count == 2) {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
				} else {
					chan->irq_error_count++;
					chan_events |= BIT(UHC_DWC2_CHAN_DO_REENABLE_CHANNEL);
				}
			}
		}

	} else if (!is_split && USB_EP_DIR_IS_OUT(chan->ep_addr) &&
		   chan->ep_type == USB_EP_TYPE_ISO) {
		/* BULK/CONTROL OUT */

		if (hcint & USB_DWC2_HCINT_CHHLTD) {
			if (hcint & (USB_DWC2_HCINT_XFERCOMPL | USB_DWC2_HCINT_FRMOVRUN)) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
			}
		}

	} else if (is_split && USB_EP_DIR_IS_IN(chan->ep_addr) &&
		   chan->ep_type == USB_EP_TYPE_ISO) {
		/* BULK/CONTROL IN (split) */

		if ((hcint & USB_DWC2_HCINT_CHHLTD) == 0) {
			/* Nothing to do */
		} else if (!chan->irq_do_csplit) {
			/* Start split transaction (SSPLIT) */
			if (hcint & USB_DWC2_HCINT_ACK) {
				chan->irq_do_csplit = true;
			} else if (hcint & USB_DWC2_HCINT_FRMOVRUN) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
			}
		} else {
			/* Complete split transaction (CSPLIT) */
			if (hcint & USB_DWC2_HCINT_XFERCOMPL) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_CPLT);
			} else if (hcint & USB_DWC2_HCINT_NAK) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_RETRY_SSPLIT);
			} else if (hcint & USB_DWC2_HCINT_NYET) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_NEXT_CSPLIT);
			} else if (hcint & (USB_DWC2_HCINT_STALL | USB_DWC2_HCINT_FRMOVRUN |
					    USB_DWC2_HCINT_BBLERR)) {
				chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
			} else if (hcint & USB_DWC2_HCINT_XACTERR) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_REWIND_BUFFER);
				if (FIELD_GET(hcchar, USB_DWC2_HCCHAR_EC_MASK) == 3) {
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
					chan_events |= BIT(UHC_DWC2_CHAN_DO_NEXT_SSPLIT);
				} else {
					chan_events |= BIT(UHC_DWC2_CHAN_DO_RELEASE);
					chan_events |= BIT(UHC_DWC2_CHAN_EVENT_ERROR);
				}
			}
		}

	} else if (is_split && USB_EP_DIR_IS_OUT(chan->ep_addr) &&
		   chan->ep_type == USB_EP_TYPE_ISO) {
		/* BULK/CONTROL OUT (split) */

		/* No verification of the completion status in isochronous OUT, so no
		 * Split transaction completion (CSPLIT) token, only (SSPLIT).
		 */
		if (hcint & USB_DWC2_HCINT_CHHLTD) {
			if (hcint & USB_DWC2_HCINT_ACK) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_NEXT_SSPLIT);
			} else if (hcint & USB_DWC2_HCINT_FRMOVRUN) {
				chan_events |= BIT(UHC_DWC2_CHAN_DO_NEXT_TRANSACTION);
			}
		}
	}

	LOG_DBG("ISR: events=0x%08x", chan_events);

	if ((chan_events & BIT(UHC_DWC2_CHAN_EVENT_CPLT)) && !is_transfer_done) {
		/* Optimization: Handle some events directly */
		uhc_dwc2_buffer_continue(dev, chan);
	} else {
		/* Handle others in a thread */
		atomic_or(&chan->events, chan_events);
		k_event_set(&priv->event, BIT(UHC_DWC2_EVENT_CHAN0 + chan->chan_idx));
	}
}

/*
 * Interrupt handler (ISR)
 *
 * Handle the interrupts being dispatched into events, as well as some immediate handling of
 * events directly from the IRQ handler.
 */

static void uhc_dwc2_isr_handler(const struct device *const dev)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	uint32_t core_intrs;
	uint32_t port_intrs = 0;
	uint32_t channels = 0;

	/* Read and clear core interrupt status */
	core_intrs = sys_read32((mem_addr_t)&dwc2->gintsts);
	sys_write32(core_intrs, (mem_addr_t)&dwc2->gintsts);

	LOG_DBG("GINTSTS=%08Xh, HPRT=%08Xh", core_intrs, port_intrs);

	if (core_intrs & USB_DWC2_GINTSTS_PRTINT) {
		port_intrs = sys_read32((mem_addr_t)&dwc2->hprt);
		/* Clear the interrupt status by writing 1 to the W1C bits, except the PRTENA bit */
		sys_write32(port_intrs & ~USB_DWC2_HPRT_PRTENA, (mem_addr_t)&dwc2->hprt);
	}

	/* Disconnection takes precedense over connection */
	if (core_intrs & USB_DWC2_GINTSTS_DISCONNINT) {
		/* Disconnect event */
		k_event_set(&priv->event, BIT(UHC_DWC2_EVENT_DISCONNECTION));
		/* Port still connected, check port event */
	} else if (port_intrs & USB_DWC2_HPRT_PRTCONNDET) {
		k_event_set(&priv->event, BIT(UHC_DWC2_EVENT_CONNECTION));
	} else {
		/* Nothing */
	}

	if (core_intrs & USB_DWC2_GINTSTS_HCHINT) {
		/* One or more channels have pending interrupts. Store the mask of those channels */
		channels = sys_read32((mem_addr_t)&dwc2->haint);
		for (uint8_t i; channels != 0; channels &= ~BIT(i)) {
			i = __builtin_ffs(channels) - 1;

			/* Decode the registers into an event */
			uhc_dwc2_isr_chan_handler(dev, &priv->chan[i]);
		}
	}

	if (port_intrs & USB_DWC2_HPRT_PRTOVRCURRCHNG) {
		/* Check if this is an overcurrent or an overcurrent cleared */
		if (port_intrs & USB_DWC2_HPRT_PRTOVRCURRACT) {
			/* TODO: Verify handling logic during overcurrent */
			k_event_set(&priv->event, BIT(UHC_DWC2_EVENT_OVERCURRENT));
		} else {
			k_event_set(&priv->event, BIT(UHC_DWC2_EVENT_OVERCURRENT_CLEAR));
		}
	}

	if (port_intrs & USB_DWC2_HPRT_PRTENCHNG) {
		if (port_intrs & USB_DWC2_HPRT_PRTENA) {
			/* Host port was enabled */
			k_event_set(&priv->event, BIT(UHC_DWC2_EVENT_ENABLED));
		} else {
			/* Host port has been disabled */
			k_event_set(&priv->event, BIT(UHC_DWC2_EVENT_DISABLED));
		}
	}

	(void)uhc_dwc2_quirk_irq_clear(dev);
}

/*
 * Initialization sequence
 *
 * Configure registers as described by the programmer manual.
 */

static inline void uhc_dwc2_init_gusbcfg(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	uint32_t gusbcfg;

	/* Init PHY based on the speed */
	if (FIELD_GET(USB_DWC2_GHWCFG2_HSPHYTYPE_MASK, config->ghwcfg2) !=
	    USB_DWC2_GHWCFG2_HSPHYTYPE_NO_HS) {
		gusbcfg = sys_read32((mem_addr_t)&dwc2->gusbcfg);

		/* De-select FS PHY */
		gusbcfg &= ~USB_DWC2_GUSBCFG_PHYSEL_USB11;

		if (FIELD_GET(USB_DWC2_GHWCFG2_HSPHYTYPE_MASK, config->ghwcfg2) ==
		    USB_DWC2_GHWCFG2_HSPHYTYPE_ULPI) {
			LOG_INF("Highspeed ULPI PHY init");
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
			LOG_INF("Highspeed UTMI+ PHY init");
			/* Select UTMI+ PHY (internal) */
			gusbcfg &= ~USB_DWC2_GUSBCFG_ULPI_UTMI_SEL_ULPI;
			/* Set 16-bit interface if supported */
			if (FIELD_GET(USB_DWC2_GHWCFG4_PHYDATAWIDTH_MASK, config->ghwcfg4) > 0) {
				gusbcfg |= USB_DWC2_GUSBCFG_PHYIF_16_BIT;
			} else {
				gusbcfg &= ~USB_DWC2_GUSBCFG_PHYIF_16_BIT;
			}
		}
		sys_write32(gusbcfg, (mem_addr_t)&dwc2->gusbcfg);
	} else {
		sys_set_bits((mem_addr_t)&dwc2->gusbcfg, USB_DWC2_GUSBCFG_PHYSEL_USB11);
	}
}

static inline void uhc_dwc2_init_gahbcfg(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	uint32_t core_intrs;
	uint32_t gahbcfg;

	/* Disable Global Interrupt */
	sys_clear_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);

	/* Enable Host mode */
	sys_set_bits((mem_addr_t)&dwc2->gusbcfg, USB_DWC2_GUSBCFG_FORCEHSTMODE);
	/* Wait until core is in host mode */
	while ((sys_read32((mem_addr_t)&dwc2->gintsts) & USB_DWC2_GINTSTS_CURMOD) != 1) {
		continue;
	}

	/* TODO: Set AHB burst mode for some ECO only for ESP32S2 */
	/* Make config quirk? */

	/* TODO: Disable HNP and SRP capabilities */
	/* Also move to quirk? */

	sys_clear_bits((mem_addr_t)&dwc2->gintmsk, 0xFFFFFFFFUL);

	sys_set_bits((mem_addr_t)&dwc2->gintmsk, CORE_INTRS_EN_MSK);

	/* Clear status */
	core_intrs = sys_read32((mem_addr_t)&dwc2->gintsts);
	sys_write32(core_intrs, (mem_addr_t)&dwc2->gintsts);

	/* Configure AHB */
	gahbcfg = sys_read32((mem_addr_t)&dwc2->gahbcfg);
	gahbcfg |= USB_DWC2_GAHBCFG_NPTXFEMPLVL;
	gahbcfg &= ~USB_DWC2_GAHBCFG_HBSTLEN_MASK;
	gahbcfg |= (USB_DWC2_GAHBCFG_HBSTLEN_INCR16 << USB_DWC2_GAHBCFG_HBSTLEN_POS);
	sys_write32(gahbcfg, (mem_addr_t)&dwc2->gahbcfg);

	if (FIELD_GET(USB_DWC2_GHWCFG2_OTGARCH_MASK, config->ghwcfg2) ==
	    USB_DWC2_GHWCFG2_OTGARCH_INTERNALDMA) {
		sys_set_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_DMAEN);
	}

	/* Enable Global Interrupt */
	sys_set_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);
}

static void uhc_dwc2_init_hcfg(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	const enum uhc_dwc2_speed speed = dwc2_hal_get_port_speed(dwc2);
	uint32_t hcfg;

	hcfg = sys_read32((mem_addr_t)&dwc2->hcfg);

	/* We can select Buffer DMA of Scatter-Gather DMA mode here: Buffer DMA by default */
	hcfg &= ~USB_DWC2_HCFG_DESCDMA;

	/* Disable periodic scheduling, will enable later */
	hcfg &= ~USB_DWC2_HCFG_PERSCHEDENA;

	if (FIELD_GET(USB_DWC2_GHWCFG2_HSPHYTYPE_MASK, config->ghwcfg2) ==
	    USB_DWC2_GHWCFG2_HSPHYTYPE_NO_HS) {
		/* Disable HighSpeed support */
		hcfg |= USB_DWC2_HCFG_FSLSSUPP;
	} else {
		/* Enable HighSpeed support */
		hcfg &= ~USB_DWC2_HCFG_FSLSSUPP;
	}

	/* Indicate to the OTG core what speed the PHY clock is at
	 * Note: FSLS PHY has an implicit 8 divider applied when in LS mode,
	 * so the values of FSLSPclkSel and FrInt have to be adjusted accordingly.
	 */
	switch (speed) {
	case UHC_DWC2_SPEED_LOW:
		hcfg &= ~USB_DWC2_HCFG_FSLSPCLKSEL_MASK;
		hcfg |= 2 << USB_DWC2_HCFG_FSLSPCLKSEL_POS;
		break;
	case UHC_DWC2_SPEED_FULL:
		hcfg &= ~USB_DWC2_HCFG_FSLSPCLKSEL_MASK;
		hcfg |= 1 << USB_DWC2_HCFG_FSLSPCLKSEL_POS;
		break;
	case UHC_DWC2_SPEED_HIGH:
		/* Leave to default value */
		break;
	}

	sys_write32(hcfg, (mem_addr_t)&dwc2->hcfg);
}

/*
 * Submit a new device connected event to the higher logic.
 */
static inline void uhc_dwc2_submit_new_device(const struct device *const dev,
					      const enum uhc_dwc2_speed speed)
{
	switch (speed) {
	case UHC_DWC2_SPEED_LOW:
		LOG_INF("New Low-Speed device");
		uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_LS, 0);
		break;
	case UHC_DWC2_SPEED_FULL:
		LOG_INF("New Full-Speed device");
		uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_FS, 0);
		break;
	case UHC_DWC2_SPEED_HIGH:
		LOG_INF("New High-Speed device");
		uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_HS, 0);
		break;
	default:
		LOG_ERR("Unsupported speed %d", speed);
	}
}

/*
 * Submit a device removed event to the higher logic.
 */
static inline void uhc_dwc2_submit_dev_removed(const struct device *const dev)
{
	LOG_INF("Device removed");
	uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
}

/*
 * Allocate a chan holding the underlying channel object and the DMA buffer for transfer purposes.
 */
static inline void uhc_dwc2_chan_config(const struct device *const dev,
					const uint8_t chan_idx,
					const uint8_t ep_addr,
					const uint8_t dev_addr,
					const enum uhc_dwc2_speed dev_speed,
					const enum uhc_dwc2_xfer_type type)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct uhc_dwc2_chan *const chan = &priv->chan[0];
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	const struct usb_dwc2_host_chan *const chan_regs = UHC_DWC2_CHAN_REG(dwc2, chan_idx);
	uint32_t hcchar;
	uint32_t hcint;

	/* TODO: Double buffering scheme? */

	/* Set the default chan's MPS to the worst case MPS for the device's speed */
	chan->ep_mps =
		(dev_speed == UHC_DWC2_SPEED_LOW) ? CTRL_EP_MAX_MPS_LS : CTRL_EP_MAX_MPS_HSFS;
	chan->type = type;
	chan->ep_addr = ep_addr;
	chan->chan_idx = chan_idx;
	chan->dev_addr = dev_addr;
	chan->ls_via_fs_hub = 0;
	chan->interval = 0;

	LOG_DBG("Allocating channel %d", chan->chan_idx);

	/* Init underlying channel registers */

	/* Clear the interrupt bits by writing them back */
	hcint = sys_read32((mem_addr_t)&chan_regs->hcint);
	sys_write32(hcint, (mem_addr_t)&chan_regs->hcint);

	/* Enable channel interrupts in the core */
	sys_set_bits((mem_addr_t)&dwc2->haintmsk, (1 << chan->chan_idx));

	/* Enable transfer complete and channel halted interrupts */
	sys_set_bits((mem_addr_t)&chan_regs->hcintmsk,
		     USB_DWC2_HCINT_XFERCOMPL | USB_DWC2_HCINT_CHHLTD);

	hcchar = ((uint32_t)chan->ep_mps << USB_DWC2_HCCHAR_MPS_POS);
	hcchar |= ((uint32_t)USB_EP_GET_IDX(chan->ep_addr) << USB_DWC2_HCCHAR_EPNUM_POS);
	hcchar |= ((uint32_t)chan->type << USB_DWC2_HCCHAR_EPTYPE_POS);
	hcchar |= ((uint32_t)1UL /* TODO: chan->mult */ << USB_DWC2_HCCHAR_EC_POS);
	hcchar |= ((uint32_t)chan->dev_addr << USB_DWC2_HCCHAR_DEVADDR_POS);

	if (USB_EP_DIR_IS_IN(chan->ep_addr)) {
		hcchar |= USB_DWC2_HCCHAR_EPDIR;
	}

	/* TODO: LS device plugged to HUB */
	if (false) {
		hcchar |= USB_DWC2_HCCHAR_LSPDDEV;
	}

	if (chan->type == UHC_DWC2_XFER_TYPE_INTR) {
		hcchar |= USB_DWC2_HCCHAR_ODDFRM;
	}

	if (chan->type == UHC_DWC2_XFER_TYPE_ISOCHRONOUS) {
		LOG_ERR("ISOC channels are note supported yet");
	}

	if (chan->type == UHC_DWC2_XFER_TYPE_INTR) {
		LOG_ERR("INTR channels are note supported yet");
	}

	sys_write32(hcchar, (mem_addr_t)&chan_regs->hcchar);

	/* TODO: sync CACHE */

	/* TODO: Add the chan to the list of idle chans in the port object */
}

/*
 * Free the chan and its resources.
 */
static void uhc_dwc2_chan_deinit(const struct device *const dev, struct uhc_dwc2_chan *const chan)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;

	sys_clear_bits((mem_addr_t)&dwc2->haintmsk, (1 << chan->chan_idx));
}

static inline void uhc_dwc2_handle_port_events(const struct device *const dev,
					       uint32_t events)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	enum uhc_dwc2_speed port_speed;
	bool port_has_device;

	LOG_DBG("Port events: 0x08%x", events);

	if (events & BIT(UHC_DWC2_EVENT_ENABLED)) {
		priv->port_state = UHC_PORT_STATE_ENABLED;

		/* Configuring clock for selected speed */
		uhc_dwc2_init_hfir(dev);

		port_speed = dwc2_hal_get_port_speed(dwc2);

		dwc2_apply_fifo_config(dev);
		dwc2_hal_set_frame_list(dwc2, NULL /* priv->frame_list , FRAME_LIST_LEN */);
		dwc2_hal_periodic_enable(dwc2);

		uhc_dwc2_chan_config(dev, 0, 0, 0, port_speed, UHC_DWC2_XFER_TYPE_CTRL);

		/* Notify the higher logic about the new device */
		uhc_dwc2_submit_new_device(dev, port_speed);
	}

	if (events & BIT(UHC_DWC2_EVENT_DISABLED)) {
		/* Could be due to a disable request or reset request, or due to a port error */
		/* Ignore the disable event if it's due to a reset request */
		switch (priv->port_state) {
		case UHC_PORT_STATE_RESETTING:
		case UHC_PORT_STATE_ENABLED:
			break;
		default:
			LOG_DBG("port state %d", priv->port_state);
			/* Disabled due to a port error */
			LOG_ERR("Port disabled due to an error, changing state to recovery");
			priv->port_state = UHC_PORT_STATE_RECOVERY;
			events |= BIT(UHC_DWC2_EVENT_ERROR);
			/* TODO: Notify the port event from ISR */
			/* TODO: Port disabled by request, not implemented yet */
		}
	}

	if (events & BIT(UHC_DWC2_EVENT_CONNECTION)) {
		uhc_dwc2_port_reset(dev);
	}

	if ((events & BIT(UHC_DWC2_EVENT_OVERCURRENT)) ||
	    (events & BIT(UHC_DWC2_EVENT_OVERCURRENT_CLEAR))) {
		/* If port state powered, we need to power it off to protect it
		 * change port state to recovery
		 * generate port event UHC_DWC2_EVENT_OVERCURRENT
		 */
		LOG_ERR("Overcurrent detected on port, not implemented yet");
		/* TODO: Handle overcurrent event */
	}

	if ((events & BIT(UHC_DWC2_EVENT_DISCONNECTION)) ||
	    (events & BIT(UHC_DWC2_EVENT_ERROR)) ||
	    (events & BIT(UHC_DWC2_EVENT_OVERCURRENT))) {
		port_has_device = false;

		switch (priv->port_state) {
		case UHC_PORT_STATE_DISABLED:
			break;
		case UHC_PORT_STATE_NOT_POWERED:
		case UHC_PORT_STATE_ENABLED:
			port_has_device = true;
			break;
		default:
			LOG_ERR("Unexpected port state %d", priv->port_state);
			break;
		}

		if (port_has_device) {
			uhc_dwc2_chan_deinit(dev, &priv->chan[0]);
			uhc_dwc2_submit_dev_removed(dev);
		}
	}

	/* Failure events that need a port recovery */
	if ((events & BIT(UHC_DWC2_EVENT_ERROR)) ||
	    (events & BIT(UHC_DWC2_EVENT_OVERCURRENT))) {
		uhc_dwc2_port_recovery(dev);
	}
}

static inline void uhc_dwc2_handle_chan_events(const struct device *const dev,
					       struct uhc_dwc2_chan *const chan)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	const struct usb_dwc2_host_chan *const chan_regs = UHC_DWC2_CHAN_REG(dwc2, chan->chan_idx);
	const uint32_t chan_events = chan->events;

	LOG_DBG("Channel events: 0x%08x", chan_events);

	if (chan_events & BIT(UHC_DWC2_CHAN_EVENT_CPLT)) {
		/* XFER transfer is done, process the transfer and release the chan buffer */
		struct uhc_transfer *const xfer = chan->xfer;

		if (xfer->buf != NULL) {
			LOG_HEXDUMP_DBG(xfer->buf->data, xfer->buf->len, "data");
		}

		/* TODO: Refactor the address setting logic. */
		/* To configure the channel, we need to get the dev addr from higher logic */
		if (chan->is_setting_addr) {
			chan->is_setting_addr = 0;
			chan->dev_addr = chan->new_addr;
			/* Set the new device address in the channel */
			sys_set_bits((mem_addr_t)&chan_regs->hcchar,
				     (chan->dev_addr << USB_DWC2_HCCHAR_DEVADDR_POS));
			k_msleep(SET_ADDR_DELAY_MS);
		}

		uhc_xfer_return(dev, xfer, 0);
	}

	if (chan_events & BIT(UHC_DWC2_CHAN_EVENT_ERROR)) {
		LOG_ERR("Channel error handling not implemented yet");
		/* TODO: get channel error, halt the chan */
	}

	if (chan_events & BIT(UHC_DWC2_CHAN_EVENT_HALT_REQ)) {
		LOG_ERR("Channel halt request handling not implemented yet");

		/* TODO: Implement halting the ongoing transfer */

		/* Hint:
		 * We've halted a transfer, so we need to trigger the chan callback
		 * Halt request event is triggered when packet is successful completed.
		 * But just treat all halted transfers as errors
		 * Notify the task waiting for the chan halt or halt it right away
		 * _internal_chan_event_notify(chan, true);
		 */
	}
}

static void uhc_dwc2_thread(void *const arg1, void *const arg2, void *const arg3)
{
	const struct device *const dev = (const struct device *)arg1;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	uint32_t events;

	while (true) {
		events = k_event_wait_safe(&priv->event, UINT32_MAX, false, K_FOREVER);

		uhc_lock_internal(dev, K_FOREVER);

		uhc_dwc2_handle_port_events(dev, events);

		for (uint32_t i = 0; i < UHC_DWC2_MAX_CHAN; i++) {
			if (events & BIT(UHC_DWC2_EVENT_CHAN0 + i)) {
				uhc_dwc2_handle_chan_events(dev, &priv->chan[i]);
			}
		}

		uhc_unlock_internal(dev);
	}
}

/*
 * UHC DWC2 Driver API
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
	/* TODO: move the reset logic here */

	/* Hint:
	 * First reset is done by the uhc dwc2 driver, so we don't need to do anything here.
	 */
	uhc_submit_event(dev, UHC_EVT_RESETED, 0);

	return 0;
}

static int uhc_dwc2_bus_resume(const struct device *const dev)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_config_hctsiz(const struct device *const dev, struct uhc_dwc2_chan *const chan,
				  const size_t buf_size)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	const struct usb_dwc2_host_chan *const chan_regs = UHC_DWC2_CHAN_REG(dwc2, chan->chan_idx);
	uint16_t pkt_cnt;
	uint16_t pkt_size;
	uint8_t pkt_type;
	uint8_t pkt_pid;
	uint32_t hctsiz;

	pkt_type = USB_EP_TYPE_CONTROL;
	pkt_cnt = calc_packet_count(buf_size, chan->ep_mps);
	pkt_size = MIN(buf_size, chan->ep_mps);

	switch (pkt_type) {
	case USB_EP_TYPE_CONTROL:
		pkt_pid = USB_DWC2_HCTSIZ_PID_SETUP;
		pkt_cnt = calc_packet_count(sizeof(struct usb_setup_packet), chan->ep_mps);
		pkt_size = sizeof(struct usb_setup_packet);
		break;
	case USB_EP_TYPE_BULK:
	case USB_EP_TYPE_INTERRUPT:
		pkt_pid = USB_DWC2_HCTSIZ_PID_DATA0;
		break;
	case USB_EP_TYPE_ISO:
		if (dwc2_hal_get_port_speed(dwc2) == UHC_DWC2_SPEED_HIGH) {
			/* Full-Speed isochronous transfers are always having a single packet:
			 * - only 1 pkt needed: DATA0
			 */
			if (pkt_cnt != 1) {
				pkt_pid = USB_DWC2_HCTSIZ_PID_DATA0;
			} else {
				return -EINVAL;
			}
		} else if (USB_EP_DIR_IS_OUT(chan->ep_addr)) {
			/* High-Speed isochronous OUT transfers are "high-bandwidth":
			 * - if 1 pkt needed: DATA0
			 * - if 2 pkt needed: MDATA, DATA1
			 * - if 3 pkt needed: MDATA, MDATA, DATA2
			 */
			if (pkt_cnt == 1) {
				pkt_pid = USB_DWC2_HCTSIZ_PID_DATA0;
			} else {
				pkt_pid = USB_DWC2_HCTSIZ_PID_MDATA;
			}
		} else {
			/* High-Speed isochronous IN transfers are "high-bandwidth":
			 * - if 1 pkt needed: DATA0
			 * - if 2 pkt needed: DATA1, DATA0
			 * - if 3 pkt needed: DATA2, DATA1, DATA0
			 */
			if (pkt_cnt == 1) {
				pkt_pid = USB_DWC2_HCTSIZ_PID_DATA0;
			} else if (pkt_cnt == 2) {
				pkt_pid = USB_DWC2_HCTSIZ_PID_DATA1;
			} else if (pkt_cnt == 3) {
				pkt_pid = USB_DWC2_HCTSIZ_PID_DATA2;
			} else {
				LOG_ERR("unsupported transfer size %d, aborting", buf_size);
				return -EINVAL;
			}
		}
		break;
	default:
		LOG_ERR("unsupported transfer type %d, aborting", pkt_type);
		return -ENOSYS;
	}

	LOG_DBG("ep=%02X, mps=%d", chan->ep_addr, chan->ep_mps);

	if (USB_EP_GET_IDX(chan->ep_addr) == 0) {
		/* Control stage is always OUT */
		sys_clear_bits((mem_addr_t)&chan_regs->hcchar, USB_DWC2_HCCHAR_EPDIR);
	}

	hctsiz = (pkt_pid << USB_DWC2_HCTSIZ_PID_POS) & USB_DWC2_HCTSIZ_PID_MASK;
	hctsiz |= (pkt_cnt << USB_DWC2_HCTSIZ_PKTCNT_POS) & USB_DWC2_HCTSIZ_PKTCNT_MASK;
	hctsiz |= (pkt_size << USB_DWC2_HCTSIZ_XFERSIZE_POS) & USB_DWC2_HCTSIZ_XFERSIZE_MASK;
	sys_write32(hctsiz, (mem_addr_t)&chan_regs->hctsiz);

	return 0;
}

static int uhc_dwc2_process_ctrl_xfer(const struct device *const dev,
				      struct uhc_transfer *const xfer)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct uhc_dwc2_chan *const chan = &priv->chan[USB_EP_GET_IDX(xfer->ep)];
	const struct usb_dwc2_host_chan *const chan_regs = UHC_DWC2_CHAN_REG(dwc2, chan->chan_idx);
	const size_t buf_size = USB_EP_DIR_IS_IN(xfer->ep) ? xfer->buf->size : xfer->buf->len;
	const struct usb_setup_packet *setup_pkt = (const void *)xfer->setup_pkt;
	uint32_t hcint;
	uint32_t hcchar;
	int ret;

	/* net_buf library default to sizeof(void *) alignment, which is at least 4 bytes */
	if ((uintptr_t)setup_pkt % 4 != 0) {
		LOG_ERR("Setup packet address %p is not 4-byte aligned", (void *)setup_pkt);
		return -EINVAL;
	}

	uhc_lock_internal(dev, K_FOREVER);

	chan->cur_stg = 0;
	chan->data_stg_in = usb_reqtype_is_to_host(setup_pkt);
	chan->data_stg_skip = (setup_pkt->wLength == 0);
	chan->is_setting_addr = 0;

	if (setup_pkt->bRequest == USB_SREQ_SET_ADDRESS) {
		chan->is_setting_addr = 1;
		chan->new_addr = setup_pkt->wValue & 0x7F;
		LOG_DBG("Set address request, new address %d", chan->new_addr);
	}

	LOG_DBG("data_stg_in: %d, data_stg_skip: %d", chan->data_stg_in, chan->data_stg_skip);

	/* Save the xfer pointer in the buffer */
	chan->xfer = xfer;

	/* TODO Sync data from cache to memory. For OUT and CTRL transfers */

	LOG_DBG("endpoint=%02Xh, mps=%d, interval=%d, start_frame=%d, stage=%d, no_status=%d",
		xfer->ep, xfer->mps, xfer->interval, xfer->start_frame, xfer->stage,
		xfer->no_status);

	LOG_HEXDUMP_DBG(setup_pkt, 8, "setup");

	ret = uhc_dwc2_config_hctsiz(dev, chan, buf_size);
	if (ret != 0) {
		goto err;
	}

	sys_write32((uint32_t)setup_pkt, (mem_addr_t)&chan_regs->hcdma);

	/* TODO: Configure split transaction if needed */

	hcint = sys_read32((mem_addr_t)&chan_regs->hcint);
	sys_write32(hcint, (mem_addr_t)&chan_regs->hcint);

	/* TODO: sync CACHE */

	hcchar = sys_read32((mem_addr_t)&chan_regs->hcchar);
	hcchar |= USB_DWC2_HCCHAR_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
	hcchar &= ~USB_DWC2_HCCHAR_EPDIR; /* Setup stage is always OUT direction */
	sys_write32(hcchar, (mem_addr_t)&chan_regs->hcchar);

	ret = 0;
err:
	uhc_unlock_internal(dev);
	return ret;
}

static int uhc_dwc2_process_data_xfer(const struct device *const dev,
				      struct uhc_transfer *const xfer)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct uhc_dwc2_chan *const chan = &priv->chan[USB_EP_GET_IDX(xfer->ep)];
	const struct usb_dwc2_host_chan *const chan_regs = UHC_DWC2_CHAN_REG(dwc2, chan->chan_idx);
	const size_t buf_size = USB_EP_DIR_IS_IN(xfer->ep) ? xfer->buf->size : xfer->buf->len;
	uint32_t hcint;
	uint32_t hcchar;
	int ret;

	/* TODO: Buffer addr that will used as dma addr also should be aligned */
	if (xfer->buf != NULL && (uintptr_t)net_buf_tail(xfer->buf) % 4 != 0) {
		LOG_ERR("XFER buffer address %08lXh is not 4-byte aligned",
			(uintptr_t)net_buf_tail(xfer->buf));
		return -EINVAL;
	}

	if (xfer->interval != 0) {
		LOG_ERR("Periodic transfer is not supported");
		return -EINVAL;
	}

	uhc_lock_internal(dev, K_FOREVER);

	/* TODO: Use bmAttributes from the descriptors from the host class */

	ret = uhc_dwc2_config_hctsiz(dev, chan, buf_size);
	if (ret != 0) {
		goto err;
	}

	sys_write32((uint32_t)xfer->buf->data, (mem_addr_t)&chan_regs->hcdma);

	/* TODO: Configure split transaction if needed */

	hcint = sys_read32((mem_addr_t)&chan_regs->hcint);
	sys_write32(hcint, (mem_addr_t)&chan_regs->hcint);

	/* TODO: sync CACHE */

	hcchar = sys_read32((mem_addr_t)&chan_regs->hcchar);
	hcchar |= USB_DWC2_HCCHAR_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&chan_regs->hcchar);

	ret = 0;
err:
	uhc_unlock_internal(dev);
	return ret;
}

static int uhc_dwc2_enqueue(const struct device *const dev, struct uhc_transfer *const xfer)
{
	uhc_xfer_append(dev, xfer);
	return uhc_dwc2_process_ctrl_xfer(dev, uhc_xfer_get_next(dev));
}

static int uhc_dwc2_dequeue(const struct device *const dev, struct uhc_transfer *const xfer)
{
	LOG_WRN("%s has not been implemented", __func__);

	return -ENOSYS;
}

static int uhc_dwc2_preinit(const struct device *const dev)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);
	struct uhc_data *const data = dev->data;

	/* Initialize the private data structure */
	memset(priv, 0, sizeof(struct uhc_dwc2_data));
	k_mutex_init(&data->mutex);
	k_event_init(&priv->event);

	/* TODO: Overwrite the DWC2 register values with the devicetree values? */

	uhc_dwc2_quirk_caps(dev);

	k_thread_create(&priv->thread, uhc_dwc2_stack, K_THREAD_STACK_SIZEOF(uhc_dwc2_stack),
			uhc_dwc2_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_UHC_DWC2_THREAD_PRIORITY), K_ESSENTIAL, K_NO_WAIT);
	k_thread_name_set(&priv->thread, dev->name);

	return 0;
}

static int uhc_dwc2_init(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	uint32_t reg;
	int ret;

	ret = uhc_dwc2_quirk_init(dev);
	if (ret) {
		LOG_ERR("Quirk init failed %d", ret);
		return ret;
	}

	/* Read hardware configuration registers */

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

	if ((config->ghwcfg4 & USB_DWC2_GHWCFG4_DEDFIFOMODE) == 0) {
		LOG_ERR("Only dedicated TX FIFO mode is supported");
		return -ENOTSUP;
	}

	ret = uhc_dwc2_quirk_phy_pre_select(dev);
	if (ret) {
		LOG_ERR("Quirk PHY pre select failed %d", ret);
		return ret;
	}

	/* Software reset won't finish without PHY clock */
	if (uhc_dwc2_quirk_is_phy_clk_off(dev)) {
		LOG_ERR("PHY clock is  turned off, cannot reset");
		return -EIO;
	}

	/* Reset core after selecting PHY */
	ret = dwc2_hal_core_reset(config->base, K_MSEC(10));
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
	uhc_dwc2_config_fifo_fixed_dma(dev);

	/* Program the GAHBCFG register */
	uhc_dwc2_init_gahbcfg(dev);

	/* Disable RX FIFO level interrupts for the time of the configuration */
	/* TODO */

	/* Configure the reference clock */
	/* TODO */

	/* Program the GUSBCFG register */
	uhc_dwc2_init_gusbcfg(dev);

	/* Disable OTG and mode-mismatch interrupts */
	/* TODO */

	/* Program the HCFG register */
	uhc_dwc2_init_hcfg(dev);

	return 0;
}

static int uhc_dwc2_enable(const struct device *const dev)
{
	int ret;

	ret = uhc_dwc2_quirk_pre_enable(dev);
	if (ret) {
		LOG_ERR("Quirk pre enable failed %d", ret);
		return ret;
	}

	ret = uhc_dwc2_quirk_irq_enable_func(dev);
	if (ret) {
		LOG_ERR("Quirk IRQ enable failed %d", ret);
		return ret;
	}

	ret = uhc_dwc2_power_on(dev);
	if (ret) {
		LOG_ERR("Failed to power on port: %d", ret);
		return ret;
	}

	return 0;
}

static int uhc_dwc2_disable(const struct device *const dev)
{
	int ret;

	LOG_WRN("%s has not been implemented", __func__);

	ret = uhc_dwc2_quirk_disable(dev);
	if (ret) {
		LOG_ERR("Quirk disable failed %d", ret);
		return ret;
	}

	return -ENOSYS;
}

static int uhc_dwc2_shutdown(const struct device *const dev)
{
	int ret;

	LOG_WRN("%s has not been implemented", __func__);

	/* TODO: Release memory for channel handles */

	ret = uhc_dwc2_quirk_shutdown(dev);
	if (ret) {
		LOG_ERR("Quirk shutdown failed %d", ret);
		return ret;
	}

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

#define UHC_DWC2_DT_INST_REG_ADDR(n)                                                               \
	COND_CODE_1(DT_NUM_REGS(DT_DRV_INST(n)),                                                   \
		    (DT_INST_REG_ADDR(n)),                                                         \
		    (DT_INST_REG_ADDR_BY_NAME(n, core)))

static struct uhc_dwc2_data uhc_dwc2_data = {
	.irq_sem = Z_SEM_INITIALIZER(uhc_dwc2_data.irq_sem, 0, 1),
};

static const struct uhc_dwc2_config uhc_dwc2_config_host = {
	.base = (struct usb_dwc2_reg *)UHC_DWC2_DT_INST_REG_ADDR(0),
	.quirks = UHC_DWC2_VENDOR_QUIRK_GET(0),
	.gsnpsid = DT_INST_PROP(0, gsnpsid),
	.ghwcfg1 = DT_INST_PROP(0, ghwcfg1),
	.ghwcfg2 = DT_INST_PROP(0, ghwcfg2),
	.ghwcfg3 = DT_INST_PROP(0, ghwcfg3),
	.ghwcfg4 = DT_INST_PROP(0, ghwcfg4),
};

static struct uhc_data uhc_dwc2_priv_data = {
	.priv = &uhc_dwc2_data,
};

DEVICE_DT_INST_DEFINE(0, uhc_dwc2_preinit, NULL, &uhc_dwc2_priv_data, &uhc_dwc2_config_host,
		      POST_KERNEL, 99, &uhc_dwc2_api);
