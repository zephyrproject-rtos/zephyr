/*
 * Copyright (c) 2018 Sundar Subramaniyan <sundar.subramaniyan@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  usb_dc_nrf5.c
 * @brief nRF52840 USB device controller driver
 *
 * The driver implements the low level control routines to deal directly
 * with the nRF52840 USBD peripheral.
 */

#include <soc.h>
#include <string.h>
#include <stdio.h>
#include <kernel.h>
#include <usb/usb_dc.h>
#include <usb/usb_device.h>
#include <clock_control.h>
#include <nrf_power.h>
#include <drivers/clock_control/nrf5_clock_control.h>

/* TODO: Find alternative for ASSERT in nrf_usbd.h */
#ifndef ASSERT
#define ASSERT(__x) __ASSERT_NO_MSG((__x))
#endif

#include <nrf_usbd.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DRIVER_LEVEL
#define SYS_LOG_DOMAIN "usb/dc"
#include <logging/sys_log.h>

#define MAX_EP_BUF_SZ		64UL
#define MAX_ISO_EP_BUF_SZ	1024UL

#define USBD_EPSTATUS_EPIN_MASK		(0x1FF << USBD_EPSTATUS_EPIN0_Pos)
#define USBD_EPSTATUS_EPOUT_MASK	(0x1FF << USBD_EPSTATUS_EPOUT0_Pos)
#define USBD_EPDATASTATUS_EPIN_MASK	(0x7F << USBD_EPDATASTATUS_EPIN1_Pos)
#define USBD_EPDATASTATUS_EPOUT_MASK	(0x7F << USBD_EPDATASTATUS_EPOUT1_Pos)

/** USB Work flags */
#define NRF5_USB_STATE_CHANGE	0
#define NRF5_USB_STATUS_CHANGE	1

/**
 * @brief USBD states
 */
enum nrf5_usbd_state {
	USBD_DETACHED,
	USBD_ATTACHED,
	USBD_POWERED,
	USBD_SUSPENDED,
	USBD_DEFAULT,
	USBD_ADDRESS_SET,
	USBD_CONFIGURED,
};

/**
 * @brief Endpoint state
 */
enum ep_state {
	EP_IDLE,
	EP_SETUP,
	EP_DATA,
	EP_STATUS,
};

/**
 * @brief Endpoint event
 */
enum ep_event {
	EP_SETUP_RECV,
	EP_DATA_RECV,
	EP_DMA_START,
	EP_DMA_END,
	EP_WRITE_COMPLETE,
	EP_SOF,
};

/**
 * @brief Miscellaneous endpoint event flags
 */
#define EP_CONTROL_READ			0
#define EP_CONTROL_WRITE		1
#define EP_CONTROL_WRITE_NO_DATA	2
#define EP_OUT_DATA_RCVD		3
#define EP_WRITE_PENDING		4

/**
 * @brief Endpoint configuration
 *
 * @param en		Enable/Disable flag
 * @param addr		Endpoint address
 * @param max_sz	Max packet size supported by endpoint
 * @param type		Endpoint type
 * @param cb		Endpoint callback
 */
struct nrf5_usbd_ep_cfg {
	bool en;
	u8_t addr;
	u32_t max_sz;
	enum usb_dc_ep_type type;
	usb_dc_ep_callback cb;
};

/**
 * @brief Endpoint buffer
 *
 * @param data	Pointer to the data buffer
 *		for the endpoint
 * @param curr	Pointer to the current
 *		offset in the endpoint buffer
 * @param len	Remaining length to be read/written
 * @param block	Mempool block, for freeing up buffer after use
 */
struct nrf5_usbd_ep_buf {
	u8_t *data;
	u8_t *curr;
	u32_t len;
	struct k_mem_block block;
};

/**
 * @brief Endpoint context
 *
 * @param cfg		Endpoint configuration
 * @param buf		Endpoint buffer
 * @param state		Endpoint's current state
 * @param flags		Endpoint's flags
 */
struct nrf5_usbd_ep_ctx {
	struct nrf5_usbd_ep_cfg cfg;
	struct nrf5_usbd_ep_buf buf;
	enum ep_state state;
	u32_t flags;
};

/**
 * @brief Endpoint USB event
 *	  Used by ISR to send events to work handler
 *
 * @param node		Used by the kernel for FIFO management
 * @param ep		Endpoint context pointer that needs service
 * @param evt		Event that has occurred from the USBD peripheral
 * @param block		Mempool block pointer for freeing up after use
 * @param misc_u	Miscellaneous information passed as flags
 */
struct ep_usb_event {
	sys_snode_t node;
	struct nrf5_usbd_ep_ctx *ep;
	enum ep_event evt;
	struct k_mem_block block;
	union {
		u32_t flags;
		u32_t frame_counter;
	} misc_u;
};

/**
 * @brief Fifo element pool
 *	Used for allocating fifo elements to pass from ISR to work handler
 * TODO: The number of FIFO elements is an arbitrary number now but it should
 * be derived from the theoretical number of backlog events possible depending
 * on the number of endpoints configured.
 */
#define FIFO_ELEM_MIN_SZ	sizeof(struct ep_usb_event)
#define FIFO_ELEM_MAX_SZ	sizeof(struct ep_usb_event)
#define FIFO_ELEM_COUNT		16
#define FIFO_ELEM_ALIGN		sizeof(unsigned int)

K_MEM_POOL_DEFINE(fifo_elem_pool, FIFO_ELEM_MIN_SZ, FIFO_ELEM_MAX_SZ,
		  FIFO_ELEM_COUNT, FIFO_ELEM_ALIGN);

/**
 * @brief Endpoint buffer pool
 *	Used for allocating buffers for the endpoints' data transfer
 *	Max pool size possible: 3072 Bytes (16 EP * 64B + 2 ISO * 1024B)
 */

/** Number of IN Endpoints configured (including control) */
#define CFG_EPIN_CNT (CONFIG_USBD_NRF5_NUM_IN_EP + \
		      CONFIG_USBD_NRF5_NUM_BIDIR_EP)

/** Number of OUT Endpoints configured (including control) */
#define CFG_EPOUT_CNT (CONFIG_USBD_NRF5_NUM_OUT_EP + \
		       CONFIG_USBD_NRF5_NUM_BIDIR_EP)

/** Number of ISO IN Endpoints */
#define CFG_EP_ISOIN_CNT CONFIG_USBD_NRF5_NUM_ISOIN_EP

/** Number of ISO OUT Endpoints */
#define CFG_EP_ISOOUT_CNT CONFIG_USBD_NRF5_NUM_ISOOUT_EP

/** ISO endpoint index */
#define EP_ISOIN_INDEX CFG_EPIN_CNT
#define EP_ISOOUT_INDEX (CFG_EPIN_CNT + CFG_EP_ISOIN_CNT + CFG_EPOUT_CNT)

/** Minimum endpoint buffer size */
#define EP_BUF_MIN_SZ MAX_EP_BUF_SZ

/** Maximum endpoint buffer size */
#if (CFG_EP_ISOIN_CNT || CFG_EP_ISOOUT_CNT)
#define EP_BUF_MAX_SZ MAX_ISO_EP_BUF_SZ
#else
#define EP_BUF_MAX_SZ MAX_EP_BUF_SZ
#endif

/** Total endpoints configured */
#define CFG_EP_CNT (CFG_EPIN_CNT + CFG_EP_ISOIN_CNT + \
		    CFG_EPOUT_CNT + CFG_EP_ISOOUT_CNT)

/** Total buffer size for all endpoints */
#define EP_BUF_TOTAL ((CFG_EPIN_CNT * MAX_EP_BUF_SZ) + \
		      (CFG_EPOUT_CNT * MAX_EP_BUF_SZ) + \
		      (CFG_EP_ISOIN_CNT * MAX_ISO_EP_BUF_SZ) +	\
		      (CFG_EP_ISOOUT_CNT * MAX_ISO_EP_BUF_SZ))

/** Total number of maximum sized buffers needed */
#define EP_BUF_COUNT ((EP_BUF_TOTAL / EP_BUF_MAX_SZ) + \
		      ((EP_BUF_TOTAL % EP_BUF_MAX_SZ) ? 1 : 0))

/** 4 Byte Buffer alignment required by hardware */
#define EP_BUF_ALIGN sizeof(unsigned int)

K_MEM_POOL_DEFINE(ep_buf_pool, EP_BUF_MIN_SZ, EP_BUF_MAX_SZ,
		  EP_BUF_COUNT, EP_BUF_ALIGN);

/**
 * @brief USBD private structure
 *
 * @param enabled	USBD Enable/Disable flag
 * @param attached	USBD Attached flag
 * @param ready		USBD Ready flag set after pullup
 * @param address_set	USB Address set or unset
 * @param state		USBD state
 * @param status_code	Device Status code
 * @param flags		Flags used in work context
 * @param enable_mask	Enabled interrupts mask
 * @param usb_work	USBD work item
 * @param work_queue	FIFO used for queuing up events from ISR
 * @param dma_in_use	Semaphore to restrict access to DMA one at a time
 * @param status_cb	Status callback for USB DC notifications
 * @param ep_ctx	Endpoint contexts
 * @param buf		Buffer for the endpoints, aligned to 4 byte boundary
 */
struct nrf5_usbd_ctx {
	bool enabled;
	bool attached;
	bool ready;
	bool address_set;
	enum nrf5_usbd_state state;
	enum usb_dc_status_code status_code;
	u32_t flags;
	u32_t enable_mask;
	struct k_work usb_work;
	struct k_fifo work_queue;
	struct k_sem dma_in_use;
	usb_dc_status_callback status_cb;
	struct nrf5_usbd_ep_ctx ep_ctx[CFG_EP_CNT];
};

static struct nrf5_usbd_ctx usbd_ctx;

static inline struct nrf5_usbd_ctx *get_usbd_ctx(void)
{
	return &usbd_ctx;
}

static inline bool ep_is_valid(const u8_t ep)
{
	u8_t ep_num = NRF_USBD_EP_NR_GET(ep);

	if (NRF_USBD_EPIN_CHECK(ep)) {
		if (unlikely(NRF_USBD_EPISO_CHECK(ep))) {
			if (CFG_EP_ISOIN_CNT == 0) {
				return false;
			}
		} else {
			if (ep_num >= CFG_EPIN_CNT) {
				return false;
			}
		}
	} else {
		if (unlikely(NRF_USBD_EPISO_CHECK(ep))) {
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

static struct nrf5_usbd_ep_ctx *endpoint_ctx(const u8_t ep)
{
	struct nrf5_usbd_ctx *ctx;
	u8_t ep_num;

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

static struct nrf5_usbd_ep_ctx *in_endpoint_ctx(const u8_t ep)
{
	return endpoint_ctx(NRF_USBD_EPIN(ep));
}

static struct nrf5_usbd_ep_ctx *out_endpoint_ctx(const u8_t ep)
{
	return endpoint_ctx(NRF_USBD_EPOUT(ep));
}

static void cfg_ep_interrupt(const u8_t ep, bool set)
{
	struct nrf5_usbd_ctx *ctx = get_usbd_ctx();
	u8_t ep_num = NRF_USBD_EP_NR_GET(ep);
	u32_t mask = 0;

	if (NRF_USBD_EPIN_CHECK(ep)) {
		if (NRF_USBD_EPISO_CHECK(ep)) {
			mask |= (NRF_USBD_INT_ENDISOIN0_MASK |
				 NRF_USBD_INT_SOF_MASK);
		} else {
			mask |= BIT(ep_num + USBD_INTEN_ENDEPIN0_Pos);
			if (ep_num == 0) {
				mask |= NRF_USBD_INT_EP0DATADONE_MASK;
				mask |= NRF_USBD_INT_EP0SETUP_MASK;
			} else {
				mask |= NRF_USBD_INT_DATAEP_MASK;
			}
		}
	} else {
		if (NRF_USBD_EPISO_CHECK(ep)) {
			mask |= (NRF_USBD_INT_ENDISOOUT0_MASK |
				 NRF_USBD_INT_SOF_MASK);
		} else {
			mask |= BIT(ep_num + USBD_INTEN_ENDEPOUT0_Pos);
			if (ep_num == 0) {
				mask |= NRF_USBD_INT_EP0DATADONE_MASK;
				mask |= NRF_USBD_INT_EP0SETUP_MASK;
			} else {
				mask |= NRF_USBD_INT_DATAEP_MASK;
			}
		}
	}

	irq_disable(CONFIG_USBD_NRF5_IRQ);

	if (set) {
		mask |= NRF_USBD_INT_STARTED_MASK;
		nrf_usbd_int_enable(mask);
	} else {
		nrf_usbd_int_disable(mask);
	}

	ctx->enable_mask = nrf_usbd_int_enable_get();

	irq_enable(CONFIG_USBD_NRF5_IRQ);
}

static struct nrf5_usbd_ep_ctx *epstatus_to_ep_ctx(u32_t epstatus)
{
	int pos;
	u8_t ep = 0;

	/**
	 * TODO: Is it possible for multiple bits to be set?
	 * When debug logs are enabled, they cause delays in
	 * handling the ISR and possibly multiple EP status
	 * or EPDATA status were set. If this is common, we
	 * may have to handle them all here by allocating
	 * multiple events and queue them up to the FIFO.
	 */
	if (popcount(epstatus) > 1) {
		SYS_LOG_ERR("%d bits set in epstatus!!",
			    popcount(epstatus));
		__ASSERT_NO_MSG(0);
	}

	if (epstatus & USBD_EPSTATUS_EPIN_MASK) {
		pos = USBD_EPSTATUS_EPIN0_Pos;

		while (pos <= USBD_EPSTATUS_EPIN8_Pos) {
			if (epstatus & BIT(pos)) {
				ep = NRF_USBD_EPIN(pos);
				return endpoint_ctx(ep);
			}
			pos++;
		}
	}

	if (epstatus & USBD_EPSTATUS_EPOUT_MASK) {
		pos = USBD_EPSTATUS_EPOUT0_Pos;

		while (pos <= USBD_EPSTATUS_EPOUT8_Pos) {
			if (epstatus & BIT(pos)) {
				ep = NRF_USBD_EPOUT(pos -
						    USBD_EPSTATUS_EPOUT0_Pos);
				return endpoint_ctx(ep);
			}
			pos++;
		}
	}

	SYS_LOG_ERR("invalid epstatus 0x%08x", epstatus);
	__ASSERT_NO_MSG(0);

	return NULL;
}

static struct nrf5_usbd_ep_ctx *epdatastatus_to_ep_ctx(u32_t epdatastatus)
{
	int pos;
	u8_t ep = 0;

	/**
	 * TODO: Is it possible for multiple bits to be set?
	 * When debug logs are enabled, they cause delays in
	 * handling the ISR and possibly multiple EP status
	 * or EPDATA status were set. If this is common, we
	 * may have to handle them all here by allocating
	 * multiple events and queue them up to the FIFO.
	 */
	if (popcount(epdatastatus) > 1) {
		SYS_LOG_ERR("%d bits set in epdatastatus!!",
			    popcount(epdatastatus));
		__ASSERT_NO_MSG(0);
	}

	if (epdatastatus & USBD_EPDATASTATUS_EPIN_MASK) {
		pos = USBD_EPDATASTATUS_EPIN1_Pos;

		while (pos <= USBD_EPDATASTATUS_EPIN7_Pos) {
			if (epdatastatus & BIT(pos)) {
				ep = NRF_USBD_EPIN(pos);
				return endpoint_ctx(ep);
			}
			pos++;
		}
	}

	if (epdatastatus & USBD_EPDATASTATUS_EPOUT_MASK) {
		pos = USBD_EPDATASTATUS_EPOUT1_Pos;

		while (pos <= USBD_EPDATASTATUS_EPOUT7_Pos) {
			if (epdatastatus & BIT(pos)) {
				ep = NRF_USBD_EPOUT(pos -
						    USBD_EPSTATUS_EPOUT0_Pos);
				return endpoint_ctx(ep);
			}
			pos++;
		}
	}

	SYS_LOG_ERR("invalid epdatastatus 0x%08x", epdatastatus);
	__ASSERT_NO_MSG(0);

	return NULL;
}

static void start_epin_task(u8_t ep)
{
	u8_t epnum = NRF_USBD_EP_NR_GET(ep);
	nrf_usbd_task_t task;

	if (NRF_USBD_EPOUT_CHECK(ep)) {
		SYS_LOG_ERR("invalid endpoint!");
		return;
	}

	if (epnum > NRF_USBD_EPIN_CNT) {
		SYS_LOG_ERR("invalid endpoint %d", epnum);
		return;
	}

	task = NRF_USBD_TASK_STARTEPIN0 + (epnum * sizeof(u32_t));
	nrf_usbd_task_trigger(task);
}

static void start_epout_task(u8_t ep)
{
	u8_t epnum = NRF_USBD_EP_NR_GET(ep);
	nrf_usbd_task_t task;

	if (NRF_USBD_EPIN_CHECK(ep)) {
		SYS_LOG_ERR("invalid endpoint!");
		return;
	}

	if (epnum > NRF_USBD_EPOUT_CNT) {
		SYS_LOG_ERR("invalid endpoint %d", epnum);
		return;
	}

	task = NRF_USBD_TASK_STARTEPOUT0 + (epnum * sizeof(u32_t));
	nrf_usbd_task_trigger(task);
}

static void start_ep0rcvout_task(void)
{
	nrf_usbd_task_trigger(NRF_USBD_TASK_EP0RCVOUT);
}

static void start_ep0status_task(void)
{
	nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STATUS);
}

static void start_ep0stall_task(void)
{
	nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STALL);
}

static inline struct ep_usb_event *alloc_ep_usb_event(void)
{
	int ret;
	struct ep_usb_event *ev;
	struct k_mem_block block;

	ret = k_mem_pool_alloc(&fifo_elem_pool, &block,
			       sizeof(struct ep_usb_event),
			       K_NO_WAIT);
	if (ret < 0) {
		SYS_LOG_DBG("ep usb event alloc failed!");
		__ASSERT_NO_MSG(0);
		return NULL;
	}

	ev = (struct ep_usb_event *)block.data;
	memcpy(&ev->block, &block, sizeof(block));
	ev->misc_u.flags = 0;

	return ev;
}

static inline void free_ep_usb_event(struct ep_usb_event *ev)
{
	k_mem_pool_free(&ev->block);
}

static inline void enqueue_ep_usb_event(struct ep_usb_event *ev)
{
	k_fifo_put(&get_usbd_ctx()->work_queue, ev);
}

static inline struct ep_usb_event *dequeue_ep_usb_event(void)
{
	return k_fifo_get(&get_usbd_ctx()->work_queue, K_NO_WAIT);
}

static inline void flush_ep_usb_events(void)
{
	struct ep_usb_event *ev;

	do {
		ev = dequeue_ep_usb_event();
		if (ev) {
			free_ep_usb_event(ev);
		}
	} while (ev != NULL);
}

/** Interrupt mask to event register map */
#define USBD_INT_CNT 25

static const u32_t event_map[USBD_INT_CNT] = {
	NRF_USBD_EVENT_USBRESET,
	NRF_USBD_EVENT_STARTED,
	NRF_USBD_EVENT_ENDEPIN0,
	NRF_USBD_EVENT_ENDEPIN1,
	NRF_USBD_EVENT_ENDEPIN2,
	NRF_USBD_EVENT_ENDEPIN3,
	NRF_USBD_EVENT_ENDEPIN4,
	NRF_USBD_EVENT_ENDEPIN5,
	NRF_USBD_EVENT_ENDEPIN6,
	NRF_USBD_EVENT_ENDEPIN7,
	NRF_USBD_EVENT_EP0DATADONE,
	NRF_USBD_EVENT_ENDISOIN0,
	NRF_USBD_EVENT_ENDEPOUT0,
	NRF_USBD_EVENT_ENDEPOUT1,
	NRF_USBD_EVENT_ENDEPOUT2,
	NRF_USBD_EVENT_ENDEPOUT3,
	NRF_USBD_EVENT_ENDEPOUT4,
	NRF_USBD_EVENT_ENDEPOUT5,
	NRF_USBD_EVENT_ENDEPOUT6,
	NRF_USBD_EVENT_ENDEPOUT7,
	NRF_USBD_EVENT_ENDISOOUT0,
	NRF_USBD_EVENT_SOF,
	NRF_USBD_EVENT_USBEVENT,
	NRF_USBD_EVENT_EP0SETUP,
	NRF_USBD_EVENT_DATAEP,
};

static void usb_reset_handler(u32_t pos)
{
	struct nrf5_usbd_ctx *ctx = get_usbd_ctx();

	ARG_UNUSED(pos);

	ctx->status_code = USB_DC_RESET;
	ctx->flags |= BIT(NRF5_USB_STATUS_CHANGE);
}

static void usb_started_handler(u32_t pos)
{
	u32_t epstatus = nrf_usbd_epstatus_get_and_clear();
	struct ep_usb_event *ev;

	ARG_UNUSED(pos);

	ev = alloc_ep_usb_event();
	ev->ep = epstatus_to_ep_ctx(epstatus);
	ev->evt = EP_DMA_START;

	enqueue_ep_usb_event(ev);
}

static void end_epin_handler(u32_t pos)
{
	struct ep_usb_event *ev;
	u8_t ep;

	ep = NRF_USBD_EPIN(pos - (u8_t)USBD_INTEN_ENDEPIN0_Pos);

	ev = alloc_ep_usb_event();
	ev->ep = endpoint_ctx(ep);
	ev->evt = EP_DMA_END;

	enqueue_ep_usb_event(ev);
}

static void ep0_datadone_handler(u32_t pos)
{
	struct ep_usb_event *ev;
	u8_t ep = NRF_USBD_EPIN(0);

	ARG_UNUSED(pos);

	ev = alloc_ep_usb_event();
	ev->ep = endpoint_ctx(ep);

	/* See if this is IN or OUT event */
	if (ev->ep->state == EP_DATA) {
		ev->evt = EP_WRITE_COMPLETE;
	} else {
		ep = NRF_USBD_EPOUT(0);

		ev->ep = endpoint_ctx(ep);
		/**
		 * This is just an indication of OUT data received into
		 * local buffer. We need to trigger DMA in work handler.
		 */
		ev->evt = EP_DATA_RECV;
	}

	enqueue_ep_usb_event(ev);
}

static void end_isoin_handler(u32_t pos)
{
	struct ep_usb_event *ev;
	u8_t ep;

	ARG_UNUSED(pos);
	ep = NRF_USBD_EPIN(NRF_USBD_EPISO_FIRST);

	ev = alloc_ep_usb_event();
	ev->ep = endpoint_ctx(ep);
	ev->evt = EP_DMA_END;

	enqueue_ep_usb_event(ev);
}

static void end_epout_handler(u32_t pos)
{
	struct ep_usb_event *ev;
	u8_t ep;

	ep = NRF_USBD_EPOUT(pos - (u8_t)USBD_INTEN_ENDEPOUT0_Pos);

	ev = alloc_ep_usb_event();
	ev->ep = endpoint_ctx(ep);
	ev->ep->buf.len = nrf_usbd_ep_amount_get(ep);
	ev->evt = EP_DMA_END;

	ev->misc_u.flags |= BIT(EP_OUT_DATA_RCVD);

	enqueue_ep_usb_event(ev);
}

static void end_isoout_handler(u32_t pos)
{
	struct ep_usb_event *ev;
	u8_t ep;

	ARG_UNUSED(pos);

	/* TODO: Handle the CRC error case here */
	ep = NRF_USBD_EPOUT(NRF_USBD_EPISO_FIRST);

	ev = alloc_ep_usb_event();
	ev->ep = endpoint_ctx(ep);
	ev->ep->buf.len = nrf_usbd_ep_amount_get(ep);
	ev->evt = EP_DMA_END;

	enqueue_ep_usb_event(ev);
}

static void sof_handler(u32_t pos)
{
	struct ep_usb_event *ev;

	ARG_UNUSED(pos);

	ev = alloc_ep_usb_event();
	/* TODO: How to know if this is for ISOIN or ISOOUT? */
	ev->ep = NULL;
	ev->evt = EP_SOF;
	ev->misc_u.frame_counter = nrf_usbd_framecntr_get();

	enqueue_ep_usb_event(ev);
}

static void usb_event_handler(u32_t pos)
{
	struct nrf5_usbd_ctx *ctx = get_usbd_ctx();
	u32_t eventcause;

	ARG_UNUSED(pos);
	eventcause = nrf_usbd_eventcause_get_and_clear();

	if (eventcause & NRF_USBD_EVENTCAUSE_READY_MASK) {
		/* TODO: Should we mark this somewhere? */
	} else if (eventcause & NRF_USBD_EVENTCAUSE_ISOOUTCRC_MASK) {
		/* TODO: Handle the CRC error case properly */
	} else if (eventcause & NRF_USBD_EVENTCAUSE_SUSPEND_MASK) {
		/* TODO: Add protection to discard write during suspend */
		ctx->status_code = USB_DC_SUSPEND;
		ctx->flags |= BIT(NRF5_USB_STATUS_CHANGE);
	} else if (eventcause & NRF_USBD_EVENTCAUSE_RESUME_MASK) {
		/* TODO: Take care of resume properly */
		ctx->status_code = USB_DC_RESUME;
		ctx->flags |= BIT(NRF5_USB_STATUS_CHANGE);
	}
}

static void ep0setup_handler(u32_t pos)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;
	struct usb_setup_packet *setup;
	struct ep_usb_event *ev;
	u8_t ep = NRF_USBD_EPOUT(0);

	ARG_UNUSED(pos);

	ep_ctx = endpoint_ctx(ep);

	if (ep_ctx->buf.len) {
		/**
		 * Pending device stack read
		 * This is happening during SET ADDRESS.
		 * From what it seems, the HW completes the STATUS stage for
		 * SET ADDR transaction even before software processes it.
		 * TODO: Find out if we can avoid this.
		 */
		return;
	}

	setup = (struct usb_setup_packet *)ep_ctx->buf.data;

	setup->bmRequestType = nrf_usbd_setup_bmrequesttype_get();
	setup->bRequest = nrf_usbd_setup_brequest_get();
	setup->wValue = nrf_usbd_setup_wvalue_get();
	setup->wIndex = nrf_usbd_setup_windex_get();
	setup->wLength = nrf_usbd_setup_wlength_get();

	ep_ctx->buf.len = sizeof(*setup);

	ev = alloc_ep_usb_event();
	ev->ep = ep_ctx;
	ev->evt = EP_SETUP_RECV;

	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_DEVICE) {
		if (setup->wLength) {
			ev->misc_u.flags |= BIT(EP_CONTROL_WRITE);
		} else {
			ev->misc_u.flags |= BIT(EP_CONTROL_WRITE_NO_DATA);
		}
	} else {
		ev->misc_u.flags |= BIT(EP_CONTROL_READ);
	}

	enqueue_ep_usb_event(ev);
}

static void epdata_handler(u32_t pos)
{
	struct ep_usb_event *ev;
	u32_t epdatastatus;

	ARG_UNUSED(pos);
	epdatastatus = nrf_usbd_epdatastatus_get_and_clear();

	ev = alloc_ep_usb_event();
	ev->ep = epdatastatus_to_ep_ctx(epdatastatus);
	if (NRF_USBD_EPIN_CHECK(ev->ep->cfg.addr)) {
		ev->evt = EP_WRITE_COMPLETE;
	} else {
		ev->evt = EP_DATA_RECV;
	}

	ev->misc_u.flags = 0;

	enqueue_ep_usb_event(ev);
}

typedef void (*isr_event_handler_t)(u32_t pos);

static const isr_event_handler_t isr_event_handlers[USBD_INT_CNT] = {
	usb_reset_handler,
	usb_started_handler,
	end_epin_handler,
	end_epin_handler,
	end_epin_handler,
	end_epin_handler,
	end_epin_handler,
	end_epin_handler,
	end_epin_handler,
	end_epin_handler,
	ep0_datadone_handler,
	end_isoin_handler,
	end_epout_handler,
	end_epout_handler,
	end_epout_handler,
	end_epout_handler,
	end_epout_handler,
	end_epout_handler,
	end_epout_handler,
	end_epout_handler,
	end_isoout_handler,
	sof_handler,
	usb_event_handler,
	ep0setup_handler,
	epdata_handler,
};

/** Process interrupts that are enabled */
static u32_t process_interrupts(u32_t mask)
{
	u32_t processed = 0, pos = 0;
	isr_event_handler_t evt_hdlr;

	while (pos < USBD_INT_CNT) {
		if (!(mask & BIT(pos))) {
			pos++;
			continue;
		}

		if (nrf_usbd_event_check(event_map[pos])) {
			nrf_usbd_event_clear(event_map[pos]);

			evt_hdlr = isr_event_handlers[pos];
			if (evt_hdlr) {
				evt_hdlr(pos);
				processed++;
			}
		}
		pos++;
	}

	return processed;
}

/**
 * @brief Interrupt service routine.
 *
 * This handles the USBD interrupt
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void usbd_isr_handler(void *arg)
{
	struct nrf5_usbd_ctx *ctx = arg;

	if (!ctx->enabled) {
		return;
	}

	irq_disable(CONFIG_USBD_NRF5_IRQ);

	if (process_interrupts(ctx->enable_mask)) {
		if (!k_work_pending(&ctx->usb_work)) {
			k_work_submit(&ctx->usb_work);
		}
	}

	irq_enable(CONFIG_USBD_NRF5_IRQ);
}

/**
 * @brief Install the Interrupt service routine.
 *
 * This installs the interrupt service routine and enables the USBD interrupt.
 *
 * @return N/A
 */
static void usbd_install_isr(void)
{
	IRQ_CONNECT(CONFIG_USBD_NRF5_IRQ,
		    CONFIG_USBD_NRF5_IRQ_PRI,
		    usbd_isr_handler, &usbd_ctx, 0);
}

static void usbd_enable_interrupts(void)
{
	struct nrf5_usbd_ctx *ctx = get_usbd_ctx();

	ctx->enable_mask = NRF_USBD_INT_USBRESET_MASK |
			   NRF_USBD_INT_USBEVENT_MASK |
			   NRF_USBD_INT_DATAEP_MASK;

	nrf_usbd_int_enable(ctx->enable_mask);
	ctx->enabled = true;

	irq_enable(CONFIG_USBD_NRF5_IRQ);
}

static void usbd_disable_interrupts(void)
{
	struct nrf5_usbd_ctx *ctx = get_usbd_ctx();

	nrf_usbd_int_disable(~0);
	ctx->enabled = false;
	irq_disable(CONFIG_USBD_NRF5_IRQ);
}

void nrf5_usbd_power_event_callback(nrf_power_event_t event)
{
	struct nrf5_usbd_ctx *ctx = get_usbd_ctx();

	switch (event) {
	case NRF_POWER_EVENT_USBDETECTED:
		ctx->state = USBD_ATTACHED;
		break;
	case NRF_POWER_EVENT_USBPWRRDY:
		ctx->state = USBD_POWERED;
		break;
	case NRF_POWER_EVENT_USBREMOVED:
		ctx->state = USBD_DETACHED;
		break;
	default:
		SYS_LOG_DBG("Unknown USB event");
		return;
	}

	ctx->flags |= BIT(NRF5_USB_STATE_CHANGE);
	k_work_submit(&ctx->usb_work);
}

/**
 * @brief Enable/Disable the HF clock
 *
 * Toggle the HF clock. It needs to be enabled for USBD data exchange
 *
 * @param on		Set true to enable the HF clock, false to disable.
 * @param blocking	Set true to block wait till HF clock stabilizes.
 *
 * @return 0 on success, error number otherwise
 */
static int hf_clock_enable(bool on, bool blocking)
{
	int ret = -ENODEV;
	struct device *clock;

	clock = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_M16SRC_DRV_NAME);
	if (!clock) {
		SYS_LOG_ERR("NRF5 HF Clock device not found!");
		return ret;
	}

	if (on) {
		ret = clock_control_on(clock, (void *)blocking);
	} else {
		ret = clock_control_off(clock, (void *)blocking);
	}

	if (ret && (blocking || (ret != -EINPROGRESS))) {
		SYS_LOG_ERR("NRF5 HF clock %s fail: %d",
			    on ? "start" : "stop", ret);
		return ret;
	}

	SYS_LOG_DBG("HF clock %s success (%d)", on ? "start" : "stop", ret);

	return ret;
}

static void usbd_enable_endpoints(struct nrf5_usbd_ctx *ctx)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;
	int i;

	for (i = 0; i < NRF_USBD_EPIN_CNT; i++) {
		ep_ctx = in_endpoint_ctx(i);
		__ASSERT_NO_MSG(ep_ctx);

		if (ep_ctx->cfg.en) {
			nrf_usbd_ep_enable(ep_ctx->cfg.addr);
			cfg_ep_interrupt(ep_ctx->cfg.addr, true);
		}
	}

	for (i = 0; i < NRF_USBD_EPOUT_CNT; i++) {
		ep_ctx = out_endpoint_ctx(i);
		__ASSERT_NO_MSG(ep_ctx);

		if (ep_ctx->cfg.en) {
			nrf_usbd_ep_enable(ep_ctx->cfg.addr);
			cfg_ep_interrupt(ep_ctx->cfg.addr, true);
			nrf_usbd_epout_clear(ep_ctx->cfg.addr);
		}
	}
}

static void usbd_handle_state_change(struct nrf5_usbd_ctx *ctx)
{
	switch (ctx->state) {
	case USBD_ATTACHED:
		SYS_LOG_DBG("USB detected");
		nrf_usbd_enable();
		break;
	case USBD_POWERED:
		SYS_LOG_DBG("USB Powered");
		ctx->status_code = USB_DC_CONNECTED;
		ctx->flags |= BIT(NRF5_USB_STATUS_CHANGE);
		usbd_enable_endpoints(ctx);
		nrf_usbd_pullup_enable();
		ctx->ready = true;
		break;
	case USBD_DETACHED:
		SYS_LOG_DBG("USB Removed");
		if (nrf_usbd_pullup_check()) {
			nrf_usbd_pullup_disable();
			ctx->ready = false;
		}
		nrf_usbd_disable();
		ctx->status_code = USB_DC_DISCONNECTED;
		ctx->flags |= BIT(NRF5_USB_STATUS_CHANGE);
		break;
	default:
		SYS_LOG_ERR("Unknown USB state");
	}

	if (ctx->flags) {
		k_work_submit(&ctx->usb_work);
	}
}

static void usbd_handle_status_change(struct nrf5_usbd_ctx *ctx)
{
	if (ctx->status_cb) {
		ctx->status_cb(ctx->status_code, NULL);
	}
}

/* Control read/write: Handle the HW events during IDLE state */
static inline void handle_ctrl_ep_idle_state_events(struct ep_usb_event *ev)
{
	struct nrf5_usbd_ep_ctx *ep_ctx = ev->ep;

	switch (ev->evt) {
	case EP_SETUP_RECV:
		if (ev->misc_u.flags & BIT(EP_CONTROL_READ)) {
			struct nrf5_usbd_ep_ctx *epin0;

			/**
			 * SETUP packet comes through CTRL EPOUT0
			 * and control read data stage happens in CTRL
			 * EPIN0. So, go back to IDLE state in EPOUT0.
			 */
			ep_ctx->state = EP_IDLE;

			/* Prepare EPIN0 for DATA stage */
			epin0 = endpoint_ctx(NRF_USBD_EPIN(0));
			epin0->state = EP_DATA;
		} else if (ev->misc_u.flags & BIT(EP_CONTROL_WRITE)) {
			ep_ctx->state = EP_SETUP;

			/**
			 * Initiate reception of EP0 OUT DATA to USBD
			 * local buffer
			 */
			start_ep0rcvout_task();
		} else if (ev->misc_u.flags & BIT(EP_CONTROL_WRITE_NO_DATA)) {
			/* No DATA stage. Be in IDLE state. Redundant. */
			ep_ctx->state = EP_IDLE;
		}

		/* Inform the stack */
		ep_ctx->cfg.cb(ep_ctx->cfg.addr, USB_DC_EP_SETUP);
		break;

	case EP_DATA_RECV:
	case EP_DMA_START:
	case EP_DMA_END:
	case EP_WRITE_COMPLETE:
	case EP_SOF:
		SYS_LOG_ERR("invalid event %d in data state for EP %d",
			    ev->evt, ep_ctx->cfg.addr);
		__ASSERT_NO_MSG(0);
		break;
	}
}

/* Control read/write: Handle the HW events during SETUP stage */
static inline void handle_ctrl_ep_setup_state_events(struct ep_usb_event *ev)
{
	struct nrf5_usbd_ep_ctx *ep_ctx = ev->ep;

	switch (ev->evt) {
	case EP_DATA_RECV:
	{
		struct nrf5_usbd_ctx *ctx = get_usbd_ctx();

		/**
		 * We have received OUT data on EPOUT0 which is in USBD's local
		 * buffer. Grab it into endpoint's buffer with EasyDMA.
		 */
		nrf_usbd_ep_easydma_set(ep_ctx->cfg.addr,
					(u32_t)ep_ctx->buf.data,
					ep_ctx->buf.len);

		/* Only one DMA operation can happen at a time */
		k_sem_take(&ctx->dma_in_use, K_FOREVER);

		start_epout_task(ep_ctx->cfg.addr);

		/* Move to DATA stage */
		ep_ctx->state = EP_DATA;
	}
		break;
	case EP_SETUP_RECV:
	case EP_DMA_START:
	case EP_DMA_END:
	case EP_WRITE_COMPLETE:
	case EP_SOF:
		SYS_LOG_ERR("invalid event %d in setup state", ev->evt);
		__ASSERT_NO_MSG(0);
		break;
	}
}

/* Control read/write: Handle the HW events during data stage */
static inline void handle_ctrl_ep_data_state_events(struct ep_usb_event *ev)
{
	struct nrf5_usbd_ctx *ctx = get_usbd_ctx();
	struct nrf5_usbd_ep_ctx *ep_ctx = ev->ep;

	switch (ev->evt) {
	case EP_DMA_START:
		/* Nothing much to do here */
		break;
	case EP_DMA_END:
		/**
		 * EasyDMA can now be used by other threads
		 * possibly waiting for DMA time.
		 */
		k_sem_give(&ctx->dma_in_use);

		/**
		 * If this is an OUT data indication, indicate the device stack
		 * to read from the endpoint buffer.
		 */
		if (ev->misc_u.flags & BIT(EP_OUT_DATA_RCVD)) {
			if (ep_ctx->buf.len < ep_ctx->cfg.max_sz) {
				/**
				 * ZLP or short packet? Initiate STATUS stage
				 * TODO: Should we do this here or in read()?
				 */
				start_ep0status_task();
				ep_ctx->state = EP_IDLE;
			} else {
				/* We have some more OUT data to receive */
				/* TODO: What if it's exact multiple of MPS? */
				start_ep0rcvout_task();
			}

			/* Inform the device stack */
			ep_ctx->cfg.cb(ep_ctx->cfg.addr, USB_DC_EP_DATA_OUT);
		}
		break;
	case EP_WRITE_COMPLETE:
		/**
		 * ZLP or short packet? Initiate STATUS stage
		 * TODO: Should we do this here or during write (0)?
		 * TODO: What if the data is actually a multiple of max_sz??
		 */
		if (ep_ctx->buf.len < ep_ctx->cfg.max_sz) {
			start_ep0status_task();
			ep_ctx->state = EP_IDLE;
		}

		/* Indicate write completion to the device stack */
		ep_ctx->cfg.cb(ep_ctx->cfg.addr, USB_DC_EP_DATA_IN);
		break;
	case EP_DATA_RECV:
		/**
		 * We have received OUT data on EPOUT0.
		 * Grab it into endpoint buffer with EasyDMA.
		 */
		nrf_usbd_ep_easydma_set(ep_ctx->cfg.addr,
					(u32_t)ep_ctx->buf.data,
					ep_ctx->buf.len);

		/* Only one DMA can happen at a time */
		k_sem_take(&ctx->dma_in_use, K_FOREVER);
		start_epout_task(ep_ctx->cfg.addr);
		break;
	case EP_SETUP_RECV:
	case EP_SOF:
		SYS_LOG_ERR("invalid event %d in data state for EP %d",
			    ev->evt, ep_ctx->cfg.addr);
		__ASSERT_NO_MSG(0);
		break;
	}
}

/* Handle all control endpoint events */
static void handle_ctrl_ep_event(struct ep_usb_event *ev)
{
	struct nrf5_usbd_ep_ctx *ep_ctx = ev->ep;

	switch (ep_ctx->state) {
	case EP_IDLE:
		handle_ctrl_ep_idle_state_events(ev);
		break;
	case EP_SETUP:
		handle_ctrl_ep_setup_state_events(ev);
		break;
	case EP_DATA:
		handle_ctrl_ep_data_state_events(ev);
		break;
	case EP_STATUS:
		/**
		 * TODO: HW doesn't indicate STATUS stage completion to SW.
		 * Do we even need STATUS state?
		 */
		break;
	}
}

/* Data IN/OUT: Handle the HW events during IDLE state */
static inline void handle_data_ep_idle_state_events(struct ep_usb_event *ev)
{
	struct nrf5_usbd_ep_ctx *ep_ctx = ev->ep;

	switch (ev->evt) {
	case EP_DMA_START:
		/**
		 * For IN endpoints, application has initiated a write.
		 * For OUT endpoints work handler must've initiated EPOUT task
		 * Either way, move the state to DATA.
		 */
		ep_ctx->state = EP_DATA;
		break;
	case EP_DATA_RECV:
	{
		struct nrf5_usbd_ctx *ctx = get_usbd_ctx();
		u8_t addr = ep_ctx->cfg.addr;

		/**
		 * We have some OUT BULK/INTERRUT data received
		 * into the USBD's local buffer. Let's grab it.
		 */
		nrf_usbd_ep_easydma_set(addr, (u32_t)ep_ctx->buf.data,
					nrf_usbd_epout_size_get(addr));

		/* Only one DMA can happen at a time */
		k_sem_take(&ctx->dma_in_use, K_FOREVER);
		start_epout_task(addr);
	}
		break;
	case EP_WRITE_COMPLETE:
	case EP_DMA_END:
	case EP_SETUP_RECV:
	case EP_SOF:
		SYS_LOG_ERR("invalid event %d in idle state", ev->evt);
		__ASSERT_NO_MSG(0);
		break;
	}
}

/* Data IN/OUT: Handle the HW events during DATA state */
static inline void handle_data_ep_data_state_events(struct ep_usb_event *ev)
{
	struct nrf5_usbd_ep_ctx *ep_ctx = ev->ep;

	switch (ev->evt) {
	case EP_DMA_END:
	{
		struct nrf5_usbd_ctx *ctx = get_usbd_ctx();

		/**
		 * EasyDMA can now be used by other threads
		 * possibly waiting for DMA time.
		 */
		if (NRF_USBD_EPIN_CHECK(ep_ctx->cfg.addr)) {
			ep_ctx->flags |= BIT(EP_WRITE_PENDING);
		}
		k_sem_give(&ctx->dma_in_use);

		if (NRF_USBD_EPOUT_CHECK(ep_ctx->cfg.addr)) {
			/**
			 * We've received the OUT data in the endpoint buffer
			 * Inform upper layer.
			 */
			ep_ctx->cfg.cb(ep_ctx->cfg.addr, USB_DC_EP_DATA_OUT);

			/**
			 * Move back to IDLE state
			 * TODO: Should we do this in read() instead?
			 */
			ep_ctx->state = EP_IDLE;
		}
	}
		break;
	case EP_WRITE_COMPLETE:
	{
		ep_ctx->flags &= ~BIT(EP_WRITE_PENDING);

		/* Inform stack and go back to IDLE state */
		ep_ctx->cfg.cb(ep_ctx->cfg.addr, USB_DC_EP_DATA_IN);
		ep_ctx->state = EP_IDLE;
	}
		break;
	case EP_DATA_RECV:
	case EP_DMA_START:
	case EP_SETUP_RECV:
	case EP_SOF:
		SYS_LOG_ERR("invalid event %d in data state", ev->evt);
		__ASSERT_NO_MSG(0);
		break;
	}
}

/* Handle all data (Bulk/Interrupt) endpoint events */
static void handle_data_ep_event(struct ep_usb_event *ev)
{
	struct nrf5_usbd_ep_ctx *ep_ctx = ev->ep;

	switch (ep_ctx->state) {
	case EP_IDLE:
		handle_data_ep_idle_state_events(ev);
		break;
	case EP_DATA:
		handle_data_ep_data_state_events(ev);
		break;
	case EP_SETUP:
	case EP_STATUS:
		SYS_LOG_ERR("invalid state(%d) for data ep %d",
			    ep_ctx->state, ep_ctx->cfg.addr);
		break;
	}
}

/* ISO IN/OUT: Handle the HW events during IDLE state - WIP */
static inline void handle_iso_ep_idle_state_events(struct ep_usb_event *ev)
{
	struct nrf5_usbd_ep_ctx *ep_ctx = ev->ep;

	switch (ev->evt) {
	case EP_SOF:
	{
		struct nrf5_usbd_ctx *ctx = get_usbd_ctx();
		u8_t addr = ep_ctx->cfg.addr;

		if (NRF_USBD_EPOUT_CHECK(ep_ctx->cfg.addr)) {
			/**
			 * TODO: We should check the ISOCRC USB flag here and
			 * discard the whole data after DMA if need be. USBD
			 * will transfer the OUT data anyway if DMA is set up.
			 */

			/** Is buffer available? */
			if (!ep_ctx->buf.len) {
				u32_t maxcnt;

				maxcnt = nrf_usbd_episoout_size_get(addr);
				nrf_usbd_ep_easydma_set(addr,
							(u32_t)ep_ctx->buf.data,
							maxcnt);

				/* Only one DMA can happen at a time */
				k_sem_take(&ctx->dma_in_use, K_FOREVER);
				start_epout_task(addr);
			}
		} else {
			/**
			 * Have anything in the ISO IN buffer to write to
			 * USB bus?
			 */
			if (ep_ctx->buf.len) {
				nrf_usbd_ep_easydma_set(addr,
							(u32_t)ep_ctx->buf.data,
							ep_ctx->cfg.max_sz);

				/**
				 * TODO: What if we are stuck in this work
				 * handler forever?
				 * Because, a DMA done event would be waiting
				 * in the FIFO forever. Should we instead give
				 * the sem back in ISR?
				 */
				k_sem_take(&ctx->dma_in_use, K_FOREVER);
				start_epin_task(addr);
			}
		}
	}
		break;
	case EP_DMA_START:
		/**
		 * For IN endpoints, application has initiated a write.
		 * For OUT endpoints work handler must've initiated EPOUT task
		 * Either way, move the state to DATA.
		 */
		ep_ctx->state = EP_DATA;
		break;
	case EP_WRITE_COMPLETE:
	case EP_DMA_END:
	case EP_SETUP_RECV:
	case EP_DATA_RECV:
		SYS_LOG_ERR("invalid event %d in idle state", ev->evt);
		__ASSERT_NO_MSG(0);
		break;
	}
}

/* ISO IN/OUT: Handle the HW events during DATA state */
static inline void handle_iso_ep_data_state_events(struct ep_usb_event *ev)
{
	struct nrf5_usbd_ep_ctx *ep_ctx = ev->ep;

	switch (ev->evt) {
	case EP_DMA_END:
		if (NRF_USBD_EPOUT_CHECK(ep_ctx->cfg.addr)) {
			/**
			 * We've received ISO OUT data. Indicate upper layer.
			 * Usually the buffer should be consumed all at once.
			 */
			ep_ctx->cfg.cb(ep_ctx->cfg.addr, USB_DC_EP_DATA_OUT);
		} else {
			/**
			 * This is not an actual write complete perse.
			 * An implicit write complete callback. So to speak.
			 */
			ep_ctx->cfg.cb(ep_ctx->cfg.addr, USB_DC_EP_DATA_IN);
		}

		/* Move back to IDLE state */
		ep_ctx->state = EP_IDLE;
		break;
	case EP_DATA_RECV:
	case EP_SOF:
	case EP_DMA_START:
	case EP_WRITE_COMPLETE:
	case EP_SETUP_RECV:
		SYS_LOG_ERR("invalid event %d in idle state", ev->evt);
		__ASSERT_NO_MSG(0);
		break;
	}
}

/* Handle all isochronous endpoint events - WIP */
static void handle_iso_ep_event(struct ep_usb_event *ev)
{
	struct nrf5_usbd_ep_ctx *ep_ctx = ev->ep;

	/**
	 * TODO:
	 * How to differentiate an SOF between IN and OUT endpoints?
	 * Right now we don't have the endpoint context set in ISR.
	 * So, just don't handle it for now.
	 */
	if (!ep_ctx) {
		return;
	}

	switch (ep_ctx->state) {
	case EP_IDLE:
		handle_iso_ep_idle_state_events(ev);
		break;
	case EP_DATA:
		handle_iso_ep_data_state_events(ev);
		break;
	case EP_SETUP:
	case EP_STATUS:
		SYS_LOG_ERR("invalid state(%d) for a iso ep %d",
			    ep_ctx->state, ep_ctx->cfg.addr);
		break;
	}
}

/* Work handler */
static void usbd_work_handler(struct k_work *item)
{
	struct nrf5_usbd_ctx *ctx;
	struct ep_usb_event *ev;

	k_sched_lock();

	ctx = CONTAINER_OF(item, struct nrf5_usbd_ctx, usb_work);

	if (ctx->flags) {
		irq_disable(NRF5_IRQ_POWER_CLOCK_IRQn);

		if (ctx->flags & BIT(NRF5_USB_STATE_CHANGE)) {
			usbd_handle_state_change(ctx);
			ctx->flags &= ~BIT(NRF5_USB_STATE_CHANGE);
		}

		if (ctx->flags & BIT(NRF5_USB_STATUS_CHANGE)) {
			usbd_handle_status_change(ctx);
			ctx->flags &= ~BIT(NRF5_USB_STATUS_CHANGE);
		}

		irq_enable(NRF5_IRQ_POWER_CLOCK_IRQn);
	}

	irq_disable(CONFIG_USBD_NRF5_IRQ);

	while ((ev = dequeue_ep_usb_event()) != NULL) {
		struct nrf5_usbd_ep_ctx *ep_ctx = ev->ep;

		if (ep_ctx) {
			switch (ep_ctx->cfg.type) {
			case USB_DC_EP_CONTROL:
				handle_ctrl_ep_event(ev);
				break;
			case USB_DC_EP_BULK:
			case USB_DC_EP_INTERRUPT:
				handle_data_ep_event(ev);
				break;
			case USB_DC_EP_ISOCHRONOUS:
				handle_iso_ep_event(ev);
				break;
			}
		}

		free_ep_usb_event(ev);
	}

	irq_enable(CONFIG_USBD_NRF5_IRQ);

	k_sched_unlock();
}

static inline bool dev_attached(void)
{
	return get_usbd_ctx()->attached;
}

static inline bool dev_ready(void)
{
	return get_usbd_ctx()->ready;
}

static void endpoint_ctx_init(void)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;
	int ret;
	u32_t i;

	for (i = 0; i < CFG_EPIN_CNT; i++) {
		ep_ctx = in_endpoint_ctx(i);
		__ASSERT_NO_MSG(ep_ctx);

		ret = k_mem_pool_alloc(&ep_buf_pool, &ep_ctx->buf.block,
				       MAX_EP_BUF_SZ, K_NO_WAIT);
		if (ret < 0) {
			SYS_LOG_ERR("EP buffer alloc failed for EPIN%d", i);
			__ASSERT_NO_MSG(0);
		}

		ep_ctx->buf.data = ep_ctx->buf.block.data;
		ep_ctx->buf.curr = ep_ctx->buf.data;
	}

	for (i = 0; i < CFG_EPOUT_CNT; i++) {
		ep_ctx = out_endpoint_ctx(i);
		__ASSERT_NO_MSG(ep_ctx);

		ret = k_mem_pool_alloc(&ep_buf_pool, &ep_ctx->buf.block,
				       MAX_EP_BUF_SZ, K_NO_WAIT);
		if (ret < 0) {
			SYS_LOG_ERR("EP buffer alloc failed for EPOUT%d", i);
			__ASSERT_NO_MSG(0);
		}

		ep_ctx->buf.data = ep_ctx->buf.block.data;
		ep_ctx->buf.curr = ep_ctx->buf.data;
	}

	if (CFG_EP_ISOIN_CNT) {
		ep_ctx = in_endpoint_ctx(NRF_USBD_EPIN(8));
		__ASSERT_NO_MSG(ep_ctx);

		ret = k_mem_pool_alloc(&ep_buf_pool, &ep_ctx->buf.block,
				       MAX_ISO_EP_BUF_SZ, K_NO_WAIT);
		if (ret < 0) {
			SYS_LOG_ERR("EP buffer alloc failed for ISOIN");
			__ASSERT_NO_MSG(0);
		}

		ep_ctx->buf.data = ep_ctx->buf.block.data;
		ep_ctx->buf.curr = ep_ctx->buf.data;
	}

	if (CFG_EP_ISOOUT_CNT) {
		ep_ctx = out_endpoint_ctx(NRF_USBD_EPOUT(8));
		__ASSERT_NO_MSG(ep_ctx);

		ret = k_mem_pool_alloc(&ep_buf_pool, &ep_ctx->buf.block,
				       MAX_ISO_EP_BUF_SZ, K_NO_WAIT);
		if (ret < 0) {
			SYS_LOG_ERR("EP buffer alloc failed for ISOOUT");
			__ASSERT_NO_MSG(0);
		}

		ep_ctx->buf.data = ep_ctx->buf.block.data;
		ep_ctx->buf.curr = ep_ctx->buf.data;
	}
}

static void endpoint_ctx_deinit(void)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;
	u32_t i;

	for (i = 0; i < CFG_EPIN_CNT; i++) {
		ep_ctx = in_endpoint_ctx(i);
		__ASSERT_NO_MSG(ep_ctx);
		k_mem_pool_free(&ep_ctx->buf.block);
		(void)memset(ep_ctx, 0, sizeof(*ep_ctx));
	}

	for (i = 0; i < CFG_EPOUT_CNT; i++) {
		ep_ctx = out_endpoint_ctx(i);
		__ASSERT_NO_MSG(ep_ctx);
		k_mem_pool_free(&ep_ctx->buf.block);
		(void)memset(ep_ctx, 0, sizeof(*ep_ctx));
	}

	if (CFG_EP_ISOIN_CNT) {
		ep_ctx = in_endpoint_ctx(NRF_USBD_EPIN(8));
		__ASSERT_NO_MSG(ep_ctx);
		k_mem_pool_free(&ep_ctx->buf.block);
		(void)memset(ep_ctx, 0, sizeof(*ep_ctx));
	}

	if (CFG_EP_ISOOUT_CNT) {
		ep_ctx = out_endpoint_ctx(NRF_USBD_EPOUT(8));
		__ASSERT_NO_MSG(ep_ctx);
		k_mem_pool_free(&ep_ctx->buf.block);
		(void)memset(ep_ctx, 0, sizeof(*ep_ctx));
	}
}

static int usbd_power_int_enable(bool enable)
{
	struct device *dev = device_get_binding(CONFIG_USBD_NRF5_NAME);

	if (!dev) {
		SYS_LOG_ERR("could not get USBD power device binding");
		return -ENODEV;
	}

	nrf5_power_usb_power_int_enable(dev, enable);

	return 0;
}

int usb_dc_attach(void)
{
	struct nrf5_usbd_ctx *ctx = get_usbd_ctx();
	int ret;

	if (ctx->attached) {
		return 0;
	}

	k_work_init(&ctx->usb_work, usbd_work_handler);
	k_fifo_init(&ctx->work_queue);
	k_sem_init(&ctx->dma_in_use, 1, 1);

	ret = usbd_power_int_enable(true);
	if (ret) {
		return ret;
	}

	usbd_install_isr();
	usbd_enable_interrupts();

	/* NOTE: Non-blocking HF clock enable can return -EINPROGRESS if HF
	 * clock start was already requested.
	 */
	ret = hf_clock_enable(true, false);
	if (ret && ret != -EINPROGRESS) {
		goto err_clk_enable;
	}

	endpoint_ctx_init();
	ctx->attached = true;

	return 0;

err_clk_enable:
	usbd_disable_interrupts();
	usbd_power_int_enable(false);

	return ret;
}

int usb_dc_detach(void)
{
	struct nrf5_usbd_ctx *ctx = get_usbd_ctx();
	int ret;

	usbd_disable_interrupts();

	if (nrf_usbd_pullup_check()) {
		nrf_usbd_pullup_disable();
	}

	nrf_usbd_disable();

	ret = hf_clock_enable(false, false);
	if (ret) {
		return ret;
	}

	ret = usbd_power_int_enable(false);
	if (ret) {
		return ret;
	}

	ctx->flags = 0;
	ctx->state = USBD_DETACHED;
	ctx->status_code = USB_DC_UNKNOWN;

	flush_ep_usb_events();
	k_sem_reset(&ctx->dma_in_use);
	endpoint_ctx_deinit();

	ctx->attached = false;

	return ret;
}

int usb_dc_reset(void)
{
	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	/* TODO: Handle midway reset */

	return 0;
}

int usb_dc_set_address(const u8_t addr)
{
	struct nrf5_usbd_ctx *ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	/**
	 * Nothing to do here. The USBD HW already takes care of initiating
	 * STATUS stage. Just double check the address for sanity.
	 */
	__ASSERT(addr == (u8_t)NRF_USBD->USBADDR, "USB Address incorrect!");

	ctx = get_usbd_ctx();
	ctx->state = USBD_ADDRESS_SET;
	ctx->address_set = true;

	return 0;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	u8_t ep_idx = NRF_USBD_EP_NR_GET(cfg->ep_addr);

	SYS_LOG_DBG("ep %x, mps %d, type %d", cfg->ep_addr, cfg->ep_mps,
		    cfg->ep_type);

	if ((cfg->ep_type == USB_DC_EP_CONTROL) && ep_idx) {
		SYS_LOG_ERR("invalid endpoint configuration");
		return -1;
	}

	if (!NRF_USBD_EP_VALIDATE(cfg->ep_addr)) {
		SYS_LOG_ERR("invalid endpoint index/address");
		return -1;
	}

	if ((cfg->ep_type == USB_DC_EP_ISOCHRONOUS) &&
	    (!NRF_USBD_EPISO_CHECK(cfg->ep_addr))) {
		SYS_LOG_WRN("invalid endpoint type");
		return -1;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data * const ep_cfg)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;

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

	return 0;
}

int usb_dc_ep_set_stall(const u8_t ep)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	switch (ep_ctx->cfg.type) {
	case USB_DC_EP_CONTROL:
		start_ep0stall_task();
		break;
	case USB_DC_EP_BULK:
	case USB_DC_EP_INTERRUPT:
		nrf_usbd_ep_stall(ep);
		break;
	case USB_DC_EP_ISOCHRONOUS:
		SYS_LOG_ERR("STALL unsupported on ISO endpoints");
		return -EINVAL;
	}

	ep_ctx->state = EP_IDLE;
	ep_ctx->buf.len = 0;
	ep_ctx->buf.curr = ep_ctx->buf.data;
	ep_ctx->flags = 0;

	return 0;
}

int usb_dc_ep_clear_stall(const u8_t ep)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	nrf_usbd_ep_unstall(ep);

	return 0;
}

int usb_dc_ep_halt(const u8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

int usb_dc_ep_is_stalled(const u8_t ep, u8_t *const stalled)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	*stalled = (u8_t)nrf_usbd_ep_is_stall(ep);

	return 0;
}

int usb_dc_ep_enable(const u8_t ep)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;

	if (!dev_attached()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (ep_ctx->cfg.en) {
		return -EALREADY;
	}

	ep_ctx->cfg.en = true;

	/* Defer the endpoint enable if USBD is not ready yet */
	if (dev_ready()) {
		nrf_usbd_ep_enable(ep);
		cfg_ep_interrupt(ep, true);

		if (NRF_USBD_EPOUT_CHECK(ep)) {
			nrf_usbd_epout_clear(ep);
		}
	}

	return 0;
}

int usb_dc_ep_disable(const u8_t ep)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!ep_ctx->cfg.en) {
		return -EALREADY;
	}

	cfg_ep_interrupt(ep, false);
	nrf_usbd_ep_disable(ep);
	ep_ctx->cfg.en = false;

	return 0;
}

int usb_dc_ep_flush(const u8_t ep)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	ep_ctx->buf.len = 0;
	ep_ctx->buf.curr = ep_ctx->buf.data;
	ep_ctx->state = EP_IDLE;
	ep_ctx->flags = 0;

	switch (ep_ctx->cfg.type) {
	case USB_DC_EP_CONTROL:
		nrf_usbd_epout_clear(ep);
		break;
	case USB_DC_EP_BULK:
	case USB_DC_EP_INTERRUPT:
		nrf_usbd_epout_clear(ep);
		break;
	default:
		break;
	}

	return 0;
}

int usb_dc_ep_write(const u8_t ep, const u8_t *const data,
		    const u32_t data_len, u32_t * const ret_bytes)
{
	struct nrf5_usbd_ctx *ctx = get_usbd_ctx();
	struct nrf5_usbd_ep_ctx *ep_ctx;
	u32_t bytes_to_copy;

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

	/**
	 * TODO:
	 * With usb_write called multiple times over before the previous
	 * write is complete, there are data corruptions on the bus,
	 * possibly because software over-writes the HW's local buffer
	 * and initiates the next write before or while an ongoing
	 * USB write on the bus.
	 */
	if (ep_ctx->flags & BIT(EP_WRITE_PENDING)) {
		return -EAGAIN;
	}

	k_sem_take(&ctx->dma_in_use, K_FOREVER);

	bytes_to_copy = min(data_len, ep_ctx->cfg.max_sz);
	memcpy(ep_ctx->buf.data, data, bytes_to_copy);
	ep_ctx->buf.len = bytes_to_copy;

	if (ret_bytes) {
		*ret_bytes = bytes_to_copy;
	}

	nrf_usbd_ep_easydma_set(ep, (u32_t)ep_ctx->buf.data, ep_ctx->buf.len);

	switch (ep_ctx->cfg.type) {
	case USB_DC_EP_CONTROL:
	{
		if (bytes_to_copy) {
			start_epin_task(ep);
		} else {
			if (get_usbd_ctx()->address_set) {
				get_usbd_ctx()->address_set = false;
				/**
				 * TODO: For SET_ADDRESS, HW takes care of
				 * initiating STATUS but for some reason
				 * it looks like we need to start ep0status
				 * task. Clarify this!
				 */
				/* break; */
			}
			start_ep0status_task();
			k_sem_give(&ctx->dma_in_use);
			ep_ctx->state = EP_IDLE;
		}
	}
		break;
	case USB_DC_EP_BULK:
	case USB_DC_EP_INTERRUPT:
		/* TODO: Should we set EP_WRITE_PENDING here? */
		start_epin_task(ep);
		break;
	case USB_DC_EP_ISOCHRONOUS:
		start_epin_task(ep);
		break;
	default:
		SYS_LOG_ERR("Invalid endpoint type");
		break;
	}

	return 0;
}

int usb_dc_ep_read_wait(u8_t ep, u8_t *data, u32_t max_data_len,
			u32_t *read_bytes)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;
	u32_t bytes_to_copy;

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

	if (!data && !max_data_len) {
		if (read_bytes) {
			*read_bytes = ep_ctx->buf.len;
		}
		return 0;
	}

	bytes_to_copy = min(max_data_len, ep_ctx->buf.len);
	memcpy(data, ep_ctx->buf.curr, bytes_to_copy);
	ep_ctx->buf.curr += bytes_to_copy;
	ep_ctx->buf.len -= bytes_to_copy;

	if (read_bytes) {
		*read_bytes = bytes_to_copy;
	}

	return 0;
}

int usb_dc_ep_read_continue(u8_t ep)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;

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

	if (!ep_ctx->buf.len) {
		ep_ctx->buf.curr = ep_ctx->buf.data;

		if (ep_ctx->cfg.type == USB_DC_EP_BULK ||
		    ep_ctx->cfg.type == USB_DC_EP_INTERRUPT) {
			nrf_usbd_epout_clear(ep);
		}
	}

	return 0;
}

int usb_dc_ep_read(const u8_t ep, u8_t *const data,
		   const u32_t max_data_len, u32_t * const read_bytes)
{
	int ret;

	ret = usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes);
	if (ret) {
		return ret;
	}

	ret = usb_dc_ep_read_continue(ep);

	return ret;
}

int usb_dc_ep_set_callback(const u8_t ep, const usb_dc_ep_callback cb)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;

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

int usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	get_usbd_ctx()->status_cb = cb;

	return 0;
}

int usb_dc_ep_mps(const u8_t ep)
{
	struct nrf5_usbd_ep_ctx *ep_ctx;

	if (!dev_attached()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	return ep_ctx->cfg.max_sz;
}
