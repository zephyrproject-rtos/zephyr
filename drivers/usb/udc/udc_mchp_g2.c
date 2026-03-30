/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include "udc_common.h"

LOG_MODULE_REGISTER(udc_mchp_g2, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT microchip_usb_g2

/* Event flags for driver thread */
#define MCHP_G2_EVT_SETUP        BIT(0)
#define MCHP_G2_EVT_XFER         BIT(1) /* EP0 transfer (IN, RX-done, Status-IN) */
#define MCHP_G2_EVT_BUS_RESET    BIT(2)
#define MCHP_G2_EVT_SUSPEND      BIT(3)
#define MCHP_G2_EVT_RESUME       BIT(4)
#define MCHP_G2_EVT_VBUS_READY   BIT(5)
#define MCHP_G2_EVT_VBUS_REMOVED BIT(6)
#define MCHP_G2_EVT_BULK_TX      BIT(7) /* EP1-7 IN packet sent */
#define MCHP_G2_EVT_BULK_RX      BIT(8) /* EP1-7 OUT data received */

#define MCHP_G2_FIFO_UNIT_SIZE      8U
#define MCHP_G2_FIFO_PAGE_SIZE      256U
#define MCHP_G2_FIFO_UNITS_PER_PAGE 32U
#define MCHP_G2_FIFO_PAGES          36U
#define MCHP_G2_FIFO_ADDR_INVALID   0xFFFFU
#define MCHP_G2_EP_MAX              16U

/* TXFIFOSZ/RXFIFOSZ SIZE field: powers-of-2, 8B (code 0) to 4096B (code 9). */
#define MCHP_G2_FIFO_SIZE_CODE_INVALID 0xFFU /* No valid code found */

/* EP0 constants */
#define MCHP_G2_SETUP_PACKET_SIZE 8U /* SETUP packet length */

/* SETUP queue depth */
#define MCHP_G2_SETUP_Q_DEPTH 8U
#define MCHP_G2_SETUP_Q_MASK  (MCHP_G2_SETUP_Q_DEPTH - 1U)

/* SETUP packet field offsets: bmRequestType in word0[7:0], wLength in word1[31:16]. */
#define MCHP_G2_SETUP_BMREQTYPE_MASK 0xFFU
#define MCHP_G2_SETUP_WLENGTH_SHIFT  16U
#define MCHP_G2_SETUP_WLENGTH_MASK   0xFFFFU

/* USBHS FIFO port width */
#define MCHP_G2_FIFO_WORD_SIZE 4U

/*
 * USBHPHY calibration values (characterization data, CALOTP FCCFG67 fuse map):
 *
 * Function             Fuse       Size Offset New Value  Register/Bit
 * Squelch               SQUELCH    4d   0d     0x2        PHY04.SQUELCH210 / PHY08.SQUELCH3
 * HS amp tuning         TUNE       8d   4d     0x57       PHY0C.TUNE210 / PHY10.TUNE76543
 * On die term comp      ODT        3d   12d    0x3        PHY14.ODT0 / PHY18.ODT21
 * HS slew rate          HSSLEW     3d   15d    0x2        PHY20.HSSLEW10 / PHY24.HSSLEW2
 * Host disconnect detect DISCONDET 4d   18d    0xD        PHY28.DISCONDET
 * HS drive current comp HSDRVCOMP  3d   22d    0x3        PHY28.HSDRVCOMP
 */
#define MCHP_G2_PHY_CAL_SQUELCH_VAL   0x2U  /* 4-bit: SQUELCH210 (bits 0-2) | SQUELCH3 (bit 3) */
#define MCHP_G2_PHY_CAL_TUNE_VAL      0x57U /* 8-bit: TUNE210 (bits 0-2) | TUNE76543 (bits 3-7) */
#define MCHP_G2_PHY_CAL_ODT_VAL       0x3U  /* 3-bit: ODT0 (bit 0) | ODT21 (bits 1-2) */
#define MCHP_G2_PHY_CAL_HSSLEW_VAL    0x2U  /* 3-bit: HSSLEW10 (bits 0-1) | HSSLEW2 (bit 2) */
#define MCHP_G2_PHY_CAL_DISCONDET_VAL 0xDU  /* 4-bit: PHY28.DISCONDET */
#define MCHP_G2_PHY_CAL_HSDRVCOMP_VAL 0x3U  /* 3-bit: PHY28.HSDRVCOMP */

/* Max bulk/interrupt endpoint MPS per speed */
#define MCHP_G2_FS_EP_MPS_MAX 1023U
#define MCHP_G2_HS_EP_MPS_MAX 1024U

/* Timeout loop counts (1 us per iteration) */
#define MCHP_G2_SYNC_TIMEOUT_ITER      10000
#define MCHP_G2_PHY_READY_TIMEOUT_ITER 100000

/* Hardware timing constants */
#define MCHP_G2_AVREGEN_SETTLE_US 55U /* AVREGEN stabilisation time */
#define MCHP_G2_PHY_SETTLE_US     10U /* PHY settle after HSENABLE */
#define MCHP_G2_RESUME_KSTATE_MS  2   /* K-state hold for remote wakeup */

/* EP0 state machine states */
enum udc_mchp_g2_ep0_state {
	EP0_STATE_IDLE = 0,  /* Waiting for SETUP packet */
	EP0_STATE_TX,        /* Control Read: sending IN data */
	EP0_STATE_RX,        /* Control Write: receiving OUT data */
	EP0_STATE_STATUS_IN, /* Status stage: waiting for ZLP confirmation */
};

/* Driver static configuration */
struct udc_mchp_g2_config {
	uint32_t base;
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	k_thread_stack_t *thread_stk;
	size_t thread_stk_sz;
	int speed_idx;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	uint32_t vbus_poll_ms;
};

/* Driver private data */
struct udc_mchp_g2_data {
	struct k_thread thread_data;
	struct k_event events;

	/* SETUP packets queue.*/
	uint8_t setup_q[MCHP_G2_SETUP_Q_DEPTH][MCHP_G2_SETUP_PACKET_SIZE];
	uint8_t setup_q_head;
	uint8_t setup_q_tail;
	enum udc_mchp_g2_ep0_state ep0_state;
	uint32_t fifo_allocation_table[MCHP_G2_FIFO_PAGES];
	uint16_t fifo_in_addr[MCHP_G2_EP_MAX];
	uint16_t fifo_out_addr[MCHP_G2_EP_MAX];

	atomic_t bulk_tx_done;    /* EP1-7 IN packet sent */
	atomic_t bulk_rx_done;    /* EP1-7 OUT data received */
	atomic_t ep0_rx_done;     /* EP0 Control Write ready */
	atomic_t bulk_rx_nak;     /* RxPktRdy held; data in sw buf */
	atomic_t bulk_rx_pending; /* RxPktRdy held; data in HW FIFO */

	struct net_buf *ep0_rx_buf; /* data buf provided by class; written directly by ISR */
	atomic_t ep0_rx_pending;    /* RxPktRdy held; data buf not yet available */
	uint16_t ep0_ctrl_write_len;
	uint16_t ep0_ctrl_bytes_received;
	uint8_t ep0_ctrl_write_setup[MCHP_G2_SETUP_PACKET_SIZE];

	const struct device *dev;
	struct k_work_delayable vbus_work;
	bool vbus_present;
};

/* Round size up to the nearest power-of-2 FIFO code (8B..4096B); INVALID if size > 4096. */
static uint8_t mchp_g2_fifo_size_code(uint16_t size)
{
	uint16_t fifo_size = MCHP_G2_FIFO_UNIT_SIZE;
	uint8_t code = 0U;

	while ((fifo_size < size) && (code < USBHS_TXFIFOSZ_SZ_4096BYTES_Val)) {
		fifo_size <<= 1;
		code++;
	}

	if (fifo_size < size) {
		return MCHP_G2_FIFO_SIZE_CODE_INVALID;
	}

	return code;
}

/* Allocate a contiguous FIFO block for an endpoint. Returns address or INVALID. */
static uint16_t mchp_g2_fifo_allocate(struct udc_mchp_g2_data *priv, uint16_t size)
{
	uint32_t required;
	uint32_t total_units;
	uint32_t start_idx = 0U;
	uint32_t counted = 0U;
	uint32_t i;
	bool is_used;

	if (size == 0U || (size % MCHP_G2_FIFO_UNIT_SIZE) != 0U) {
		return MCHP_G2_FIFO_ADDR_INVALID;
	}

	required = size / MCHP_G2_FIFO_UNIT_SIZE;
	total_units = MCHP_G2_FIFO_PAGES * MCHP_G2_FIFO_UNITS_PER_PAGE;

	/* Scan for a contiguous free run of the required length. */
	for (i = 0U; i < total_units; i++) {
		is_used = (priv->fifo_allocation_table[i / MCHP_G2_FIFO_UNITS_PER_PAGE] &
			   BIT(i % MCHP_G2_FIFO_UNITS_PER_PAGE)) != 0U;

		if (!is_used) {
			if (counted == 0U) {
				start_idx = i;
			}
			if (++counted == required) {
				break;
			}
		} else {
			counted = 0U;
		}
	}

	if (counted < required) {
		return MCHP_G2_FIFO_ADDR_INVALID;
	}

	/* Mark allocated units. */
	for (i = start_idx; i < (start_idx + required); i++) {
		priv->fifo_allocation_table[i / MCHP_G2_FIFO_UNITS_PER_PAGE] |=
			BIT(i % MCHP_G2_FIFO_UNITS_PER_PAGE);
	}

	return (uint16_t)(start_idx * MCHP_G2_FIFO_UNIT_SIZE);
}

/* Process completed EP1-7 IN transfers. */
static void mchp_g2_handle_bulk_tx(const struct device *dev)
{
	const struct udc_mchp_g2_config *cfg = dev->config;
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	usbhs_registers_t *regs = (usbhs_registers_t *)cfg->base;
	uint32_t done = (uint32_t)atomic_clear(&priv->bulk_tx_done);
	uint8_t ep_idx;
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	struct udc_buf_info *bi;
	unsigned int key;
	uint16_t mps;
	volatile uint8_t *fifo;
	size_t chunk;
	size_t i;

	while (done != 0U) {
		ep_idx = (uint8_t)(find_lsb_set(done) - 1);

		done &= ~BIT(ep_idx);

		ep_cfg = udc_get_ep_cfg(dev, USB_EP_DIR_IN | ep_idx);
		buf = ep_cfg ? udc_buf_peek(ep_cfg) : NULL;

		if (buf == NULL) {
			continue;
		}

		if (buf->len > 0U) {
			/* Skip while halted; ep_clear_halt will re-arm. */
			if (ep_cfg->stat.halted) {
				continue;
			}

			/* Write next chunk to FIFO. */
			key = irq_lock();
			mps = udc_mps_ep_size(ep_cfg);
			fifo = (volatile uint8_t *)&regs->ENDPOINT0.USBHS_FIFOX[ep_idx];

			regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep_idx);
			chunk = MIN(buf->len, (size_t)mps);

			for (i = 0; i < chunk; i++) {
				*fifo = buf->data[i];
			}
			net_buf_pull(buf, chunk);
			regs->ENDPOINTX.USBHS_TXCSRL |= USBHS_ENDPOINTX_TXCSRL_TXPKTRDY_Msk;
			regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
			irq_unlock(key);
		} else {
			bi = udc_get_buf_info(buf);

			if (bi->zlp != 0U) {
				/* Skip ZLP while halted; ep_clear_halt will re-arm. */
				if (ep_cfg->stat.halted) {
					continue;
				}
				key = irq_lock();

				regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep_idx);
				regs->ENDPOINTX.USBHS_TXCSRL |= USBHS_ENDPOINTX_TXCSRL_TXPKTRDY_Msk;
				bi->zlp = 0;
				regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
				irq_unlock(key);
			} else {
				/* Dequeue completed transfer. */
				udc_submit_ep_event(dev, udc_buf_get(ep_cfg), 0);
			}
		}
	}
}

/* Handle EP0 Control Write completion. */
static void mchp_g2_handle_ep0_rx_done(const struct device *dev)
{
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep0_out = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct udc_ep_config *ep_in = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	struct net_buf *setup_hold = NULL;
	struct net_buf *out_buf;
	struct net_buf *status;

	/* Remove the SETUP buffer from the queue if it's still at the head. */
	out_buf = ep0_out ? udc_buf_peek(ep0_out) : NULL;

	if ((out_buf != NULL) && (udc_get_buf_info(out_buf)->setup != 0U)) {
		setup_hold = udc_buf_get(ep0_out);
		out_buf = ep0_out ? udc_buf_peek(ep0_out) : NULL;
	}

	/* Data buf not yet queued; re-deliver SETUP to the class. */
	if ((out_buf == NULL) || (udc_get_buf_info(out_buf)->setup != 0U)) {
		if (setup_hold != NULL) {
			memset(setup_hold->data, 0, MCHP_G2_SETUP_PACKET_SIZE);
			setup_hold->len = 0;
			udc_buf_put(ep0_out, setup_hold);
			setup_hold = NULL;
		}
		udc_setup_received(dev, priv->ep0_ctrl_write_setup);
		out_buf = ep0_out ? udc_buf_peek(ep0_out) : NULL;
	}

	/* Submit the completed OUT transfer. */
	if (out_buf != NULL && udc_get_buf_info(out_buf)->setup == 0U) {
		priv->ep0_rx_buf = NULL;
		out_buf = udc_buf_get(ep0_out);
		udc_submit_ep_event(dev, out_buf, 0);

		/* Drain status ZLP. */
		status = udc_buf_peek(ep_in);

		if (status != NULL && udc_get_buf_info(status)->status != 0U) {
			udc_submit_ep_event(dev, udc_buf_get(ep_in), 0);
		}
	}

	priv->ep0_ctrl_bytes_received = 0U;
	if (setup_hold != NULL) {
		memset(setup_hold->data, 0, MCHP_G2_SETUP_PACKET_SIZE);
		setup_hold->len = 0;
		udc_buf_put(ep0_out, setup_hold);
	}
}

/* Write next chunk of EP0 IN data to FIFO and arm TxPktRdy. */
static void mchp_g2_handle_ep0_tx(const struct device *dev, struct net_buf *buf)
{
	const struct udc_mchp_g2_config *cfg = dev->config;
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	usbhs_registers_t *regs = (usbhs_registers_t *)cfg->base;
	uint16_t mps = udc_mps_ep_size(udc_get_ep_cfg(dev, USB_CONTROL_EP_IN));
	volatile uint8_t *fifo = (volatile uint8_t *)&regs->ENDPOINT0.USBHS_FIFOX[0];
	size_t chunk;
	size_t i;
	uint8_t csr0;

	/* Write next chunk */
	chunk = MIN(buf->len, (size_t)mps);

	for (i = 0; i < chunk; i++) {
		*fifo = buf->data[i];
	}
	net_buf_pull(buf, chunk);

	csr0 = USBHS_ENDPOINT0_CSR0L_TXPKTRDY_Msk;

	/* Set DataEnd on the last packet, including a deferred trailing ZLP
	 * when the previous packet was an exact multiple of the EP0 MPS.
	 */
	if (buf->len == 0) {
		csr0 |= USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_DATAEND_Msk;
		udc_get_buf_info(buf)->zlp = 0U;
	}
	priv->ep0_state = EP0_STATE_TX;
	regs->ENDPOINT0.USBHS_CSR0L |= csr0;
}

/* Resume OUT transfers if next buffer is pre-queued */
static void udc_mchp_g2_resume_out(usbhs_registers_t *regs, atomic_t *bulk_rx_nak, uint8_t ep_idx)
{
	unsigned int key;

	if (atomic_test_and_clear_bit(bulk_rx_nak, ep_idx)) {
		key = irq_lock();
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep_idx);
		regs->ENDPOINTX.USBHS_RXCSRL &= ~USBHS_ENDPOINTX_RXCSRL_RXPKTRDY_Msk;
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
		irq_unlock(key);
	}
}

/* Driver thread: waits for events and dispatches handlers. */
static void mchp_g2_thread_handler(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = (const struct device *)arg1;
	const struct udc_mchp_g2_config *cfg = dev->config;
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	usbhs_registers_t *regs = (usbhs_registers_t *)cfg->base;
	uint32_t events;
	uint32_t done;
	uint8_t ep_idx;
	struct udc_ep_config *ep_cfg;
	struct net_buf *rx_buf;
	struct udc_ep_config *ep_in;
	struct net_buf *buf;

	while (true) {
		events = k_event_wait(&priv->events,
				      MCHP_G2_EVT_SETUP | MCHP_G2_EVT_XFER | MCHP_G2_EVT_BUS_RESET |
					      MCHP_G2_EVT_SUSPEND | MCHP_G2_EVT_RESUME |
					      MCHP_G2_EVT_BULK_TX | MCHP_G2_EVT_BULK_RX,
				      false, K_FOREVER);

		if ((events & MCHP_G2_EVT_BUS_RESET) != 0U) {
			k_event_clear(&priv->events, MCHP_G2_EVT_BUS_RESET);
			udc_submit_event(dev, UDC_EVT_RESET, 0);
		}

		if ((events & MCHP_G2_EVT_SUSPEND) != 0U) {
			k_event_clear(&priv->events, MCHP_G2_EVT_SUSPEND);
			udc_set_suspended(dev, true);
			udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
		}

		if ((events & MCHP_G2_EVT_RESUME) != 0U) {
			k_event_clear(&priv->events, MCHP_G2_EVT_RESUME);
			udc_set_suspended(dev, false);
			udc_submit_event(dev, UDC_EVT_RESUME, 0);
		}

		/* EP1-7 TX done */
		if ((events & MCHP_G2_EVT_BULK_TX) != 0U) {
			k_event_clear(&priv->events, MCHP_G2_EVT_BULK_TX);
			mchp_g2_handle_bulk_tx(dev);
		}

		/* EP1-7 RX done */
		if ((events & MCHP_G2_EVT_BULK_RX) != 0U) {
			k_event_clear(&priv->events, MCHP_G2_EVT_BULK_RX);

			done = (uint32_t)atomic_clear(&priv->bulk_rx_done);

			while (done != 0U) {
				ep_idx = (uint8_t)(find_lsb_set(done) - 1);

				done &= ~BIT(ep_idx);

				ep_cfg = udc_get_ep_cfg(dev, (uint8_t)ep_idx);

				if (ep_cfg == NULL) {
					continue;
				}

				rx_buf = udc_buf_get(ep_cfg);

				if (rx_buf != NULL) {
					/* Resume OUT transfers if next buffer is pre-queued */
					if (atomic_test_bit(&priv->bulk_rx_nak, ep_idx) &&
					    (udc_buf_peek(ep_cfg) != NULL)) {
						udc_mchp_g2_resume_out(regs, &priv->bulk_rx_nak,
								       ep_idx);
					}

					udc_submit_ep_event(dev, rx_buf, 0);
				}
			}
		}

		if ((events & MCHP_G2_EVT_XFER) != 0U) {
			k_event_clear(&priv->events, MCHP_G2_EVT_XFER);
			ep_in = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);

			if (atomic_clear(&priv->ep0_rx_done) != 0) {
				mchp_g2_handle_ep0_rx_done(dev);
			}

			buf = ep_in ? udc_buf_peek(ep_in) : NULL;

			if (buf != NULL && (buf->len > 0 || udc_get_buf_info(buf)->zlp != 0U)) {
				mchp_g2_handle_ep0_tx(dev, buf);
			} else if (buf != NULL) {
				udc_submit_ep_event(dev, udc_buf_get(ep_in), 0);
			}
		}

		if ((events & MCHP_G2_EVT_SETUP) != 0U) {
			uint8_t setup[MCHP_G2_SETUP_PACKET_SIZE];
			unsigned int key;
			bool pending;

			k_event_clear(&priv->events, MCHP_G2_EVT_SETUP);

			/* Drain every queued SETUP in arrival order. Copy out under
			 * lock so the ISR cannot overwrite the slot mid-read.
			 */
			while (true) {
				key = irq_lock();
				pending = (priv->setup_q_tail != priv->setup_q_head);
				if (pending) {
					memcpy(setup,
					       priv->setup_q[priv->setup_q_tail &
							     MCHP_G2_SETUP_Q_MASK],
					       sizeof(setup));
					priv->setup_q_tail++;
				}
				irq_unlock(key);

				if (!pending) {
					break;
				}

				LOG_DBG("SETUP packet received: %02x %02x %02x %02x "
					"%02x %02x %02x %02x",
					setup[0], setup[1], setup[2], setup[3], setup[4], setup[5],
					setup[6], setup[7]);
				udc_setup_received(dev, setup);
			}
		}
	}
}

/* Returns true if VBUS is present (DEVCTL.VBUS == AVBUSVALID). */
static bool mchp_g2_vbus_is_present(usbhs_registers_t *regs)
{
	uint8_t vbus = regs->ENDPOINT0.USBHS_DEVCTL & USBHS_DEVCTL_VBUS_Msk;

	return (vbus == USBHS_DEVCTL_VBUS_AVBUSVALID);
}

/* VBUS polling work handler. Fires VBUS_READY/REMOVED events on state change. */
static void mchp_g2_vbus_poll_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct udc_mchp_g2_data *priv = CONTAINER_OF(dwork, struct udc_mchp_g2_data, vbus_work);
	const struct udc_mchp_g2_config *cfg = priv->dev->config;
	usbhs_registers_t *regs = (usbhs_registers_t *)cfg->base;
	const bool present = mchp_g2_vbus_is_present(regs);

	if (present != priv->vbus_present) {
		priv->vbus_present = present;

		if (present) {
			LOG_INF("VBUS detected");
			udc_submit_event(priv->dev, UDC_EVT_VBUS_READY, 0);
		} else {
			LOG_INF("VBUS removed");
			udc_submit_event(priv->dev, UDC_EVT_VBUS_REMOVED, 0);
		}
	}

	k_work_reschedule(dwork, K_MSEC(cfg->vbus_poll_ms));
}

/* Read len bytes from a USBHS FIFO using 32-bit word accesses. */
static void mchp_g2_fifo_read(volatile uint32_t *fifo, uint8_t *dst, uint32_t len)
{
	uint32_t words = len / MCHP_G2_FIFO_WORD_SIZE;
	uint32_t rem = len % MCHP_G2_FIFO_WORD_SIZE;
	uint32_t i;
	uint32_t j;
	uint32_t word;

	for (i = 0U; i < words; i++) {
		word = *fifo;

		*dst++ = (uint8_t)(word);
		*dst++ = (uint8_t)(word >> 8U);
		*dst++ = (uint8_t)(word >> 16U);
		*dst++ = (uint8_t)(word >> 24U);
	}

	/* Remaining bytes (< 4). */
	if (rem > 0U) {
		word = *fifo;

		for (j = 0U; j < rem; j++) {
			*dst++ = (uint8_t)(word >> (j * 8U));
		}
	}
}

static void mchp_g2_handle_phyrdy(usbhs_registers_t *regs)
{
	regs->ENDPOINT0.USBHS_INTFLAG = USBHS_INTFLAG_PHYRDY_Msk;
	regs->ENDPOINT0.USBHS_INTENCLR = USBHS_INTENSET_PHYRDY_Msk;
}

/*
 * Handle USB bus events (reset, suspend, resume, SOF).
 * Returns true on bus reset so the ISR exits immediately.
 */
static bool mchp_g2_handle_usb_events(usbhs_registers_t *regs, struct udc_mchp_g2_data *priv,
				      uint8_t intrusb)
{
	regs->ENDPOINT0.USBHS_INTFLAG = USBHS_INTFLAG_USB_Msk;

	if ((intrusb & USBHS_INTRUSB_RESET_Msk) != 0U) {
		priv->ep0_state = EP0_STATE_IDLE;
		/* Drop any SETUPs queued before the reset. */
		priv->setup_q_tail = priv->setup_q_head;
		priv->ep0_rx_buf = NULL;
		priv->ep0_ctrl_write_len = 0U;
		priv->ep0_ctrl_bytes_received = 0U;
		atomic_clear(&priv->ep0_rx_pending);
		/* Clear stale bulk endpoint state on reset */
		atomic_clear(&priv->bulk_tx_done);
		atomic_clear(&priv->bulk_rx_done);
		atomic_clear(&priv->bulk_rx_nak);
		atomic_clear(&priv->bulk_rx_pending);
		/* Restore EP0 interrupt and FIFO config cleared by hardware on reset. */
		regs->ENDPOINT0.USBHS_INTRTXE |= USBHS_INTRTXE_EP0TXEN_Msk;
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
		regs->ENDPOINT0.USBHS_TXFIFOSZ =
			USBHS_TXFIFOSZ_SZ(mchp_g2_fifo_size_code(USB_CONTROL_EP_MPS));
		regs->ENDPOINT0.USBHS_RXFIFOSZ =
			USBHS_RXFIFOSZ_SZ(mchp_g2_fifo_size_code(USB_CONTROL_EP_MPS));
		regs->ENDPOINT0.USBHS_TXFIFOADD =
			USBHS_TXFIFOADD_ADDR(priv->fifo_in_addr[0] / MCHP_G2_FIFO_UNIT_SIZE);
		regs->ENDPOINT0.USBHS_RXFIFOADD =
			USBHS_RXFIFOADD_ADDR(priv->fifo_out_addr[0] / MCHP_G2_FIFO_UNIT_SIZE);

		k_event_post(&priv->events, MCHP_G2_EVT_BUS_RESET);
		return true;
	}

	if ((intrusb & USBHS_INTRUSB_SUSPEND_Msk) != 0U) {
		k_event_post(&priv->events, MCHP_G2_EVT_SUSPEND);
	}

	if ((intrusb & USBHS_INTRUSB_RESUME_Msk) != 0U) {
		k_event_post(&priv->events, MCHP_G2_EVT_RESUME);
	}

	if (IS_ENABLED(CONFIG_UDC_ENABLE_SOF) && ((intrusb & USBHS_INTRUSB_SOF_Msk) != 0U)) {
		udc_submit_sof_event(priv->dev);
	}

	return false;
}

/*
 * Read EP0 CSR0L and resolve SentStall/SetupEnd before the state machine.
 * Returns true if SetupEnd was cleared (ISR should exit).
 * Never clears SendStall - doing so before the STALL is sent would cancel it.
 */
static bool mchp_g2_handle_ep0_csr(usbhs_registers_t *regs, struct udc_mchp_g2_data *priv,
				   uint8_t *csr0L_out)
{
	uint8_t csr0L;

	/* Restore INDEX=0; a bulk ep_enqueue may have left it non-zero. */
	regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
	csr0L = regs->ENDPOINT0.USBHS_CSR0L;

	/* Clear SentStall; hardware does not auto-clear it. Fall through to handle any pending
	 * SETUP.
	 */
	if ((csr0L & USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SENTSTALL_Msk) != 0U) {
		regs->ENDPOINT0.USBHS_CSR0L &= ~USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SENTSTALL_Msk;
		priv->ep0_state = EP0_STATE_IDLE;
		csr0L = regs->ENDPOINT0.USBHS_CSR0L; /* re-read after clear */
	}

	if ((csr0L & USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SETUPEND_Msk) != 0U) {
		/* Clear SetupEnd. */
		regs->ENDPOINT0.USBHS_CSR0L |=
			USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SERVICEDSETUPEND_Msk;
		priv->ep0_state = EP0_STATE_IDLE;
		/* Re-read CSR0L: a new SETUP may already be waiting in the FIFO. */
		csr0L = regs->ENDPOINT0.USBHS_CSR0L;
		/* Fall through to process any pending SETUP in IDLE state. */
	}

	*csr0L_out = csr0L;
	return false;
}

/*
 * IDLE state: read SETUP packet and advance EP0 state.
 * wLen==0 : STATUS_IN, bmReqType[7] : TX (Control Read), else : RX.
 */
static void mchp_g2_ep0_state_idle(usbhs_registers_t *regs, struct udc_mchp_g2_data *priv,
				   uint8_t csr0L)
{
	volatile uint32_t *fifo;
	uint32_t *setup_ptr;
	uint8_t *setup;
	uint8_t bmReqType;
	uint16_t wLen;

	if ((csr0L & USBHS_ENDPOINT0_CSR0L_RXPKTRDY_Msk) == 0U) {
		return;
	}

	/* Read SETUP packet from FIFO into the next queue slot. If the queue is
	 * full, drop the oldest entry; the thread only touches the tail under
	 * irq_lock(), so this cannot race with it.
	 */
	if ((uint8_t)(priv->setup_q_head - priv->setup_q_tail) >= MCHP_G2_SETUP_Q_DEPTH) {
		priv->setup_q_tail++;
	}

	setup = priv->setup_q[priv->setup_q_head & MCHP_G2_SETUP_Q_MASK];

	fifo = (volatile uint32_t *)&regs->ENDPOINT0.USBHS_FIFOX[0];
	setup_ptr = (uint32_t *)setup;

	setup_ptr[0] = *fifo;
	setup_ptr[1] = *fifo;

	bmReqType = (uint8_t)(setup_ptr[0] & MCHP_G2_SETUP_BMREQTYPE_MASK);
	wLen = (uint16_t)((setup_ptr[1] >> MCHP_G2_SETUP_WLENGTH_SHIFT) &
			  MCHP_G2_SETUP_WLENGTH_MASK);

	if (wLen == 0) {
		/* Zero-data request (e.g. SET_ADDRESS). ACK and move to STATUS_IN. */
		regs->ENDPOINT0.USBHS_CSR0L |=
			USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SERVICEDRXPKTRDY_Msk |
			USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_DATAEND_Msk;
		priv->ep0_state = EP0_STATE_STATUS_IN;
	} else if ((bmReqType & USB_EP_DIR_IN) != 0U) {
		/* Control Read (IN) */
		regs->ENDPOINT0.USBHS_CSR0L |=
			USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SERVICEDRXPKTRDY_Msk;
		priv->ep0_state = EP0_STATE_TX;
	} else {
		/* Control Write (OUT) */
		regs->ENDPOINT0.USBHS_CSR0L |=
			USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SERVICEDRXPKTRDY_Msk;
		priv->ep0_state = EP0_STATE_RX;
		/* Snapshot SETUP bytes for the control-write data phase. */
		memcpy(priv->ep0_ctrl_write_setup, setup, MCHP_G2_SETUP_PACKET_SIZE);
		priv->ep0_ctrl_write_len = wLen;
		/* Reset per-transfer state; the thread may not have processed the
		 * previous transfer before the host issues the next SETUP.
		 */
		priv->ep0_ctrl_bytes_received = 0U;
		priv->ep0_rx_buf = NULL;
		atomic_clear(&priv->ep0_rx_pending);
	}

	/* Publish the slot to the thread only after it is fully written. */
	priv->setup_q_head++;
	k_event_post(&priv->events, MCHP_G2_EVT_SETUP);
}

/* STATUS_IN: Status ZLP ACKed by host. Guard on INTRTX[0] to ignore spurious events. */
static void mchp_g2_ep0_state_status_in(struct udc_mchp_g2_data *priv,
					uint16_t endpointTXInterrupts)
{
	if ((endpointTXInterrupts & USBHS_INTRTX_EP0TX_Msk) == 0U) {
		return; /* Not a real EP0 event. */
	}

	priv->ep0_state = EP0_STATE_IDLE;
	k_event_post(&priv->events, MCHP_G2_EVT_XFER);
}

/*
 * TX state: EP0 IN packet sent. Guard on INTRTX[0] to ignore spurious events.
 * Post EVT_XFER when TxPktRdy clears so the thread sends the next chunk.
 */
static void mchp_g2_ep0_state_tx(struct udc_mchp_g2_data *priv, uint8_t csr0L,
				 uint16_t endpointTXInterrupts)
{
	if ((endpointTXInterrupts & USBHS_INTRTX_EP0TX_Msk) == 0U) {
		return; /* Spurious event. */
	}

	if ((csr0L & USBHS_ENDPOINT0_CSR0L_TXPKTRDY_Msk) == 0U) {
		priv->ep0_state = EP0_STATE_IDLE;
		k_event_post(&priv->events, MCHP_G2_EVT_XFER);
	}
}

/*
 * RX state: receive Control Write data directly into the UDC-provided buffer.
 */
static void mchp_g2_ep0_state_rx(const struct device *dev, usbhs_registers_t *regs,
				 struct udc_mchp_g2_data *priv, uint8_t csr0L)
{
	uint8_t count;
	volatile uint32_t *fifo;
	uint32_t size;
	struct net_buf *buf;
	bool last;

	if ((csr0L & USBHS_ENDPOINT0_CSR0L_RXPKTRDY_Msk) == 0U) {
		return;
	}

	count = regs->ENDPOINT0.USBHS_COUNT0 & USBHS_ENDPOINT0_COUNT0_EP0RXCOUNT_Msk;
	buf = priv->ep0_rx_buf;

	if (count > 0U) {
		if (buf == NULL) {
			/* No buffer yet; hold RxPktRdy until ep_enqueue arms ep0_rx_buf. */
			atomic_set(&priv->ep0_rx_pending, 1);
			return;
		}

		fifo = (volatile uint32_t *)&regs->ENDPOINT0.USBHS_FIFOX[0];
		size = MIN((uint32_t)count, net_buf_tailroom(buf));
		mchp_g2_fifo_read(fifo, net_buf_add(buf, size), size);
		priv->ep0_ctrl_bytes_received += (uint16_t)count;
	}

	/* Transfer complete on short packet OR when all expected bytes received. */
	last = ((uint16_t)count < USB_CONTROL_EP_MPS) ||
	       (priv->ep0_ctrl_bytes_received >= priv->ep0_ctrl_write_len);

	if (last) {
		/* Last packet: ACK with DataEnd to arm Status IN ZLP. */
		regs->ENDPOINT0.USBHS_CSR0L |=
			USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SERVICEDRXPKTRDY_Msk |
			USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_DATAEND_Msk;

		priv->ep0_state = EP0_STATE_STATUS_IN;
		atomic_set(&priv->ep0_rx_done, 1);
	} else {
		/* More data expected; ACK without DataEnd. */
		regs->ENDPOINT0.USBHS_CSR0L |=
			USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SERVICEDRXPKTRDY_Msk;
	}

	k_event_post(&priv->events, MCHP_G2_EVT_XFER);
}

/* Handle EP1-7 IN completions. SentStall shares the TX interrupt bit; clear and flush if set. */
static void mchp_g2_handle_epx_tx_done(struct udc_mchp_g2_data *priv, usbhs_registers_t *regs,
				       uint16_t endpointTXInterrupts)
{
	uint16_t tx = endpointTXInterrupts >> 1; /* shift out EP0 bit */
	uint8_t ep = 1U;
	bool any_done = false;
	unsigned int key;
	uint8_t txcsrl;

	while (tx != 0U) {
		if ((tx & 1U) != 0U) {
			key = irq_lock();

			regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep);

			txcsrl = regs->ENDPOINTX.USBHS_TXCSRL;

			if ((txcsrl & USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_SENTSTALL_Msk) != 0U) {
				/* Clear SentStall, flush FIFO, dequeue aborted buffer. */
				regs->ENDPOINTX.USBHS_TXCSRL &=
					~USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_SENTSTALL_Msk;
				regs->ENDPOINTX.USBHS_TXCSRL |=
					USBHS_ENDPOINTX_TXCSRL_FLUSHFIFO_Msk;
				if ((regs->ENDPOINTX.USBHS_TXCSRL &
				     USBHS_ENDPOINTX_TXCSRL_FIFONOTEMPTY_Msk) != 0U) {
					regs->ENDPOINTX.USBHS_TXCSRL |=
						USBHS_ENDPOINTX_TXCSRL_FLUSHFIFO_Msk;
				}
				regs->ENDPOINTX.USBHS_TXCSRL &=
					~USBHS_ENDPOINTX_TXCSRL_TXPKTRDY_Msk;
			}
			atomic_set_bit(&priv->bulk_tx_done, ep);
			any_done = true;

			regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
			irq_unlock(key);
		}
		tx >>= 1;
		ep++;
	}

	if (any_done) {
		k_event_post(&priv->events, MCHP_G2_EVT_BULK_TX);
	}
}

/*
 * Handle EP1-7 OUT data. For each active endpoint:
 *   short packet or ZLP : clear RxPktRdy, mark done.
 *   full-MPS, buffer full : hold RxPktRdy (NAK host), mark done.
 *   full-MPS, buffer not full : clear RxPktRdy, accumulate.
 *   no buffer : clear RxPktRdy (drop bytes).
 */
static void mchp_g2_handle_epx_rx_done(const struct device *dev, usbhs_registers_t *regs,
				       struct udc_mchp_g2_data *priv, uint16_t endpointRXInterrupts)
{
	uint16_t rx = endpointRXInterrupts >> 1; /* bit 0 unused */
	uint8_t ep = 1U;
	unsigned int key;
	uint8_t rxcsrl;
	uint16_t count;
	struct udc_ep_config *ep_cfg;
	struct net_buf *rx_buf;
	volatile uint32_t *fifo;
	uint32_t size;
	uint8_t *data;
	uint16_t ep_mps;
	bool short_packet;
	bool buf_full;

	while (rx != 0U) {
		if ((rx & 1U) != 0U) {
			key = irq_lock();

			regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep);

			rxcsrl = regs->ENDPOINTX.USBHS_RXCSRL;

			if ((rxcsrl & USBHS_ENDPOINTX_RXCSRL_PERIPHERAL_EPX_SENTSTALL_Msk) != 0U) {
				/* SentStall: clear bit and skip FIFO. */
				regs->ENDPOINTX.USBHS_RXCSRL =
					rxcsrl &
					~USBHS_ENDPOINTX_RXCSRL_PERIPHERAL_EPX_SENTSTALL_Msk;
				regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
				irq_unlock(key);
			} else {
				count = regs->ENDPOINTX.USBHS_RXCOUNT &
					USBHS_ENDPOINTX_RXCOUNT_ENDPOINTRXCOUNT_Msk;

				ep_cfg = udc_get_ep_cfg(dev, (uint8_t)ep);
				rx_buf = ep_cfg ? udc_buf_peek(ep_cfg) : NULL;

				if (rx_buf != NULL && count > 0U) {
					fifo = (volatile uint32_t *)&regs->ENDPOINT0
						       .USBHS_FIFOX[ep];
					size = MIN((uint32_t)count, net_buf_tailroom(rx_buf));
					data = net_buf_add(rx_buf, size);

					mchp_g2_fifo_read(fifo, data, size);

					ep_mps = udc_mps_ep_size(ep_cfg);
					short_packet = ((uint16_t)count < ep_mps);
					buf_full = (net_buf_tailroom(rx_buf) == 0U);

					if (short_packet) {
						/* End of transfer; ACK. */
						regs->ENDPOINTX.USBHS_RXCSRL &=
							~USBHS_ENDPOINTX_RXCSRL_RXPKTRDY_Msk;
						atomic_set_bit(&priv->bulk_rx_done, ep);
					} else if (buf_full) {
						/* Buffer full; hold NAK until new buffer queued. */
						atomic_set_bit(&priv->bulk_rx_nak, ep);
						atomic_set_bit(&priv->bulk_rx_done, ep);
					} else {
						/* Buffer not full; ACK and continue accumulating.
						 */
						regs->ENDPOINTX.USBHS_RXCSRL &=
							~USBHS_ENDPOINTX_RXCSRL_RXPKTRDY_Msk;
					}
				} else if (rx_buf != NULL && count == 0U) {
					/* ZLP: transfer complete, submit accumulated data. */
					regs->ENDPOINTX.USBHS_RXCSRL &=
						~USBHS_ENDPOINTX_RXCSRL_RXPKTRDY_Msk;
					atomic_set_bit(&priv->bulk_rx_done, ep);
				} else {
					/* No buffer; hold RxPktRdy, drain on next enqueue. */
					atomic_set_bit(&priv->bulk_rx_pending, ep);
				}

				regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
				irq_unlock(key);
			}
		}
		rx >>= 1;
		ep++;
	}

	if ((endpointRXInterrupts & USBHS_INTRRXE_Msk) != 0U) {
		k_event_post(&priv->events, MCHP_G2_EVT_BULK_RX);
	}
}

/* Dispatch to the EP0 state machine handler for the current state. */
static void mchp_g2_handle_ep0_state(const struct device *dev, usbhs_registers_t *regs,
				     struct udc_mchp_g2_data *priv, uint8_t csr0L,
				     uint16_t endpointTXInterrupts)
{
	switch (priv->ep0_state) {
	case EP0_STATE_IDLE:
		mchp_g2_ep0_state_idle(regs, priv, csr0L);
		break;
	case EP0_STATE_STATUS_IN:
		mchp_g2_ep0_state_status_in(priv, endpointTXInterrupts);
		break;
	case EP0_STATE_TX:
		mchp_g2_ep0_state_tx(priv, csr0L, endpointTXInterrupts);
		break;
	case EP0_STATE_RX:
		mchp_g2_ep0_state_rx(dev, regs, priv, csr0L);
		break;
	}
}

static void mchp_g2_isr_handler(const struct device *dev)
{
	const struct udc_mchp_g2_config *config = dev->config;
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	usbhs_registers_t *regs = (usbhs_registers_t *)config->base;

	uint32_t int_flags;
	uint8_t intrusb, csr0L;

	int_flags = regs->ENDPOINT0.USBHS_INTFLAG;
	intrusb = regs->ENDPOINT0.USBHS_INTRUSB;
	uint16_t endpointTXInterrupts = (uint16_t)0;
	uint16_t endpointRXInterrupts = (uint16_t)0;

	endpointTXInterrupts = regs->ENDPOINT0.USBHS_INTRTX;
	endpointRXInterrupts = regs->ENDPOINT0.USBHS_INTRRX;

	if ((int_flags & USBHS_INTFLAG_PHYRDY_Msk) != 0) {
		mchp_g2_handle_phyrdy(regs);
	}

	if ((int_flags & USBHS_INTFLAG_USB_Msk) != 0) {
		if (mchp_g2_handle_usb_events(regs, priv, intrusb)) {
			return;
		}
	}

	/* Assuming mchp_g2_handle_ep0_csr returns a boolean-like int */
	if (mchp_g2_handle_ep0_csr(regs, priv, &csr0L) != 0) {
		return;
	}

	mchp_g2_handle_ep0_state(dev, regs, priv, csr0L, endpointTXInterrupts);
	mchp_g2_handle_epx_tx_done(priv, regs, endpointTXInterrupts);
	mchp_g2_handle_epx_rx_done(dev, regs, priv, endpointRXInterrupts);

	/* Handle any back-to-back SETUP packet before leaving the interrupt. */
	if (priv->ep0_state == EP0_STATE_IDLE) {
		csr0L = regs->ENDPOINT0.USBHS_CSR0L;
		mchp_g2_ep0_state_idle(regs, priv, csr0L);
	}
}

/*
 * EP1-7 OUT enqueue. Handles two deferred-NAK cases:
 *   bulk_rx_nak: buffer boundary - un-NAK now.
 *   bulk_rx_pending: data in HW FIFO with no buffer - drain now.
 */
static int mchp_g2_enqueue_epx_out(const struct device *dev, usbhs_registers_t *regs,
				   uint8_t ep_idx)
{
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	unsigned int key;
	bool do_bulk_rx;
	struct udc_ep_config *ep_cfg;
	struct net_buf *rx_buf;
	uint16_t count;
	volatile uint32_t *fifo;
	uint32_t size;
	uint8_t *data;
	uint16_t ep_mps;
	bool short_packet;
	bool buf_full;

	/* Path 1: buffer boundary NAK - un-NAK. */
	if (atomic_test_and_clear_bit(&priv->bulk_rx_nak, ep_idx)) {
		key = irq_lock();

		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep_idx);
		regs->ENDPOINTX.USBHS_RXCSRL &= ~USBHS_ENDPOINTX_RXCSRL_RXPKTRDY_Msk;
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
		irq_unlock(key);
	}

	/* Path 2: data in HW FIFO with no buffer - drain now. */
	if (atomic_test_and_clear_bit(&priv->bulk_rx_pending, ep_idx)) {
		key = irq_lock();
		do_bulk_rx = false;

		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep_idx);

		ep_cfg = udc_get_ep_cfg(dev, (uint8_t)ep_idx);
		rx_buf = ep_cfg ? udc_buf_peek(ep_cfg) : NULL;

		if (rx_buf != NULL) {
			count = regs->ENDPOINTX.USBHS_RXCOUNT &
				USBHS_ENDPOINTX_RXCOUNT_ENDPOINTRXCOUNT_Msk;
			fifo = (volatile uint32_t *)&regs->ENDPOINT0.USBHS_FIFOX[ep_idx];
			size = MIN((uint32_t)count, net_buf_tailroom(rx_buf));
			data = net_buf_add(rx_buf, size);

			mchp_g2_fifo_read(fifo, data, size);

			ep_mps = udc_mps_ep_size(ep_cfg);
			short_packet = ((uint16_t)count < ep_mps);
			buf_full = (net_buf_tailroom(rx_buf) == 0U);

			if (short_packet) {
				/* End of transfer; un-NAK and deliver. */
				regs->ENDPOINTX.USBHS_RXCSRL &=
					~USBHS_ENDPOINTX_RXCSRL_RXPKTRDY_Msk;
				atomic_set_bit(&priv->bulk_rx_done, ep_idx);
				do_bulk_rx = true;
			} else if (buf_full) {
				/* Buffer full; hold NAK, deliver, next enqueue will un-NAK. */
				atomic_set_bit(&priv->bulk_rx_nak, ep_idx);
				atomic_set_bit(&priv->bulk_rx_done, ep_idx);
				do_bulk_rx = true;
			} else {
				/* More space in buffer; un-NAK and continue. */
				regs->ENDPOINTX.USBHS_RXCSRL &=
					~USBHS_ENDPOINTX_RXCSRL_RXPKTRDY_Msk;
			}
		} else {
			/* No buffer (unexpected); clear RxPktRdy to avoid stall. */
			regs->ENDPOINTX.USBHS_RXCSRL &= ~USBHS_ENDPOINTX_RXCSRL_RXPKTRDY_Msk;
		}

		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
		irq_unlock(key);

		if (do_bulk_rx) {
			k_event_post(&priv->events, MCHP_G2_EVT_BULK_RX);
		}
	}

	return 0;
}

/*
 * EP1-7 IN enqueue: write first chunk to FIFO and set TxPktRdy.
 * Returns immediately if previous packet is still in flight.
 * Always restores INDEX=0 before unlocking to protect EP0 CSR access.
 */
static int mchp_g2_enqueue_epx_in(const struct device *dev, usbhs_registers_t *regs,
				  struct udc_ep_config *const cfg, struct net_buf *buf)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	unsigned int key = irq_lock();
	uint16_t ep_mps = udc_mps_ep_size(cfg);
	volatile uint8_t *ep_fifo = (volatile uint8_t *)&regs->ENDPOINT0.USBHS_FIFOX[ep_idx];
	size_t chunk;
	size_t i;

	regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep_idx);

	if ((regs->ENDPOINTX.USBHS_TXCSRL & USBHS_ENDPOINTX_TXCSRL_TXPKTRDY_Msk) != 0U) {
		/* Restore INDEX=0 before unlock; ISR reads CSR0L via indexed window. */
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
		irq_unlock(key);
		return 0; /* busy, buf already queued */
	}

	chunk = MIN(buf->len, (size_t)ep_mps);

	for (i = 0; i < chunk; i++) {
		*ep_fifo = buf->data[i];
	}
	net_buf_pull(buf, chunk);

	regs->ENDPOINTX.USBHS_TXCSRL |= USBHS_ENDPOINTX_TXCSRL_TXPKTRDY_Msk;

	/* Restore INDEX=0. */
	regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
	irq_unlock(key);

	return 0;
}

/*
 * EP0 IN enqueue. Three cases:
 *   status ZLP : post EVT_XFER only (hardware already sent it).
 *   TxPktRdy set : FIFO busy.
 *   data/ZLP : write FIFO, set TxPktRdy, set DataEnd on last packet.
 * Restores INDEX=0 first; a prior bulk enqueue may have left it non-zero.
 */
static int mchp_g2_enqueue_ep0_in(const struct device *dev, usbhs_registers_t *regs,
				  struct udc_ep_config *const cfg, struct net_buf *buf)
{
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	uint16_t mps = udc_mps_ep_size(cfg);
	volatile uint8_t *fifo = (volatile uint8_t *)&regs->ENDPOINT0.USBHS_FIFOX[0];
	uint8_t csr0;
	size_t len;
	size_t i;

	/* Restore INDEX=0 before any EP0 CSR0L access. */
	regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);

	if (bi->status != 0U) {
		/* Status ZLP sent by hardware; notify the thread. */
		LOG_DBG("ep_enqueue: status IN ZLP queued, posting EVT_XFER");
		k_event_post(&priv->events, MCHP_G2_EVT_XFER);
		return 0;
	}

	csr0 = regs->ENDPOINT0.USBHS_CSR0L;
	if ((csr0 & USBHS_ENDPOINT0_CSR0L_TXPKTRDY_Msk) != 0U) {
		/* FIFO busy */
		LOG_WRN("ep_enqueue EP0_IN: TxPktRdy already set "
			"(CSR0L=0x%02x)  skipped",
			csr0);
		return 0;
	}

	if (buf->len == 0U && bi->zlp == 0U) {
		/* ZLP explicitly requested (not status) */
		priv->ep0_state = EP0_STATE_TX;
		regs->ENDPOINT0.USBHS_CSR0L |= USBHS_ENDPOINT0_CSR0L_TXPKTRDY_Msk |
					       USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_DATAEND_Msk;
		return 0;
	}

	len = MIN(buf->len, (size_t)mps);
	for (i = 0; i < len; i++) {
		*fifo = buf->data[i];
	}
	net_buf_pull(buf, len);

	csr0 |= USBHS_ENDPOINT0_CSR0L_TXPKTRDY_Msk;
	if (len < mps || (buf->len == 0U && bi->zlp == 0U)) {
		csr0 |= USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_DATAEND_Msk;
	}

	priv->ep0_state = EP0_STATE_TX;
	regs->ENDPOINT0.USBHS_CSR0L = csr0;

	return 0;
}

/*
 * EP0 OUT enqueue.  Called when the class driver provides a receive buffer for
 * a Control Write data stage.
 */
static int mchp_g2_enqueue_ep0_out(const struct device *dev, usbhs_registers_t *regs,
				   struct udc_ep_config *const cfg, struct net_buf *buf)
{
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	unsigned int key;
	uint8_t count;
	uint32_t size;
	volatile uint32_t *fifo;
	bool last;

	if (bi->setup != 0U) {
		/* SETUP buffer - handled by the ISR, nothing to arm here. */
		return 0;
	}

	key = irq_lock();

	/* Arm ep0_rx_buf so the ISR writes subsequent packets directly. */
	priv->ep0_rx_buf = buf;

	/* Drain a packet held in the FIFO while ep0_rx_buf was NULL. */
	if (atomic_clear(&priv->ep0_rx_pending)) {
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
		count = regs->ENDPOINT0.USBHS_COUNT0 & USBHS_ENDPOINT0_COUNT0_EP0RXCOUNT_Msk;

		if (count > 0U) {
			fifo = (volatile uint32_t *)&regs->ENDPOINT0.USBHS_FIFOX[0];
			size = MIN((uint32_t)count, net_buf_tailroom(buf));
			mchp_g2_fifo_read(fifo, net_buf_add(buf, size), size);
			priv->ep0_ctrl_bytes_received += (uint16_t)count;
		}

		last = ((uint16_t)count < USB_CONTROL_EP_MPS) ||
		       (priv->ep0_ctrl_bytes_received >= priv->ep0_ctrl_write_len);

		if (last) {
			regs->ENDPOINT0.USBHS_CSR0L |=
				USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SERVICEDRXPKTRDY_Msk |
				USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_DATAEND_Msk;
			priv->ep0_state = EP0_STATE_STATUS_IN;
			atomic_set(&priv->ep0_rx_done, 1);
			k_event_post(&priv->events, MCHP_G2_EVT_XFER);
		} else {
			/* Release RxPktRdy; host will send remaining packets. */
			regs->ENDPOINT0.USBHS_CSR0L |=
				USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SERVICEDRXPKTRDY_Msk;
		}
	}

	irq_unlock(key);
	return 0;
}

/* Enqueue a transfer request. Must not block. */
static int udc_mchp_g2_ep_enqueue(const struct device *dev, struct udc_ep_config *const cfg,
				  struct net_buf *buf)
{
	const struct udc_mchp_g2_config *config = dev->config;
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);

	LOG_DBG("%p enqueue %p to ep 0x%02x, len=%u", dev, buf, cfg->addr, buf->len);
	udc_buf_put(cfg, buf);

	if (cfg->stat.halted) {
		LOG_DBG("ep 0x%02x halted", cfg->addr);
		return 0;
	}

	if (USB_EP_GET_IDX(cfg->addr) != 0U) {
		if (!USB_EP_DIR_IS_IN(cfg->addr)) {
			return mchp_g2_enqueue_epx_out(dev, regs, USB_EP_GET_IDX(cfg->addr));
		}
		return mchp_g2_enqueue_epx_in(dev, regs, cfg, buf);
	}

	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		return mchp_g2_enqueue_ep0_in(dev, regs, cfg, buf);
	}

	return mchp_g2_enqueue_ep0_out(dev, regs, cfg, buf);
}

/* Flush FIFO and cancel all queued requests for an endpoint. */
static int udc_mchp_g2_ep_dequeue(const struct device *dev, struct udc_ep_config *const cfg)
{
	const struct udc_mchp_g2_config *config = dev->config;
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);

	LOG_DBG("Dequeue ep 0x%02x", cfg->addr);

	unsigned int lock_key = irq_lock();

	if (ep_idx == 0U) {
		/* Restore INDEX=0 in case a prior bulk op left it non-zero. */
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
		regs->ENDPOINT0.USBHS_CSR0H |= USBHS_ENDPOINT0_CSR0H_FLUSHFIFO_Msk;
		atomic_clear(&priv->ep0_rx_done);
		atomic_clear(&priv->ep0_rx_pending);
		priv->ep0_rx_buf = NULL;
		priv->ep0_state = EP0_STATE_IDLE;
	} else if (USB_EP_DIR_IS_IN(cfg->addr)) {
		regs->ENDPOINT0.USBHS_INTRTXE &= ~BIT(ep_idx);
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep_idx);

		/* Double-flush for double-buffered endpoints. */
		regs->ENDPOINTX.USBHS_TXCSRL |= USBHS_ENDPOINTX_TXCSRL_FLUSHFIFO_Msk;
		if ((regs->ENDPOINTX.USBHS_TXCSRL & USBHS_ENDPOINTX_TXCSRL_FIFONOTEMPTY_Msk) !=
		    0U) {
			regs->ENDPOINTX.USBHS_TXCSRL |= USBHS_ENDPOINTX_TXCSRL_FLUSHFIFO_Msk;
		}
		regs->ENDPOINTX.USBHS_TXCSRL &= ~USBHS_ENDPOINTX_TXCSRL_TXPKTRDY_Msk;
		atomic_clear_bit(&priv->bulk_tx_done, ep_idx);

		/* Restore INDEX=0 before re-enabling TX interrupt. */
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
		regs->ENDPOINT0.USBHS_INTRTXE |= BIT(ep_idx);
	} else {
		regs->ENDPOINT0.USBHS_INTRRXE &= ~BIT(ep_idx);
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep_idx);

		/* Double-flush for double-buffered endpoints. */
		regs->ENDPOINTX.USBHS_RXCSRL |= USBHS_ENDPOINTX_RXCSRL_FLUSHFIFO_Msk;
		if ((regs->ENDPOINTX.USBHS_RXCSRL & USBHS_ENDPOINTX_RXCSRL_RXPKTRDY_Msk) != 0U) {
			regs->ENDPOINTX.USBHS_RXCSRL |= USBHS_ENDPOINTX_RXCSRL_FLUSHFIFO_Msk;
		}
		/* Must clear RxPktRdy; leaving it set permanently NAKs OUT tokens. */
		regs->ENDPOINTX.USBHS_RXCSRL &= ~USBHS_ENDPOINTX_RXCSRL_RXPKTRDY_Msk;
		atomic_clear_bit(&priv->bulk_rx_done, ep_idx);
		atomic_clear_bit(&priv->bulk_rx_nak, ep_idx);
		atomic_clear_bit(&priv->bulk_rx_pending, ep_idx);

		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
		regs->ENDPOINT0.USBHS_INTRRXE |= BIT(ep_idx);
	}

	udc_ep_cancel_queued(dev, cfg);

	irq_unlock(lock_key);

	return 0;
}

/* Allocate FIFO for an endpoint. EP0 IN and OUT share one block. */
static uint16_t mchp_g2_fifo_alloc_for_ep(struct udc_mchp_g2_data *priv, uint8_t ep_idx, bool is_in,
					  uint16_t size_bytes)
{
	unsigned int key = irq_lock();
	uint16_t addr;

	if (ep_idx == 0U) {
		/* EP0: IN and OUT share one block */
		addr = priv->fifo_in_addr[0];
		if (addr == MCHP_G2_FIFO_ADDR_INVALID) {
			addr = mchp_g2_fifo_allocate(priv, size_bytes);
			if (addr != MCHP_G2_FIFO_ADDR_INVALID) {
				priv->fifo_in_addr[0] = addr;
				priv->fifo_out_addr[0] = addr;
			}
		}
	} else if (is_in) {
		addr = priv->fifo_in_addr[ep_idx];
		if (addr == MCHP_G2_FIFO_ADDR_INVALID) {
			addr = mchp_g2_fifo_allocate(priv, size_bytes);
			if (addr != MCHP_G2_FIFO_ADDR_INVALID) {
				priv->fifo_in_addr[ep_idx] = addr;
			}
		}
	} else {
		addr = priv->fifo_out_addr[ep_idx];
		if (addr == MCHP_G2_FIFO_ADDR_INVALID) {
			addr = mchp_g2_fifo_allocate(priv, size_bytes);
			if (addr != MCHP_G2_FIFO_ADDR_INVALID) {
				priv->fifo_out_addr[ep_idx] = addr;
			}
		}
	}

	irq_unlock(key);
	return addr;
}

/* Program FIFO and endpoint registers. Always restores INDEX=0 on exit. */
static void mchp_g2_ep_configure_fifo(usbhs_registers_t *regs, struct udc_ep_config *const cfg,
				      uint8_t ep_idx, bool is_in, uint8_t size_code, uint16_t addr)
{
	/* TXFIFOADD/RXFIFOADD hold the FIFO start address in 8-byte units. */
	uint16_t addr_units = addr / MCHP_G2_FIFO_UNIT_SIZE;

	regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep_idx);

	if (ep_idx == 0U) {
		regs->ENDPOINT0.USBHS_TXFIFOSZ = USBHS_TXFIFOSZ_SZ(size_code);
		regs->ENDPOINT0.USBHS_RXFIFOSZ = USBHS_RXFIFOSZ_SZ(size_code);
		regs->ENDPOINT0.USBHS_TXFIFOADD = USBHS_TXFIFOADD_ADDR(addr_units);
		regs->ENDPOINT0.USBHS_RXFIFOADD = USBHS_RXFIFOADD_ADDR(addr_units);
	} else if (is_in) {
		regs->ENDPOINT0.USBHS_TXFIFOSZ = USBHS_TXFIFOSZ_SZ(size_code);
		regs->ENDPOINT0.USBHS_TXFIFOADD = USBHS_TXFIFOADD_ADDR(addr_units);
		regs->ENDPOINTX.USBHS_TXMAXP =
			USBHS_ENDPOINTX_TXMAXP_MAXPAYLOAD(udc_mps_ep_size(cfg));
		regs->ENDPOINTX.USBHS_TXCSRL |= USBHS_ENDPOINTX_TXCSRL_CLRDATATOG_Msk;
		regs->ENDPOINT0.USBHS_INTRTXE |= BIT(ep_idx);
	} else {
		regs->ENDPOINT0.USBHS_RXFIFOSZ = USBHS_RXFIFOSZ_SZ(size_code);
		regs->ENDPOINT0.USBHS_RXFIFOADD = USBHS_RXFIFOADD_ADDR(addr_units);
		regs->ENDPOINTX.USBHS_RXMAXP =
			USBHS_ENDPOINTX_RXMAXP_MAXPAYLOAD(udc_mps_ep_size(cfg));
		regs->ENDPOINTX.USBHS_RXCSRL |= USBHS_ENDPOINTX_RXCSRL_CLRDATATOG_Msk;
		regs->ENDPOINT0.USBHS_INTRRXE |= BIT(ep_idx);
	}

	regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
}

/* Enable endpoint. */
static int udc_mchp_g2_ep_enable(const struct device *dev, struct udc_ep_config *const cfg)
{
	const struct udc_mchp_g2_config *config = dev->config;
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	bool is_in = USB_EP_DIR_IS_IN(cfg->addr);
	uint16_t size_bytes;
	uint8_t size_code;
	uint16_t addr;

	LOG_DBG("Enable ep 0x%02x", cfg->addr);

	if (ep_idx >= MCHP_G2_EP_MAX) {
		return -EINVAL;
	}

	size_bytes = MAX(udc_mps_ep_size(cfg), MCHP_G2_FIFO_UNIT_SIZE);
	size_code = mchp_g2_fifo_size_code(size_bytes);

	if (size_code == MCHP_G2_FIFO_SIZE_CODE_INVALID) {
		return -EINVAL;
	}
	size_bytes = MCHP_G2_FIFO_UNIT_SIZE << size_code;

	addr = mchp_g2_fifo_alloc_for_ep(priv, ep_idx, is_in, size_bytes);

	if (addr == MCHP_G2_FIFO_ADDR_INVALID) {
		return -ENOMEM;
	}

	mchp_g2_ep_configure_fifo(regs, cfg, ep_idx, is_in, size_code, addr);

	return 0;
}

/* Free FIFO allocation table bits for a range. Caller holds irq_lock. */
static void mchp_g2_fifo_free(struct udc_mchp_g2_data *priv, uint16_t addr, uint16_t size_bytes)
{
	uint32_t start_idx;
	uint32_t num_units;
	uint32_t i;

	if (addr == MCHP_G2_FIFO_ADDR_INVALID || size_bytes == 0U) {
		return;
	}

	start_idx = addr / MCHP_G2_FIFO_UNIT_SIZE;
	num_units = size_bytes / MCHP_G2_FIFO_UNIT_SIZE;

	for (i = start_idx; i < (start_idx + num_units); i++) {
		priv->fifo_allocation_table[i / MCHP_G2_FIFO_UNITS_PER_PAGE] &=
			~BIT(i % MCHP_G2_FIFO_UNITS_PER_PAGE);
	}
}

/* Free FIFO for an endpoint. EP0 shared block freed on first call only. */
static void mchp_g2_fifo_free_for_ep(struct udc_mchp_g2_data *priv, uint8_t ep_idx, bool is_in,
				     uint16_t size_bytes)
{
	unsigned int key = irq_lock();
	uint16_t addr;

	if (ep_idx == 0U) {
		/* EP0: IN and OUT share one block */
		addr = priv->fifo_in_addr[0];

		if (addr != MCHP_G2_FIFO_ADDR_INVALID) {
			mchp_g2_fifo_free(priv, addr, size_bytes);
			priv->fifo_in_addr[0] = MCHP_G2_FIFO_ADDR_INVALID;
			priv->fifo_out_addr[0] = MCHP_G2_FIFO_ADDR_INVALID;
		}
	} else if (is_in) {
		addr = priv->fifo_in_addr[ep_idx];

		if (addr != MCHP_G2_FIFO_ADDR_INVALID) {
			mchp_g2_fifo_free(priv, addr, size_bytes);
			priv->fifo_in_addr[ep_idx] = MCHP_G2_FIFO_ADDR_INVALID;
		}
	} else {
		addr = priv->fifo_out_addr[ep_idx];

		if (addr != MCHP_G2_FIFO_ADDR_INVALID) {
			mchp_g2_fifo_free(priv, addr, size_bytes);
			priv->fifo_out_addr[ep_idx] = MCHP_G2_FIFO_ADDR_INVALID;
		}
	}

	irq_unlock(key);
}

/* Zero FIFO and interrupt registers for an endpoint. Restores INDEX=0. */
static void mchp_g2_ep_deconfigure_fifo(usbhs_registers_t *regs, uint8_t ep_idx, bool is_in)
{
	regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep_idx);

	if (ep_idx == 0U) {
		regs->ENDPOINT0.USBHS_TXFIFOSZ = 0U;
		regs->ENDPOINT0.USBHS_RXFIFOSZ = 0U;
		regs->ENDPOINT0.USBHS_TXFIFOADD = 0U;
		regs->ENDPOINT0.USBHS_RXFIFOADD = 0U;
	} else if (is_in) {
		regs->ENDPOINT0.USBHS_INTRTXE &= ~BIT(ep_idx);
		regs->ENDPOINT0.USBHS_TXFIFOSZ = 0U;
		regs->ENDPOINT0.USBHS_TXFIFOADD = 0U;
		regs->ENDPOINTX.USBHS_TXMAXP = 0U;
	} else {
		regs->ENDPOINT0.USBHS_INTRRXE &= ~BIT(ep_idx);
		regs->ENDPOINT0.USBHS_RXFIFOSZ = 0U;
		regs->ENDPOINT0.USBHS_RXFIFOADD = 0U;
		regs->ENDPOINTX.USBHS_RXMAXP = 0U;
	}

	regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
}

/* Disable endpoint. */
static int udc_mchp_g2_ep_disable(const struct device *dev, struct udc_ep_config *const cfg)
{
	const struct udc_mchp_g2_config *config = dev->config;
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	bool is_in = USB_EP_DIR_IS_IN(cfg->addr);
	uint16_t size_bytes;
	uint8_t size_code;

	LOG_DBG("Disable ep 0x%02x", cfg->addr);

	if (ep_idx >= MCHP_G2_EP_MAX) {
		return -EINVAL;
	}

	/* cfg->mps is unchanged since ep_enable, so size_bytes is identical */
	size_bytes = MAX(udc_mps_ep_size(cfg), MCHP_G2_FIFO_UNIT_SIZE);
	size_code = mchp_g2_fifo_size_code(size_bytes);

	if (size_code == MCHP_G2_FIFO_SIZE_CODE_INVALID) {
		return -EINVAL;
	}
	size_bytes = MCHP_G2_FIFO_UNIT_SIZE << size_code;

	mchp_g2_ep_deconfigure_fifo(regs, ep_idx, is_in);
	mchp_g2_fifo_free_for_ep(priv, ep_idx, is_in, size_bytes);

	return 0;
}

/* Set endpoint STALL. */
static int udc_mchp_g2_ep_set_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	const struct udc_mchp_g2_config *config = dev->config;
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);
	unsigned int key;
	uint8_t ep_idx;

	LOG_DBG("Set halt ep 0x%02x", cfg->addr);

	if (USB_EP_GET_IDX(cfg->addr) == 0U) {
		/* Set SendStall; hardware clears it after sending the handshake. */
		key = irq_lock();
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
		regs->ENDPOINT0.USBHS_CSR0L |= USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SENDSTALL_Msk;
		priv->ep0_state = EP0_STATE_IDLE;
		irq_unlock(key);
	} else {
		ep_idx = USB_EP_GET_IDX(cfg->addr);

		key = irq_lock();
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep_idx);
		if (USB_EP_DIR_IS_IN(cfg->addr)) {
			regs->ENDPOINTX.USBHS_TXCSRL |=
				USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_SENDSTALL_Msk;
		} else {
			regs->ENDPOINTX.USBHS_RXCSRL |=
				USBHS_ENDPOINTX_RXCSRL_PERIPHERAL_EPX_SENDSTALL_Msk;
		}
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);
		irq_unlock(key);
		cfg->stat.halted = true;
	}

	return 0;
}

/* Clear endpoint STALL. */
static int udc_mchp_g2_ep_clear_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	const struct udc_mchp_g2_config *config = dev->config;
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	unsigned int key;
	struct net_buf *pending;

	LOG_DBG("Clear halt ep 0x%02x", cfg->addr);

	if (ep_idx == 0U) {
		/* EP0 STALL is self-clearing; the state machine handles recovery */
		return 0;
	}

	key = irq_lock();

	regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(ep_idx);
	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		regs->ENDPOINTX.USBHS_TXCSRL &=
			~(USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_SENDSTALL_Msk |
			  USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_SENTSTALL_Msk);
		regs->ENDPOINTX.USBHS_TXCSRL |= USBHS_ENDPOINTX_TXCSRL_CLRDATATOG_Msk;
		/* Flush FIFO; DATA0 must be clean after halt clear. */
		regs->ENDPOINTX.USBHS_TXCSRL |= USBHS_ENDPOINTX_TXCSRL_FLUSHFIFO_Msk;
		if ((regs->ENDPOINTX.USBHS_TXCSRL & USBHS_ENDPOINTX_TXCSRL_FIFONOTEMPTY_Msk) !=
		    0U) {
			regs->ENDPOINTX.USBHS_TXCSRL |= USBHS_ENDPOINTX_TXCSRL_FLUSHFIFO_Msk;
		}
		regs->ENDPOINTX.USBHS_TXCSRL &= ~USBHS_ENDPOINTX_TXCSRL_TXPKTRDY_Msk;
	} else {
		regs->ENDPOINTX.USBHS_RXCSRL &=
			~(USBHS_ENDPOINTX_RXCSRL_PERIPHERAL_EPX_SENDSTALL_Msk |
			  USBHS_ENDPOINTX_RXCSRL_PERIPHERAL_EPX_SENTSTALL_Msk);
		regs->ENDPOINTX.USBHS_RXCSRL |= USBHS_ENDPOINTX_RXCSRL_CLRDATATOG_Msk;
	}
	regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);

	irq_unlock(key);

	cfg->stat.halted = false;

	/* Re-arm any pending IN transfer. */
	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		pending = udc_buf_peek(cfg);

		/* Skip empty buffers; bulk_tx_done will dequeue them. */
		if (!udc_ep_is_busy(cfg) && pending != NULL && pending->len > 0) {
			mchp_g2_enqueue_epx_in(dev, regs, cfg, pending);
		}
	}

	return 0;
}

/* Set USB device address. */
static int udc_mchp_g2_set_address(const struct device *dev, const uint8_t addr)
{
	const struct udc_mchp_g2_config *config = dev->config;
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);

	LOG_DBG("Set address %u for %p", addr, dev);

	/* Redundant; ISR already set FADDR at SETUP decode. */
	regs->ENDPOINT0.USBHS_FADDR = USBHS_FADDR_FUNCADDR(addr);

	LOG_INF("USB device address set to %u", addr);

	return 0;
}

/* Initiate remote wakeup: assert K-state for 2 ms then release. */
static int udc_mchp_g2_host_wakeup(const struct device *dev)
{
	const struct udc_mchp_g2_config *config = dev->config;
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);
	uint8_t power_reg;

	LOG_DBG("Remote wakeup from %p", dev);

	if (!udc_is_suspended(dev)) {
		LOG_WRN("Remote wakeup requested but device is not suspended");
		return -EACCES;
	}

	/* Exit suspend and assert K-state. */
	power_reg = regs->ENDPOINT0.USBHS_POWER;
	power_reg &= ~USBHS_POWER_SUSPENDMODE_Msk;
	power_reg |= USBHS_POWER_RESUME_Msk;
	regs->ENDPOINT0.USBHS_POWER = power_reg;

	k_msleep(MCHP_G2_RESUME_KSTATE_MS);

	/* De-assert resume. */
	power_reg = regs->ENDPOINT0.USBHS_POWER;
	power_reg &= ~USBHS_POWER_RESUME_Msk;
	regs->ENDPOINT0.USBHS_POWER = power_reg;

	udc_set_suspended(dev, false);
	udc_submit_event(dev, UDC_EVT_RESUME, 0);

	return 0;
}

/* Return the negotiated USB bus speed. */
static enum udc_bus_speed udc_mchp_g2_device_speed(const struct device *dev)
{
	const struct udc_mchp_g2_config *config = dev->config;
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);

	/* POWER.HSMODE is set by hardware after successful HS negotiation. */
	return (regs->ENDPOINT0.USBHS_POWER & USBHS_POWER_HSMODE_Msk) ? UDC_BUS_SPEED_HS
								      : UDC_BUS_SPEED_FS;
}

/* Attach to bus: enable EP0, unmask interrupts, assert SOFTCONN. */
static int udc_mchp_g2_enable(const struct device *dev)
{
	const struct udc_mchp_g2_config *config = dev->config;
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);
	uint8_t power_reg;
	int ret;

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL,
				     USB_CONTROL_EP_MPS, 0);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed enabling ep 0x%02x", USB_CONTROL_EP_OUT);
		return ret;
	}

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL,
				     USB_CONTROL_EP_MPS, 0);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed enabling ep 0x%02x", USB_CONTROL_EP_IN);
		return ret;
	}

	regs->ENDPOINT0.USBHS_INTRUSBE |= USBHS_INTRUSBE_RESETEN_Msk |
					  USBHS_INTRUSBE_SUSPENDEN_Msk |
					  USBHS_INTRUSBE_RESUMEEN_Msk;
	/* Disable VBUSERR; conditionally disable SOFEN. */
	regs->ENDPOINT0.USBHS_INTRUSBE &= ~USBHS_INTRUSBE_VBUSERREN_Msk;
	if (!IS_ENABLED(CONFIG_UDC_ENABLE_SOF)) {
		regs->ENDPOINT0.USBHS_INTRUSBE &= ~USBHS_INTRUSBE_SOFEN_Msk;
	}
	regs->ENDPOINT0.USBHS_INTRTXE |= USBHS_INTRTXE_EP0TXEN_Msk;
	regs->ENDPOINT0.USBHS_INTENSET = USBHS_INTENSET_USB_Msk;
	LOG_DBG("USB intrusbe=0x%02x, inten=0x%08x", regs->ENDPOINT0.USBHS_INTRUSBE,
		regs->ENDPOINT0.USBHS_INTENSET);

	/* Set HSENABLE only for High-Speed mode; clear it for Full-Speed. */
	power_reg = regs->ENDPOINT0.USBHS_POWER;
	if (config->speed_idx == UDC_BUS_SPEED_HS) {
		power_reg |= USBHS_POWER_HSENABLE_Msk;
	} else {
		power_reg &= ~USBHS_POWER_HSENABLE_Msk;
	}
	regs->ENDPOINT0.USBHS_POWER = power_reg;
	k_usleep(MCHP_G2_PHY_SETTLE_US);

	power_reg = regs->ENDPOINT0.USBHS_POWER;
	power_reg |= USBHS_POWER_SOFTCONN_Msk;
	regs->ENDPOINT0.USBHS_POWER = power_reg;

	return 0;
}

/* Detach from bus: disable EP0 and clear SOFTCONN. */
static int udc_mchp_g2_disable(const struct device *dev)
{
	const struct udc_mchp_g2_config *config = dev->config;
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);
	uint8_t power_reg;
	int ret;

	ret = udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed to disable control endpoint OUT");
		return ret;
	}

	ret = udc_ep_disable_internal(dev, USB_CONTROL_EP_IN);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed to disable control endpoint IN");
		return ret;
	}

	power_reg = regs->ENDPOINT0.USBHS_POWER;
	power_reg &= ~USBHS_POWER_SOFTCONN_Msk;
	regs->ENDPOINT0.USBHS_POWER = power_reg;

	return 0;
}

static int udc_mchp_g2_init(const struct device *dev)
{
	const struct udc_mchp_g2_config *config = dev->config;
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);
	uint32_t status;
	int timeout = MCHP_G2_SYNC_TIMEOUT_ITER;
	uint16_t addr;
	uint32_t i;

	LOG_INF("Initialize USB device %p at base 0x%08x", dev, config->base);

	for (i = 0; i < MCHP_G2_FIFO_PAGES; i++) {
		priv->fifo_allocation_table[i] = 0U;
	}
	for (i = 0; i < MCHP_G2_EP_MAX; i++) {
		priv->fifo_in_addr[i] = MCHP_G2_FIFO_ADDR_INVALID;
		priv->fifo_out_addr[i] = MCHP_G2_FIFO_ADDR_INVALID;
	}

	addr = mchp_g2_fifo_allocate(priv, USB_CONTROL_EP_MPS);
	if (addr == MCHP_G2_FIFO_ADDR_INVALID) {
		LOG_ERR("Failed to reserve EP0 FIFO");
		return -ENOMEM;
	}
	priv->fifo_in_addr[0] = addr;
	priv->fifo_out_addr[0] = addr;

	priv->setup_q_head = 0U;
	priv->setup_q_tail = 0U;

	/* Enable AVREGEN and wait for it to stabilise. */
	SUPC_REGS->SUPC_VREGCTRL |= SUPC_VREGCTRL_AVREGEN_Msk;
	k_usleep(MCHP_G2_AVREGEN_SETTLE_US);

	regs->ENDPOINT0.USBHS_CTRLA = USBHS_CTRLA_SWRST_Msk;

	/* Wait for software reset to complete. */
	while ((regs->ENDPOINT0.USBHS_CTRLA & USBHS_CTRLA_SWRST_Msk ||
		regs->ENDPOINT0.USBHS_SYNCBUSY & USBHS_SYNCBUSY_SWRST_Msk) &&
	       timeout--) {
		k_usleep(1);
	}
	if (timeout <= 0) {
		LOG_ERR("USB controller reset timeout");
		return -ETIMEDOUT;
	}

	/* Configure as B-device (peripheral). Use |= to preserve IDOVEN/IDVAL. */
	regs->ENDPOINT0.USBHS_CTRLA |= USBHS_CTRLA_IDOVEN(1);
	regs->ENDPOINT0.USBHS_CTRLA |= USBHS_CTRLA_IDVAL(1);

	regs->ENDPOINT0.USBHS_CTRLA |= USBHS_CTRLA_ENABLE_Msk;

	/* Wait for ENABLE sync. */
	timeout = MCHP_G2_SYNC_TIMEOUT_ITER;
	while ((regs->ENDPOINT0.USBHS_SYNCBUSY & USBHS_SYNCBUSY_ENABLE_Msk) && timeout--) {
		k_usleep(1);
	}
	if (timeout <= 0) {
		LOG_ERR("USB controller enable sync timeout");
		return -ETIMEDOUT;
	}

	regs->ENDPOINT0.USBHS_INTENSET = USBHS_INTENSET_PHYRDY_Msk;

	/* Wait for PHY ready before accessing INTRUSBE/POWER. */
	status = regs->ENDPOINT0.USBHS_STATUS;

	if ((status & USBHS_STATUS_PHYRDY_Msk) == 0) {
		timeout = MCHP_G2_PHY_READY_TIMEOUT_ITER;

		while ((regs->ENDPOINT0.USBHS_STATUS & USBHS_STATUS_PHYRDY_Msk) == 0) {
			timeout--;

			if (timeout == 0) {
				LOG_ERR("USB PHY ready timeout");
				return -ETIMEDOUT;
			}

			k_usleep(1);
		}
	}

	/* Apply USBHPHY characterization calibration values. PHY0x registers
	 * are only accessible once PHYRDY is set.
	 */
	regs->ENDPOINT0.USBHS_PHY04 = (regs->ENDPOINT0.USBHS_PHY04 & ~USBHS_PHY04_SQUELCH210_Msk) |
				      USBHS_PHY04_SQUELCH210(MCHP_G2_PHY_CAL_SQUELCH_VAL);
	regs->ENDPOINT0.USBHS_PHY08 = (regs->ENDPOINT0.USBHS_PHY08 & ~USBHS_PHY08_SQUELCH3_Msk) |
				      USBHS_PHY08_SQUELCH3(MCHP_G2_PHY_CAL_SQUELCH_VAL >> 3);

	regs->ENDPOINT0.USBHS_PHY0C = (regs->ENDPOINT0.USBHS_PHY0C & ~USBHS_PHY0C_TUNE210_Msk) |
				      USBHS_PHY0C_TUNE210(MCHP_G2_PHY_CAL_TUNE_VAL);
	regs->ENDPOINT0.USBHS_PHY10 = (regs->ENDPOINT0.USBHS_PHY10 & ~USBHS_PHY10_TUNE76543_Msk) |
				      USBHS_PHY10_TUNE76543(MCHP_G2_PHY_CAL_TUNE_VAL >> 3);

	regs->ENDPOINT0.USBHS_PHY14 = (regs->ENDPOINT0.USBHS_PHY14 & ~USBHS_PHY14_ODT0_Msk) |
				      USBHS_PHY14_ODT0(MCHP_G2_PHY_CAL_ODT_VAL);
	regs->ENDPOINT0.USBHS_PHY18 = (regs->ENDPOINT0.USBHS_PHY18 & ~USBHS_PHY18_ODT21_Msk) |
				      USBHS_PHY18_ODT21(MCHP_G2_PHY_CAL_ODT_VAL >> 1);

	regs->ENDPOINT0.USBHS_PHY20 = (regs->ENDPOINT0.USBHS_PHY20 & ~USBHS_PHY20_HSSLEW10_Msk) |
				      USBHS_PHY20_HSSLEW10(MCHP_G2_PHY_CAL_HSSLEW_VAL);
	regs->ENDPOINT0.USBHS_PHY24 = (regs->ENDPOINT0.USBHS_PHY24 & ~USBHS_PHY24_HSSLEW2_Msk) |
				      USBHS_PHY24_HSSLEW2(MCHP_G2_PHY_CAL_HSSLEW_VAL >> 2);

	regs->ENDPOINT0.USBHS_PHY28 = (regs->ENDPOINT0.USBHS_PHY28 &
				       ~(USBHS_PHY28_DISCONDET_Msk | USBHS_PHY28_HSDRVCOMP_Msk)) |
				      USBHS_PHY28_DISCONDET(MCHP_G2_PHY_CAL_DISCONDET_VAL) |
				      USBHS_PHY28_HSDRVCOMP(MCHP_G2_PHY_CAL_HSDRVCOMP_VAL);

	/* Open VBUS detection switch (not needed in peripheral-only mode). */
	regs->ENDPOINT0.USBHS_PHY24 |= USBHS_PHY24_VBUSDETEN_Msk;

	if (IS_ENABLED(CONFIG_UDC_ENABLE_SOF)) {
		regs->ENDPOINT0.USBHS_INTRUSBE |= USBHS_INTRUSBE_SOFEN_Msk;
	}

	/* Start VBUS polling; first event fires after init() returns. */
	priv->vbus_present = false;
	k_work_reschedule(&priv->vbus_work, K_NO_WAIT);

	return 0;
}

/* Shut down the controller and reset all FIFO state. */
static int udc_mchp_g2_shutdown(const struct device *dev)
{
	const struct udc_mchp_g2_config *config = dev->config;
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);
	struct k_work_sync sync;
	int timeout;
	uint32_t i;
	int ret;

	LOG_DBG("Shutdown device %p", dev);

	/* Stop VBUS polling before touching hardware */
	k_work_cancel_delayable_sync(&priv->vbus_work, &sync);

	/* Silence all interrupt enables so nothing fires during teardown */
	regs->ENDPOINT0.USBHS_INTRTXE = 0x0000U;
	regs->ENDPOINT0.USBHS_INTRRXE = 0x0000U;
	regs->ENDPOINT0.USBHS_INTRUSBE = 0x00U;
	regs->ENDPOINT0.USBHS_INTENCLR = USBHS_INTENCLR_USB_Msk | USBHS_INTENCLR_PHYRDY_Msk;

	/* Disable the IRQ line itself; enabled again on next init(). */
	config->irq_disable_func(dev);

	ret = udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed to disable control endpoint OUT");
		return ret;
	}

	ret = udc_ep_disable_internal(dev, USB_CONTROL_EP_IN);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed to disable control endpoint IN");
		return ret;
	}

	/* Disable then reset the controller */
	regs->ENDPOINT0.USBHS_CTRLA &= ~USBHS_CTRLA_ENABLE_Msk;
	timeout = MCHP_G2_SYNC_TIMEOUT_ITER;

	while ((regs->ENDPOINT0.USBHS_SYNCBUSY & USBHS_SYNCBUSY_ENABLE_Msk) && timeout--) {
		k_usleep(1);
	}
	regs->ENDPOINT0.USBHS_CTRLA = USBHS_CTRLA_SWRST_Msk;
	timeout = MCHP_G2_SYNC_TIMEOUT_ITER;
	while ((regs->ENDPOINT0.USBHS_CTRLA & USBHS_CTRLA_SWRST_Msk ||
		regs->ENDPOINT0.USBHS_SYNCBUSY & USBHS_SYNCBUSY_SWRST_Msk) &&
	       timeout--) {
		k_usleep(1);
	}

	/* Reinitialise the software FIFO allocator. */
	for (i = 0; i < MCHP_G2_FIFO_PAGES; i++) {
		priv->fifo_allocation_table[i] = 0U;
	}
	for (i = 0; i < MCHP_G2_EP_MAX; i++) {
		priv->fifo_in_addr[i] = MCHP_G2_FIFO_ADDR_INVALID;
		priv->fifo_out_addr[i] = MCHP_G2_FIFO_ADDR_INVALID;
	}

	return 0;
}

/*
 * Enter a USB 2.0 test mode (sec. 7.1.20 / sec. 9.4.9).
 * dryrun=true validates the selector without touching hardware.
 * dryrun=false programs the hardware after Status-IN completes.
 * Exit requires a power cycle; there is no software exit path.
 */
static int udc_mchp_g2_test_mode(const struct device *dev, const uint8_t mode, const bool dryrun)
{
	const struct udc_mchp_g2_config *config = dev->config;
	usbhs_registers_t *regs = (usbhs_registers_t *)(config->base);
	volatile uint8_t *ep0_fifo;
	size_t i;

	/* Mandated 53-byte test packet sequence (USB 2.0 Table 7-8). */
	static const uint8_t test_packet_data[53] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
		0xAA, 0xAA, 0xAA, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xFE, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xBF, 0xDF, 0xEF, 0xF7,
		0xFB, 0xFD, 0xFC, 0x7E, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0x7E};

	switch (mode) {
	case USB_SFS_TEST_MODE_J:
	case USB_SFS_TEST_MODE_K:
	case USB_SFS_TEST_MODE_SE0_NAK:
	case USB_SFS_TEST_MODE_PACKET:
		break;
	case USB_SFS_TEST_MODE_FORCE_ENABLE:
		/* Host-mode only; not valid in peripheral mode (Table 9-7). */
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	if (dryrun) {
		return 0;
	}

	/* Use assignment (not |=) so exactly one test bit is active. */
	switch (mode) {
	case USB_SFS_TEST_MODE_J:
		regs->ENDPOINT0.USBHS_TESTMODE = USBHS_TESTMODE_TESTJ_Msk;
		break;
	case USB_SFS_TEST_MODE_K:
		regs->ENDPOINT0.USBHS_TESTMODE = USBHS_TESTMODE_TESTK_Msk;
		break;
	case USB_SFS_TEST_MODE_SE0_NAK:
		regs->ENDPOINT0.USBHS_TESTMODE = USBHS_TESTMODE_TESTSE0NAK_Msk;
		break;
	case USB_SFS_TEST_MODE_PACKET:
		/* INDEX must be 0; ISR or enqueue may have left it non-zero. */
		regs->ENDPOINT0.USBHS_INDEX = USBHS_INDEX_SELEP(0);

		/* FIFO must be loaded before asserting TESTPACKET. */
		ep0_fifo = (volatile uint8_t *)&regs->ENDPOINT0.USBHS_FIFOX[0];

		for (i = 0U; i < sizeof(test_packet_data); i++) {
			*ep0_fifo = test_packet_data[i];
		}

		regs->ENDPOINT0.USBHS_TESTMODE = USBHS_TESTMODE_TESTPACKET_Msk;
		/* Set TxPktRdy to begin continuous TX; DataEnd must NOT be set. */
		regs->ENDPOINT0.USBHS_CSR0L |= USBHS_ENDPOINT0_CSR0L_TXPKTRDY_Msk;
		break;
	default:
		/* Unreachable - validated above. */
		return -EINVAL;
	}

	return 0;
}

/* One-time driver setup: register endpoints, start thread, enable IRQ. */
static int udc_mchp_g2_driver_preinit(const struct device *dev)
{
	const struct udc_mchp_g2_config *config = dev->config;
	struct udc_mchp_g2_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	uint16_t mps = MCHP_G2_FS_EP_MPS_MAX;
	int err;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);

	/* dev pointer for CONTAINER_OF access in vbus_work. */
	priv->dev = dev;

	k_work_init_delayable(&priv->vbus_work, mchp_g2_vbus_poll_work);

	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;
	data->caps.can_detect_vbus = true;

	/* Hardware auto-completes Status OUT; don't enqueue a buffer for it. */
	data->caps.out_ack = true;

	if (config->speed_idx == UDC_BUS_SPEED_HS) {
		data->caps.hs = true;
		mps = MCHP_G2_HS_EP_MPS_MAX;
	}

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			config->ep_cfg_out[i].caps.control = 1;
			config->ep_cfg_out[i].caps.mps = USB_CONTROL_EP_MPS;
		} else {
			config->ep_cfg_out[i].caps.bulk = 1;
			config->ep_cfg_out[i].caps.interrupt = 1;
			config->ep_cfg_out[i].caps.iso = 1;
			config->ep_cfg_out[i].caps.mps = mps;
		}

		config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register OUT endpoint %d", i);
			return err;
		}
	}

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			config->ep_cfg_in[i].caps.control = 1;
			config->ep_cfg_in[i].caps.mps = USB_CONTROL_EP_MPS;
		} else {
			config->ep_cfg_in[i].caps.bulk = 1;
			config->ep_cfg_in[i].caps.interrupt = 1;
			config->ep_cfg_in[i].caps.iso = 1;
			config->ep_cfg_in[i].caps.mps = mps;
		}

		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register IN endpoint %d", i);
			return err;
		}
	}

	priv->ep0_state = EP0_STATE_IDLE;
	priv->ep0_rx_buf = NULL;
	priv->ep0_ctrl_bytes_received = 0U;

	k_thread_create(&priv->thread_data, config->thread_stk, config->thread_stk_sz,
			mchp_g2_thread_handler, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_UDC_MCHP_G2_THREAD_PRIORITY), K_ESSENTIAL, K_NO_WAIT);
	k_thread_name_set(&priv->thread_data, dev->name);

	config->irq_enable_func(dev);

	LOG_INF("MCHP G2 UDC initialized: %p (speed: %s, endpoints: %d IN + %d OUT)", dev,
		config->speed_idx == UDC_BUS_SPEED_HS ? "High-Speed" : "Full-Speed",
		config->num_of_eps, config->num_of_eps);

	return 0;
}

static void udc_mchp_g2_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_mchp_g2_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static const struct udc_api udc_mchp_g2_api = {
	.lock = udc_mchp_g2_lock,
	.unlock = udc_mchp_g2_unlock,
	.device_speed = udc_mchp_g2_device_speed,
	.init = udc_mchp_g2_init,
	.enable = udc_mchp_g2_enable,
	.disable = udc_mchp_g2_disable,
	.shutdown = udc_mchp_g2_shutdown,
	.set_address = udc_mchp_g2_set_address,
	.host_wakeup = udc_mchp_g2_host_wakeup,
	.test_mode = udc_mchp_g2_test_mode,
	.ep_enable = udc_mchp_g2_ep_enable,
	.ep_disable = udc_mchp_g2_ep_disable,
	.ep_set_halt = udc_mchp_g2_ep_set_halt,
	.ep_clear_halt = udc_mchp_g2_ep_clear_halt,
	.ep_enqueue = udc_mchp_g2_ep_enqueue,
	.ep_dequeue = udc_mchp_g2_ep_dequeue,
};

#define UDC_MCHP_G2_IRQ_ENABLE(n)                                                                  \
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mchp_g2_isr_handler,                \
		    DEVICE_DT_INST_GET(n), 0);                                                     \
	irq_enable(DT_INST_IRQN(n));

#define UDC_MCHP_G2_IRQ_DISABLE(n) irq_disable(DT_INST_IRQN(n));

#define UDC_MCHP_G2_DEVICE_DEFINE(n)                                                               \
	K_THREAD_STACK_DEFINE(udc_mchp_g2_stack_##n, CONFIG_UDC_MCHP_G2_STACK_SIZE);               \
                                                                                                   \
	static void udc_mchp_g2_irq_enable_func_##n(const struct device *dev)                      \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		UDC_MCHP_G2_IRQ_ENABLE(n)                                                          \
	}                                                                                          \
                                                                                                   \
	static void udc_mchp_g2_irq_disable_func_##n(const struct device *dev)                     \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		UDC_MCHP_G2_IRQ_DISABLE(n)                                                         \
	}                                                                                          \
                                                                                                   \
	static struct udc_ep_config ep_cfg_out_##n[DT_INST_PROP(n, num_out_endpoints)];            \
	static struct udc_ep_config ep_cfg_in_##n[DT_INST_PROP(n, num_in_endpoints)];              \
                                                                                                   \
	BUILD_ASSERT(DT_INST_PROP(n, num_in_endpoints) == DT_INST_PROP(n, num_out_endpoints),      \
		     "IN and OUT endpoint counts must match");                                     \
                                                                                                   \
	static const struct udc_mchp_g2_config udc_mchp_g2_config_##n = {                          \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.num_of_eps = DT_INST_PROP(n, num_in_endpoints),                                   \
		.ep_cfg_in = ep_cfg_in_##n,                                                        \
		.ep_cfg_out = ep_cfg_out_##n,                                                      \
		.thread_stk = udc_mchp_g2_stack_##n,                                               \
		.thread_stk_sz = K_THREAD_STACK_SIZEOF(udc_mchp_g2_stack_##n),                     \
		.speed_idx = DT_ENUM_IDX(DT_DRV_INST(n), maximum_speed),                           \
		.irq_enable_func = udc_mchp_g2_irq_enable_func_##n,                                \
		.irq_disable_func = udc_mchp_g2_irq_disable_func_##n,                              \
		.vbus_poll_ms = DT_INST_PROP_OR(n, vbus_poll_period_ms,                            \
						CONFIG_UDC_MCHP_G2_VBUS_POLL_PERIOD_MS),           \
	};                                                                                         \
                                                                                                   \
	static struct udc_mchp_g2_data udc_priv_##n = {};                                          \
                                                                                                   \
	static struct udc_data udc_data_##n = {                                                    \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),                                  \
		.priv = &udc_priv_##n,                                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, udc_mchp_g2_driver_preinit, NULL, &udc_data_##n,                  \
			      &udc_mchp_g2_config_##n, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_mchp_g2_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_MCHP_G2_DEVICE_DEFINE)
