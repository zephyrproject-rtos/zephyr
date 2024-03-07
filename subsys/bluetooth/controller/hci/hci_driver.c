/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/sys/util.h>
#include <zephyr/debug/stack.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>

#ifdef CONFIG_CLOCK_CONTROL_NRF
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#endif

#include "hal/debug.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "hal/ccm.h"

#if defined(CONFIG_SOC_FAMILY_NORDIC_NRF)
#include "hal/radio.h"
#endif /* CONFIG_SOC_FAMILY_NORDIC_NRF */

#include "ll_sw/pdu_df.h"
#include "lll/pdu_vendor.h"
#include "ll_sw/pdu.h"

#include "ll_sw/lll.h"
#include "lll/lll_df_types.h"
#include "ll_sw/lll_sync_iso.h"
#include "ll_sw/lll_conn.h"
#include "ll_sw/lll_conn_iso.h"
#include "ll_sw/isoal.h"

#include "ll_sw/ull_iso_types.h"
#include "ll_sw/ull_conn_iso_types.h"

#include "ll_sw/ull_iso_internal.h"
#include "ll_sw/ull_sync_iso_internal.h"
#include "ll_sw/ull_conn_internal.h"
#include "ll_sw/ull_conn_iso_internal.h"

#include "ll.h"

#include "hci_internal.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_ctlr_hci_driver);

static struct k_sem sem_prio_recv;
static struct k_fifo recv_fifo;

struct k_thread prio_recv_thread_data;
static K_KERNEL_STACK_DEFINE(prio_recv_thread_stack,
			     CONFIG_BT_CTLR_RX_PRIO_STACK_SIZE);
struct k_thread recv_thread_data;
static K_KERNEL_STACK_DEFINE(recv_thread_stack, CONFIG_BT_RX_STACK_SIZE);

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
static struct k_poll_signal hbuf_signal;
static sys_slist_t hbuf_pend;
static int32_t hbuf_count;
#endif

#if !defined(CONFIG_BT_RECV_BLOCKING)
/* Copied here from `hci_raw.c`, which would be used in
 * conjunction with this driver when serializing HCI over wire.
 * This serves as a converter from the more complicated
 * `CONFIG_BT_RECV_BLOCKING` API to the normal single-receiver
 * `bt_recv` API.
 */
int bt_recv_prio(struct net_buf *buf)
{
	if (bt_buf_get_type(buf) == BT_BUF_EVT) {
		struct bt_hci_evt_hdr *hdr = (void *)buf->data;
		uint8_t evt_flags = bt_hci_evt_get_flags(hdr->evt);

		if ((evt_flags & BT_HCI_EVT_FLAG_RECV_PRIO) &&
		    (evt_flags & BT_HCI_EVT_FLAG_RECV)) {
			/* Avoid queuing the event twice */
			return 0;
		}
	}

	return bt_recv(buf);
}
#endif /* CONFIG_BT_RECV_BLOCKING */

#if defined(CONFIG_BT_CTLR_ISO)

#define SDU_HCI_HDR_SIZE (BT_HCI_ISO_HDR_SIZE + BT_HCI_ISO_TS_DATA_HDR_SIZE)

isoal_status_t sink_sdu_alloc_hci(const struct isoal_sink    *sink_ctx,
				  const struct isoal_pdu_rx  *valid_pdu,
				  struct isoal_sdu_buffer    *sdu_buffer)
{
	ARG_UNUSED(sink_ctx);
	ARG_UNUSED(valid_pdu); /* TODO copy valid pdu into netbuf ? */

	struct net_buf *buf  = bt_buf_get_rx(BT_BUF_ISO_IN, K_FOREVER);

	if (buf) {
		/* Reserve space for headers */
		net_buf_reserve(buf, SDU_HCI_HDR_SIZE);

		sdu_buffer->dbuf = buf;
		sdu_buffer->size = net_buf_tailroom(buf);
	} else {
		LL_ASSERT(0);
	}

	return ISOAL_STATUS_OK;
}


isoal_status_t sink_sdu_emit_hci(const struct isoal_sink             *sink_ctx,
				 const struct isoal_emitted_sdu_frag *sdu_frag,
				 const struct isoal_emitted_sdu      *sdu)
{
	struct bt_hci_iso_ts_data_hdr *data_hdr;
	uint16_t packet_status_flag;
	struct bt_hci_iso_hdr *hdr;
	uint16_t handle_packed;
	uint16_t slen_packed;
	struct net_buf *buf;
	uint16_t total_len;
	uint16_t handle;
	uint8_t  ts, pb;
	uint16_t len;

	buf = (struct net_buf *) sdu_frag->sdu.contents.dbuf;


	if (buf) {
#if defined(CONFIG_BT_CTLR_CONN_ISO_HCI_DATAPATH_SKIP_INVALID_DATA)
		if (sdu_frag->sdu.status != ISOAL_SDU_STATUS_VALID) {
			/* unref buffer if invalid fragment */
			net_buf_unref(buf);

			return ISOAL_STATUS_OK;
		}
#endif /* CONFIG_BT_CTLR_CONN_ISO_HCI_DATAPATH_SKIP_INVALID_DATA */

		pb  = sdu_frag->sdu_state;
		len = sdu_frag->sdu_frag_size;
		total_len = sdu->total_sdu_size;
		packet_status_flag = sdu->collated_status;

		/* BT Core V5.3 : Vol 4 HCI I/F : Part G HCI Func. Spec.:
		 * 5.4.5 HCI ISO Data packets
		 * If Packet_Status_Flag equals 0b10 then PB_Flag shall equal 0b10.
		 * When Packet_Status_Flag is set to 0b10 in packets from the Controller to the
		 * Host, there is no data and ISO_SDU_Length shall be set to zero.
		 */
		if (packet_status_flag == ISOAL_SDU_STATUS_LOST_DATA) {
			if (len > 0 && buf->len >= len) {
				/* Discard data */
				net_buf_pull_mem(buf, len);
			}
			len = 0;
			total_len = 0;
		}

		/*
		 * BLUETOOTH CORE SPECIFICATION Version 5.3 | Vol 4, Part E
		 * 5.4.5 HCI ISO Data packets
		 *
		 * PB_Flag:
		 *  Value   Parameter Description
		 *  0b00    The ISO_Data_Load field contains a header and the first fragment
		 *          of a fragmented SDU.
		 *  0b01    The ISO_Data_Load field contains a continuation fragment of an SDU.
		 *  0b10    The ISO_Data_Load field contains a header and a complete SDU.
		 *  0b11    The ISO_Data_Load field contains the last fragment of an SDU.
		 *
		 * The TS_Flag bit shall be set if the ISO_Data_Load field contains a
		 * Time_Stamp field. This bit shall only be set if the PB_Flag field equals 0b00 or
		 * 0b10.
		 */
		ts = (pb & 0x1) == 0x0;

		if (ts) {
			data_hdr = net_buf_push(buf, BT_HCI_ISO_TS_DATA_HDR_SIZE);
			slen_packed = bt_iso_pkt_len_pack(total_len, packet_status_flag);

			data_hdr->ts = sys_cpu_to_le32((uint32_t) sdu_frag->sdu.timestamp);
			data_hdr->data.sn   = sys_cpu_to_le16((uint16_t) sdu_frag->sdu.sn);
			data_hdr->data.slen = sys_cpu_to_le16(slen_packed);

			len += BT_HCI_ISO_TS_DATA_HDR_SIZE;
		}

		hdr = net_buf_push(buf, BT_HCI_ISO_HDR_SIZE);

		handle = sink_ctx->session.handle;
		handle_packed = bt_iso_handle_pack(handle, pb, ts);

		hdr->handle = sys_cpu_to_le16(handle_packed);
		hdr->len = sys_cpu_to_le16(len);

		/* send fragment up the chain */
		bt_recv(buf);
	}

	return ISOAL_STATUS_OK;
}

isoal_status_t sink_sdu_write_hci(void *dbuf,
				  const uint8_t *pdu_payload,
				  const size_t consume_len)
{
	struct net_buf *buf = (struct net_buf *) dbuf;

	LL_ASSERT(buf);
	net_buf_add_mem(buf, pdu_payload, consume_len);

	return ISOAL_STATUS_OK;
}
#endif

void hci_recv_fifo_reset(void)
{
	/* NOTE: As there is no equivalent API to wake up a waiting thread and
	 * reinitialize the queue so it is empty, we use the cancel wait and
	 * initialize the queue. As the Tx thread and Rx thread are co-operative
	 * we should be relatively safe doing the below.
	 * Added k_sched_lock and k_sched_unlock, as native_posix seems to
	 * swap to waiting thread on call to k_fifo_cancel_wait!.
	 */
	k_sched_lock();
	k_fifo_cancel_wait(&recv_fifo);
	k_fifo_init(&recv_fifo);
	k_sched_unlock();
}

static struct net_buf *process_prio_evt(struct node_rx_pdu *node_rx,
					uint8_t *evt_flags)
{
#if defined(CONFIG_BT_CONN)
	if (node_rx->hdr.user_meta == HCI_CLASS_EVT_CONNECTION) {
		uint16_t handle;
		struct pdu_data *pdu_data = (void *)node_rx->pdu;

		handle = node_rx->hdr.handle;
		if (node_rx->hdr.type == NODE_RX_TYPE_TERMINATE) {
			struct net_buf *buf;

			buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false,
					     K_FOREVER);
			hci_disconn_complete_encode(pdu_data, handle, buf);
			hci_disconn_complete_process(handle);
			*evt_flags = BT_HCI_EVT_FLAG_RECV_PRIO | BT_HCI_EVT_FLAG_RECV;
			return buf;
		}
	}
#endif /* CONFIG_BT_CONN */

	*evt_flags = BT_HCI_EVT_FLAG_RECV;
	return NULL;
}

/**
 * @brief Handover from Controller thread to Host thread
 * @details Execution context: Controller thread
 *   Pull from memq_ll_rx and push up to Host thread recv_thread() via recv_fifo
 * @param p1  Unused. Required to conform with Zephyr thread prototype
 * @param p2  Unused. Required to conform with Zephyr thread prototype
 * @param p3  Unused. Required to conform with Zephyr thread prototype
 */
static void prio_recv_thread(void *p1, void *p2, void *p3)
{
	while (1) {
		struct node_rx_pdu *node_rx;
		struct net_buf *buf;
		bool iso_received;
		uint8_t num_cmplt;
		uint16_t handle;

		iso_received = false;

#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
		node_rx = ll_iso_rx_get();
		if (node_rx) {
			ll_iso_rx_dequeue();

			/* Find out and store the class for this node */
			node_rx->hdr.user_meta = hci_get_class(node_rx);

			/* Send the rx node up to Host thread,
			 * recv_thread()
			 */
			LOG_DBG("ISO RX node enqueue");
			k_fifo_put(&recv_fifo, node_rx);

			iso_received = true;
		}
#endif /* CONFIG_BT_CTLR_SYNC_ISO || CONFIG_BT_CTLR_CONN_ISO */

		/* While there are completed rx nodes */
		while ((num_cmplt = ll_rx_get((void *)&node_rx, &handle))) {
#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_CTLR_ADV_ISO) || \
	defined(CONFIG_BT_CTLR_CONN_ISO)

			buf = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS,
					     false, K_FOREVER);
			hci_num_cmplt_encode(buf, handle, num_cmplt);
			LOG_DBG("Num Complete: 0x%04x:%u", handle, num_cmplt);
			bt_recv_prio(buf);
			k_yield();
#endif /* CONFIG_BT_CONN || CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */
		}

		if (node_rx) {
			uint8_t evt_flags;

			/* Until now we've only peeked, now we really do
			 * the handover
			 */
			ll_rx_dequeue();

			/* Find out and store the class for this node */
			node_rx->hdr.user_meta = hci_get_class(node_rx);

			buf = process_prio_evt(node_rx, &evt_flags);
			if (buf) {
				LOG_DBG("Priority event");
				if (!(evt_flags & BT_HCI_EVT_FLAG_RECV)) {
					node_rx->hdr.next = NULL;
					ll_rx_mem_release((void **)&node_rx);
				}

				bt_recv_prio(buf);
				/* bt_recv_prio would not release normal evt
				 * buf.
				 */
				if (evt_flags & BT_HCI_EVT_FLAG_RECV) {
					net_buf_unref(buf);
				}
			}

			if (evt_flags & BT_HCI_EVT_FLAG_RECV) {
				/* Send the rx node up to Host thread,
				 * recv_thread()
				 */
				LOG_DBG("RX node enqueue");
				k_fifo_put(&recv_fifo, node_rx);
			}
		}

		if (iso_received || node_rx) {
			/* There may still be completed nodes, continue
			 * pushing all those up to Host before waiting
			 * for ULL mayfly
			 */
			continue;
		}

		LOG_DBG("sem take...");
		/* Wait until ULL mayfly has something to give us.
		 * Blocking-take of the semaphore; we take it once ULL mayfly
		 * has let it go in ll_rx_sched().
		 */
		k_sem_take(&sem_prio_recv, K_FOREVER);
		/* Now, ULL mayfly has something to give to us */
		LOG_DBG("sem taken");
	}
}

static inline struct net_buf *encode_node(struct node_rx_pdu *node_rx,
					  int8_t class)
{
	struct net_buf *buf = NULL;

	/* Check if we need to generate an HCI event or ACL data */
	switch (class) {
	case HCI_CLASS_EVT_DISCARDABLE:
	case HCI_CLASS_EVT_REQUIRED:
	case HCI_CLASS_EVT_CONNECTION:
	case HCI_CLASS_EVT_LLCP:
		if (class == HCI_CLASS_EVT_DISCARDABLE) {
			buf = bt_buf_get_evt(BT_HCI_EVT_UNKNOWN, true,
					     K_NO_WAIT);
		} else {
			buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);
		}
		if (buf) {
			hci_evt_encode(node_rx, buf);
		}
		break;
#if defined(CONFIG_BT_CONN)
	case HCI_CLASS_ACL_DATA:
		/* generate ACL data */
		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
		hci_acl_encode(node_rx, buf);
		break;
#endif
#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	case HCI_CLASS_ISO_DATA: {
		if (false) {

#if defined(CONFIG_BT_CTLR_CONN_ISO)
		} else if (IS_CIS_HANDLE(node_rx->hdr.handle)) {
			struct ll_conn_iso_stream *cis;

			cis = ll_conn_iso_stream_get(node_rx->hdr.handle);
			if (cis && !cis->teardown) {
				struct ll_iso_stream_hdr *hdr;
				struct ll_iso_datapath *dp;

				hdr = &cis->hdr;
				dp = hdr->datapath_out;
				if (dp && dp->path_id == BT_HCI_DATAPATH_ID_HCI) {
					/* If HCI datapath pass to ISO AL here */
					struct isoal_pdu_rx pckt_meta = {
						.meta = &node_rx->hdr.rx_iso_meta,
						.pdu  = (void *)&node_rx->pdu[0],
					};

					/* Pass the ISO PDU through ISO-AL */
					isoal_status_t err =
						isoal_rx_pdu_recombine(dp->sink_hdl, &pckt_meta);

					/* TODO handle err */
					LL_ASSERT(err == ISOAL_STATUS_OK);
				}
			}
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
		} else if (IS_SYNC_ISO_HANDLE(node_rx->hdr.handle)) {
			const struct lll_sync_iso_stream *stream;
			struct isoal_pdu_rx isoal_rx;
			uint16_t stream_handle;
			isoal_status_t err;

			stream_handle = LL_BIS_SYNC_IDX_FROM_HANDLE(node_rx->hdr.handle);
			stream = ull_sync_iso_stream_get(stream_handle);

			/* Check validity of the data path sink. FIXME: A channel disconnect race
			 * may cause ISO data pending without valid data path.
			 */
			if (stream && stream->dp) {
				isoal_rx.meta = &node_rx->hdr.rx_iso_meta;
				isoal_rx.pdu = (void *)node_rx->pdu;
				err = isoal_rx_pdu_recombine(stream->dp->sink_hdl, &isoal_rx);

				LL_ASSERT(err == ISOAL_STATUS_OK ||
					  err == ISOAL_STATUS_ERR_SDU_ALLOC);
			}
#endif /* CONFIG_BT_CTLR_SYNC_ISO */

		} else {
			LL_ASSERT(0);
		}

		node_rx->hdr.next = NULL;
		ll_iso_rx_mem_release((void **)&node_rx);

		return buf;
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO || CONFIG_BT_CTLR_CONN_ISO */

	default:
		LL_ASSERT(0);
		break;
	}

	node_rx->hdr.next = NULL;
	ll_rx_mem_release((void **)&node_rx);

	return buf;
}

static inline struct net_buf *process_node(struct node_rx_pdu *node_rx)
{
	uint8_t class = node_rx->hdr.user_meta;
	struct net_buf *buf = NULL;

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	if (hbuf_count != -1) {
		bool pend = !sys_slist_is_empty(&hbuf_pend);

		/* controller to host flow control enabled */
		switch (class) {
		case HCI_CLASS_ISO_DATA:
		case HCI_CLASS_EVT_DISCARDABLE:
		case HCI_CLASS_EVT_REQUIRED:
			break;
		case HCI_CLASS_EVT_CONNECTION:
		case HCI_CLASS_EVT_LLCP:
			/* for conn-related events, only pend is relevant */
			hbuf_count = 1;
			__fallthrough;
		case HCI_CLASS_ACL_DATA:
			if (pend || !hbuf_count) {
				sys_slist_append(&hbuf_pend, (void *)node_rx);
				LOG_DBG("FC: Queuing item: %d", class);
				return NULL;
			}
			break;
		default:
			LL_ASSERT(0);
			break;
		}
	}
#endif

	/* process regular node from radio */
	buf = encode_node(node_rx, class);

	return buf;
}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
static inline struct net_buf *process_hbuf(struct node_rx_pdu *n)
{
	/* shadow total count in case of preemption */
	struct node_rx_pdu *node_rx = NULL;
	int32_t hbuf_total = hci_hbuf_total;
	struct net_buf *buf = NULL;
	uint8_t class;
	int reset;

	reset = atomic_test_and_clear_bit(&hci_state_mask, HCI_STATE_BIT_RESET);
	if (reset) {
		/* flush queue, no need to free, the LL has already done it */
		sys_slist_init(&hbuf_pend);
	}

	if (hbuf_total <= 0) {
		hbuf_count = -1;
		return NULL;
	}

	/* available host buffers */
	hbuf_count = hbuf_total - (hci_hbuf_sent - hci_hbuf_acked);

	/* host acked ACL packets, try to dequeue from hbuf */
	node_rx = (void *)sys_slist_peek_head(&hbuf_pend);
	if (!node_rx) {
		return NULL;
	}

	/* Return early if this iteration already has a node to process */
	class = node_rx->hdr.user_meta;
	if (n) {
		if (class == HCI_CLASS_EVT_CONNECTION ||
		    class == HCI_CLASS_EVT_LLCP ||
		    (class == HCI_CLASS_ACL_DATA && hbuf_count)) {
			/* node to process later, schedule an iteration */
			LOG_DBG("FC: signalling");
			k_poll_signal_raise(&hbuf_signal, 0x0);
		}
		return NULL;
	}

	switch (class) {
	case HCI_CLASS_EVT_CONNECTION:
	case HCI_CLASS_EVT_LLCP:
		LOG_DBG("FC: dequeueing event");
		(void) sys_slist_get(&hbuf_pend);
		break;
	case HCI_CLASS_ACL_DATA:
		if (hbuf_count) {
			LOG_DBG("FC: dequeueing ACL data");
			(void) sys_slist_get(&hbuf_pend);
		} else {
			/* no buffers, HCI will signal */
			node_rx = NULL;
		}
		break;
	case HCI_CLASS_EVT_DISCARDABLE:
	case HCI_CLASS_EVT_REQUIRED:
	default:
		LL_ASSERT(0);
		break;
	}

	if (node_rx) {
		buf = encode_node(node_rx, class);
		/* Update host buffers after encoding */
		hbuf_count = hbuf_total - (hci_hbuf_sent - hci_hbuf_acked);
		/* next node */
		node_rx = (void *)sys_slist_peek_head(&hbuf_pend);
		if (node_rx) {
			class = node_rx->hdr.user_meta;

			if (class == HCI_CLASS_EVT_CONNECTION ||
			    class == HCI_CLASS_EVT_LLCP ||
			    (class == HCI_CLASS_ACL_DATA && hbuf_count)) {
				/* more to process, schedule an
				 * iteration
				 */
				LOG_DBG("FC: signalling");
				k_poll_signal_raise(&hbuf_signal, 0x0);
			}
		}
	}

	return buf;
}
#endif

/**
 * @brief Blockingly pull from Controller thread's recv_fifo
 * @details Execution context: Host thread
 */
static void recv_thread(void *p1, void *p2, void *p3)
{
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	/* @todo: check if the events structure really needs to be static */
	static struct k_poll_event events[2] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SIGNAL,
						K_POLL_MODE_NOTIFY_ONLY,
						&hbuf_signal, 0),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&recv_fifo, 0),
	};
#endif

	while (1) {
		struct node_rx_pdu *node_rx = NULL;
		struct net_buf *buf = NULL;

		LOG_DBG("blocking");
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
		int err;

		err = k_poll(events, 2, K_FOREVER);
		LL_ASSERT(err == 0 || err == -EINTR);
		if (events[0].state == K_POLL_STATE_SIGNALED) {
			events[0].signal->signaled = 0U;
		} else if (events[1].state ==
			   K_POLL_STATE_FIFO_DATA_AVAILABLE) {
			node_rx = k_fifo_get(events[1].fifo, K_NO_WAIT);
		}

		events[0].state = K_POLL_STATE_NOT_READY;
		events[1].state = K_POLL_STATE_NOT_READY;

		/* process host buffers first if any */
		buf = process_hbuf(node_rx);

#else
		node_rx = k_fifo_get(&recv_fifo, K_FOREVER);
#endif
		LOG_DBG("unblocked");

		if (node_rx && !buf) {
			/* process regular node from radio */
			buf = process_node(node_rx);
		}

		while (buf) {
			struct net_buf *frag;

			/* Increment ref count, which will be
			 * unref on call to net_buf_frag_del
			 */
			frag = net_buf_ref(buf);
			buf = net_buf_frag_del(NULL, buf);

			if (frag->len) {
				LOG_DBG("Packet in: type:%u len:%u", bt_buf_get_type(frag),
					frag->len);

				bt_recv(frag);
			} else {
				net_buf_unref(frag);
			}

			k_yield();
		}
	}
}

static int cmd_handle(struct net_buf *buf)
{
	struct node_rx_pdu *node_rx = NULL;
	struct net_buf *evt;

	evt = hci_cmd_handle(buf, (void **) &node_rx);
	if (evt) {
		LOG_DBG("Replying with event of %u bytes", evt->len);
		bt_recv_prio(evt);

		if (node_rx) {
			LOG_DBG("RX node enqueue");
			node_rx->hdr.user_meta = hci_get_class(node_rx);
			k_fifo_put(&recv_fifo, node_rx);
		}
	}

	return 0;
}

#if defined(CONFIG_BT_CONN)
static int acl_handle(struct net_buf *buf)
{
	struct net_buf *evt;
	int err;

	err = hci_acl_handle(buf, &evt);
	if (evt) {
		LOG_DBG("Replying with event of %u bytes", evt->len);
		bt_recv_prio(evt);
	}

	return err;
}
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
static int iso_handle(struct net_buf *buf)
{
	struct net_buf *evt;
	int err;

	err = hci_iso_handle(buf, &evt);
	if (evt) {
		LOG_DBG("Replying with event of %u bytes", evt->len);
		bt_recv_prio(evt);
	}

	return err;
}
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

static int hci_driver_send(struct net_buf *buf)
{
	uint8_t type;
	int err;

	LOG_DBG("enter");

	if (!buf->len) {
		LOG_ERR("Empty HCI packet");
		return -EINVAL;
	}

	type = bt_buf_get_type(buf);
	switch (type) {
#if defined(CONFIG_BT_CONN)
	case BT_BUF_ACL_OUT:
		err = acl_handle(buf);
		break;
#endif /* CONFIG_BT_CONN */
	case BT_BUF_CMD:
		err = cmd_handle(buf);
		break;
#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	case BT_BUF_ISO_OUT:
		err = iso_handle(buf);
		break;
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */
	default:
		LOG_ERR("Unknown HCI type %u", type);
		return -EINVAL;
	}

	if (!err) {
		net_buf_unref(buf);
	}

	LOG_DBG("exit: %d", err);

	return err;
}

static int hci_driver_open(void)
{
	uint32_t err;

	DEBUG_INIT();

	k_fifo_init(&recv_fifo);
	k_sem_init(&sem_prio_recv, 0, K_SEM_MAX_LIMIT);

	err = ll_init(&sem_prio_recv);
	if (err) {
		LOG_ERR("LL initialization failed: %d", err);
		return err;
	}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	k_poll_signal_init(&hbuf_signal);
	hci_init(&hbuf_signal);
#else
	hci_init(NULL);
#endif

	k_thread_create(&prio_recv_thread_data, prio_recv_thread_stack,
			K_KERNEL_STACK_SIZEOF(prio_recv_thread_stack),
			prio_recv_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(&prio_recv_thread_data, "BT RX pri");

	k_thread_create(&recv_thread_data, recv_thread_stack,
			K_KERNEL_STACK_SIZEOF(recv_thread_stack),
			recv_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_RX_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(&recv_thread_data, "BT RX");

	LOG_DBG("Success.");

	return 0;
}

static int hci_driver_close(void)
{
	/* Resetting the LL stops all roles */
	ll_deinit();

	/* Abort prio RX thread */
	k_thread_abort(&prio_recv_thread_data);

	/* Abort RX thread */
	k_thread_abort(&recv_thread_data);

	return 0;
}

static const struct bt_hci_driver drv = {
	.name	= "Controller",
	.bus	= BT_HCI_DRIVER_BUS_VIRTUAL,
	.quirks = BT_QUIRK_NO_AUTO_DLE,
	.open	= hci_driver_open,
	.close	= hci_driver_close,
	.send	= hci_driver_send,
};

static int hci_driver_init(void)
{

	bt_hci_driver_register(&drv);

	return 0;
}

SYS_INIT(hci_driver_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
