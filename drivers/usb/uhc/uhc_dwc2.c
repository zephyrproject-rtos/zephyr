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
#define SET_ADDR_DELAY_MS       10
#define CTRL_EP_MAX_MPS_LS      8U
#define CTRL_EP_MAX_MPS_HSFS    64U

#define BENDPOINTADDRESS_NUM_MSK     0x0F   // Endpoint number mask of the bEndpointAddress field of an endpoint descriptor
#define BENDPOINTADDRESS_DIR_MSK     0x80   // Endpoint direction mask of the bEndpointAddress field of an endpoint descriptor

#define USB_DWC2_REG_GSNPSID    0x4F54400A      //  Release number of USB DWC2 used in SoCs

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


typedef struct uhc_dwc2_status_s *uhc_dwc2_status_t;

typedef enum {
    PIPE_EVENT_NONE = 0,
    PIPE_EVENT_URB_DONE,
    PIPE_EVENT_ERROR,
    PIPE_EVENT_HALTED,
} pipe_event_t;

const char *uhc_pipe_event_str[] = {
    "None",
    "URB Done",
    "Error",
    "Halted",
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
    /* Port context? */
    // void *context;
    /* Port callback? */
    // void *callback;
    // void *callback_arg;
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
    //Channel control, status, and information
    union {
        struct {
            uint32_t active: 1;             /**< Debugging bit to indicate whether channel is enabled */
            uint32_t halt_requested: 1;     /**< A halt has been requested */
            uint32_t reserved: 2;
            uint32_t chan_idx: 4;           /**< The index number of the channel */
            uint32_t reserved24: 24;
        };
        uint32_t val;
    } flags;                                /**< Flags regarding channel's status and information */
    usb_dwc2_host_chan_regs_t *regs;         /**< Pointer to the channel's register set */
    // usb_dwc_hal_chan_error_t error;         /**< The last error that occurred on the channel */
    uhc_dwc2_xfer_type_t type;               /**< The transfer type of the channel */
    void *chan_ctx;                         /**< Context variable for the owner of the channel */
} uhc_dwc2_channel_t;

enum {
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
    // hcd_pipe_callback_t callback;           /**< HCD pipe event ISR callback */
    // void *callback_arg;                     /**< User argument for HCD pipe callback */
    // void *context;                          /**< Context variable used to associate the pipe with upper layer object */
    // const usb_ep_desc_t *ep_desc;           /**< Pointer to endpoint descriptor of the pipe */
    uhc_dwc2_speed_t dev_speed;             /**< Speed of the device */
    uint8_t dev_addr;                       /**< Device address of the pipe */
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
    // void *xfer_desc_list;                   // Only for Scatter-Gather transfers
    // int xfer_desc_list_len_bytes;           // Only for cache msync
    struct uhc_transfer *xfer;            // Pointer to the transfer object associated with this buffer
    union {
        struct {
            uint32_t data_stg_in: 1;        // Data stage of the control transfer is IN
            uint32_t data_stg_skip: 1;      // Control transfer has no data stage
            uint32_t cur_stg: 2;            // Index of the current stage (e.g., 0 is setup stage, 2 is status stage)
            uint32_t set_addr: 1;           // Set address stage is in progress
            uint32_t new_addr: 7;           // New address to set in the status stage
            uint32_t reserved20: 20;
        } ctrl;                             // Control transfer related
        struct {
            uint32_t zero_len_packet: 1;    // Added a zero length packet, so transfer consists of 2 QTDs
            uint32_t reserved31: 31;
        } bulk;                             // Bulk transfer related
        struct {
            uint32_t num_qtds: 8;           // Number of transfer descriptors filled (excluding zero length packet)
            uint32_t zero_len_packet: 1;    // Added a zero length packet, so true number descriptors is num_qtds + 1
            uint32_t reserved23: 23;
        } intr;                             // Interrupt transfer related
        struct {
            uint32_t num_qtds: 8;           // Number of transfer descriptors filled (including NULL descriptors)
            uint32_t interval: 8;           // Interval (in number of SOF i.e., ms)
            uint32_t start_idx: 8;          // Index of the first transfer descriptor in the list
            uint32_t next_start_idx: 8;     // Index for the first descriptor of the next buffer
        } isoc;
        uint32_t val;
    } flags;
    union {
        struct {
            uint32_t executing: 1;          // The buffer is currently executing
            uint32_t was_canceled: 1;      // Buffer was done due to a cancellation (i.e., a halt request)
            uint32_t reserved6: 6;
            uint32_t stop_idx: 8;           // The descriptor index when the channel was halted
            pipe_event_t pipe_event: 8; // The pipe event when the buffer was done
            uint32_t reserved8: 8;
        };
        uint32_t val;
    } status_flags;                         // Status flags for the buffer
} dma_buffer_t;

struct pipe_obj_s {
    // URB queuing related
    // TAILQ_HEAD(tailhead_urb_pending, urb_s) pending_urb_tailq;
    // TAILQ_HEAD(tailhead_urb_done, urb_s) done_urb_tailq;
    int num_urb_pending;
    int num_urb_done;

    // Single-buffer control
    dma_buffer_t *buffer;                         // Pointer to the buffer of the pipe

    // Multi-buffer control
    // dma_buffer_block_t *buffers[NUM_BUFFERS];  // Double buffering scheme
    // union {
    //     struct {
    //         uint32_t buffer_num_to_fill: 2; // Number of buffers that can be filled
    //         uint32_t buffer_num_to_exec: 2; // Number of buffers that are filled and need to be executed
    //         uint32_t buffer_num_to_parse: 2;// Number of buffers completed execution and waiting to be parsed
    //         uint32_t reserved2: 2;
    //         uint32_t wr_idx: 1;             // Index of the next buffer to fill. Bit width must allow NUM_BUFFERS to wrap automatically
    //         uint32_t rd_idx: 1;             // Index of the current buffer in-flight. Bit width must allow NUM_BUFFERS to wrap automatically
    //         uint32_t fr_idx: 1;             // Index of the next buffer to parse. Bit width must allow NUM_BUFFERS to wrap automatically
    //         uint32_t buffer_is_executing: 1;// One of the buffers is in flight
    //         uint32_t reserved20: 20;
    //     };
    //     uint32_t val;
    // } multi_buffer_control;

    // HAL related
    uhc_dwc2_channel_t *chan_obj;
    uhc_dwc2_ep_char_t ep_char;
    // Port related
    // port_t *port;                           // The port to which this pipe is routed through
    // TAILQ_ENTRY(pipe_obj) tailq_entry;      // TailQ entry for port's list of pipes
    // Pipe status/state/events related
    pipe_state_t state;
    pipe_event_t last_event;
    // volatile TaskHandle_t task_waiting_pipe_notif;  // Task handle used for internal pipe events. Set by waiter, cleared by notifier
    union {
        struct {
            uint32_t waiting_halt: 1;
            uint32_t pipe_cmd_processing: 1;
            uint32_t has_urb: 1;            // Indicates there is at least one URB either pending, in-flight, or done
            uint32_t event_pending: 1;      // Indicates that a pipe event is pending and needs to be processed
            uint32_t reserved28: 28;
        };
        uint32_t val;
    } flags;
    // Pipe callback and context
    // hcd_pipe_callback_t callback;
    // void *callback_arg;
    // void *context;
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
    // struct k_spinlock lock;
	struct k_thread thread_data;

	/* Main events the driver thread waits for */
	struct k_event drv_evt;

    /* FRAME LIST? */
    void *frame_list;
    /* Pipes/channels LIST? */
    void *idle_channels;
    void *active_channels;
    /* Port status, state, and events? */
    uhc_dwc2_status_t status;
    /* FIFO */
    uhc_dwc2_fifo_config_t fifo;
    /* Mutex for port access? */
    struct k_mutex mutex;

    uhc_dwc2_constant_config_t const_cfg; /* Data, that doesn't changed after initialization */

    struct {
        size_t num_allocated; /**< Number of channels currently allocated */
        uint32_t pending_intrs_msk;    /**< Bit mask of channels with pending interrupts */
        uhc_dwc2_channel_t **hdls;              /**< Handles of each channel. Set to NULL if channel has not been allocated */
    } channels; /* Data, that is used in single thread */

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

    /* Temporal - refactoring required */
    // TODO: alloc pipe object
    pipe_t pipe;    // Currently support only one static pipe per port (1ch for CTRL EP)
    pipe_hdl_t ctrl_pipe_hdl; // Handle to the control pipe

    uint8_t num_pipes_idle; // Number of idle pipes
    uint8_t num_pipes_queued; // Number of pipes queued for processing

    /* Rest */
	// struct dwc2_reg_backup backup;
	// uint32_t ghwcfg1;
	// uint32_t max_xfersize;
	// uint32_t max_pktcnt;
	/* Defect workarounds */
	// unsigned int wa_essregrestored : 1;
	/* Runtime state flags */
	// unsigned int hibernated : 1;
	// unsigned int hfir_set : 1;
	// enum dwc2_suspend_type suspend_type;
};

// =================================================================================================
// ================================ DWC2 FIFO Management ===========================================
// =================================================================================================

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

static int uhc_dwc2_config_fixed_dma_fifo(const uhc_dwc2_constant_config_t *const_cfg,
                                            uhc_dwc2_fifo_config_t *fifo)
{
    // Fixed allocation for now, Buffer DMA only
    LOG_DBG("Configuring FIFO sizes");
    fifo->top = const_cfg->fifo.depth;
    // Buffer DMA needs 1 words per channel
    fifo->top -= const_cfg->num_channels;

    // - ptx_largest is limited to 256 for FS since most FS core only has 1024 bytes total

    // We need to reserve space for the RX FIFO, NPTX FIFO, and PTX FIFO.

    // TODO: support HS
    uint32_t nptx_largest = EPSIZE_BULK_FS / 4;
    uint32_t ptx_largest = 256 / 4; // Why 256?

    fifo->nptxfsiz = 2 * nptx_largest;
    fifo->rxfsiz = 2 * (ptx_largest + 2) + const_cfg->num_channels;
    fifo->ptxfsiz = fifo->top - (fifo->nptxfsiz + fifo->rxfsiz);

    // TODO: verify ptxfsiz is overflowed

    LOG_DBG("FIFO sizes calculated");
    LOG_DBG("\ttop=%u, nptx=%u, rx=%u, ptx=%u", fifo->top * 4,  fifo->nptxfsiz * 4, fifo->rxfsiz * 4, fifo->ptxfsiz * 4);

    return 0;
}

// =================================================================================================
// =================================== DWC2 HAL Functions ==========================================
// =================================================================================================

static inline void dwc2_ll_set_frame_list(struct usb_dwc2_reg *const dwc2, void *frame_list)
{
    LOG_WRN("Setting frame list not implemented yet");
}

static inline void dwc2_ll_periodic_enable(struct usb_dwc2_reg *const dwc2)
{
    LOG_WRN("Enabling periodic scheduling not implemented yet");
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
    __ASSERT((dwc2->gsnpsid == USB_DWC2_REG_GSNPSID),
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
    cfg->fifo.flags.dedicated = ghwcfg4_reg.dedfifomode;  // TODO: check the logic with spec
    cfg->fifo.flags.dynamic = ghwcfg2_reg.enabledynamicfifo;

    cfg->hsphy_type = ghwcfg2_reg.hsphytype;
    cfg->fsphy_type = ghwcfg2_reg.fsphytype;
    cfg->num_channels = ghwcfg2_reg.numhostch + 1;

    // TODO: Different speed modes are not supported yet

    // TODO: Max packet count and transfer size?
    // priv->max_pktcnt = GHWCFG3_PKTCOUNT(usb_dwc2_get_ghwcfg3_pktsizewidth(ghwcfg3));
    // priv->max_xfersize = GHWCFG3_XFERSIZE(usb_dwc2_get_ghwcfg3_xfersizewidth(ghwcfg3));

    // TODO: Vendor control interface support?
    // LOG_DBG("Vendor Control interface support enabled: %s",
        // (ghwcfg3 & USB_DWC2_GHWCFG3_VNDCTLSUPT) ? "true" : "false");

    // TODO: LPM support?
    // LOG_DBG("LPM mode is %s",
    //     (ghwcfg3 & USB_DWC2_GHWCFG3_LPMMODE) ? "enabled" : "disabled");
    // Synch reset support
    // if (ghwcfg3 & USB_DWC2_GHWCFG3_RSTTYPE) {
    //     priv->syncrst = 1;
    // }

    cfg->dma = dwc2_hal_is_dma_supported(dwc2);
    return 0;
}

static void dwc2_hal_channel_configure(uhc_dwc2_channel_t *chan_obj, uhc_dwc2_ep_char_t *ep_char)
{
    // Cannot change ep_char whilst channel is still active or in error
    __ASSERT(!chan_obj->flags.active && !chan_obj->flags.halt_requested,
             "Cannot change endpoint characteristics while channel is active or halted");

    // Set the endpoint characteristics of the pipe
    dwc2_ll_hcchar_init_channel(chan_obj->regs,
                             ep_char->dev_addr,
                             ep_char->bEndpointAddress & BENDPOINTADDRESS_NUM_MSK,
                             ep_char->mps,
                             ep_char->type,
                             ep_char->bEndpointAddress & BENDPOINTADDRESS_DIR_MSK,
                             ep_char->ls_via_fs_hub);
    // Save channel type
    chan_obj->type = ep_char->type;
    // If this is a periodic endpoint/channel, schedule in the frame list
    if (ep_char->type == UHC_DWC2_XFER_TYPE_ISOCHRONOUS || ep_char->type == UHC_DWC2_XFER_TYPE_INTR) {
        LOG_WRN("ISOC and INTR channels are note supported yet");
    }
}

static inline void dwc2_hal_set_fifo_config(struct usb_dwc2_reg *const dwc2, uhc_dwc2_fifo_config_t *const fifo)
{
    dwc2->gdfifocfg = (fifo->top << USB_DWC2_GDFIFOCFG_EPINFOBASEADDR_POS) | fifo->top;
    // TODO: make via hal call
    // dwc2_ll_gdfifocfg_set_fifo_top(dwc2, priv->fifo.top);

    fifo->top -= fifo->rxfsiz;
    dwc2->grxfsiz = fifo->rxfsiz;
    // TODO: make via hal call
    // dwc2_ll_grxfsiz_set_rx_fifo_size(dwc2, rxfsiz);

    fifo->top -= fifo->nptxfsiz;
    dwc2->gnptxfsiz = (fifo->nptxfsiz << 16) | fifo->top;
    // TODO: make via hal call
    // dwc2_ll_gnptxfsiz_set_nptx_fifo_size(dwc2, nptxfsiz, priv->fifo.top);

    fifo->top -= fifo->ptxfsiz;
    dwc2->hptxfsiz = fifo->ptxfsiz << 16 | fifo->top;
    // TODO: make via hal call
    // dwc2_ll_hptxfsiz_set_ptx_fifo_size(dwc2, ptxfsiz, priv->fifo.top);

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
        return -ENODEV;  // Port is not powered
    }

    // Get the port speed from the HPRT register
    *speed = (uhc_dwc2_speed_t)dwc2_ll_hprt_get_port_speed(dwc2);
    return 0;
}

// =================================================================================================
// ================================== DWC2 Port Management =========================================
// =================================================================================================

/* Host Port Control and Status Register */
#define USB_DWC2_HPRT_PRTENCHNG               BIT(3)
#define USB_DWC2_HPRT_PRTOVRCURRCHNG          BIT(5)
#define USB_DWC2_HPRT_PRTCONNDET              BIT(1)

#define CORE_INTRS_EN_MSK   (USB_DWC2_GINTSTS_DISCONNINT)

//Interrupts that pertain to core events
#define CORE_EVENTS_INTRS_MSK (USB_DWC2_GINTSTS_DISCONNINT | \
                               USB_DWC2_GINTSTS_HCHINT)

//Interrupt that pertain to host port events
#define PORT_EVENTS_INTRS_MSK (USB_DWC2_HPRT_PRTCONNDET | \
                               USB_DWC2_HPRT_PRTENCHNG | \
                               USB_DWC2_HPRT_PRTOVRCURRCHNG)

static void uhc_dwc2_lock_enable(const struct device *dev)
{
    const struct uhc_dwc2_config *const config = dev->config;
    struct usb_dwc2_reg *const dwc2 = config->base;
    struct uhc_dwc2_data_s *const priv = uhc_get_private(dev);
    // Disable the hprt (connection) and disconnection interrupts to prevent repeated triggerings
    dwc2_ll_gintmsk_dis_intrs(dwc2, USB_DWC2_GINTSTS_PRTINT | USB_DWC2_GINTSTS_DISCONNINT);
    // Enable the lock
    priv->dynamic.flags.lock_enabled = 1;
}

static inline void uhc_dwc2_lock_disable(const struct device *dev)
{
    const struct uhc_dwc2_config *const config = dev->config;
    struct usb_dwc2_reg *const dwc2 = config->base;
    struct uhc_dwc2_data_s *const priv = uhc_get_private(dev);
    // Disable the lock
    priv->dynamic.flags.lock_enabled = 1;
    // Clear Connection and disconnection interrupt in case it triggered again
    dwc2_ll_gintsts_clear_intrs(dwc2, USB_DWC2_GINTSTS_DISCONNINT);
    dwc2_ll_hprt_intr_clear(dwc2, USB_DWC2_HPRT_PRTCONNDET);
    // Re-enable the hprt (connection) and disconnection interrupts
    dwc2_ll_gintmsk_en_intrs(dwc2, USB_DWC2_GINTSTS_PRTINT | USB_DWC2_GINTSTS_DISCONNINT);
}

static int uhc_dwc2_power_on(const struct device *dev)
{
    const struct uhc_dwc2_config *const config = dev->config;
    struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
    struct usb_dwc2_reg *const dwc2 = config->base;
    int ret;

    // Port can only be powered on if it's currently unpowered
    if (priv->dynamic.port_state == UHC_PORT_STATE_NOT_POWERED) {
        priv->dynamic.port_state = UHC_PORT_STATE_DISCONNECTED;
        // Configure Host related interrupts
        dwc2_hal_port_init(dwc2);
        dwc2_hal_toggle_power(dwc2, true);
        ret = 0;
    } else {
        ret = -EINVAL;
    }

    return ret;
}

static inline int uhc_dwc2_config_phy(const struct device *dev)
{
    // const struct uhc_dwc2_config *const config = dev->config;
    // struct usb_dwc2_reg *const dwc2 = config->base;
    struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

    // PHY configuration is done in uhc_dwc2_config_controller
    // Init PHY based on the speed
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
#endif //
    } else {
#if (0)
        uint32_t gusbcfg = dwc2->gusbcfg;

        // Select FS PHY
        gusbcfg |= GUSBCFG_PHYSEL;
        dwc2->gusbcfg = gusbcfg;

        // TODO: vendor quirk specific phy init

        // Reset core after selecting PHY
        ret = dwc2_core_soft_reset(dwc2);

        // USB turnaround time is critical for certification where long cables and 5-Hubs are used.
        // So if you need the AHB to run at less than 30 MHz, and if USB turnaround time is not critical,
        // these bits can be programmed to a larger value. Default is 5
        gusbcfg &= ~GUSBCFG_TRDT_Msk;
        gusbcfg |= 5u << GUSBCFG_TRDT_Pos;
        dwc2->gusbcfg = gusbcfg;

        // TODO: vendor specific quirk phy update post reset
#else
        LOG_WRN("FS PHY config not implemented yet");
#endif //
    }
    return 0;
}

static inline int uhc_dwc2_config_controller(const struct device *dev)
{
    const struct uhc_dwc2_config *const config = dev->config;
    struct usb_dwc2_reg *const dwc2 = config->base;
    struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

    dwc2_ll_gahbcfg_dis_global_intrs(dwc2);

    if (priv->const_cfg.dma) {
        dwc2_ll_gahbcfg_en_dma(dwc2);
    }

    // TODO: Set AHB burst mode for some ECO only for ESP32S2
    // hbstlen = 1;

    // TODO: Disable HNP and SRP capabilities
    // usb_dwc_ll_gusbcfg_dis_hnp_cap(hal->dev);
    // usb_dwc_ll_gusbcfg_dis_srp_cap(hal->dev);

    dwc2_ll_gintmsk_dis_intrs(dwc2, 0xFFFFFFFF);
    dwc2_ll_gintmsk_en_intrs(dwc2, CORE_INTRS_EN_MSK);
    dwc2_ll_gintsts_read_and_clear_intrs(dwc2);
    dwc2_ll_gahbcfg_en_global_intrs(dwc2);
    dwc2_ll_gusbcfg_en_host_mode(dwc2);

    while ((sys_read32((mem_addr_t)&dwc2->gintsts) & USB_DWC2_GINTSTS_CURMOD) != 1) {
	}

    // Flush FIFO
    dwc2_ll_grstctl_flush_tx_fifo(dwc2, 0x10); // all TX fifo
    dwc2_ll_grstctl_flush_rx_fifo(dwc2);

	return 0;
}

static int uhc_dwc2_core_soft_reset(const struct device *dev)
{
    const struct uhc_dwc2_config *const config = dev->config;
    // struct uhc_dwc2_data_s *const priv = uhc_get_private(dev);
    struct usb_dwc2_reg *const dwc2 = config->base;
    const unsigned int csr_timeout_us = 10000UL;
    uint32_t cnt = 0UL;

    LOG_DBG("Performing DWC2 core soft reset and config controller");

    dwc2_ll_grstctl_core_soft_reset(dwc2);
    while (dwc2_ll_grstctl_is_core_soft_reset_in_progress(dwc2)) {
        // Wait until core reset is done or timeout occurs
        k_busy_wait(1);
        if (++cnt > csr_timeout_us) {
            LOG_ERR("Wait for core soft reset timeout");
            return -EIO;
        }

    }
    cnt = 0UL;
    while (!dwc2_ll_grstctl_is_ahb_idle(dwc2)) {
        // Wait until AHB Master bus is idle before doing any other operations
        k_busy_wait(1);
        if (++cnt > csr_timeout_us) {
            LOG_ERR("Wait for AHB idle timeout");
            return -EIO;
        }
    }

    // Set the default bits in USB-DWC registers
    int ret = uhc_dwc2_config_controller(dev);
    if (ret) {
        LOG_ERR("Failed to configure DWC2 controller: %d", ret);
        return ret;
    }

    // TODO: Clear all the flags and channels
    // hal->periodic_frame_list = NULL;
    // hal->flags.val = 0;
    // hal->channels.num_allocated = 0;
    // hal->channels.chan_pend_intrs_msk = 0;
    // if (hal->channels.hdls) {
    //     for (int i = 0; i < hal->constant_config.chan_num_total; i++) {
    //         hal->channels.hdls[i] = NULL;
    //     }
    // }
    return 0;
}

static int uhc_dwc2_init_controller(const struct device *dev)
{
    const struct uhc_dwc2_config *const config = dev->config;
    struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
    struct usb_dwc2_reg *const dwc2 = config->base;

    int ret;
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

    ret = uhc_dwc2_config_phy(dev);
    if (ret) {
        LOG_ERR("Failed to configure DWC2 PHY: %d", ret);
        return ret;
    }

    ret = uhc_dwc2_config_fixed_dma_fifo(&priv->const_cfg, &priv->fifo);
    if (ret) {
        LOG_ERR("Failed to configure DWC2 FIFO: %d", ret);
        return ret;
    }

    return uhc_dwc2_core_soft_reset(dev);
}

static inline void dwc2_ll_port_enable(struct usb_dwc2_reg *const dwc2)
{
    dwc2_ll_hcfg_en_buffer_dma(dwc2);
    dwc2_ll_hcfg_dis_perio_sched(dwc2);

    uhc_dwc2_speed_t speed = dwc2_ll_hprt_get_port_speed(dwc2);
    // Configure PHY clock: Only for USB-DWC with FSLS PHY
    // TODO: we are always on FSLS PHY, refactor this
    LOG_WRN("Configuring clocks only for FSLS PHY for now");
    // if (priv->const_cfg.hsphy_type == 0) {
        dwc2_ll_hcfg_set_fsls_phy_clock(dwc2, speed);
        dwc2_ll_hfir_set_frame_interval(dwc2, speed);
    // }
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
            // priv->dynamic.port_state = UHC_PORT_STATE_RECOVERY;
            port_event = UHC_PORT_EVENT_DISCONNECTION;
            priv->dynamic.flags.conn_dev_ena = 0;
            break;
        }
        case UHC_DWC2_CORE_EVENT_ENABLED: {
            // Initialize remaining host port registers
            dwc2_ll_port_enable(dwc2);
            // Retrieve the port speed
            // port->speed = get_usb_port_speed(usb_dwc_hal_port_get_conn_speed(port->hal));
            port_event = UHC_PORT_EVENT_ENABLED;
            priv->dynamic.flags.conn_dev_ena = 1;
            // This was triggered by a command, so no event needs to be propagated.
            break;
        }
        case UHC_DWC2_CORE_EVENT_DISABLED: {
            priv->dynamic.flags.conn_dev_ena = 0;
            // Disabled could be due to a disable request or reset request, or due to a port error
            if (priv->dynamic.port_state != UHC_PORT_STATE_RESETTING) {  // Ignore the disable event if it's due to a reset request
                if (priv->dynamic.flags.waiting_disable) {
                    // Disabled by request (i.e. by port command). Generate an internal event
                    priv->dynamic.port_state = UHC_PORT_STATE_DISABLED;
                    priv->dynamic.flags.waiting_disable = 0;
                    // TODO: Notify the port event from ISR
                    LOG_ERR("Port disabled by request, not implemented yet");
                } else {
                    // Disabled due to a port error
                    LOG_ERR("Port disabled due to an error, changing state to recovery");
                    priv->dynamic.port_state = UHC_PORT_STATE_RECOVERY;
                    port_event = UHC_PORT_EVENT_ERROR;
                }
            }
            break;
        }
        case UHC_DWC2_CORE_EVENT_OVRCUR:
        case UHC_DWC2_CORE_EVENT_OVRCUR_CLR: {  // Could occur if a quick overcurrent then clear happens
            // TODO: Handle overcurrent event
            // if port state powered, we need to power it off to protect it
            // change port state to recovery
            // generate port event UHC_PORT_EVENT_OVERCURRENT
            // disable the flag conn_dev_ena
            LOG_ERR("Overcurrent detected on port, not implemented yet");
            break;
        }
        default: {
            // No event occurred or could not decode the interrupt
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
        // There are host port interrupts. Read and clear those as well.
        port_intrs = dwc2_ll_hprt_intr_read_and_clear(dwc2);
    }

    LOG_DBG("GINTSTS=%08Xh, HPRT=%08Xh", core_intrs, port_intrs);

    // Note: ENABLED < DISABLED < CONN < DISCONN < OVRCUR
    // Do not change order of checks. Regressing events (e.g. enable -> disabled,
    // connected -> connected) always take precedence.
    if ((core_intrs & CORE_EVENTS_INTRS_MSK) || (port_intrs & PORT_EVENTS_INTRS_MSK)) {
        //Do not change the order of the following checks. Some events/interrupts take precedence over others
        if (core_intrs & USB_DWC2_GINTSTS_DISCONNINT) {
            core_event = UHC_DWC2_CORE_EVENT_DISCONN;
            uhc_dwc2_lock_enable(dev);
            // Mask the port connection and disconnection interrupts to prevent repeated triggering
        } else if (port_intrs & USB_DWC2_HPRT_PRTOVRCURRCHNG) {
            //Check if this is an overcurrent or an overcurrent cleared
            if (dwc2_ll_hprt_get_port_overcur(dwc2)) {
                core_event = UHC_DWC2_CORE_EVENT_OVRCUR;
            } else {
                core_event = UHC_DWC2_CORE_EVENT_OVRCUR_CLR;
            }
        } else if (port_intrs & USB_DWC2_HPRT_PRTENCHNG) {
            if (dwc2_ll_hprt_get_port_en(dwc2)) {
                // Host port was enabled
                core_event = UHC_DWC2_CORE_EVENT_ENABLED;
            } else {
                // Host port has been disabled
                core_event = UHC_DWC2_CORE_EVENT_DISABLED;
            }
        } else if (port_intrs & USB_DWC2_HPRT_PRTCONNDET /* && !hal->flags.dbnc_lock_enabled */) {
            core_event = UHC_DWC2_CORE_EVENT_CONN;
            uhc_dwc2_lock_enable(dev);
        }
    }
    // Port events always take precedence over channel events
    if (core_event == UHC_DWC2_CORE_EVENT_NONE && (core_intrs & USB_DWC2_GINTSTS_HCHINT)) {
        // One or more channels have pending interrupts. Store the mask of those channels
        priv->channels.pending_intrs_msk = dwc2_ll_haint_get_chan_intrs(dwc2);
        core_event = UHC_DWC2_CORE_EVENT_CHAN;
    }

    return core_event;
}

uhc_dwc2_channel_t *uhc_dwc2_get_chan_pending_intr(const struct device *dev)
{
    // const struct uhc_dwc2_config *const config = dev->config;
    struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

    if (priv->channels.hdls == NULL) {
        LOG_WRN("uhc_dwc2_get_chan_pending_intr: No channels allocated");
        return NULL; // No channels allocated
    }

    int chan_num = __builtin_ffs(priv->channels.pending_intrs_msk);
    if (chan_num) {
        // Clear the pending bit for that channel
        priv->channels.pending_intrs_msk &= ~(1 << (chan_num - 1));
        return priv->channels.hdls[chan_num - 1];
    } else {
        return NULL;
    }
}

static inline void uhc_dwc2_pipe_callback(pipe_t *pipe, pipe_event_t event, bool in_isr)
{
    // This function is called when a pipe event occurs
    // It should be implemented to handle the specific events for the pipe
    // For now, we just log that it is not implemented
    LOG_ERR("uhc_dwc2_pipe_callback is not implemented yet");
}

static inline void *uhc_dwc2_chan_get_context(uhc_dwc2_channel_t *chan_obj)
{
    // Assuming the context is stored in the pipe_t structure
    if (chan_obj == NULL) {
        LOG_ERR("uhc_dwc2_chan_get_context: Channel object is NULL");
        return NULL;
    }
    return chan_obj->chan_ctx; // Return the context associated with the channel
}


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

dwc2_hal_chan_event_t uhc_dwc2_hal_chan_decode_intr(uhc_dwc2_channel_t *chan_obj)
{
    uint32_t chan_intrs = dwc2_ll_hcint_read_and_clear_intrs(chan_obj->regs);
    dwc2_hal_chan_event_t chan_event;
    //Note: We don't assert on (chan_obj->flags.active) here as it could have been already cleared by usb_dwc_hal_chan_request_halt()

    /*
    Note: Do not change order of checks as some events take precedence over others.
    Errors > Channel Halt Request > Transfer completed
    */
    if (chan_intrs & CHAN_INTRS_ERROR_MSK) {    // Note: Errors are uncommon, so we check against the entire interrupt mask to reduce frequency of entering this call path
        // HAL_ASSERT(chan_intrs & USB_DWC_LL_INTR_CHAN_CHHLTD);  //An error should have halted the channel
        LOG_ERR("Channel %d error: 0x%08x", chan_obj->flags.chan_idx, chan_intrs);
        // TODO: Update flags
        // TODO: Store the error in hal context
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
        A transfer complete interrupt WITHOUT the channel halting only occurs when receiving a short interrupt IN packet
        and the underlying QTD does not have the HOC bit set. This signifies the last packet of the Interrupt transfer
        as all interrupt packets must MPS sized except the last.
        */
        // The channel isn't halted yet, so we need to halt it manually to stop the execution of the next packet
        dwc2_ll_hcchar_dis_channel(chan_obj->regs);
        /*
        After setting the halt bit, this will generate another channel halted interrupt. We treat this interrupt as
        a NONE event, then cycle back with the channel halted interrupt to handle the CPLT event.
        */
        chan_event = DWC2_CHAN_EVENT_NONE;
    } else {
        __ASSERT(false,
                "uhc_dwc2_hal_chan_decode_intr: Unknown channel interrupt: 0x%08x",
                chan_intrs);
        chan_event = DWC2_CHAN_EVENT_NONE; // Calm compiler warnings when asserts are disabled
    }
    return chan_event;
}

static inline bool _buffer_check_done(pipe_t *pipe)
{
    dma_buffer_t *buffer = pipe->buffer;
    // Only control transfers need to be continued
    if (pipe->ep_char.type != UHC_DWC2_XFER_TYPE_CTRL) {
        return true;
    }

    return (buffer->flags.ctrl.cur_stg == 2);
}

static inline void _buffer_exec(pipe_t *pipe)
{
    dma_buffer_t *buffer = pipe->buffer;
    struct uhc_transfer *const xfer = pipe->buffer->xfer;

    bool next_dir_is_in;
    int next_pid;
    // TODO: Calculate packet count
    uint16_t pkt_cnt = 1; // For now, we assume only one packet per transfer. This should be adjusted based on the transfer size and endpoint characteristics
    uint16_t size;
    uint8_t *dma_addr = NULL;

    // TODO: CTRL stage should not be 2, it should be 0 or 1
    // assert(buffer->flags.ctrl.cur_stg != 2);

    if (buffer->flags.ctrl.cur_stg == 0) { // Just finished control stage
        if (buffer->flags.ctrl.data_stg_skip) {
            // No data stage. Go straight to status stage
            next_dir_is_in = true;             // With no data stage, status stage must be IN
            next_pid = CTRL_STAGE_DATA1;       // Status stage always has a PID of DATA1
            buffer->flags.ctrl.cur_stg = 2;    // Skip over the null descriptor representing the skipped data stage
            size = 0;
        } else {
            // Go to data stage
            next_dir_is_in = buffer->flags.ctrl.data_stg_in;
            next_pid = CTRL_STAGE_DATA1;        // Data stage always starts with a PID of DATA1
            buffer->flags.ctrl.cur_stg = 1;
            size = xfer->buf->size;
        }
    } else {        // cur_stg == 1. // Just finished data stage. Go to status stage
        next_dir_is_in = !buffer->flags.ctrl.data_stg_in;  // Status stage is always the opposite direction of data stage
        next_pid = CTRL_STAGE_DATA1;   // Status stage always has a PID of DATA1
        buffer->flags.ctrl.cur_stg = 2;
        size = 0;
    }


    // TODO:
    // Check if the buffer is large enough for the next transfer
    // Check that the buffer is DMA and CACHE aligned and compatible with the DMA controller (better to do this on enqueue)
    if (xfer->buf != NULL) {
        dma_addr = net_buf_tail(xfer->buf);             // Get the tail of the buffer to append data
        net_buf_add(xfer->buf, size);                   // Ensure the buffer has enough space for the next transfer
    }

    dwc2_ll_hcchar_set_dir(pipe->chan_obj->regs, next_dir_is_in);
    dwc2_ll_hctsiz_prep_transfer(pipe->chan_obj->regs, next_pid, pkt_cnt, size);
    dwc2_ll_hctsiz_do_ping(pipe->chan_obj->regs, false);
    dwc2_ll_hcdma_set_buffer_addr(pipe->chan_obj->regs, dma_addr);
    dwc2_ll_hcchar_en_channel(pipe->chan_obj->regs);
}

static pipe_event_t uhc_dwc2_decode_chan(pipe_t* pipe, uhc_dwc2_channel_t *chan_obj)
{
    dwc2_hal_chan_event_t chan_event = uhc_dwc2_hal_chan_decode_intr(chan_obj);
    pipe_event_t pipe_event = PIPE_EVENT_NONE;

    LOG_DBG("Channel event: %s", dwc2_chan_event_str[chan_event]);

    switch (chan_event) {
    case DWC2_CHAN_EVENT_CPLT: {
        if (!_buffer_check_done(pipe)) {
            _buffer_exec(pipe);
            break;
        }
        pipe->last_event = PIPE_EVENT_URB_DONE;
        pipe_event = pipe->last_event;
        break;
    }
    case DWC2_CHAN_EVENT_ERROR: {
        // Get and store the pipe error event
        LOG_ERR("Channel error handling not implemented yet");
        // TODO:
        // get channel error
        // halt the pipe
        break;
    }
    case DWC2_CHAN_EVENT_HALT_REQ: {
        LOG_ERR("Channel halt request handling not implemented yet");
        // TODO:
        // assert(pipe->cs_flags.waiting_halt);

        // TODO: We've halted a transfer, so we need to trigger the pipe callback
        // pipe->last_event = PIPE_EVENT_URB_DONE;
        // pipe_event = pipe->last_event;
        // Halt request event is triggered when packet is successful completed. But just treat all halted transfers as errors
        pipe->state = UHC_PIPE_STATE_HALTED;
        // Notify the task waiting for the pipe halt or halt it right away
        // _internal_pipe_event_notify(pipe, true);
        break;
    }
    case DWC2_CHAN_EVENT_NONE: {
        break;  // Nothing to do
    }
    default:
    __ASSERT(false,
            "uhc_dwc2_decode_chan: Unknown channel event: %s",
            dwc2_chan_event_str[chan_event]);
        break;
    }
    return pipe_event;
}

static void uhc_dwc2_isr_handler(const struct device *dev)
{
    struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

    // TODO: enter critical section
    uhc_dwc2_core_event_t core_event = uhc_dwc2_decode_intr(dev);
    if (core_event == UHC_DWC2_CORE_EVENT_CHAN) {
        // Channel event. Cycle through each pending channel
        uhc_dwc2_channel_t *chan_obj = uhc_dwc2_get_chan_pending_intr(dev);
        while (chan_obj != NULL) {
            pipe_t *pipe = (pipe_t*) uhc_dwc2_chan_get_context(chan_obj);
            pipe_event_t pipe_event = uhc_dwc2_decode_chan(pipe, chan_obj);
            if (pipe_event != PIPE_EVENT_NONE) {
                pipe->last_event = pipe_event;
                pipe->flags.event_pending = 1;
                k_event_post(&priv->drv_evt, BIT(UHC_DWC2_EVENT_PIPE));
            }
            // Check for more channels with pending interrupts. Returns NULL if there are no more
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
    // TODO: exit critical section

    (void)uhc_dwc2_quirk_irq_clear(dev);
}

// TODO: critical section
static inline bool uhc_dwc2_port_debounce(const struct device *dev)
{
    const struct uhc_dwc2_config *const config = dev->config;
    struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
    struct usb_dwc2_reg *const dwc2 = config->base;

    // TODO: exit critical section
    k_msleep(DEBOUNCE_DELAY_MS);  // Wait for the debounce delay
    // TODO: enter critical section
    // Check the post-debounce state of the bus (i.e., whether it's actually connected/disconnected)
    bool is_connected = dwc2_ll_hprt_get_conn_status(dwc2);
    if (is_connected) {
        priv->dynamic.port_state = UHC_PORT_STATE_DISABLED;
    } else {
        priv->dynamic.port_state = UHC_PORT_STATE_DISCONNECTED;
    }
    // Disable debounce lock
    uhc_dwc2_lock_disable(dev);
    return is_connected;  // Return whether the port is connected or not
}

static inline uhc_port_event_t uhc_dwc2_get_port_event(const struct device *dev)
{
    // const struct uhc_dwc2_config *const config = dev->config;
    struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
    // struct usb_dwc2_reg *const dwc2 = config->base;

    uhc_port_event_t ret = UHC_PORT_EVENT_NONE;
    // TODO: enter critial section
    if (priv->dynamic.flags.event_pending) {
        priv->dynamic.flags.event_pending = 0;
        ret = priv->dynamic.last_event;
        switch (ret) {
            case UHC_PORT_EVENT_CONNECTION: {
                // Don't update state immediately, we still need to debounce.
                if (uhc_dwc2_port_debounce(dev)) {
                    ret = UHC_PORT_EVENT_CONNECTION;
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
    // TODO: exit critical section
    return ret;
}

static inline void uhc_dwc2_flush_pipes(const struct device *dev)
{
    LOG_WRN("Flushing pipes on reset is not implemented yet");
    // struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
    // TODO: For each pipe, reinitialize the channel with EP characteristics
    // pipe_t *pipe = &priv->pipe;
    // dwc2_ll_chan_apply_ep_char(pipe->chan_obj, &pipe->ep_char);
    // TODO: Sync CACHE

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

    // TODO: enter critical section

    // TODO: implement port checks
    // Port can only a reset when it is in the enabled or disabled (in the case of a new connection) states.
    // priv->dynamic.port_state == UHC_PORT_STATE_ENABLED;
    // priv->dynamic.port_state == UHC_PORT_STATE_DISABLED;
    // priv->channels.num_pipes_queued == 0

    /*
    Proceed to resetting the bus:
    - Update the port's state variable
    - Hold the bus in the reset state for RESET_HOLD_MS.
    - Return the bus to the idle state for RESET_RECOVERY_MS
    During this reset the port state should be set to RESETTING and do not change.
    */
    priv->dynamic.port_state = UHC_PORT_STATE_RESETTING;
    dwc2_ll_hprt_set_port_reset(dwc2, true);
    // TODO: exit critical section
    k_msleep(RESET_HOLD_MS);
    // TODO: enter critical section
    if (priv->dynamic.port_state != UHC_PORT_STATE_RESETTING) {
        // The port state has unexpectedly changed
        LOG_ERR("Port state changed during reset");
        ret = -EIO;
        goto bailout;
    }

    // Return the bus to the idle state. Port enabled event should occur
    dwc2_ll_hprt_set_port_reset(dwc2, false);
    // TODO: exit critical section
    k_msleep(RESET_RECOVERY_MS);
    // TODO: enter critical section
    if (priv->dynamic.port_state != UHC_PORT_STATE_RESETTING || !priv->dynamic.flags.conn_dev_ena) {
        // The port state has unexpectedly changed
        LOG_ERR("Port state changed during reset");
        ret = -EIO;
        goto bailout;
    }

    dwc2_hal_set_fifo_config(dwc2, &priv->fifo);
    dwc2_ll_set_frame_list(dwc2, priv->frame_list /* , FRAME_LIST_LEN */);
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
    struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
    int ret;

    // TODO: Implement port checks
    // priv->dynamic.state == UHC_PORT_STATE_RECOVERY;  // Port should be awaiting recovery
    // priv->num_channels_idle == 0;                    // No idle channels
    // priv->num_channels_queued == 0                   // No queued channels
    // priv->dynamic.flags.val == 0                     // Port should have no pending flags

    // TODO: enter critical section
    ret = uhc_dwc2_quirk_irq_disable_func(dev);
    if (ret) {
        LOG_ERR("Quirk IRQ disable failed %d", ret);
        return ret;
    }
    // Perform soft reset on the core
    ret = uhc_dwc2_core_soft_reset(dev);
    if (ret) {
        LOG_ERR("Failed to reset root port: %d", ret);
        return ret;
    }

    // Update the port state and flags
    priv->dynamic.port_state = UHC_PORT_STATE_NOT_POWERED;
    priv->dynamic.last_event = UHC_PORT_EVENT_NONE;
    priv->dynamic.flags.val = 0;

    ret = uhc_dwc2_quirk_irq_enable_func(dev);
    if (ret) {
        LOG_ERR("Quirk IRQ enable failed %d", ret);
        return ret;
    }
    // TODO: exit critical section

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

    LOG_DBG("New device, speed %s", uhc_dwc2_speed_str[speed]);

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
    LOG_WRN("Device gone");
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
    // Initialize EP characteristics
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
        // Set the default pipe's MPS to the worst case MPS for the device's speed
        ep_char->mps = (pipe_config->dev_speed == UHC_DWC2_SPEED_LOW) ? CTRL_EP_MAX_MPS_LS : CTRL_EP_MAX_MPS_HSFS;
    } else {
        // TODO: Implement for non-control pipes
        LOG_WRN("Setting up pipe characteristics for non-control pipe has not implemented yet");
        return;
    }

    ep_char->dev_addr = pipe_config->dev_addr;
    // TODO: Valid only with external hub support
    ep_char->ls_via_fs_hub = 0; // first ctrl pipe is always the default control pipe, not connected via a hub
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

    // TODO: FIFO sizes should be set before attempting to allocate a channel
    // __ASSERT(priv->fifo.configured,
            // "uhc_dwc2_chan_alloc: FIFO sizes not configured");

    if (priv->channels.num_allocated == priv->const_cfg.num_channels) {
        // Out of free channels
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

    // We should have a free channel index here as the number of allocated channels is the same as
    // the number of free channels in hardware
    __ASSERT(chan_idx != -1,
            "No free channels available, num_allocated=%d, num_channels=%d",
            priv->channels.num_allocated, priv->const_cfg.num_channels);

    // Initialize channel object
    LOG_DBG("Allocating channel %d", chan_idx);
    memset(chan_obj, 0, sizeof(uhc_dwc2_channel_t));
    chan_obj->flags.chan_idx = chan_idx;
    chan_obj->regs = dwc2_ll_chan_get_regs(dwc2, chan_idx);
    chan_obj->chan_ctx = context;
    // Init underlying channel registers
    dwc2_ll_hcint_read_and_clear_intrs(chan_obj->regs);
    dwc2_ll_haintmsk_en_chan_intr(dwc2, chan_idx);
    dwc2_ll_hcintmsk_set_intr_mask(chan_obj->regs, CHAN_INTRS_EN_MSK);
    dwc2_ll_hctsiz_init(chan_obj->regs);
    return true;
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
    (void) type;  // For Scatter-Gather mode we need create a descriptor list with different sizes, based on the type
    // Buffer DMA mode needs only one simple buffer for now
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

    // Allocate the pipe resources
    pipe_t *pipe = &priv->pipe;
    uhc_dwc2_channel_t *chan_obj = k_malloc(sizeof(uhc_dwc2_channel_t));
    if (pipe == NULL || chan_obj == NULL) {
        LOG_ERR("Failed to allocate pipe or channel object");
        ret = -ENOMEM;
        goto err;
    }

    // Save channel object in the pipe object
    pipe->chan_obj = chan_obj;

    // TODO: Double buffering scheme?

    // Single buffer scheme for now
    // TODO: currently supported only for control transfers
    usb_transfer_type_t type = USB_TRANSFER_TYPE_CTRL;

    pipe->buffer = dma_buffer_block_alloc(type);

    if (pipe->buffer == NULL) {
        LOG_ERR("Failed to allocate pipe buffer");
        ret = -ENOMEM;
        goto err;
    }

    // TODO: Initialize pipe object list
    // - For dequeue purposes
    // - Init pending urb list
    // - Init done urb list

    // Configure the pipe related EP characteristics and save them in the pipe object
    uhc_dwc2_ep_char_t ep_char;

    // TODO: Support other transfer types
    bool is_default = true;
    int pipe_idx = 0;

    // TODO: Refactor to get port speed, static for now
    uhc_dwc2_speed_t port_speed = UHC_DWC2_SPEED_FULL;

    uhc_dwc2_pipe_set_ep_char(pipe_config, type, is_default, pipe_idx, port_speed, &ep_char);
    memcpy(&pipe->ep_char, &ep_char, sizeof(uhc_dwc2_ep_char_t));

    // Set the pipe state and callback
    pipe->state = UHC_PIPE_STATE_ACTIVE;

    // TODO: Do we need a pipe callback? Yes, for external hubs probably
    // pipe->callback = pipe_config->callback;
    // pipe->callback_arg = pipe_config->callback_arg;


    // Allocate DWC2 HAL channel
    // Port should be initialized and be enabled (has a device inserted) before allocating channels
    // TODO: enter critical section
    if (!priv->dynamic.flags.conn_dev_ena) {
        // TODO: exit critical section
        LOG_ERR("Port is not enabled, cannot allocate channel");
        ret = -ENODEV;  // Port is not enabled, cannot allocate channel
        goto err;
    }

    // Allocate memory for the channel objects
    priv->channels.hdls = k_malloc(priv->const_cfg.num_channels * sizeof(uhc_dwc2_channel_t*));
    for (uint8_t i = 0; i < priv->const_cfg.num_channels; i++) {
        priv->channels.hdls[i] = NULL;  // Initialize all channel handles to NULL
    }

    bool chan_allocated = uhc_dwc2_chan_alloc(dev, pipe->chan_obj, (void *) pipe);
    if (!chan_allocated) {
        // TODO: exit critical section
        LOG_ERR("No more free channels available");
        ret = -ENOMEM;  // No more free channels available
        goto err;
    }

    // Configure the channel's EP characteristics
    dwc2_hal_channel_configure(pipe->chan_obj, &pipe->ep_char);
    // TODO: sync CACHE

    // TODO: Add the pipe to the list of idle pipes in the port object
    // Just increment the idle pipe counter for now
    priv->num_pipes_idle++;
    // TODO: exit critical section

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
    (void) dev;
    (void) pipe_hdl;

    LOG_WRN("Pipe freeing is not implemented yet");

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
            // Nothing to do
            break;
        case UHC_PORT_EVENT_CONNECTION: {
            // New device connected, reset the port
            uhc_dwc2_port_reset(dev);
            break;
        }

        case UHC_PORT_EVENT_ENABLED: {
            // TODO: enter critical section
            priv->dynamic.port_state = UHC_PORT_STATE_ENABLED;
            // TODO: exit critical section

            uhc_dwc2_speed_t speed;

            ret = dwc2_hal_port_get_speed(dev, &speed);
            if (ret) {
                LOG_ERR("Failed to get port speed");
                break;
            }

            pipe_hdl_t ctrl_pipe_hdl;
            uhc_pipe_config_t pipe_config = {
                .dev_speed = speed,
                .dev_addr = 0,
            };
            // Allocate the Pipe for the EP0 Control Endpoint
            ret = uhc_dwc2_pipe_alloc(dev, &pipe_config, &ctrl_pipe_hdl);
            if (ret) {
                LOG_ERR("Failed to initialize channels: %d", ret);
                break;
            }
            // Save the control pipe handle in the port object
            priv->ctrl_pipe_hdl = ctrl_pipe_hdl;
            // Notify the USB Host that a new device has been connected
            uhc_dwc2_submit_new_device(dev, speed);
            break;
        }
        case UHC_PORT_EVENT_DISCONNECTION:
        case UHC_PORT_EVENT_ERROR:
        case UHC_PORT_EVENT_OVERCURRENT: {
            bool port_has_device = false;

            // TODO: enter critical section
            switch (priv->dynamic.port_state) {
                case UHC_PORT_STATE_DISABLED: // This occurred after the device has already been disabled
                    // Therefore, there's no device object to clean up, and we can go straight to port recovery
                    // TODO: Recover port right now or request port recovery later?
                    uhc_dwc2_port_recovery(dev);
                    break;
                case UHC_PORT_STATE_NOT_POWERED: // The user turned off ports' power. Indicate to USBH that the device is gone
                case UHC_PORT_STATE_ENABLED: // There is an enabled (active) device. Indicate to USBH that the device is gone
                    port_has_device = true;
                    break;
                default:
                    LOG_ERR("Unexpected port state %s", uhc_port_state_str[priv->dynamic.port_state]);
                    break;
            }
            // TODO: exit critical section

            if (port_has_device) {
                uhc_dwc2_submit_dev_gone(dev);
                uhc_dwc2_port_recovery(dev);
            }
            break;
        }
        default:
            break;  // No action for other events
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

    // TODO: support more than CTRL pipe
    pipe_t* pipe = &priv->pipe;

    LOG_DBG("Pipe event: %s", uhc_pipe_event_str[pipe->last_event]);

    switch (pipe->last_event) {
        case PIPE_EVENT_URB_DONE: {
            // URB transfer is done, process the transfer and release the pipe
            struct uhc_transfer *const xfer = (struct uhc_transfer *)pipe->buffer->xfer;

            if (xfer->buf != NULL && xfer->buf->len) {
                LOG_HEXDUMP_WRN(xfer->buf->data, xfer->buf->len, "data");
            }

            // TODO: Refactor the address setting logic
            if (pipe->buffer->flags.ctrl.set_addr) {
                pipe->buffer->flags.ctrl.set_addr = 0;
                // Save dev address in the pipe characteristics
                pipe->ep_char.dev_addr = pipe->buffer->flags.ctrl.new_addr;
                // Update the underlying channel's register
                dwc2_ll_hcchar_set_dev_addr(pipe->chan_obj->regs, pipe->ep_char.dev_addr);
                // Wait for device to accept the new address with delay
                k_msleep(SET_ADDR_DELAY_MS);
            }

            // TODO: Refactor pipe release
            pipe->num_urb_pending--; // Decrease the number of pending URBs
            pipe->flags.has_urb = 0; // Clear the URB flag
            priv->num_pipes_idle++; // Return back the pipe to the idle list
            priv->num_pipes_queued--; // Decrease the number of queued pipes

            // Notify the upper layer that the transfer is done
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
    // const struct uhc_dwc2_config *const config = dev->config;
    struct uhc_dwc2_data_s *const priv = uhc_get_private(dev);
    // struct usb_dwc2_reg *const dwc2 = config->base;

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

#if (0)
static inline bool _buffer_can_fill(pipe_t *pipe)
{
    // We can only fill if there are pending URBs and at least one unfilled buffer
    return (pipe->num_urb_pending > 0);

    // if (pipe->num_urb_pending > 0 && pipe->multi_buffer_control.buffer_num_to_fill > 0) {
    //     return true;
    // } else {
    //     return false;
    // }
}

static inline bool _buffer_can_exec(pipe_t *pipe)
{
    return true;
    // We can only execute if there is not already a buffer executing and if there are filled buffers awaiting execution
    // if (!pipe->multi_buffer_control.buffer_is_executing && pipe->multi_buffer_control.buffer_num_to_exec > 0) {
    //     return true;
    // } else {
    //     return false;
    // }
}

static inline void _buffer_fill_ctrl(/* dma_buffer_block_t *buffer, usb_transfer_t *transfer*/)
{
    // Get information about the control transfer by analyzing the setup packet (the first 8 bytes of the URB's data)
    // usb_setup_packet_t *setup_pkt = (usb_setup_packet_t *)transfer->data_buffer;
    // bool data_stg_in = (setup_pkt->bmRequestType & USB_BM_REQUEST_TYPE_DIR_IN);
    // bool data_stg_skip = (setup_pkt->wLength == 0);
    // Fill setup stage
    // usb_dwc_hal_xfer_desc_fill(buffer->xfer_desc_list, 0, transfer->data_buffer, sizeof(usb_setup_packet_t),
                            //    USB_DWC_HAL_XFER_DESC_FLAG_SETUP | USB_DWC_HAL_XFER_DESC_FLAG_HOC);
    // Fill data stage
    // if (data_stg_skip) {
        // Not data stage. Fill with an empty descriptor
        // usb_dwc_hal_xfer_desc_clear(buffer->xfer_desc_list, 1);
    // } else {
        // Fill data stage. Note that we still fill with transfer->num_bytes instead of setup_pkt->wLength as it's possible to require more bytes than wLength
        // usb_dwc_hal_xfer_desc_fill(buffer->xfer_desc_list, 1, transfer->data_buffer + sizeof(usb_setup_packet_t), transfer->num_bytes - sizeof(usb_setup_packet_t),
                                //    ((data_stg_in) ? USB_DWC_HAL_XFER_DESC_FLAG_IN : 0) | USB_DWC_HAL_XFER_DESC_FLAG_HOC);
    // }
    // Fill status stage (i.e., a zero length packet). If data stage is skipped, the status stage is always IN.
    // usb_dwc_hal_xfer_desc_fill(buffer->xfer_desc_list, 2, NULL, 0,
                            //    ((data_stg_in && !data_stg_skip) ? 0 : USB_DWC_HAL_XFER_DESC_FLAG_IN) | USB_DWC_HAL_XFER_DESC_FLAG_HOC);
    // Update buffer flags
    // buffer->flags.ctrl.data_stg_in = data_stg_in;
    // buffer->flags.ctrl.data_stg_skip = data_stg_skip;
    // buffer->flags.ctrl.cur_stg = 0;
}

// TODO: IRAM_ATTR
static void _buffer_fill(pipe_t *pipe)
{
    // Get an URB from the pending tailq
    // urb_t *urb = TAILQ_FIRST(&pipe->pending_urb_tailq);
    // assert(pipe->num_urb_pending > 0 && urb != NULL);
    // TAILQ_REMOVE(&pipe->pending_urb_tailq, urb, tailq_entry);
    pipe->num_urb_pending--;

    // Select the inactive buffer
    // assert(pipe->multi_buffer_control.buffer_num_to_exec <= NUM_BUFFERS);
    // dma_buffer_block_t *buffer_to_fill = pipe->buffers[pipe->multi_buffer_control.wr_idx];
    // buffer_to_fill->status_flags.val = 0;   // Clear the buffer's status flags
    // assert(buffer_to_fill->urb == NULL);

    // bool is_in = pipe->ep_char.bEndpointAddress & BENDPOINTADDRESS_DIR_MSK;
    // int mps = pipe->ep_char.mps;
    // usb_transfer_t *transfer = &urb->transfer;
    switch (pipe->ep_char.type) {
    case UHC_DWC2_XFER_TYPE_CTRL: {
            LOG_WRN("Filling control buffer");
            // _buffer_fill_ctrl(buffer_to_fill, transfer);
            break;
        }
    default: {
            // Unsupported transfer type
            LOG_ERR("Unsupported transfer type %d", pipe->ep_char.type);
            break;
        }
    }
    // TODO: sync CACHE
}

// TODO: IRAM_ATTR
static void _buffer_exec(pipe_t *pipe)
{
    // assert(pipe->multi_buffer_control.rd_idx != pipe->multi_buffer_control.wr_idx || pipe->multi_buffer_control.buffer_num_to_exec > 0);
    // dma_buffer_block_t *buffer_to_exec = pipe->buffers[pipe->multi_buffer_control.rd_idx];
    // assert(buffer_to_exec->urb != NULL);

    // uint32_t start_idx;
    // int desc_list_len;
    switch (pipe->ep_char.type) {
        case UHC_DWC2_XFER_TYPE_CTRL: {
            // start_idx = 0;
            // desc_list_len = XFER_LIST_LEN_CTRL;

            // Set the channel's direction to OUT and PID to 0 respectively for the the setup stage
            LOG_WRN("Executing control buffer");

            // dwc2_ll_hcchar_set_dir(pipe->chan_obj->regs, false);  // Setup stage is always OUT
            // dwc2_ll_hcchar_set_pid(pipe->chan_obj->regs, 0);  // Setup stage always has a PID of DATA0
            break;
        }
        default: {
            LOG_WRN("Buffer execution for non CTRL transfer type %d not implemented", pipe->ep_char.type);
            return;
        }
    }
    // Update buffer and multi buffer flags
    // buffer_to_exec->status_flags.executing = 1;
    // pipe->multi_buffer_control.buffer_is_executing = 1;
    // usb_dwc_hal_chan_activate(pipe->chan_obj, buffer_to_exec->xfer_desc_list, desc_list_len, start_idx);
}
#endif //

static inline uint16_t calc_packet_count(const uint16_t size, const uint8_t mps)
{
    if (size == 0) {
        return 1; // in Buffer DMA mode Zero Length Packet still counts as 1 packet
    } else {
        return DIV_ROUND_UP(size, mps);
    }
}

static inline int uhc_dwc2_submit_ctrl_xfer(const struct device *dev,
                pipe_hdl_t pipe_hdl, struct uhc_transfer *const xfer)
{
    struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
    const struct usb_setup_packet *setup_pkt = (const struct usb_setup_packet *)xfer->setup_pkt;

    LOG_DBG("Control xfer, buf=%p, size=%d", xfer->buf->data, xfer->buf->size);

    LOG_HEXDUMP_WRN(xfer->setup_pkt, 8, "setup");

    LOG_DBG("endpoint=%02Xh, mps=%d, interval=%d, start_frame=%d",
        xfer->ep, xfer->mps, xfer->interval, xfer->start_frame);

    LOG_DBG("stage=%d, no_status=%d",
            xfer->stage, xfer->no_status);

    // TODO: Check that URB has not already been enqueued
    pipe_t *pipe = (pipe_t *)pipe_hdl;

    // TODO: move to _buffer_fill(pipe)
    pipe->buffer->xfer = xfer;              // Save the xfer context in the buffer pipe
    pipe->buffer->flags.ctrl.cur_stg = 0;
    pipe->buffer->flags.ctrl.data_stg_in = usb_reqtype_is_to_host(setup_pkt);
    pipe->buffer->flags.ctrl.data_stg_skip = (setup_pkt->wLength == 0);
    pipe->buffer->flags.ctrl.set_addr = 0;

    if (setup_pkt->bRequest == USB_SREQ_SET_ADDRESS) {
        pipe->buffer->flags.ctrl.set_addr = 1; // Set address request
        pipe->buffer->flags.ctrl.new_addr = setup_pkt->wValue & 0x7F; // New address is in wValue, but only lower 7 bits are valid
        LOG_DBG("Set address request, new address %d", pipe->buffer->flags.ctrl.new_addr);
    }

    LOG_DBG("data_in: %s, data_skip: %s",
            pipe->buffer->flags.ctrl.data_stg_in ? "true" : "false",
            pipe->buffer->flags.ctrl.data_stg_skip ? "true" : "false");

    // TODO: Check if the ISOC pipe can handle all packets:

    // TODO Sync data from cache to memory. For OUT and CTRL transfers

    // TODO: enter critical section
    // TODO: Check that pipe and port are in the correct state to receive URBs
    // priv->dynamic.port_state == UHC_PORT_STATE_ENABLED
    // pipe->state == UHC_PIPE_STATE_ACTIVE
    // pipe->flags.pipe_cmd_processing != 1;

    // Save the context on xfer buffer
    // xfer_buf->context = (void *)pipe_hdl;

    // TODO: Add the URB to the pipe's pending tailq
    // TAILQ_INSERT_TAIL(&pipe->pending_urb_tailq, urb, tailq_entry);
    pipe->num_urb_pending++;

    // use the URB's reserved_flags to store the URB's current state

#if (1)
    // xfer start
    const uint8_t ep_num = xfer->ep & ~0x80;
    const uint8_t ep_dir = ep_num & 0x80 ? 1 : 0;

    LOG_DBG("ep_num=%d, ep_dir=%d, mps=%d", ep_num, ep_dir, pipe->ep_char.mps);

    if (ep_num == 0) {
        // update direction in channel register since control endpoint can switch direction
        dwc2_ll_hcchar_set_dir(pipe->chan_obj->regs, ep_dir);
    }

    const bool is_period = (xfer->interval != 0);

    if (is_period) {
        LOG_WRN("Periodic transfer is not supported");
        return -ENOTSUP; // Periodic transfers are not supported yet
    }

    // TODO: ? do we need to disable the channel before reconfiguring it?
    // dwc2_ll_hcchar_disable_chan(pipe->chan_obj->regs);

    const uint16_t pkt_count = calc_packet_count(sizeof(struct usb_setup_packet), pipe->ep_char.mps);
    LOG_DBG("xfer: pkt_count=%d, size=%d",
                        pkt_count,
                        xfer->buf->size);

    dwc2_ll_hctsiz_prep_transfer(pipe->chan_obj->regs, CTRL_STAGE_SETUP, pkt_count, sizeof(struct usb_setup_packet));
    dwc2_ll_hctsiz_do_ping(pipe->chan_obj->regs, false);

    // TODO: Configure split transaction if needed
    // channel->hcsplt = edpt->hcsplt;

    dwc2_ll_hcint_read_and_clear_intrs(pipe->chan_obj->regs);
    dwc2_ll_hcdma_set_buffer_addr(pipe->chan_obj->regs, xfer->setup_pkt);

    if (ep_dir == 1) {
        // IN transfer
        LOG_WRN("IN transfer, not implemented yet");
    } else {
        // TODO: sync CACHE
        dwc2_ll_hcchar_en_channel(pipe->chan_obj->regs);
    }
#else
    // if (_buffer_can_fill(pipe)) {
    //     _buffer_fill(pipe);
    // }
    // if (_buffer_can_exec(pipe)) {
    //     _buffer_exec(pipe);
    // }
#endif //

    if (!pipe->flags.has_urb) {
        // This is the first URB to be enqueued into the pipe. Move the pipe to the list of active pipes
        // TODO: remove pipe from idle pipes list
        // TODO: add pipe to active pipes list
        priv->num_pipes_idle--;
        priv->num_pipes_queued++;
        pipe->flags.has_urb = 1;
    }
    // TODO: exit critical section

    return 0;
}

// =================================================================================================
// ================================== UHC DWC2 Driver API ==========================================
// =================================================================================================

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
    LOG_WRN("uhc_dwc2_sof_enable");
    return 0;
}

static int uhc_dwc2_bus_suspend(const struct device *dev)
{
    LOG_WRN("uhc_dwc2_bus_suspend");
    return 0;
}

static int uhc_dwc2_bus_reset(const struct device *dev)
{
    // First reset is done by the uhc dwc2 driver, so we don't need to do anything here
    uhc_submit_event(dev, UHC_EVT_RESETED, 0);
    return 0;
}

static int uhc_dwc2_bus_resume(const struct device *dev)
{
    LOG_WRN("uhc_dwc2_bus_resume");
    return 0;
}

static int uhc_dwc2_enqueue(const struct device *dev,
                struct uhc_transfer *const xfer)
{
    struct uhc_dwc2_data_s *priv = uhc_get_private(dev);

    uhc_xfer_append(dev, xfer);

    int ret;
    if (USB_EP_GET_IDX(xfer->ep) == 0) {
        // Control endpoint
        ret = uhc_dwc2_submit_ctrl_xfer(dev, priv->ctrl_pipe_hdl, xfer);
        if (ret) {
            LOG_ERR("Failed to submit xfer: %d", ret);
            return ret;
        }
    } else {
        // Non-control endpoint
        LOG_ERR("Non-control endpoint enqueue not implemented yet");
        return -ENOSYS;  // Not implemented
    }

    return 0;
}

static int uhc_dwc2_dequeue(const struct device *dev,
                struct uhc_transfer *const xfer)
{
    int ret = -ENOSYS;  // Not implemented
    LOG_WRN("uhc_dwc2_dequeue");
    return ret;
}

static int uhc_dwc2_preinit(const struct device *dev)
{
    LOG_WRN("uhc_dwc2_preinit");

    const struct uhc_dwc2_config *const config = dev->config;
    struct uhc_dwc2_data_s *priv = uhc_get_private(dev);
    // struct usb_dwc2_reg *const dwc2 = config->base;

    // Initialize the private data structure
    memset(priv, 0, sizeof(struct uhc_dwc2_data_s));
    // Initialize the mutex
    k_mutex_init(&priv->mutex);
    // Initialize the event queue and atomic flags
    k_event_init(&priv->drv_evt);

    // TODO: Overwrite the DWC2 register values with the devicetree values?

    // Run thread for processing events
	config->make_thread(dev);

    return 0;
}

static int uhc_dwc2_init(const struct device *dev)
{
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
    LOG_WRN("uhc_dwc2_disable");
    return 0;
}

static int uhc_dwc2_shutdown(const struct device *dev)
{
    LOG_WRN("uhc_dwc2_shutdown");
    return 0;
}

// =================================================================================================
// ======================== Device Definition and Initialization ===================================
// =================================================================================================

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
