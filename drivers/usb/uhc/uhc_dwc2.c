/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 * Copyright (c) 2025 Nordic Semiconductor ASA
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
#include <usb_dwc2_ll.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_dwc2, CONFIG_UHC_DRIVER_LOG_LEVEL);

#define DEBOUNCE_DELAY_MS       CONFIG_UHC_DWC2_PORT_DEBOUNCE_DELAY_MS
#define RESET_HOLD_MS           CONFIG_UHC_DWC2_PORT_RESET_HOLD_MS
#define RESET_RECOVERY_MS       CONFIG_UHC_DWC2_PORT_RESET_RECOVERY_MS
#define SET_ADDR_DELAY_MS       CONFIG_UHC_DWC2_PORT_SET_ADDR_DELAY_MS

#define CTRL_EP_MAX_MPS_LS      8U
#define CTRL_EP_MAX_MPS_HSFS    64U

enum {
	UHC_DWC2_EVENT_PORT,       /**< Root port event */
	UHC_DWC2_EVENT_PIPE,       /**< Root pipe event */
} uhc_dwc2_event_t;

const char* uhc_dwc2_speed_str[] = {
	"High Speed",
	"Full Speed",
	"Low Speed",
};
typedef enum {
	UHC_PORT_EVENT_NONE,            /**< No event has occurred. Or the previous event is no longer valid */
	UHC_PORT_EVENT_CONNECTION,      /**< A device has been connected to the port */
	UHC_PORT_EVENT_ENABLED,         /**< A device has compete reset and enabled on the port. SOFs are being sent */
	UHC_PORT_EVENT_DISCONNECTION,   /**< A device disconnection has been detected */
	UHC_PORT_EVENT_ERROR,           /**< A port error has been detected. Port is now UHC_PORT_STATE_RECOVERY  */
	UHC_PORT_EVENT_OVERCURRENT,     /**< Overcurrent detected on the port. Port is now UHC_PORT_STATE_RECOVERY */
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
	UHC_PORT_STATE_NOT_POWERED,     /**< The port is not powered */
	UHC_PORT_STATE_DISCONNECTED,    /**< The port is powered but no device is connected */
	UHC_PORT_STATE_DISABLED,        /**< A device has connected to the port but has not been reset. SOF/keep alive are not being sent */
	UHC_PORT_STATE_RESETTING,       /**< The port is issuing a reset condition */
	UHC_PORT_STATE_SUSPENDED,       /**< The port has been suspended. */
	UHC_PORT_STATE_RESUMING,        /**< The port is issuing a resume condition */
	UHC_PORT_STATE_ENABLED,         /**< The port has been enabled. SOF/keep alive are being sent */
	UHC_PORT_STATE_RECOVERY,        /**< Port needs to be recovered from a fatal error (port error, overcurrent, or sudden disconnection) */
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
	UHC_DWC2_CORE_EVENT_NONE,            /**< No event occurred, or could not decode interrupt */
	UHC_DWC2_CORE_EVENT_CHAN,            /**< A channel event has occurred. Call the the channel event handler instead */
	UHC_DWC2_CORE_EVENT_CONN,            /**< The host port has detected a connection */
	UHC_DWC2_CORE_EVENT_DISCONN,         /**< The host port has been disconnected */
	UHC_DWC2_CORE_EVENT_ENABLED,         /**< The host port has been enabled (i.e., connected to a device that has been reset. Started sending SOFs) */
	UHC_DWC2_CORE_EVENT_DISABLED,        /**< The host port has been disabled (no more SOFs). Could be due to disable/reset request, or a port error (e.g. port babble condition. See 11.8.1 of USB2.0 spec) */
	UHC_DWC2_CORE_EVENT_OVRCUR,          /**< The host port has encountered an overcurrent condition */
	UHC_DWC2_CORE_EVENT_OVRCUR_CLR,      /**< The host port has been cleared of the overcurrent condition */
} uhc_dwc2_core_event_t;

const char* dwc2_core_event_str[] = {
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
	DWC2_CHAN_EVENT_CPLT,            /**< The channel has completed execution of a transfer descriptor that had the USB_DWC_HAL_XFER_DESC_FLAG_HOC flag set. Channel is now halted */
	DWC2_CHAN_EVENT_ERROR,           /**< The channel has encountered an error. Channel is now halted. */
	DWC2_CHAN_EVENT_HALT_REQ,        /**< The channel has been successfully halted as requested */
	DWC2_CHAN_EVENT_NONE,            /**< No event (interrupt ran for internal processing) */
} dwc2_hal_chan_event_t;

const char *dwc2_chan_event_str[] = {
	"CPLT",
	"ERROR",
	"HALT_REQ",
	"NONE",
};
typedef struct {
	size_t num_channels; /**< Number of available channels */
	uint8_t hsphy_type;  /**< High-speed PHY type */
	uint8_t fsphy_type;  /**< Full-speed PHY type */
	bool dma;            /**< DMA mode is supported and enabled */
	struct {
		union {
			struct {
				uint8_t dedicated: 1;      /**< Only dedicated FIFO mode is supported */
				uint8_t dynamic: 1;         /**< Dynamic FIFO sizing is supported */
				uint8_t reserved6: 6;           /**< Reserved bits */
			};
			uint8_t val;
		} flags;
		uint16_t depth;                         /**< FIFO depth in WORDs */
	} fifo;

	/* TODO: Port context and callback? */
} uhc_dwc2_constant_config_t;

typedef struct {
	union {
		struct {
			uhc_dwc2_xfer_type_t type: 2;    /**< The type of endpoint */
			uint32_t bEndpointAddress: 8;   /**< Endpoint address (containing endpoint number and direction) */
			uint32_t mps: 11;               /**< Maximum Packet Size */
			uint32_t dev_addr: 8;           /**< Device Address */
			uint32_t ls_via_fs_hub: 1;      /**< The endpoint is on a LS device that is routed through an FS hub.
												 Setting this bit will lead to the addition of the PREamble packet */
			uint32_t reserved2: 2;
		};
		uint32_t val;
	};
	struct {
		unsigned int interval;              /**< The interval of the endpoint in frames (FS) or microframes (HS) */
		uint32_t offset;                    /**< Offset of this channel in the periodic scheduler */
		bool is_hs;                         /**< This endpoint is HighSpeed. Needed for Periodic Frame List (HAL layer) scheduling */
	} periodic;     /**< Characteristic for periodic (interrupt/isochronous) endpoints only */
} uhc_dwc2_ep_char_t;
typedef enum {
	UHC_PIPE_STATE_ACTIVE,          /**< The pipe is active */
	UHC_PIPE_STATE_HALTED,          /**< The pipe is halted */
} pipe_state_t;

typedef struct {
	union {
		struct {
			uint32_t active: 1;             /**< Debugging bit to indicate whether channel is enabled */
			uint32_t halt_requested: 1;     /**< A halt has been requested */
			uint32_t reserved: 2;
			uint32_t chan_idx: 4;           /**< The index number of the channel */
			uint32_t reserved24: 24;
		};
		uint32_t val;
	} flags;                                 /**< Flags regarding channel's status and information */
	usb_dwc2_host_chan_regs_t *regs;         /**< Pointer to the channel's register set */
	/* TODO: Add channel error */
	uhc_dwc2_xfer_type_t type;               /**< The transfer type of the channel */
	void *chan_ctx;                          /**< Context variable for the owner of the channel */
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
	uhc_dwc2_speed_t dev_speed;             	/**< Speed of the device */
	uint8_t dev_addr;                       	/**< Device address of the pipe */
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
	struct uhc_transfer *xfer;            /**< Pointer to the transfer object associated with this buffer */
	union {
		struct {
			uint32_t data_stg_in: 1;        /**< Data stage of the control transfer is IN */
			uint32_t data_stg_skip: 1;      /**< Control transfer has no data stage */
			uint32_t cur_stg: 2;            /**< Index of the current stage (e.g., 0 is setup stage, 2 is status stage) */
			uint32_t set_addr: 1;           /**< Set address stage is in progress */
			uint32_t new_addr: 7;           /**< New address to set in the status stage */
			uint32_t reserved20: 20;
		} ctrl;                             /**< Control transfer related */
		struct {
			uint32_t zero_len_packet: 1;    /**< Added a zero length packet, so transfer consists of 2 QTDs */
			uint32_t reserved31: 31;
		} bulk;                             /**< Bulk transfer related */
		struct {
			uint32_t num_qtds: 8;           /**< Number of transfer descriptors filled (excluding zero length packet) */
			uint32_t zero_len_packet: 1;    /**< Added a zero length packet, so true number descriptors is num_qtds + 1 */
			uint32_t reserved23: 23;
		} intr;                             /**< Interrupt transfer related */
		struct {
			uint32_t num_qtds: 8;           /**< Number of transfer descriptors filled (including NULL descriptors) */
			uint32_t interval: 8;           /**< Interval (in number of SOF i.e., ms) */
			uint32_t start_idx: 8;          /**< Index of the first transfer descriptor in the list */
			uint32_t next_start_idx: 8;     /**< Index for the first descriptor of the next buffer */
		} isoc;
		uint32_t val;
	} flags;
	union {
		struct {
			uint32_t executing: 1;          /**< The buffer is currently executing */
			uint32_t was_canceled: 1;      	/**< Buffer was done due to a cancellation (i.e., a halt request) */
			uint32_t reserved6: 6;
			uint32_t stop_idx: 8;           /**< The descriptor index when the channel was halted */
			pipe_event_t pipe_event: 8; 	/**< The pipe event when the buffer was done */
			uint32_t reserved8: 8;
		};
		uint32_t val;
	} status_flags;                         /**< Status flags for the buffer */
} dma_buffer_t;

struct pipe_obj_s {
	/* XFER queuing related */
	sys_dlist_t xfer_pending_list;

	/* TODO: Lists of pending and done? */
	int num_xfer_pending;
	int num_xfer_done;

	/* Single-buffer control */
	dma_buffer_t *buffer;                         /**< Pointer to the buffer of the pipe */

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
			uint32_t has_xfer: 1;           /**< Indicates there is at least one XFER either pending, in-flight, or done */
			uint32_t event_pending: 1;      /**< Indicates that a pipe event is pending and needs to be processed */
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
	/* Defect workarounds */
	unsigned int wa_essregrestored : 1;
	/* Transfer triggers (IN on bits 0-15, OUT on bits 16-31) */
	atomic_t xfer_new;
	/* Finished transactions (IN on bits 0-15, OUT on bits 16-31) */
	atomic_t xfer_finished;
	uint32_t ghwcfg1;
	uint32_t max_xfersize;
	uint32_t max_pktcnt;
	/* Configuration flags */
	unsigned int dynfifosizing : 1;
	unsigned int bufferdma : 1;
	unsigned int syncrst : 1;
	/* Number of endpoints including control endpoint */
	uint8_t numdeveps;
	/* Number of IN endpoints including control endpoint */
	uint8_t ineps;
	/* Number of OUT endpoints including control endpoint */
	uint8_t outeps;
	/* Isochronous endpoint enabled (IN on bits 0-15, OUT on bits 16-31) */
	uint32_t iso_enabled;
	uint16_t dfifodepth;
	uint16_t rxfifo_depth;
	uint16_t max_txfifo_depth[16];

	/* TODO: FRAME LIST? */
	/* TODO: Pipes/channels LIST? */

	/* FIFO */
	uhc_dwc2_fifo_config_t fifo;

	/* Config */
	uhc_dwc2_constant_config_t const_cfg; /* Data, that doesn't changed after initialization */
	struct {
		size_t num_allocated; 				/**< Number of channels currently allocated */
		uint32_t pending_intrs_msk;    		/**< Bit mask of channels with pending interrupts */
		uhc_dwc2_channel_t **hdls;          /**< Handles of each channel. Set to NULL if channel has not been allocated */
	} channels; 							/* Data, that is used in single thread */

	struct {
		union {
			struct {
				uint32_t lock_enabled: 1;           /**< Debounce lock enabled */
				uint32_t fifo_sizes_set: 1;         /**< Whether the FIFO sizes have been set or not */
				uint32_t periodic_sched_enabled: 1; /**< Periodic scheduling (for interrupt and isochronous transfers) is enabled */
				uint32_t event_pending: 1;          /**< Port event is pending */
				uint32_t conn_dev_ena: 1;           /**< Device connected to the port */
				uint32_t waiting_disable: 1;        /**< Waiting for the port to be disabled */
				uint32_t reserved: 2;
				uint32_t reserved24: 24;
			};
			uint32_t val;
		} flags;
		uhc_port_event_t last_event;
		uhc_port_state_t port_state;
	} dynamic; /* Data, that is used in multiple threads */

	/* TODO: Dynamic pipe allocation on enqueue? */
	pipe_t pipe;    								/**< Static CTRL pipe per port */
	pipe_hdl_t ctrl_pipe_hdl; 						/**< CTRL pipe handle */
	uint8_t num_pipes_idle; 						/**< Number of idle pipes */
	uint8_t num_pipes_queued; 						/**< Number of pipes queued for processing */
};

/* Minimum RX FIFO size in 32-bit words considering the largest used OUT packet
 * of 512 bytes. The value must be adjusted according to the number of OUT
 * endpoints.
 */
#define UHC_DWC2_GRXFSIZ_FS_DEFAULT	(15U + 512U/4U)
/* Default Rx FIFO size in 32-bit words calculated to support High-Speed with:
 *   * 1 control endpoint in Completer/Buffer DMA mode: 13 locations
 *   * Global OUT NAK: 1 location
 *   * Space for 3 * 1024 packets: ((1024/4) + 1) * 3 = 774 locations
 * Driver adds 2 locations for each OUT endpoint to this value.
 */
#define UHC_DWC2_GRXFSIZ_HS_DEFAULT	(13 + 1 + 774)

/* TX FIFO0 depth in 32-bit words (used by control IN endpoint)
 * Try 2 * bMaxPacketSize0 to allow simultaneous operation with a fallback to
 * whatever is available when 2 * bMaxPacketSize0 is not possible.
 */
#define UHC_DWC2_FIFO0_DEPTH		(2 * 16U)

#if defined(CONFIG_PINCTRL)
#include <zephyr/drivers/pinctrl.h>

static int dwc2_init_pinctrl(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	const struct pinctrl_dev_config *const pcfg = config->pcfg;
	int ret = 0;

	if (pcfg == NULL) {
		LOG_INF("Skip pinctrl configuration");
		return 0;
	}

	ret = pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to apply default pinctrl state (%d)", ret);
	}

	LOG_DBG("Apply pinctrl");

	return ret;
}
#else
static int dwc2_init_pinctrl(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}
#endif

static inline struct usb_dwc2_reg *dwc2_get_base(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;

	return config->base;
}

static int dwc2_core_soft_reset(const struct device *dev)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	mem_addr_t grstctl_reg = (mem_addr_t)&base->grstctl;
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

	return 0;
}

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
	fifo->top = const_cfg->fifo.depth;
	fifo->top -= const_cfg->num_channels;

	/* TODO: support HS */

	uint32_t nptx_largest = EPSIZE_BULK_FS / 4;
	uint32_t ptx_largest = 256 / 4;

	fifo->nptxfsiz = 2 * nptx_largest;
	fifo->rxfsiz = 2 * (ptx_largest + 2) + const_cfg->num_channels;
	fifo->ptxfsiz = fifo->top - (fifo->nptxfsiz + fifo->rxfsiz);

	/* TODO: verify ptxfsiz is overflowed */

	LOG_DBG("FIFO sizes calculated");
	LOG_DBG("\ttop=%u, nptx=%u, rx=%u, ptx=%u", fifo->top * 4,  fifo->nptxfsiz * 4, fifo->rxfsiz * 4, fifo->ptxfsiz * 4);
}

/* =============================================================================================== */
/* =================================== DWC2 HAL Functions ======================================== */
/* =============================================================================================== */

static inline void dwc2_ll_set_frame_list(struct usb_dwc2_reg *const dwc2, void *frame_list)
{
	// LOG_WRN("Setting frame list not implemented yet");
}

static inline void dwc2_ll_periodic_enable(struct usb_dwc2_reg *const dwc2)
{
	// LOG_WRN("Enabling periodic scheduling not implemented yet");
}

static inline void dwc2_hal_port_init(struct usb_dwc2_reg *const dwc2)
{
	dwc2_ll_haintmsk_dis_chan_intr(dwc2, 0xFFFFFFFF);
	dwc2_ll_gintmsk_en_intrs(dwc2, USB_DWC2_GINTSTS_PRTINT | USB_DWC2_GINTSTS_HCHINT);
}

static int dwc2_hal_toggle_power(struct usb_dwc2_reg *const dwc2, bool power_on)
{
	if (power_on) {
		dwc2_ll_hprt_en_pwr(dwc2);
	} else {
		dwc2_ll_hprt_dis_pwr(dwc2);
	}
	return 0;
}

static inline bool dwc2_hal_is_dma_supported(struct usb_dwc2_reg *const dwc2)
{
	if (IS_ENABLED(CONFIG_UHC_DWC2_DMA)) {
		usb_dwc2_ghwcfg2_reg_t ghwcfg2_reg = dwc2_ll_ghwcfg2_read_reg(dwc2);
		return (ghwcfg2_reg.arch == USB_DWC2_GHWCFG2_OTGARCH_INTERNALDMA);
	} else {
		return false;
	}
}

static inline int dwc2_hal_load_config(struct usb_dwc2_reg *const dwc2, uhc_dwc2_constant_config_t *const cfg)
{
	LOG_DBG("GSNPSID=%08Xh, GHWCFG1=%08Xh, GHWCFG2=%08Xh, GHWCFG3=%08Xh, GHWCFG4=%08Xh",
			dwc2->gsnpsid, dwc2->ghwcfg1, dwc2->ghwcfg2, dwc2->ghwcfg3, dwc2->ghwcfg4);

	// Check Synopsis ID register, failed if controller clock/power is not enabled
	__ASSERT((dwc2->gsnpsid == USB_DWC2_GSNPSID_REV_5_00A),
			 "DWC2 core ID is not compatible with the driver, GSNPSID: 0x%08x",
			 dwc2->gsnpsid);

	if (dwc2->gsnpsid == 0) {
		LOG_ERR("Unable to read DWC2 Core ID, core is not powered on");
		return -ENODEV;
	}

	usb_dwc2_ghwcfg2_reg_t ghwcfg2_reg = dwc2_ll_ghwcfg2_read_reg(dwc2);
	usb_dwc2_ghwcfg3_reg_t ghwcfg3_reg = dwc2_ll_ghwcfg3_read_reg(dwc2);
	usb_dwc2_ghwcfg4_reg_t ghwcfg4_reg = dwc2_ll_ghwcfg4_read_reg(dwc2);

	cfg->fifo.depth = ghwcfg3_reg.dfifodepth;
	/* TODO: Support dedicated FIFO mode */
	cfg->fifo.flags.dedicated = ghwcfg4_reg.dedfifomode;
	cfg->fifo.flags.dynamic = ghwcfg2_reg.enabledynamicfifo;

	cfg->hsphy_type = ghwcfg2_reg.hsphytype;
	cfg->fsphy_type = ghwcfg2_reg.fsphytype;
	cfg->num_channels = ghwcfg2_reg.numhostch + 1;

	/* TODO: Support different speed modes */

	/* TODO: Support vendor control interface */

	/* TODO: Support LPM */

	/* TODO: Support synch reset */

	cfg->dma = dwc2_hal_is_dma_supported(dwc2);
	return 0;
}

static void dwc2_hal_channel_configure(uhc_dwc2_channel_t *chan_obj, uhc_dwc2_ep_char_t *ep_char)
{
	__ASSERT(!chan_obj->flags.active && !chan_obj->flags.halt_requested,
			 "Cannot change endpoint characteristics while channel is active or halted");

	dwc2_ll_hcchar_init_channel(chan_obj->regs,
							 ep_char->dev_addr,
							 USB_EP_GET_IDX(ep_char->bEndpointAddress),
							 ep_char->mps,
							 ep_char->type,
							 USB_EP_GET_DIR(ep_char->bEndpointAddress),
							 ep_char->ls_via_fs_hub);
	chan_obj->type = ep_char->type;

	if (ep_char->type == UHC_DWC2_XFER_TYPE_ISOCHRONOUS || ep_char->type == UHC_DWC2_XFER_TYPE_INTR) {
		LOG_WRN("ISOC and INTR channels are note supported yet");
	}
}

static inline void dwc2_hal_set_fifo_config(struct usb_dwc2_reg *const dwc2, uhc_dwc2_fifo_config_t *const fifo)
{
	/* Set FIFO top */
	uint16_t fifo_available = fifo->top;

	dwc2_ll_gdfifocfg_set_ep_info_base_addr(dwc2, fifo_available);
	dwc2_ll_gdfifocfg_set_gdfifo_cfg(dwc2, fifo_available);

	fifo_available -= fifo->rxfsiz;

	dwc2_ll_grxfsiz_set_rx_fifo_depth(dwc2, fifo->rxfsiz);

	fifo_available -= fifo->nptxfsiz;

	dwc2_ll_gnptxfsiz_set_nptx_fifo_start_addr(dwc2, fifo_available);
	dwc2_ll_gnptxfsiz_set_nptx_fifo_depth(dwc2, fifo->nptxfsiz);

	fifo_available -= fifo->ptxfsiz;

	dwc2_ll_hptxfsiz_set_host_tx_fifo_start_addr(dwc2, fifo_available);
	dwc2_ll_hptxfsiz_set_host_tx_fifo_depth(dwc2, fifo->ptxfsiz);

	dwc2_ll_grstctl_flush_tx_fifo(dwc2, 0x10);
	dwc2_ll_grstctl_flush_rx_fifo(dwc2);

	LOG_DBG("FIFO sizes configured");
	LOG_DBG("\tnptx=%u, rx=%u, ptx=%u", fifo->nptxfsiz * 4, fifo->rxfsiz * 4, fifo->ptxfsiz * 4);
}

static inline int dwc2_hal_port_get_speed(const struct device *dev, uhc_dwc2_speed_t *speed)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	if (priv->dynamic.port_state != UHC_PORT_STATE_ENABLED) {
		LOG_ERR("Port is not enabled, cannot get speed");
		return -ENODEV;
	}
	*speed = (uhc_dwc2_speed_t)dwc2_ll_hprt_get_port_speed(dwc2);
	return 0;
}

static inline void dwc2_hal_port_enable(const struct device *dev, struct usb_dwc2_reg *const dwc2)
{
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

	dwc2_ll_hcfg_en_buffer_dma(dwc2);
	dwc2_ll_hcfg_dis_perio_sched(dwc2);

	uhc_dwc2_speed_t speed = dwc2_ll_hprt_get_port_speed(dwc2);

	if (priv->const_cfg.hsphy_type == 0) {
		dwc2_ll_hcfg_set_fsls_phy_clock(dwc2, speed);
		dwc2_ll_hfir_set_frame_interval(dwc2, speed);
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
	dwc2_ll_gintmsk_dis_intrs(dwc2, USB_DWC2_GINTSTS_PRTINT | USB_DWC2_GINTSTS_DISCONNINT);
	priv->dynamic.flags.lock_enabled = 1;
}

static inline void uhc_dwc2_lock_disable(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data_s *const priv = uhc_get_private(dev);
	priv->dynamic.flags.lock_enabled = 0;
	/* Clear Connection and disconnection interrupt in case it triggered again */
	dwc2_ll_gintsts_clear_intrs(dwc2, USB_DWC2_GINTSTS_DISCONNINT);
	dwc2_ll_hprt_intr_clear(dwc2, USB_DWC2_HPRT_PRTCONNDET);
	/* Re-enable the hprt (connection) and disconnection interrupts */
	dwc2_ll_gintmsk_en_intrs(dwc2, USB_DWC2_GINTSTS_PRTINT | USB_DWC2_GINTSTS_DISCONNINT);
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

static inline void uhc_dwc2_config_phy(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	/* Init PHY based on the speed */
	if (priv->const_cfg.hsphy_type != 0) {
#if (0)
		uint32_t gusbcfg = dwc2->gusbcfg;

		// De-select FS PHY
		gusbcfg &= ~GUSBCFG_PHYSEL;

		if (dwc2->ghwcfg2_reg.hs_phy_type == GHWCFG2_HSPHY_ULPI) {
			LOG_WRN("Highspeed ULPI PHY init");
			// Select ULPI PHY (external)
			gusbcfg |= GUSBCFG_ULPI_UTMI_SEL;
			// ULPI is always 8-bit interface
			gusbcfg &= ~GUSBCFG_PHYIF16;
			// ULPI select single data rate
			gusbcfg &= ~GUSBCFG_DDRSEL;
			// default internal VBUS Indicator and Drive
			gusbcfg &= ~(GUSBCFG_ULPIEVBUSD | GUSBCFG_ULPIEVBUSI);
			// Disable FS/LS ULPI
			gusbcfg &= ~(GUSBCFG_ULPIFSLS | GUSBCFG_ULPICSM);
		} else {
			LOG_WRN("Highspeed UTMI+ PHY init");
			// Select UTMI+ PHY (internal)
			gusbcfg &= ~GUSBCFG_ULPI_UTMI_SEL;
			// Set 16-bit interface if supported
			if (dwc2->ghwcfg4_reg.phy_data_width) {
				gusbcfg |= GUSBCFG_PHYIF16; // 16 bit
			} else {
				gusbcfg &= ~GUSBCFG_PHYIF16; // 8 bit
			}
		}

		// Apply config
		dwc2->gusbcfg = gusbcfg;

		// TODO: vendor quirk specific phy init

		// Reset core after selecting PHY
		ret = dwc2_core_soft_reset(dwc2);

		// Set turn-around, must after core reset otherwise it will be clear
		// - 9 if using 8-bit PHY interface
		// - 5 if using 16-bit PHY interface
		gusbcfg &= ~GUSBCFG_TRDT_Msk;
		gusbcfg |= (dwc2->ghwcfg4_reg.phy_data_width ? 5u : 9u) << GUSBCFG_TRDT_Pos;
		dwc2->gusbcfg = gusbcfg;

		// TODO: vendor specific quirk phy update post reset
#else
		LOG_WRN("HS PHY config not implemented yet");
#endif
	} else {
		dwc2_ll_gusbcfg_en_fs_phy(dwc2);

		/* TODO: vendor quirk specific phy init */

		/* Reset core after selecting PHY */
		// ret = dwc2_core_soft_reset(dwc2);

		/*
		* USB turnaround time is critical for certification where long cables and 5-Hubs are used.
		* So if you need the AHB to run at less than 30 MHz, and if USB turnaround time is not critical,
		* these bits can be programmed to a larger value. Default is 5
		*/
		dwc2_ll_gusbcfg_set_trdtim(dwc2, 5);

		/* TODO: vendor specific quirk phy update post reset */
	}
}

static inline void uhc_dwc2_set_defaults(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const dwc2 = config->base;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

	dwc2_ll_gahbcfg_dis_global_intrs(dwc2);
	dwc2_ll_gusbcfg_en_host_mode(dwc2);

	/* TODO: Set AHB burst mode for some ECO only for ESP32S2 */
	/* Make config quirk? */

	/* TODO: Disable HNP and SRP capabilities */

	dwc2_ll_gintmsk_dis_intrs(dwc2, 0xFFFFFFFF);
	dwc2_ll_gintmsk_en_intrs(dwc2, CORE_INTRS_EN_MSK);
	dwc2_ll_gintsts_read_and_clear_intrs(dwc2);
	dwc2_ll_gahbcfg_en_global_intrs(dwc2);

	while ((sys_read32((mem_addr_t)&dwc2->gintsts) & USB_DWC2_GINTSTS_CURMOD) != 1) {
	}

	dwc2_ll_gahbcfg_nptx_half_empty_lvl(dwc2, true);
	dwc2_ll_gahbcfg_set_hbstlen(dwc2, 7);

	if (priv->const_cfg.dma) {
		dwc2_ll_gahbcfg_en_dma(dwc2);
	}
}

static inline int uhc_dwc2_reset_core_soft(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;
	const unsigned int csr_timeout_us = 10000UL;
	uint32_t cnt = 0UL;

	LOG_DBG("Performing DWC2 core soft reset and config controller");

	dwc2_ll_grstctl_core_soft_reset(dwc2);
	while (dwc2_ll_grstctl_is_core_reset_in_progress(dwc2)) {
		/* Wait until core reset is done or timeout occurs */
		k_busy_wait(1);
		if (++cnt > csr_timeout_us) {
			LOG_ERR("Wait for core soft reset timeout");
			return -EIO;
		}

	}
	cnt = 0UL;
	while (!dwc2_ll_grstctl_is_ahb_idle(dwc2)) {
		/* Wait until AHB Master bus is idle before doing any other operations */
		k_busy_wait(1);
		if (++cnt > csr_timeout_us) {
			LOG_ERR("Wait for AHB idle timeout");
			return -EIO;
		}
	}

	uhc_dwc2_set_defaults(dev);

	/* Update the port state and flags */
	priv->dynamic.port_state = UHC_PORT_STATE_NOT_POWERED;
	priv->dynamic.last_event = UHC_PORT_EVENT_NONE;
	priv->dynamic.flags.val = 0;

	/* TODO: Clear all the flags and channels */
	priv->channels.num_allocated = 0;
	priv->channels.pending_intrs_msk = 0;
	if (priv->channels.hdls) {
	    for (int i = 0; i < priv->const_cfg.num_channels; i++) {
	        priv->channels.hdls[i] = NULL;
	    }
	}
	return 0;
}

static int uhc_dwc2_init_controller(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const base = config->base;
	struct usb_dwc2_reg *const dwc2 = config->base;
	mem_addr_t grxfsiz_reg = (mem_addr_t)&base->grxfsiz;
	mem_addr_t gahbcfg_reg = (mem_addr_t)&base->gahbcfg;
	mem_addr_t gusbcfg_reg = (mem_addr_t)&base->gusbcfg;
	mem_addr_t dcfg_reg = (mem_addr_t)&base->dcfg;
	mem_addr_t dctl_reg = (mem_addr_t)&base->dctl;
	uint32_t gsnpsid;
	uint32_t dcfg;
	uint32_t dctl;
	uint32_t gusbcfg;
	uint32_t gahbcfg;
	uint32_t gintmsk;
	uint32_t ghwcfg2;
	uint32_t ghwcfg3;
	uint32_t ghwcfg4;
	uint32_t val;
	int ret;
	bool hs_phy;

	ret = dwc2_hal_load_config(dwc2, &priv->const_cfg);
	if (ret) {
		LOG_ERR("Failed to get DWC2 core parameters: %d", ret);
		return ret;
	}

	LOG_DBG("DWC2 Core parameters");
	LOG_DBG("\tFIFO:");
	LOG_DBG("\t\t Depth %u", priv->const_cfg.fifo.depth);
	LOG_DBG("\t\t Dedicated: %s", priv->const_cfg.fifo.flags.dedicated ? "YES" : "NO");
	LOG_DBG("\t\t Dynamic sizing: %s", priv->const_cfg.fifo.flags.dynamic ? "YES" : "NO");
	LOG_DBG("\tNumber of channels: %u", priv->const_cfg.num_channels);
	LOG_DBG("\tHS PHY type: 0x%08x", priv->const_cfg.hsphy_type);
	LOG_DBG("\tFS PHY type: 0x%08x", priv->const_cfg.fsphy_type);
	LOG_DBG("\tDMA supported: %s", priv->const_cfg.dma ? "YES" : "NO");

	uhc_dwc2_config_phy(dev);
	uhc_dwc2_config_fifo_fixed_dma(&priv->const_cfg, &priv->fifo);

	ret = dwc2_core_soft_reset(dev);
	if (ret) {
		return ret;
	}

	/* Enable RTL workarounds based on controller revision */
	gsnpsid = sys_read32((mem_addr_t)&base->gsnpsid);
	priv->wa_essregrestored = gsnpsid < USB_DWC2_GSNPSID_REV_5_00A;

	priv->ghwcfg1 = sys_read32((mem_addr_t)&base->ghwcfg1);
	ghwcfg2 = sys_read32((mem_addr_t)&base->ghwcfg2);
	ghwcfg3 = sys_read32((mem_addr_t)&base->ghwcfg3);
	ghwcfg4 = sys_read32((mem_addr_t)&base->ghwcfg4);

	if (!(ghwcfg4 & USB_DWC2_GHWCFG4_DEDFIFOMODE)) {
		LOG_ERR("Only dedicated TX FIFO mode is supported");
		return -ENOTSUP;
	}

	/*
	 * Force device mode as we do no support role changes.
	 * Wait 25ms for the change to take effect.
	 */
	gusbcfg = USB_DWC2_GUSBCFG_FORCEDEVMODE;
	sys_write32(gusbcfg, gusbcfg_reg);
	k_msleep(25);

	/* Buffer DMA is always supported in Internal DMA mode.
	 * TODO: check and support descriptor DMA if available
	 */
	priv->bufferdma = (usb_dwc2_get_ghwcfg2_otgarch(ghwcfg2) ==
			   USB_DWC2_GHWCFG2_OTGARCH_INTERNALDMA);

	if (!IS_ENABLED(CONFIG_UHC_DWC2_DMA)) {
		priv->bufferdma = 0;
	} else if (priv->bufferdma) {
		LOG_WRN("Experimental DMA enabled");
	}

	if (ghwcfg2 & USB_DWC2_GHWCFG2_DYNFIFOSIZING) {
		LOG_DBG("Dynamic FIFO Sizing is enabled");
		priv->dynfifosizing = true;
	}

	/* Get the number or endpoints and IN endpoints we can use later */
	priv->numdeveps = usb_dwc2_get_ghwcfg2_numdeveps(ghwcfg2) + 1U;
	priv->ineps = usb_dwc2_get_ghwcfg4_ineps(ghwcfg4) + 1U;
	LOG_DBG("Number of endpoints (NUMDEVEPS + 1) %u", priv->numdeveps);
	LOG_DBG("Number of IN endpoints (INEPS + 1) %u", priv->ineps);

	LOG_DBG("Number of periodic IN endpoints (NUMDEVPERIOEPS) %u",
		usb_dwc2_get_ghwcfg4_numdevperioeps(ghwcfg4));
	LOG_DBG("Number of additional control endpoints (NUMCTLEPS) %u",
		usb_dwc2_get_ghwcfg4_numctleps(ghwcfg4));

	LOG_DBG("OTG architecture (OTGARCH) %u, mode (OTGMODE) %u",
		usb_dwc2_get_ghwcfg2_otgarch(ghwcfg2),
		usb_dwc2_get_ghwcfg2_otgmode(ghwcfg2));

	priv->dfifodepth = usb_dwc2_get_ghwcfg3_dfifodepth(ghwcfg3);
	LOG_DBG("DFIFO depth (DFIFODEPTH) %u bytes", priv->dfifodepth * 4);

	priv->max_pktcnt = GHWCFG3_PKTCOUNT(usb_dwc2_get_ghwcfg3_pktsizewidth(ghwcfg3));
	priv->max_xfersize = GHWCFG3_XFERSIZE(usb_dwc2_get_ghwcfg3_xfersizewidth(ghwcfg3));
	LOG_DBG("Max packet count %u, Max transfer size %u",
		priv->max_pktcnt, priv->max_xfersize);

	LOG_DBG("Vendor Control interface support enabled: %s",
		(ghwcfg3 & USB_DWC2_GHWCFG3_VNDCTLSUPT) ? "true" : "false");

	LOG_DBG("PHY interface type: FSPHYTYPE %u, HSPHYTYPE %u, DATAWIDTH %u",
		usb_dwc2_get_ghwcfg2_fsphytype(ghwcfg2),
		usb_dwc2_get_ghwcfg2_hsphytype(ghwcfg2),
		usb_dwc2_get_ghwcfg4_phydatawidth(ghwcfg4));

	LOG_DBG("LPM mode is %s",
		(ghwcfg3 & USB_DWC2_GHWCFG3_LPMMODE) ? "enabled" : "disabled");

	if (ghwcfg3 & USB_DWC2_GHWCFG3_RSTTYPE) {
		priv->syncrst = 1;
	}

	/* Configure AHB, select Completer or DMA mode */
	gahbcfg = sys_read32(gahbcfg_reg);

	if (priv->bufferdma) {
		gahbcfg |= USB_DWC2_GAHBCFG_DMAEN;
	} else {
		gahbcfg &= ~USB_DWC2_GAHBCFG_DMAEN;
	}

	sys_write32(gahbcfg, gahbcfg_reg);

	dcfg = sys_read32(dcfg_reg);

	dcfg &= ~USB_DWC2_DCFG_DESCDMA;

	/* Configure PHY and device speed */
	dcfg &= ~USB_DWC2_DCFG_DEVSPD_MASK;
	switch (usb_dwc2_get_ghwcfg2_hsphytype(ghwcfg2)) {
	case USB_DWC2_GHWCFG2_HSPHYTYPE_UTMIPLUSULPI:
		__fallthrough;
	case USB_DWC2_GHWCFG2_HSPHYTYPE_ULPI:
		gusbcfg |= USB_DWC2_GUSBCFG_PHYSEL_USB20 |
			   USB_DWC2_GUSBCFG_ULPI_UTMI_SEL_ULPI;
		if (IS_ENABLED(CONFIG_UHC_DRIVER_HIGH_SPEED_SUPPORT_ENABLED)) {
			dcfg |= usb_dwc2_set_dcfg_devspd(USB_DWC2_DCFG_DEVSPD_USBHS20);
		} else {
			dcfg |= usb_dwc2_set_dcfg_devspd(USB_DWC2_DCFG_DEVSPD_USBFS20);
		}
		hs_phy = true;
		break;
	case USB_DWC2_GHWCFG2_HSPHYTYPE_UTMIPLUS:
		gusbcfg |= USB_DWC2_GUSBCFG_PHYSEL_USB20 |
			   USB_DWC2_GUSBCFG_ULPI_UTMI_SEL_UTMI;
		if (IS_ENABLED(CONFIG_UHC_DRIVER_HIGH_SPEED_SUPPORT_ENABLED)) {
			dcfg |= usb_dwc2_set_dcfg_devspd(USB_DWC2_DCFG_DEVSPD_USBHS20);
		} else {
			dcfg |= usb_dwc2_set_dcfg_devspd(USB_DWC2_DCFG_DEVSPD_USBFS20);
		}
		hs_phy = true;
		break;
	case USB_DWC2_GHWCFG2_HSPHYTYPE_NO_HS:
		__fallthrough;
	default:
		if (usb_dwc2_get_ghwcfg2_fsphytype(ghwcfg2) !=
		    USB_DWC2_GHWCFG2_FSPHYTYPE_NO_FS) {
			gusbcfg |= USB_DWC2_GUSBCFG_PHYSEL_USB11;
		}

		dcfg |= usb_dwc2_set_dcfg_devspd(USB_DWC2_DCFG_DEVSPD_USBFS1148);
		hs_phy = false;
	}

	if (usb_dwc2_get_ghwcfg4_phydatawidth(ghwcfg4)) {
		gusbcfg |= USB_DWC2_GUSBCFG_PHYIF_16_BIT;
	}

	/* Update PHY configuration */
	sys_write32(gusbcfg, gusbcfg_reg);
	sys_write32(dcfg, dcfg_reg);

	/* Configure Periodic Transfer Interrupt feature */
	dctl = USB_DWC2_DCTL_SFTDISCON;
	if (IS_ENABLED(CONFIG_UHC_DWC2_PTI)) {
		dctl |= USB_DWC2_DCTL_IGNRFRMNUM;
	}
	sys_write32(dctl, dctl_reg);

	LOG_DBG("Number of OUT endpoints %u", priv->outeps);

	/* Read and store all TxFIFO depths because Programmed FIFO Depths must
	 * not exceed the power-on values.
	 */
	val = sys_read32((mem_addr_t)&base->gnptxfsiz);

	priv->rxfifo_depth = usb_dwc2_get_grxfsiz(sys_read32(grxfsiz_reg));

	if (priv->dynfifosizing) {
		uint32_t gnptxfsiz;
		uint32_t default_depth;

		/* TODO: For proper runtime FIFO sizing UHC driver would have to
		 * have prior knowledge of the USB configurations. Only with the
		 * prior knowledge, the driver will be able to fairly distribute
		 * available resources. For the time being just use different
		 * defaults based on maximum configured PHY speed, but this has
		 * to be revised if e.g. thresholding support would be necessary
		 * on some target.
		 */
		if (hs_phy) {
			default_depth = UHC_DWC2_GRXFSIZ_HS_DEFAULT;
		} else {
			default_depth = UHC_DWC2_GRXFSIZ_FS_DEFAULT;
		}
		default_depth += priv->outeps * 2U;

		/* Driver does not dynamically resize RxFIFO so there is no need
		 * to store reset value. Read the reset value and make sure that
		 * the programmed value is not greater than what driver sets.
		 */
		priv->rxfifo_depth = MIN(priv->rxfifo_depth, default_depth);
		sys_write32(usb_dwc2_set_grxfsiz(priv->rxfifo_depth), grxfsiz_reg);

		/* Set TxFIFO 0 depth */
		val = MIN(UHC_DWC2_FIFO0_DEPTH, priv->max_txfifo_depth[0]);
		gnptxfsiz = usb_dwc2_set_gnptxfsiz_nptxfdep(val) |
			    usb_dwc2_set_gnptxfsiz_nptxfstaddr(priv->rxfifo_depth);

		sys_write32(gnptxfsiz, (mem_addr_t)&base->gnptxfsiz);
	}

	/* Unmask interrupts */
	gintmsk = USB_DWC2_GINTSTS_OEPINT | USB_DWC2_GINTSTS_IEPINT |
		  USB_DWC2_GINTSTS_ENUMDONE | USB_DWC2_GINTSTS_USBRST |
		  USB_DWC2_GINTSTS_WKUPINT | USB_DWC2_GINTSTS_USBSUSP |
		  USB_DWC2_GINTSTS_GOUTNAKEFF;

	if (IS_ENABLED(CONFIG_UHC_ENABLE_SOF)) {
		gintmsk |= USB_DWC2_GINTSTS_SOF;

		if (!IS_ENABLED(CONFIG_UHC_DWC2_PTI)) {
			gintmsk |= USB_DWC2_GINTSTS_INCOMPISOOUT |
				   USB_DWC2_GINTSTS_INCOMPISOIN;
		}
	}

	sys_write32(gintmsk, (mem_addr_t)&base->gintmsk);

	return 0;
}

static uhc_port_event_t uhc_dwc2_decode_hprt(const struct device *dev, uhc_dwc2_core_event_t core_event)
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
			/* Disabled could be due to a disable request or reset request, or due to a port error */
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

	uhc_dwc2_core_event_t core_event = UHC_DWC2_CORE_EVENT_NONE;
	uint32_t core_intrs = dwc2_ll_gintsts_read_and_clear_intrs(dwc2);
	uint32_t port_intrs = 0;

	if (core_intrs & USB_DWC2_GINTSTS_PRTINT) {
		/* There are host port interrupts, read and clear those as well */
		port_intrs = dwc2_ll_hprt_intr_read_and_clear(dwc2);
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
			core_event = UHC_DWC2_CORE_EVENT_DISCONN;
			/* Debounce lock */
			uhc_dwc2_lock_enable(dev);
		} else if (port_intrs & USB_DWC2_HPRT_PRTOVRCURRCHNG) {
			/* Check if this is an overcurrent or an overcurrent cleared */
			if (dwc2_ll_hprt_get_port_overcur(dwc2)) {
				core_event = UHC_DWC2_CORE_EVENT_OVRCUR;
			} else {
				core_event = UHC_DWC2_CORE_EVENT_OVRCUR_CLR;
			}
		} else if (port_intrs & USB_DWC2_HPRT_PRTENCHNG) {
			if (dwc2_ll_hprt_get_port_en(dwc2)) {
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
		}
	}
	/* Port events always take precedence over channel events */
	if (core_event == UHC_DWC2_CORE_EVENT_NONE && (core_intrs & USB_DWC2_GINTSTS_HCHINT)) {
		/* One or more channels have pending interrupts. Store the mask of those channels */
		priv->channels.pending_intrs_msk = dwc2_ll_haint_get_chan_intrs(dwc2);
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
	uint32_t chan_intrs = dwc2_ll_hcint_read_and_clear_intrs(chan_obj->regs);
	dwc2_hal_chan_event_t chan_event;

	/* Note: We don't assert on (chan_obj->flags.active) here as it could have been already cleared by usb_dwc_hal_chan_request_halt() */

	/*
	 * Note:
	 * Do not change order of checks as some events take precedence over others.
	 * Errors > Channel Halt Request > Transfer completed
	*/
	if (chan_intrs & CHAN_INTRS_ERROR_MSK) {
		__ASSERT(chan_intrs & USB_DWC2_HCINT_CHHLTD,
			 "uhc_dwc2_hal_chan_decode_intr: Channel error without channel halted interrupt");

		LOG_ERR("Channel %d error: 0x%08x", chan_obj->flags.chan_idx, chan_intrs);
		/* TODO: Store the error in hal context */
		chan_event = DWC2_CHAN_EVENT_ERROR;
	} else if (chan_intrs & USB_DWC2_HCINT_CHHLTD) {
		if (chan_obj->flags.halt_requested) {
			chan_obj->flags.halt_requested = 0;
			chan_event = DWC2_CHAN_EVENT_HALT_REQ;
		} else {
			chan_event = DWC2_CHAN_EVENT_CPLT;
		}
		chan_obj->flags.active = 0;
	} else if (chan_intrs & USB_DWC2_HCINT_XFERCOMPL) {
		/*
		 * A transfer complete interrupt WITHOUT the channel halting only occurs when receiving a short interrupt IN packet
		 * and the underlying QTD does not have the HOC bit set. This signifies the last packet of the Interrupt transfer
		 * as all interrupt packets must MPS sized except the last.
		*/

		/* The channel isn't halted yet, so we need to halt it manually to stop the execution of the next packet */
		dwc2_ll_hcchar_dis_channel(chan_obj->regs, true);

		/*
		 * After setting the halt bit, this will generate another channel halted interrupt. We treat this interrupt as
		 * a NONE event, then cycle back with the channel halted interrupt to handle the CPLT event.
		*/
		chan_event = DWC2_CHAN_EVENT_NONE;
	} else {
		__ASSERT(false,
				"uhc_dwc2_hal_chan_decode_intr: Unknown channel interrupt: 0x%08x",
				chan_intrs);
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

static inline ctrl_stage_t cal_next_pid(ctrl_stage_t pid, uint8_t pkt_count) {
  if (pkt_count & 0x01) {
    return pid ^ 0x02; // toggle DATA0 and DATA1
  } else {
    return pid;
  }
}

static void IRAM_ATTR _buffer_exec_proceed(pipe_t *pipe)
{
	dma_buffer_t *buffer_exec = pipe->buffer;
	struct uhc_transfer *const xfer = pipe->buffer->xfer;

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
			next_dir_is_in = true;             /* With no data stage, status stage must be IN */
			next_pid = CTRL_STAGE_DATA1;       /* Status stage always has a PID of DATA1 */
			buffer_exec->flags.ctrl.cur_stg = 2;    /* Skip over the null descriptor representing the skipped data stage */
		} else {
			/* Go to data stage */
			next_dir_is_in = buffer_exec->flags.ctrl.data_stg_in;
			next_pid = CTRL_STAGE_DATA1;        /* Data stage always starts with a PID of DATA1 */
			buffer_exec->flags.ctrl.cur_stg = 1;

			/* NOTE:
			* For OUT - number of bytes host sends to device
			* For IN - number of bytes host reserves to receive
			*/
			size = xfer->buf->size;

			/* TODO: Toggle PID? */

			/* TODO: check if the buffer is large enough for the next transfer */
			/* TODO: check that the buffer is DMA and CACHE aligned and compatible with the DMA controller (better to do this on enqueue) */
			if (xfer->buf != NULL) {
				dma_addr = net_buf_tail(xfer->buf);             /* Get the tail of the buffer to append data */
				net_buf_add(xfer->buf, size);                   /* Ensure the buffer has enough space for the next transfer */
			}
		}
	} else {        /* cur_stg == 1. // Just finished data stage. Go to status stage */
		next_dir_is_in = !buffer_exec->flags.ctrl.data_stg_in;  /* Status stage is always the opposite direction of data stage */
		next_pid = CTRL_STAGE_DATA1;   /* Status stage always has a PID of DATA1 */
		buffer_exec->flags.ctrl.cur_stg = 2;
	}

	/* Calculate new packet count */
	const uint16_t pkt_cnt = calc_packet_count(size, pipe->ep_char.mps);

	dwc2_ll_hcchar_set_dir(pipe->chan_obj->regs, next_dir_is_in);
	dwc2_ll_hctsiz_prep_transfer(pipe->chan_obj->regs, next_pid, pkt_cnt, size);
	dwc2_ll_hctsiz_do_ping(pipe->chan_obj->regs, false);
	dwc2_ll_hcdma_set_buffer_addr(pipe->chan_obj->regs, dma_addr);

	/* TODO: Configure split transaction if needed */

	/* TODO: sync CACHE */

	dwc2_ll_hcchar_en_channel(pipe->chan_obj->regs, true);
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
		 * Halt request event is triggered when packet is successful completed. But just treat all halted transfers as errors
		 * pipe->state = UHC_PIPE_STATE_HALTED;
		 * Notify the task waiting for the pipe halt or halt it right away
		 * _internal_pipe_event_notify(pipe, true);
		 */
		break;
	}
	default:
	__ASSERT(false,
			"uhc_dwc2_decode_chan: Unknown channel event: %s",
			dwc2_chan_event_str[chan_event]);
		break;
	}

	return pipe_event;
}

static IRAM_ATTR void _buffer_exec(pipe_t *pipe)
{
	dma_buffer_t *buffer_exec = pipe->buffer;
	struct uhc_transfer *const xfer = (struct uhc_transfer *)buffer_exec->xfer;

	LOG_DBG("ep=%02X, mps=%d", xfer->ep, pipe->ep_char.mps);

	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		/* Control stage is always OUT */
		dwc2_ll_hcchar_set_dir(pipe->chan_obj->regs, false);
	}

	if (xfer->interval != 0) {
		LOG_ERR("Periodic transfer is not supported");
	}

	const uint16_t pkt_cnt = calc_packet_count(sizeof(struct usb_setup_packet), pipe->ep_char.mps);
	int next_pid = CTRL_STAGE_SETUP;
	uint16_t size = sizeof(struct usb_setup_packet);

	dwc2_ll_hctsiz_prep_transfer(pipe->chan_obj->regs, next_pid, pkt_cnt, size);
	dwc2_ll_hctsiz_do_ping(pipe->chan_obj->regs, false);
	/* TODO: Configure split transaction if needed */
	dwc2_ll_hcint_read_and_clear_intrs(pipe->chan_obj->regs);
	dwc2_ll_hcdma_set_buffer_addr(pipe->chan_obj->regs, xfer->setup_pkt);
	/* TODO: sync CACHE */
	dwc2_ll_hcchar_en_channel(pipe->chan_obj->regs, true);
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
	} else if (core_event != UHC_DWC2_CORE_EVENT_NONE) {  // Port event
		uhc_port_event_t port_event = uhc_dwc2_decode_hprt(dev, core_event);
		if (port_event != UHC_PORT_EVENT_NONE) {
			priv->dynamic.last_event = port_event;
			priv->dynamic.flags.event_pending = 1;
			k_event_post(&priv->drv_evt, BIT(UHC_DWC2_EVENT_PORT));
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

	/* Check the post-debounce state of the bus (i.e., whether it's actually connected/disconnected) */
	bool is_connected = dwc2_ll_hprt_get_conn_status(dwc2);
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
					LOG_ERR("Port connection event occurred, but port is not connected after debounce");
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

	/* TODO: enter critical section */

	/* TODO: implement port checks */

	/*
	* Hint:
	* Port can only a reset when it is in the enabled or disabled (in the case of a new connection) states.
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
	dwc2_ll_hprt_set_port_reset(dwc2, true);
	/* TODO: exit critical section */
	k_msleep(RESET_HOLD_MS);
	/* TODO: enter critical section */
	if (priv->dynamic.port_state != UHC_PORT_STATE_RESETTING) {
		/* The port state has unexpectedly changed */
		LOG_ERR("Port state changed during reset");
		ret = -EIO;
		goto bailout;
	}

	/* Return the bus to the idle state. Port enabled event should occur */
	dwc2_ll_hprt_set_port_reset(dwc2, false);
	/* TODO: exit critical section */
	k_msleep(RESET_RECOVERY_MS);
	/* TODO: enter critical section */
	if (priv->dynamic.port_state != UHC_PORT_STATE_RESETTING || !priv->dynamic.flags.conn_dev_ena) {
		/* The port state has unexpectedly changed */
		LOG_ERR("Port state changed during reset");
		ret = -EIO;
		goto bailout;
	}

	dwc2_hal_set_fifo_config(dwc2, &priv->fifo);
	dwc2_ll_set_frame_list(dwc2, NULL /* priv->frame_list , FRAME_LIST_LEN */);
	dwc2_ll_periodic_enable(dwc2);
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
	/*
	* Hint:
	* struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	* priv->dynamic.state == UHC_PORT_STATE_RECOVERY;
	* priv->num_channels_idle == 0;
	* priv->num_channels_queued == 0
	* priv->dynamic.flags.val == 0
	*/

	/* TODO: enter critical section */
	ret = uhc_dwc2_quirk_irq_disable_func(dev);
	if (ret) {
		LOG_ERR("Quirk IRQ disable failed %d", ret);
		return ret;
	}

	/* Perform soft reset on the core */
	ret = uhc_dwc2_reset_core_soft(dev);
	if (ret) {
		LOG_ERR("Failed to reset root port: %d", ret);
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
	switch (type) {
	case USB_TRANSFER_TYPE_CTRL:
		dw2_ll_xfer_type = UHC_DWC2_XFER_TYPE_CTRL;
		break;
	default:
		LOG_ERR("Unsupported transfer type %d", type);
		return;
	}

	ep_char->type = dw2_ll_xfer_type;

	if (is_ctrl_pipe) {
		ep_char->bEndpointAddress = 0;
		/* Set the default pipe's MPS to the worst case MPS for the device's speed */
		ep_char->mps = (pipe_config->dev_speed == UHC_DWC2_SPEED_LOW) ? CTRL_EP_MAX_MPS_LS : CTRL_EP_MAX_MPS_HSFS;
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
static inline bool uhc_dwc2_chan_alloc(const struct device *dev, uhc_dwc2_channel_t *chan_obj, void *context)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	__ASSERT(priv->channels.hdls,
			"uhc_dwc2_chan_alloc: Channel handles list not allocated");

	/* TODO: FIFO sizes should be set before attempting to allocate a channel */

	if (priv->channels.num_allocated == priv->const_cfg.num_channels) {
		return false;
	}

	int chan_idx = -1;
	for (int i = 0; i < priv->const_cfg.num_channels; i++) {
		if (priv->channels.hdls[i] == NULL) {
			priv->channels.hdls[i] = chan_obj;
			chan_idx = i;
			priv->channels.num_allocated++;
			break;
		}
	}

	__ASSERT(chan_idx != -1,
			"No free channels available, num_allocated=%d, num_channels=%d",
			priv->channels.num_allocated, priv->const_cfg.num_channels);

	/* Initialize channel object */
	LOG_DBG("Allocating channel %d", chan_idx);
	memset(chan_obj, 0, sizeof(uhc_dwc2_channel_t));
	chan_obj->flags.chan_idx = chan_idx;
	chan_obj->regs = dwc2_ll_chan_get_regs(dwc2, chan_idx);
	chan_obj->chan_ctx = context;
	/* Init underlying channel registers */
	dwc2_ll_hcint_read_and_clear_intrs(chan_obj->regs);
	dwc2_ll_haintmsk_en_chan_intr(dwc2, chan_idx);
	dwc2_ll_hcintmsk_set_intr_mask(chan_obj->regs, CHAN_INTRS_EN_MSK);
	dwc2_ll_hctsiz_init(chan_obj->regs);
	return true;
}

static inline void uhc_dwc2_chan_free(const struct device *dev, uhc_dwc2_channel_t *chan_obj)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
	struct usb_dwc2_reg *const dwc2 = config->base;

	__ASSERT(priv->channels.hdls,
			"uhc_dwc2_chan_alloc: Channel handles list not allocated");

	if (chan_obj->type == UHC_DWC2_XFER_TYPE_INTR || chan_obj->type == UHC_DWC2_XFER_TYPE_ISOCHRONOUS) {
		/* TODO: Unschedule this channel */
		LOG_WRN("uhc_dwc2_chan_free: Cannot free interrupt or isochronous channels yet");
	}

	__ASSERT(!chan_obj->flags.active,
			"uhc_dwc2_chan_free: Cannot free channel %d, it is still active",
			chan_obj->flags.chan_idx);

	dwc2_ll_haintmsk_dis_chan_intr(dwc2, 1 << chan_obj->flags.chan_idx);

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
	(void) type;  /* Hint: For Scatter-Gather mode we need create a descriptor list with different sizes, based on the type */
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
 * @note Pipe holds the underlying channel object and the DMA buffer for transfer purposes. Thread context.
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
	uhc_dwc2_speed_t port_speed = UHC_DWC2_SPEED_FULL; /* TODO: Refactor to get port speed, static for now */
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
	dma_buffer_block_free(pipe->buffer);
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

			ret = dwc2_hal_port_get_speed(dev, &speed);
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
					LOG_ERR("Unexpected port state %s", uhc_port_state_str[priv->dynamic.port_state]);
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

	switch (pipe->last_event) {
		case PIPE_EVENT_XFER_DONE: {
			/* XFER transfer is done, process the transfer and release the pipe buffer */
			struct uhc_transfer *const xfer = (struct uhc_transfer *)pipe->buffer->xfer;

			if (xfer->buf != NULL && xfer->buf->len) {
				LOG_HEXDUMP_WRN(xfer->buf->data, xfer->buf->len, "data");
			}

			/* TODO: Refactor the address setting logic */
			if (pipe->buffer->flags.ctrl.set_addr) {
				pipe->buffer->flags.ctrl.set_addr = 0;
				pipe->ep_char.dev_addr = pipe->buffer->flags.ctrl.new_addr;
				dwc2_ll_hcchar_set_dev_addr(pipe->chan_obj->regs, pipe->ep_char.dev_addr);
				k_msleep(SET_ADDR_DELAY_MS);
			}

			/* TODO: Refactor pipe resources release */
			pipe->flags.has_xfer = 0;
			priv->num_pipes_idle++;
			priv->num_pipes_queued--;

			uhc_xfer_return(dev, xfer, 0);
			break;
		}
		case PIPE_EVENT_ERROR:
		case PIPE_EVENT_HALTED:
		case PIPE_EVENT_NONE:
		default: {
			LOG_ERR("Unhandled pipe event %s", uhc_pipe_event_str[pipe->last_event]);
			break;
		}
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

	/* TODO: Check that XFER has not already been enqueued */

	/* TODO: dma addr must be aligned 4 bytes */
	// if (((uintptr_t)xfer->setup_pkt % 4)) {
		// LOG_WRN("xfer->setup_pkt must be 4 bytes aligned");
	// }

	/* TODO: Buffer addr also should be aligned */
	// if(((uintptr_t)net_buf_tail(xfer->buf) % 4))
	// {
		// LOG_WRN("xfer->buf must be 4 bytes aligned");
	// }

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
		/* This is the first XFER to be enqueued into the pipe. Move the pipe to the list of active pipes */
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
	struct uhc_data *data = dev->data;
	uint32_t numdeveps;
	uint32_t ineps;

	k_mutex_init(&data->mutex);

	/* Initialize the private data structure */
	memset(priv, 0, sizeof(struct uhc_dwc2_data_s));
	priv->ctrl_pipe_hdl = NULL;
	k_mutex_init(&priv->mutex);
	k_event_init(&priv->drv_evt);
	atomic_clear(&priv->xfer_new);
	atomic_clear(&priv->xfer_finished);
	priv->iso_enabled = 0;

	(void)uhc_dwc2_quirk_caps(dev);

	/* TODO: Overwrite the DWC2 register values with the devicetree values? */

	config->make_thread(dev);

	/*
	 * At this point, we cannot or do not want to access the hardware
	 * registers to get GHWCFGn values. For now, we will use devicetree to
	 * get GHWCFGn values and use them to determine the number and type of
	 * configured endpoints in the hardware. This can be considered a
	 * workaround, and we may change the upper layer internals to avoid it
	 * in the future.
	 */
	ineps = usb_dwc2_get_ghwcfg4_ineps(config->ghwcfg4) + 1U;
	numdeveps = usb_dwc2_get_ghwcfg2_numdeveps(config->ghwcfg2) + 1U;
	LOG_DBG("Number of endpoints (NUMDEVEPS + 1) %u", numdeveps);
	LOG_DBG("Number of IN endpoints (INEPS + 1) %u", ineps);

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

	/* Allocate memory for the channel objects */
	priv->channels.hdls = k_malloc(priv->const_cfg.num_channels * sizeof(uhc_dwc2_channel_t*));
	if (priv->channels.hdls == NULL) {
		LOG_ERR("Failed to allocate channel handles");
		return -ENOMEM;
	}

	for (uint8_t i = 0; i < priv->const_cfg.num_channels; i++) {
		priv->channels.hdls[i] = NULL;
	}

	return dwc2_init_pinctrl(dev);
}

static int uhc_dwc2_enable(const struct device *dev)
{
	const struct uhc_dwc2_config *const config = dev->config;
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	int ret;

	ret = uhc_dwc2_quirk_pre_enable(dev);
	if (ret) {
		LOG_ERR("Quirk pre enable failed %d", ret);
		return ret;
	}

	ret = uhc_dwc2_init_controller(dev);
	if (ret) {
		return ret;
	}

	ret = uhc_dwc2_quirk_pre_enable(dev);
	if (ret) {
		LOG_ERR("Quirk pre enable failed %d", ret);
		return ret;
	}

	/* Enable global interrupt */
	sys_set_bits((mem_addr_t)&base->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);
	/* Disable soft disconnect */
	sys_clear_bits((mem_addr_t)&base->dctl, USB_DWC2_DCTL_SFTDISCON);
	LOG_DBG("Enable device %p", base);

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

	ret = uhc_dwc2_quirk_post_enable(dev);
	if (ret) {
		LOG_ERR("Quirk post enable failed %d", ret);
		return ret;
	}

	return 0;
}

static int uhc_dwc2_disable(const struct device *dev)
{
	LOG_WRN("uhc_dwc2_disable");
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	mem_addr_t dctl_reg = (mem_addr_t)&base->dctl;
	int err;

	LOG_DBG("Disable device %p", dev);

	uhc_dwc2_quirk_irq_disable_func(dev);

	sys_clear_bits((mem_addr_t)&base->gahbcfg, USB_DWC2_GAHBCFG_GLBINTRMASK);

	/* Enable soft disconnect */
	sys_set_bits(dctl_reg, USB_DWC2_DCTL_SFTDISCON);

	/* OUT endpoint 0 cannot be disabled by software. The buffer allocated
	 * in dwc2_ctrl_feed_dout() can only be freed after core reset if the
	 * core was in Buffer DMA mode.
	 *
	 * Soft Reset does timeout if PHY clock is not running. However, just
	 * triggering Soft Reset seems to be enough on shutdown clean up.
	 */
	dwc2_core_soft_reset(dev);

	err = uhc_dwc2_quirk_disable(dev);
	if (err) {
		LOG_ERR("Quirk disable failed %d", err);
		return err;
	}

	return 0;
}

static int uhc_dwc2_shutdown(const struct device *dev)
{
	int ret;

	LOG_WRN("uhc_dwc2_shutdown");

	ret = uhc_dwc2_quirk_shutdown(dev);
	if (ret) {
		LOG_ERR("Quirk shutdown failed %d", ret);
		return ret;
	}

	/* TODO: Release memory for channel handles */
	/* Hint : k_free(priv->channels.hdls); */

	return 0;
}

/* =============================================================================================== */
/* ======================== Device Definition and Initialization ================================= */
/* =============================================================================================== */

#define UHC_DWC2_PINCTRL_DT_INST_DEV_CONFIG_GET(n)				\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),			\
		    ((void *)PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL))

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
	.make_thread = uhc_dwc2_make_thread,                            /* Function to create the thread */
	.quirks = UHC_DWC2_VENDOR_QUIRK_GET(0),                         /* Vendors' quirks */
	.num_out_eps = DT_INST_PROP(0, num_out_eps),
	.num_in_eps = DT_INST_PROP(0, num_in_eps),
	.base = (struct usb_dwc2_reg *)UHC_DWC2_DT_INST_REG_ADDR(0),
	.pcfg = UHC_DWC2_PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.ghwcfg1 = DT_INST_PROP(0, ghwcfg1),
	.ghwcfg2 = DT_INST_PROP(0, ghwcfg2),
	.ghwcfg4 = DT_INST_PROP(0, ghwcfg4),
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
