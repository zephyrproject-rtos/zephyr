/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
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

#define DEBOUNCE_DELAY_MS       CONFIG_UHC_DWC2_PORT_DEBOUNCE_DELAY_MS
#define RESET_HOLD_MS           CONFIG_UHC_DWC2_PORT_RESET_HOLD_MS
#define RESET_RECOVERY_MS       CONFIG_UHC_DWC2_PORT_RESET_RECOVERY_MS
#define SET_ADDR_DELAY_MS       CONFIG_UHC_DWC2_PORT_SET_ADDR_DELAY_MS

#define CTRL_EP_MAX_MPS_LS      8U
#define CTRL_EP_MAX_MPS_HSFS    64U

enum {
	/* Root port event */
	UHC_DWC2_EVENT_PORT,
	/* Root pipe event */
	UHC_DWC2_EVENT_PIPE,
} uhc_dwc2_event_t;

const char *uhc_dwc2_speed_str[] = {
	"High Speed",
	"Full Speed",
	"Low Speed",
};

typedef enum {
	UHC_DWC2_XFER_TYPE_CTRL = 0,
	UHC_DWC2_XFER_TYPE_ISOCHRONOUS = 1,
	UHC_DWC2_XFER_TYPE_BULK = 2,
	UHC_DWC2_XFER_TYPE_INTR = 3,
} uhc_dwc2_xfer_type_t;

typedef enum {
	UHC_DWC2_SPEED_HIGH = 0,
	UHC_DWC2_SPEED_FULL = 1,
	UHC_DWC2_SPEED_LOW = 2,
} uhc_dwc2_speed_t;
typedef enum {
	/* No event has occurred or the event is no longer valid */
	UHC_PORT_EVENT_NONE,
	/* A device has been connected to the port */
	UHC_PORT_EVENT_CONNECTION,
	/* Device has completed reset and enabled on the port */
	UHC_PORT_EVENT_ENABLED,
	/* A device disconnection has been detected */
	UHC_PORT_EVENT_DISCONNECTION,
	/* Port error detected. Port is now UHC_PORT_STATE_RECOVERY */
	UHC_PORT_EVENT_ERROR,
	/* Overcurrent detected. Port is now UHC_PORT_STATE_RECOVERY */
	UHC_PORT_EVENT_OVERCURRENT,
} uhc_port_event_t;


const char *uhc_port_event_str[] = {
	"None",
	"Connection",
	"Enabled",
	"Disconnection",
	"Error",
	"Overcurrent",
};
typedef enum {
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
} uhc_port_state_t;

const char *uhc_port_state_str[] = {
	"Not Powered",
	"Disconnected",
	"Disabled",
	"Resetting",
	"Suspended",
	"Resuming",
	"Enabled",
	"Recovery",
};
typedef enum {
	/* No event occurred, or could not decode interrupt */
	UHC_DWC2_CORE_EVENT_NONE,
	/* A channel event has occurred. Call the channel event handler instead */
	UHC_DWC2_CORE_EVENT_CHAN,
	/* The host port has detected a connection */
	UHC_DWC2_CORE_EVENT_CONN,
	/* The host port has detected a disconnection */
	UHC_DWC2_CORE_EVENT_DISCONN,
	/* The host port has been enabled (i.e., connected device has been reset. Send SOFs) */
	UHC_DWC2_CORE_EVENT_ENABLED,
	/* The host port has been disabled (no more SOFs)  */
	UHC_DWC2_CORE_EVENT_DISABLED,
	/* The host port has encountered an overcurrent condition */
	UHC_DWC2_CORE_EVENT_OVRCUR,
	/* The host port has been cleared of the overcurrent condition */
	UHC_DWC2_CORE_EVENT_OVRCUR_CLR,
} uhc_dwc2_core_event_t;

const char *dwc2_core_event_str[] = {
	"None",
	"Channel",
	"Connect",
	"Disconnect",
	"Enabled",
	"Disabled",
	"Overcurrent",
	"Overcurrent Cleared",
};

typedef enum {
	PIPE_EVENT_NONE = 0,
	PIPE_EVENT_XFER_DONE,
	PIPE_EVENT_ERROR,
	PIPE_EVENT_HALTED,
} pipe_event_t;

const char *uhc_pipe_event_str[] = {
	"None",
	"XFER Done",
	"Error",
	"Halted",
};

typedef enum {
	/* The channel has completed execution of a transfer. Channel is now halted */
	DWC2_CHAN_EVENT_CPLT,
	/* The channel has encountered an error. Channel is now halted */
	DWC2_CHAN_EVENT_ERROR,
	/* A halt has been requested on the channel */
	DWC2_CHAN_EVENT_HALT_REQ,
	/* No event (interrupt ran for internal processing) */
	DWC2_CHAN_EVENT_NONE,
} dwc2_hal_chan_event_t;

const char *dwc2_chan_event_str[] = {
	"CPLT",
	"ERROR",
	"HALT_REQ",
	"NONE",
};
typedef struct {
	/* Number of available channels */
	size_t numchannels;
	/* High-speed PHY type */
	uint8_t hsphytype;
	/* Full-speed PHY type */
	uint8_t fsphytype;
	/* PHY data width */
	uint8_t phydatawidth;
	/* Buffer DMA mode flag */
	bool bufferdma;
	/* FIFO depth in WORDs */
	uint16_t fifodepth;
	/* TODO: Port context and callback? */
} uhc_dwc2_constant_config_t;

typedef struct {
	union {
		struct {
			uint32_t type: 2;				/**< Type of endpoint */
			uint32_t bEndpointAddress: 8;	/**< Endpoint address */
			uint32_t mps: 11;				/**< Maximum Packet Size */
			uint32_t dev_addr: 8;			/**< Device Address */
			uint32_t ls_via_fs_hub: 1;		/**< LS device is routed via FS hub */
			uint32_t reserved2: 2;
		};
		uint32_t val;
	};
	struct {
		/* Interval in frames (FS) or microframes (HS) */
		unsigned int interval;
		/* Offset in the periodic scheduler */
		uint32_t offset;
		/* High-speed flag */
		bool is_hs;
	} periodic;
} uhc_dwc2_ep_char_t;
typedef enum {
	/* Pipe is active */
	UHC_PIPE_STATE_ACTIVE,
	/* Pipe is halted */
	UHC_PIPE_STATE_HALTED,
} pipe_state_t;

typedef struct {
	union {
		struct {
			uint32_t active: 1;				/**< Is channel enabled */
			uint32_t halt_requested: 1;		/**< Halt has been requested */
			uint32_t reserved: 2;
			uint32_t chan_idx: 4;			/**< The index of the channel */
			uint32_t reserved24: 24;
		};
		uint32_t val;
	} flags;

	/* Pointer to the Host channel's register set */
	const struct usb_dwc2_host_chan *regs;

	/* TODO: Add channel error? */

	/* The transfer type of the channel */
	uhc_dwc2_xfer_type_t type;

	/* Context variable for the owner of the channel */
	void *chan_ctx;
} uhc_dwc2_channel_t;

typedef enum {
  CTRL_STAGE_DATA0 = 0,
  CTRL_STAGE_DATA2 = 1,
  CTRL_STAGE_DATA1 = 2,
  CTRL_STAGE_SETUP = 3,
} ctrl_stage_t;

const char *pipe_buffer_stage_str[] = {
	"Data0",
	"Data2",
	"Data1",
	"Setup",
};

typedef struct {
	uhc_dwc2_speed_t dev_speed;			/**< Speed of the device */
	uint8_t dev_addr;					/**< Device address */
} uhc_pipe_config_t;

typedef enum {
	USB_TRANSFER_TYPE_CTRL = 0,
	USB_TRANSFER_TYPE_ISOCHRONOUS,
	USB_TRANSFER_TYPE_BULK,
	USB_TRANSFER_TYPE_INTR,
} usb_transfer_type_t;

/**
 * @brief Object representing a buffer of a pipe's sindle or multi buffer implementation
 */
typedef struct {
	/* Pointer to the transfer associated with the buffer */
	struct uhc_transfer *xfer;
	union {
		struct {
			uint32_t data_stg_in: 1;		/**< Data stage is IN */
			uint32_t data_stg_skip: 1;		/**< Has no data stage */
			uint32_t cur_stg: 2;			/**< Stage index */
			uint32_t set_addr: 1;			/**< Set address request */
			uint32_t new_addr: 7;			/**< New address */
			uint32_t reserved20: 20;
		} ctrl;
		uint32_t val;
	} flags;
	union {
		struct {
			uint32_t executing: 1;
			uint32_t was_canceled: 1;
			uint32_t reserved6: 6;
			uint32_t reserved8_1: 8;
			uint32_t pipe_event: 8;
			uint32_t reserved8_2: 8;
		};
		uint32_t val;
	} status_flags;
} dma_buffer_t;

struct pipe_obj_s {
	/* XFER queuing related */
	sys_dlist_t xfer_pending_list;

	/* TODO: Lists of pending and done? */
	int num_xfer_pending;
	int num_xfer_done;

	/* Single-buffer control */
	dma_buffer_t *buffer;

	/* HAL related */
	uhc_dwc2_channel_t *chan_obj;
	uhc_dwc2_ep_char_t ep_char;

	/* Pipe status/state/events related */
	pipe_state_t state;
	pipe_event_t last_event;
	union {
		struct {
			uint32_t waiting_halt: 1;
			uint32_t pipe_cmd_processing: 1;
			uint32_t has_xfer: 1;		/**< XFER: pending, in-flight or done */
			uint32_t event_pending: 1;	/**< Pipe event is pending */
			uint32_t reserved28: 28;
		};
		uint32_t val;
	} flags;
};

typedef struct pipe_obj_s pipe_t;
typedef pipe_t *pipe_hdl_t;

typedef struct {
	uint16_t top;
	uint16_t nptxfsiz;
	uint16_t rxfsiz;
	uint16_t ptxfsiz;
} uhc_dwc2_fifo_config_t;

struct uhc_dwc2_data_s {
	struct k_sem irq_sem;
	/* TODO: spinlock? */
	struct k_thread thread_data;
	/* Mutex for port access */
	struct k_mutex mutex;
	/* Main events the driver thread waits for */
	struct k_event drv_evt;

	/* TODO: FRAME LIST? */
	/* TODO: Pipes/channels LIST? */

	/* FIFO */
	uhc_dwc2_fifo_config_t fifo;

	/* Data, that doesn't changed after initialization */
	uhc_dwc2_constant_config_t const_cfg;
	struct {
		/* Number of channels currently allocated */
		size_t num_allocated;
		/* Bit mask of channels with pending interrupts */
		uint32_t pending_intrs_msk;
		/* Handles of each channel */
		uhc_dwc2_channel_t **hdls;
	} channels;

	struct {
		union {
			struct {
				uint32_t lock_enabled: 1;			/**< Debounce lock */
				uint32_t event_pending: 1;			/**< Port event is pending */
				uint32_t conn_dev_ena: 1;			/**< Device is connected */
				uint32_t waiting_disable: 1;		/**< Waiting to be disabled */
				uint32_t reserved: 4;
				uint32_t reserved24: 24;
			};
			uint32_t val;
		} flags;
		uhc_port_event_t last_event;
		uhc_port_state_t port_state;
	} dynamic; /* Data, that is used in multiple threads */

	/* Number of idle pipes */
	uint8_t num_pipes_idle;
	/* Number of pipes queued for processing */
	uint8_t num_pipes_queued;

	/* TODO: Dynamic pipe allocation on enqueue? */
	pipe_t pipe;
	pipe_hdl_t ctrl_pipe_hdl;
};

/* Host channel registers address */
#define UHC_DWC2_CHAN_REG(base, chan_idx)    ((struct usb_dwc2_host_chan *)(((mem_addr_t)(base)) + 0x500UL + ((chan_idx)*0x20UL)))

/* =============================================================================================== */
/* ================================ DWC2 FIFO Management ========================================= */
/* =============================================================================================== */

/* Programming Guide 2.1.2 FIFO RAM allocation
 * RX
 * - Largest-EPsize/4 + 2 (status info). recommended x2 if high bandwidth or multiple ISO are used.
 * - 2 for transfer complete and channel halted status
 * - 1 for each Control/Bulk out endpoint to Handle NAK/NYET (i.e max is number of host channel)
 *
 * TX non-periodic (NPTX)
 * - At least largest-EPsize/4, recommended x2
 *
 * TX periodic (PTX)
 * - At least largest-EPsize*MulCount/4 (MulCount up to 3 for high-bandwidth ISO/interrupt)
*/
enum {
  EPSIZE_BULK_FS = 64,
  EPSIZE_BULK_HS = 512,

  EPSIZE_ISO_FS_MAX = 1023,
  EPSIZE_ISO_HS_MAX = 1024,
};

static inline void uhc_dwc2_config_fifo_fixed_dma(const uhc_dwc2_constant_config_t *const_cfg,
											uhc_dwc2_fifo_config_t *fifo)
{
	LOG_DBG("Configuring FIFO sizes");
	fifo->top = const_cfg->fifodepth;
	fifo->top -= const_cfg->numchannels;

	/* TODO: support HS */

	uint32_t nptx_largest = EPSIZE_BULK_FS / 4;
	uint32_t ptx_largest = 256 / 4;

	fifo->nptxfsiz = 2 * nptx_largest;
	fifo->rxfsiz = 2 * (ptx_largest + 2) + const_cfg->numchannels;
	fifo->ptxfsiz = fifo->top - (fifo->nptxfsiz + fifo->rxfsiz);

	/* TODO: verify ptxfsiz is overflowed */

	LOG_DBG("FIFO sizes calculated");
	LOG_DBG("\ttop=%u, nptx=%u, rx=%u, ptx=%u",
		fifo->top * 4,  fifo->nptxfsiz * 4, fifo->rxfsiz * 4, fifo->ptxfsiz * 4);
}

/* =============================================================================================== */
/* =================================== DWC2 HAL Functions ======================================== */
/* =============================================================================================== */

/*
 * Common functions for both DWC2 device and host driver
 */

void dwc2_hal_flush_rx_fifo(struct usb_dwc2_reg *const dwc2)
{
	mem_addr_t grstctl_reg = (mem_addr_t)&dwc2->grstctl;

	sys_write32(USB_DWC2_GRSTCTL_RXFFLSH, grstctl_reg);
	while (sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_RXFFLSH) {
	}
}

void dwc2_hal_flush_tx_fifo(struct usb_dwc2_reg *const dwc2, const uint8_t fnum)
{
	mem_addr_t grstctl_reg = (mem_addr_t)&dwc2->grstctl;
	uint32_t grstctl;

	grstctl = usb_dwc2_set_grstctl_txfnum(fnum) | USB_DWC2_GRSTCTL_TXFFLSH;

	sys_write32(grstctl, grstctl_reg);
	while (sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_TXFFLSH) {
	}
}

int dwc2_hal_core_reset(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;

	mem_addr_t grstctl_reg = (mem_addr_t)&dwc2->grstctl;
	const unsigned int csr_timeout_us = 10000UL;
	uint32_t cnt = 0UL;

	/* Check AHB master idle state */
	while (!(sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_AHBIDLE)) {
		k_busy_wait(1);

		if (++cnt > csr_timeout_us) {
			LOG_ERR("Wait for AHB idle timeout, GRSTCTL 0x%08x",
				sys_read32(grstctl_reg));
			return -EIO;
		}
	}

	/* Apply Core Soft Reset */
	sys_write32(USB_DWC2_GRSTCTL_CSFTRST, grstctl_reg);

	cnt = 0UL;
	do {
		if (++cnt > csr_timeout_us) {
			LOG_ERR("Wait for CSR done timeout, GRSTCTL 0x%08x",
				sys_read32(grstctl_reg));
			return -EIO;
		}

		k_busy_wait(1);
		if (uhc_dwc2_quirk_is_phy_clk_off(dev)) {
			/* Software reset won't finish without PHY clock */
			return -EIO;
		}
	} while (sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_CSFTRST &&
		 !(sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_CSFTRSTDONE));

	/* CSFTRSTDONE is W1C so the write must have the bit set to clear it */
	sys_clear_bits(grstctl_reg, USB_DWC2_GRSTCTL_CSFTRST);

	LOG_DBG("DWC2 core reset done");

	return 0;
}

static inline void dwc2_hal_set_frame_list(struct usb_dwc2_reg *const dwc2, void *frame_list)
{
	LOG_WRN("Setting frame list not implemented yet");
}

static inline void dwc2_hal_periodic_enable(struct usb_dwc2_reg *const dwc2)
{
	LOG_WRN("Enabling periodic scheduling not implemented yet");
}

static inline void dwc2_hal_port_init(struct usb_dwc2_reg *const dwc2)
{
	sys_clear_bits((mem_addr_t)&dwc2->haintmsk, 0xFFFFFFFFUL);
	sys_set_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_PRTINT | USB_DWC2_GINTSTS_HCHINT);
}

#define USB_DWC2_HPRT_W1C_MSK             (USB_DWC2_HPRT_PRTENA | 		\
	                                       USB_DWC2_HPRT_PRTCONNDET | 	\
										   USB_DWC2_HPRT_PRTENCHNG | 	\
										   USB_DWC2_HPRT_PRTOVRCURRCHNG)

static inline void dwc2_hal_toggle_reset(struct usb_dwc2_reg *const dwc2, bool reset_on)
{
	uint32_t hprt = sys_read32((mem_addr_t)&dwc2->hprt);

	if (reset_on) {
		hprt |= USB_DWC2_HPRT_PRTRST;
	} else {
		hprt &= ~USB_DWC2_HPRT_PRTRST;
	}

	sys_write32(hprt & (~USB_DWC2_HPRT_W1C_MSK), (mem_addr_t)&dwc2->hprt);
}

static inline void dwc2_hal_toggle_power(struct usb_dwc2_reg *const dwc2, bool power_on)
{
	uint32_t hprt = sys_read32((mem_addr_t)&dwc2->hprt);

	if (power_on) {
		hprt |= USB_DWC2_HPRT_PRTPWR;
	} else {
		hprt &= ~USB_DWC2_HPRT_PRTPWR;
	}

	sys_write32(hprt & (~USB_DWC2_HPRT_W1C_MSK), (mem_addr_t)&dwc2->hprt);
}

static inline int dwc2_hal_get_config(struct usb_dwc2_reg *const dwc2,
	 uhc_dwc2_constant_config_t *const cfg)
{
	uint32_t gsnpsid = sys_read32((mem_addr_t)&dwc2->gsnpsid);
	uint32_t ghwcfg2 = sys_read32((mem_addr_t)&dwc2->ghwcfg2);
	uint32_t ghwcfg3 = sys_read32((mem_addr_t)&dwc2->ghwcfg3);
	uint32_t ghwcfg4 = sys_read32((mem_addr_t)&dwc2->ghwcfg4);

	LOG_DBG("GSNPSID=%08Xh, GHWCFG2=%08Xh, GHWCFG3=%08Xh, GHWCFG4=%08Xh",
			gsnpsid, ghwcfg2, ghwcfg3, ghwcfg4);

	/* Check Synopsis ID register, failed if controller clock/power is not enabled */
	__ASSERT((gsnpsid == USB_DWC2_GSNPSID_REV_5_00A),
			 "DWC2 core ID is not compatible with the driver, GSNPSID: 0x%08x",
			 gsnpsid);

	if (gsnpsid == 0) {
		LOG_ERR("Unable to read DWC2 Core ID, core is not powered on");
		return -ENODEV;
	}

	if (!(ghwcfg4 & USB_DWC2_GHWCFG4_DEDFIFOMODE)) {
		LOG_ERR("Only dedicated TX FIFO mode is supported");
		return -ENOTSUP;
	}

	/* Buffer DMA is always supported in Internal DMA mode.
	 * TODO: check and support descriptor DMA if available
	 */
	cfg->bufferdma = (usb_dwc2_get_ghwcfg2_otgarch(ghwcfg2) ==
			USB_DWC2_GHWCFG2_OTGARCH_INTERNALDMA);

	if (IS_ENABLED(CONFIG_UHC_DWC2_DMA)) {
		if (cfg->bufferdma) {
			LOG_DBG("Buffer DMA enabled");
		}
	} else {
		cfg->bufferdma = 0;
	}

	if (ghwcfg2 & USB_DWC2_GHWCFG2_DYNFIFOSIZING) {
		LOG_DBG("Dynamic FIFO Sizing is enabled");
		/* TODO: support FIFO dynamic sizing */
	}

	/* TODO: Support hybernation */

	LOG_DBG("OTG architecture (OTGARCH) %u, mode (OTGMODE) %u",
		usb_dwc2_get_ghwcfg2_otgarch(ghwcfg2),
		usb_dwc2_get_ghwcfg2_otgmode(ghwcfg2));

	cfg->fifodepth = usb_dwc2_get_ghwcfg3_dfifodepth(ghwcfg3);
	LOG_DBG("DFIFO depth (DFIFODEPTH) %u bytes", cfg->fifodepth * 4);

	/* TODO: Support vendor control interface */
	LOG_DBG("Vendor Control interface support enabled: %s",
		(ghwcfg3 & USB_DWC2_GHWCFG3_VNDCTLSUPT) ? "true" : "false");

	LOG_DBG("PHY interface type: FSPHYTYPE %u, HSPHYTYPE %u, DATAWIDTH %u",
		usb_dwc2_get_ghwcfg2_fsphytype(ghwcfg2),
		usb_dwc2_get_ghwcfg2_hsphytype(ghwcfg2),
		usb_dwc2_get_ghwcfg4_phydatawidth(ghwcfg4));

	/* TODO: Support LPM */
	LOG_DBG("LPM mode is %s",
		(ghwcfg3 & USB_DWC2_GHWCFG3_LPMMODE) ? "enabled" : "disabled");

	if (ghwcfg3 & USB_DWC2_GHWCFG3_RSTTYPE) {
		/* TODO: Support sync reset */
	}

	/* TODO: Support dedicated FIFO mode */

	cfg->hsphytype = usb_dwc2_get_ghwcfg2_hsphytype(ghwcfg2);
	cfg->fsphytype = usb_dwc2_get_ghwcfg2_fsphytype(ghwcfg2);
	cfg->phydatawidth = usb_dwc2_get_ghwcfg4_phydatawidth(ghwcfg4);
	cfg->numchannels = usb_dwc2_get_ghwcfg2_numhstchnl(ghwcfg2) + 1U;

	LOG_DBG("PHY interface type: FSPHYTYPE %u, HSPHYTYPE %u, DATAWIDTH %u",
		cfg->fsphytype,
		cfg->hsphytype,
		cfg->phydatawidth);

	LOG_DBG("Number of host channels (NUMHSTCHNL + 1) %u", cfg->numchannels);

	return 0;
}

static void dwc2_hal_channel_configure(uhc_dwc2_channel_t *chan_obj, uhc_dwc2_ep_char_t *ep_char)
{
	__ASSERT(!chan_obj->flags.active && !chan_obj->flags.halt_requested,
			 "Cannot change endpoint characteristics while channel is active or halted");

	mem_addr_t hcchar_reg = (mem_addr_t)&chan_obj->regs->hcchar;

	uint32_t hcchar = ((uint32_t)ep_char->mps << USB_DWC2_HCCHAR0_MPS_POS) |
			((uint32_t)USB_EP_GET_IDX(ep_char->bEndpointAddress) << USB_DWC2_HCCHAR0_EPNUM_POS) |
			((uint32_t)ep_char->type << USB_DWC2_HCCHAR0_EPTYPE_POS) |
			((uint32_t)1UL /* TODO: ep_char->mult */ << USB_DWC2_HCCHAR0_EC_POS)  |
			((uint32_t)ep_char->dev_addr << USB_DWC2_HCCHAR0_DEVADDR_POS);

	if (USB_EP_DIR_IS_IN(ep_char->bEndpointAddress)) {
		hcchar |= USB_DWC2_HCCHAR0_EPDIR;
	}

	/* TODO: LS device plugged to HUB */
	if (false) {
		hcchar |= USB_DWC2_HCCHAR0_LSPDDEV;
	}

	if (ep_char->type == UHC_DWC2_XFER_TYPE_INTR) {
		hcchar |= USB_DWC2_HCCHAR0_ODDFRM;
	}

	sys_write32(hcchar, hcchar_reg);

	chan_obj->type = ep_char->type;

	if (ep_char->type == UHC_DWC2_XFER_TYPE_ISOCHRONOUS ||
		ep_char->type == UHC_DWC2_XFER_TYPE_INTR) {
		LOG_WRN("ISOC and INTR channels are note supported yet");
	}
}

static inline void dwc2_hal_apply_fifo_config(struct usb_dwc2_reg *const dwc2,
	uhc_dwc2_fifo_config_t *const fifo)
{
	/* Get FIFO top from config */
	uint16_t fifo_available = fifo->top;

	sys_write32(fifo_available << USB_DWC2_GDFIFOCFG_EPINFOBASEADDR_POS | fifo_available,
		(mem_addr_t)&dwc2->gdfifocfg);

	fifo_available -= fifo->rxfsiz;

	sys_write32(fifo->rxfsiz << USB_DWC2_GRXFSIZ_RXFDEP_POS, (mem_addr_t)&dwc2->grxfsiz);

	fifo_available -= fifo->nptxfsiz;

	sys_write32(fifo->nptxfsiz << USB_DWC2_GNPTXFSIZ_NPTXFDEP_POS | fifo_available,
		(mem_addr_t)&dwc2->gnptxfsiz);

	fifo_available -= fifo->ptxfsiz;

	sys_write32(fifo->ptxfsiz << USB_DWC2_HPTXFSIZ_PTXFSIZE_POS | fifo_available,
		(mem_addr_t)&dwc2->hptxfsiz);

	dwc2_hal_flush_tx_fifo(dwc2, 0x10UL);
	dwc2_hal_flush_rx_fifo(dwc2);

	LOG_DBG("FIFO configuration applied");
	LOG_DBG("\tnptx=%u, rx=%u, ptx=%u", fifo->nptxfsiz * 4, fifo->rxfsiz * 4, fifo->ptxfsiz * 4);
}

static inline uhc_dwc2_speed_t dwc2_hal_get_port_speed(struct usb_dwc2_reg *const dwc2)
{
	uint32_t hprt = sys_read32((mem_addr_t)&dwc2->hprt);
	return (hprt & USB_DWC2_HPRT_PRTSPD_MASK) >> USB_DWC2_HPRT_PRTSPD_POS;
}

static inline int uhc_dwc2_get_port_speed(const struct device *dev, uhc_dwc2_speed_t *speed)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	if (priv->dynamic.port_state != UHC_PORT_STATE_ENABLED) {
		LOG_ERR("Port is not enabled, cannot get speed");
		return -ENODEV;
	}

	*speed = dwc2_hal_get_port_speed(dwc2);

	return 0;
}

static inline void dwc2_hal_port_enable(const struct device *dev, struct usb_dwc2_reg *const dwc2)
{
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	uhc_dwc2_speed_t speed = dwc2_hal_get_port_speed(dwc2);
	uint32_t hcfg = sys_read32((mem_addr_t)&dwc2->hcfg);
	uint32_t hfir = sys_read32((mem_addr_t)&dwc2->hfir);

	/* We can select Buffer DMA of Scatter-Gather DMA mode here: Buffer DMA by default */
	hcfg &= ~USB_DWC2_HCFG_DESCDMA;

	/* Disable periodic scheduling, will enable later */
	hcfg &= ~USB_DWC2_HCFG_PERSCHEDENA;

	if (priv->const_cfg.hsphytype == 0) {
		/*
		Indicate to the OTG core what speed the PHY clock is at
		Note: FSLS PHY has an implicit 8 divider applied when in LS mode,
		so the values of FSLSPclkSel and FrInt have to be adjusted accordingly.
		*/
		uint8_t fslspclksel = (speed == UHC_DWC2_SPEED_FULL) ? 1 : 2;
		hcfg &= ~USB_DWC2_HCFG_FSLSPCLKSEL_MASK;
		hcfg |= (fslspclksel << USB_DWC2_HCFG_FSLSPCLKSEL_POS);

		/* Disable dynamic loading */
		hfir &= ~USB_DWC2_HFIR_HFIRRLDCTRL;
		/*
		Set frame interval to be equal to 1ms
		Note: FSLS PHY has an implicit 8 divider applied when in LS mode,
			so the values of FSLSPclkSel and FrInt have to be adjusted accordingly.
		*/
		uint16_t frint = (speed == UHC_DWC2_SPEED_FULL) ? 48000 : 6000;
		hfir &= ~USB_DWC2_HFIR_FRINT_MASK;
		hfir |= (frint << USB_DWC2_HFIR_FRINT_POS);

		sys_write32(hcfg, (mem_addr_t)&dwc2->hcfg);
		sys_write32(hfir, (mem_addr_t)&dwc2->hfir);
	} else {
		LOG_ERR("Configuring clocks for HS PHY is not implemented");
	}

}

/* =============================================================================================== */
/* ================================== DWC2 Port Management ======================================= */
/* =============================================================================================== */

/* Host Port Control and Status Register */
#define USB_DWC2_HPRT_PRTENCHNG               BIT(3)
#define USB_DWC2_HPRT_PRTOVRCURRCHNG          BIT(5)
#define USB_DWC2_HPRT_PRTCONNDET              BIT(1)

#define CORE_INTRS_EN_MSK   (USB_DWC2_GINTSTS_DISCONNINT)

/* Interrupts that pertain to core events */
#define CORE_EVENTS_INTRS_MSK (USB_DWC2_GINTSTS_DISCONNINT | \
							   USB_DWC2_GINTSTS_HCHINT)

/* Interrupt that pertain to host port events */
#define PORT_EVENTS_INTRS_MSK (USB_DWC2_HPRT_PRTCONNDET | \
							   USB_DWC2_HPRT_PRTENCHNG | \
							   USB_DWC2_HPRT_PRTOVRCURRCHNG)

static void uhc_dwc2_lock_enable(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data_s *const priv = uhc_get_private(dev);
	/* Disable Connection and disconnection interrupts to prevent spurious events */
	sys_clear_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_PRTINT |
												USB_DWC2_GINTSTS_DISCONNINT);
	priv->dynamic.flags.lock_enabled = 1;
}

static inline void uhc_dwc2_lock_disable(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data_s *const priv = uhc_get_private(dev);
	priv->dynamic.flags.lock_enabled = 0;
	/* Clear Connection and disconnection interrupt in case it triggered again */
	sys_set_bits((mem_addr_t)&dwc2->gintsts, USB_DWC2_GINTSTS_DISCONNINT);
	/* Clear the PRTCONNDET interrupt by writing 1 to the corresponding bit (W1C logic) */
	sys_set_bits((mem_addr_t)&dwc2->hprt, USB_DWC2_HPRT_PRTCONNDET);
	/* Re-enable the HPRT (connection) and disconnection interrupts */
	sys_set_bits((mem_addr_t)&dwc2->gintmsk, USB_DWC2_GINTSTS_PRTINT |
												USB_DWC2_GINTSTS_DISCONNINT);
}

static int uhc_dwc2_power_on(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	/* Port can only be powered on if it's currently unpowered */
	if (priv->dynamic.port_state == UHC_PORT_STATE_NOT_POWERED) {
		priv->dynamic.port_state = UHC_PORT_STATE_DISCONNECTED;
		dwc2_hal_port_init(dwc2);
		dwc2_hal_toggle_power(dwc2, true);
		return 0;
	}

	return -EINVAL;
}

static inline int uhc_dwc2_config_phy(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	/* Init PHY based on the speed */
	int ret;

	if (priv->const_cfg.hsphytype != 0) {
		uint32_t gusbcfg = sys_read32((mem_addr_t)&dwc2->gusbcfg);

		/* De-select FS PHY */
		gusbcfg &= ~USB_DWC2_GUSBCFG_PHYSEL_USB11;

		if (priv->const_cfg.hsphytype == USB_DWC2_GHWCFG2_HSPHYTYPE_ULPI) {
			LOG_WRN("Highspeed ULPI PHY init");
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
			LOG_WRN("Highspeed UTMI+ PHY init");
			/* Select UTMI+ PHY (internal) */
			gusbcfg &= ~USB_DWC2_GUSBCFG_ULPI_UTMI_SEL_ULPI;
			/* Set 16-bit interface if supported */
			if (priv->const_cfg.phydatawidth) {
				gusbcfg |= USB_DWC2_GUSBCFG_PHYIF_16_BIT;
			} else {
				gusbcfg &= ~USB_DWC2_GUSBCFG_PHYIF_16_BIT;
			}
		}
		sys_write32(gusbcfg, (mem_addr_t)&dwc2->gusbcfg);

		ret = uhc_dwc2_quirk_phy_pre_select(dev);
		if (ret) {
			LOG_ERR("Quirk PHY pre select failed %d", ret);
			return ret;
		}

		/* Reset core after selecting PHY */
		ret = dwc2_hal_core_reset(dev);
		if (ret) {
			LOG_ERR("DWC2 core reset failed after PHY init: %d", ret);
			return ret;
		}

		ret = uhc_dwc2_quirk_phy_post_select(dev);
		if (ret) {
			LOG_ERR("Quirk PHY post select failed %d", ret);
			return ret;
		}
	} else {
		sys_set_bits((mem_addr_t)&dwc2->gusbcfg, USB_DWC2_GUSBCFG_PHYSEL_USB11);

		ret = uhc_dwc2_quirk_phy_pre_select(dev);
		if (ret) {
			LOG_ERR("Quirk PHY pre select failed %d", ret);
			return ret;
		}

		/* Reset core after selecting PHY */
		ret = dwc2_hal_core_reset(dev);
		if (ret) {
			LOG_ERR("DWC2 core reset failed after PHY init: %d", ret);
			return ret;
		}

		ret = uhc_dwc2_quirk_phy_post_select(dev);
		if (ret) {
			LOG_ERR("Quirk PHY post select failed %d", ret);
			return ret;
		}
	}

	return ret;
}

static inline void uhc_dwc2_set_defaults(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

	/* Disable Global Interrupt */
	sys_clear_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);

	/* Enable Host mode */
	sys_set_bits((mem_addr_t)&dwc2->gusbcfg, USB_DWC2_GUSBCFG_FORCEHSTMODE);
	/* Wait until core is in host mode */
	while ((sys_read32((mem_addr_t)&dwc2->gintsts) & USB_DWC2_GINTSTS_CURMOD) != 1) {
	}

	/* TODO: Set AHB burst mode for some ECO only for ESP32S2 */
	/* Make config quirk? */

	/* TODO: Disable HNP and SRP capabilities */
	/* Also move to quirk? */

	sys_clear_bits((mem_addr_t)&dwc2->gintmsk, 0xFFFFFFFFUL);

	sys_set_bits((mem_addr_t)&dwc2->gintmsk, CORE_INTRS_EN_MSK);

	/* Clear status */
	uint32_t core_intrs = sys_read32((mem_addr_t)&dwc2->gintsts);
	sys_write32(core_intrs, (mem_addr_t)&dwc2->gintsts);

	/* Configure AHB */
	uint32_t gahbcfg = sys_read32((mem_addr_t)&dwc2->gahbcfg);
	gahbcfg |= USB_DWC2_GAHBCFG_NPTXFEMPLVL;
	gahbcfg &= ~USB_DWC2_GAHBCFG_HBSTLEN_MASK;
	gahbcfg |= (USB_DWC2_GAHBCFG_HBSTLEN_INCR16 << USB_DWC2_GAHBCFG_HBSTLEN_POS);
	sys_write32(gahbcfg, (mem_addr_t)&dwc2->gahbcfg);

	if (priv->const_cfg.bufferdma) {
		sys_set_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_DMAEN);
	}

	/* Enable Global Interrupt */
	sys_set_bits((mem_addr_t)&dwc2->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);
}

static int uhc_dwc2_init_controller(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	int ret;

	/* Get hardware configuration */
	ret = dwc2_hal_get_config(dwc2, &priv->const_cfg);
	if (ret) {
		LOG_ERR("Failed to get DWC2 core parameters: %d", ret);
		return ret;
	}

	/* Pre-calculate FIFO settings */
	uhc_dwc2_config_fifo_fixed_dma(&priv->const_cfg, &priv->fifo);

	/* Config PHY */
	ret = uhc_dwc2_config_phy(dev);
	if (ret) {
		LOG_ERR("Failed to configure DWC2 PHY: %d", ret);
		return ret;
	}

	/* Set defaults */
	uhc_dwc2_set_defaults(dev);

	/* Update the port state and flags */
	priv->dynamic.port_state = UHC_PORT_STATE_NOT_POWERED;
	priv->dynamic.last_event = UHC_PORT_EVENT_NONE;
	priv->dynamic.flags.val = 0;

	/* TODO: Clear all the flags and channels */
	priv->channels.num_allocated = 0;
	priv->channels.pending_intrs_msk = 0;
	if (priv->channels.hdls) {
	    for (int i = 0; i < priv->const_cfg.numchannels; i++) {
	        priv->channels.hdls[i] = NULL;
	    }
	}

	return ret;
}

static uhc_port_event_t uhc_dwc2_decode_hprt(const struct device *dev,
	 uhc_dwc2_core_event_t core_event)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	uhc_port_event_t port_event = UHC_PORT_EVENT_NONE;
	switch (core_event) {
		case UHC_DWC2_CORE_EVENT_CONN: {
			port_event = UHC_PORT_EVENT_CONNECTION;
			break;
		}
		case UHC_DWC2_CORE_EVENT_DISCONN: {
			/* TODO: priv->dynamic.port_state = UHC_PORT_STATE_RECOVERY */
			port_event = UHC_PORT_EVENT_DISCONNECTION;
			priv->dynamic.flags.conn_dev_ena = 0;
			break;
		}
		case UHC_DWC2_CORE_EVENT_ENABLED: {
			/* Initialize remaining host port registers */
			dwc2_hal_port_enable(dev, dwc2);
			port_event = UHC_PORT_EVENT_ENABLED;
			priv->dynamic.flags.conn_dev_ena = 1;
			break;
		}
		case UHC_DWC2_CORE_EVENT_DISABLED: {
			priv->dynamic.flags.conn_dev_ena = 0;
			/* Could be due to a disable request or reset request, or due to a port error */
			/* Ignore the disable event if it's due to a reset request */
			if (priv->dynamic.port_state != UHC_PORT_STATE_RESETTING) {
				if (priv->dynamic.flags.waiting_disable) {
					/* Disabled by request (i.e. by port command). Generate an internal event */
					priv->dynamic.port_state = UHC_PORT_STATE_DISABLED;
					priv->dynamic.flags.waiting_disable = 0;
					/* TODO: Notify the port event from ISR */
					LOG_ERR("Port disabled by request, not implemented yet");
				} else {
					/* Disabled due to a port error */
					LOG_ERR("Port disabled due to an error, changing state to recovery");
					priv->dynamic.port_state = UHC_PORT_STATE_RECOVERY;
					port_event = UHC_PORT_EVENT_ERROR;
				}
			}
			break;
		}
		case UHC_DWC2_CORE_EVENT_OVRCUR:
		case UHC_DWC2_CORE_EVENT_OVRCUR_CLR: {
			/* TODO: Handle overcurrent event */

			/*
			 * If port state powered, we need to power it off to protect it
			 * change port state to recovery
			 * generate port event UHC_PORT_EVENT_OVERCURRENT
			 * disable the flag conn_dev_ena
			*/

			LOG_ERR("Overcurrent detected on port, not implemented yet");
			break;
		}
		default: {
			__ASSERT(false,
				 "uhc_dwc2_decode_hprt: Unexpected core event %d", core_event);
			break;
		}
	}
	return port_event;
}

static inline uhc_dwc2_core_event_t uhc_dwc2_decode_intr(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;
	mem_addr_t hprt_reg = (mem_addr_t)&dwc2->hprt;

	uhc_dwc2_core_event_t core_event = UHC_DWC2_CORE_EVENT_NONE;
	/* Read and clear core interrupt status */
	uint32_t core_intrs = sys_read32((mem_addr_t)&dwc2->gintsts);
	sys_write32(core_intrs, (mem_addr_t)&dwc2->gintsts);

	uint32_t port_intrs = 0;

	if (core_intrs & USB_DWC2_GINTSTS_PRTINT) {
		port_intrs = sys_read32(hprt_reg);
		/* Clear the interrupt status by writing 1 to the W1C bits, except the PRTENA bit */
		sys_write32(port_intrs & (~USB_DWC2_HPRT_PRTENA), hprt_reg);
	}

	LOG_DBG("GINTSTS=%08Xh, HPRT=%08Xh", core_intrs, port_intrs);

	/*
	 * Note:
	 * ENABLED < DISABLED < CONN < DISCONN < OVRCUR
	 * The order of events is important, as some events take precedence over others.
	 * Do not change order of checks. Regressing events (e.g. enable -> disabled,
	 * connected -> connected) always take precedence.
	 */
	if ((core_intrs & CORE_EVENTS_INTRS_MSK) || (port_intrs & PORT_EVENTS_INTRS_MSK)) {
		if (core_intrs & USB_DWC2_GINTSTS_DISCONNINT) {
			/* Disconnect event */
			core_event = UHC_DWC2_CORE_EVENT_DISCONN;
			/* Debounce lock */
			uhc_dwc2_lock_enable(dev);
		} else {
			/* Port still connected, check port event */
			if (port_intrs & USB_DWC2_HPRT_PRTOVRCURRCHNG) {
				/* Check if this is an overcurrent or an overcurrent cleared */
				if (port_intrs & USB_DWC2_HPRT_PRTOVRCURRACT) {
					/* TODO: Verify handling logic during overcurrent */
					core_event = UHC_DWC2_CORE_EVENT_OVRCUR;
				} else {
					core_event = UHC_DWC2_CORE_EVENT_OVRCUR_CLR;
				}
			} else if (port_intrs & USB_DWC2_HPRT_PRTENCHNG) {
				if (port_intrs & USB_DWC2_HPRT_PRTENA) {
					/* Host port was enabled */
					core_event = UHC_DWC2_CORE_EVENT_ENABLED;
				} else {
					/* Host port has been disabled */
					core_event = UHC_DWC2_CORE_EVENT_DISABLED;
				}
			} else if (port_intrs & USB_DWC2_HPRT_PRTCONNDET && !priv->dynamic.flags.lock_enabled) {
				core_event = UHC_DWC2_CORE_EVENT_CONN;
				/* Debounce lock */
				uhc_dwc2_lock_enable(dev);
			} else {
				/* Should never happened, as port event masked with PORT_EVENTS_INTRS_MSK */
				__ASSERT(false,
					 "uhc_dwc2_decode_intr: Unknown port interrupt, HPRT=%08Xh", port_intrs);
			}
		}
	}
	/* Port events always take precedence over channel events */
	if (core_event == UHC_DWC2_CORE_EVENT_NONE && (core_intrs & USB_DWC2_GINTSTS_HCHINT)) {
		/* One or more channels have pending interrupts. Store the mask of those channels */
		priv->channels.pending_intrs_msk = sys_read32((mem_addr_t)&dwc2->haint);
		core_event = UHC_DWC2_CORE_EVENT_CHAN;
	}

	return core_event;
}

uhc_dwc2_channel_t *uhc_dwc2_get_chan_pending_intr(const struct device *dev)
{
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

	if (priv->channels.hdls == NULL) {
		LOG_WRN("uhc_dwc2_get_chan_pending_intr: No channels allocated");
		return NULL;
	}

	int chan_num = __builtin_ffs(priv->channels.pending_intrs_msk);
	if (chan_num) {
		/* Clear the pending bit for that channel */
		priv->channels.pending_intrs_msk &= ~(1 << (chan_num - 1));
		return priv->channels.hdls[chan_num - 1];
	} else {
		return NULL;
	}
}

static inline void *uhc_dwc2_chan_get_context(uhc_dwc2_channel_t *chan_obj)
{
	if (chan_obj == NULL) {
		LOG_ERR("uhc_dwc2_chan_get_context: Channel object is NULL");
		return NULL;
	}
	return chan_obj->chan_ctx;
}

dwc2_hal_chan_event_t uhc_dwc2_hal_chan_decode_intr(uhc_dwc2_channel_t *chan_obj)
{
	mem_addr_t hcint_reg = (mem_addr_t)&chan_obj->regs->hcint;

	uint32_t hcint = sys_read32(hcint_reg);
	/* Clear the interrupt bits by writing them back */
	sys_write32(hcint, hcint_reg);

	dwc2_hal_chan_event_t chan_event;

	/*
	 * Note:
	 * We don't assert on (chan_obj->flags.active) here as it could have been already cleared
	 * by usb_dwc_hal_chan_request_halt()
	*/

	/*
	 * Note:
	 * Do not change order of checks as some events take precedence over others.
	 * Errors > Channel Halt Request > Transfer completed
	*/
	if (hcint & (USB_DWC2_HCINT_STALL | USB_DWC2_HCINT_BBLERR | USB_DWC2_HCINT_XACTERR)) {
		__ASSERT(hcint & USB_DWC2_HCINT_CHHLTD,
			 "uhc_dwc2_hal_chan_decode_intr: Channel error without channel halted interrupt");

		LOG_ERR("Channel %d error: 0x%08x", chan_obj->flags.chan_idx, hcint);
		/* TODO: Store the error in hal context */
		chan_event = DWC2_CHAN_EVENT_ERROR;
	} else if (hcint & USB_DWC2_HCINT_CHHLTD) {
		if (chan_obj->flags.halt_requested) {
			chan_obj->flags.halt_requested = 0;
			chan_event = DWC2_CHAN_EVENT_HALT_REQ;
		} else {
			chan_event = DWC2_CHAN_EVENT_CPLT;
		}
		chan_obj->flags.active = 0;
	} else if (hcint & USB_DWC2_HCINT_XFERCOMPL) {
		/* Note:
		* The channel isn't halted yet, so we need to halt it manually to stop the execution
		* of the next packet. Relevant only for Scatter-Gather DMA and never occurs oin Buffer DMA.
		*/
		sys_set_bits((mem_addr_t)&chan_obj->regs->hcchar, USB_DWC2_HCCHAR0_CHDIS);

		/*
		 * After setting the halt bit, this will generate another channel halted interrupt.
		 * We treat this interrupt as a NONE event, then cycle back with the channel halted
		 * interrupt to handle the CPLT event.
		*/
		chan_event = DWC2_CHAN_EVENT_NONE;
	} else {
		__ASSERT(false,
				"Unknown channel interrupt, HCINT=%08Xh", hcint);
		chan_event = DWC2_CHAN_EVENT_NONE;
	}
	return chan_event;
}

struct uhc_transfer *pipe_get_next_xfer(pipe_t *pipe)
{
	struct uhc_transfer *xfer;
	sys_dnode_t *node = sys_dlist_peek_head(&pipe->xfer_pending_list);
	return (node == NULL) ? NULL : SYS_DLIST_CONTAINER(node, xfer, node);
}

static inline uint16_t calc_packet_count(const uint16_t size, const uint8_t mps)
{
	if (size == 0) {
		return 1; /* in Buffer DMA mode Zero Length Packet still counts as 1 packet */
	} else {
		return DIV_ROUND_UP(size, mps);
	}
}

static inline bool _buffer_check_done(pipe_t *pipe)
{
	dma_buffer_t *buffer = pipe->buffer;
	/* Only control transfers need to be continued */
	if (pipe->ep_char.type != UHC_DWC2_XFER_TYPE_CTRL) {
		return true;
	}

	return (buffer->flags.ctrl.cur_stg == 2);
}

static inline void _buffer_fill_ctrl(dma_buffer_t *buffer, struct uhc_transfer *const xfer)
{
	/* Get information about the control transfer by analyzing the setup packet */
	const struct usb_setup_packet *setup_pkt = (const struct usb_setup_packet *)xfer->setup_pkt;

	buffer->flags.ctrl.cur_stg = 0;
	buffer->flags.ctrl.data_stg_in = usb_reqtype_is_to_host(setup_pkt);
	buffer->flags.ctrl.data_stg_skip = (setup_pkt->wLength == 0);
	buffer->flags.ctrl.set_addr = 0;

	if (setup_pkt->bRequest == USB_SREQ_SET_ADDRESS) {
		buffer->flags.ctrl.set_addr = 1;
		buffer->flags.ctrl.new_addr = setup_pkt->wValue & 0x7F;
		LOG_DBG("Set address request, new address %d", buffer->flags.ctrl.new_addr);
	}

	LOG_DBG("data_in: %s, data_skip: %s",
			buffer->flags.ctrl.data_stg_in ? "true" : "false",
			buffer->flags.ctrl.data_stg_skip ? "true" : "false");

	/* Save the xfer pointer in the buffer */
	buffer->xfer = xfer;

	/* TODO Sync data from cache to memory. For OUT and CTRL transfers */
}

static void IRAM_ATTR _buffer_fill(pipe_t *pipe)
{
	struct uhc_transfer * xfer = pipe_get_next_xfer(pipe);
	pipe->num_xfer_pending--;

	/* TODO: Double buffering scheme? */

	switch (pipe->ep_char.type) {
		case UHC_DWC2_XFER_TYPE_CTRL: {
				_buffer_fill_ctrl(pipe->buffer, xfer);
				break;
				}
		default: {
				LOG_ERR("Unsupported transfer type %d", pipe->ep_char.type);
				break;
			}
	}
	/* TODO: sync CACHE */
}

static inline ctrl_stage_t cal_next_pid(ctrl_stage_t pid, uint8_t pkt_count)
{
	if (pkt_count & 0x01) {
		/* Toggle DATA0 and DATA1 */
		return pid ^ 0x02;
	} else {
		return pid;
	}
}

static void IRAM_ATTR _buffer_exec_proceed(pipe_t *pipe)
{
	dma_buffer_t *buffer_exec = pipe->buffer;
	struct uhc_transfer *const xfer = pipe->buffer->xfer;
	const struct usb_dwc2_host_chan *const chan_regs = pipe->chan_obj->regs;

	__ASSERT(buffer_exec != NULL,
			"_buffer_exec_proceed: No buffer assigned to pipe");
	__ASSERT(xfer != NULL,
			"_buffer_exec_proceed: No transfer assigned to buffer");
	__ASSERT(buffer_exec->flags.ctrl.cur_stg != 2,
			"_buffer_exec: Invalid control stage: %d", buffer_exec->flags.ctrl.cur_stg);

	bool next_dir_is_in;
	ctrl_stage_t next_pid;
	uint16_t size = 0;
	uint8_t *dma_addr = NULL;

	if (buffer_exec->flags.ctrl.cur_stg == 0) { /* Just finished control stage */
		if (buffer_exec->flags.ctrl.data_stg_skip) {
			/* No data stage. Go straight to status stage */
			next_dir_is_in = true;		/* With no data stage, status stage must be IN */
			next_pid = CTRL_STAGE_DATA1;	/* Status stage always has a PID of DATA1 */
			buffer_exec->flags.ctrl.cur_stg = 2;	/* Skip over */
		} else {
			/* Go to data stage */
			next_dir_is_in = buffer_exec->flags.ctrl.data_stg_in;
			next_pid = CTRL_STAGE_DATA1;	/* Data stage always starts with a PID of DATA1 */
			buffer_exec->flags.ctrl.cur_stg = 1;

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
	} else {        /* cur_stg == 1. Just finished data stage. Go to status stage */
		/* Status stage is always the opposite direction of data stage */
		next_dir_is_in = !buffer_exec->flags.ctrl.data_stg_in;
		next_pid = CTRL_STAGE_DATA1;	/* Status stage always has a PID of DATA1 */
		buffer_exec->flags.ctrl.cur_stg = 2;
	}

	/* Calculate new packet count */
	const uint16_t pkt_cnt = calc_packet_count(size, pipe->ep_char.mps);


	if (next_dir_is_in) {
		sys_set_bits((mem_addr_t)&chan_regs->hcchar, USB_DWC2_HCCHAR0_EPDIR);
	} else {
		sys_clear_bits((mem_addr_t)&chan_regs->hcchar, USB_DWC2_HCCHAR0_EPDIR);
	}

	uint32_t hctsiz = ((next_pid << USB_DWC2_HCTSIZ_PID_POS) & USB_DWC2_HCTSIZ_PID_MASK) |
					((pkt_cnt << USB_DWC2_HCTSIZ_PKTCNT_POS) & USB_DWC2_HCTSIZ_PKTCNT_MASK) |
					((size << USB_DWC2_HCTSIZ_XFERSIZE_POS) & USB_DWC2_HCTSIZ_XFERSIZE_MASK);
	sys_write32(hctsiz, (mem_addr_t)&chan_regs->hctsiz);
	sys_write32((uint32_t)dma_addr, (mem_addr_t)&chan_regs->hcdma);

	/* TODO: Configure split transaction if needed */

	/* TODO: sync CACHE */
	uint32_t hcchar = sys_read32((mem_addr_t)&chan_regs->hcchar);
	hcchar |= USB_DWC2_HCCHAR0_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR0_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&chan_regs->hcchar);
}

static inline void _buffer_done(pipe_t *pipe, pipe_event_t pipe_event, bool canceled)
{
	dma_buffer_t *buffer_done = pipe->buffer;
	buffer_done->status_flags.executing = 0;
	buffer_done->status_flags.was_canceled = canceled;
	buffer_done->status_flags.pipe_event = pipe_event;
}

static inline bool _buffer_can_fill(pipe_t *pipe)
{
	/* We can only fill if there are pending XFRs and at least one unfilled buffer */
	return (pipe->num_xfer_pending > 0);

	/* TODO: Double buffering scheme? */
}

static inline bool _buffer_can_exec(pipe_t *pipe)
{
	/* TODO: Double buffering scheme? */
	/* For one buffer we can execute it always */
	return true;
}

/**
 * @brief Decode a channel interrupt and take appropriate action
 *
 * @note Interrupt context.
 *
 * @param pipe The pipe associated with the channel
 * @param chan_obj The channel object
 */
static pipe_event_t uhc_dwc2_decode_chan(pipe_t* pipe, uhc_dwc2_channel_t *chan_obj)
{
	dwc2_hal_chan_event_t chan_event = uhc_dwc2_hal_chan_decode_intr(chan_obj);
	pipe_event_t pipe_event = PIPE_EVENT_NONE;

	LOG_DBG("Channel event: %s", dwc2_chan_event_str[chan_event]);

	switch (chan_event) {
	case DWC2_CHAN_EVENT_NONE: {
		/* No event, nothing to do */
		break;
	}
	case DWC2_CHAN_EVENT_CPLT: {
		if (!_buffer_check_done(pipe)) {
			_buffer_exec_proceed(pipe);
			break;
		}
		pipe->last_event = PIPE_EVENT_XFER_DONE;
		pipe_event = pipe->last_event;
		_buffer_done(pipe, pipe->last_event, false);
		break;
	}
	case DWC2_CHAN_EVENT_ERROR: {
		LOG_ERR("Channel error handling not implemented yet");
		/* TODO: get channel error, halt the pipe */
		break;
	}
	case DWC2_CHAN_EVENT_HALT_REQ: {
		LOG_ERR("Channel halt request handling not implemented yet");

		__ASSERT(pipe->flags.waiting_halt,
				"uhc_dwc2_decode_chan: Pipe is not watiting to be halted");

		/* TODO: Implement halting the ongoing transfer */

		/* Hint:
		 * We've halted a transfer, so we need to trigger the pipe callback
		 * pipe->last_event = PIPE_EVENT_XFER_DONE;
		 * pipe_event = pipe->last_event;
		 * Halt request event is triggered when packet is successful completed.
		 * But just treat all halted transfers as errors
		 * pipe->state = UHC_PIPE_STATE_HALTED;
		 * Notify the task waiting for the pipe halt or halt it right away
		 * _internal_pipe_event_notify(pipe, true);
		 */
		break;
	}
	default:
		/* Should never happen */
		LOG_WRN("Unknown channel event %d", chan_event);
		break;
	}

	return pipe_event;
}

static IRAM_ATTR void _buffer_exec(pipe_t *pipe)
{
	dma_buffer_t *buffer_exec = pipe->buffer;
	struct uhc_transfer *const xfer = (struct uhc_transfer *)buffer_exec->xfer;
	const struct usb_dwc2_host_chan *const chan_regs = pipe->chan_obj->regs;

	LOG_DBG("ep=%02X, mps=%d", xfer->ep, pipe->ep_char.mps);

	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		/* Control stage is always OUT */
		sys_clear_bits((mem_addr_t)&chan_regs->hcchar, USB_DWC2_HCCHAR0_EPDIR);
	}

	if (xfer->interval != 0) {
		LOG_ERR("Periodic transfer is not supported");
	}

	const uint16_t pkt_cnt = calc_packet_count(sizeof(struct usb_setup_packet), pipe->ep_char.mps);
	int next_pid = CTRL_STAGE_SETUP;
	uint16_t size = sizeof(struct usb_setup_packet);

	uint32_t hctsiz = ((next_pid << USB_DWC2_HCTSIZ_PID_POS) & USB_DWC2_HCTSIZ_PID_MASK) |
					((pkt_cnt << USB_DWC2_HCTSIZ_PKTCNT_POS) & USB_DWC2_HCTSIZ_PKTCNT_MASK) |
					((size << USB_DWC2_HCTSIZ_XFERSIZE_POS) & USB_DWC2_HCTSIZ_XFERSIZE_MASK);
	sys_write32(hctsiz, (mem_addr_t)&chan_regs->hctsiz);

	sys_write32((uint32_t)xfer->setup_pkt, (mem_addr_t)&chan_regs->hcdma);

	/* TODO: Configure split transaction if needed */

	uint32_t hcint = sys_read32((mem_addr_t)&chan_regs->hcint);
	sys_write32(hcint, (mem_addr_t)&chan_regs->hcint);

	/* TODO: sync CACHE */
	uint32_t hcchar = sys_read32((mem_addr_t)&chan_regs->hcchar);
	hcchar |= USB_DWC2_HCCHAR0_CHENA;
	hcchar &= ~USB_DWC2_HCCHAR0_CHDIS;
	sys_write32(hcchar, (mem_addr_t)&chan_regs->hcchar);

	buffer_exec->status_flags.executing = 1;
}

static void uhc_dwc2_isr_handler(const struct device *dev)
{
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

	unsigned int key = irq_lock();

	uhc_dwc2_core_event_t core_event = uhc_dwc2_decode_intr(dev);
	if (core_event == UHC_DWC2_CORE_EVENT_CHAN) {
		/* Channel event. Cycle through each pending channel */
		uhc_dwc2_channel_t *chan_obj = uhc_dwc2_get_chan_pending_intr(dev);
		while (chan_obj != NULL) {
			pipe_t *pipe = (pipe_t*) uhc_dwc2_chan_get_context(chan_obj);
			pipe_event_t pipe_event = uhc_dwc2_decode_chan(pipe, chan_obj);
			if (pipe_event != PIPE_EVENT_NONE) {
				pipe->last_event = pipe_event;
				pipe->flags.event_pending = 1;
				k_event_post(&priv->drv_evt, BIT(UHC_DWC2_EVENT_PIPE));
			}
			/* Check for more channels with pending interrupts. Returns NULL if there are no more */
			chan_obj = uhc_dwc2_get_chan_pending_intr(dev);
		}
	} else {
		if (core_event != UHC_DWC2_CORE_EVENT_NONE) {
			/* Port event */
			uhc_port_event_t port_event = uhc_dwc2_decode_hprt(dev, core_event);
			if (port_event != UHC_PORT_EVENT_NONE) {
				priv->dynamic.last_event = port_event;
				priv->dynamic.flags.event_pending = 1;
				k_event_post(&priv->drv_evt, BIT(UHC_DWC2_EVENT_PORT));
			}
		} else {
			/* No core event, nothing to do. Should never occur */
			__ASSERT(false, "uhc_dwc2_isr_handler: No core event detected");
		}
	}

	irq_unlock(key);

	(void)uhc_dwc2_quirk_irq_clear(dev);
}

/* TODO: critical section */
static inline bool uhc_dwc2_port_debounce(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	/* TODO: exit critical section */
	k_msleep(DEBOUNCE_DELAY_MS);
	/* TODO: enter critical section */

	/* Check the post-debounce state (i.e., whether it's actually connected/disconnected) */
	bool is_connected = ((sys_read32((mem_addr_t)&dwc2->hprt) & USB_DWC2_HPRT_PRTCONNSTS) != 0);
	if (is_connected) {
		priv->dynamic.port_state = UHC_PORT_STATE_DISABLED;
	} else {
		priv->dynamic.port_state = UHC_PORT_STATE_DISCONNECTED;
	}
	/* Disable debounce lock */
	uhc_dwc2_lock_disable(dev);
	return is_connected;
}

static inline uhc_port_event_t uhc_dwc2_get_port_event(const struct device *dev)
{
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

	uhc_port_event_t ret = UHC_PORT_EVENT_NONE;
	/* TODO: enter critial section */
	if (priv->dynamic.flags.event_pending) {
		priv->dynamic.flags.event_pending = 0;
		ret = priv->dynamic.last_event;
		switch (ret) {
			case UHC_PORT_EVENT_CONNECTION: {
				/* Don't update state immediately, we still need to debounce. */
				if (uhc_dwc2_port_debounce(dev)) {
					ret = UHC_PORT_EVENT_CONNECTION;
				} else {
					LOG_ERR("Port is not connected after debounce");
					/* TODO: Simulate and/or verify */
					LOG_WRN("Port debounce error handling is not implemented yet");
				}
				break;
			}
			case UHC_PORT_EVENT_DISCONNECTION:
			case UHC_PORT_EVENT_ERROR:
			case UHC_PORT_EVENT_OVERCURRENT: {
				break;
			}
			default: {
				break;
			}
		}
	} else {
		ret = UHC_PORT_EVENT_NONE;
	}
	/* TODO: exit critical section */
	return ret;
}

static inline void uhc_dwc2_flush_pipes(const struct device *dev)
{
	/* TODO: For each pipe, reinitialize the channel with EP characteristics */
	/* Flush the channel EP characteristics */
	/* TODO: Sync CACHE */
}

/**
 * @brief Reset the port.
 *
 * @note Port-related logic, thread context.
 *
 * @param[in] dev Pointer to the device structure.
 *
 * @return 0 on success, negative error code on failure.
 */
static inline int uhc_dwc2_port_reset(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	int ret;

	/* Enter critical section */
	unsigned int key = irq_lock();

	/* TODO: implement port checks */

	/*
	* Hint:
	* Port can only a reset when it is in the enabled or disabled (in the case of a new connection)
	* states.
	* priv->dynamic.port_state == UHC_PORT_STATE_ENABLED;
	* priv->dynamic.port_state == UHC_PORT_STATE_DISABLED;
	* priv->channels.num_pipes_queued == 0
	*/

	/*
	Proceed to resetting the bus:
	- Update the port's state variable
	- Hold the bus in the reset state for RESET_HOLD_MS.
	- Return the bus to the idle state for RESET_RECOVERY_MS
	During this reset the port state should be set to RESETTING and do not change.
	*/
	priv->dynamic.port_state = UHC_PORT_STATE_RESETTING;
	dwc2_hal_toggle_reset(dwc2, true);

	/* Exit critical section */
	irq_unlock(key);

	/* Hold the bus in the reset state */
	k_msleep(RESET_HOLD_MS);

	/* Enter critical section */
	key = irq_lock();

	if (priv->dynamic.port_state != UHC_PORT_STATE_RESETTING) {
		/* The port state has unexpectedly changed */
		LOG_ERR("Port state changed during reset");
		ret = -EIO;
		goto bailout;
	}

	/* Return the bus to the idle state. Port enabled event should occur */
	dwc2_hal_toggle_reset(dwc2, false);

	/* Exit critical section */
	irq_unlock(key);

	/* Give the port time to recover */
	k_msleep(RESET_RECOVERY_MS);

	/* TODO: enter critical section */
	if (priv->dynamic.port_state != UHC_PORT_STATE_RESETTING || !priv->dynamic.flags.conn_dev_ena) {
		/* The port state has unexpectedly changed */
		LOG_ERR("Port state changed during reset");
		ret = -EIO;
		goto bailout;
	}

	dwc2_hal_apply_fifo_config(dwc2, &priv->fifo);
	dwc2_hal_set_frame_list(dwc2, NULL /* priv->frame_list , FRAME_LIST_LEN */);
	dwc2_hal_periodic_enable(dwc2);
	ret = 0;
bailout:
	uhc_dwc2_flush_pipes(dev);
	return ret;
}

/**
 * @brief Perform a port recovery operation.
 *
 * Port recovery is necessary when the port is in an error state and needs to be reset.
 *
 * @note Port-related logic, thread context.
 *
 * @param[in] dev Pointer to the device structure.
 *
 * @return 0 on success, negative error code on failure.
 */
static inline int uhc_dwc2_port_recovery(const struct device *dev)
{
	int ret;

	/* TODO: Implement port checks */
	/* Port should be in recovery state and no ongoing transfers */
	/* Port flags should be 0 */

	/* TODO: enter critical section */
	ret = uhc_dwc2_quirk_irq_disable_func(dev);
	if (ret) {
		LOG_ERR("Quirk IRQ disable failed %d", ret);
		return ret;
	}

	/* Init controller */
	ret = uhc_dwc2_init_controller(dev);
	if (ret) {
		LOG_ERR("Failed to init controller: %d", ret);
		return ret;
	}

	ret = uhc_dwc2_quirk_irq_enable_func(dev);
	if (ret) {
		LOG_ERR("Quirk IRQ enable failed %d", ret);
		return ret;
	}
	/* TODO: exit critical section */

	ret = uhc_dwc2_power_on(dev);
	if (ret) {
		LOG_ERR("Failed to power on root port: %d", ret);
		return ret;
	}

	return ret;
}

/**
 * @brief Submit a new device connected event to the higher logic.
 *
 * @param[in] dev Pointer to the device structure.
 * @param[in] speed The speed of the new device.
 */
static inline void uhc_dwc2_submit_new_device(const struct device *dev, uhc_dwc2_speed_t speed)
{
	enum uhc_event_type type;

	LOG_WRN("New dev, speed %s", uhc_dwc2_speed_str[speed]);

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

/**
 * @brief Submit a device gone event to the higher logic.
 *
 * @param[in] dev Pointer to the device structure.
 */
static inline void uhc_dwc2_submit_dev_gone(const struct device *dev)
{
	LOG_WRN("Dev gone");
	uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
}

/**
 * @brief Fills the endpoint characteristics for a pipe.
 *
 * @param[in] pipe_config Pointer to the pipe configuration.
 * @param[in] type The type of USB transfer.
 * @param[in] is_ctrl_pipe Whether the pipe is a control pipe.
 * @param[in] pipe_idx The index of the pipe.
 * @param[in] port_speed The speed of the port.
 * @param[out] ep_char Pointer to the endpoint characteristics structure to be filled.
 */
static void uhc_dwc2_pipe_set_ep_char(const uhc_pipe_config_t *pipe_config,
									usb_transfer_type_t type,
									bool is_ctrl_pipe,
									int pipe_idx,
									uhc_dwc2_speed_t port_speed,
									uhc_dwc2_ep_char_t *ep_char)
{
	uhc_dwc2_xfer_type_t dw2_ll_xfer_type;

	if (type == USB_TRANSFER_TYPE_CTRL) {
		dw2_ll_xfer_type = UHC_DWC2_XFER_TYPE_CTRL;
	} else {
		LOG_ERR("Unsupported transfer type %d", type);
		return;
	}

	ep_char->type = dw2_ll_xfer_type;

	if (is_ctrl_pipe) {
		ep_char->bEndpointAddress = 0;
		/* Set the default pipe's MPS to the worst case MPS for the device's speed */
		ep_char->mps = (pipe_config->dev_speed == UHC_DWC2_SPEED_LOW)
						? CTRL_EP_MAX_MPS_LS
						: CTRL_EP_MAX_MPS_HSFS;
	} else {
		/* TODO: Implement for non-control pipes */
		LOG_WRN("Setting up pipe characteristics for non-control pipe has not implemented yet");
		return;
	}

	ep_char->dev_addr = pipe_config->dev_addr;
	ep_char->ls_via_fs_hub = 0;
	ep_char->periodic.interval = 0;
	ep_char->periodic.offset = 0;
}

/**
 * @brief Allocate a DWC2 HAL channel.
 *
 * Adds the channel object to the channel list and initializes it.
 *
 * @param[in] dev Pointer to the device structure.
 * @param[in] chan_obj Pointer to the channel object to be allocated.
 * @param[in] context Pointer to the context associated with the channel.
 *
 * @return true if the channel was successfully allocated, false otherwise.
 */
static inline bool uhc_dwc2_chan_alloc(const struct device *dev,
	uhc_dwc2_channel_t *chan_obj, void *context)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	__ASSERT(priv->channels.hdls,
			"uhc_dwc2_chan_alloc: Channel handles list not allocated");

	/* TODO: FIFO sizes should be set before attempting to allocate a channel */

	if (priv->channels.num_allocated == priv->const_cfg.numchannels) {
		return false;
	}

	uint8_t chan_idx = 0xff;
	for (uint8_t i = 0; i < priv->const_cfg.numchannels; i++) {
		if (priv->channels.hdls[i] == NULL) {
			priv->channels.hdls[i] = chan_obj;
			chan_idx = i;
			priv->channels.num_allocated++;
			break;
		}
	}

	__ASSERT(chan_idx != 0xff,
			"No free channels available, num_allocated=%d, numchannels=%d",
			priv->channels.num_allocated, priv->const_cfg.numchannels);

	/* Initialize channel object */
	LOG_DBG("Allocating channel %d", chan_idx);

	memset(chan_obj, 0, sizeof(uhc_dwc2_channel_t));

	chan_obj->flags.chan_idx = chan_idx;
	chan_obj->regs = UHC_DWC2_CHAN_REG(dwc2, chan_idx);
	chan_obj->chan_ctx = context;

	/* Init underlying channel registers */

	/* Clear the interrupt bits by writing them back */
	uint32_t hcint = sys_read32((mem_addr_t)&chan_obj->regs->hcint);
	sys_write32(hcint, (mem_addr_t)&chan_obj->regs->hcint);

	/* Enable channel interrupts in the core */
	sys_set_bits((mem_addr_t)&dwc2->haintmsk, (1 << chan_idx));

	/* Enable transfer complete and channel halted interrupts */
	sys_set_bits((mem_addr_t)&chan_obj->regs->hcintmsk,
						USB_DWC2_HCINT_XFERCOMPL | USB_DWC2_HCINT_CHHLTD);
	return true;
}

static inline void uhc_dwc2_chan_free(const struct device *dev, uhc_dwc2_channel_t *chan_obj)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	__ASSERT(priv->channels.hdls,
			"uhc_dwc2_chan_alloc: Channel handles list not allocated");

	if (chan_obj->type == UHC_DWC2_XFER_TYPE_INTR ||
		chan_obj->type == UHC_DWC2_XFER_TYPE_ISOCHRONOUS) {
		/* TODO: Unschedule this channel */
		LOG_WRN("uhc_dwc2_chan_free: Cannot free interrupt or isochronous channels yet");
	}

	__ASSERT(!chan_obj->flags.active,
			"uhc_dwc2_chan_free: Cannot free channel %d, it is still active",
			chan_obj->flags.chan_idx);

	sys_clear_bits((mem_addr_t)&dwc2->haintmsk, (1 << chan_obj->flags.chan_idx));

	priv->channels.hdls[chan_obj->flags.chan_idx] = NULL;
	priv->channels.num_allocated--;

	LOG_DBG("Freeing channel %d, num_allocated=%d",
			chan_obj->flags.chan_idx, priv->channels.num_allocated);

	__ASSERT(priv->channels.num_allocated >= 0,
			"uhc_dwc2_chan_free: Number of allocated channels is negative: %d",
			priv->channels.num_allocated);
	k_free(chan_obj);
}

/**
 * @brief Allocate one DMA buffer block for a pipe.
 *
 * @param[in] type The type of USB transfer.
 *
 * @return Pointer to the allocated DMA buffer block, or NULL on failure.
 */
static dma_buffer_t *dma_buffer_block_alloc(usb_transfer_type_t type)
{
	(void) type;
	/* Buffer DMA mode needs only one simple buffer for now */
	dma_buffer_t *buffer = k_malloc(sizeof(dma_buffer_t));
	return buffer;
}

/**
 * @brief Free a DMA buffer block.
 *
 * @param[in] buffer Pointer to the DMA buffer block to be freed.
 */
static void dma_buffer_block_free(dma_buffer_t *buffer)
{
	if (buffer) {
		k_free(buffer);
	}
}

/**
 * @brief Allocate a pipe and its resources.
 *
 * @note Pipe holds the underlying channel object and the DMA buffer for transfer purposes.
 *
 * Thread context.
 *
 * @param[in] dev Pointer to the device structure.
 * @param[in] pipe_config Pointer to the pipe configuration structure.
 * @param[out] pipe_hdl Pointer to the pipe handle to be filled.
 */
static inline int uhc_dwc2_pipe_alloc(const struct device *dev,
		const uhc_pipe_config_t *pipe_config, pipe_hdl_t *pipe_hdl)
{
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

	int ret;

	/* TODO: Allocate the pipe and it's resources */
	pipe_t *pipe = &priv->pipe;

	uhc_dwc2_channel_t *chan_obj = k_malloc(sizeof(uhc_dwc2_channel_t));
	if (pipe == NULL || chan_obj == NULL) {
		LOG_ERR("Failed to allocate pipe or channel object");
		ret = -ENOMEM;
		goto err;
	}
	memset(chan_obj, 0, sizeof(uhc_dwc2_channel_t));

	/* Save channel object in the pipe object */
	pipe->chan_obj = chan_obj;

	/* TODO: Double buffering scheme? */

	/* TODO: Support all types of transfers */
	uhc_dwc2_ep_char_t ep_char;

	usb_transfer_type_t type = USB_TRANSFER_TYPE_CTRL;
	/* TODO: Refactor to get port speed, static for now */
	uhc_dwc2_speed_t port_speed = UHC_DWC2_SPEED_FULL;
	bool is_default = true;
	int pipe_idx = 0;

	pipe->buffer = dma_buffer_block_alloc(type);

	if (pipe->buffer == NULL) {
		LOG_ERR("Failed to allocate pipe buffer");
		ret = -ENOMEM;
		goto err;
	}

	uhc_dwc2_pipe_set_ep_char(pipe_config, type, is_default, pipe_idx, port_speed, &ep_char);
	memcpy(&pipe->ep_char, &ep_char, sizeof(uhc_dwc2_ep_char_t));
	pipe->state = UHC_PIPE_STATE_ACTIVE;

	/* TODO: enter critical section */
	if (!priv->dynamic.flags.conn_dev_ena) {
		/* TODO: exit critical section */
		LOG_ERR("Port is not enabled, cannot allocate channel");
		ret = -ENODEV;
		goto err;
	}

	bool chan_allocated = uhc_dwc2_chan_alloc(dev, pipe->chan_obj, (void *) pipe);
	if (!chan_allocated) {
		/* TODO: exit critical section */
		LOG_ERR("No more free channels available");
		ret = -ENOMEM;
		goto err;
	}

	dwc2_hal_channel_configure(pipe->chan_obj, &pipe->ep_char);

	/* TODO: sync CACHE */

	/* TODO: Add the pipe to the list of idle pipes in the port object */
	sys_dlist_init(&pipe->xfer_pending_list);
	priv->num_pipes_idle++;

	/* TODO: exit critical section */

	*pipe_hdl = (pipe_hdl_t)pipe;
	return 0;
err:
	if (pipe->buffer) {
		dma_buffer_block_free(pipe->buffer);
	}
	if (chan_obj) {
		k_free(chan_obj);
	}
	return ret;
}

/**
 * @brief Free the pipe and its resources.
 *
 * @param dev Pointer to the device structure.
 */
static inline int uhc_dwc2_pipe_free(const struct device *dev, pipe_hdl_t pipe_hdl)
{
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	pipe_t *pipe = (pipe_t *)pipe_hdl;

	if (pipe->num_xfer_pending && pipe->flags.has_xfer) {
		LOG_ERR("Unable to free pipe with pending XFERs");
		return -EBUSY;
	}

	uhc_dwc2_chan_free(dev, pipe->chan_obj);
	dma_buffer_block_free(pipe->buffer);

	/* TODO: Remove the pipe from the list of idle pipes in the port object */
	priv->num_pipes_idle--;

	return 0;
}

/**
 * @brief Handle port events.
 *
 * @note Thread context.
 *
 * @param dev Pointer to the device structure.
 */
static inline void uhc_dwc2_handle_port_events(const struct device *dev)
{
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	uhc_port_event_t port_event = uhc_dwc2_get_port_event(dev);
	int ret;

	LOG_DBG("Port event: %s", uhc_port_event_str[port_event]);

	switch (port_event) {
		case UHC_PORT_EVENT_NONE:
			/* No event, nothing to do */
			break;
		case UHC_PORT_EVENT_CONNECTION: {
			uhc_dwc2_port_reset(dev);
			break;
		}

		case UHC_PORT_EVENT_ENABLED: {
			/* TODO: enter critical section */
			priv->dynamic.port_state = UHC_PORT_STATE_ENABLED;
			/* TODO: exit critical section */

			uhc_dwc2_speed_t speed;

			ret = uhc_dwc2_get_port_speed(dev, &speed);
			if (ret) {
				LOG_ERR("Failed to get port speed");
				break;
			}

			/* Allocate the Pipe for the EP0 Control Endpoint */
			pipe_hdl_t ctrl_pipe_hdl;
			uhc_pipe_config_t pipe_config = {
				.dev_speed = speed,
				.dev_addr = 0,
			};

			ret = uhc_dwc2_pipe_alloc(dev, &pipe_config, &ctrl_pipe_hdl);
			if (ret) {
				LOG_ERR("Failed to initialize channels: %d", ret);
				break;
			}
			/* Save the control pipe handle in the private data */
			priv->ctrl_pipe_hdl = ctrl_pipe_hdl;
			/* Notify the higher logic about the new device */
			uhc_dwc2_submit_new_device(dev, speed);
			break;
		}
		case UHC_PORT_EVENT_DISCONNECTION:
		case UHC_PORT_EVENT_ERROR:
		case UHC_PORT_EVENT_OVERCURRENT: {
			bool port_has_device = false;

			/* TODO: enter critical section */
			switch (priv->dynamic.port_state) {
				case UHC_PORT_STATE_DISABLED:
					break;
				case UHC_PORT_STATE_NOT_POWERED:
				case UHC_PORT_STATE_ENABLED:
					port_has_device = true;
					break;
				default:
					LOG_ERR("Unexpected port state %s",
						uhc_port_state_str[priv->dynamic.port_state]);
					break;
			}
			/* TODO: exit critical section */

			if (port_has_device) {
				uhc_dwc2_pipe_free(dev, priv->ctrl_pipe_hdl);
				priv->ctrl_pipe_hdl = NULL;
				uhc_dwc2_submit_dev_gone(dev);
			}
			/* Recover the port */
			uhc_dwc2_port_recovery(dev);
			break;
		}
		default:
			break;
	}
}

/**
 * @brief Handle pipe events.
 *
 * @note Thread context.
 *
 * @param dev Pointer to the device structure.
 */
static inline void uhc_dwc2_handle_pipe_events(const struct device *dev)
{
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

	/* TODO: support more than CTRL pipe */
	pipe_t* pipe = &priv->pipe;

	LOG_DBG("Pipe event: %s", uhc_pipe_event_str[pipe->last_event]);

	if (pipe->last_event == PIPE_EVENT_XFER_DONE) {
		/* XFER transfer is done, process the transfer and release the pipe buffer */
		struct uhc_transfer *const xfer = (struct uhc_transfer *)pipe->buffer->xfer;

		if (xfer->buf != NULL && xfer->buf->len) {
			LOG_HEXDUMP_WRN(xfer->buf->data, xfer->buf->len, "data");
		}

		/* TODO: Refactor the address setting logic. */
		/* To configure the channel, we need to get the dev addr from higher logic */
		if (pipe->buffer->flags.ctrl.set_addr) {
			pipe->buffer->flags.ctrl.set_addr = 0;
			pipe->ep_char.dev_addr = pipe->buffer->flags.ctrl.new_addr;
			/* Set the new device address in the channel */
			sys_set_bits((mem_addr_t)&pipe->chan_obj->regs->hcchar,
							(pipe->ep_char.dev_addr << USB_DWC2_HCCHAR0_DEVADDR_POS));
			k_msleep(SET_ADDR_DELAY_MS);
		}

		/* TODO: Refactor pipe resources release */
		pipe->flags.has_xfer = 0;
		priv->num_pipes_idle++;
		priv->num_pipes_queued--;

		uhc_xfer_return(dev, xfer, 0);

	} else {
		/* TODO: Handle the rest pipe events */
		LOG_ERR("Unhandled pipe event %s", uhc_pipe_event_str[pipe->last_event]);
	}
}

/**
 * @brief Thread handler for the UHC DWC2 USB driver.
 *
 * Thread that processes USB events from the DWC2 controller: Port, Pipe.
 *
 * @param arg Pointer to the device structure.
 */
static inline void uhc_dwc2_thread_handler(void *const arg)
{
	const struct device *dev = (const struct device *)arg;
	struct uhc_dwc2_data_s *const priv = uhc_get_private(dev);

	uint32_t evt = k_event_wait(&priv->drv_evt, UINT32_MAX, false, K_FOREVER);

	uhc_lock_internal(dev, K_FOREVER);

	if (evt & BIT(UHC_DWC2_EVENT_PORT)) {
		k_event_clear(&priv->drv_evt, BIT(UHC_DWC2_EVENT_PORT));
		uhc_dwc2_handle_port_events(dev);
	}

	if (evt & BIT(UHC_DWC2_EVENT_PIPE)) {
		k_event_clear(&priv->drv_evt, BIT(UHC_DWC2_EVENT_PIPE));
		uhc_dwc2_handle_pipe_events(dev);
	}

	uhc_unlock_internal(dev);
}

static inline int uhc_dwc2_submit_ctrl_xfer(const struct device *dev,
				pipe_hdl_t pipe_hdl, struct uhc_transfer *const xfer)
{
	pipe_t *pipe = (pipe_t *)pipe_hdl;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

	LOG_HEXDUMP_WRN(xfer->setup_pkt, 8, "setup");

	LOG_DBG("endpoint=%02Xh, mps=%d, interval=%d, start_frame=%d, stage=%d, no_status=%d",
		xfer->ep, xfer->mps, xfer->interval, xfer->start_frame, xfer->stage, xfer->no_status);

	/* TODO: Check that XFER has not already been enqueued? */

	/* TODO: setup packet must be aligned 4 bytes? */
	if (((uintptr_t)xfer->setup_pkt % 4)) {
		LOG_WRN("Setup packet address %p is not 4-byte aligned", xfer->setup_pkt);
	}

	/* TODO: Buffer addr that will used as dma addr also should be aligned */
	if((xfer->buf != NULL) && ((uintptr_t)net_buf_tail(xfer->buf) % 4)) {
		LOG_WRN("XFER buffer address %08lXh is not 4-byte aligned", (uintptr_t)net_buf_tail(xfer->buf));
	}

	sys_dlist_append(&pipe->xfer_pending_list, &xfer->node);
	pipe->num_xfer_pending++;

	unsigned int key = irq_lock();

	if (_buffer_can_fill(pipe)) {
	    _buffer_fill(pipe);
	}
	if (_buffer_can_exec(pipe)) {
		_buffer_exec(pipe);
	}

	if (!pipe->flags.has_xfer) {
		/* This is the first XFER to be enqueued into the pipe. */
		/* TODO: remove pipe from idle pipes list */
		/* TODO: add pipe to active pipes list */
		priv->num_pipes_idle--;
		priv->num_pipes_queued++;
		pipe->flags.has_xfer = 1;
	}

	irq_unlock(key);

	return 0;
}

/* =============================================================================================== */
/* ================================== UHC DWC2 Driver API ======================================== */
/* =============================================================================================== */

static int uhc_dwc2_lock(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_lock(&data->mutex, K_FOREVER);
}

static int uhc_dwc2_unlock(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_unlock(&data->mutex);
}

static int uhc_dwc2_sof_enable(const struct device *dev)
{
	LOG_ERR("uhc_dwc2_sof_enable is not implemented yet");
	return -ENOSYS;
}

static int uhc_dwc2_bus_suspend(const struct device *dev)
{
	LOG_ERR("uhc_dwc2_bus_suspend is not implemented yet");
	return -ENOSYS;
}

static int uhc_dwc2_bus_reset(const struct device *dev)
{
	/* TODO: move the reset logic here */

	/* Hint: First reset is done by the uhc dwc2 driver, so we don't need to do anything here */
	uhc_submit_event(dev, UHC_EVT_RESETED, 0);
	return 0;
}

static int uhc_dwc2_bus_resume(const struct device *dev)
{
	LOG_ERR("uhc_dwc2_bus_resume is not implemented yet");
	return -ENOSYS;
}

static int uhc_dwc2_enqueue(const struct device *dev,
				struct uhc_transfer *const xfer)
{
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

	int ret;
	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		ret = uhc_dwc2_submit_ctrl_xfer(dev, priv->ctrl_pipe_hdl, xfer);
		if (ret) {
			LOG_ERR("Failed to submit xfer: %d", ret);
			return ret;
		}
	} else {
		LOG_ERR("Non-control endpoint enqueue not implemented yet");
		return -ENOSYS;
	}

	return 0;
}

static int uhc_dwc2_dequeue(const struct device *dev,
				struct uhc_transfer *const xfer)
{
	LOG_ERR("uhc_dwc2_dequeue is not implemented yet");
	return -ENOSYS;
}

static int uhc_dwc2_preinit(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

	/* Initialize the private data structure */
	memset(priv, 0, sizeof(struct uhc_dwc2_data_s));
	priv->ctrl_pipe_hdl = NULL;
	k_mutex_init(&priv->mutex);
	k_event_init(&priv->drv_evt);

	/* TODO: Overwrite the DWC2 register values with the devicetree values? */

	(void)uhc_dwc2_quirk_caps(dev);

	config->make_thread(dev);
	return 0;
}

static int uhc_dwc2_init(const struct device *dev)
{
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

	int ret;

	ret = uhc_dwc2_quirk_init(dev);
	if (ret) {
		LOG_ERR("Quirk init failed %d", ret);
		return ret;
	}

	ret = uhc_dwc2_init_controller(dev);
	if (ret) {
		return ret;
	}

	/* Allocate memory for the channel objects */
	priv->channels.hdls = k_malloc(priv->const_cfg.numchannels * sizeof(uhc_dwc2_channel_t*));
	if (priv->channels.hdls == NULL) {
		LOG_ERR("Failed to allocate channel handles");
		return -ENOMEM;
	}

	for (uint8_t i = 0; i < priv->const_cfg.numchannels; i++) {
		priv->channels.hdls[i] = NULL;
	}

	return 0;
}

static int uhc_dwc2_enable(const struct device *dev)
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

static int uhc_dwc2_disable(const struct device *dev)
{
	int ret;

	LOG_WRN("uhc_dwc2_disable has not been fully implemented yet");

	ret = uhc_dwc2_quirk_disable(dev);
	if (ret) {
		LOG_ERR("Quirk disable failed %d", ret);
		return ret;
	}

	return -ENOSYS;
}

static int uhc_dwc2_shutdown(const struct device *dev)
{
	int ret;

	LOG_WRN("uhc_dwc2_shutdown has not been fully implemented yet");

	/* TODO: Release memory for channel handles */
	/* Hint : k_free(priv->channels.hdls); */

	ret = uhc_dwc2_quirk_shutdown(dev);
	if (ret) {
		LOG_ERR("Quirk shutdown failed %d", ret);
		return ret;
	}

	return -ENOSYS;
}

/* =============================================================================================== */
/* ======================== Device Definition and Initialization ================================= */
/* =============================================================================================== */

K_THREAD_STACK_DEFINE(uhc_dwc2_stack, CONFIG_UHC_DWC2_STACK_SIZE);

static void uhc_dwc2_thread(void *arg1, void *arg2, void *arg3)
{
	while (true) {
		uhc_dwc2_thread_handler(arg1);
	}
}

static void uhc_dwc2_make_thread(const struct device *dev)
{
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

	k_thread_create(&priv->thread_data,
			uhc_dwc2_stack,
			K_THREAD_STACK_SIZEOF(uhc_dwc2_stack),
			uhc_dwc2_thread,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_UHC_DWC2_THREAD_PRIORITY),
			K_ESSENTIAL,
			K_NO_WAIT);
	k_thread_name_set(&priv->thread_data, dev->name);
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
	.sof_enable  = uhc_dwc2_sof_enable,
	.bus_suspend = uhc_dwc2_bus_suspend,
	.bus_resume = uhc_dwc2_bus_resume,
	/* EP related */
	.ep_enqueue = uhc_dwc2_enqueue,
	.ep_dequeue = uhc_dwc2_dequeue,
};

#define UHC_DWC2_DT_INST_REG_ADDR(n)						                    \
			COND_CODE_1(DT_NUM_REGS(DT_DRV_INST(n)), (DT_INST_REG_ADDR(n)),		\
					(DT_INST_REG_ADDR_BY_NAME(n, core)))

static struct uhc_dwc2_data_s uhc_dwc2_data = {
	.irq_sem = Z_SEM_INITIALIZER(uhc_dwc2_data.irq_sem, 0, 1),
};

static const struct uhc_dwc2_config uhc_dwc2_config_host = {
	.base = (struct usb_dwc2_reg *)UHC_DWC2_DT_INST_REG_ADDR(0),    /* Base register address */
	.make_thread = uhc_dwc2_make_thread,                            /* Create the thread */
	.quirks = UHC_DWC2_VENDOR_QUIRK_GET(0),                         /* Vendors' quirks */
};

static struct uhc_data uhc_dwc2_priv_data = {
	.priv = &uhc_dwc2_data, /* Pointer to the private data */
};

DEVICE_DT_INST_DEFINE(
	0,                      /* Device instance number */
	uhc_dwc2_preinit,       /* Initialization function (called before main) */
	NULL,                   /* Power management resources (optional) */
	&uhc_dwc2_priv_data,    /* Reference to instance data */
	&uhc_dwc2_config_host,  /* Reference to instance configuration */
	POST_KERNEL,            /* Initialization level */
	99,                     /* Initialization priority */
	&uhc_dwc2_api           /* Reference to API operations */
);
