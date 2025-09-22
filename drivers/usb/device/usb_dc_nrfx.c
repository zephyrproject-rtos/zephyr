/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 * Copyright (c) 2018 Sundar Subramaniyan <sundar.subramaniyan@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  usb_dc_nrfx.c
 * @brief Nordic USB device controller driver
 *
 * The driver implements the interface between the USBD peripheral
 * driver from nrfx package and the operating system.
 */

#include <soc.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/usb_dc.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/dt-bindings/regulator/nrf5x.h>
#include <nrf_usbd_common.h>
#include <hal/nrf_usbd.h>
#include <nrfx_power.h>


#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(usb_nrfx);

/* USB device controller access from devicetree */
#define DT_DRV_COMPAT nordic_nrf_usbd

/**
 * @brief nRF USBD peripheral states
 */
enum usbd_periph_state {
	USBD_DETACHED,
	USBD_ATTACHED,
	USBD_POWERED,
	USBD_SUSPENDED,
	USBD_RESUMED,
	USBD_DEFAULT,
	USBD_ADDRESS_SET,
	USBD_CONFIGURED,
};

/**
 * @brief Endpoint event types.
 */
enum usbd_ep_event_type {
	EP_EVT_SETUP_RECV,
	EP_EVT_RECV_REQ,
	EP_EVT_RECV_COMPLETE,
	EP_EVT_WRITE_COMPLETE,
};

/**
 * @brief USBD peripheral event types.
 */
enum usbd_event_type {
	USBD_EVT_POWER,
	USBD_EVT_EP,
	USBD_EVT_RESET,
	USBD_EVT_SOF,
	USBD_EVT_REINIT
};

/**
 * @brief Endpoint configuration.
 *
 * @param cb      Endpoint callback.
 * @param max_sz  Max packet size supported by endpoint.
 * @param en      Enable/Disable flag.
 * @param addr    Endpoint address.
 * @param type    Endpoint transfer type.
 */
struct nrf_usbd_ep_cfg {
	usb_dc_ep_callback cb;
	uint32_t max_sz;
	bool en;
	uint8_t addr;
	enum usb_dc_ep_transfer_type type;

};

struct usbd_mem_block {
	void *data;
};

/**
 * @brief Endpoint buffer
 *
 * @param len    Remaining length to be read/written.
 * @param block  Mempool block, for freeing up buffer after use.
 * @param data	 Pointer to the data buffer	for the endpoint.
 * @param curr	 Pointer to the current offset in the endpoint buffer.
 */
struct nrf_usbd_ep_buf {
	uint32_t len;
	struct usbd_mem_block block;
	uint8_t *data;
	uint8_t *curr;
};

/**
 * @brief Endpoint context
 *
 * @param cfg			Endpoint configuration
 * @param buf			Endpoint buffer
 * @param read_complete		A flag indicating that DMA read operation
 *				has been completed.
 * @param read_pending		A flag indicating that the Host has requested
 *				a data transfer.
 * @param write_in_progress	A flag indicating that write operation has
 *				been scheduled.
 * @param trans_zlp		Flag required for Control IN Endpoint. It
 *				indicates that ZLP is required to end data
 *				stage of the control request.
 */
struct nrf_usbd_ep_ctx {
	struct nrf_usbd_ep_cfg cfg;
	struct nrf_usbd_ep_buf buf;
	volatile bool read_complete;
	volatile bool read_pending;
	volatile bool write_in_progress;
	bool trans_zlp;
};

/**
 * @brief Endpoint event structure
 *
 * @param ep		Endpoint control block pointer
 * @param evt_type	Event type
 */
struct usbd_ep_event {
	struct nrf_usbd_ep_ctx *ep;
	enum usbd_ep_event_type evt_type;
};

/**
 * @brief Power event structure
 *
 * @param state		New USBD peripheral state.
 */
struct usbd_pwr_event {
	enum usbd_periph_state state;
};

/**
 * @brief Endpoint USB event
 *	  Used by ISR to send events to work handler
 *
 * @param node		Used by the kernel for FIFO management
 * @param block		Mempool block pointer for freeing up after use
 * @param evt		Event data field
 * @param evt_type	Type of event that has occurred from the USBD peripheral
 */
struct usbd_event {
	sys_snode_t node;
	struct usbd_mem_block block;
	union {
		struct usbd_ep_event ep_evt;
		struct usbd_pwr_event pwr_evt;
	} evt;
	enum usbd_event_type evt_type;
};

/**
 * @brief Fifo element slab
 *	Used for allocating fifo elements to pass from ISR to work handler
 * TODO: The number of FIFO elements is an arbitrary number now but it should
 * be derived from the theoretical number of backlog events possible depending
 * on the number of endpoints configured.
 */
#define FIFO_ELEM_SZ            sizeof(struct usbd_event)
#define FIFO_ELEM_ALIGN         sizeof(unsigned int)

K_MEM_SLAB_DEFINE(fifo_elem_slab, FIFO_ELEM_SZ,
		  CONFIG_USB_NRFX_EVT_QUEUE_SIZE, FIFO_ELEM_ALIGN);


/** Number of IN Endpoints configured (including control) */
#define CFG_EPIN_CNT (DT_INST_PROP(0, num_in_endpoints) +	\
		      DT_INST_PROP(0, num_bidir_endpoints))

/** Number of OUT Endpoints configured (including control) */
#define CFG_EPOUT_CNT (DT_INST_PROP(0, num_out_endpoints) +	\
		       DT_INST_PROP(0, num_bidir_endpoints))

/** Number of ISO IN Endpoints */
#define CFG_EP_ISOIN_CNT DT_INST_PROP(0, num_isoin_endpoints)

/** Number of ISO OUT Endpoints */
#define CFG_EP_ISOOUT_CNT DT_INST_PROP(0, num_isoout_endpoints)

/** ISO endpoint index */
#define EP_ISOIN_INDEX CFG_EPIN_CNT
#define EP_ISOOUT_INDEX (CFG_EPIN_CNT + CFG_EP_ISOIN_CNT + CFG_EPOUT_CNT)

#define EP_BUF_MAX_SZ		64UL
#define ISO_EP_BUF_MAX_SZ	1024UL

/**
 * @brief Output endpoint buffers
 *	Used as buffers for the endpoints' data transfer
 *	Max buffers size possible: 1536 Bytes (8 EP * 64B + 1 ISO * 1024B)
 */
static uint8_t ep_out_bufs[CFG_EPOUT_CNT][EP_BUF_MAX_SZ]
	       __aligned(sizeof(uint32_t));
static uint8_t ep_isoout_bufs[CFG_EP_ISOOUT_CNT][ISO_EP_BUF_MAX_SZ]
	       __aligned(sizeof(uint32_t));

/** Total endpoints configured */
#define CFG_EP_CNT (CFG_EPIN_CNT + CFG_EP_ISOIN_CNT + \
		    CFG_EPOUT_CNT + CFG_EP_ISOOUT_CNT)

/**
 * @brief USBD control structure
 *
 * @param status_cb	Status callback for USB DC notifications
 * @param setup		Setup packet for Control requests
 * @param hfxo_cli	Onoff client used to control HFXO
 * @param hfxo_mgr	Pointer to onoff manager associated with HFXO.
 * @param clk_requested	Flag used to protect against double stop.
 * @param attached	USBD Attached flag
 * @param ready		USBD Ready flag set after pullup
 * @param usb_work	USBD work item
 * @param drv_lock	Mutex for thread-safe nrfx driver use
 * @param ep_ctx	Endpoint contexts
 * @param ctrl_read_len	State of control read operation (EP0).
 */
struct nrf_usbd_ctx {
	usb_dc_status_callback status_cb;
	struct usb_setup_packet setup;
	struct onoff_client hfxo_cli;
	struct onoff_manager *hfxo_mgr;
	atomic_t clk_requested;

	bool attached;
	bool ready;

	struct k_work  usb_work;
	struct k_mutex drv_lock;

	struct nrf_usbd_ep_ctx ep_ctx[CFG_EP_CNT];

	uint16_t ctrl_read_len;
};


/* FIFO used for queuing up events from ISR. */
K_FIFO_DEFINE(usbd_evt_fifo);

/* Work queue used for handling the ISR events (i.e. for notifying the USB
 * device stack, for executing the endpoints callbacks, etc.) out of the ISR
 * context.
 * The system work queue cannot be used for this purpose as it might be used in
 * applications for scheduling USB transfers and this could lead to a deadlock
 * when the USB device stack would not be notified about certain event because
 * of a system work queue item waiting for a USB transfer to be finished.
 */
static struct k_work_q usbd_work_queue;
static K_KERNEL_STACK_DEFINE(usbd_work_queue_stack,
			     CONFIG_USB_NRFX_WORK_QUEUE_STACK_SIZE);


static struct nrf_usbd_ctx usbd_ctx = {
	.attached = false,
	.ready = false,
};

static inline struct nrf_usbd_ctx *get_usbd_ctx(void)
{
	return &usbd_ctx;
}

static inline bool dev_attached(void)
{
	return get_usbd_ctx()->attached;
}

static inline bool dev_ready(void)
{
	return get_usbd_ctx()->ready;
}

static inline nrf_usbd_common_ep_t ep_addr_to_nrfx(uint8_t ep)
{
	return (nrf_usbd_common_ep_t)ep;
}

static inline uint8_t nrfx_addr_to_ep(nrf_usbd_common_ep_t ep)
{
	return (uint8_t)ep;
}

static inline bool ep_is_valid(const uint8_t ep)
{
	uint8_t ep_num = USB_EP_GET_IDX(ep);

	if (NRF_USBD_EPIN_CHECK(ep)) {
		if (unlikely(ep_num == NRF_USBD_EPISO_FIRST)) {
			if (CFG_EP_ISOIN_CNT == 0) {
				return false;
			}
		} else {
			if (ep_num >= CFG_EPIN_CNT) {
				return false;
			}
		}
	} else {
		if (unlikely(ep_num == NRF_USBD_EPISO_FIRST)) {
			if (CFG_EP_ISOOUT_CNT == 0) {
				return false;
			}
		} else {
			if (ep_num >= CFG_EPOUT_CNT) {
				return false;
			}
		}
	}

	return true;
}

static struct nrf_usbd_ep_ctx *endpoint_ctx(const uint8_t ep)
{
	struct nrf_usbd_ctx *ctx;
	uint8_t ep_num;

	if (!ep_is_valid(ep)) {
		return NULL;
	}

	ctx = get_usbd_ctx();
	ep_num = NRF_USBD_EP_NR_GET(ep);

	if (NRF_USBD_EPIN_CHECK(ep)) {
		if (unlikely(NRF_USBD_EPISO_CHECK(ep))) {
			return &ctx->ep_ctx[EP_ISOIN_INDEX];
		} else {
			return &ctx->ep_ctx[ep_num];
		}
	} else {
		if (unlikely(NRF_USBD_EPISO_CHECK(ep))) {
			return &ctx->ep_ctx[EP_ISOOUT_INDEX];
		} else {
			return &ctx->ep_ctx[CFG_EPIN_CNT +
					    CFG_EP_ISOIN_CNT +
					    ep_num];
		}
	}

	return NULL;
}

static struct nrf_usbd_ep_ctx *in_endpoint_ctx(const uint8_t ep)
{
	return endpoint_ctx(NRF_USBD_EPIN(ep));
}

static struct nrf_usbd_ep_ctx *out_endpoint_ctx(const uint8_t ep)
{
	return endpoint_ctx(NRF_USBD_EPOUT(ep));
}

/**
 * @brief Schedule USBD event processing.
 *
 * Should be called after usbd_evt_put().
 */
static inline void usbd_work_schedule(void)
{
	k_work_submit_to_queue(&usbd_work_queue, &get_usbd_ctx()->usb_work);
}

/**
 * @brief Free previously allocated USBD event.
 *
 * Should be called after usbd_evt_get().
 *
 * @param Pointer to the USBD event structure.
 */
static inline void usbd_evt_free(struct usbd_event *ev)
{
	k_mem_slab_free(&fifo_elem_slab, (void *)ev->block.data);
}

/**
 * @brief Enqueue USBD event.
 *
 * @param Pointer to the previously allocated and filled event structure.
 */
static inline void usbd_evt_put(struct usbd_event *ev)
{
	k_fifo_put(&usbd_evt_fifo, ev);
}

/**
 * @brief Get next enqueued USBD event if present.
 */
static inline struct usbd_event *usbd_evt_get(void)
{
	return k_fifo_get(&usbd_evt_fifo, K_NO_WAIT);
}

/**
 * @brief Drop all enqueued events.
 */
static inline void usbd_evt_flush(void)
{
	struct usbd_event *ev;

	do {
		ev = usbd_evt_get();
		if (ev) {
			usbd_evt_free(ev);
		}
	} while (ev != NULL);
}

/**
 * @brief Allocate USBD event.
 *
 * This function should be called prior to usbd_evt_put().
 *
 * @returns Pointer to the allocated event or NULL if there was no space left.
 */
static inline struct usbd_event *usbd_evt_alloc(void)
{
	struct usbd_event *ev;
	struct usbd_mem_block block;

	if (k_mem_slab_alloc(&fifo_elem_slab,
			     (void **)&block.data, K_NO_WAIT)) {
		LOG_ERR("USBD event allocation failed!");

		/*
		 * Allocation may fail if workqueue thread is starved or event
		 * queue size is too small (CONFIG_USB_NRFX_EVT_QUEUE_SIZE).
		 * Wipe all events, free the space and schedule
		 * reinitialization.
		 */
		usbd_evt_flush();

		if (k_mem_slab_alloc(&fifo_elem_slab, (void **)&block.data, K_NO_WAIT)) {
			LOG_ERR("USBD event memory corrupted");
			__ASSERT_NO_MSG(0);
			return NULL;
		}

		ev = (struct usbd_event *)block.data;
		ev->block = block;
		ev->evt_type = USBD_EVT_REINIT;
		usbd_evt_put(ev);
		usbd_work_schedule();

		return NULL;
	}

	ev = (struct usbd_event *)block.data;
	ev->block = block;

	return ev;
}

static void submit_dc_power_event(enum usbd_periph_state state)
{
	struct usbd_event *ev = usbd_evt_alloc();

	if (!ev) {
		return;
	}

	ev->evt_type = USBD_EVT_POWER;
	ev->evt.pwr_evt.state = state;

	usbd_evt_put(ev);

	if (usbd_ctx.attached) {
		usbd_work_schedule();
	}
}

#if CONFIG_USB_NRFX_ATTACHED_EVENT_DELAY
static void attached_evt_delay_handler(struct k_timer *timer)
{
	LOG_DBG("ATTACHED event delay done");
	submit_dc_power_event(USBD_ATTACHED);
}

static K_TIMER_DEFINE(delay_timer, attached_evt_delay_handler, NULL);
#endif

static void usb_dc_power_event_handler(nrfx_power_usb_evt_t event)
{
	enum usbd_periph_state new_state;

	switch (event) {
	case NRFX_POWER_USB_EVT_DETECTED:
#if !CONFIG_USB_NRFX_ATTACHED_EVENT_DELAY
		new_state = USBD_ATTACHED;
		break;
#else
		LOG_DBG("ATTACHED event delayed");
		k_timer_start(&delay_timer,
			      K_MSEC(CONFIG_USB_NRFX_ATTACHED_EVENT_DELAY),
			      K_NO_WAIT);
		return;
#endif
	case NRFX_POWER_USB_EVT_READY:
		new_state = USBD_POWERED;
		break;
	case NRFX_POWER_USB_EVT_REMOVED:
		new_state = USBD_DETACHED;
		break;
	default:
		LOG_ERR("Unknown USB power event %d", event);
		return;
	}

	submit_dc_power_event(new_state);
}

/* Stopping HFXO, algorithm supports case when stop comes before clock is
 * started. In that case, it is stopped from the callback context.
 */
static int hfxo_stop(struct nrf_usbd_ctx *ctx)
{
	if (atomic_cas(&ctx->clk_requested, 1, 0)) {
		return onoff_cancel_or_release(ctx->hfxo_mgr, &ctx->hfxo_cli);
	}

	return 0;
}

static int hfxo_start(struct nrf_usbd_ctx *ctx)
{
	if (atomic_cas(&ctx->clk_requested, 0, 1)) {
		sys_notify_init_spinwait(&ctx->hfxo_cli.notify);

		return onoff_request(ctx->hfxo_mgr, &ctx->hfxo_cli);
	}

	return 0;
}

static void usbd_enable_endpoints(struct nrf_usbd_ctx *ctx)
{
	struct nrf_usbd_ep_ctx *ep_ctx;
	int i;

	for (i = 0; i < CFG_EPIN_CNT; i++) {
		ep_ctx = in_endpoint_ctx(i);
		__ASSERT_NO_MSG(ep_ctx);

		if (ep_ctx->cfg.en) {
			nrf_usbd_common_ep_enable(ep_addr_to_nrfx(ep_ctx->cfg.addr));
		}
	}

	if (CFG_EP_ISOIN_CNT) {
		ep_ctx = in_endpoint_ctx(NRF_USBD_EPIN(8));
		__ASSERT_NO_MSG(ep_ctx);

		if (ep_ctx->cfg.en) {
			nrf_usbd_common_ep_enable(ep_addr_to_nrfx(ep_ctx->cfg.addr));
		}
	}

	for (i = 0; i < CFG_EPOUT_CNT; i++) {
		ep_ctx = out_endpoint_ctx(i);
		__ASSERT_NO_MSG(ep_ctx);

		if (ep_ctx->cfg.en) {
			nrf_usbd_common_ep_enable(ep_addr_to_nrfx(ep_ctx->cfg.addr));
		}
	}

	if (CFG_EP_ISOOUT_CNT) {
		ep_ctx = out_endpoint_ctx(NRF_USBD_EPOUT(8));
		__ASSERT_NO_MSG(ep_ctx);

		if (ep_ctx->cfg.en) {
			nrf_usbd_common_ep_enable(ep_addr_to_nrfx(ep_ctx->cfg.addr));
		}
	}
}

/**
 * @brief Reset endpoint state.
 *
 * Resets the internal logic state for a given endpoint.
 *
 * @param[in]  ep_cts   Endpoint structure control block
 */
static void ep_ctx_reset(struct nrf_usbd_ep_ctx *ep_ctx)
{
	ep_ctx->buf.data = ep_ctx->buf.block.data;
	ep_ctx->buf.curr = ep_ctx->buf.data;
	ep_ctx->buf.len  = 0U;

	/* Abort ongoing write operation. */
	if (ep_ctx->write_in_progress) {
		nrf_usbd_common_ep_abort(ep_addr_to_nrfx(ep_ctx->cfg.addr));
	}

	ep_ctx->read_complete = true;
	ep_ctx->read_pending = false;
	ep_ctx->write_in_progress = false;
	ep_ctx->trans_zlp = false;
}

/**
 * @brief Initialize all endpoint structures.
 *
 * Endpoint buffers are allocated during the first call of this function.
 * This function may also be called again on every USB reset event
 * to reinitialize the state of all endpoints.
 */
static int eps_ctx_init(void)
{
	struct nrf_usbd_ep_ctx *ep_ctx;
	uint32_t i;

	for (i = 0U; i < CFG_EPIN_CNT; i++) {
		ep_ctx = in_endpoint_ctx(i);
		__ASSERT_NO_MSG(ep_ctx);
		ep_ctx_reset(ep_ctx);
	}

	for (i = 0U; i < CFG_EPOUT_CNT; i++) {
		ep_ctx = out_endpoint_ctx(i);
		__ASSERT_NO_MSG(ep_ctx);

		if (!ep_ctx->buf.block.data) {
			ep_ctx->buf.block.data = ep_out_bufs[i];
		}

		ep_ctx_reset(ep_ctx);
	}

	if (CFG_EP_ISOIN_CNT) {
		ep_ctx = in_endpoint_ctx(NRF_USBD_EPIN(8));
		__ASSERT_NO_MSG(ep_ctx);
		ep_ctx_reset(ep_ctx);
	}

	if (CFG_EP_ISOOUT_CNT) {
		BUILD_ASSERT(CFG_EP_ISOOUT_CNT <= 1);

		ep_ctx = out_endpoint_ctx(NRF_USBD_EPOUT(8));
		__ASSERT_NO_MSG(ep_ctx);

		if (!ep_ctx->buf.block.data) {
			ep_ctx->buf.block.data = ep_isoout_bufs[0];
		}

		ep_ctx_reset(ep_ctx);
	}

	return 0;
}

static inline void usbd_work_process_pwr_events(struct usbd_pwr_event *pwr_evt)
{
	struct nrf_usbd_ctx *ctx = get_usbd_ctx();
	int err;

	switch (pwr_evt->state) {
	case USBD_ATTACHED:
		if (!nrf_usbd_common_is_enabled()) {
			LOG_DBG("USB detected");
			nrf_usbd_common_enable();
			err = hfxo_start(ctx);
			__ASSERT_NO_MSG(err >= 0);
		}

		/* No callback here.
		 * Stack will be notified when the peripheral is ready.
		 */
		break;

	case USBD_POWERED:
		usbd_enable_endpoints(ctx);
		nrf_usbd_common_start(IS_ENABLED(CONFIG_USB_DEVICE_SOF));
		ctx->ready = true;

		LOG_DBG("USB Powered");

		if (ctx->status_cb) {
			ctx->status_cb(USB_DC_CONNECTED, NULL);
		}
		break;

	case USBD_DETACHED:
		ctx->ready = false;
		nrf_usbd_common_disable();
		err = hfxo_stop(ctx);
		__ASSERT_NO_MSG(err >= 0);

		LOG_DBG("USB Removed");

		if (ctx->status_cb) {
			ctx->status_cb(USB_DC_DISCONNECTED, NULL);
		}
		break;

	case USBD_SUSPENDED:
		if (dev_ready()) {
			nrf_usbd_common_suspend();
			LOG_DBG("USB Suspend state");

			if (ctx->status_cb) {
				ctx->status_cb(USB_DC_SUSPEND, NULL);
			}
		}
		break;
	case USBD_RESUMED:
		if (ctx->status_cb && dev_ready()) {
			LOG_DBG("USB resume");
			ctx->status_cb(USB_DC_RESUME, NULL);
		}
		break;

	default:
		break;
	}
}

static inline void usbd_work_process_setup(struct nrf_usbd_ep_ctx *ep_ctx)
{
	__ASSERT_NO_MSG(ep_ctx);
	__ASSERT(ep_ctx->cfg.type == USB_DC_EP_CONTROL,
		 "Invalid event on CTRL EP.");

	struct usb_setup_packet *usbd_setup;

	/* SETUP packets are handled by USBD hardware.
	 * For compatibility with the USB stack,
	 * SETUP packet must be reassembled.
	 */
	usbd_setup = (struct usb_setup_packet *)ep_ctx->buf.data;
	memset(usbd_setup, 0, sizeof(struct usb_setup_packet));
	usbd_setup->bmRequestType = nrf_usbd_setup_bmrequesttype_get(NRF_USBD);
	usbd_setup->bRequest = nrf_usbd_setup_brequest_get(NRF_USBD);
	usbd_setup->wValue = nrf_usbd_setup_wvalue_get(NRF_USBD);
	usbd_setup->wIndex = nrf_usbd_setup_windex_get(NRF_USBD);
	usbd_setup->wLength = nrf_usbd_setup_wlength_get(NRF_USBD);
	ep_ctx->buf.len = sizeof(struct usb_setup_packet);

	/* Copy setup packet to driver internal structure */
	memcpy(&usbd_ctx.setup, usbd_setup, sizeof(struct usb_setup_packet));

	LOG_DBG("SETUP: bR:0x%02x bmRT:0x%02x wV:0x%04x wI:0x%04x wL:%d",
		(uint32_t)usbd_setup->bRequest,
		(uint32_t)usbd_setup->bmRequestType,
		(uint32_t)usbd_setup->wValue,
		(uint32_t)usbd_setup->wIndex,
		(uint32_t)usbd_setup->wLength);

	/* Inform the stack. */
	ep_ctx->cfg.cb(ep_ctx->cfg.addr, USB_DC_EP_SETUP);

	struct nrf_usbd_ctx *ctx = get_usbd_ctx();

	if (usb_reqtype_is_to_device(usbd_setup) && usbd_setup->wLength) {
		ctx->ctrl_read_len = usbd_setup->wLength;
		/* Allow data chunk on EP0 OUT */
		nrf_usbd_common_setup_data_clear();
	} else {
		ctx->ctrl_read_len = 0U;
	}
}

static inline void usbd_work_process_recvreq(struct nrf_usbd_ctx *ctx,
					     struct nrf_usbd_ep_ctx *ep_ctx)
{
	if (!ep_ctx->read_pending) {
		return;
	}
	if (!ep_ctx->read_complete) {
		return;
	}

	ep_ctx->read_pending = false;
	ep_ctx->read_complete = false;

	k_mutex_lock(&ctx->drv_lock, K_FOREVER);
	NRF_USBD_COMMON_TRANSFER_OUT(transfer, ep_ctx->buf.data,
				     ep_ctx->cfg.max_sz);
	nrfx_err_t err = nrf_usbd_common_ep_transfer(
		ep_addr_to_nrfx(ep_ctx->cfg.addr), &transfer);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nRF USBD transfer error (OUT): 0x%02x", err);
	}
	k_mutex_unlock(&ctx->drv_lock);
}


static inline void usbd_work_process_ep_events(struct usbd_ep_event *ep_evt)
{
	struct nrf_usbd_ctx *ctx = get_usbd_ctx();
	struct nrf_usbd_ep_ctx *ep_ctx = ep_evt->ep;

	__ASSERT_NO_MSG(ep_ctx);

	switch (ep_evt->evt_type) {
	case EP_EVT_SETUP_RECV:
		usbd_work_process_setup(ep_ctx);
		break;

	case EP_EVT_RECV_REQ:
		usbd_work_process_recvreq(ctx, ep_ctx);
		break;

	case EP_EVT_RECV_COMPLETE:
		ep_ctx->cfg.cb(ep_ctx->cfg.addr,
			       USB_DC_EP_DATA_OUT);
		break;

	case EP_EVT_WRITE_COMPLETE:
		if (ep_ctx->cfg.type == USB_DC_EP_CONTROL &&
		    !ep_ctx->trans_zlp) {
			/* Trigger the hardware to perform
			 * status stage, but only if there is
			 * no ZLP required.
			 */
			k_mutex_lock(&ctx->drv_lock, K_FOREVER);
			nrf_usbd_common_setup_clear();
			k_mutex_unlock(&ctx->drv_lock);
		}
		ep_ctx->cfg.cb(ep_ctx->cfg.addr,
			       USB_DC_EP_DATA_IN);
		break;
	default:
		break;
	}
}

static void usbd_event_transfer_ctrl(nrf_usbd_common_evt_t const *const p_event)
{
	struct nrf_usbd_ep_ctx *ep_ctx =
		endpoint_ctx(p_event->data.eptransfer.ep);

	if (NRF_USBD_EPIN_CHECK(p_event->data.eptransfer.ep)) {
		switch (p_event->data.eptransfer.status) {
		case NRF_USBD_COMMON_EP_OK: {
			struct usbd_event *ev = usbd_evt_alloc();

			if (!ev) {
				return;
			}

			ep_ctx->write_in_progress = false;
			ev->evt_type = USBD_EVT_EP;
			ev->evt.ep_evt.evt_type = EP_EVT_WRITE_COMPLETE;
			ev->evt.ep_evt.ep = ep_ctx;

			LOG_DBG("ctrl write complete");
			usbd_evt_put(ev);
			usbd_work_schedule();
		}
		break;

		case NRF_USBD_COMMON_EP_ABORTED: {
			LOG_DBG("Endpoint 0x%02x write aborted",
				p_event->data.eptransfer.ep);
		}
		break;

		default: {
			LOG_ERR("Unexpected event (nrfx_usbd): %d, ep 0x%02x",
				p_event->data.eptransfer.status,
				p_event->data.eptransfer.ep);
		}
		break;
		}
	} else {
		switch (p_event->data.eptransfer.status) {
		case NRF_USBD_COMMON_EP_WAITING: {
			struct usbd_event *ev = usbd_evt_alloc();

			if (!ev) {
				return;
			}

			LOG_DBG("ctrl read request");

			ep_ctx->read_pending = true;
			ev->evt_type = USBD_EVT_EP;
			ev->evt.ep_evt.evt_type = EP_EVT_RECV_REQ;
			ev->evt.ep_evt.ep = ep_ctx;

			usbd_evt_put(ev);
			usbd_work_schedule();
		}
		break;

		case NRF_USBD_COMMON_EP_OK: {
			struct nrf_usbd_ctx *ctx = get_usbd_ctx();
			struct usbd_event *ev = usbd_evt_alloc();

			if (!ev) {
				return;
			}
			nrf_usbd_common_ep_status_t err_code;

			ev->evt_type = USBD_EVT_EP;
			ev->evt.ep_evt.evt_type = EP_EVT_RECV_COMPLETE;
			ev->evt.ep_evt.ep = ep_ctx;

			err_code = nrf_usbd_common_ep_status_get(
				p_event->data.eptransfer.ep, &ep_ctx->buf.len);

			if (err_code != NRF_USBD_COMMON_EP_OK) {
				LOG_ERR("_ep_status_get failed! Code: %d",
					err_code);
				__ASSERT_NO_MSG(0);
			}
			LOG_DBG("ctrl read done: %d", ep_ctx->buf.len);

			if (ctx->ctrl_read_len > ep_ctx->buf.len) {
				ctx->ctrl_read_len -= ep_ctx->buf.len;
				/* Allow next data chunk on EP0 OUT */
				nrf_usbd_common_setup_data_clear();
			} else {
				ctx->ctrl_read_len = 0U;
			}

			usbd_evt_put(ev);
			usbd_work_schedule();
		}
		break;

		default: {
			LOG_ERR("Unexpected event (nrfx_usbd): %d, ep 0x%02x",
				p_event->data.eptransfer.status,
				p_event->data.eptransfer.ep);
		}
		break;
		}
	}
}

static void usbd_event_transfer_data(nrf_usbd_common_evt_t const *const p_event)
{
	struct nrf_usbd_ep_ctx *ep_ctx =
		endpoint_ctx(p_event->data.eptransfer.ep);

	if (NRF_USBD_EPIN_CHECK(p_event->data.eptransfer.ep)) {
		switch (p_event->data.eptransfer.status) {
		case NRF_USBD_COMMON_EP_OK: {
			struct usbd_event *ev = usbd_evt_alloc();

			if (!ev) {
				return;
			}

			LOG_DBG("write complete, ep 0x%02x",
				(uint32_t)p_event->data.eptransfer.ep);

			ep_ctx->write_in_progress = false;
			ev->evt_type = USBD_EVT_EP;
			ev->evt.ep_evt.evt_type = EP_EVT_WRITE_COMPLETE;
			ev->evt.ep_evt.ep = ep_ctx;
			usbd_evt_put(ev);
			usbd_work_schedule();
		}
		break;

		case NRF_USBD_COMMON_EP_ABORTED: {
			LOG_DBG("Endpoint 0x%02x write aborted",
				p_event->data.eptransfer.ep);
		}
		break;

		default: {
			LOG_ERR("Unexpected event (nrfx_usbd): %d, ep 0x%02x",
				p_event->data.eptransfer.status,
				p_event->data.eptransfer.ep);
		}
		break;
		}

	} else {
		switch (p_event->data.eptransfer.status) {
		case NRF_USBD_COMMON_EP_WAITING: {
			struct usbd_event *ev = usbd_evt_alloc();

			if (!ev) {
				return;
			}

			LOG_DBG("read request, ep 0x%02x",
				(uint32_t)p_event->data.eptransfer.ep);

			ep_ctx->read_pending = true;
			ev->evt_type = USBD_EVT_EP;
			ev->evt.ep_evt.evt_type = EP_EVT_RECV_REQ;
			ev->evt.ep_evt.ep = ep_ctx;

			usbd_evt_put(ev);
			usbd_work_schedule();
		}
		break;

		case NRF_USBD_COMMON_EP_OK: {
			struct usbd_event *ev = usbd_evt_alloc();

			if (!ev) {
				return;
			}

			ep_ctx->buf.len = nrf_usbd_ep_amount_get(NRF_USBD,
				p_event->data.eptransfer.ep);

			LOG_DBG("read complete, ep 0x%02x, len %d",
				(uint32_t)p_event->data.eptransfer.ep,
				ep_ctx->buf.len);

			ev->evt_type = USBD_EVT_EP;
			ev->evt.ep_evt.evt_type = EP_EVT_RECV_COMPLETE;
			ev->evt.ep_evt.ep = ep_ctx;

			usbd_evt_put(ev);
			usbd_work_schedule();
		}
		break;

		default: {
			LOG_ERR("Unexpected event (nrfx_usbd): %d, ep 0x%02x",
				p_event->data.eptransfer.status,
				p_event->data.eptransfer.ep);
		}
		break;
		}
	}
}

/**
 * @brief nRFx USBD driver event handler function.
 */
static void usbd_event_handler(nrf_usbd_common_evt_t const *const p_event)
{
	struct usbd_event evt = {0};
	bool put_evt = false;

	switch (p_event->type) {
	case NRF_USBD_COMMON_EVT_SUSPEND:
		LOG_DBG("SUSPEND state detected");
		evt.evt_type = USBD_EVT_POWER;
		evt.evt.pwr_evt.state = USBD_SUSPENDED;
		put_evt = true;
		break;
	case NRF_USBD_COMMON_EVT_RESUME:
		LOG_DBG("RESUMING from suspend");
		evt.evt_type = USBD_EVT_POWER;
		evt.evt.pwr_evt.state = USBD_RESUMED;
		put_evt = true;
		break;
	case NRF_USBD_COMMON_EVT_WUREQ:
		LOG_DBG("RemoteWU initiated");
		evt.evt_type = USBD_EVT_POWER;
		evt.evt.pwr_evt.state = USBD_RESUMED;
		put_evt = true;
		break;
	case NRF_USBD_COMMON_EVT_RESET:
		evt.evt_type = USBD_EVT_RESET;
		put_evt = true;
		break;
	case NRF_USBD_COMMON_EVT_SOF:
		if (IS_ENABLED(CONFIG_USB_DEVICE_SOF)) {
			evt.evt_type = USBD_EVT_SOF;
			put_evt = true;
		}
		break;

	case NRF_USBD_COMMON_EVT_EPTRANSFER: {
		struct nrf_usbd_ep_ctx *ep_ctx;

		ep_ctx = endpoint_ctx(p_event->data.eptransfer.ep);
		switch (ep_ctx->cfg.type) {
		case USB_DC_EP_CONTROL:
			usbd_event_transfer_ctrl(p_event);
			break;
		case USB_DC_EP_BULK:
		case USB_DC_EP_INTERRUPT:
			usbd_event_transfer_data(p_event);
			break;
		case USB_DC_EP_ISOCHRONOUS:
			usbd_event_transfer_data(p_event);
			break;
		default:
			break;
		}
		break;
	}

	case NRF_USBD_COMMON_EVT_SETUP: {
		nrf_usbd_common_setup_t drv_setup;

		nrf_usbd_common_setup_get(&drv_setup);
		if ((drv_setup.bRequest != USB_SREQ_SET_ADDRESS)
		    || (USB_REQTYPE_GET_TYPE(drv_setup.bmRequestType)
			!= USB_REQTYPE_TYPE_STANDARD)) {
			/* SetAddress is handled by USBD hardware.
			 * No software action required.
			 */

			struct nrf_usbd_ep_ctx *ep_ctx =
				endpoint_ctx(NRF_USBD_EPOUT(0));

			evt.evt_type = USBD_EVT_EP;
			evt.evt.ep_evt.ep = ep_ctx;
			evt.evt.ep_evt.evt_type = EP_EVT_SETUP_RECV;
			put_evt = true;
		}
		break;
	}

	default:
		break;
	}

	if (put_evt) {
		struct usbd_event *ev;

		ev = usbd_evt_alloc();
		if (!ev) {
			return;
		}
		ev->evt_type = evt.evt_type;
		ev->evt = evt.evt;
		usbd_evt_put(ev);
		usbd_work_schedule();
	}
}

static inline void usbd_reinit(void)
{
	int ret;
	nrfx_err_t err;

	nrfx_power_usbevt_disable();
	nrf_usbd_common_disable();
	nrf_usbd_common_uninit();

	usbd_evt_flush();

	ret = eps_ctx_init();
	__ASSERT_NO_MSG(ret == 0);

	nrfx_power_usbevt_enable();
	err = nrf_usbd_common_init(usbd_event_handler);

	if (err != NRFX_SUCCESS) {
		LOG_DBG("nRF USBD driver reinit failed. Code: %d", err);
		__ASSERT_NO_MSG(0);
	}
}

/**
 * @brief function to generate fake receive request for
 * ISO OUT EP.
 *
 * ISO OUT endpoint does not generate irq by itself and reading
 * from ISO OUT ep is synchronized with SOF frame. For more details
 * refer to Nordic usbd specification.
 */
static void usbd_sof_trigger_iso_read(void)
{
	struct usbd_event *ev;
	struct nrf_usbd_ep_ctx *ep_ctx;

	ep_ctx = endpoint_ctx(NRF_USBD_COMMON_EPOUT8);
	if (!ep_ctx) {
		LOG_ERR("There is no ISO ep");
		return;
	}

	if (ep_ctx->cfg.en) {
		/* Dissect receive request
		 * if the iso OUT ep is enabled
		 */
		ep_ctx->read_pending = true;
		ep_ctx->read_complete = true;
		ev = usbd_evt_alloc();
		if (!ev) {
			LOG_ERR("Failed to alloc evt");
			return;
		}
		ev->evt_type = USBD_EVT_EP;
		ev->evt.ep_evt.evt_type = EP_EVT_RECV_REQ;
		ev->evt.ep_evt.ep = ep_ctx;
		usbd_evt_put(ev);
		usbd_work_schedule();
	} else {
		LOG_DBG("Endpoint is not enabled");
	}
}

/* Work handler */
static void usbd_work_handler(struct k_work *item)
{
	struct nrf_usbd_ctx *ctx;
	struct usbd_event *ev;

	ctx = CONTAINER_OF(item, struct nrf_usbd_ctx, usb_work);

	while ((ev = usbd_evt_get()) != NULL) {
		if (!dev_ready() && ev->evt_type != USBD_EVT_POWER) {
			/* Drop non-power events when cable is detached. */
			usbd_evt_free(ev);
			continue;
		}

		switch (ev->evt_type) {
		case USBD_EVT_EP:
			if (!ctx->attached) {
				LOG_ERR("not attached, EP 0x%02x event dropped",
					(uint32_t)ev->evt.ep_evt.ep->cfg.addr);
			}
			usbd_work_process_ep_events(&ev->evt.ep_evt);
			break;
		case USBD_EVT_POWER:
			usbd_work_process_pwr_events(&ev->evt.pwr_evt);
			break;
		case USBD_EVT_RESET:
			LOG_DBG("USBD reset event");
			k_mutex_lock(&ctx->drv_lock, K_FOREVER);
			eps_ctx_init();
			k_mutex_unlock(&ctx->drv_lock);

			if (ctx->status_cb) {
				ctx->status_cb(USB_DC_RESET, NULL);
			}
			break;
		case USBD_EVT_SOF:
			usbd_sof_trigger_iso_read();

			if (ctx->status_cb) {
				ctx->status_cb(USB_DC_SOF, NULL);
			}
			break;
		case USBD_EVT_REINIT: {
				/*
				 * Reinitialize the peripheral after queue
				 * overflow.
				 */
				LOG_ERR("USBD event queue full!");
				usbd_reinit();
				break;
			}
		default:
			LOG_ERR("Unknown USBD event: %"PRId16, ev->evt_type);
			break;
		}
		usbd_evt_free(ev);
	}
}

int usb_dc_attach(void)
{
	struct nrf_usbd_ctx *ctx = get_usbd_ctx();
	int ret;

	if (ctx->attached) {
		return 0;
	}

	k_mutex_init(&ctx->drv_lock);
	ctx->hfxo_mgr =
		z_nrf_clock_control_get_onoff(
			COND_CODE_1(NRF_CLOCK_HAS_HFCLK192M,
				    (CLOCK_CONTROL_NRF_SUBSYS_HF192M),
				    (CLOCK_CONTROL_NRF_SUBSYS_HF)));

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    nrfx_isr, nrf_usbd_common_irq_handler, 0);

	nrfx_power_usbevt_enable();

	ret = eps_ctx_init();
	if (ret == 0) {
		ctx->attached = true;
	}

	if (!k_fifo_is_empty(&usbd_evt_fifo)) {
		usbd_work_schedule();
	}

	if (nrfx_power_usbstatus_get() != NRFX_POWER_USB_STATE_DISCONNECTED) {
		/* USBDETECTED event is be generated on cable attachment and
		 * when cable is already attached during reset, but not when
		 * the peripheral is re-enabled.
		 * When USB-enabled bootloader is used, target application
		 * will not receive this event and it needs to be generated
		 * again here.
		 */
		usb_dc_power_event_handler(NRFX_POWER_USB_EVT_DETECTED);
	}

	return ret;
}

int usb_dc_detach(void)
{
	struct nrf_usbd_ctx *ctx = get_usbd_ctx();

	k_mutex_lock(&ctx->drv_lock, K_FOREVER);

	usbd_evt_flush();

	if (nrf_usbd_common_is_enabled()) {
		nrf_usbd_common_disable();
	}

	(void)hfxo_stop(ctx);
	nrfx_power_usbevt_disable();

	ctx->attached = false;
	k_mutex_unlock(&ctx->drv_lock);

	return 0;
}

int usb_dc_reset(void)
{
	int ret;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	LOG_DBG("USBD Reset");

	ret = usb_dc_detach();
	if (ret) {
		return ret;
	}

	ret = usb_dc_attach();
	if (ret) {
		return ret;
	}

	return 0;
}

int usb_dc_set_address(const uint8_t addr)
{
	struct nrf_usbd_ctx *ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	/**
	 * Nothing to do here. The USBD HW already takes care of initiating
	 * STATUS stage. Just double check the address for sanity.
	 */
	__ASSERT(addr == (uint8_t)NRF_USBD->USBADDR, "USB Address incorrect!");

	ctx = get_usbd_ctx();

	LOG_DBG("Address set to: %d", addr);

	return 0;
}


int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data *const ep_cfg)
{
	uint8_t ep_idx = NRF_USBD_EP_NR_GET(ep_cfg->ep_addr);

	LOG_DBG("ep 0x%02x, mps %d, type %d", ep_cfg->ep_addr, ep_cfg->ep_mps,
		ep_cfg->ep_type);

	if ((ep_cfg->ep_type == USB_DC_EP_CONTROL) && ep_idx) {
		LOG_ERR("invalid endpoint configuration");
		return -1;
	}

	if (!NRF_USBD_EP_VALIDATE(ep_cfg->ep_addr)) {
		LOG_ERR("invalid endpoint index/address");
		return -1;
	}

	if ((ep_cfg->ep_type == USB_DC_EP_ISOCHRONOUS) &&
	    (!NRF_USBD_EPISO_CHECK(ep_cfg->ep_addr))) {
		LOG_WRN("invalid endpoint type");
		return -1;
	}

	if ((ep_cfg->ep_type != USB_DC_EP_ISOCHRONOUS) &&
	    (NRF_USBD_EPISO_CHECK(ep_cfg->ep_addr))) {
		LOG_WRN("iso endpoint can only be iso");
		return -1;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data *const ep_cfg)
{
	struct nrf_usbd_ep_ctx *ep_ctx;

	if (!dev_attached()) {
		return -ENODEV;
	}

	/**
	 * TODO:
	 * For ISO endpoints, application has to use EPIN/OUT 8
	 * but right now there's no standard way of knowing the
	 * ISOIN/ISOOUT endpoint number in advance to configure
	 * accordingly. So either this needs to be chosen in the
	 * menuconfig in application area or perhaps in device tree
	 * at compile time or introduce a new API to read the endpoint
	 * configuration at runtime before configuring them.
	 */
	ep_ctx = endpoint_ctx(ep_cfg->ep_addr);
	if (!ep_ctx) {
		return -EINVAL;
	}

	ep_ctx->cfg.addr = ep_cfg->ep_addr;
	ep_ctx->cfg.type = ep_cfg->ep_type;
	ep_ctx->cfg.max_sz = ep_cfg->ep_mps;

	if (!NRF_USBD_EPISO_CHECK(ep_cfg->ep_addr)) {
		if ((ep_cfg->ep_mps & (ep_cfg->ep_mps - 1)) != 0U) {
			LOG_ERR("EP max packet size must be a power of 2");
			return -EINVAL;
		}
	}

	nrf_usbd_common_ep_max_packet_size_set(ep_addr_to_nrfx(ep_cfg->ep_addr),
					       ep_cfg->ep_mps);

	return 0;
}

int usb_dc_ep_set_stall(const uint8_t ep)
{
	struct nrf_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	switch (ep_ctx->cfg.type) {
	case USB_DC_EP_CONTROL:
		nrf_usbd_common_setup_stall();
		break;
	case USB_DC_EP_BULK:
	case USB_DC_EP_INTERRUPT:
		nrf_usbd_common_ep_stall(ep_addr_to_nrfx(ep));
		break;
	case USB_DC_EP_ISOCHRONOUS:
		LOG_ERR("STALL unsupported on ISO endpoint");
		return -EINVAL;
	}

	ep_ctx->buf.len = 0U;
	ep_ctx->buf.curr = ep_ctx->buf.data;

	LOG_DBG("STALL on EP 0x%02x", ep);

	return 0;
}

int usb_dc_ep_clear_stall(const uint8_t ep)
{

	struct nrf_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (NRF_USBD_EPISO_CHECK(ep)) {
		/* ISO transactions do not support a handshake phase. */
		return -EINVAL;
	}

	nrf_usbd_common_ep_dtoggle_clear(ep_addr_to_nrfx(ep));
	nrf_usbd_common_ep_stall_clear(ep_addr_to_nrfx(ep));
	LOG_DBG("Unstall on EP 0x%02x", ep);

	return 0;
}

int usb_dc_ep_halt(const uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *const stalled)
{
	struct nrf_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!stalled) {
		return -EINVAL;
	}

	*stalled = (uint8_t) nrf_usbd_common_ep_stall_check(ep_addr_to_nrfx(ep));

	return 0;
}

int usb_dc_ep_enable(const uint8_t ep)
{
	struct nrf_usbd_ep_ctx *ep_ctx;

	if (!dev_attached()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!NRF_USBD_EPISO_CHECK(ep)) {
		/* ISO transactions for full-speed device do not support
		 * toggle sequencing and should only send DATA0 PID.
		 */
		nrf_usbd_common_ep_dtoggle_clear(ep_addr_to_nrfx(ep));
		/** Endpoint is enabled on SetInterface request.
		 * This should also clear EP's halt status.
		 */
		nrf_usbd_common_ep_stall_clear(ep_addr_to_nrfx(ep));
	}
	if (ep_ctx->cfg.en) {
		return -EALREADY;
	}

	LOG_DBG("EP enable: 0x%02x", ep);

	ep_ctx->cfg.en = true;

	/* Defer the endpoint enable if USBD is not ready yet. */
	if (dev_ready()) {
		nrf_usbd_common_ep_enable(ep_addr_to_nrfx(ep));
	}

	return 0;
}

int usb_dc_ep_disable(const uint8_t ep)
{
	struct nrf_usbd_ep_ctx *ep_ctx;

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!ep_ctx->cfg.en) {
		return -EALREADY;
	}

	LOG_DBG("EP disable: 0x%02x", ep);

	nrf_usbd_common_ep_disable(ep_addr_to_nrfx(ep));
	/* Clear write_in_progress as nrf_usbd_common_ep_disable()
	 * terminates endpoint transaction.
	 */
	ep_ctx->write_in_progress = false;
	ep_ctx_reset(ep_ctx);
	ep_ctx->cfg.en = false;

	return 0;
}

int usb_dc_ep_flush(const uint8_t ep)
{
	struct nrf_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	ep_ctx->buf.len = 0U;
	ep_ctx->buf.curr = ep_ctx->buf.data;

	nrf_usbd_common_transfer_out_drop(ep_addr_to_nrfx(ep));

	return 0;
}

int usb_dc_ep_write(const uint8_t ep, const uint8_t *const data,
		    const uint32_t data_len, uint32_t *const ret_bytes)
{
	LOG_DBG("ep_write: ep 0x%02x, len %d", ep, data_len);
	struct nrf_usbd_ctx *ctx = get_usbd_ctx();
	struct nrf_usbd_ep_ctx *ep_ctx;
	int result = 0;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	if (NRF_USBD_EPOUT_CHECK(ep)) {
		return -EINVAL;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!ep_ctx->cfg.en) {
		LOG_ERR("Endpoint 0x%02x is not enabled", ep);
		return -EINVAL;
	}

	k_mutex_lock(&ctx->drv_lock, K_FOREVER);

	/* USBD driver does not allow scheduling multiple DMA transfers
	 * for one EP at a time. Next USB transfer on this endpoint can be
	 * triggered after the completion of previous one.
	 */
	if (ep_ctx->write_in_progress) {
		k_mutex_unlock(&ctx->drv_lock);
		return -EAGAIN;
	}

	/** Clear the ZLP flag if current write is ZLP. After the ZLP will be
	 * send the driver will perform status stage.
	 */
	if (!data_len && ep_ctx->trans_zlp) {
		ep_ctx->trans_zlp = false;
	}

	/** If writing to a Control Endpoint there might be a need to transfer
	 * ZLP. If the Hosts asks for more data that the device may return and
	 * the last packet is wMaxPacketSize long. The driver must send ZLP.
	 * For consistence with the Zephyr USB stack sending ZLP must be issued
	 * from the stack level. Making trans_zlp flag true results in blocking
	 * the driver from starting setup stage without required ZLP.
	 */
	if (ep_ctx->cfg.type == USB_DC_EP_CONTROL) {
		if (data_len && usbd_ctx.setup.wLength > data_len &&
		    !(data_len % ep_ctx->cfg.max_sz)) {
			ep_ctx->trans_zlp = true;
		}
	}

	/* Setup stage is handled by hardware.
	 * Detect the setup stage initiated by the stack
	 * and perform appropriate action.
	 */
	if ((ep_ctx->cfg.type == USB_DC_EP_CONTROL)
	    && (nrf_usbd_common_last_setup_dir_get() != ep)) {
		nrf_usbd_common_setup_clear();
		k_mutex_unlock(&ctx->drv_lock);
		return 0;
	}

	ep_ctx->write_in_progress = true;
	NRF_USBD_COMMON_TRANSFER_IN(transfer, data, data_len, 0);
	nrfx_err_t err = nrf_usbd_common_ep_transfer(ep_addr_to_nrfx(ep), &transfer);

	if (err != NRFX_SUCCESS) {
		ep_ctx->write_in_progress = false;
		if (ret_bytes) {
			*ret_bytes = 0;
		}
		result = -EIO;
		LOG_ERR("nRF USBD write error: %d", (uint32_t)err);
	} else {
		if (ret_bytes) {
			*ret_bytes = data_len;
		}
	}

	k_mutex_unlock(&ctx->drv_lock);
	return result;
}

int usb_dc_ep_read_wait(uint8_t ep, uint8_t *data, uint32_t max_data_len,
			uint32_t *read_bytes)
{
	struct nrf_usbd_ep_ctx *ep_ctx;
	struct nrf_usbd_ctx *ctx = get_usbd_ctx();
	uint32_t bytes_to_copy;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	if (NRF_USBD_EPIN_CHECK(ep)) {
		return -EINVAL;
	}

	if (!data && max_data_len) {
		return -EINVAL;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!ep_ctx->cfg.en) {
		LOG_ERR("Endpoint 0x%02x is not enabled", ep);
		return -EINVAL;
	}

	k_mutex_lock(&ctx->drv_lock, K_FOREVER);

	bytes_to_copy = MIN(max_data_len, ep_ctx->buf.len);

	if (!data && !max_data_len) {
		if (read_bytes) {
			*read_bytes = ep_ctx->buf.len;
		}
		k_mutex_unlock(&ctx->drv_lock);
		return 0;
	}

	memcpy(data, ep_ctx->buf.curr, bytes_to_copy);

	ep_ctx->buf.curr += bytes_to_copy;
	ep_ctx->buf.len -= bytes_to_copy;
	if (read_bytes) {
		*read_bytes = bytes_to_copy;
	}

	k_mutex_unlock(&ctx->drv_lock);
	return 0;
}

int usb_dc_ep_read_continue(uint8_t ep)
{
	struct nrf_usbd_ep_ctx *ep_ctx;
	struct nrf_usbd_ctx *ctx = get_usbd_ctx();

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	if (NRF_USBD_EPIN_CHECK(ep)) {
		return -EINVAL;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!ep_ctx->cfg.en) {
		LOG_ERR("Endpoint 0x%02x is not enabled", ep);
		return -EINVAL;
	}

	k_mutex_lock(&ctx->drv_lock, K_FOREVER);
	if (!ep_ctx->buf.len) {
		ep_ctx->buf.curr = ep_ctx->buf.data;
		ep_ctx->read_complete = true;

		if (ep_ctx->read_pending) {
			struct usbd_event *ev = usbd_evt_alloc();

			if (!ev) {
				k_mutex_unlock(&ctx->drv_lock);
				return -ENOMEM;
			}

			ev->evt_type = USBD_EVT_EP;
			ev->evt.ep_evt.ep = ep_ctx;
			ev->evt.ep_evt.evt_type = EP_EVT_RECV_REQ;
			usbd_evt_put(ev);
			usbd_work_schedule();
		}
	}
	k_mutex_unlock(&ctx->drv_lock);

	return 0;
}

int usb_dc_ep_read(const uint8_t ep, uint8_t *const data,
		   const uint32_t max_data_len, uint32_t *const read_bytes)
{
	LOG_DBG("ep_read: ep 0x%02x, maxlen %d", ep, max_data_len);
	int ret;

	ret = usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes);
	if (ret) {
		return ret;
	}

	if (!data && !max_data_len) {
		return ret;
	}

	ret = usb_dc_ep_read_continue(ep);
	return ret;
}

int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	struct nrf_usbd_ep_ctx *ep_ctx;

	if (!dev_attached()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	ep_ctx->cfg.cb = cb;

	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	get_usbd_ctx()->status_cb = cb;
}

int usb_dc_ep_mps(const uint8_t ep)
{
	struct nrf_usbd_ep_ctx *ep_ctx;

	if (!dev_attached()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	return ep_ctx->cfg.max_sz;
}

int usb_dc_wakeup_request(void)
{
	bool res = nrf_usbd_common_wakeup_req();

	if (!res) {
		return -EAGAIN;
	}
	return 0;
}

static int usb_init(void)
{
	struct nrf_usbd_ctx *ctx = get_usbd_ctx();
	nrfx_err_t err;

#ifdef CONFIG_HAS_HW_NRF_USBREG
	/* Use CLOCK/POWER priority for compatibility with other series where
	 * USB events are handled by CLOCK interrupt handler.
	 */
	IRQ_CONNECT(USBREGULATOR_IRQn,
		    DT_IRQ(DT_INST(0, nordic_nrf_clock), priority),
		    nrfx_isr, nrfx_usbreg_irq_handler, 0);
#endif

	static const nrfx_power_config_t power_config = {
		.dcdcen = (DT_PROP(DT_INST(0, nordic_nrf5x_regulator), regulator_initial_mode)
			   == NRF5X_REG_MODE_DCDC),
#if NRFX_POWER_SUPPORTS_DCDCEN_VDDH
		.dcdcenhv = COND_CODE_1(CONFIG_SOC_SERIES_NRF52X,
			(DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nordic_nrf52x_regulator_hv))),
			(DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nordic_nrf53x_regulator_hv)))),
#endif
	};

	static const nrfx_power_usbevt_config_t usbevt_config = {
		.handler = usb_dc_power_event_handler
	};

	err = nrf_usbd_common_init(usbd_event_handler);
	if (err != NRFX_SUCCESS) {
		LOG_DBG("nRF USBD driver init failed. Code: %d", (uint32_t)err);
		return -EIO;
	}

	/* Ignore the return value, as NRFX_ERROR_ALREADY_INITIALIZED is not
	 * a problem here.
	 */
	(void)nrfx_power_init(&power_config);
	nrfx_power_usbevt_init(&usbevt_config);

	k_work_queue_start(&usbd_work_queue,
			   usbd_work_queue_stack,
			   K_KERNEL_STACK_SIZEOF(usbd_work_queue_stack),
			   CONFIG_SYSTEM_WORKQUEUE_PRIORITY, NULL);

	k_thread_name_set(&usbd_work_queue.thread, "usbd_workq");
	k_work_init(&ctx->usb_work, usbd_work_handler);

	return 0;
}

SYS_INIT(usb_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
