/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mentor_musb

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

#include "udc_common.h"
#include "udc_musb.h"

LOG_MODULE_REGISTER(udc_musb, CONFIG_UDC_DRIVER_LOG_LEVEL);

/* Round size up to the nearest power-of-2 FIFO code (8B..4096B); INVALID if size > 4096. */
static uint8_t musb_fifo_size_code(uint16_t size)
{
	uint16_t fifo_size = MUSB_FIFO_UNIT_SIZE;
	uint8_t code = 0U;

	while ((fifo_size < size) && (code < MUSB_FIFO_SIZE_CODE_MAX)) {
		fifo_size <<= 1;
		code++;
	}

	if (fifo_size < size) {
		return MUSB_FIFO_SIZE_CODE_INVALID;
	}

	return code;
}

/* Allocate a contiguous FIFO block for an endpoint. Returns address or INVALID. */
static uint16_t musb_fifo_allocate(struct udc_musb_data *priv, uint16_t size)
{
	uint32_t required;
	uint32_t total_units;
	uint32_t start_idx = 0U;
	uint32_t counted = 0U;
	uint32_t i;
	bool is_used;

	if (size == 0U || (size % MUSB_FIFO_UNIT_SIZE) != 0U) {
		return MUSB_FIFO_ADDR_INVALID;
	}

	required = size / MUSB_FIFO_UNIT_SIZE;
	total_units = priv->fifo_units;

	/* Scan for a contiguous free run of the required length. */
	for (i = 0U; i < total_units; i++) {
		is_used = (priv->fifo_allocation_table[i / MUSB_FIFO_UNITS_PER_WORD] &
			   BIT(i % MUSB_FIFO_UNITS_PER_WORD)) != 0U;

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
		return MUSB_FIFO_ADDR_INVALID;
	}

	/* Mark allocated units. */
	for (i = start_idx; i < (start_idx + required); i++) {
		priv->fifo_allocation_table[i / MUSB_FIFO_UNITS_PER_WORD] |=
			BIT(i % MUSB_FIFO_UNITS_PER_WORD);
	}

	return (uint16_t)(start_idx * MUSB_FIFO_UNIT_SIZE);
}

/* Process completed EP1-15 IN transfers. */
static void musb_handle_bulk_tx(const struct device *dev)
{
	struct udc_musb_data *priv = udc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	uint32_t done = (uint32_t)atomic_clear(&priv->bulk_tx_done);
	uint8_t ep_idx;
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	struct udc_buf_info *bi;
	k_spinlock_key_t key;
	uint16_t mps;
	mem_addr_t fifo;
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
			key = k_spin_lock(&priv->lock);
			mps = udc_mps_ep_size(ep_cfg);
			fifo = musb_fifo_addr(base, ep_idx);

			musb_write8(&base->index, MUSB_INDEX_SELEP(ep_idx));
			chunk = MIN(buf->len, (size_t)mps);

			for (i = 0; i < chunk; i++) {
				sys_write8(buf->data[i], fifo);
			}
			net_buf_pull(buf, chunk);
			musb_set8(&base->txcsrl, MUSB_TXCSRL_TXPKTRDY_Msk);
			musb_write8(&base->index, MUSB_INDEX_SELEP(0));
			k_spin_unlock(&priv->lock, key);
		} else {
			bi = udc_get_buf_info(buf);

			if (bi->zlp != 0U) {
				/* Skip ZLP while halted; ep_clear_halt will re-arm. */
				if (ep_cfg->stat.halted) {
					continue;
				}
				key = k_spin_lock(&priv->lock);

				musb_write8(&base->index, MUSB_INDEX_SELEP(ep_idx));
				musb_set8(&base->txcsrl, MUSB_TXCSRL_TXPKTRDY_Msk);
				bi->zlp = 0;
				musb_write8(&base->index, MUSB_INDEX_SELEP(0));
				k_spin_unlock(&priv->lock, key);
			} else {
				/* Dequeue completed transfer. */
				udc_submit_ep_event(dev, udc_buf_get(ep_cfg), 0);
			}
		}
	}
}

/* Handle EP0 Control Write completion. */
static void musb_handle_ep0_rx_done(const struct device *dev)
{
	struct udc_musb_data *priv = udc_get_private(dev);
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
			memset(setup_hold->data, 0, MUSB_SETUP_PACKET_SIZE);
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
		memset(setup_hold->data, 0, MUSB_SETUP_PACKET_SIZE);
		setup_hold->len = 0;
		udc_buf_put(ep0_out, setup_hold);
	}
}

/* Write next chunk of EP0 IN data to FIFO and arm TxPktRdy. */
static void musb_handle_ep0_tx(const struct device *dev, struct net_buf *buf)
{
	struct udc_musb_data *priv = udc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	uint16_t mps = udc_mps_ep_size(udc_get_ep_cfg(dev, USB_CONTROL_EP_IN));
	mem_addr_t fifo = musb_fifo_addr(base, 0);
	size_t chunk;
	size_t i;
	uint8_t csr0;

	/* Write next chunk */
	chunk = MIN(buf->len, (size_t)mps);

	for (i = 0; i < chunk; i++) {
		sys_write8(buf->data[i], fifo);
	}
	net_buf_pull(buf, chunk);

	csr0 = MUSB_CSR0L_TXPKTRDY_Msk;

	/* Set DataEnd on the last packet, including a deferred trailing ZLP
	 * when the previous packet was an exact multiple of the EP0 MPS.
	 */
	if (buf->len == 0) {
		csr0 |= MUSB_CSR0L_DATAEND_Msk;
		udc_get_buf_info(buf)->zlp = 0U;
	}
	priv->ep0_state = MUSB_EP0_STATE_TX;
	musb_set8(&base->csr0l, csr0);
}

/* Resume OUT transfers if next buffer is pre-queued */
static void udc_musb_resume_out(struct usb_musb_reg *const base, struct udc_musb_data *priv,
				uint8_t ep_idx)
{
	k_spinlock_key_t key;

	if (atomic_test_and_clear_bit(&priv->bulk_rx_nak, ep_idx)) {
		key = k_spin_lock(&priv->lock);
		musb_write8(&base->index, MUSB_INDEX_SELEP(ep_idx));
		musb_clear8(&base->rxcsrl, MUSB_RXCSRL_RXPKTRDY_Msk);
		musb_write8(&base->index, MUSB_INDEX_SELEP(0));
		k_spin_unlock(&priv->lock, key);
	}
}

/* Driver thread: waits for events and dispatches handlers. */
static void musb_thread_handler(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = (const struct device *)arg1;
	struct udc_musb_data *priv = udc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	uint32_t events;
	uint32_t done;
	uint8_t ep_idx;
	struct udc_ep_config *ep_cfg;
	struct net_buf *rx_buf;
	struct udc_ep_config *ep_in;
	struct net_buf *buf;

	while (true) {
		events = k_event_wait(&priv->events,
				      MUSB_EVT_SETUP | MUSB_EVT_XFER | MUSB_EVT_BUS_RESET |
					      MUSB_EVT_SUSPEND | MUSB_EVT_RESUME |
					      MUSB_EVT_BULK_TX | MUSB_EVT_BULK_RX,
				      false, K_FOREVER);

		if ((events & MUSB_EVT_BUS_RESET) != 0U) {
			k_event_clear(&priv->events, MUSB_EVT_BUS_RESET);
			udc_submit_event(dev, UDC_EVT_RESET, 0);
		}

		if ((events & MUSB_EVT_SUSPEND) != 0U) {
			k_event_clear(&priv->events, MUSB_EVT_SUSPEND);
			udc_set_suspended(dev, true);
			udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
		}

		if ((events & MUSB_EVT_RESUME) != 0U) {
			k_event_clear(&priv->events, MUSB_EVT_RESUME);
			udc_set_suspended(dev, false);
			udc_submit_event(dev, UDC_EVT_RESUME, 0);
		}

		/* EP1-15 TX done */
		if ((events & MUSB_EVT_BULK_TX) != 0U) {
			k_event_clear(&priv->events, MUSB_EVT_BULK_TX);
			musb_handle_bulk_tx(dev);
		}

		/* EP1-15 RX done */
		if ((events & MUSB_EVT_BULK_RX) != 0U) {
			k_event_clear(&priv->events, MUSB_EVT_BULK_RX);

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
						udc_musb_resume_out(base, priv, ep_idx);
					}

					udc_submit_ep_event(dev, rx_buf, 0);
				}
			}
		}

		if ((events & MUSB_EVT_XFER) != 0U) {
			k_event_clear(&priv->events, MUSB_EVT_XFER);
			ep_in = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);

			if (atomic_clear(&priv->ep0_rx_done) != 0) {
				musb_handle_ep0_rx_done(dev);
			}

			buf = ep_in ? udc_buf_peek(ep_in) : NULL;

			if (buf != NULL && (buf->len > 0 || udc_get_buf_info(buf)->zlp != 0U)) {
				musb_handle_ep0_tx(dev, buf);
			} else if (buf != NULL) {
				udc_submit_ep_event(dev, udc_buf_get(ep_in), 0);
			}
		}

		if ((events & MUSB_EVT_SETUP) != 0U) {
			uint8_t setup[MUSB_SETUP_PACKET_SIZE];
			k_spinlock_key_t key;
			bool pending;

			k_event_clear(&priv->events, MUSB_EVT_SETUP);

			/* Drain every queued SETUP in arrival order. Copy out under
			 * lock so the ISR cannot overwrite the slot mid-read.
			 */
			while (true) {
				key = k_spin_lock(&priv->lock);
				pending = (priv->setup_q_tail != priv->setup_q_head);
				if (pending) {
					memcpy(setup,
					       priv->setup_q[priv->setup_q_tail &
							     MUSB_SETUP_Q_MASK],
					       sizeof(setup));
					priv->setup_q_tail++;
				}
				k_spin_unlock(&priv->lock, key);

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
static bool musb_vbus_is_present(struct usb_musb_reg *const base)
{
	uint8_t vbus = musb_read8(&base->devctl) & MUSB_DEVCTL_VBUS_Msk;

	return (vbus == MUSB_DEVCTL_VBUS_AVBUSVALID);
}

/* VBUS polling work handler. Fires VBUS_READY/REMOVED events on state change. */
static void musb_vbus_poll_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct udc_musb_data *priv = CONTAINER_OF(dwork, struct udc_musb_data, vbus_work);
	const struct udc_musb_config *cfg = priv->dev->config;
	const bool present = musb_vbus_is_present(musb_get_base(priv->dev));

	if (present != priv->vbus_present) {
		priv->vbus_present = present;

		if (present) {
			LOG_DBG("VBUS detected");
			udc_submit_event(priv->dev, UDC_EVT_VBUS_READY, 0);
		} else {
			LOG_DBG("VBUS removed");
			udc_submit_event(priv->dev, UDC_EVT_VBUS_REMOVED, 0);
		}
	}

	k_work_reschedule(dwork, K_MSEC(cfg->vbus_poll_ms));
}

/* Read len bytes from an MUSB FIFO using 32-bit word accesses. */
static void musb_fifo_read(mem_addr_t fifo, uint8_t *dst, uint32_t len)
{
	uint32_t words = len / MUSB_FIFO_WORD_SIZE;
	uint32_t rem = len % MUSB_FIFO_WORD_SIZE;
	uint32_t i;
	uint32_t j;
	uint32_t word;

	for (i = 0U; i < words; i++) {
		word = sys_read32(fifo);

		*dst++ = (uint8_t)(word);
		*dst++ = (uint8_t)(word >> 8U);
		*dst++ = (uint8_t)(word >> 16U);
		*dst++ = (uint8_t)(word >> 24U);
	}

	/* Remaining bytes (< 4). */
	if (rem > 0U) {
		word = sys_read32(fifo);

		for (j = 0U; j < rem; j++) {
			*dst++ = (uint8_t)(word >> (j * 8U));
		}
	}
}

/*
 * Handle USB bus events (reset, suspend, resume, SOF).
 * Returns true on bus reset so the ISR exits immediately.
 */
static bool musb_handle_usb_events(struct usb_musb_reg *const base,
					      struct udc_musb_data *priv, uint8_t intrusb)
{
	if ((intrusb & MUSB_INTRUSB_RESET_Msk) != 0U) {
		priv->ep0_state = MUSB_EP0_STATE_IDLE;
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
		musb_set16(&base->intrtxe, MUSB_INTRTXE_EP0TXEN_Msk);
		musb_write8(&base->index, MUSB_INDEX_SELEP(0));
		musb_write8(&base->txfifosz,
			    MUSB_TXFIFOSZ_SZ(musb_fifo_size_code(USB_CONTROL_EP_MPS)));
		musb_write8(&base->rxfifosz,
			    MUSB_RXFIFOSZ_SZ(musb_fifo_size_code(USB_CONTROL_EP_MPS)));
		musb_write16(&base->txfifoadd,
			     MUSB_TXFIFOADD_ADDR(priv->fifo_in_addr[0] / MUSB_FIFO_UNIT_SIZE));
		musb_write16(&base->rxfifoadd,
			     MUSB_RXFIFOADD_ADDR(priv->fifo_out_addr[0] / MUSB_FIFO_UNIT_SIZE));

		k_event_post(&priv->events, MUSB_EVT_BUS_RESET);
		return true;
	}

	if ((intrusb & MUSB_INTRUSB_SUSPEND_Msk) != 0U) {
		k_event_post(&priv->events, MUSB_EVT_SUSPEND);
	}

	if ((intrusb & MUSB_INTRUSB_RESUME_Msk) != 0U) {
		k_event_post(&priv->events, MUSB_EVT_RESUME);
	}

	if (IS_ENABLED(CONFIG_UDC_ENABLE_SOF) && ((intrusb & MUSB_INTRUSB_SOF_Msk) != 0U)) {
		udc_submit_sof_event(priv->dev);
	}

	return false;
}

/*
 * Read EP0 CSR0L and resolve SentStall/SetupEnd before the state machine,
 * returning the CSR0L value to process. SendStall is never cleared here;
 * clearing it before the STALL is sent would cancel the handshake.
 */
static uint8_t musb_handle_ep0_csr(struct usb_musb_reg *const base, struct udc_musb_data *priv)
{
	uint8_t csr0l;

	/* Restore INDEX=0; a bulk ep_enqueue may have left it non-zero. */
	musb_write8(&base->index, MUSB_INDEX_SELEP(0));
	csr0l = musb_read8(&base->csr0l);

	/* Clear SentStall; hardware does not auto-clear it. Fall through to handle any pending
	 * SETUP.
	 */
	if ((csr0l & MUSB_CSR0L_SENTSTALL_Msk) != 0U) {
		musb_clear8(&base->csr0l, MUSB_CSR0L_SENTSTALL_Msk);
		priv->ep0_state = MUSB_EP0_STATE_IDLE;
		csr0l = musb_read8(&base->csr0l); /* re-read after clear */
	}

	if ((csr0l & MUSB_CSR0L_SETUPEND_Msk) != 0U) {
		/* Clear SetupEnd. */
		musb_set8(&base->csr0l, MUSB_CSR0L_SERVICEDSETUPEND_Msk);
		priv->ep0_state = MUSB_EP0_STATE_IDLE;
		/* Re-read CSR0L: a new SETUP may already be waiting in the FIFO. */
		csr0l = musb_read8(&base->csr0l);
		/* Fall through to process any pending SETUP in IDLE state. */
	}

	return csr0l;
}

/*
 * IDLE state: read SETUP packet and advance EP0 state.
 * w_length==0 : STATUS_IN, bm_request_type[7] : TX (Control Read), else : RX.
 */
static void musb_ep0_state_idle(struct usb_musb_reg *const base, struct udc_musb_data *priv,
				   uint8_t csr0l)
{
	mem_addr_t fifo;
	uint32_t *setup_ptr;
	uint8_t *setup;
	uint8_t bm_request_type;
	uint16_t w_length;

	if ((csr0l & MUSB_CSR0L_RXPKTRDY_Msk) == 0U) {
		return;
	}

	/* Read SETUP packet from FIFO into the next queue slot. If the queue is
	 * full, drop the oldest entry. The caller holds priv->lock, which the
	 * thread also holds while it updates the tail.
	 */
	if ((uint8_t)(priv->setup_q_head - priv->setup_q_tail) >= MUSB_SETUP_Q_DEPTH) {
		priv->setup_q_tail++;
	}

	setup = priv->setup_q[priv->setup_q_head & MUSB_SETUP_Q_MASK];

	fifo = musb_fifo_addr(base, 0);
	setup_ptr = (uint32_t *)setup;

	setup_ptr[0] = sys_read32(fifo);
	setup_ptr[1] = sys_read32(fifo);

	bm_request_type = (uint8_t)(setup_ptr[0] & MUSB_SETUP_BMREQTYPE_MASK);
	w_length = (uint16_t)((setup_ptr[1] >> MUSB_SETUP_WLENGTH_SHIFT) &
			  MUSB_SETUP_WLENGTH_MASK);

	if (w_length == 0) {
		/* Zero-data request (e.g. SET_ADDRESS). ACK and move to STATUS_IN. */
		musb_set8(&base->csr0l, MUSB_CSR0L_SERVICEDRXPKTRDY_Msk |
			MUSB_CSR0L_DATAEND_Msk);
		priv->ep0_state = MUSB_EP0_STATE_STATUS_IN;
	} else if ((bm_request_type & USB_EP_DIR_IN) != 0U) {
		/* Control Read (IN) */
		musb_set8(&base->csr0l, MUSB_CSR0L_SERVICEDRXPKTRDY_Msk);
		priv->ep0_state = MUSB_EP0_STATE_TX;
	} else {
		/* Control Write (OUT) */
		musb_set8(&base->csr0l, MUSB_CSR0L_SERVICEDRXPKTRDY_Msk);
		priv->ep0_state = MUSB_EP0_STATE_RX;
		/* Snapshot SETUP bytes for the control-write data phase. */
		memcpy(priv->ep0_ctrl_write_setup, setup, MUSB_SETUP_PACKET_SIZE);
		priv->ep0_ctrl_write_len = w_length;
		/* Reset per-transfer state; the thread may not have processed the
		 * previous transfer before the host issues the next SETUP.
		 */
		priv->ep0_ctrl_bytes_received = 0U;
		priv->ep0_rx_buf = NULL;
		atomic_clear(&priv->ep0_rx_pending);
	}

	/* Publish the slot to the thread only after it is fully written. */
	priv->setup_q_head++;
	k_event_post(&priv->events, MUSB_EVT_SETUP);
}

/* STATUS_IN: Status ZLP ACKed by host. Guard on INTRTX[0] to ignore spurious events. */
static void musb_ep0_state_status_in(struct udc_musb_data *priv,
					uint16_t tx_irqs)
{
	if ((tx_irqs & MUSB_INTRTX_EP0TX_Msk) == 0U) {
		return; /* Not a real EP0 event. */
	}

	priv->ep0_state = MUSB_EP0_STATE_IDLE;
	k_event_post(&priv->events, MUSB_EVT_XFER);
}

/*
 * TX state: EP0 IN packet sent. Guard on INTRTX[0] to ignore spurious events.
 * Post EVT_XFER when TxPktRdy clears so the thread sends the next chunk.
 */
static void musb_ep0_state_tx(struct udc_musb_data *priv, uint8_t csr0l,
				 uint16_t tx_irqs)
{
	if ((tx_irqs & MUSB_INTRTX_EP0TX_Msk) == 0U) {
		return; /* Spurious event. */
	}

	if ((csr0l & MUSB_CSR0L_TXPKTRDY_Msk) == 0U) {
		priv->ep0_state = MUSB_EP0_STATE_IDLE;
		k_event_post(&priv->events, MUSB_EVT_XFER);
	}
}

/*
 * RX state: receive Control Write data directly into the UDC-provided buffer.
 */
static void musb_ep0_state_rx(const struct device *dev, struct usb_musb_reg *const base,
				 struct udc_musb_data *priv, uint8_t csr0l)
{
	uint8_t count;
	mem_addr_t fifo;
	uint32_t size;
	struct net_buf *buf;
	bool last;

	if ((csr0l & MUSB_CSR0L_RXPKTRDY_Msk) == 0U) {
		return;
	}

	count = musb_read8(&base->count0) & MUSB_COUNT0_EP0RXCOUNT_Msk;
	buf = priv->ep0_rx_buf;

	if (count > 0U) {
		if (buf == NULL) {
			/* No buffer yet; hold RxPktRdy until ep_enqueue arms ep0_rx_buf. */
			atomic_set(&priv->ep0_rx_pending, 1);
			return;
		}

		fifo = musb_fifo_addr(base, 0);
		size = MIN((uint32_t)count, net_buf_tailroom(buf));
		musb_fifo_read(fifo, net_buf_add(buf, size), size);
		priv->ep0_ctrl_bytes_received += (uint16_t)count;
	}

	/* Transfer complete on short packet OR when all expected bytes received. */
	last = ((uint16_t)count < USB_CONTROL_EP_MPS) ||
	       (priv->ep0_ctrl_bytes_received >= priv->ep0_ctrl_write_len);

	if (last) {
		/* Last packet: ACK with DataEnd to arm Status IN ZLP. */
		musb_set8(&base->csr0l, MUSB_CSR0L_SERVICEDRXPKTRDY_Msk |
			MUSB_CSR0L_DATAEND_Msk);

		priv->ep0_state = MUSB_EP0_STATE_STATUS_IN;
		atomic_set(&priv->ep0_rx_done, 1);
	} else {
		/* More data expected; ACK without DataEnd. */
		musb_set8(&base->csr0l, MUSB_CSR0L_SERVICEDRXPKTRDY_Msk);
	}

	k_event_post(&priv->events, MUSB_EVT_XFER);
}

/* Handle EP1-15 IN completions. SentStall shares the TX interrupt bit; clear and flush if set. */
static void musb_handle_epx_tx_done(struct udc_musb_data *priv, struct usb_musb_reg *const base,
				       uint16_t tx_irqs)
{
	uint16_t tx = tx_irqs >> 1; /* shift out EP0 bit */
	uint8_t ep = 1U;
	bool any_done = false;
	k_spinlock_key_t key;
	uint8_t txcsrl;

	while (tx != 0U) {
		if ((tx & 1U) != 0U) {
			key = k_spin_lock(&priv->lock);

			musb_write8(&base->index, MUSB_INDEX_SELEP(ep));

			txcsrl = musb_read8(&base->txcsrl);

			if ((txcsrl & MUSB_TXCSRL_SENTSTALL_Msk) != 0U) {
				/* Clear SentStall, flush FIFO, dequeue aborted buffer. */
				musb_clear8(&base->txcsrl, MUSB_TXCSRL_SENTSTALL_Msk);
				musb_set8(&base->txcsrl, MUSB_TXCSRL_FLUSHFIFO_Msk);
				if ((musb_read8(&base->txcsrl) &
				     MUSB_TXCSRL_FIFONOTEMPTY_Msk) != 0U) {
					musb_set8(&base->txcsrl, MUSB_TXCSRL_FLUSHFIFO_Msk);
				}
				musb_clear8(&base->txcsrl, MUSB_TXCSRL_TXPKTRDY_Msk);
			}
			atomic_set_bit(&priv->bulk_tx_done, ep);
			any_done = true;

			musb_write8(&base->index, MUSB_INDEX_SELEP(0));
			k_spin_unlock(&priv->lock, key);
		}
		tx >>= 1;
		ep++;
	}

	if (any_done) {
		k_event_post(&priv->events, MUSB_EVT_BULK_TX);
	}
}

/*
 * Handle EP1-15 OUT data. For each active endpoint:
 *   short packet or ZLP : clear RxPktRdy, mark done.
 *   full-MPS, buffer full : hold RxPktRdy (NAK host), mark done.
 *   full-MPS, buffer not full : clear RxPktRdy, accumulate.
 *   no buffer : clear RxPktRdy (drop bytes).
 */
static void musb_handle_epx_rx_done(const struct device *dev, struct usb_musb_reg *const base,
				       struct udc_musb_data *priv, uint16_t rx_irqs)
{
	uint16_t rx = rx_irqs >> 1; /* bit 0 unused */
	uint8_t ep = 1U;
	k_spinlock_key_t key;
	uint8_t rxcsrl;
	uint16_t count;
	struct udc_ep_config *ep_cfg;
	struct net_buf *rx_buf;
	mem_addr_t fifo;
	uint32_t size;
	uint8_t *data;
	uint16_t ep_mps;
	bool short_packet;
	bool buf_full;

	while (rx != 0U) {
		if ((rx & 1U) != 0U) {
			key = k_spin_lock(&priv->lock);

			musb_write8(&base->index, MUSB_INDEX_SELEP(ep));

			rxcsrl = musb_read8(&base->rxcsrl);

			if ((rxcsrl & MUSB_RXCSRL_SENTSTALL_Msk) != 0U) {
				/* SentStall: clear bit and skip FIFO. */
				musb_write8(&base->rxcsrl, rxcsrl &
					~MUSB_RXCSRL_SENTSTALL_Msk);
				musb_write8(&base->index, MUSB_INDEX_SELEP(0));
				k_spin_unlock(&priv->lock, key);
			} else {
				count = musb_read16(&base->rxcount) &
					MUSB_RXCOUNT_ENDPOINTRXCOUNT_Msk;

				ep_cfg = udc_get_ep_cfg(dev, (uint8_t)ep);
				rx_buf = ep_cfg ? udc_buf_peek(ep_cfg) : NULL;

				if (rx_buf != NULL && count > 0U) {
					fifo = musb_fifo_addr(base, ep);
					size = MIN((uint32_t)count, net_buf_tailroom(rx_buf));
					data = net_buf_add(rx_buf, size);

					musb_fifo_read(fifo, data, size);

					ep_mps = udc_mps_ep_size(ep_cfg);
					short_packet = ((uint16_t)count < ep_mps);
					buf_full = (net_buf_tailroom(rx_buf) == 0U);

					if (short_packet) {
						/* End of transfer; ACK. */
						musb_clear8(&base->rxcsrl,
							    MUSB_RXCSRL_RXPKTRDY_Msk);
						atomic_set_bit(&priv->bulk_rx_done, ep);
					} else if (buf_full) {
						/* Buffer full; hold NAK until new buffer queued. */
						atomic_set_bit(&priv->bulk_rx_nak, ep);
						atomic_set_bit(&priv->bulk_rx_done, ep);
					} else {
						/* Buffer not full; ACK and continue accumulating.
						 */
						musb_clear8(&base->rxcsrl,
							    MUSB_RXCSRL_RXPKTRDY_Msk);
					}
				} else if (rx_buf != NULL && count == 0U) {
					/* ZLP: transfer complete, submit accumulated data. */
					musb_clear8(&base->rxcsrl, MUSB_RXCSRL_RXPKTRDY_Msk);
					atomic_set_bit(&priv->bulk_rx_done, ep);
				} else {
					/* No buffer; hold RxPktRdy, drain on next enqueue. */
					atomic_set_bit(&priv->bulk_rx_pending, ep);
				}

				musb_write8(&base->index, MUSB_INDEX_SELEP(0));
				k_spin_unlock(&priv->lock, key);
			}
		}
		rx >>= 1;
		ep++;
	}

	if ((rx_irqs & MUSB_INTRRXE_Msk) != 0U) {
		k_event_post(&priv->events, MUSB_EVT_BULK_RX);
	}
}

/* Dispatch to the EP0 state machine handler for the current state. */
static void musb_handle_ep0_state(const struct device *dev, struct usb_musb_reg *const base,
				     struct udc_musb_data *priv, uint8_t csr0l,
				     uint16_t tx_irqs)
{
	switch (priv->ep0_state) {
	case MUSB_EP0_STATE_IDLE:
		musb_ep0_state_idle(base, priv, csr0l);
		break;
	case MUSB_EP0_STATE_STATUS_IN:
		musb_ep0_state_status_in(priv, tx_irqs);
		break;
	case MUSB_EP0_STATE_TX:
		musb_ep0_state_tx(priv, csr0l, tx_irqs);
		break;
	case MUSB_EP0_STATE_RX:
		musb_ep0_state_rx(dev, base, priv, csr0l);
		break;
	}
}

static void udc_musb_isr_handler(const struct device *dev)
{
	struct udc_musb_data *priv = udc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);

	uint16_t tx_irqs;
	uint16_t rx_irqs;
	uint8_t intrusb;
	uint8_t csr0l;

	k_spinlock_key_t key;

	if (musb_quirk_irq_clear(dev) != 0) {
		return;
	}

	/* Interrupt status registers are cleared on read. */
	intrusb = musb_read8(&base->intrusb);
	tx_irqs = musb_read16(&base->intrtx);
	rx_irqs = musb_read16(&base->intrrx);

	key = k_spin_lock(&priv->lock);

	if (musb_handle_usb_events(base, priv, intrusb)) {
		k_spin_unlock(&priv->lock, key);
		return;
	}

	csr0l = musb_handle_ep0_csr(base, priv);
	musb_handle_ep0_state(dev, base, priv, csr0l, tx_irqs);

	k_spin_unlock(&priv->lock, key);

	/* The endpoint handlers take the lock for each endpoint. */
	musb_handle_epx_tx_done(priv, base, tx_irqs);
	musb_handle_epx_rx_done(dev, base, priv, rx_irqs);

	/* Handle any back-to-back SETUP packet before leaving the interrupt. */
	key = k_spin_lock(&priv->lock);
	if (priv->ep0_state == MUSB_EP0_STATE_IDLE) {
		musb_write8(&base->index, MUSB_INDEX_SELEP(0));
		csr0l = musb_read8(&base->csr0l);
		musb_ep0_state_idle(base, priv, csr0l);
	}
	k_spin_unlock(&priv->lock, key);
}

/*
 * EP1-15 OUT enqueue. Handles two deferred-NAK cases:
 *   bulk_rx_nak: buffer boundary - un-NAK now.
 *   bulk_rx_pending: data in HW FIFO with no buffer - drain now.
 */
static int musb_enqueue_epx_out(const struct device *dev, struct usb_musb_reg *const base,
				   uint8_t ep_idx)
{
	struct udc_musb_data *priv = udc_get_private(dev);
	k_spinlock_key_t key;
	bool do_bulk_rx;
	struct udc_ep_config *ep_cfg;
	struct net_buf *rx_buf;
	uint16_t count;
	mem_addr_t fifo;
	uint32_t size;
	uint8_t *data;
	uint16_t ep_mps;
	bool short_packet;
	bool buf_full;

	/* Path 1: buffer boundary NAK - un-NAK. */
	if (atomic_test_and_clear_bit(&priv->bulk_rx_nak, ep_idx)) {
		key = k_spin_lock(&priv->lock);

		musb_write8(&base->index, MUSB_INDEX_SELEP(ep_idx));
		musb_clear8(&base->rxcsrl, MUSB_RXCSRL_RXPKTRDY_Msk);
		musb_write8(&base->index, MUSB_INDEX_SELEP(0));
		k_spin_unlock(&priv->lock, key);
	}

	/* Path 2: data in HW FIFO with no buffer - drain now. */
	if (atomic_test_and_clear_bit(&priv->bulk_rx_pending, ep_idx)) {
		key = k_spin_lock(&priv->lock);
		do_bulk_rx = false;

		musb_write8(&base->index, MUSB_INDEX_SELEP(ep_idx));

		ep_cfg = udc_get_ep_cfg(dev, (uint8_t)ep_idx);
		rx_buf = ep_cfg ? udc_buf_peek(ep_cfg) : NULL;

		if (rx_buf != NULL) {
			count = musb_read16(&base->rxcount) &
				MUSB_RXCOUNT_ENDPOINTRXCOUNT_Msk;
			fifo = musb_fifo_addr(base, ep_idx);
			size = MIN((uint32_t)count, net_buf_tailroom(rx_buf));
			data = net_buf_add(rx_buf, size);

			musb_fifo_read(fifo, data, size);

			ep_mps = udc_mps_ep_size(ep_cfg);
			short_packet = ((uint16_t)count < ep_mps);
			buf_full = (net_buf_tailroom(rx_buf) == 0U);

			if (short_packet) {
				/* End of transfer; un-NAK and deliver. */
				musb_clear8(&base->rxcsrl, MUSB_RXCSRL_RXPKTRDY_Msk);
				atomic_set_bit(&priv->bulk_rx_done, ep_idx);
				do_bulk_rx = true;
			} else if (buf_full) {
				/* Buffer full; hold NAK, deliver, next enqueue will un-NAK. */
				atomic_set_bit(&priv->bulk_rx_nak, ep_idx);
				atomic_set_bit(&priv->bulk_rx_done, ep_idx);
				do_bulk_rx = true;
			} else {
				/* More space in buffer; un-NAK and continue. */
				musb_clear8(&base->rxcsrl, MUSB_RXCSRL_RXPKTRDY_Msk);
			}
		} else {
			/* No buffer (unexpected); clear RxPktRdy to avoid stall. */
			musb_clear8(&base->rxcsrl, MUSB_RXCSRL_RXPKTRDY_Msk);
		}

		musb_write8(&base->index, MUSB_INDEX_SELEP(0));
		k_spin_unlock(&priv->lock, key);

		if (do_bulk_rx) {
			k_event_post(&priv->events, MUSB_EVT_BULK_RX);
		}
	}

	return 0;
}

/*
 * EP1-15 IN enqueue: write first chunk to FIFO and set TxPktRdy.
 * Returns immediately if previous packet is still in flight.
 * Always restores INDEX=0 before unlocking to protect EP0 CSR access.
 */
static int musb_enqueue_epx_in(const struct device *dev, struct usb_musb_reg *const base,
				  struct udc_ep_config *const cfg, struct net_buf *buf)
{
	struct udc_musb_data *priv = udc_get_private(dev);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	k_spinlock_key_t key = k_spin_lock(&priv->lock);
	uint16_t ep_mps = udc_mps_ep_size(cfg);
	mem_addr_t fifo = musb_fifo_addr(base, ep_idx);
	size_t chunk;
	size_t i;

	musb_write8(&base->index, MUSB_INDEX_SELEP(ep_idx));

	if ((musb_read8(&base->txcsrl) & MUSB_TXCSRL_TXPKTRDY_Msk) != 0U) {
		/* Restore INDEX=0 before unlock; ISR reads CSR0L via indexed window. */
		musb_write8(&base->index, MUSB_INDEX_SELEP(0));
		k_spin_unlock(&priv->lock, key);
		return 0; /* busy, buf already queued */
	}

	chunk = MIN(buf->len, (size_t)ep_mps);

	for (i = 0; i < chunk; i++) {
		sys_write8(buf->data[i], fifo);
	}
	net_buf_pull(buf, chunk);

	musb_set8(&base->txcsrl, MUSB_TXCSRL_TXPKTRDY_Msk);

	/* Restore INDEX=0. */
	musb_write8(&base->index, MUSB_INDEX_SELEP(0));
	k_spin_unlock(&priv->lock, key);

	return 0;
}

/*
 * EP0 IN enqueue. Three cases:
 *   status ZLP : post EVT_XFER only (hardware already sent it).
 *   TxPktRdy set : FIFO busy.
 *   data/ZLP : write FIFO, set TxPktRdy, set DataEnd on last packet.
 * Restores INDEX=0 first; a prior bulk enqueue may have left it non-zero.
 */
static int musb_enqueue_ep0_in(const struct device *dev, struct usb_musb_reg *const base,
				  struct udc_ep_config *const cfg, struct net_buf *buf)
{
	struct udc_musb_data *priv = udc_get_private(dev);
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	uint16_t mps = udc_mps_ep_size(cfg);
	mem_addr_t fifo = musb_fifo_addr(base, 0);
	k_spinlock_key_t key;
	uint8_t csr0;
	size_t len;
	size_t i;

	if (bi->status != 0U) {
		/* Status ZLP sent by hardware; notify the thread. */
		LOG_DBG("ep_enqueue: status IN ZLP queued, posting EVT_XFER");
		k_event_post(&priv->events, MUSB_EVT_XFER);
		return 0;
	}

	key = k_spin_lock(&priv->lock);

	/* Select EP0 before accessing CSR0L. */
	musb_write8(&base->index, MUSB_INDEX_SELEP(0));

	csr0 = musb_read8(&base->csr0l);
	if ((csr0 & MUSB_CSR0L_TXPKTRDY_Msk) != 0U) {
		k_spin_unlock(&priv->lock, key);
		LOG_WRN("EP0 IN enqueue skipped, TxPktRdy set (CSR0L 0x%02x)", csr0);
		return 0;
	}

	if (buf->len == 0U && bi->zlp == 0U) {
		/* ZLP explicitly requested (not status) */
		priv->ep0_state = MUSB_EP0_STATE_TX;
		musb_set8(&base->csr0l, MUSB_CSR0L_TXPKTRDY_Msk | MUSB_CSR0L_DATAEND_Msk);
		k_spin_unlock(&priv->lock, key);
		return 0;
	}

	len = MIN(buf->len, (size_t)mps);
	for (i = 0; i < len; i++) {
		sys_write8(buf->data[i], fifo);
	}
	net_buf_pull(buf, len);

	csr0 |= MUSB_CSR0L_TXPKTRDY_Msk;
	if (len < mps || (buf->len == 0U && bi->zlp == 0U)) {
		csr0 |= MUSB_CSR0L_DATAEND_Msk;
	}

	priv->ep0_state = MUSB_EP0_STATE_TX;
	musb_write8(&base->csr0l, csr0);

	k_spin_unlock(&priv->lock, key);

	return 0;
}

/*
 * EP0 OUT enqueue.  Called when the class driver provides a receive buffer for
 * a Control Write data stage.
 */
static int musb_enqueue_ep0_out(const struct device *dev, struct usb_musb_reg *const base,
				   struct udc_ep_config *const cfg, struct net_buf *buf)
{
	struct udc_musb_data *priv = udc_get_private(dev);
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	k_spinlock_key_t key;
	uint8_t count;
	uint32_t size;
	mem_addr_t fifo;
	bool last;

	if (bi->setup != 0U) {
		/* SETUP buffer - handled by the ISR, nothing to arm here. */
		return 0;
	}

	key = k_spin_lock(&priv->lock);

	/* Arm ep0_rx_buf so the ISR writes subsequent packets directly. */
	priv->ep0_rx_buf = buf;

	/* Drain a packet held in the FIFO while ep0_rx_buf was NULL. */
	if (atomic_clear(&priv->ep0_rx_pending)) {
		musb_write8(&base->index, MUSB_INDEX_SELEP(0));
		count = musb_read8(&base->count0) & MUSB_COUNT0_EP0RXCOUNT_Msk;

		if (count > 0U) {
			fifo = musb_fifo_addr(base, 0);
			size = MIN((uint32_t)count, net_buf_tailroom(buf));
			musb_fifo_read(fifo, net_buf_add(buf, size), size);
			priv->ep0_ctrl_bytes_received += (uint16_t)count;
		}

		last = ((uint16_t)count < USB_CONTROL_EP_MPS) ||
		       (priv->ep0_ctrl_bytes_received >= priv->ep0_ctrl_write_len);

		if (last) {
			musb_set8(&base->csr0l, MUSB_CSR0L_SERVICEDRXPKTRDY_Msk |
				MUSB_CSR0L_DATAEND_Msk);
			priv->ep0_state = MUSB_EP0_STATE_STATUS_IN;
			atomic_set(&priv->ep0_rx_done, 1);
			k_event_post(&priv->events, MUSB_EVT_XFER);
		} else {
			/* Release RxPktRdy; host will send remaining packets. */
			musb_set8(&base->csr0l, MUSB_CSR0L_SERVICEDRXPKTRDY_Msk);
		}
	}

	k_spin_unlock(&priv->lock, key);
	return 0;
}

/* Enqueue a transfer request. Must not block. */
static int udc_musb_ep_enqueue(const struct device *dev, struct udc_ep_config *const cfg,
				  struct net_buf *buf)
{
	struct usb_musb_reg *const base = musb_get_base(dev);

	LOG_DBG("%p enqueue %p to ep 0x%02x, len=%u", dev, buf, cfg->addr, buf->len);
	udc_buf_put(cfg, buf);

	if (cfg->stat.halted) {
		LOG_DBG("ep 0x%02x halted", cfg->addr);
		return 0;
	}

	if (USB_EP_GET_IDX(cfg->addr) != 0U) {
		if (!USB_EP_DIR_IS_IN(cfg->addr)) {
			return musb_enqueue_epx_out(dev, base, USB_EP_GET_IDX(cfg->addr));
		}
		return musb_enqueue_epx_in(dev, base, cfg, buf);
	}

	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		return musb_enqueue_ep0_in(dev, base, cfg, buf);
	}

	return musb_enqueue_ep0_out(dev, base, cfg, buf);
}

/* Flush FIFO and cancel all queued requests for an endpoint. */
static int udc_musb_ep_dequeue(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_musb_data *priv = udc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);

	LOG_DBG("Dequeue ep 0x%02x", cfg->addr);

	k_spinlock_key_t lock_key = k_spin_lock(&priv->lock);

	if (ep_idx == 0U) {
		/* Restore INDEX=0 in case a prior bulk op left it non-zero. */
		musb_write8(&base->index, MUSB_INDEX_SELEP(0));
		musb_set8(&base->csr0h, MUSB_CSR0H_FLUSHFIFO_Msk);
		atomic_clear(&priv->ep0_rx_done);
		atomic_clear(&priv->ep0_rx_pending);
		priv->ep0_rx_buf = NULL;
		priv->ep0_state = MUSB_EP0_STATE_IDLE;
	} else if (USB_EP_DIR_IS_IN(cfg->addr)) {
		musb_clear16(&base->intrtxe, BIT(ep_idx));
		musb_write8(&base->index, MUSB_INDEX_SELEP(ep_idx));

		/* Double-flush for double-buffered endpoints. */
		musb_set8(&base->txcsrl, MUSB_TXCSRL_FLUSHFIFO_Msk);
		if ((musb_read8(&base->txcsrl) & MUSB_TXCSRL_FIFONOTEMPTY_Msk) !=
		    0U) {
			musb_set8(&base->txcsrl, MUSB_TXCSRL_FLUSHFIFO_Msk);
		}
		musb_clear8(&base->txcsrl, MUSB_TXCSRL_TXPKTRDY_Msk);
		atomic_clear_bit(&priv->bulk_tx_done, ep_idx);

		/* Restore INDEX=0 before re-enabling TX interrupt. */
		musb_write8(&base->index, MUSB_INDEX_SELEP(0));
		musb_set16(&base->intrtxe, BIT(ep_idx));
	} else {
		musb_clear16(&base->intrrxe, BIT(ep_idx));
		musb_write8(&base->index, MUSB_INDEX_SELEP(ep_idx));

		/* Double-flush for double-buffered endpoints. */
		musb_set8(&base->rxcsrl, MUSB_RXCSRL_FLUSHFIFO_Msk);
		if ((musb_read8(&base->rxcsrl) & MUSB_RXCSRL_RXPKTRDY_Msk) != 0U) {
			musb_set8(&base->rxcsrl, MUSB_RXCSRL_FLUSHFIFO_Msk);
		}
		/* Must clear RxPktRdy; leaving it set permanently NAKs OUT tokens. */
		musb_clear8(&base->rxcsrl, MUSB_RXCSRL_RXPKTRDY_Msk);
		atomic_clear_bit(&priv->bulk_rx_done, ep_idx);
		atomic_clear_bit(&priv->bulk_rx_nak, ep_idx);
		atomic_clear_bit(&priv->bulk_rx_pending, ep_idx);

		musb_write8(&base->index, MUSB_INDEX_SELEP(0));
		musb_set16(&base->intrrxe, BIT(ep_idx));
	}

	k_spin_unlock(&priv->lock, lock_key);

	udc_ep_cancel_queued(dev, cfg);

	return 0;
}

/* Allocate FIFO for an endpoint. EP0 IN and OUT share one block. */
static uint16_t musb_fifo_alloc_for_ep(struct udc_musb_data *priv, uint8_t ep_idx, bool is_in,
					  uint16_t size_bytes)
{
	k_spinlock_key_t key = k_spin_lock(&priv->lock);
	uint16_t addr;

	if (ep_idx == 0U) {
		/* EP0: IN and OUT share one block */
		addr = priv->fifo_in_addr[0];
		if (addr == MUSB_FIFO_ADDR_INVALID) {
			addr = musb_fifo_allocate(priv, size_bytes);
			if (addr != MUSB_FIFO_ADDR_INVALID) {
				priv->fifo_in_addr[0] = addr;
				priv->fifo_out_addr[0] = addr;
			}
		}
	} else if (is_in) {
		addr = priv->fifo_in_addr[ep_idx];
		if (addr == MUSB_FIFO_ADDR_INVALID) {
			addr = musb_fifo_allocate(priv, size_bytes);
			if (addr != MUSB_FIFO_ADDR_INVALID) {
				priv->fifo_in_addr[ep_idx] = addr;
			}
		}
	} else {
		addr = priv->fifo_out_addr[ep_idx];
		if (addr == MUSB_FIFO_ADDR_INVALID) {
			addr = musb_fifo_allocate(priv, size_bytes);
			if (addr != MUSB_FIFO_ADDR_INVALID) {
				priv->fifo_out_addr[ep_idx] = addr;
			}
		}
	}

	k_spin_unlock(&priv->lock, key);
	return addr;
}

/* Program FIFO and endpoint registers. Always restores INDEX=0 on exit. */
static void musb_ep_configure_fifo(struct usb_musb_reg *const base, struct udc_ep_config *const cfg,
				      uint8_t ep_idx, bool is_in, uint8_t size_code, uint16_t addr)
{
	/* TXFIFOADD/RXFIFOADD hold the FIFO start address in 8-byte units. */
	uint16_t addr_units = addr / MUSB_FIFO_UNIT_SIZE;

	musb_write8(&base->index, MUSB_INDEX_SELEP(ep_idx));

	if (ep_idx == 0U) {
		musb_write8(&base->txfifosz, MUSB_TXFIFOSZ_SZ(size_code));
		musb_write8(&base->rxfifosz, MUSB_RXFIFOSZ_SZ(size_code));
		musb_write16(&base->txfifoadd, MUSB_TXFIFOADD_ADDR(addr_units));
		musb_write16(&base->rxfifoadd, MUSB_RXFIFOADD_ADDR(addr_units));
	} else if (is_in) {
		musb_write8(&base->txfifosz, MUSB_TXFIFOSZ_SZ(size_code));
		musb_write16(&base->txfifoadd, MUSB_TXFIFOADD_ADDR(addr_units));
		musb_write16(&base->txmaxp, MUSB_TXMAXP_MAXPAYLOAD(udc_mps_ep_size(cfg)));
		musb_set8(&base->txcsrl, MUSB_TXCSRL_CLRDATATOG_Msk);
		musb_set16(&base->intrtxe, BIT(ep_idx));
	} else {
		musb_write8(&base->rxfifosz, MUSB_RXFIFOSZ_SZ(size_code));
		musb_write16(&base->rxfifoadd, MUSB_RXFIFOADD_ADDR(addr_units));
		musb_write16(&base->rxmaxp, MUSB_RXMAXP_MAXPAYLOAD(udc_mps_ep_size(cfg)));
		musb_set8(&base->rxcsrl, MUSB_RXCSRL_CLRDATATOG_Msk);
		musb_set16(&base->intrrxe, BIT(ep_idx));
	}

	musb_write8(&base->index, MUSB_INDEX_SELEP(0));
}

/* Enable endpoint. */
static int udc_musb_ep_enable(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_musb_data *priv = udc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	bool is_in = USB_EP_DIR_IS_IN(cfg->addr);
	k_spinlock_key_t key;
	uint16_t size_bytes;
	uint8_t size_code;
	uint16_t addr;

	LOG_DBG("Enable ep 0x%02x", cfg->addr);

	if (ep_idx >= MUSB_EP_MAX) {
		return -EINVAL;
	}

	size_bytes = MAX(udc_mps_ep_size(cfg), MUSB_FIFO_UNIT_SIZE);
	size_code = musb_fifo_size_code(size_bytes);

	if (size_code == MUSB_FIFO_SIZE_CODE_INVALID) {
		return -EINVAL;
	}
	size_bytes = MUSB_FIFO_UNIT_SIZE << size_code;

	addr = musb_fifo_alloc_for_ep(priv, ep_idx, is_in, size_bytes);

	if (addr == MUSB_FIFO_ADDR_INVALID) {
		return -ENOMEM;
	}

	key = k_spin_lock(&priv->lock);
	musb_ep_configure_fifo(base, cfg, ep_idx, is_in, size_code, addr);
	k_spin_unlock(&priv->lock, key);

	return 0;
}

/* Free FIFO allocation table bits for a range. Caller holds priv->lock. */
static void musb_fifo_free(struct udc_musb_data *priv, uint16_t addr, uint16_t size_bytes)
{
	uint32_t start_idx;
	uint32_t num_units;
	uint32_t i;

	if (addr == MUSB_FIFO_ADDR_INVALID || size_bytes == 0U) {
		return;
	}

	start_idx = addr / MUSB_FIFO_UNIT_SIZE;
	num_units = size_bytes / MUSB_FIFO_UNIT_SIZE;

	for (i = start_idx; i < (start_idx + num_units); i++) {
		priv->fifo_allocation_table[i / MUSB_FIFO_UNITS_PER_WORD] &=
			~BIT(i % MUSB_FIFO_UNITS_PER_WORD);
	}
}

/* Release all FIFO RAM and forget every endpoint FIFO assignment. */
static void musb_fifo_reset(struct udc_musb_data *priv)
{
	uint32_t i;

	for (i = 0U; i < ARRAY_SIZE(priv->fifo_allocation_table); i++) {
		priv->fifo_allocation_table[i] = 0U;
	}

	for (i = 0U; i < MUSB_EP_MAX; i++) {
		priv->fifo_in_addr[i] = MUSB_FIFO_ADDR_INVALID;
		priv->fifo_out_addr[i] = MUSB_FIFO_ADDR_INVALID;
	}
}

/* Free FIFO for an endpoint. EP0 shared block freed on first call only. */
static void musb_fifo_free_for_ep(struct udc_musb_data *priv, uint8_t ep_idx, bool is_in,
				     uint16_t size_bytes)
{
	k_spinlock_key_t key = k_spin_lock(&priv->lock);
	uint16_t addr;

	if (ep_idx == 0U) {
		/* EP0: IN and OUT share one block */
		addr = priv->fifo_in_addr[0];

		if (addr != MUSB_FIFO_ADDR_INVALID) {
			musb_fifo_free(priv, addr, size_bytes);
			priv->fifo_in_addr[0] = MUSB_FIFO_ADDR_INVALID;
			priv->fifo_out_addr[0] = MUSB_FIFO_ADDR_INVALID;
		}
	} else if (is_in) {
		addr = priv->fifo_in_addr[ep_idx];

		if (addr != MUSB_FIFO_ADDR_INVALID) {
			musb_fifo_free(priv, addr, size_bytes);
			priv->fifo_in_addr[ep_idx] = MUSB_FIFO_ADDR_INVALID;
		}
	} else {
		addr = priv->fifo_out_addr[ep_idx];

		if (addr != MUSB_FIFO_ADDR_INVALID) {
			musb_fifo_free(priv, addr, size_bytes);
			priv->fifo_out_addr[ep_idx] = MUSB_FIFO_ADDR_INVALID;
		}
	}

	k_spin_unlock(&priv->lock, key);
}

/* Zero FIFO and interrupt registers for an endpoint. Restores INDEX=0. */
static void musb_ep_deconfigure_fifo(struct usb_musb_reg *const base, uint8_t ep_idx, bool is_in)
{
	musb_write8(&base->index, MUSB_INDEX_SELEP(ep_idx));

	if (ep_idx == 0U) {
		musb_write8(&base->txfifosz, 0U);
		musb_write8(&base->rxfifosz, 0U);
		musb_write16(&base->txfifoadd, 0U);
		musb_write16(&base->rxfifoadd, 0U);
	} else if (is_in) {
		musb_clear16(&base->intrtxe, BIT(ep_idx));
		musb_write8(&base->txfifosz, 0U);
		musb_write16(&base->txfifoadd, 0U);
		musb_write16(&base->txmaxp, 0U);
	} else {
		musb_clear16(&base->intrrxe, BIT(ep_idx));
		musb_write8(&base->rxfifosz, 0U);
		musb_write16(&base->rxfifoadd, 0U);
		musb_write16(&base->rxmaxp, 0U);
	}

	musb_write8(&base->index, MUSB_INDEX_SELEP(0));
}

/* Disable endpoint. */
static int udc_musb_ep_disable(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_musb_data *priv = udc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	bool is_in = USB_EP_DIR_IS_IN(cfg->addr);
	k_spinlock_key_t key;
	uint16_t size_bytes;
	uint8_t size_code;

	LOG_DBG("Disable ep 0x%02x", cfg->addr);

	if (ep_idx >= MUSB_EP_MAX) {
		return -EINVAL;
	}

	/* cfg->mps is unchanged since ep_enable, so size_bytes is identical */
	size_bytes = MAX(udc_mps_ep_size(cfg), MUSB_FIFO_UNIT_SIZE);
	size_code = musb_fifo_size_code(size_bytes);

	if (size_code == MUSB_FIFO_SIZE_CODE_INVALID) {
		return -EINVAL;
	}
	size_bytes = MUSB_FIFO_UNIT_SIZE << size_code;

	key = k_spin_lock(&priv->lock);
	musb_ep_deconfigure_fifo(base, ep_idx, is_in);
	k_spin_unlock(&priv->lock, key);

	musb_fifo_free_for_ep(priv, ep_idx, is_in, size_bytes);

	return 0;
}

/* Set endpoint STALL. */
static int udc_musb_ep_set_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_musb_data *priv = udc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	k_spinlock_key_t key;
	uint8_t ep_idx;

	LOG_DBG("Set halt ep 0x%02x", cfg->addr);

	if (USB_EP_GET_IDX(cfg->addr) == 0U) {
		/* Set SendStall; hardware clears it after sending the handshake. */
		key = k_spin_lock(&priv->lock);
		musb_write8(&base->index, MUSB_INDEX_SELEP(0));
		musb_set8(&base->csr0l, MUSB_CSR0L_SENDSTALL_Msk);
		priv->ep0_state = MUSB_EP0_STATE_IDLE;
		k_spin_unlock(&priv->lock, key);
	} else {
		ep_idx = USB_EP_GET_IDX(cfg->addr);

		key = k_spin_lock(&priv->lock);
		musb_write8(&base->index, MUSB_INDEX_SELEP(ep_idx));
		if (USB_EP_DIR_IS_IN(cfg->addr)) {
			musb_set8(&base->txcsrl, MUSB_TXCSRL_SENDSTALL_Msk);
		} else {
			musb_set8(&base->rxcsrl, MUSB_RXCSRL_SENDSTALL_Msk);
		}
		musb_write8(&base->index, MUSB_INDEX_SELEP(0));
		k_spin_unlock(&priv->lock, key);
		cfg->stat.halted = true;
	}

	return 0;
}

/* Clear endpoint STALL. */
static int udc_musb_ep_clear_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_musb_data *priv = udc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	k_spinlock_key_t key;
	struct net_buf *pending;

	LOG_DBG("Clear halt ep 0x%02x", cfg->addr);

	if (ep_idx == 0U) {
		/* EP0 STALL is self-clearing; the state machine handles recovery */
		return 0;
	}

	key = k_spin_lock(&priv->lock);

	musb_write8(&base->index, MUSB_INDEX_SELEP(ep_idx));
	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		musb_clear8(&base->txcsrl, (MUSB_TXCSRL_SENDSTALL_Msk |
			  MUSB_TXCSRL_SENTSTALL_Msk));
		musb_set8(&base->txcsrl, MUSB_TXCSRL_CLRDATATOG_Msk);
		/* Flush FIFO; DATA0 must be clean after halt clear. */
		musb_set8(&base->txcsrl, MUSB_TXCSRL_FLUSHFIFO_Msk);
		if ((musb_read8(&base->txcsrl) & MUSB_TXCSRL_FIFONOTEMPTY_Msk) !=
		    0U) {
			musb_set8(&base->txcsrl, MUSB_TXCSRL_FLUSHFIFO_Msk);
		}
		musb_clear8(&base->txcsrl, MUSB_TXCSRL_TXPKTRDY_Msk);
	} else {
		musb_clear8(&base->rxcsrl, (MUSB_RXCSRL_SENDSTALL_Msk |
			  MUSB_RXCSRL_SENTSTALL_Msk));
		musb_set8(&base->rxcsrl, MUSB_RXCSRL_CLRDATATOG_Msk);
	}
	musb_write8(&base->index, MUSB_INDEX_SELEP(0));

	k_spin_unlock(&priv->lock, key);

	cfg->stat.halted = false;

	/* Re-arm any pending IN transfer. */
	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		pending = udc_buf_peek(cfg);

		/* Skip empty buffers; bulk_tx_done will dequeue them. */
		if (!udc_ep_is_busy(cfg) && pending != NULL && pending->len > 0) {
			musb_enqueue_epx_in(dev, base, cfg, pending);
		}
	}

	return 0;
}

/* Set USB device address. */
static int udc_musb_set_address(const struct device *dev, const uint8_t addr)
{
	struct usb_musb_reg *const base = musb_get_base(dev);

	LOG_DBG("Set address %u for %p", addr, dev);

	musb_write8(&base->faddr, MUSB_FADDR_FUNCADDR(addr));

	return 0;
}

/* Initiate remote wakeup: assert K-state for 2 ms then release. */
static int udc_musb_host_wakeup(const struct device *dev)
{
	struct usb_musb_reg *const base = musb_get_base(dev);
	uint8_t power_reg;

	LOG_DBG("Remote wakeup from %p", dev);

	if (!udc_is_suspended(dev)) {
		LOG_WRN("Remote wakeup requested but device is not suspended");
		return -EACCES;
	}

	/* Exit suspend and assert K-state. */
	power_reg = musb_read8(&base->power);
	power_reg &= ~MUSB_POWER_SUSPENDMODE_Msk;
	power_reg |= MUSB_POWER_RESUME_Msk;
	musb_write8(&base->power, power_reg);

	k_msleep(MUSB_RESUME_KSTATE_MS);

	/* De-assert resume. */
	power_reg = musb_read8(&base->power);
	power_reg &= ~MUSB_POWER_RESUME_Msk;
	musb_write8(&base->power, power_reg);

	udc_set_suspended(dev, false);
	udc_submit_event(dev, UDC_EVT_RESUME, 0);

	return 0;
}

/* Return the negotiated USB bus speed. */
static enum udc_bus_speed udc_musb_device_speed(const struct device *dev)
{
	struct usb_musb_reg *const base = musb_get_base(dev);

	/* POWER.HSMODE is set by hardware after successful HS negotiation. */
	return (musb_read8(&base->power) & MUSB_POWER_HSMODE_Msk) ? UDC_BUS_SPEED_HS
								      : UDC_BUS_SPEED_FS;
}

/* Attach to bus: enable EP0, unmask interrupts, assert SOFTCONN. */
static int udc_musb_enable(const struct device *dev)
{
	const struct udc_musb_config *config = dev->config;
	struct usb_musb_reg *const base = musb_get_base(dev);
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

	musb_set8(&base->intrusbe, MUSB_INTRUSBE_RESETEN_Msk | MUSB_INTRUSBE_SUSPENDEN_Msk |
		  MUSB_INTRUSBE_RESUMEEN_Msk);
	/* Disable VBUSERR; enable SOFEN only if SOF events are requested. */
	musb_clear8(&base->intrusbe, MUSB_INTRUSBE_VBUSERREN_Msk);
	if (IS_ENABLED(CONFIG_UDC_ENABLE_SOF)) {
		musb_set8(&base->intrusbe, MUSB_INTRUSBE_SOFEN_Msk);
	} else {
		musb_clear8(&base->intrusbe, MUSB_INTRUSBE_SOFEN_Msk);
	}
	musb_set16(&base->intrtxe, MUSB_INTRTXE_EP0TXEN_Msk);

	/* Set HSENABLE only for High-Speed mode; clear it for Full-Speed. */
	power_reg = musb_read8(&base->power);
	if (config->speed_idx == UDC_BUS_SPEED_HS) {
		power_reg |= MUSB_POWER_HSENABLE_Msk;
	} else {
		power_reg &= ~MUSB_POWER_HSENABLE_Msk;
	}
	musb_write8(&base->power, power_reg);

	ret = musb_quirk_post_enable(dev);
	if (ret != 0) {
		LOG_ERR("Quirk post enable failed %d", ret);
		return ret;
	}

	power_reg = musb_read8(&base->power);
	power_reg |= MUSB_POWER_SOFTCONN_Msk;
	musb_write8(&base->power, power_reg);

	return 0;
}

/* Detach from bus: disable EP0 and clear SOFTCONN. */
static int udc_musb_disable(const struct device *dev)
{
	struct usb_musb_reg *const base = musb_get_base(dev);
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

	power_reg = musb_read8(&base->power);
	power_reg &= ~MUSB_POWER_SOFTCONN_Msk;
	musb_write8(&base->power, power_reg);

	return 0;
}

static int udc_musb_init(const struct device *dev)
{
	const struct udc_musb_config *config = dev->config;
	struct usb_musb_reg *const base = musb_get_base(dev);
	struct udc_musb_data *priv = udc_get_private(dev);
	uint8_t rambits;
	uint16_t addr;
	int ret;

	LOG_DBG("Initialize USB device %p at base 0x%lx", dev,
		(unsigned long)config->base);

	priv->setup_q_head = 0U;
	priv->setup_q_tail = 0U;

	/* The core registers are accessible once the vendor init completes. */
	ret = musb_quirk_init(dev);
	if (ret != 0) {
		LOG_ERR("Quirk init failed %d", ret);
		return ret;
	}

	rambits = MUSB_RAMINFO_RAMBITS(musb_read8(&base->raminfo));
	if (rambits > MUSB_RAMINFO_RAMBITS_MAX) {
		LOG_ERR("Unsupported FIFO RAM size, RAMINFO.RAMBITS %u", rambits);
		return -ENOTSUP;
	}

	priv->fifo_units = MUSB_RAMINFO_RAM_SIZE(rambits) / MUSB_FIFO_UNIT_SIZE;
	musb_fifo_reset(priv);
	LOG_DBG("FIFO RAM %lu bytes", MUSB_RAMINFO_RAM_SIZE(rambits));

	addr = musb_fifo_allocate(priv, USB_CONTROL_EP_MPS);
	if (addr == MUSB_FIFO_ADDR_INVALID) {
		LOG_ERR("Failed to reserve EP0 FIFO");
		return -ENOMEM;
	}
	priv->fifo_in_addr[0] = addr;
	priv->fifo_out_addr[0] = addr;

	/* shutdown() disables the IRQ line; re-enable it after controller init. */
	config->irq_enable_func(dev);

	/* Start VBUS polling; first event fires after init() returns. */
	priv->vbus_present = false;
	k_work_reschedule(&priv->vbus_work, K_NO_WAIT);

	return 0;
}

/* Shut down the controller and reset all FIFO state. */
static int udc_musb_shutdown(const struct device *dev)
{
	const struct udc_musb_config *config = dev->config;
	struct usb_musb_reg *const base = musb_get_base(dev);
	struct udc_musb_data *priv = udc_get_private(dev);
	struct k_work_sync sync;
	int ret;

	LOG_DBG("Shutdown device %p", dev);

	/* Stop VBUS polling before touching hardware */
	k_work_cancel_delayable_sync(&priv->vbus_work, &sync);

	/* Silence all interrupt enables so nothing fires during teardown */
	musb_write16(&base->intrtxe, 0U);
	musb_write16(&base->intrrxe, 0U);
	musb_write8(&base->intrusbe, 0U);
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

	ret = musb_quirk_shutdown(dev);
	if (ret != 0) {
		LOG_ERR("Quirk shutdown failed %d", ret);
		return ret;
	}

	musb_fifo_reset(priv);

	return 0;
}

/*
 * Enter a USB 2.0 test mode (sec. 7.1.20 / sec. 9.4.9).
 * dryrun=true validates the selector without touching hardware.
 * dryrun=false programs the hardware after Status-IN completes.
 * Exit requires a power cycle; there is no software exit path.
 */
static int udc_musb_test_mode(const struct device *dev, const uint8_t mode, const bool dryrun)
{
	struct udc_musb_data *priv = udc_get_private(dev);
	struct usb_musb_reg *const base = musb_get_base(dev);
	k_spinlock_key_t key;
	mem_addr_t ep0_fifo;
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
		musb_write8(&base->testmode, MUSB_TESTMODE_TESTJ_Msk);
		break;
	case USB_SFS_TEST_MODE_K:
		musb_write8(&base->testmode, MUSB_TESTMODE_TESTK_Msk);
		break;
	case USB_SFS_TEST_MODE_SE0_NAK:
		musb_write8(&base->testmode, MUSB_TESTMODE_TESTSE0NAK_Msk);
		break;
	case USB_SFS_TEST_MODE_PACKET:
		key = k_spin_lock(&priv->lock);
		musb_write8(&base->index, MUSB_INDEX_SELEP(0));

		/* FIFO must be loaded before asserting TESTPACKET. */
		ep0_fifo = musb_fifo_addr(base, 0);

		for (i = 0U; i < sizeof(test_packet_data); i++) {
			sys_write8(test_packet_data[i], ep0_fifo);
		}

		musb_write8(&base->testmode, MUSB_TESTMODE_TESTPACKET_Msk);
		/* Set TxPktRdy to begin continuous TX; DataEnd must NOT be set. */
		musb_set8(&base->csr0l, MUSB_CSR0L_TXPKTRDY_Msk);
		k_spin_unlock(&priv->lock, key);
		break;
	default:
		/* Unreachable - validated above. */
		return -EINVAL;
	}

	return 0;
}

/* One-time driver setup: register endpoints, start thread, enable IRQ. */
static int udc_musb_driver_preinit(const struct device *dev)
{
	const struct udc_musb_config *config = dev->config;
	struct udc_musb_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	uint16_t mps = MUSB_FS_EP_MPS_MAX;
	int err;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);

	/* Back-pointer used by the VBUS work handler. */
	priv->dev = dev;

	k_work_init_delayable(&priv->vbus_work, musb_vbus_poll_work);

	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;
	data->caps.can_detect_vbus = true;

	/* Hardware auto-completes Status OUT; don't enqueue a buffer for it. */
	data->caps.out_ack = true;

	if (config->speed_idx == UDC_BUS_SPEED_HS) {
		data->caps.hs = true;
		mps = MUSB_HS_EP_MPS_MAX;
	}

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			config->ep_cfg_out[i].caps.control = 1;
			config->ep_cfg_out[i].caps.mps = USB_CONTROL_EP_MPS;
		} else {
			config->ep_cfg_out[i].caps.bulk = 1;
			config->ep_cfg_out[i].caps.interrupt = 1;
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
			config->ep_cfg_in[i].caps.mps = mps;
		}

		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register IN endpoint %d", i);
			return err;
		}
	}

	priv->ep0_state = MUSB_EP0_STATE_IDLE;
	priv->ep0_rx_buf = NULL;
	priv->ep0_ctrl_bytes_received = 0U;

	k_thread_create(&priv->thread_data, config->thread_stk, config->thread_stk_sz,
			musb_thread_handler, (void *)dev, NULL, NULL,
			K_PRIO_COOP(config->thread_priority), K_ESSENTIAL, K_NO_WAIT);
	k_thread_name_set(&priv->thread_data, dev->name);

	config->irq_enable_func(dev);

	LOG_DBG("MUSB UDC initialized: %p (speed: %s, endpoints: %d IN + %d OUT)", dev,
		config->speed_idx == UDC_BUS_SPEED_HS ? "High-Speed" : "Full-Speed",
		config->num_of_eps, config->num_of_eps);

	return 0;
}

static void udc_musb_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_musb_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static const struct udc_api udc_musb_api = {
	.lock = udc_musb_lock,
	.unlock = udc_musb_unlock,
	.device_speed = udc_musb_device_speed,
	.init = udc_musb_init,
	.enable = udc_musb_enable,
	.disable = udc_musb_disable,
	.shutdown = udc_musb_shutdown,
	.set_address = udc_musb_set_address,
	.host_wakeup = udc_musb_host_wakeup,
	.test_mode = udc_musb_test_mode,
	.ep_enable = udc_musb_ep_enable,
	.ep_disable = udc_musb_ep_disable,
	.ep_set_halt = udc_musb_ep_set_halt,
	.ep_clear_halt = udc_musb_ep_clear_halt,
	.ep_enqueue = udc_musb_ep_enqueue,
	.ep_dequeue = udc_musb_ep_dequeue,
};

/*
 * A node with a single register region describes the MUSB core only. A vendor
 * wrapper in front of the core is described by the named regions "wrapper"
 * and "core".
 */
#define UDC_MUSB_DT_INST_CORE_ADDR(n)						\
	COND_CODE_1(DT_NUM_REGS(DT_DRV_INST(n)),				\
		    (DT_INST_REG_ADDR(n)),					\
		    (DT_INST_REG_ADDR_BY_NAME(n, core)))

#define UDC_MUSB_DT_INST_WRAPPER_ADDR(n)					\
	COND_CODE_1(DT_INST_REG_HAS_NAME(n, wrapper),				\
		    (DT_INST_REG_ADDR_BY_NAME(n, wrapper)),			\
		    (0))

#define UDC_MUSB_IRQ_DT_INST_DEFINE(n)						\
	static void udc_musb_irq_enable_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    udc_musb_isr_handler,				\
			    DEVICE_DT_INST_GET(n),				\
			    0);							\
										\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static void udc_musb_irq_disable_func_##n(const struct device *dev)	\
	{									\
		irq_disable(DT_INST_IRQN(n));					\
	}

#define UDC_MUSB_DEVICE_DEFINE(n)						\
	K_THREAD_STACK_DEFINE(udc_musb_stack_##n, CONFIG_UDC_MUSB_STACK_SIZE);	\
										\
	UDC_MUSB_IRQ_DT_INST_DEFINE(n)						\
										\
	static struct udc_ep_config						\
		ep_cfg_out_##n[DT_INST_PROP(n, num_out_endpoints)];		\
	static struct udc_ep_config						\
		ep_cfg_in_##n[DT_INST_PROP(n, num_in_endpoints)];		\
										\
	BUILD_ASSERT(DT_INST_PROP(n, num_in_endpoints) ==			\
		     DT_INST_PROP(n, num_out_endpoints),			\
		     "IN and OUT endpoint counts must match");			\
										\
	static const struct udc_musb_config udc_musb_config_##n = {		\
		.base = UDC_MUSB_DT_INST_CORE_ADDR(n),				\
		.wrapper = UDC_MUSB_DT_INST_WRAPPER_ADDR(n),			\
		.num_of_eps = DT_INST_PROP(n, num_in_endpoints),		\
		.ep_cfg_in = ep_cfg_in_##n,					\
		.ep_cfg_out = ep_cfg_out_##n,					\
		.thread_stk = udc_musb_stack_##n,				\
		.thread_stk_sz = K_THREAD_STACK_SIZEOF(udc_musb_stack_##n),	\
		.thread_priority = CONFIG_UDC_MUSB_THREAD_PRIORITY,		\
		.speed_idx = DT_ENUM_IDX(DT_DRV_INST(n), maximum_speed),	\
		.vbus_poll_ms = CONFIG_UDC_MUSB_VBUS_POLL_PERIOD_MS,		\
		.irq_enable_func = udc_musb_irq_enable_func_##n,		\
		.irq_disable_func = udc_musb_irq_disable_func_##n,		\
		.quirks = UDC_MUSB_VENDOR_QUIRK_GET(n),				\
	};									\
										\
	static struct udc_musb_data udc_priv_##n = {				\
	};									\
										\
	static struct udc_data udc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),		\
		.priv = &udc_priv_##n,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, udc_musb_driver_preinit, NULL,			\
			      &udc_data_##n, &udc_musb_config_##n,		\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &udc_musb_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_MUSB_DEVICE_DEFINE)
