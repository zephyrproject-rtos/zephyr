/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <soc.h>

#include "hal/cpu.h"
#include "hal/ccm.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/mem.h"
#include "util/mfifo.h"
#include "util/mayfly.h"

#include "pdu.h"

#include "lll.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_adv_iso.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"
#include "lll_iso_tx.h"

#include "isoal.h"

#include "ull_adv_types.h"
#include "ull_sync_types.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_internal.h"
#include "ull_iso_internal.h"

#include "ull_adv_internal.h"
#include "ull_conn_internal.h"
#include "ull_sync_iso_internal.h"
#include "ull_conn_iso_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_iso
#include "common/log.h"
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_CONN_ISO_STREAMS)
#define BT_CTLR_CONN_ISO_STREAMS CONFIG_BT_CTLR_CONN_ISO_STREAMS
#else /* !CONFIG_BT_CTLR_CONN_ISO_STREAMS */
#define BT_CTLR_CONN_ISO_STREAMS 0
#endif /* !CONFIG_BT_CTLR_CONN_ISO_STREAMS */

#if defined(CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT)
#define BT_CTLR_SYNC_ISO_STREAMS (CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT)
#else /* !CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT */
#define BT_CTLR_SYNC_ISO_STREAMS 0
#endif /* CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT */

static int init_reset(void);

/* Allocate data path pools for RX/TX directions for each stream */
#define BT_CTLR_ISO_STREAMS ((2 * (BT_CTLR_CONN_ISO_STREAMS)) + \
			     BT_CTLR_SYNC_ISO_STREAMS)
static struct ll_iso_datapath datapath_pool[BT_CTLR_ISO_STREAMS];

static void *datapath_free;

#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
#define NODE_RX_HEADER_SIZE (offsetof(struct node_rx_pdu, pdu))
/* ISO LL conformance tests require a PDU size of maximum 251 bytes + header */
#define ISO_RX_BUFFER_SIZE (2 + 251)

/* Declare the ISO rx node RXFIFO. This is a composite pool-backed MFIFO for
 * rx_nodes. The declaration constructs the following data structures:
 * - mfifo_iso_rx:    FIFO with pointers to PDU buffers
 * - mem_iso_rx:      Backing data pool for PDU buffer elements
 * - mem_link_iso_rx: Pool of memq_link_t elements
 *
 * Two extra links are reserved for use by the ll_iso_rx and ull_iso_rx memq.
 */
static RXFIFO_DEFINE(iso_rx, NODE_RX_HEADER_SIZE + ISO_RX_BUFFER_SIZE,
			     CONFIG_BT_CTLR_ISO_RX_BUFFERS, 2);

static MEMQ_DECLARE(ll_iso_rx);
#if defined(CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH)
static MEMQ_DECLARE(ull_iso_rx);
static void iso_rx_demux(void *param);
#endif /* CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH */
#endif /* CONFIG_BT_CTLR_SYNC_ISO) || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
#define ISO_TX_BUF_SIZE MROUND(offsetof(struct node_tx_iso, pdu) + \
			       offsetof(struct pdu_iso, payload) + \
			       CONFIG_BT_CTLR_ISO_TX_BUFFER_SIZE)
static struct {
	void *free;
	uint8_t pool[ISO_TX_BUF_SIZE * CONFIG_BT_CTLR_ISO_TX_BUFFERS];
} mem_iso_tx;

static struct {
	void *free;
	uint8_t pool[sizeof(memq_link_t) * CONFIG_BT_CTLR_ISO_TX_BUFFERS];
} mem_link_tx;

static MFIFO_DEFINE(iso_ack, sizeof(struct lll_tx),
		    CONFIG_BT_CTLR_ISO_TX_BUFFERS);
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

/* Must be implemented by vendor */
__weak bool ll_data_path_configured(uint8_t data_path_dir,
				    uint8_t data_path_id)
{
	ARG_UNUSED(data_path_dir);
	ARG_UNUSED(data_path_id);

	return false;
}

/* Contains vendor specific argument, function to be implemented by vendors */
__weak uint8_t ll_configure_data_path(uint8_t data_path_dir,
				      uint8_t data_path_id,
				      uint8_t vs_config_len,
				      uint8_t *vs_config)
{
	ARG_UNUSED(data_path_dir);
	ARG_UNUSED(data_path_id);
	ARG_UNUSED(vs_config_len);
	ARG_UNUSED(vs_config);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_read_iso_tx_sync(uint16_t handle, uint16_t *seq,
			    uint32_t *timestamp, uint32_t *offset)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(seq);
	ARG_UNUSED(timestamp);
	ARG_UNUSED(offset);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

/* Must be implemented by vendor */
__weak bool ll_data_path_sink_create(struct ll_iso_datapath *datapath,
				     isoal_sink_sdu_alloc_cb *sdu_alloc,
				     isoal_sink_sdu_emit_cb *sdu_emit,
				     isoal_sink_sdu_write_cb *sdu_write)
{
	ARG_UNUSED(datapath);

	*sdu_alloc = NULL;
	*sdu_emit  = NULL;
	*sdu_write = NULL;

	return false;
}

static inline bool path_is_vendor_specific(uint8_t path_id)
{
	return (path_id >= BT_HCI_DATAPATH_ID_VS &&
		path_id <= BT_HCI_DATAPATH_ID_VS_END);
}

uint8_t ll_setup_iso_path(uint16_t handle, uint8_t path_dir, uint8_t path_id,
			  uint8_t coding_format, uint16_t company_id,
			  uint16_t vs_codec_id, uint32_t controller_delay,
			  uint8_t codec_config_len, uint8_t *codec_config)
{
	ARG_UNUSED(controller_delay);
	ARG_UNUSED(codec_config_len);
	ARG_UNUSED(codec_config);

	if (path_id == BT_HCI_DATAPATH_ID_DISABLED) {
		return 0;
	}

	if (path_dir != BT_HCI_DATAPATH_DIR_CTLR_TO_HOST) {
		/* FIXME: workaround to succeed datapath setup for ISO
		 *        broadcaster until Tx datapath is implemented, in the
		 *        future.
		 */
		return BT_HCI_ERR_SUCCESS;
	}

#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	isoal_sink_handle_t sink_handle;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	uint8_t flush_timeout;
	uint16_t iso_interval;
	uint32_t sdu_interval;
	uint8_t  burst_number;
	isoal_status_t err;
	uint8_t role;

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	struct ll_iso_datapath *dp_in = NULL;
	struct ll_iso_datapath *dp_out = NULL;

	struct ll_conn_iso_stream *cis = NULL;
	struct ll_conn_iso_group *cig = NULL;

	if (IS_CIS_HANDLE(handle)) {
		cis = ll_conn_iso_stream_get(handle);
		cig = cis->group;
		dp_in = cis->hdr.datapath_in;
		dp_out = cis->hdr.datapath_out;
	}

	/* If the Host attempts to set a data path with a Connection Handle
	 * that does not exist or that is not for a CIS or a BIS, the Controller
	 * shall return the error code Unknown Connection Identifier (0x02)
	 */
	if (!cis) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if ((path_dir == BT_HCI_DATAPATH_DIR_HOST_TO_CTLR  && dp_in) ||
	    (path_dir == BT_HCI_DATAPATH_DIR_CTLR_TO_HOST && dp_out)) {
		/* Data path has been set up, can only do setup once */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (path_is_vendor_specific(path_id) &&
	    !ll_data_path_configured(path_dir, path_id)) {
		/* Data path must be configured prior to setup */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* If Codec_Configuration_Length non-zero and Codec_ID set to
	 * transparent air mode, the Controller shall return the error code
	 * Invalid HCI Command Parameters (0x12).
	 */
	if (codec_config_len && vs_codec_id == BT_HCI_CODING_FORMAT_TRANSPARENT) {
		return BT_HCI_ERR_INVALID_PARAM;
	}
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	struct lll_sync_iso_stream *stream;
	uint16_t stream_handle;

	if (handle < BT_CTLR_SYNC_ISO_STREAM_HANDLE_BASE) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
	stream_handle = handle - BT_CTLR_SYNC_ISO_STREAM_HANDLE_BASE;

	stream = ull_sync_iso_stream_get(stream_handle);
	if (!stream || stream->dp) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO */

	/* Allocate and configure datapath */
	struct ll_iso_datapath *dp = mem_acquire(&datapath_free);
	if (!dp) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	dp->path_dir = path_dir;
	dp->path_id = path_id;
	dp->coding_format = coding_format;
	dp->company_id = company_id;

	/* TODO dp->sync_delay    = controller_delay; ?*/

	flush_timeout = 0;
	stream_sync_delay = 0;
	group_sync_delay = 0;

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	role = cig->lll.role;
	iso_interval = cig->iso_interval;
	stream_sync_delay = cis->sync_delay;
	group_sync_delay = cig->sync_delay;

	if (path_dir == BT_HCI_DATAPATH_DIR_HOST_TO_CTLR) {
		burst_number  = cis->lll.tx.burst_number;
		flush_timeout = cis->lll.tx.flush_timeout;

		if (role) {
			/* peripheral */
			sdu_interval = cig->p_sdu_interval;
		} else {
			/* central */
			sdu_interval = cig->c_sdu_interval;
		}

		cis->hdr.datapath_in = dp;
	} else {
		burst_number =  cis->lll.rx.burst_number;
		flush_timeout = cis->lll.rx.flush_timeout;

		if (role) {
			/* peripheral */
			sdu_interval = cig->c_sdu_interval;
		} else {
			/* central */
			sdu_interval = cig->p_sdu_interval;
		}

		cis->hdr.datapath_out = dp;
	}
#endif

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	struct ll_sync_iso_set *sync_iso;
	struct lll_sync_iso *lll_iso;

	sync_iso = ull_sync_iso_by_stream_get(stream_handle);
	lll_iso = &sync_iso->lll;

	role = 1U; /* FIXME: Set role from LLL struct */
	burst_number = lll_iso->bn;
	sdu_interval = lll_iso->sdu_interval;
	iso_interval = lll_iso->iso_interval;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */

	if (path_id == BT_HCI_DATAPATH_ID_HCI) {
		/* Not vendor specific, thus alloc and emit functions known */
		err = isoal_sink_create(handle, role,
					burst_number, flush_timeout,
					sdu_interval, iso_interval,
					stream_sync_delay, group_sync_delay,
					sink_sdu_alloc_hci, sink_sdu_emit_hci,
					sink_sdu_write_hci, &sink_handle);
	} else {
		/* Set up vendor specific data path */
		isoal_sink_sdu_alloc_cb sdu_alloc;
		isoal_sink_sdu_emit_cb  sdu_emit;
		isoal_sink_sdu_write_cb sdu_write;

		/* Request vendor sink callbacks for path */
		if (ll_data_path_sink_create(dp, &sdu_alloc, &sdu_emit, &sdu_write)) {
			err = isoal_sink_create(handle, role,
						burst_number, flush_timeout,
						sdu_interval, iso_interval,
						stream_sync_delay, group_sync_delay,
						sdu_alloc, sdu_emit, sdu_write,
						&sink_handle);
		} else {
			ull_iso_datapath_release(dp);

			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}

	if (!err) {
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
		stream->dp = dp;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */

		dp->sink_hdl = sink_handle;
		isoal_sink_enable(sink_handle);
	} else {
		ull_iso_datapath_release(dp);

		return BT_HCI_ERR_CMD_DISALLOWED;
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO || CONFIG_BT_CTLR_CONN_ISO */

	return 0;
}

uint8_t ll_remove_iso_path(uint16_t handle, uint8_t path_dir)
{
	struct ll_iso_datapath *dp = NULL;

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	struct ll_conn_iso_stream *cis = NULL;
	struct ll_iso_stream_hdr *hdr = NULL;

	if (IS_CIS_HANDLE(handle)) {
		cis = ll_conn_iso_stream_get(handle);
		hdr = &cis->hdr;
	}

	/* If the Host issues this command with a Connection_Handle that does not exist
	 * or is not for a CIS or a BIS, the Controller shall return the error code Unknown
	 * Connection Identifier (0x02).
	 */
	if (!cis) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (path_dir == BT_HCI_DATAPATH_DIR_HOST_TO_CTLR) {
		dp = hdr->datapath_in;
		if (dp) {
			hdr->datapath_in = NULL;
			ull_iso_datapath_release(dp);
		}
	} else if (path_dir == BT_HCI_DATAPATH_DIR_CTLR_TO_HOST) {
		dp = hdr->datapath_out;
		if (dp) {
			hdr->datapath_out = NULL;
			ull_iso_datapath_release(dp);
		}
	} else {
		/* Reserved for future use */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	struct lll_sync_iso_stream *stream;
	uint16_t stream_handle;

	if (path_dir != BT_HCI_DATAPATH_DIR_CTLR_TO_HOST) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (handle < BT_CTLR_SYNC_ISO_STREAM_HANDLE_BASE) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
	stream_handle = handle - BT_CTLR_SYNC_ISO_STREAM_HANDLE_BASE;

	stream = ull_sync_iso_stream_get(stream_handle);
	if (!stream) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	dp = stream->dp;
	if (dp) {
		stream->dp = NULL;
		isoal_sink_destroy(dp->sink_hdl);
		ull_iso_datapath_release(dp);
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO */

	if (!dp) {
		/* Datapath was not previously set up */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	return 0;
}

#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
uint8_t ll_iso_receive_test(uint16_t handle, uint8_t payload_type)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(payload_type);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_iso_read_test_counters(uint16_t handle, uint32_t *received_cnt,
				  uint32_t *missed_cnt,
				  uint32_t *failed_cnt)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(received_cnt);
	ARG_UNUSED(missed_cnt);
	ARG_UNUSED(failed_cnt);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

#if defined(CONFIG_BT_CTLR_READ_ISO_LINK_QUALITY)
uint8_t ll_read_iso_link_quality(uint16_t  handle,
				 uint32_t *tx_unacked_packets,
				 uint32_t *tx_flushed_packets,
				 uint32_t *tx_last_subevent_packets,
				 uint32_t *retransmitted_packets,
				 uint32_t *crc_error_packets,
				 uint32_t *rx_unreceived_packets,
				 uint32_t *duplicate_packets)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(tx_unacked_packets);
	ARG_UNUSED(tx_flushed_packets);
	ARG_UNUSED(tx_last_subevent_packets);
	ARG_UNUSED(retransmitted_packets);
	ARG_UNUSED(crc_error_packets);
	ARG_UNUSED(rx_unreceived_packets);
	ARG_UNUSED(duplicate_packets);

	return BT_HCI_ERR_CMD_DISALLOWED;
}
#endif /* CONFIG_BT_CTLR_READ_ISO_LINK_QUALITY */

#endif /* CONFIG_BT_CTLR_SYNC_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
uint8_t ll_iso_transmit_test(uint16_t handle, uint8_t payload_type)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(payload_type);

	return BT_HCI_ERR_CMD_DISALLOWED;
}
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

uint8_t ll_iso_test_end(uint16_t handle, uint32_t *received_cnt,
			uint32_t *missed_cnt, uint32_t *failed_cnt)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(received_cnt);
	ARG_UNUSED(missed_cnt);
	ARG_UNUSED(failed_cnt);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
void *ll_iso_tx_mem_acquire(void)
{
	return mem_acquire(&mem_iso_tx.free);
}

void ll_iso_tx_mem_release(void *node_tx)
{
	mem_release(node_tx, &mem_iso_tx.free);
}

int ll_iso_tx_mem_enqueue(uint16_t handle, void *node_tx)
{
	struct lll_adv_iso_stream *stream;
	memq_link_t *link;

	/* FIXME: Translate to CIS or BIS handle
	 */

	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_ISO)) {
		stream = ull_adv_iso_stream_get(handle);
	} else {
		/* FIXME: Get connected ISO stream instance */
		stream = NULL;
	}

	if (!stream) {
		return -EINVAL;
	}

	link = mem_acquire(&mem_link_tx.free);
	LL_ASSERT(link);

	memq_enqueue(link, node_tx, &stream->memq_tx.tail);

	return 0;
}
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

int ull_iso_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_iso_reset(void)
{
	int err;

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	/* Re-initialize the Tx Ack mfifo */
	MFIFO_INIT(iso_ack);
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
void ull_iso_lll_ack_enqueue(uint16_t handle, struct node_tx_iso *node_tx)
{
	struct lll_tx *tx;
	uint8_t idx;

	idx = MFIFO_ENQUEUE_GET(iso_ack, (void **)&tx);
	LL_ASSERT(tx);

	tx->handle = handle;
	tx->node = node_tx;

	MFIFO_ENQUEUE(iso_ack, idx);

	ll_rx_sched();
}

uint8_t ull_iso_tx_ack_get(uint16_t *handle)
{
	struct lll_tx *tx;
	uint8_t cmplt = 0U;

	tx = MFIFO_DEQUEUE_GET(iso_ack);
	if (tx) {
		*handle = tx->handle;

		do {
			struct node_tx_iso *node_tx;

			cmplt++;

			node_tx = tx->node;

			MFIFO_DEQUEUE(iso_ack);

			mem_release(node_tx->link, &mem_link_tx.free);
			mem_release(node_tx, &mem_iso_tx.free);

			tx = MFIFO_DEQUEUE_GET(iso_ack);
		} while (tx && (tx->handle == *handle));
	}

	return cmplt;
}
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
void *ull_iso_pdu_rx_alloc_peek(uint8_t count)
{
	if (count > MFIFO_AVAIL_COUNT_GET(iso_rx)) {
		return NULL;
	}

	return MFIFO_DEQUEUE_PEEK(iso_rx);
}

void *ull_iso_pdu_rx_alloc(void)
{
	return MFIFO_DEQUEUE(iso_rx);
}

#if defined(CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH)
void ull_iso_rx_put(memq_link_t *link, void *rx)
{
	/* Enqueue the Rx object */
	memq_enqueue(link, rx, &memq_ull_iso_rx.tail);
}

void ull_iso_rx_sched(void)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, iso_rx_demux};

	/* Kick the ULL (using the mayfly, tailchain it) */
	mayfly_enqueue(TICKER_USER_ID_LLL, TICKER_USER_ID_ULL_HIGH, 1, &mfy);
}

static void iso_rx_demux(void *param)
{
	struct ll_conn_iso_stream *cis;
	struct ll_iso_datapath *dp;
	struct node_rx_pdu *rx_pdu;
	isoal_sink_handle_t sink;
	struct node_rx_hdr *rx;
	memq_link_t *link;

	do {
		link = memq_peek(memq_ull_iso_rx.head, memq_ull_iso_rx.tail,
				 (void **)&rx);
		if (link) {
			/* Demux Rx objects */
			switch (rx->type) {
			case NODE_RX_TYPE_RELEASE:
				(void)memq_dequeue(memq_ull_iso_rx.tail,
						   &memq_ull_iso_rx.head, NULL);
				ll_iso_rx_put(link, rx);
				ll_rx_sched();
				break;

			case NODE_RX_TYPE_ISO_PDU:
				/* Remove from receive-queue; ULL has received this now */
				(void)memq_dequeue(memq_ull_iso_rx.tail, &memq_ull_iso_rx.head,
						   NULL);

#if defined(CONFIG_BT_CTLR_CONN_ISO)
				rx_pdu = (struct node_rx_pdu *)rx;
				cis = ll_conn_iso_stream_get(rx_pdu->hdr.handle);
				dp = cis->hdr.datapath_out;
				sink = dp->sink_hdl;

				if (dp->path_id != BT_HCI_DATAPATH_ID_HCI) {
					/* If vendor specific datapath pass to ISO AL here,
					 * in case of HCI destination it will be passed in
					 * HCI context.
					 */
					struct isoal_pdu_rx pckt_meta = {
						.meta = &rx_pdu->hdr.rx_iso_meta,
						.pdu  = (struct pdu_iso *)&rx_pdu->pdu[0]
					};

					/* Pass the ISO PDU through ISO-AL */
					const isoal_status_t err =
						isoal_rx_pdu_recombine(sink, &pckt_meta);

					LL_ASSERT(err == ISOAL_STATUS_OK); /* TODO handle err */
				}
#endif /* CONFIG_BT_CTLR_CONN_ISO */

				/* Let ISO PDU start its long journey upwards */
				ll_iso_rx_put(link, rx);
				ll_rx_sched();
				break;

			default:
				LL_ASSERT(0);
				break;
			}
		}
	} while (link);
}
#endif /* CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH */

void ll_iso_rx_put(memq_link_t *link, void *rx)
{
	/* Enqueue the Rx object */
	memq_enqueue(link, rx, &memq_ll_iso_rx.tail);
}

void *ll_iso_rx_get(void)
{
	struct node_rx_hdr *rx;
	memq_link_t *link;

	link = memq_peek(memq_ll_iso_rx.head, memq_ll_iso_rx.tail, (void **)&rx);
	while (link) {
		/* Do not send up buffers to Host thread that are
		 * marked for release
		 */
		if (rx->type == NODE_RX_TYPE_RELEASE) {
			(void)memq_dequeue(memq_ll_iso_rx.tail,
						&memq_ll_iso_rx.head, NULL);
			mem_release(link, &mem_link_iso_rx.free);
			mem_release(rx, &mem_iso_rx.free);
			RXFIFO_ALLOC(iso_rx, 1);

			link = memq_peek(memq_ll_iso_rx.head, memq_ll_iso_rx.tail, (void **)&rx);
			continue;
		}
		return rx;
	}

	return NULL;
}

void ll_iso_rx_dequeue(void)
{
	struct node_rx_hdr *rx = NULL;
	memq_link_t *link;

	link = memq_dequeue(memq_ll_iso_rx.tail, &memq_ll_iso_rx.head,
			    (void **)&rx);
	LL_ASSERT(link);

	mem_release(link, &mem_link_iso_rx.free);

	/* Handle object specific clean up */
	switch (rx->type) {
	case NODE_RX_TYPE_ISO_PDU:
		break;
	default:
		LL_ASSERT(0);
		break;
	}
}

void ll_iso_rx_mem_release(void **node_rx)
{
	struct node_rx_hdr *rx;

	rx = *node_rx;
	while (rx) {
		struct node_rx_hdr *rx_free;

		rx_free = rx;
		rx = rx->next;

		switch (rx_free->type) {
		case NODE_RX_TYPE_ISO_PDU:
			mem_release(rx_free, &mem_iso_rx.free);
			break;
		default:
			LL_ASSERT(0);
			break;
		}
	}

	*node_rx = rx;

	RXFIFO_ALLOC(iso_rx, UINT8_MAX);
}
#endif /* CONFIG_BT_CTLR_SYNC_ISO) || CONFIG_BT_CTLR_CONN_ISO */

void ull_iso_datapath_release(struct ll_iso_datapath *dp)
{
	mem_release(dp, &datapath_free);
}

static int init_reset(void)
{
#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	memq_link_t *link;

	RXFIFO_INIT(iso_rx);

	/* Acquire a link to initialize ull rx memq */
	link = mem_acquire(&mem_link_iso_rx.free);
	LL_ASSERT(link);

#if defined(CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH)
	/* Initialize ull rx memq */
	MEMQ_INIT(ull_iso_rx, link);
#endif /* CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH */

	/* Acquire a link to initialize ll_iso_rx memq */
	link = mem_acquire(&mem_link_iso_rx.free);
	LL_ASSERT(link);

	/* Initialize ll_iso_rx memq */
	MEMQ_INIT(ll_iso_rx, link);

	RXFIFO_ALLOC(iso_rx, UINT8_MAX);
#endif /* CONFIG_BT_CTLR_SYNC_ISO) || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	/* Initialize tx pool. */
	mem_init(mem_iso_tx.pool, ISO_TX_BUF_SIZE,
		 CONFIG_BT_CTLR_ISO_TX_BUFFERS, &mem_iso_tx.free);

	/* Initialize tx link pool. */
	mem_init(mem_link_tx.pool, sizeof(memq_link_t),
		 CONFIG_BT_CTLR_ISO_TX_BUFFERS, &mem_link_tx.free);
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

	/* Initialize ISO Datapath pool */
	mem_init(datapath_pool, sizeof(struct ll_iso_datapath),
		 sizeof(datapath_pool) / sizeof(struct ll_iso_datapath), &datapath_free);

	return 0;
}
