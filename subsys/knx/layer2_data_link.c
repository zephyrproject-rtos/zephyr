/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>
#include "layer2_data_link.h"
#include "layer1_physical.h"
#include "layer3_network.h"
#include "object_device.h"
#include "object_address_table.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(knx_l2, CONFIG_KNX_STACK_LOG_LEVEL);

extern void Ph_Data__txdebug(struct knx_pkt *pkt);

#define NUMBER_BUFFER_TX_HIGH CONFIG_KNX_L2_TX_QUEUE_DEPTH_HIGH
#define NUMBER_BUFFER_TX_LOW  CONFIG_KNX_L2_TX_QUEUE_DEPTH_LOW
#define NUMBER_BUFFER_RX      CONFIG_KNX_L2_RX_QUEUE_DEPTH

/*
 * Maximum number of 2 ms "bus not free" waits before a frame is dropped
 * (50 × 2 ms = 100 ms).  Bounds the -EBUSY path so a transceiver stuck out of
 * NORMAL state cannot spin the TX thread forever.
 */
#define L_TX_MAX_BUSY_WAIT 50

/*
 * TX: two priority-ordered message queues of knx_pkt pointers, drained in
 * strict priority order by the same TX thread.
 *
 *   l_tx_msgq_hi — KNX_PRIORITY_SYSTEM and KNX_PRIORITY_URGENT: ETS device
 *                  programming, alarms, and anything the Transport Layer's
 *                  3 s L_Data.con / T_ACK window depends on.
 *   l_tx_msgq_lo — KNX_PRIORITY_NORMAL and KNX_PRIORITY_LOW: everyday group
 *                  communication.
 *
 * The real KNX priority order is System > Urgent > Normal > Low, which does
 * NOT sort by the raw knx_priority_t enum value (NORMAL=1 is numerically
 * below URGENT=2, even though Urgent is the more important class) — see
 * l_tx_priority_is_high() below. Never route by a numeric threshold on
 * knx_priority_t.
 */
K_MSGQ_DEFINE(l_tx_msgq_hi, sizeof(struct knx_pkt *), NUMBER_BUFFER_TX_HIGH,
	      sizeof(struct knx_pkt *));
K_MSGQ_DEFINE(l_tx_msgq_lo, sizeof(struct knx_pkt *), NUMBER_BUFFER_TX_LOW,
	      sizeof(struct knx_pkt *));

/* Explicit {SYSTEM,URGENT} vs {NORMAL,LOW} split — see block comment above. */
static inline bool l_tx_priority_is_high(knx_priority_t prio)
{
	switch (prio) {
	case KNX_PRIORITY_SYSTEM:
	case KNX_PRIORITY_URGENT:
		return true;
	default:
		return false;
	}
}

static inline struct k_msgq *l_tx_queue_for(knx_priority_t prio)
{
	return l_tx_priority_is_high(prio) ? &l_tx_msgq_hi : &l_tx_msgq_lo;
}

/*
 * l_tx_dequeue_next — non-blocking strict-priority pick between the two TX
 * queues. Tries l_tx_msgq_hi first so a queued System/Urgent frame is always
 * picked ahead of a Normal/Low one, then falls back to l_tx_msgq_lo.
 *
 * Factored out of l_tx_thread_fn() so the strict-priority decision itself
 * can be unit-tested synchronously without the real TX thread, its k_poll()
 * wait, or its retry loop running — mirrors how l_process_rx_frame() is
 * factored out and called directly by the RX-side test harness.
 *
 * Returns 0 and writes *out on success, -ENOMSG if both queues are empty.
 */
static int l_tx_dequeue_next(struct knx_pkt **out)
{
	if (k_msgq_get(&l_tx_msgq_hi, out, K_NO_WAIT) == 0) {
		return 0;
	}
	return k_msgq_get(&l_tx_msgq_lo, out, K_NO_WAIT);
}

#define L_TX_STACK_SIZE CONFIG_KNX_L2_TX_STACK_SIZE
K_THREAD_STACK_DEFINE(l_tx_stack, L_TX_STACK_SIZE);
static struct k_thread l_tx_thread_data;

/*
 * RX path:
 *   l_rx_pkt_msgq — queue of fully-allocated knx_pkt * passed from L1 to
 *                   the RX thread; pkt->buf holds the raw frame, pkt->buf_len
 *                   the raw byte count, until l_process_rx_frame() parses them.
 *   l_rx_frame_sem — given by l_frame_rx() to wake the RX thread.
 *
 * The raw byte accumulator (rx_frame[]/rx_frame_len) lives in layer1_physical.c
 * and is NOT duplicated here.
 */
K_MSGQ_DEFINE(l_rx_pkt_msgq, sizeof(struct knx_pkt *), NUMBER_BUFFER_RX, sizeof(struct knx_pkt *));

static K_SEM_DEFINE(l_reset_sem, 0, 1);
static K_SEM_DEFINE(l_rx_frame_sem, 0, NUMBER_BUFFER_RX);

void l_frame_rx(struct knx_pkt *pkt)
{
	if (k_msgq_put(&l_rx_pkt_msgq, &pkt, K_NO_WAIT) != 0) {
		LOG_ERR("RX: queue full");
		knx_pkt_free(pkt);
		return;
	}

	k_sem_give(&l_rx_frame_sem);
}

void l_reset(void)
{
	k_sem_give(&l_reset_sem);
}

/*
 * Ph_Data__con — per-byte UART completion callback from the driver.
 * With the burst-TX path (knx_l1_send_frame) this is only called for
 * Req_ack_char sends (single-byte U_Ackn.req).  It is retained for
 * compatibility; the TX thread no longer loops on it.
 */
void Ph_Data__con(P_Status p_status)
{
	ARG_UNUSED(p_status);
}

/* -------- L_Data services (upward/downward dispatch) -------- */

void L_Data__req(struct knx_pkt *pkt)
{
	uint8_t lsdu_len = pkt->lsdu_len;

	pkt->src = device_individual_address();

	if (lsdu_len <= 16) {
		/*
		 * Standard frame layout (ANALYZE.MD §22.2):
		 * [CTRL][SA_H][SA_L][DA_H][DA_L][AT|HC|LG][LSDU…]
		 * CTRL format: 1 0 r 1 p1 p0 0 0  (standard, not-repeated)
		 */
		memmove(&pkt->buf[6], pkt->buf, lsdu_len);
		pkt->buf[0] = 0xB0u | ((pkt->priority << 2) & 0x0Cu);
		pkt->buf[1] = (pkt->src >> 8) & 0xFFu;
		pkt->buf[2] = pkt->src & 0xFFu;
		pkt->buf[3] = (pkt->dst >> 8) & 0xFFu;
		pkt->buf[4] = pkt->dst & 0xFFu;
		pkt->buf[5] = ((pkt->addr_type << 7) & 0x80u) | ((pkt->hop_count << 4) & 0x70u) |
			      ((lsdu_len - 1u) & 0x0Fu);
		pkt->buf_len = 6u + lsdu_len;
		/*
		 * Re-point lsdu at the TPCI after the memmove.  Callers set
		 * pkt->lsdu = pkt->buf before calling us; leaving it there
		 * makes lsdu[0] the CTRL byte, and T_Data_Individual__con()
		 * then decodes 0xB0 as a TPCI — every individual .con was
		 * dispatched as T_Connect.con instead of E21..E24.
		 */
		pkt->lsdu = &pkt->buf[6];
	} else if (lsdu_len <= KNX_MAX_LSDU_OCTETS) {
		/*
		 * Extended frame layout (KNX spec §22.3):
		 * [CTRL][CTRLE][SA_H][SA_L][DA_H][DA_L][LG][TPCI][APCI+data…][FCS]
		 * CTRL  = 0 0 r 1 p1 p0 0 0  (extended, not-repeated; FT bit 7 = 0)
		 * CTRLE = [AT][HC][EFF=0000]
		 * LG    = APCI+data byte count = lsdu_len - 1  (TPCI not counted)
		 * total wire bytes = 9 + LG = 8 + lsdu_len
		 */
		memmove(&pkt->buf[7], pkt->buf, lsdu_len);
		pkt->buf[0] = 0x30u | ((pkt->priority << 2) & 0x0Cu);
		pkt->buf[1] = ((pkt->addr_type << 7) & 0x80u) | ((pkt->hop_count << 4) & 0x70u);
		pkt->buf[2] = (pkt->src >> 8) & 0xFFu;
		pkt->buf[3] = pkt->src & 0xFFu;
		pkt->buf[4] = (pkt->dst >> 8) & 0xFFu;
		pkt->buf[5] = pkt->dst & 0xFFu;
		pkt->buf[6] = (uint8_t)(lsdu_len - 1u); /* LG = APCI+data count */
		pkt->buf_len = 7u + lsdu_len;
		pkt->frame_format = KNX_FRAME_FORMAT_EXTENDED;
		pkt->lsdu = &pkt->buf[7]; /* see comment in the standard-frame branch */
	} else {
		/*
		 * Bounded by the APDU length this device publishes in
		 * PID_MAX_APDU_LENGTH, not by the wire format's 254-octet LG limit:
		 * knx_l1_send_frame() cannot address past the NCN5130's first
		 * 64-octet block, so anything longer would be dropped down there
		 * with a far less obvious -ENOTSUP.  See KNX_MAX_APDU_OCTETS.
		 */
		LOG_ERR("%s: LSDU %u octets exceeds the %u this device "
			"advertises (PID_MAX_APDU_LENGTH = %u)",
			__func__, lsdu_len, (unsigned int)KNX_MAX_LSDU_OCTETS,
			(unsigned int)KNX_MAX_APDU_OCTETS);
		knx_pkt_free(pkt);
		return;
	}

	if (k_msgq_put(l_tx_queue_for(pkt->priority), &pkt, K_NO_WAIT) != 0) {
		LOG_ERR("TX queue full, dropping frame");
		pkt->status = KNX_STATUS_NOT_OK;
		L_Data__con(pkt);
	}
}

void L_Data__ind(struct knx_pkt *pkt)
{

#if defined(CONFIG_KNX_DEBUG_VERBOSE_L2) || defined(CONFIG_KNX_DEBUG_VERBOSE_SYSTEM)
#if !defined(CONFIG_KNX_DEBUG_VERBOSE_L2)
	if (pkt->priority == KNX_PRIORITY_SYSTEM) {
#endif
		const char *ft = KNX_FRAME_VAL(pkt->frame_format);
		const char *priority = KNX_PRIORITY_VAL(pkt->priority);

		if (pkt->addr_type == KNX_ADDR_TYPE_INDIVIDUAL) {
			LOG_DBG("L_Data.ind(ack=%d src=%i.%i.%i dst=%i.%i.%i hop=%i %s %s)",
				pkt->ack_request, KNX_ADDR_VAL(pkt->src), KNX_ADDR_VAL(pkt->dst),
				pkt->hop_count, ft, priority);
		} else {
			LOG_DBG("L_Data.ind(ack=%d src=%i.%i.%i dst=%i/%i/%i hop=%i %s %s)",
				pkt->ack_request, KNX_ADDR_VAL(pkt->src), KNX_GROUP_VAL(pkt->dst),
				pkt->hop_count, ft, priority);
		}
#if !defined(CONFIG_KNX_DEBUG_VERBOSE_L2)
	}
#endif
#endif

	/*
	 * ACK is NOT sent here in software.  U_SetAddress.req was issued at
	 * boot (Ph_Reset__con → layer1_physical.c) so the NCN5130 sends IACK
	 * automatically for frames matching our Individual Address.  Sending an
	 * additional U_Ackn.req from this thread would arrive far past the
	 * 15 bit-time ACK window (ANALYZE.MD §2.4 / §2.5).
	 */

	/*
	 * IA duplication check (KNX spec §14.9, System B rule):
	 * If another device on the bus sends a frame with our own Individual
	 * Address as source, two devices share the same IA — set
	 * PID_DEVICE_CONTROL bit 1 to signal the conflict.
	 * Skip the check for 0xFFFF (unregistered default) to avoid false
	 * positives when multiple freshly-flashed devices are on the bus.
	 */
	{
		knx_addr_t own_ia = device_individual_address();

		if (pkt->src == own_ia && own_ia != 0xFFFFu) {
			device_set_ia_duplication();
		}
	}

	if (pkt->addr_type == KNX_ADDR_TYPE_INDIVIDUAL && pkt->dst != 0) {
		if (pkt->dst == device_individual_address()) {
			pkt->type = KNX_PKT_DATA_INDIVIDUAL;
			N_Data_Individual__ind(pkt);
			return;
		}
	} else {
		if (pkt->dst != 0) {
			/*
			 * DLL acceptance per KNX spec §4.16.8.2.4: use
			 * address_table_contains() (accept-all when the table
			 * is empty), not get_tsap() — TSAP resolution is the
			 * Network Layer's job (N_Data_Group__ind), and it may
			 * legitimately fail to find a route even when the DLL
			 * has correctly accepted the frame.
			 */
			if (address_table_contains(pkt->dst)) {
				pkt->type = KNX_PKT_DATA_GROUP;
				N_Data_Group__ind(pkt);
				return;
			}
		} else {
			pkt->type = KNX_PKT_DATA_BROADCAST;
			N_Data_Broadcast__ind(pkt);
			return;
		}
	}

	/* Frame not addressed to this device — discard */
	knx_pkt_free(pkt);
}

void L_Data__con(struct knx_pkt *pkt)
{
	switch (pkt->type) {
	case KNX_PKT_DATA_INDIVIDUAL:
		N_Data_Individual__con(pkt);
		break;
	case KNX_PKT_DATA_GROUP:
		N_Data_Group__con(pkt);
		break;
	case KNX_PKT_DATA_BROADCAST:
		N_Data_Broadcast__con(pkt);
		break;
	case KNX_PKT_DATA_SYSTEM_BROADCAST:
		N_Data_SystemBroadcast__con(pkt);
		break;
	default:
		knx_pkt_free(pkt);
		break;
	}
}

void L_SystemBroadcast__req(struct knx_pkt *pkt)
{
	/* On TP1, system broadcast is sent as a normal broadcast (DA=0, AT=group).
	 * The NCN5130 does not differentiate SBC at the physical level.
	 */
	pkt->addr_type = KNX_ADDR_TYPE_GROUP;
	pkt->dst = 0x0000u;
	L_Data__req(pkt);
}

void L_SystemBroadcast__con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

/*
 * L_SystemBroadcast__ind — deleted. It was dead code: on a
 * TP1 standard frame, broadcast and system broadcast are indistinguishable,
 * so L_Data__ind() always classifies dst=0 as KNX_PKT_DATA_BROADCAST and
 * nothing ever routed a frame here. Implementing the alternative (parsing
 * the SB flag from CTRLE on extended frames) was verified against Volume 6
 * §4.2's System B feature checklist (06 Profiles v02.01.01.pdf, p.37) not
 * to be worth it: System Broadcast / A_SystemNetworkParameter_* is not a
 * mandatory or optional row there for mask 07B0h at all.
 */

/* -------- TX thread -------- */

/*
 * L2 TX thread — burst-TX path (Phase 1.5.1).
 *
 * Dequeues knx_pkt pointers, appends the FCS, and sends the entire KNX
 * wire frame as a single DMA burst via knx_l1_send_frame().
 * The function blocks until the NCN5130 has confirmed the frame on the bus
 * (L_Data.con 0x0B/0x8B), so pkt->status reflects the real bus result.
 *
 * On EBUSY the thread waits 2 ms and retries (KNX BUSY retry behaviour).
 * On EMSGSIZE (extended frame > 24 bytes) the frame is dropped with an
 * error log until Phase 3 extended-frame TX is implemented.
 */
static void l_tx_thread_fn(void *p1, void *p2, void *p3)
{
	struct knx_pkt *pkt;
	struct k_poll_event tx_poll_events[2];

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		/*
		 * Strict priority: l_tx_dequeue_next() always tries the high
		 * queue first. When both are empty, block on k_poll() over
		 * both — the only place this thread sleeps, so it never
		 * busy-polls.
		 *
		 * No fairness/anti-starvation mechanism between the two
		 * queues: strict priority is intentional, not
		 * fairness, and KNX System/Urgent traffic (ETS programming,
		 * alarms) is inherently rare and short-lived — not a
		 * sustained stream that could starve group traffic in
		 * practice.
		 *
		 * No preemption mid-send: once a frame is popped, the retry
		 * loop below always runs to completion even if a
		 * higher-priority frame is enqueued meanwhile. With the
		 * shortened L_Data.con timeout (CONFIG_KNX_L_DATA_CON_TIMEOUT_MS),
		 * the worst case a queued high-priority frame
		 * waits behind one in-flight low-priority send is a few
		 * hundred ms — comfortably inside the L4 3 s ACK budget — so
		 * tearing down mid-retry for a small extra benefit isn't
		 * worth the added complexity.
		 */
		if (l_tx_dequeue_next(&pkt) != 0) {
			k_poll_event_init(&tx_poll_events[0], K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					  K_POLL_MODE_NOTIFY_ONLY, &l_tx_msgq_hi);
			k_poll_event_init(&tx_poll_events[1], K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					  K_POLL_MODE_NOTIFY_ONLY, &l_tx_msgq_lo);
			k_poll(tx_poll_events, ARRAY_SIZE(tx_poll_events), K_FOREVER);
			continue;
		}

		/* pkt->buf[0..buf_len-1] is the complete frame without FCS.
		 *
		 * KNX FCS = NOT(XOR of all frame bytes from CTRL through last TPDU
		 * byte), including CTRLE for extended frames (KNX spec 3/2/2 §3.3).
		 * The NCN5130 validates this exact formula before transmitting, and
		 * forwards the host-provided FCS byte onto the KNX bus unchanged.
		 */
		uint16_t raw_len = pkt->buf_len;
		uint8_t crc = 0;

		for (uint16_t i = 0; i < raw_len; i++) {
			crc ^= pkt->buf[i];
		}
		pkt->buf[raw_len] = ~crc;
		pkt->buf_len += 1;

		Ph_Data__txdebug(pkt);

		uint16_t wire_len = raw_len + 1u;

		int ret;
		int retry_count = 0;
		int busy_count = 0;
		bool repeated_bit_set = false;

		/* KNX CSMA/CA retry:
		 *  -EBUSY        bus not free before TX started → wait 2 ms
		 *  -EIO          NCN5130 reported a genuine tx_err/rx_err via
		 *                U_State.ind (real collision — lost bus
		 *                arbitration against another device) → retry
		 *  -ECONNREFUSED negative L_Data.con: frame went out cleanly but
		 *                no device on the bus ACKed it (not a collision —
		 *                e.g. no device at that address) → retry
		 *  -ETIMEDOUT    NCN5130 sent no L_Data.con within
		 *                CONFIG_KNX_L_DATA_CON_TIMEOUT_MS → retry
		 *
		 * Max KNX retries per spec: nak_retry=3 + busy_retry=3 = 6 total.
		 * We use the same limit for all three retryable causes above.
		 *
		 * -EBUSY is bounded separately by L_TX_MAX_BUSY_WAIT.  It has two
		 * distinct causes inside knx_l1_send_frame(): the bus is genuinely
		 * occupied (transient, clears in a few ms), or the NCN5130 is not in
		 * NORMAL state (permanent — dead transceiver, wrong strapping,
		 * brownout).  Without a bound the second case spins this thread
		 * forever at 2 ms intervals and every queued frame starves behind it.
		 */
		for (;;) {
			ret = knx_l1_send_frame(pkt->buf, wire_len);

			if (ret == -EBUSY) {
				if (++busy_count > L_TX_MAX_BUSY_WAIT) {
					LOG_ERR("TX: bus not free after %d attempts "
						"(transceiver down?), dropping frame",
						busy_count);
					break;
				}
				k_msleep(2);
				continue;
			}

			if ((ret == -EIO || ret == -ECONNREFUSED || ret == -ETIMEDOUT) &&
			    retry_count < 6) {
				retry_count++;
				if (!repeated_bit_set) {
					/*
					 * Mark the CTRL "repeated" bit (r=0) so a
					 * receiver that got the first attempt fine but
					 * whose ACK was lost (-ECONNREFUSED/-ETIMEDOUT)
					 * — or that partially saw it before a collision
					 * (-EIO) — can recognize this retransmission as
					 * a duplicate and discard it without re-delivering
					 * it upward. FCS must be recomputed: flipping one
					 * input bit flips the same bit in
					 * ~XOR(all bytes), since NOT distributes over
					 * XOR with a fixed mask.
					 */
					pkt->buf[0] &= ~0x20u;
					pkt->buf[raw_len] ^= 0x20u;
					repeated_bit_set = true;
				}
				/* Backoff: 2 + retry × 2 ms (4, 6, 8, 10, 12, 14 ms) */
				k_msleep(2 + retry_count * 2);
				if (ret == -EIO) {
					LOG_DBG("TX: collision, retry %d", retry_count);
				} else if (ret == -ECONNREFUSED) {
					LOG_DBG("TX: no ACK, retry %d", retry_count);
				} else {
					LOG_DBG("TX: timeout, retry %d", retry_count);
				}
				continue;
			}

			/* 0, -EMSGSIZE, -ENOTSUP, or retries exhausted: done. */
			break;
		}

		if (ret == -EMSGSIZE) {
			LOG_ERR("TX: frame length %u out of range, dropping", wire_len);
		} else if (ret == -ENOTSUP) {
			LOG_ERR("TX: extended frame of %u bytes exceeds the driver's "
				"64-byte block limit, dropping",
				wire_len);
		}
		pkt->status = (ret == 0) ? KNX_STATUS_OK : KNX_STATUS_NOT_OK;

		L_Data__con(pkt);
	}
}

/* -------- App work-queue handler — dispatches parsed frames to L3..L7 ---- */

/*
 * l_rx_dispatch_work — executed on knx_app_wq, not on the L2 RX thread.
 * By the time this runs, pkt->buf/lsdu/metadata are fully populated by
 * l_process_rx_frame.  Ownership transfers to L_Data__ind (and then up
 * through L3..L7); whoever takes final ownership must call knx_pkt_free.
 */
static void l_rx_dispatch_work(struct k_work *work)
{
	struct knx_pkt *pkt = CONTAINER_OF(work, struct knx_pkt, work);

	L_Data__ind(pkt);
}

/* -------- RX frame parser (L2 RX thread context) -------- */

/*
 * Identity of the last correctly received frame, for the repetition filter
 * (KNX spec 3/2/2 §2.4.2).  `fcs` is normalised as if the frame were not
 * marked repeated — see l_process_rx_frame() for why that matters.
 *
 * Only ever touched from the L2 RX thread, so it needs no locking.
 */
static struct {
	bool valid;
	knx_addr_t src;
	knx_addr_t dst;
	uint8_t lsdu_len;
	uint8_t fcs;
} s_last_rx;

/*
 * Takes one knx_pkt from l_rx_pkt_msgq.  pkt->buf holds the raw KNX wire
 * frame (header + LSDU + FCS) and pkt->buf_len the byte count.
 *
 * 1. Length sanity check (trim trailing noise, discard truncated).
 * 2. Verify FCS.
 * 3. Decode header fields into pkt metadata.
 * 4. Submit pkt->work to knx_app_wq for L3..L7 dispatch.
 */
static void l_process_rx_frame(void)
{
	struct knx_pkt *pkt;
	static uint32_t nbframe;
	static uint32_t nberror;

	if (k_msgq_get(&l_rx_pkt_msgq, &pkt, K_NO_WAIT) != 0) {
		return;
	}
	uint8_t *raw = pkt->buf;
	uint16_t raw_len = pkt->buf_len;
	++nbframe;

	/*
	 * Frame length sanity check.
	 *
	 * For BOTH frame types, LG = APCI+data byte count (TPCI is not counted).
	 * lsdu_len = LG + 1  (TPCI + APCI + data).
	 *
	 * Standard: [CTRL SA_H SA_L DA_H DA_L AT|HC|LG TPCI APCI+data(LG) FCS]
	 *           total = 6 + 1 + LG + 1 = 8 + LG   (max LG=15, 4-bit field)
	 * Extended: [CTRL CTRLE SA_H SA_L DA_H DA_L LG TPCI APCI+data(LG) FCS]
	 *           total = 7 + 1 + LG + 1 = 9 + LG   (max LG=254)
	 *
	 * A trailing noise byte (e.g. the NCN5130 L_Data.con 0x0B arriving
	 * before the EOF timer fires) makes raw_len one too large.
	 */
	/*
	 * Minimum frame length (KNX spec 3/2/2 §2.4.2): a frame is only "correct"
	 * if it is 8..23 characters for a standard frame or 9..263 for an extended
	 * one, check octet included.  Anything shorter cannot carry a header, so
	 * reject it before the FCS check — otherwise a runt reports a misleading
	 * "CRC error" and pollutes the error ratio.
	 */
	{
		bool std_hdr = (raw[0] & 0x80u) != 0;
		uint16_t min_len = std_hdr ? 8u : 9u;

		if (raw_len < min_len) {
			LOG_WRN("RX: runt %s frame (%u octets, minimum %u) — discarding",
				std_hdr ? "standard" : "extended", raw_len, min_len);
			knx_pkt_free(pkt);
			return;
		}
	}

	if (raw_len >= 7) {
		bool std_frame_hdr = (raw[0] & 0x80u) != 0;
		uint16_t expected_len = std_frame_hdr
						? (uint16_t)(8u + (raw[5] & 0x0Fu))
						: (raw_len >= 8 ? (uint16_t)(9u + raw[6]) : 0u);

		if (expected_len > 0 && raw_len != expected_len) {
			if (raw_len > expected_len) {
				/* Trailing noise byte(s): trim silently */
				LOG_DBG("RX: trim %u trailing byte(s) (got %u expected %u)",
					raw_len - expected_len, raw_len, expected_len);
				raw_len = expected_len;
				pkt->buf_len = expected_len;
			} else {
				/* Frame truncated: not recoverable, discard */
				LOG_WRN("RX: frame too short (got %u expected %u) — discarding",
					raw_len, expected_len);
				knx_pkt_free(pkt);
				return;
			}
		}
	}

	/* CRC: XOR of all bytes except CRC itself, then bitwise-NOT */
	uint8_t crc = 0;

	for (int i = 0; i < raw_len - 1; i++) {
		crc ^= raw[i];
	}
	crc = ~crc;

	if (crc != raw[raw_len - 1]) {
		++nberror;
		LOG_WRN("RX: CRC error (got 0x%02x expected 0x%02x) [%i%%]", raw[raw_len - 1], crc,
			(nberror * 100) / nbframe);
		knx_pkt_free(pkt);

		return;
	}

	bool std_frame = (raw[0] & 0x80u);

	pkt->frame_format = std_frame ? KNX_FRAME_FORMAT_STANDARD : KNX_FRAME_FORMAT_EXTENDED;
	pkt->addr_type = std_frame ? ((raw[5] & 0x80u) >> 7) : ((raw[1] & 0x80u) >> 7);
	pkt->src =
		std_frame ? (((uint16_t)raw[1] << 8) | raw[2]) : (((uint16_t)raw[2] << 8) | raw[3]);
	pkt->dst =
		std_frame ? (((uint16_t)raw[3] << 8) | raw[4]) : (((uint16_t)raw[4] << 8) | raw[5]);
	pkt->priority = (raw[0] & 0x0Cu) >> 2;
	/*
	 * Standard frame: hop count in AT|HC|LG byte (raw[5]) bits[6:4].
	 * Extended frame: hop count in CTRLE byte (raw[1]) bits[6:4].
	 *                 raw[6] is the LG field — it does NOT carry hop count.
	 *
	 * Standard frame: LG = raw[5] bits[3:0] = APCI+data count.
	 *                 hop_count = raw[5] bits[6:4].
	 * Extended frame: LG = raw[6] = APCI+data count (same semantics).
	 *                 hop_count = raw[1] bits[6:4] (CTRLE byte).
	 * Both: lsdu_len = LG + 1  (adds the TPCI byte not counted in LG).
	 */
	pkt->hop_count = std_frame ? ((raw[5] >> 4) & 0x07u) : ((raw[1] >> 4) & 0x07u);
	pkt->lsdu_len = std_frame ? ((raw[5] & 0x0Fu) + 1u) : (raw[6] + 1u);
	pkt->ack_request = 1;
	/*
	 * Repetition service parameter.  CTRL bit 5 has INVERTED polarity
	 * (KNX spec 3/2/2 §2.2.2): r = 0 means repeated, r = 1 means not
	 * repeated.  Normalise it here so nothing downstream has to remember.
	 */
	pkt->repeated = ((raw[0] & 0x20u) == 0u) ? 1u : 0u;
	pkt->status = KNX_STATUS_OK;

	/*
	 * RX repetition filter — KNX spec 3/2/2 §2.4.2 (p.37-41): pass
	 * L_Data.ind up only if the frame is "not a repetition of the directly
	 * preceding correctly received frame".
	 *
	 * Without it a retransmitted A_GroupValue_Write is applied twice — a
	 * relay toggled twice, a counter double-incremented.  The L4 sequence
	 * numbers absorb the connection-oriented case, but group and broadcast
	 * traffic has no such protection.
	 *
	 * Two things make this correct, and both are easy to get wrong:
	 *
	 * 1. The FCS DIFFERS between an original and its repetition, because
	 *    clearing CTRL bit 5 flips bit 5 of ~XOR(all bytes).  Comparing raw
	 *    FCS values would therefore never match.  The stored digest is
	 *    normalised to "as if not repeated" by XORing 0x20 back in.
	 *
	 * 2. Dropping the frame here does NOT suppress the bus acknowledgment.
	 *    The NCN5130 sends IACK autonomously once auto-acknowledge is armed
	 *    by U_SetAddress.req at boot (see layer1_physical.c), long before
	 *    the frame reaches this function.  If the ACK were ever moved into
	 *    software it would have to be sent BEFORE this filter, otherwise the
	 *    sender would never learn its repetition arrived and would keep
	 *    repeating.
	 *
	 * The comparison runs after the FCS check so only correctly received
	 * frames become the reference, exactly as the spec words it.  A frame
	 * that is byte-identical but marked "not repeated" is legitimate traffic
	 * (a sensor resending the same value) and is never dropped.
	 */
	{
		uint8_t norm_fcs = raw[raw_len - 1] ^ (pkt->repeated ? 0x20u : 0x00u);

		if (pkt->repeated && s_last_rx.valid && s_last_rx.src == pkt->src &&
		    s_last_rx.dst == pkt->dst && s_last_rx.lsdu_len == pkt->lsdu_len &&
		    s_last_rx.fcs == norm_fcs) {
			LOG_DBG("RX: dropping repetition of the preceding frame "
				"(src=0x%04x dst=0x%04x len=%u)",
				pkt->src, pkt->dst, pkt->lsdu_len);
			knx_pkt_free(pkt);
			return;
		}

		s_last_rx.valid = true;
		s_last_rx.src = pkt->src;
		s_last_rx.dst = pkt->dst;
		s_last_rx.lsdu_len = pkt->lsdu_len;
		s_last_rx.fcs = norm_fcs;
	}

	/* LSDU pointer — points into pkt->buf, not moved */
	uint8_t lsdu_offset = std_frame ? 6u : 7u;

	pkt->lsdu = &pkt->buf[lsdu_offset];

	/* Initialise and submit to the app work-queue (Phase 1.5.2).
	 * L3..L7 processing runs on knx_app_wq, not on the L2 RX thread.
	 * k_work_init is safe to call here because the pkt was just dequeued
	 * from l_rx_pkt_msgq and is not in any other work queue.
	 */
	k_work_init(&pkt->work, l_rx_dispatch_work);
	k_work_submit_to_queue(&knx_app_wq, &pkt->work);
}

/* -------- RX thread -------- */

#define KNX_RX_STACK_SIZE CONFIG_KNX_L2_RX_STACK_SIZE
K_THREAD_STACK_DEFINE(knx_rx_stack, KNX_RX_STACK_SIZE);
static struct k_thread knx_rx_thread_data;

static void knx_rx_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Wait for the NCN5130 to send U_Reset.ind (0x03 → Ph_Reset__con →
	 * l_reset_sem given).  Retry up to 3 times with a 2 s timeout each.
	 */
	int retries = 5;

	while (retries-- > 0) {
		Ph_Reset__req();
		int ret = k_sem_take(&l_reset_sem, K_SECONDS(2));

		if (ret == 0) {
			LOG_DBG("L2: entered normal mode");
			goto running;
		}
		LOG_WRN("L2: reset timeout (attempt %d/%d)", 5 - retries, 5);
	}

	/* All retries exhausted — set safe-state and halt this thread. */
	LOG_ERR("L2: NCN5130 did not respond to reset — halting RX thread");
	/* TODO: set PID_DEVICE_CONTROL bit 3 (safe state) */
	return;

running:
	while (1) {
		k_sem_take(&l_rx_frame_sem, K_FOREVER);
		l_process_rx_frame();
	}
}

/* -------- Initialisation -------- */

void L_Init(void)
{
	k_thread_create(&l_tx_thread_data, l_tx_stack, L_TX_STACK_SIZE, l_tx_thread_fn, NULL, NULL,
			NULL, K_PRIO_PREEMPT(CONFIG_KNX_L2_TX_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(&l_tx_thread_data, "knx_l2_tx");

	k_thread_create(&knx_rx_thread_data, knx_rx_stack, KNX_RX_STACK_SIZE, knx_rx_thread_fn,
			NULL, NULL, NULL, K_PRIO_PREEMPT(CONFIG_KNX_L2_RX_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(&knx_rx_thread_data, "knx_l2_rx");
}
