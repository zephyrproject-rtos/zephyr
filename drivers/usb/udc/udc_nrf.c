/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include <soc.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/dt-bindings/regulator/nrf5x.h>

#include <nrf_usbd_common.h>
#include <nrf_usbd_common_errata.h>
#include <hal/nrf_usbd.h>
#include <nrfx_power.h>
#include "udc_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_nrf, CONFIG_UDC_DRIVER_LOG_LEVEL);

/*
 * There is no real advantage to change control endpoint size
 * but we can use it for testing UDC driver API and higher layers.
 */
#define UDC_NRF_MPS0		UDC_MPS0_64
#define UDC_NRF_EP0_SIZE	64

enum udc_nrf_event_type {
	/* Trigger next transfer (buffer enqueued) */
	UDC_NRF_EVT_XFER,
	/* Transfer finished */
	UDC_NRF_EVT_EP_FINISHED,
	/* SETUP data received */
	UDC_NRF_EVT_SETUP,
	/* USB bus suspended */
	UDC_NRF_EVT_SUSPEND,
	/* USB bus resumed */
	UDC_NRF_EVT_RESUME,
	/* Remote Wakeup initiated */
	UDC_NRF_EVT_WUREQ,
	/* Let controller perform status stage */
	UDC_NRF_EVT_STATUS_IN,
};

/* Main events the driver thread waits for */
static K_EVENT_DEFINE(drv_evt);
/* Transfer triggers */
static atomic_t xfer_new;
/* Finished transactions */
static atomic_t xfer_finished;

static K_KERNEL_STACK_DEFINE(drv_stack, CONFIG_UDC_NRF_THREAD_STACK_SIZE);
static struct k_thread drv_stack_data;

/* USB device controller access from devicetree */
#define DT_DRV_COMPAT nordic_nrf_usbd

#define CFG_EPIN_CNT		DT_INST_PROP(0, num_in_endpoints)
#define CFG_EPOUT_CNT		DT_INST_PROP(0, num_out_endpoints)
#define CFG_EP_ISOIN_CNT	DT_INST_PROP(0, num_isoin_endpoints)
#define CFG_EP_ISOOUT_CNT	DT_INST_PROP(0, num_isoout_endpoints)

static struct udc_ep_config ep_cfg_out[CFG_EPOUT_CNT + CFG_EP_ISOOUT_CNT + 1];
static struct udc_ep_config ep_cfg_in[CFG_EPIN_CNT + CFG_EP_ISOIN_CNT + 1];
static bool udc_nrf_setup_set_addr, udc_nrf_fake_setup;
static uint8_t udc_nrf_address;
const static struct device *udc_nrf_dev;

#define NRF_USBD_COMMON_EPIN_CNT      9
#define NRF_USBD_COMMON_EPOUT_CNT     9
#define NRF_USBD_COMMON_EP_NUM(ep)    (ep & 0xF)
#define NRF_USBD_COMMON_EP_IS_IN(ep)  ((ep & 0x80) == 0x80)
#define NRF_USBD_COMMON_EP_IS_OUT(ep) ((ep & 0x80) == 0)
#define NRF_USBD_COMMON_EP_IS_ISO(ep) ((ep & 0xF) >= 8)

#ifndef NRF_USBD_COMMON_ISO_DEBUG
/* Also generate information about ISOCHRONOUS events and transfers.
 * Turn this off if no ISOCHRONOUS transfers are going to be debugged and this
 * option generates a lot of useless messages.
 */
#define NRF_USBD_COMMON_ISO_DEBUG 1
#endif

#ifndef NRF_USBD_COMMON_USE_WORKAROUND_FOR_ANOMALY_211
/* Anomaly 211 - Device remains in SUSPEND too long when host resumes
 * a bus activity (sending SOF packets) without a RESUME condition.
 */
#define NRF_USBD_COMMON_USE_WORKAROUND_FOR_ANOMALY_211 0
#endif

/* Assert endpoint is valid */
#define NRF_USBD_COMMON_ASSERT_EP_VALID(ep) __ASSERT_NO_MSG(         \
	((NRF_USBD_COMMON_EP_IS_IN(ep) &&                            \
	 (NRF_USBD_COMMON_EP_NUM(ep) < NRF_USBD_COMMON_EPIN_CNT)) || \
	 (NRF_USBD_COMMON_EP_IS_OUT(ep) &&                           \
	 (NRF_USBD_COMMON_EP_NUM(ep) < NRF_USBD_COMMON_EPOUT_CNT))));

/* Lowest IN endpoint bit position */
#define NRF_USBD_COMMON_EPIN_BITPOS_0 0

/* Lowest OUT endpoint bit position */
#define NRF_USBD_COMMON_EPOUT_BITPOS_0 16

/* Input endpoint bits mask */
#define NRF_USBD_COMMON_EPIN_BIT_MASK (0xFFFFU << NRF_USBD_COMMON_EPIN_BITPOS_0)

/* Output endpoint bits mask */
#define NRF_USBD_COMMON_EPOUT_BIT_MASK (0xFFFFU << NRF_USBD_COMMON_EPOUT_BITPOS_0)

/* Isochronous endpoint bit mask */
#define USBD_EPISO_BIT_MASK                                                    \
	((1U << NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT8)) |           \
	 (1U << NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN8)))

/* Convert endpoint number to bit position */
#define NRF_USBD_COMMON_EP_BITPOS(ep) ((NRF_USBD_COMMON_EP_IS_IN(ep)      \
	? NRF_USBD_COMMON_EPIN_BITPOS_0 : NRF_USBD_COMMON_EPOUT_BITPOS_0) \
	+ NRF_USBD_COMMON_EP_NUM(ep))

/* Check it the bit positions values match defined DATAEPSTATUS bit positions */
BUILD_ASSERT(
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN1) == USBD_EPDATASTATUS_EPIN1_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN2) == USBD_EPDATASTATUS_EPIN2_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN3) == USBD_EPDATASTATUS_EPIN3_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN4) == USBD_EPDATASTATUS_EPIN4_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN5) == USBD_EPDATASTATUS_EPIN5_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN6) == USBD_EPDATASTATUS_EPIN6_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN7) == USBD_EPDATASTATUS_EPIN7_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT1) == USBD_EPDATASTATUS_EPOUT1_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT2) == USBD_EPDATASTATUS_EPOUT2_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT3) == USBD_EPDATASTATUS_EPOUT3_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT4) == USBD_EPDATASTATUS_EPOUT4_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT5) == USBD_EPDATASTATUS_EPOUT5_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT6) == USBD_EPDATASTATUS_EPOUT6_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT7) == USBD_EPDATASTATUS_EPOUT7_Pos),
	"NRF_USBD_COMMON bit positions do not match hardware"
);

/* True if USB bus is suspended, updated in interrupt handler */
static volatile bool m_bus_suspend;

/* Data Stage direction used to map EP0DATADONE to actual endpoint */
static uint8_t m_ep0_data_dir;

/* Set bit indicates that endpoint is ready for DMA transfer.
 *
 * OUT endpoint is ready when DATA packet has been ACKed by device.
 * IN endpoint is ready when IN endpoint buffer has no pending data.
 *
 * When endpoint is ready it responds with NAK to any further traffic.
 */
static uint32_t m_ep_ready;

/* Set bit indicates that endpoint is waiting for DMA transfer, i.e. there is
 * USB stack buffer queued for the transfer.
 */
static uint32_t m_ep_dma_waiting;

/* Set bit indicates that endpoint is armed.
 *
 * OUT endpoint armed means that valid DATA packet from host will be ACKed.
 * IN endpoint armed means that device will respond with DATA packet.
 */
static uint32_t m_ep_armed;

/* Semaphore to guard EasyDMA access.
 * In USBD there is only one DMA channel working in background, and new transfer
 * cannot be started when there is ongoing transfer on any other channel.
 */
static K_SEM_DEFINE(dma_available, 1, 1);

/* Endpoint on which DMA was started. */
static nrf_usbd_common_ep_t dma_ep;

/* Tracks whether total bytes transferred by DMA is even or odd. */
static uint8_t m_dma_odd;

/* First time enabling after reset. Used in nRF52 errata 223. */
static bool m_first_enable = true;

#define NRF_USBD_COMMON_FEEDER_BUFFER_SIZE NRF_USBD_COMMON_EPSIZE

/* Bounce buffer for sending data from FLASH */
static uint32_t m_tx_buffer[NRFX_CEIL_DIV(NRF_USBD_COMMON_FEEDER_BUFFER_SIZE, sizeof(uint32_t))];

/* Early declaration. Documentation above definition. */
static void usbd_dmareq_process(void);
static inline void usbd_int_rise(void);
static void nrf_usbd_common_stop(void);
static void nrf_usbd_legacy_transfer_out_drop(nrf_usbd_common_ep_t ep);
static size_t nrf_usbd_legacy_epout_size_get(nrf_usbd_common_ep_t ep);
static bool nrf_usbd_legacy_suspend_check(void);

/* Get EasyDMA end event address for given endpoint */
static volatile uint32_t *usbd_ep_to_endevent(nrf_usbd_common_ep_t ep)
{
	int ep_in = NRF_USBD_COMMON_EP_IS_IN(ep);
	int ep_num = NRF_USBD_COMMON_EP_NUM(ep);

	NRF_USBD_COMMON_ASSERT_EP_VALID(ep);

	if (!NRF_USBD_COMMON_EP_IS_ISO(ep_num)) {
		if (ep_in) {
			return &NRF_USBD->EVENTS_ENDEPIN[ep_num];
		} else {
			return &NRF_USBD->EVENTS_ENDEPOUT[ep_num];
		}
	}

	return ep_in ? &NRF_USBD->EVENTS_ENDISOIN : &NRF_USBD->EVENTS_ENDISOOUT;
}

/* Return number of bytes last transferred by EasyDMA on given endpoint */
static uint32_t usbd_ep_amount_get(nrf_usbd_common_ep_t ep)
{
	int ep_in = NRF_USBD_COMMON_EP_IS_IN(ep);
	int ep_num = NRF_USBD_COMMON_EP_NUM(ep);

	NRF_USBD_COMMON_ASSERT_EP_VALID(ep);

	if (!NRF_USBD_COMMON_EP_IS_ISO(ep_num)) {
		if (ep_in) {
			return NRF_USBD->EPIN[ep_num].AMOUNT;
		} else {
			return NRF_USBD->EPOUT[ep_num].AMOUNT;
		}
	}

	return ep_in ? NRF_USBD->ISOIN.AMOUNT : NRF_USBD->ISOOUT.AMOUNT;
}

/* Start EasyDMA on given endpoint */
static void usbd_ep_dma_start(nrf_usbd_common_ep_t ep, uint32_t addr, size_t len)
{
	int ep_in = NRF_USBD_COMMON_EP_IS_IN(ep);
	int ep_num = NRF_USBD_COMMON_EP_NUM(ep);

	NRF_USBD_COMMON_ASSERT_EP_VALID(ep);

	if (!NRF_USBD_COMMON_EP_IS_ISO(ep_num)) {
		if (ep_in) {
			NRF_USBD->EPIN[ep_num].PTR = addr;
			NRF_USBD->EPIN[ep_num].MAXCNT = len;
			NRF_USBD->TASKS_STARTEPIN[ep_num] = 1;
		} else {
			NRF_USBD->EPOUT[ep_num].PTR = addr;
			NRF_USBD->EPOUT[ep_num].MAXCNT = len;
			NRF_USBD->TASKS_STARTEPOUT[ep_num] = 1;
		}
	} else if (ep_in) {
		NRF_USBD->ISOIN.PTR = addr;
		NRF_USBD->ISOIN.MAXCNT = len;
		NRF_USBD->TASKS_STARTISOIN = 1;
	} else {
		NRF_USBD->ISOOUT.PTR = addr;
		NRF_USBD->ISOOUT.MAXCNT = len;
		NRF_USBD->TASKS_STARTISOOUT = 1;
	}
}

/* Convert endpoint number to bit position matching EPDATASTATUS register.
 * Control and isochronous endpoints occupy unused EPDATASTATUS bits.
 */
static inline uint8_t ep2bit(nrf_usbd_common_ep_t ep)
{
	NRF_USBD_COMMON_ASSERT_EP_VALID(ep);
	return NRF_USBD_COMMON_EP_BITPOS(ep);
}

static inline nrf_usbd_common_ep_t bit2ep(uint8_t bitpos)
{
	BUILD_ASSERT(NRF_USBD_COMMON_EPOUT_BITPOS_0 > NRF_USBD_COMMON_EPIN_BITPOS_0,
		     "OUT endpoint bits should be higher than IN endpoint bits");
	return (nrf_usbd_common_ep_t)((bitpos >= NRF_USBD_COMMON_EPOUT_BITPOS_0)
		? NRF_USBD_COMMON_EPOUT(bitpos - NRF_USBD_COMMON_EPOUT_BITPOS_0)
		: NRF_USBD_COMMON_EPIN(bitpos));
}

/* Prepare DMA for transfer */
static inline void usbd_dma_pending_set(void)
{
	if (nrf_usbd_common_errata_199()) {
		*((volatile uint32_t *)0x40027C1C) = 0x00000082;
	}
}

/* DMA transfer finished */
static inline void usbd_dma_pending_clear(void)
{
	if (nrf_usbd_common_errata_199()) {
		*((volatile uint32_t *)0x40027C1C) = 0x00000000;
	}
}

static void disarm_endpoint(uint8_t ep)
{
	if (ep == USB_CONTROL_EP_OUT || ep == USB_CONTROL_EP_IN) {
		/* EP0 cannot be disarmed. This is not a problem because SETUP
		 * token automatically disarms EP0 IN and EP0 OUT.
		 */
		return;
	}

	if (NRF_USBD_COMMON_EP_IS_ISO(ep)) {
		/* Isochronous endpoints cannot be disarmed */
		return;
	}

	if (!(m_ep_armed & BIT(ep2bit(ep)))) {
		/* Endpoint is not armed, nothing to do */
		return;
	}

	m_ep_armed &= ~BIT(ep2bit(ep));

	/* Disarm the endpoint if there is any data buffered. For OUT endpoints
	 * disarming means that the endpoint won't ACK (will NAK) DATA packet.
	 */
	*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x800)) = 0x7B6 +
		2 * (USB_EP_GET_IDX(ep) - 1) + USB_EP_DIR_IS_OUT(ep) * 0x10;
	*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x804)) |= BIT(1);
}

static inline void usbd_ep_abort(nrf_usbd_common_ep_t ep)
{
	unsigned int irq_lock_key = irq_lock();

	disarm_endpoint(ep);

	/* Do not process any data until endpoint is enqueued again */
	m_ep_dma_waiting &= ~BIT(ep2bit(ep));

	if (!NRF_USBD_COMMON_EP_IS_ISO(ep)) {
		if (NRF_USBD_COMMON_EP_IS_OUT(ep)) {
			m_ep_ready &= ~BIT(ep2bit(ep));
		} else {
			m_ep_ready |= BIT(ep2bit(ep));
		}
	}

	/* Disarming endpoint is inherently a race between the driver and host.
	 * Clear EPDATASTATUS to prevent interrupt handler from processing the
	 * data if disarming lost the race (i.e. host finished first).
	 */
	NRF_USBD->EPDATASTATUS = BIT(ep2bit(ep));

	irq_unlock(irq_lock_key);
}

static void nrf_usbd_legacy_ep_abort(nrf_usbd_common_ep_t ep)
{
	/* Only abort if there is no active DMA */
	k_sem_take(&dma_available, K_FOREVER);
	usbd_ep_abort(ep);
	k_sem_give(&dma_available);

	/* This function was holding DMA semaphore and could potentially prevent
	 * next DMA from executing. Fire IRQ handler to check if any DMA needs
	 * to be started.
	 */
	usbd_int_rise();
}

static void usbd_ep_abort_all(void)
{
	uint32_t ep_waiting = m_ep_dma_waiting | (m_ep_ready & NRF_USBD_COMMON_EPOUT_BIT_MASK);

	while (ep_waiting != 0) {
		uint8_t bitpos = NRF_CTZ(ep_waiting);

		if (!NRF_USBD_COMMON_EP_IS_ISO(bit2ep(bitpos))) {
			usbd_ep_abort(bit2ep(bitpos));
		}
		ep_waiting &= ~(1U << bitpos);
	}

	m_ep_ready = (((1U << NRF_USBD_COMMON_EPIN_CNT) - 1U) << NRF_USBD_COMMON_EPIN_BITPOS_0);
}

/* Rise USBD interrupt to trigger interrupt handler */
static inline void usbd_int_rise(void)
{
	NVIC_SetPendingIRQ(USBD_IRQn);
}

static void ev_usbreset_handler(void)
{
	m_bus_suspend = false;

	LOG_INF("Reset");
	udc_submit_event(udc_nrf_dev, UDC_EVT_RESET, 0);
}

static void nrf_usbd_dma_finished(nrf_usbd_common_ep_t ep)
{
	/* DMA finished, track if total bytes transferred is even or odd */
	m_dma_odd ^= usbd_ep_amount_get(ep) & 1;
	usbd_dma_pending_clear();

	if ((m_ep_dma_waiting & BIT(ep2bit(ep))) == 0) {
		if (NRF_USBD_COMMON_EP_IS_OUT(ep) || (ep == NRF_USBD_COMMON_EPIN8)) {
			/* Send event to the user - for an ISO IN or any OUT endpoint,
			 * the whole transfer is finished in this moment
			 */
			atomic_set_bit(&xfer_finished, ep2bit(ep));
			k_event_post(&drv_evt, BIT(UDC_NRF_EVT_EP_FINISHED));
		}
	} else if (ep == NRF_USBD_COMMON_EPOUT0) {
		/* Allow receiving next OUT Data Stage chunk */
		NRF_USBD->TASKS_EP0RCVOUT = 1;
	}

	if (NRF_USBD_COMMON_EP_IS_IN(ep) ||
	    (ep >= NRF_USBD_COMMON_EPOUT1 && ep <= NRF_USBD_COMMON_EPOUT7)) {
		m_ep_armed |= BIT(ep2bit(ep));
	}

	k_sem_give(&dma_available);
}

static void ev_sof_handler(void)
{
	/* Process isochronous endpoints */
	uint32_t iso_ready_mask = (1U << ep2bit(NRF_USBD_COMMON_EPIN8));

	/* SIZE.ISOOUT is 0 only when no packet was received at all */
	if (NRF_USBD->SIZE.ISOOUT) {
		iso_ready_mask |= (1U << ep2bit(NRF_USBD_COMMON_EPOUT8));
	}
	m_ep_ready |= iso_ready_mask;

	m_ep_armed &= ~USBD_EPISO_BIT_MASK;

	udc_submit_event(udc_nrf_dev, UDC_EVT_SOF, 0);
}

static void usbd_in_packet_sent(uint8_t ep)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(udc_nrf_dev, ep);
	struct net_buf *buf = udc_buf_peek(ep_cfg);

	net_buf_pull(buf, usbd_ep_amount_get(ep));

	if (buf->len) {
		/* More packets will be sent, nothing to do here */
	} else if (udc_ep_buf_has_zlp(buf)) {
		/* Actual payload sent, only ZLP left */
		udc_ep_buf_clear_zlp(buf);
	} else {
		LOG_DBG("USBD event: EndpointData: In finished");
		/* No more data to be send - transmission finished */
		atomic_set_bit(&xfer_finished, ep2bit(ep));
		k_event_post(&drv_evt, BIT(UDC_NRF_EVT_EP_FINISHED));
	}
}

static void ev_setup_handler(void)
{
	LOG_DBG("USBD event: Setup (rt:%.2x r:%.2x v:%.4x i:%.4x l:%u )",
		       NRF_USBD->BMREQUESTTYPE, NRF_USBD->BREQUEST,
		       NRF_USBD->WVALUEL | (NRF_USBD->WVALUEH << 8),
		       NRF_USBD->WINDEXL | (NRF_USBD->WINDEXH << 8),
		       NRF_USBD->WLENGTHL | (NRF_USBD->WLENGTHH << 8));

	m_ep_dma_waiting &= ~((1U << ep2bit(NRF_USBD_COMMON_EPOUT0)) |
			      (1U << ep2bit(NRF_USBD_COMMON_EPIN0)));
	m_ep_ready &= ~(1U << ep2bit(NRF_USBD_COMMON_EPOUT0));
	m_ep_ready |= 1U << ep2bit(NRF_USBD_COMMON_EPIN0);

	m_ep_armed &= ~(BIT(ep2bit(USB_CONTROL_EP_OUT)) |
			BIT(ep2bit(USB_CONTROL_EP_IN)));

	k_event_post(&drv_evt, BIT(UDC_NRF_EVT_SETUP));
}

static void ev_usbevent_handler(void)
{
	uint32_t event = NRF_USBD->EVENTCAUSE;

	/* Clear handled events */
	NRF_USBD->EVENTCAUSE = event;

	if (event & USBD_EVENTCAUSE_ISOOUTCRC_Msk) {
		LOG_DBG("USBD event: ISOOUTCRC");
		/* Currently no support */
	}
	if (event & USBD_EVENTCAUSE_SUSPEND_Msk) {
		LOG_DBG("USBD event: SUSPEND");
		m_bus_suspend = true;

		k_event_post(&drv_evt, BIT(UDC_NRF_EVT_SUSPEND));
	}
	if (event & USBD_EVENTCAUSE_RESUME_Msk) {
		LOG_DBG("USBD event: RESUME");
		m_bus_suspend = false;

		k_event_post(&drv_evt, BIT(UDC_NRF_EVT_RESUME));
	}
	if (event & USBD_EVENTCAUSE_USBWUALLOWED_Msk) {
		LOG_DBG("USBD event: WUREQ (%s)", m_bus_suspend ? "In Suspend" : "Active");
		if (m_bus_suspend) {
			__ASSERT_NO_MSG(!nrf_usbd_legacy_suspend_check());
			m_bus_suspend = false;

			NRF_USBD->DPDMVALUE = USBD_DPDMVALUE_STATE_Resume
				<< USBD_DPDMVALUE_STATE_Pos;
			NRF_USBD->TASKS_DPDMDRIVE = 1;

			k_event_post(&drv_evt, BIT(UDC_NRF_EVT_WUREQ));
		}
	}
}

static void ev_epdata_handler(uint32_t dataepstatus)
{
	if (dataepstatus) {
		LOG_DBG("USBD event: EndpointEPStatus: %x", dataepstatus);

		/* Mark endpoints ready for next DMA access */
		m_ep_ready |= dataepstatus & ~USBD_EPISO_BIT_MASK;

		/* IN endpoints are no longer armed after host read the data.
		 * OUT endpoints are no longer armed before DMA reads the data.
		 */
		m_ep_armed &= ~(dataepstatus & ~USBD_EPISO_BIT_MASK);

		/* Peripheral automatically enables endpoint for data reception
		 * after OUT endpoint DMA transfer. This makes the device ACK
		 * the OUT DATA even if the stack did not enqueue any buffer.
		 *
		 * This behaviour most likely cannot be avoided and therefore
		 * there's nothing more to do for OUT endpoints.
		 */
		dataepstatus &= NRF_USBD_COMMON_EPIN_BIT_MASK;
	}

	/* Prepare next packet on IN endpoints */
	while (dataepstatus) {
		uint8_t bitpos = NRF_CTZ(dataepstatus);

		dataepstatus &= ~BIT(bitpos);

		usbd_in_packet_sent(bit2ep(bitpos));
	}
}

/* Select endpoint for next DMA transfer.
 *
 * Passed value has at least one bit set. Each bit set indicates which endpoints
 * can have data transferred between peripheral and USB stack buffer.
 *
 * Return bit position indicating which endpoint to transfer.
 */
static uint8_t usbd_dma_scheduler_algorithm(uint32_t req)
{
	/* Only prioritized scheduling mode is supported. */
	return NRF_CTZ(req);
}

/* Process next DMA request, called at the end of interrupt handler */
static void usbd_dmareq_process(void)
{
	uint32_t req = m_ep_dma_waiting & m_ep_ready;
	struct net_buf *buf;
	struct udc_ep_config *ep_cfg;
	uint8_t *payload_buf;
	size_t payload_len;
	bool last_packet;
	uint8_t pos;

	if (req == 0 || nrf_usbd_legacy_suspend_check() ||
	    k_sem_take(&dma_available, K_NO_WAIT) != 0) {
		/* DMA cannot be started */
		return;
	}

	if (NRFX_USBD_CONFIG_DMASCHEDULER_ISO_BOOST &&
	    ((req & USBD_EPISO_BIT_MASK) != 0)) {
		pos = usbd_dma_scheduler_algorithm(req & USBD_EPISO_BIT_MASK);
	} else {
		pos = usbd_dma_scheduler_algorithm(req);
	}

	nrf_usbd_common_ep_t ep = bit2ep(pos);

	ep_cfg = udc_get_ep_cfg(udc_nrf_dev, ep);
	buf = udc_buf_peek(ep_cfg);

	__ASSERT_NO_MSG(buf);

	if (NRF_USBD_COMMON_EP_IS_IN(ep)) {
		/* Device -> Host */
		payload_buf = buf->data;

		if (buf->len > udc_mps_ep_size(ep_cfg)) {
			payload_len = udc_mps_ep_size(ep_cfg);
			last_packet = false;
		} else {
			payload_len = buf->len;
			last_packet = !udc_ep_buf_has_zlp(buf);
		}

		if (!nrfx_is_in_ram(payload_buf)) {
			__ASSERT_NO_MSG(payload_len <= NRF_USBD_COMMON_FEEDER_BUFFER_SIZE);
			memcpy(m_tx_buffer, payload_buf, payload_len);
			payload_buf = (uint8_t *)m_tx_buffer;
		}
	} else {
		/* Host -> Device */
		const size_t received = nrf_usbd_legacy_epout_size_get(ep);

		payload_buf = net_buf_tail(buf);
		payload_len = net_buf_tailroom(buf);

		__ASSERT_NO_MSG(nrfx_is_in_ram(payload_buf));

		if (received > payload_len) {
			LOG_ERR("buffer too small: r: %u, l: %u", received, payload_len);
		} else {
			payload_len = received;
		}

		/* DMA will copy the received data, update the buffer here so received
		 * does not have to be stored (can be done because there is no cache).
		 */
		net_buf_add(buf, payload_len);

		last_packet = (udc_mps_ep_size(ep_cfg) != received) ||
			      (net_buf_tailroom(buf) == 0);
	}

	if (last_packet) {
		m_ep_dma_waiting &= ~BIT(pos);
	}

	usbd_dma_pending_set();
	m_ep_ready &= ~BIT(pos);
	if (NRF_USBD_COMMON_ISO_DEBUG || (!NRF_USBD_COMMON_EP_IS_ISO(ep))) {
		LOG_DBG("USB DMA process: Starting transfer on EP: %x, size: %u",
			ep, payload_len);
	}

	/* Start transfer to the endpoint buffer */
	dma_ep = ep;
	usbd_ep_dma_start(ep, (uint32_t)payload_buf, payload_len);
}

static inline void usbd_errata_171_begin(void)
{
	unsigned int irq_lock_key = irq_lock();

	if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000) {
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
		*((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
	} else {
		*((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
	}

	irq_unlock(irq_lock_key);
}

static inline void usbd_errata_171_end(void)
{
	unsigned int irq_lock_key = irq_lock();

	if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000) {
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
		*((volatile uint32_t *)(0x4006EC14)) = 0x00000000;
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
	} else {
		*((volatile uint32_t *)(0x4006EC14)) = 0x00000000;
	}

	irq_unlock(irq_lock_key);
}

static inline void usbd_errata_187_211_begin(void)
{
	unsigned int irq_lock_key = irq_lock();

	if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000) {
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
		*((volatile uint32_t *)(0x4006ED14)) = 0x00000003;
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
	} else {
		*((volatile uint32_t *)(0x4006ED14)) = 0x00000003;
	}

	irq_unlock(irq_lock_key);
}

static inline void usbd_errata_187_211_end(void)
{
	unsigned int irq_lock_key = irq_lock();

	if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000) {
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
		*((volatile uint32_t *)(0x4006ED14)) = 0x00000000;
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
	} else {
		*((volatile uint32_t *)(0x4006ED14)) = 0x00000000;
	}

	irq_unlock(irq_lock_key);
}

static void nrf_usbd_peripheral_enable(void)
{
	if (nrf_usbd_common_errata_187()) {
		usbd_errata_187_211_begin();
	}

	if (nrf_usbd_common_errata_171()) {
		usbd_errata_171_begin();
	}

	/* Enable the peripheral */
	NRF_USBD->ENABLE = 1;

	/* Waiting for peripheral to enable, this should take a few us */
	while ((NRF_USBD->EVENTCAUSE & USBD_EVENTCAUSE_READY_Msk) == 0) {
	}
	NRF_USBD->EVENTCAUSE = USBD_EVENTCAUSE_READY_Msk;

	if (nrf_usbd_common_errata_171()) {
		usbd_errata_171_end();
	}

	if (nrf_usbd_common_errata_187()) {
		usbd_errata_187_211_end();
	}
}

static void nrf_usbd_irq_handler(void)
{
	volatile uint32_t *dma_endevent;
	uint32_t epdatastatus = 0;

	/* Always check and clear SOF but call handler only if SOF interrupt
	 * is actually enabled.
	 */
	if (NRF_USBD->EVENTS_SOF) {
		NRF_USBD->EVENTS_SOF = 0;
		if (NRF_USBD->INTENSET & USBD_INTEN_SOF_Msk) {
			ev_sof_handler();
		}
	}

	/* Clear EPDATA event and only then get and clear EPDATASTATUS to make
	 * sure we don't miss any event.
	 */
	if (NRF_USBD->EVENTS_EPDATA) {
		NRF_USBD->EVENTS_EPDATA = 0;
		epdatastatus = NRF_USBD->EPDATASTATUS;
		NRF_USBD->EPDATASTATUS = epdatastatus;
	}

	/* Use common variable to store EP0DATADONE processing needed flag */
	if (NRF_USBD->EVENTS_EP0DATADONE) {
		NRF_USBD->EVENTS_EP0DATADONE = 0;
		epdatastatus |= BIT(ep2bit(m_ep0_data_dir));
	}

	/* Check DMA end event only for last enabled DMA channel. Other channels
	 * cannot be active and there's no harm in rechecking the event multiple
	 * times (it is not a problem to check it even if DMA is not active).
	 *
	 * It is important to check DMA and handle DMA finished event before
	 * handling acknowledged data transfer bits (epdatastatus) to avoid
	 * a race condition between interrupt handler and host IN token.
	 */
	dma_endevent = usbd_ep_to_endevent(dma_ep);
	if (*dma_endevent) {
		*dma_endevent = 0;
		nrf_usbd_dma_finished(dma_ep);
	}

	/* Process acknowledged transfers so we can prepare next DMA (if any) */
	ev_epdata_handler(epdatastatus);

	if (NRF_USBD->EVENTS_USBRESET) {
		NRF_USBD->EVENTS_USBRESET = 0;
		ev_usbreset_handler();
	}

	if (NRF_USBD->EVENTS_USBEVENT) {
		NRF_USBD->EVENTS_USBEVENT = 0;
		ev_usbevent_handler();
	}

	/* Handle SETUP only if there is no active DMA on EP0 */
	if (unlikely(NRF_USBD->EVENTS_EP0SETUP) &&
	    (k_sem_count_get(&dma_available) ||
	     (dma_ep != NRF_USBD_COMMON_EPIN0 && dma_ep != NRF_USBD_COMMON_EPOUT0))) {
		NRF_USBD->EVENTS_EP0SETUP = 0;
		ev_setup_handler();
	}

	usbd_dmareq_process();
}

static void nrf_usbd_legacy_enable(void)
{
	/* Prepare for READY event receiving */
	NRF_USBD->EVENTCAUSE = USBD_EVENTCAUSE_READY_Msk;

	nrf_usbd_peripheral_enable();

	if (nrf_usbd_common_errata_223() && m_first_enable) {
		NRF_USBD->ENABLE = 0;

		nrf_usbd_peripheral_enable();

		m_first_enable = false;
	}

#if NRF_USBD_COMMON_USE_WORKAROUND_FOR_ANOMALY_211
	if (nrf_usbd_common_errata_187() || nrf_usbd_common_errata_211()) {
#else
	if (nrf_usbd_common_errata_187()) {
#endif
		usbd_errata_187_211_begin();
	}

	if (nrf_usbd_common_errata_166()) {
		*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x800)) = 0x7E3;
		*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x804)) = 0x40;
		__ISB();
		__DSB();
	}

	NRF_USBD->ISOSPLIT = USBD_ISOSPLIT_SPLIT_HalfIN << USBD_ISOSPLIT_SPLIT_Pos;

	if (IS_ENABLED(CONFIG_NRF_USBD_ISO_IN_ZLP)) {
		NRF_USBD->ISOINCONFIG = USBD_ISOINCONFIG_RESPONSE_ZeroData
			<< USBD_ISOINCONFIG_RESPONSE_Pos;
	} else {
		NRF_USBD->ISOINCONFIG = USBD_ISOINCONFIG_RESPONSE_NoResp
			<< USBD_ISOINCONFIG_RESPONSE_Pos;
	}

	m_ep_ready = (((1U << NRF_USBD_COMMON_EPIN_CNT) - 1U) << NRF_USBD_COMMON_EPIN_BITPOS_0);
	m_ep_dma_waiting = 0;
	m_ep_armed = 0;
	m_dma_odd = 0;
	__ASSERT_NO_MSG(k_sem_count_get(&dma_available) == 1);
	usbd_dma_pending_clear();
	m_ep0_data_dir = USB_CONTROL_EP_OUT;

#if NRF_USBD_COMMON_USE_WORKAROUND_FOR_ANOMALY_211
	if (nrf_usbd_common_errata_187() && !nrf_usbd_common_errata_211()) {
#else
	if (nrf_usbd_common_errata_187()) {
#endif
		usbd_errata_187_211_end();
	}
}

static void nrf_usbd_legacy_disable(void)
{
	/* Make sure DMA is not active */
	k_sem_take(&dma_available, K_FOREVER);

	/* Stop just in case */
	nrf_usbd_common_stop();

	/* Disable all parts */
	if (m_dma_odd) {
		/* Prevent invalid bus request after next USBD enable by ensuring
		 * that total number of bytes transferred by DMA is even.
		 */
		NRF_USBD->EVENTS_ENDEPIN[0] = 0;
		usbd_ep_dma_start(NRF_USBD_COMMON_EPIN0, (uint32_t)&m_dma_odd, 1);
		while (!NRF_USBD->EVENTS_ENDEPIN[0]) {
		}
		NRF_USBD->EVENTS_ENDEPIN[0] = 0;
		m_dma_odd = 0;
	}
	NRF_USBD->ENABLE = 0;
	usbd_dma_pending_clear();
	k_sem_give(&dma_available);

#if NRF_USBD_COMMON_USE_WORKAROUND_FOR_ANOMALY_211
	if (nrf_usbd_common_errata_211()) {
		usbd_errata_187_211_end();
	}
#endif
}

static void nrf_usbd_legacy_start(bool enable_sof)
{
	m_bus_suspend = false;

	uint32_t int_mask = USBD_INTEN_USBRESET_Msk | USBD_INTEN_ENDEPIN0_Msk |
		USBD_INTEN_ENDEPIN1_Msk | USBD_INTEN_ENDEPIN2_Msk |
		USBD_INTEN_ENDEPIN3_Msk | USBD_INTEN_ENDEPIN4_Msk |
		USBD_INTEN_ENDEPIN5_Msk | USBD_INTEN_ENDEPIN6_Msk |
		USBD_INTEN_ENDEPIN7_Msk | USBD_INTEN_EP0DATADONE_Msk |
		USBD_INTEN_ENDISOIN_Msk | USBD_INTEN_ENDEPOUT0_Msk |
		USBD_INTEN_ENDEPOUT1_Msk | USBD_INTEN_ENDEPOUT2_Msk |
		USBD_INTEN_ENDEPOUT3_Msk | USBD_INTEN_ENDEPOUT4_Msk |
		USBD_INTEN_ENDEPOUT5_Msk | USBD_INTEN_ENDEPOUT6_Msk |
		USBD_INTEN_ENDEPOUT7_Msk | USBD_INTEN_ENDISOOUT_Msk |
		USBD_INTEN_USBEVENT_Msk | USBD_INTEN_EP0SETUP_Msk |
		USBD_INTEN_EPDATA_Msk;

	if (enable_sof) {
		int_mask |= USBD_INTEN_SOF_Msk;
	}

	/* Enable all required interrupts */
	NRF_USBD->INTEN = int_mask;

	/* Enable interrupt globally */
	irq_enable(USBD_IRQn);

	/* Enable pullups */
	NRF_USBD->USBPULLUP = 1;
}

static void nrf_usbd_common_stop(void)
{
	/* Clear interrupt */
	NVIC_ClearPendingIRQ(USBD_IRQn);

	if (irq_is_enabled(USBD_IRQn)) {
		/* Abort transfers */
		usbd_ep_abort_all();

		/* Disable pullups */
		NRF_USBD->USBPULLUP = 0;

		/* Disable interrupt globally */
		irq_disable(USBD_IRQn);

		/* Disable all interrupts */
		NRF_USBD->INTEN = 0;
	}
}

static bool nrf_usbd_legacy_suspend(void)
{
	bool suspended = false;
	unsigned int irq_lock_key;

	/* DMA doesn't work in Low Power mode, ensure there is no active DMA */
	k_sem_take(&dma_available, K_FOREVER);
	irq_lock_key = irq_lock();

	if (m_bus_suspend) {
		if (!(NRF_USBD->EVENTCAUSE & USBD_EVENTCAUSE_RESUME_Msk)) {
			NRF_USBD->LOWPOWER = USBD_LOWPOWER_LOWPOWER_LowPower
				<< USBD_LOWPOWER_LOWPOWER_Pos;
			(void)NRF_USBD->LOWPOWER;
			if (NRF_USBD->EVENTCAUSE & USBD_EVENTCAUSE_RESUME_Msk) {
				NRF_USBD->LOWPOWER = USBD_LOWPOWER_LOWPOWER_ForceNormal
					<< USBD_LOWPOWER_LOWPOWER_Pos;
			} else {
				suspended = true;
			}
		}
	}

	irq_unlock(irq_lock_key);
	k_sem_give(&dma_available);

	return suspended;
}

static bool nrf_usbd_legacy_wakeup_req(void)
{
	bool started = false;
	unsigned int irq_lock_key = irq_lock();

	if (m_bus_suspend && nrf_usbd_legacy_suspend_check()) {
		NRF_USBD->LOWPOWER = USBD_LOWPOWER_LOWPOWER_ForceNormal
			<< USBD_LOWPOWER_LOWPOWER_Pos;
		started = true;

		if (nrf_usbd_common_errata_171()) {
			if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000) {
				*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
				*((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
				*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
			} else {
				*((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
			}
		}
	}

	irq_unlock(irq_lock_key);

	return started;
}

static bool nrf_usbd_legacy_suspend_check(void)
{
	return NRF_USBD->LOWPOWER !=
		(USBD_LOWPOWER_LOWPOWER_ForceNormal << USBD_LOWPOWER_LOWPOWER_Pos);
}

static bool nrf_usbd_legacy_ep_enable_check(nrf_usbd_common_ep_t ep)
{
	int ep_in = NRF_USBD_COMMON_EP_IS_IN(ep);
	int ep_num = NRF_USBD_COMMON_EP_NUM(ep);

	NRF_USBD_COMMON_ASSERT_EP_VALID(ep);

	return (ep_in ? NRF_USBD->EPINEN : NRF_USBD->EPOUTEN) & BIT(ep_num);
}

static void nrf_usbd_legacy_ep_enable(nrf_usbd_common_ep_t ep)
{
	int ep_in = NRF_USBD_COMMON_EP_IS_IN(ep);
	int ep_num = NRF_USBD_COMMON_EP_NUM(ep);

	if (nrf_usbd_legacy_ep_enable_check(ep)) {
		return;
	}

	if (ep_in) {
		NRF_USBD->EPINEN |= BIT(ep_num);
	} else {
		NRF_USBD->EPOUTEN |= BIT(ep_num);
	}
}

static void nrf_usbd_legacy_ep_disable(nrf_usbd_common_ep_t ep)
{
	int ep_in = NRF_USBD_COMMON_EP_IS_IN(ep);
	int ep_num = NRF_USBD_COMMON_EP_NUM(ep);

	/* Only disable endpoint if there is no active DMA */
	k_sem_take(&dma_available, K_FOREVER);
	usbd_ep_abort(ep);
	if (ep_in) {
		NRF_USBD->EPINEN &= ~BIT(ep_num);
	} else {
		NRF_USBD->EPOUTEN &= ~BIT(ep_num);
	}
	k_sem_give(&dma_available);

	/* This function was holding DMA semaphore and could potentially prevent
	 * next DMA from executing. Fire IRQ handler to check if any DMA needs
	 * to be started.
	 */
	usbd_int_rise();
}

static void nrf_usbd_start_transfer(uint8_t ep)
{
	const uint8_t ep_bitpos = ep2bit(ep);
	unsigned int irq_lock_key = irq_lock();

	if (ep >= NRF_USBD_COMMON_EPOUT1 && ep <= NRF_USBD_COMMON_EPOUT7) {
		if (!(m_ep_armed & BIT(ep_bitpos)) &&
		    !(m_ep_ready & BIT(ep_bitpos))) {
			/* Allow receiving DATA packet on OUT endpoint */
			NRF_USBD->SIZE.EPOUT[NRF_USBD_COMMON_EP_NUM(ep)] = 0;
			m_ep_armed |= BIT(ep_bitpos);
		}
	} else if (ep == NRF_USBD_COMMON_EPIN8) {
		/* ISO IN endpoint can be already armed if application is double
		 * buffering ISO IN data. When the endpoint is already armed it
		 * must not be ready for next DMA transfer (until SOF).
		 */
		__ASSERT(!(m_ep_armed & BIT(ep_bitpos)) ||
			 !(m_ep_ready & BIT(ep_bitpos)),
			 "ISO IN must not be armed and ready");
	} else if (NRF_USBD_COMMON_EP_IS_IN(ep)) {
		/* IN endpoint must not have data armed */
		__ASSERT(!(m_ep_armed & BIT(ep_bitpos)),
			 "ep 0x%02x already armed", ep);
	}

	__ASSERT(!(m_ep_dma_waiting & BIT(ep_bitpos)),
		 "ep 0x%02x already waiting", ep);

	m_ep_dma_waiting |= BIT(ep_bitpos);
	usbd_int_rise();

	irq_unlock(irq_lock_key);
}

static size_t nrf_usbd_legacy_epout_size_get(nrf_usbd_common_ep_t ep)
{
	if (NRF_USBD_COMMON_EP_IS_ISO(ep)) {
		size_t size = NRF_USBD->SIZE.ISOOUT;

		if ((size & USBD_SIZE_ISOOUT_ZERO_Msk) ==
		    (USBD_SIZE_ISOOUT_ZERO_ZeroData << USBD_SIZE_ISOOUT_ZERO_Pos)) {
			size = 0;
		}
		return size;
	}

	return NRF_USBD->SIZE.EPOUT[NRF_USBD_COMMON_EP_NUM(ep)];
}

static void nrf_usbd_legacy_ep_stall(nrf_usbd_common_ep_t ep)
{
	__ASSERT_NO_MSG(!NRF_USBD_COMMON_EP_IS_ISO(ep));

	LOG_DBG("USB: EP %x stalled.", ep);
	NRF_USBD->EPSTALL = (USBD_EPSTALL_STALL_Stall << USBD_EPSTALL_STALL_Pos) | ep;
}

static bool nrf_usbd_legacy_ep_stall_check(nrf_usbd_common_ep_t ep)
{
	int ep_in = NRF_USBD_COMMON_EP_IS_IN(ep);
	int ep_num = NRF_USBD_COMMON_EP_NUM(ep);

	if (!NRF_USBD_COMMON_EP_IS_ISO(ep_num)) {
		if (ep_in) {
			return NRF_USBD->HALTED.EPIN[ep_num];
		} else {
			return NRF_USBD->HALTED.EPOUT[ep_num];
		}
	}

	return false;
}

static void nrf_usbd_legacy_ep_stall_clear(nrf_usbd_common_ep_t ep)
{
	__ASSERT_NO_MSG(!NRF_USBD_COMMON_EP_IS_ISO(ep));

	if (NRF_USBD_COMMON_EP_IS_OUT(ep) && nrf_usbd_legacy_ep_stall_check(ep)) {
		nrf_usbd_legacy_transfer_out_drop(ep);
	}
	NRF_USBD->EPSTALL = (USBD_EPSTALL_STALL_UnStall << USBD_EPSTALL_STALL_Pos) | ep;
}

static void nrf_usbd_legacy_ep_dtoggle_clear(nrf_usbd_common_ep_t ep)
{
	__ASSERT_NO_MSG(!NRF_USBD_COMMON_EP_IS_ISO(ep));

	NRF_USBD->DTOGGLE = ep | (USBD_DTOGGLE_VALUE_Nop << USBD_DTOGGLE_VALUE_Pos);
	NRF_USBD->DTOGGLE = ep | (USBD_DTOGGLE_VALUE_Data0 << USBD_DTOGGLE_VALUE_Pos);
}

static void nrf_usbd_legacy_transfer_out_drop(nrf_usbd_common_ep_t ep)
{
	unsigned int irq_lock_key = irq_lock();

	__ASSERT_NO_MSG(NRF_USBD_COMMON_EP_IS_OUT(ep));

	m_ep_ready &= ~(1U << ep2bit(ep));
	if (!NRF_USBD_COMMON_EP_IS_ISO(ep)) {
		NRF_USBD->SIZE.EPOUT[NRF_USBD_COMMON_EP_NUM(ep)] = 0;
	}

	irq_unlock(irq_lock_key);
}

struct udc_nrf_config {
	clock_control_subsys_t clock;
	nrfx_power_config_t pwr;
	nrfx_power_usbevt_config_t evt;
};

static struct onoff_manager *hfxo_mgr;
static struct onoff_client hfxo_cli;

static void udc_event_xfer_in_next(const struct device *dev, const uint8_t ep)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	struct net_buf *buf;

	if (udc_ep_is_busy(ep_cfg)) {
		return;
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf != NULL) {
		nrf_usbd_start_transfer(ep);
		udc_ep_set_busy(ep_cfg, true);
	}
}

static void udc_event_xfer_ctrl_in(const struct device *dev,
				   struct net_buf *const buf)
{
	if (udc_ctrl_stage_is_status_in(dev) ||
	    udc_ctrl_stage_is_no_data(dev)) {
		/* Status stage finished, notify upper layer */
		udc_ctrl_submit_status(dev, buf);
	}

	if (udc_ctrl_stage_is_data_in(dev)) {
		/*
		 * s-in-[status] finished, release buffer.
		 * Since the controller supports auto-status we cannot use
		 * if (udc_ctrl_stage_is_status_out()) after state update.
		 */
		net_buf_unref(buf);
	}

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (!udc_nrf_setup_set_addr) {
		/* Allow status stage */
		NRF_USBD->TASKS_EP0STATUS = 1;
	}
}

static void udc_event_fake_status_in(const struct device *dev)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	struct net_buf *buf;

	buf = udc_buf_get(ep_cfg);
	if (unlikely(buf == NULL)) {
		LOG_DBG("ep 0x%02x queue is empty", USB_CONTROL_EP_IN);
		return;
	}

	LOG_DBG("Fake status IN %p", buf);
	udc_event_xfer_ctrl_in(dev, buf);
}

static void udc_event_xfer_in(const struct device *dev, const uint8_t ep)
{
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;

	ep_cfg = udc_get_ep_cfg(dev, ep);

	buf = udc_buf_get(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("ep 0x%02x queue is empty", ep);
		__ASSERT_NO_MSG(false);
		return;
	}

	udc_ep_set_busy(ep_cfg, false);
	if (ep == USB_CONTROL_EP_IN) {
		udc_event_xfer_ctrl_in(dev, buf);
	} else {
		udc_submit_ep_event(dev, buf, 0);
	}
}

static void udc_event_xfer_ctrl_out(const struct device *dev,
				    struct net_buf *const buf)
{
	/*
	 * In case s-in-status, controller supports auto-status therefore we
	 * do not have to call udc_ctrl_stage_is_status_out().
	 */

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_status_in(dev)) {
		udc_ctrl_submit_s_out_status(dev, buf);
	}
}

static void udc_event_xfer_out_next(const struct device *dev, const uint8_t ep)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	struct net_buf *buf;

	if (udc_ep_is_busy(ep_cfg)) {
		return;
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf != NULL) {
		nrf_usbd_start_transfer(ep);
		udc_ep_set_busy(ep_cfg, true);
	} else {
		LOG_DBG("ep 0x%02x waiting, queue is empty", ep);
	}
}

static void udc_event_xfer_out(const struct device *dev, const uint8_t ep)
{
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	buf = udc_buf_get(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("ep 0x%02x ok, queue is empty", ep);
		return;
	}

	udc_ep_set_busy(ep_cfg, false);
	if (ep == USB_CONTROL_EP_OUT) {
		udc_event_xfer_ctrl_out(dev, buf);
	} else {
		udc_submit_ep_event(dev, buf, 0);
	}
}

static int usbd_ctrl_feed_dout(const struct device *dev,
			       const size_t length)
{
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	udc_buf_put(cfg, buf);

	__ASSERT_NO_MSG(k_current_get() == &drv_stack_data);
	udc_event_xfer_out_next(dev, USB_CONTROL_EP_OUT);

	/* Allow receiving first OUT Data Stage packet */
	NRF_USBD->TASKS_EP0RCVOUT = 1;

	return 0;
}

static int udc_event_xfer_setup(const struct device *dev)
{
	struct udc_ep_config *cfg_out = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct udc_ep_config *cfg_in = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	struct usb_setup_packet *setup;
	struct net_buf *buf;
	int err;

	/* Make sure there isn't any obsolete data stage buffer queued */
	buf = udc_buf_get_all(cfg_out);
	if (buf) {
		net_buf_unref(buf);
	}

	buf = udc_buf_get_all(cfg_in);
	if (buf) {
		net_buf_unref(buf);
	}

	udc_ep_set_busy(cfg_out, false);
	udc_ep_set_busy(cfg_in, false);

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT,
			     sizeof(struct usb_setup_packet));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate for setup");
		return -ENOMEM;
	}

	udc_ep_buf_set_setup(buf);
	setup = (struct usb_setup_packet *)buf->data;
	setup->bmRequestType = NRF_USBD->BMREQUESTTYPE;
	setup->bRequest = NRF_USBD->BREQUEST;
	setup->wValue = NRF_USBD->WVALUEL | (NRF_USBD->WVALUEH << 8);
	setup->wIndex = NRF_USBD->WINDEXL | (NRF_USBD->WINDEXH << 8);
	setup->wLength = NRF_USBD->WLENGTHL | (NRF_USBD->WLENGTHH << 8);

	/* USBD peripheral automatically handles Set Address in slightly
	 * different manner than the USB stack.
	 *
	 * USBD peripheral doesn't care about wLength, but the peripheral
	 * switches to new address only after status stage. The device won't
	 * automatically accept Data Stage packets.
	 *
	 * However, in the case the host:
	 *   * sends SETUP Set Address with non-zero wLength
	 *   * does not send corresponding OUT DATA packets (to match wLength)
	 *     or sends the packets but disregards NAK
	 *     or sends the packets that device ACKs
	 *   * sends IN token (either incorrectly proceeds to status stage, or
	 *     manages to send IN before SW sets STALL)
	 * then the USBD peripheral will accept the address and USB stack won't.
	 * This will lead to state mismatch between the stack and peripheral.
	 *
	 * In cases where the USB stack would like to STALL the request there is
	 * a race condition between host issuing Set Address status stage (IN
	 * token) and SW setting STALL bit. If host wins the race, the device
	 * ACKs status stage and use new address. If device wins the race, the
	 * device STALLs status stage and address remains unchanged.
	 */
	udc_nrf_setup_set_addr =
		setup->bmRequestType == 0 &&
		setup->bRequest == USB_SREQ_SET_ADDRESS;
	if (udc_nrf_setup_set_addr) {
		if (setup->wLength) {
			/* Currently USB stack only STALLs OUT Data Stage when
			 * buffer allocation fails. To prevent the device from
			 * ACKing the Data Stage, simply ignore the request
			 * completely.
			 *
			 * If host incorrectly proceeds to status stage there
			 * will be address mismatch (unless the new address is
			 * equal to current device address). If host does not
			 * issue IN token then the mismatch will be avoided.
			 */
			net_buf_unref(buf);
			return 0;
		}

		/* nRF52/nRF53 USBD doesn't care about wValue bits 8..15 and
		 * wIndex value but USB device stack does.
		 *
		 * Just clear the bits so stack will handle the request in the
		 * same way as USBD peripheral does, avoiding the mismatch.
		 */
		setup->wValue &= 0x7F;
		setup->wIndex = 0;
	}

	if (!udc_nrf_setup_set_addr && udc_nrf_address != NRF_USBD->USBADDR) {
		/* Address mismatch detected. Fake Set Address handling to
		 * correct the situation, then repeat handling.
		 */
		udc_nrf_fake_setup = true;
		udc_nrf_setup_set_addr = true;

		setup->bmRequestType = 0;
		setup->bRequest = USB_SREQ_SET_ADDRESS;
		setup->wValue = NRF_USBD->USBADDR;
		setup->wIndex = 0;
		setup->wLength = 0;
	} else {
		udc_nrf_fake_setup = false;
	}

	net_buf_add(buf, sizeof(nrf_usbd_common_setup_t));

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for data OUT stage */
		LOG_DBG("s:%p|feed for -out-", buf);
		m_ep0_data_dir = USB_CONTROL_EP_OUT;
		err = usbd_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			err = udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		m_ep0_data_dir = USB_CONTROL_EP_IN;
		err = udc_ctrl_submit_s_in_status(dev);
	} else {
		err = udc_ctrl_submit_s_status(dev);
	}

	return err;
}

static void udc_nrf_thread_handler(const struct device *dev)
{
	uint32_t evt;

	/* Wait for at least one event */
	k_event_wait(&drv_evt, UINT32_MAX, false, K_FOREVER);

	/* Process all events that are set */
	evt = k_event_clear(&drv_evt, UINT32_MAX);

	if (evt & BIT(UDC_NRF_EVT_SUSPEND)) {
		LOG_INF("SUSPEND state detected");
		nrf_usbd_legacy_suspend();
		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
	}

	if (evt & BIT(UDC_NRF_EVT_RESUME)) {
		LOG_INF("RESUMING from suspend");
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
	}

	if (evt & BIT(UDC_NRF_EVT_WUREQ)) {
		LOG_INF("Remote wakeup initiated");
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
	}

	if (evt & BIT(UDC_NRF_EVT_EP_FINISHED)) {
		uint32_t eps = atomic_clear(&xfer_finished);

		while (eps) {
			uint8_t bitpos = NRF_CTZ(eps);
			nrf_usbd_common_ep_t ep = bit2ep(bitpos);

			eps &= ~BIT(bitpos);

			if (USB_EP_DIR_IS_IN(ep)) {
				udc_event_xfer_in(dev, ep);
				udc_event_xfer_in_next(dev, ep);
			} else {
				udc_event_xfer_out(dev, ep);
				udc_event_xfer_out_next(dev, ep);
			}
		}
	}

	if (evt & BIT(UDC_NRF_EVT_XFER)) {
		uint32_t eps = atomic_clear(&xfer_new);

		while (eps) {
			uint8_t bitpos = NRF_CTZ(eps);
			nrf_usbd_common_ep_t ep = bit2ep(bitpos);

			eps &= ~BIT(bitpos);

			if (USB_EP_DIR_IS_IN(ep)) {
				udc_event_xfer_in_next(dev, ep);
			} else {
				udc_event_xfer_out_next(dev, ep);
			}
		}
	}

	if (evt & BIT(UDC_NRF_EVT_STATUS_IN)) {
		udc_event_fake_status_in(dev);
	}

	if (evt & BIT(UDC_NRF_EVT_SETUP)) {
		udc_event_xfer_setup(dev);
	}
}

static void udc_nrf_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;

	while (true) {
		udc_nrf_thread_handler(dev);
	}
}

static void udc_nrf_power_handler(nrfx_power_usb_evt_t pwr_evt)
{
	switch (pwr_evt) {
	case NRFX_POWER_USB_EVT_DETECTED:
		LOG_DBG("POWER event detected");
		udc_submit_event(udc_nrf_dev, UDC_EVT_VBUS_READY, 0);
		break;
	case NRFX_POWER_USB_EVT_READY:
		LOG_DBG("POWER event ready");
		nrf_usbd_legacy_start(true);
		break;
	case NRFX_POWER_USB_EVT_REMOVED:
		LOG_DBG("POWER event removed");
		udc_submit_event(udc_nrf_dev, UDC_EVT_VBUS_REMOVED, 0);
		break;
	default:
		LOG_ERR("Unknown power event %d", pwr_evt);
	}
}

static int udc_nrf_ep_enqueue(const struct device *dev,
			      struct udc_ep_config *cfg,
			      struct net_buf *buf)
{
	udc_buf_put(cfg, buf);

	if (cfg->addr == USB_CONTROL_EP_IN && buf->len == 0) {
		const struct udc_buf_info *bi = udc_get_buf_info(buf);

		if (bi->status) {
			/* Controller automatically performs status IN stage */
			k_event_post(&drv_evt, BIT(UDC_NRF_EVT_STATUS_IN));
			return 0;
		}
	}

	atomic_set_bit(&xfer_new, ep2bit(cfg->addr));
	k_event_post(&drv_evt, BIT(UDC_NRF_EVT_XFER));

	return 0;
}

static int udc_nrf_ep_dequeue(const struct device *dev,
			      struct udc_ep_config *cfg)
{
	struct net_buf *buf;

	nrf_usbd_legacy_ep_abort(cfg->addr);

	buf = udc_buf_get_all(cfg);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	} else {
		LOG_INF("ep 0x%02x queue is empty", cfg->addr);
	}

	udc_ep_set_busy(cfg, false);

	return 0;
}

static int udc_nrf_ep_enable(const struct device *dev,
			     struct udc_ep_config *cfg)
{
	uint16_t mps;

	__ASSERT_NO_MSG(cfg);
	mps = (udc_mps_ep_size(cfg) == 0) ? cfg->caps.mps : udc_mps_ep_size(cfg);
	nrf_usbd_legacy_ep_enable(cfg->addr);
	if (!NRF_USBD_EPISO_CHECK(cfg->addr)) {
		/* ISO transactions for full-speed device do not support
		 * toggle sequencing and should only send DATA0 PID.
		 */
		nrf_usbd_legacy_ep_dtoggle_clear(cfg->addr);
		nrf_usbd_legacy_ep_stall_clear(cfg->addr);
	}

	LOG_DBG("Enable ep 0x%02x", cfg->addr);

	return 0;
}

static int udc_nrf_ep_disable(const struct device *dev,
			      struct udc_ep_config *cfg)
{
	__ASSERT_NO_MSG(cfg);
	nrf_usbd_legacy_ep_disable(cfg->addr);
	LOG_DBG("Disable ep 0x%02x", cfg->addr);

	return 0;
}

static int udc_nrf_ep_set_halt(const struct device *dev,
				struct udc_ep_config *cfg)
{
	LOG_DBG("Halt ep 0x%02x", cfg->addr);

	if (cfg->addr == USB_CONTROL_EP_OUT ||
	    cfg->addr == USB_CONTROL_EP_IN) {
		NRF_USBD->TASKS_EP0STALL = 1;
	} else {
		nrf_usbd_legacy_ep_stall(cfg->addr);
	}

	return 0;
}

static int udc_nrf_ep_clear_halt(const struct device *dev,
				struct udc_ep_config *cfg)
{
	LOG_DBG("Clear halt ep 0x%02x", cfg->addr);

	nrf_usbd_legacy_ep_dtoggle_clear(cfg->addr);
	nrf_usbd_legacy_ep_stall_clear(cfg->addr);

	return 0;
}

static int udc_nrf_set_address(const struct device *dev, const uint8_t addr)
{
	/*
	 * If the status stage already finished (which depends entirely on when
	 * the host sends IN token) then NRF_USBD->USBADDR will have the same
	 * address, otherwise it won't (unless new address is unchanged).
	 *
	 * Store the address so the driver can detect address mismatches
	 * between USB stack and USBD peripheral. The mismatches can occur if:
	 *   * SW has high enough latency in SETUP handling, or
	 *   * Host did not issue Status Stage after Set Address request
	 *
	 * The SETUP handling latency is a problem because the Set Address is
	 * automatically handled by device. Because whole Set Address handling
	 * can finish in less than 21 us, the latency required (with perfect
	 * timing) to hit the issue is relatively short (2 ms Set Address
	 * recovery interval + negligible Set Address handling time). If host
	 * sends new SETUP before SW had a chance to read the Set Address one,
	 * the Set Address one will be overwritten without a trace.
	 */
	udc_nrf_address = addr;

	if (udc_nrf_fake_setup) {
		/* Finished handling lost Set Address, now handle the pending
		 * SETUP transfer.
		 */
		k_event_post(&drv_evt, BIT(UDC_NRF_EVT_SETUP));
	}

	return 0;
}

static int udc_nrf_host_wakeup(const struct device *dev)
{
	bool res = nrf_usbd_legacy_wakeup_req();

	LOG_DBG("Host wakeup request");
	if (!res) {
		return -EAGAIN;
	}

	return 0;
}

static int udc_nrf_enable(const struct device *dev)
{
	unsigned int key;
	int ret;

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
				   USB_EP_TYPE_CONTROL, UDC_NRF_EP0_SIZE, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN,
				   USB_EP_TYPE_CONTROL, UDC_NRF_EP0_SIZE, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	sys_notify_init_spinwait(&hfxo_cli.notify);
	ret = onoff_request(hfxo_mgr, &hfxo_cli);
	if (ret < 0) {
		LOG_ERR("Failed to start HFXO %d", ret);
		return ret;
	}

	/* Disable interrupts until USBD is enabled */
	key = irq_lock();
	nrf_usbd_legacy_enable();
	irq_unlock(key);

	return 0;
}

static int udc_nrf_disable(const struct device *dev)
{
	int ret;

	nrf_usbd_legacy_disable();

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	ret = onoff_cancel_or_release(hfxo_mgr, &hfxo_cli);
	if (ret < 0) {
		LOG_ERR("Failed to stop HFXO %d", ret);
		return ret;
	}

	return 0;
}

static int udc_nrf_init(const struct device *dev)
{
	const struct udc_nrf_config *cfg = dev->config;

	hfxo_mgr = z_nrf_clock_control_get_onoff(cfg->clock);

#ifdef CONFIG_HAS_HW_NRF_USBREG
	/* Use CLOCK/POWER priority for compatibility with other series where
	 * USB events are handled by CLOCK interrupt handler.
	 */
	IRQ_CONNECT(USBREGULATOR_IRQn,
		    DT_IRQ(DT_INST(0, nordic_nrf_clock), priority),
		    nrfx_isr, nrfx_usbreg_irq_handler, 0);
#endif

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    nrfx_isr, nrf_usbd_irq_handler, 0);

	(void)nrfx_power_init(&cfg->pwr);
	nrfx_power_usbevt_init(&cfg->evt);

	nrfx_power_usbevt_enable();
	LOG_INF("Initialized");

	return 0;
}

static int udc_nrf_shutdown(const struct device *dev)
{
	LOG_INF("shutdown");

	nrfx_power_usbevt_disable();
	nrfx_power_usbevt_uninit();

	return 0;
}

static int udc_nrf_driver_init(const struct device *dev)
{
	struct udc_data *data = dev->data;
	int err;

	LOG_INF("Preinit");
	udc_nrf_dev = dev;
	k_mutex_init(&data->mutex);
	k_thread_create(&drv_stack_data, drv_stack,
			K_KERNEL_STACK_SIZEOF(drv_stack),
			udc_nrf_thread,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(8), 0, K_NO_WAIT);

	k_thread_name_set(&drv_stack_data, "udc_nrfx");

	for (int i = 0; i < ARRAY_SIZE(ep_cfg_out); i++) {
		ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			ep_cfg_out[i].caps.control = 1;
			ep_cfg_out[i].caps.mps = NRF_USBD_COMMON_EPSIZE;
		} else if (i < (CFG_EPOUT_CNT + 1)) {
			ep_cfg_out[i].caps.bulk = 1;
			ep_cfg_out[i].caps.interrupt = 1;
			ep_cfg_out[i].caps.mps = NRF_USBD_COMMON_EPSIZE;
		} else {
			ep_cfg_out[i].caps.iso = 1;
			ep_cfg_out[i].caps.mps = NRF_USBD_COMMON_ISOSIZE / 2;
		}

		ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (int i = 0; i < ARRAY_SIZE(ep_cfg_in); i++) {
		ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			ep_cfg_in[i].caps.control = 1;
			ep_cfg_in[i].caps.mps = NRF_USBD_COMMON_EPSIZE;
		} else if (i < (CFG_EPIN_CNT + 1)) {
			ep_cfg_in[i].caps.bulk = 1;
			ep_cfg_in[i].caps.interrupt = 1;
			ep_cfg_in[i].caps.mps = NRF_USBD_COMMON_EPSIZE;
		} else {
			ep_cfg_in[i].caps.iso = 1;
			ep_cfg_in[i].caps.mps = NRF_USBD_COMMON_ISOSIZE / 2;
		}

		ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	data->caps.rwup = true;
	data->caps.out_ack = true;
	data->caps.mps0 = UDC_NRF_MPS0;
	data->caps.can_detect_vbus = true;

	return 0;
}

static void udc_nrf_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_nrf_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static const struct udc_nrf_config udc_nrf_cfg = {
	.clock = COND_CODE_1(NRF_CLOCK_HAS_HFCLK192M,
			     (CLOCK_CONTROL_NRF_SUBSYS_HF192M),
			     (CLOCK_CONTROL_NRF_SUBSYS_HF)),
	.pwr = {
		.dcdcen = (DT_PROP(DT_INST(0, nordic_nrf5x_regulator), regulator_initial_mode)
			   == NRF5X_REG_MODE_DCDC),
#if NRFX_POWER_SUPPORTS_DCDCEN_VDDH
		.dcdcenhv = COND_CODE_1(CONFIG_SOC_SERIES_NRF52X,
			(DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nordic_nrf52x_regulator_hv))),
			(DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nordic_nrf53x_regulator_hv)))),
#endif
	},

	.evt = {
		.handler = udc_nrf_power_handler
	},
};

static struct udc_data udc_nrf_data = {
	.mutex = Z_MUTEX_INITIALIZER(udc_nrf_data.mutex),
	.priv = NULL,
};

static const struct udc_api udc_nrf_api = {
	.lock = udc_nrf_lock,
	.unlock = udc_nrf_unlock,
	.init = udc_nrf_init,
	.enable = udc_nrf_enable,
	.disable = udc_nrf_disable,
	.shutdown = udc_nrf_shutdown,
	.set_address = udc_nrf_set_address,
	.host_wakeup = udc_nrf_host_wakeup,
	.ep_try_config = NULL,
	.ep_enable = udc_nrf_ep_enable,
	.ep_disable = udc_nrf_ep_disable,
	.ep_set_halt = udc_nrf_ep_set_halt,
	.ep_clear_halt = udc_nrf_ep_clear_halt,
	.ep_enqueue = udc_nrf_ep_enqueue,
	.ep_dequeue = udc_nrf_ep_dequeue,
};

DEVICE_DT_INST_DEFINE(0, udc_nrf_driver_init, NULL,
		      &udc_nrf_data, &udc_nrf_cfg,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &udc_nrf_api);
