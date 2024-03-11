/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/buf.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_adv_iso.h"
#include "lll/lll_df_types.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"
#include "lll_iso_tx.h"

#include "ll_sw/ull_tx_queue.h"

#include "isoal.h"

#include "ull_adv_types.h"
#include "ull_sync_types.h"
#include "ull_conn_types.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_llcp.h"

#include "ull_internal.h"
#include "ull_adv_internal.h"
#include "ull_conn_internal.h"
#include "ull_iso_internal.h"
#include "ull_sync_iso_internal.h"
#include "ull_conn_iso_internal.h"

#include "ll_feat.h"

#include "hal/debug.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_ctlr_ull_iso);

#if defined(CONFIG_BT_CTLR_CONN_ISO_STREAMS)
#define BT_CTLR_CONN_ISO_STREAMS CONFIG_BT_CTLR_CONN_ISO_STREAMS
#else /* !CONFIG_BT_CTLR_CONN_ISO_STREAMS */
#define BT_CTLR_CONN_ISO_STREAMS 0
#endif /* !CONFIG_BT_CTLR_CONN_ISO_STREAMS */

#if defined(CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT)
#define BT_CTLR_ADV_ISO_STREAMS (CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT)
#else /* !CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT */
#define BT_CTLR_ADV_ISO_STREAMS 0
#endif /* CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT */

#if defined(CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT)
#define BT_CTLR_SYNC_ISO_STREAMS (CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT)
#else /* !CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT */
#define BT_CTLR_SYNC_ISO_STREAMS 0
#endif /* CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT */

static int init_reset(void);

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
static isoal_status_t ll_iso_pdu_alloc(struct isoal_pdu_buffer *pdu_buffer);
static isoal_status_t ll_iso_pdu_write(struct isoal_pdu_buffer *pdu_buffer,
				       const size_t   offset,
				       const uint8_t *sdu_payload,
				       const size_t   consume_len);
static isoal_status_t ll_iso_pdu_emit(struct node_tx_iso *node_tx,
				      const uint16_t handle);
static isoal_status_t ll_iso_pdu_release(struct node_tx_iso *node_tx,
					 const uint16_t handle,
					 const isoal_status_t status);
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

/* Allocate data path pools for RX/TX directions for each stream */
#define BT_CTLR_ISO_STREAMS ((2 * (BT_CTLR_CONN_ISO_STREAMS)) + \
			     BT_CTLR_ADV_ISO_STREAMS + \
			     BT_CTLR_SYNC_ISO_STREAMS)
#if BT_CTLR_ISO_STREAMS
static struct ll_iso_datapath datapath_pool[BT_CTLR_ISO_STREAMS];
#endif /* BT_CTLR_ISO_STREAMS */

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
 * One extra rx buffer is reserved for empty ISO PDU reception.
 * Two extra links are reserved for use by the ll_iso_rx and ull_iso_rx memq.
 */
static RXFIFO_DEFINE(iso_rx, ((NODE_RX_HEADER_SIZE) + (ISO_RX_BUFFER_SIZE)),
			     (CONFIG_BT_CTLR_ISO_RX_BUFFERS + 1U), 2U);

static MEMQ_DECLARE(ll_iso_rx);
#if defined(CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH)
static MEMQ_DECLARE(ull_iso_rx);
static void iso_rx_demux(void *param);
#endif /* CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH */
#endif /* CONFIG_BT_CTLR_SYNC_ISO) || CONFIG_BT_CTLR_CONN_ISO */

#define ISO_TEST_PACKET_COUNTER_SIZE 4U

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
void ll_iso_link_tx_release(void *link);
void ll_iso_tx_mem_release(void *node_tx);

#define NODE_TX_BUFFER_SIZE MROUND(offsetof(struct node_tx_iso, pdu) + \
				   offsetof(struct pdu_iso, payload) + \
				   MAX(LL_BIS_OCTETS_TX_MAX, \
				       LL_CIS_OCTETS_TX_MAX))

#define ISO_TEST_TX_BUFFER_SIZE 32U

static struct {
	void *free;
	uint8_t pool[NODE_TX_BUFFER_SIZE * BT_CTLR_ISO_TX_BUFFERS];
} mem_iso_tx;

static struct {
	void *free;
	uint8_t pool[sizeof(memq_link_t) * BT_CTLR_ISO_TX_BUFFERS];
} mem_link_iso_tx;

#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

uint8_t ll_read_iso_tx_sync(uint16_t handle, uint16_t *seq,
			    uint32_t *timestamp, uint32_t *offset)
{
	if (IS_CIS_HANDLE(handle)) {
		struct ll_iso_datapath *dp = NULL;
		struct ll_conn_iso_stream *cis;

		cis = ll_conn_iso_stream_get(handle);

		if (cis) {
			dp = cis->hdr.datapath_in;
		}

		if (dp &&
			isoal_tx_get_sync_info(dp->source_hdl, seq,
					timestamp, offset) == ISOAL_STATUS_OK) {
			return BT_HCI_ERR_SUCCESS;
		}

		return BT_HCI_ERR_CMD_DISALLOWED;

	} else if (IS_ADV_ISO_HANDLE(handle)) {
		const struct lll_adv_iso_stream *adv_stream;
		uint16_t stream_handle;

		stream_handle = LL_BIS_ADV_IDX_FROM_HANDLE(handle);
		adv_stream = ull_adv_iso_stream_get(stream_handle);
		if (!adv_stream || !adv_stream->dp ||
		    isoal_tx_get_sync_info(adv_stream->dp->source_hdl, seq,
					   timestamp, offset) != ISOAL_STATUS_OK) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		return BT_HCI_ERR_SUCCESS;

	} else if (IS_SYNC_ISO_HANDLE(handle)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	return BT_HCI_ERR_UNKNOWN_CONN_ID;
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
	struct lll_sync_iso_stream *sync_stream = NULL;
	struct lll_adv_iso_stream *adv_stream = NULL;
	struct ll_conn_iso_stream *cis = NULL;
	struct ll_iso_datapath *dp;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	uint8_t flush_timeout;
	uint16_t iso_interval;
	uint32_t sdu_interval;
	uint8_t  burst_number;
	uint8_t max_octets;
	uint8_t framed;
	uint8_t role;

	ARG_UNUSED(controller_delay);
	ARG_UNUSED(codec_config);

	if (false) {

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	} else if (IS_CIS_HANDLE(handle)) {
		struct ll_conn_iso_group *cig;
		struct ll_conn *conn;

		/* If the Host attempts to set a data path with a Connection
		 * Handle that does not exist or that is not for a CIS or a BIS,
		 * the Controller shall return the error code Unknown Connection
		 * Identifier (0x02)
		 */
		cis = ll_conn_iso_stream_get(handle);
		if (!cis || !cis->group) {
			/* CIS does not belong to a CIG */
			return BT_HCI_ERR_UNKNOWN_CONN_ID;
		}

		conn = ll_connected_get(cis->lll.acl_handle);
		if (conn) {
			/* If we're still waiting for accept/response from
			 * host, path setup is premature and we must return
			 * disallowed status.
			 */
#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
			const uint8_t cis_waiting = ull_cp_cc_awaiting_reply(conn);

			if (cis_waiting) {
				return BT_HCI_ERR_CMD_DISALLOWED;
			}
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */
		}

		if ((path_dir == BT_HCI_DATAPATH_DIR_HOST_TO_CTLR && cis->hdr.datapath_in) ||
		    (path_dir == BT_HCI_DATAPATH_DIR_CTLR_TO_HOST && cis->hdr.datapath_out)) {
			/* Data path has been set up, can only do setup once */
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		cig = cis->group;

		role = cig->lll.role;
		iso_interval = cig->iso_interval;
		group_sync_delay = cig->sync_delay;
		stream_sync_delay = cis->sync_delay;
		framed = cis->framed;

		if (path_dir == BT_HCI_DATAPATH_DIR_CTLR_TO_HOST) {
			/* Create sink for RX data path */
			burst_number  = cis->lll.rx.bn;
			flush_timeout = cis->lll.rx.ft;
			max_octets    = cis->lll.rx.max_pdu;

			if (role) {
				/* peripheral */
				sdu_interval = cig->c_sdu_interval;
			} else {
				/* central */
				sdu_interval = cig->p_sdu_interval;
			}
		} else {
			/* path_dir == BT_HCI_DATAPATH_DIR_HOST_TO_CTLR */
			burst_number  = cis->lll.tx.bn;
			flush_timeout = cis->lll.tx.ft;
			max_octets    = cis->lll.tx.max_pdu;

			if (role) {
				/* peripheral */
				sdu_interval = cig->p_sdu_interval;
			} else {
				/* central */
				sdu_interval = cig->c_sdu_interval;
			}
		}
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	} else if (IS_ADV_ISO_HANDLE(handle)) {
		struct ll_adv_iso_set *adv_iso;
		struct lll_adv_iso *lll_iso;
		uint16_t stream_handle;

		stream_handle = LL_BIS_ADV_IDX_FROM_HANDLE(handle);
		adv_stream = ull_adv_iso_stream_get(stream_handle);
		if (!adv_stream || adv_stream->dp) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		adv_iso = ull_adv_iso_by_stream_get(stream_handle);
		lll_iso = &adv_iso->lll;

		role = ISOAL_ROLE_BROADCAST_SOURCE;
		iso_interval = lll_iso->iso_interval;
		sdu_interval = lll_iso->sdu_interval;
		burst_number = lll_iso->bn;
		flush_timeout = 0U; /* Not used for Broadcast ISO */
		group_sync_delay = 0U; /* FIXME: */
		stream_sync_delay = 0U; /* FIXME: */
		framed = 0U; /* FIXME: pick the framing value from context */
		max_octets = lll_iso->max_pdu;
#endif /* CONFIG_BT_CTLR_ADV_ISO */

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	} else if (IS_SYNC_ISO_HANDLE(handle)) {
		struct ll_sync_iso_set *sync_iso;
		struct lll_sync_iso *lll_iso;
		uint16_t stream_handle;

		stream_handle = LL_BIS_SYNC_IDX_FROM_HANDLE(handle);
		sync_stream = ull_sync_iso_stream_get(stream_handle);
		if (!sync_stream || sync_stream->dp) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		sync_iso = ull_sync_iso_by_stream_get(stream_handle);
		lll_iso = &sync_iso->lll;

		role = ISOAL_ROLE_BROADCAST_SINK;
		iso_interval = lll_iso->iso_interval;
		sdu_interval = lll_iso->sdu_interval;
		burst_number = lll_iso->bn;
		flush_timeout = 0U; /* Not used for Broadcast ISO */
		group_sync_delay = 0U; /* FIXME: */
		stream_sync_delay = 0U; /* FIXME: */
		framed = 0U; /* FIXME: pick the framing value from context */
		max_octets = 0U;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */

	} else {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (path_is_vendor_specific(path_id) &&
	    (!IS_ENABLED(CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH) ||
	     !ll_data_path_configured(path_dir, path_id))) {
		/* Data path must be configured prior to setup */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* If Codec_Configuration_Length non-zero and Codec_ID set to
	 * transparent air mode, the Controller shall return the error code
	 * Invalid HCI Command Parameters (0x12).
	 */
	if (codec_config_len &&
	    (vs_codec_id == BT_HCI_CODING_FORMAT_TRANSPARENT)) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* Allocate and configure datapath */
	dp = mem_acquire(&datapath_free);
	if (!dp) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	dp->path_dir = path_dir;
	dp->path_id = path_id;
	dp->coding_format = coding_format;
	dp->company_id = company_id;

	/* TODO dp->sync_delay    = controller_delay; ?*/

	if (false) {

#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	} else if ((path_dir == BT_HCI_DATAPATH_DIR_CTLR_TO_HOST) &&
		   (cis || sync_stream)) {
		isoal_sink_handle_t sink_handle;
		isoal_status_t err;

		if (path_id == BT_HCI_DATAPATH_ID_HCI) {
			/* Not vendor specific, thus alloc and emit functions
			 * known
			 */
			err = isoal_sink_create(handle, role, framed,
						burst_number, flush_timeout,
						sdu_interval, iso_interval,
						stream_sync_delay,
						group_sync_delay,
						sink_sdu_alloc_hci,
						sink_sdu_emit_hci,
						sink_sdu_write_hci,
						&sink_handle);
		} else {
			/* Set up vendor specific data path */
			isoal_sink_sdu_alloc_cb sdu_alloc;
			isoal_sink_sdu_emit_cb  sdu_emit;
			isoal_sink_sdu_write_cb sdu_write;

			/* Request vendor sink callbacks for path */
			if (IS_ENABLED(CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH) &&
			    ll_data_path_sink_create(handle, dp, &sdu_alloc,
						     &sdu_emit, &sdu_write)) {
				err = isoal_sink_create(handle, role, framed,
							burst_number,
							flush_timeout,
							sdu_interval,
							iso_interval,
							stream_sync_delay,
							group_sync_delay,
							sdu_alloc, sdu_emit,
							sdu_write,
							&sink_handle);
			} else {
				ull_iso_datapath_release(dp);

				return BT_HCI_ERR_CMD_DISALLOWED;
			}
		}

		if (!err) {
			if (cis) {
				cis->hdr.datapath_out = dp;
			}

			if (sync_stream) {
				sync_stream->dp = dp;
			}

			dp->sink_hdl = sink_handle;
			isoal_sink_enable(sink_handle);
		} else {
			ull_iso_datapath_release(dp);

			return BT_HCI_ERR_CMD_DISALLOWED;
		}
#else /* !CONFIG_BT_CTLR_SYNC_ISO && !CONFIG_BT_CTLR_CONN_ISO */
		ARG_UNUSED(sync_stream);
#endif /* !CONFIG_BT_CTLR_SYNC_ISO && !CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	} else if ((path_dir == BT_HCI_DATAPATH_DIR_HOST_TO_CTLR) &&
		   (cis || adv_stream)) {
		isoal_source_handle_t source_handle;
		isoal_status_t err;

		/* Create source for TX data path */
		isoal_source_pdu_alloc_cb   pdu_alloc;
		isoal_source_pdu_write_cb   pdu_write;
		isoal_source_pdu_emit_cb    pdu_emit;
		isoal_source_pdu_release_cb pdu_release;

		if (path_is_vendor_specific(path_id)) {
			if (!IS_ENABLED(CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH) ||
			    !ll_data_path_source_create(handle, dp,
							&pdu_alloc, &pdu_write,
							&pdu_emit,
							&pdu_release)) {
				ull_iso_datapath_release(dp);

				return BT_HCI_ERR_CMD_DISALLOWED;
			}
		} else {
			/* Set default callbacks when not vendor specific
			 * or that the vendor specific path is the same.
			 */
			pdu_alloc   = ll_iso_pdu_alloc;
			pdu_write   = ll_iso_pdu_write;
			pdu_emit    = ll_iso_pdu_emit;
			pdu_release = ll_iso_pdu_release;
		}

		err = isoal_source_create(handle, role, framed, burst_number,
					  flush_timeout, max_octets,
					  sdu_interval, iso_interval,
					  stream_sync_delay, group_sync_delay,
					  pdu_alloc, pdu_write, pdu_emit,
					  pdu_release, &source_handle);

		if (!err) {
			if (IS_ENABLED(CONFIG_BT_CTLR_CONN_ISO) && cis != NULL) {
				cis->hdr.datapath_in = dp;
			}

			if (IS_ENABLED(CONFIG_BT_CTLR_ADV_ISO) && adv_stream != NULL) {
				adv_stream->dp = dp;
			}

			dp->source_hdl = source_handle;
			isoal_source_enable(source_handle);
		} else {
			ull_iso_datapath_release(dp);

			return BT_HCI_ERR_CMD_DISALLOWED;
		}

#else /* !CONFIG_BT_CTLR_ADV_ISO && !CONFIG_BT_CTLR_CONN_ISO */
		ARG_UNUSED(adv_stream);
#endif /* !CONFIG_BT_CTLR_ADV_ISO && !CONFIG_BT_CTLR_CONN_ISO */

	} else {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ll_remove_iso_path(uint16_t handle, uint8_t path_dir)
{
	/* If the Host issues this command with a Connection_Handle that does
	 * not exist or is not for a CIS or a BIS, the Controller shall return
	 * the error code Unknown Connection Identifier (0x02).
	 */
	if (false) {

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	} else if (IS_CIS_HANDLE(handle)) {
		struct ll_conn_iso_stream *cis;
		struct ll_iso_stream_hdr *hdr;
		struct ll_iso_datapath *dp;

		cis = ll_conn_iso_stream_get(handle);
		hdr = &cis->hdr;

		if (path_dir & BIT(BT_HCI_DATAPATH_DIR_HOST_TO_CTLR)) {
			dp = hdr->datapath_in;
			if (dp) {
				isoal_source_destroy(dp->source_hdl);

				hdr->datapath_in = NULL;
				ull_iso_datapath_release(dp);
			} else {
				/* Datapath was not previously set up */
				return BT_HCI_ERR_CMD_DISALLOWED;
			}
		}

		if (path_dir & BIT(BT_HCI_DATAPATH_DIR_CTLR_TO_HOST)) {
			dp = hdr->datapath_out;
			if (dp) {
				isoal_sink_destroy(dp->sink_hdl);

				hdr->datapath_out = NULL;
				ull_iso_datapath_release(dp);
			} else {
				/* Datapath was not previously set up */
				return BT_HCI_ERR_CMD_DISALLOWED;
			}
		}
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	} else if (IS_ADV_ISO_HANDLE(handle)) {
		struct lll_adv_iso_stream *adv_stream;
		struct ll_iso_datapath *dp;
		uint16_t stream_handle;

		if (!(path_dir & BIT(BT_HCI_DATAPATH_DIR_HOST_TO_CTLR))) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		stream_handle = LL_BIS_ADV_IDX_FROM_HANDLE(handle);
		adv_stream = ull_adv_iso_stream_get(stream_handle);
		if (!adv_stream) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		dp = adv_stream->dp;
		if (dp) {
			adv_stream->dp = NULL;
			isoal_source_destroy(dp->source_hdl);
			ull_iso_datapath_release(dp);
		} else {
			/* Datapath was not previously set up */
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
#endif /* CONFIG_BT_CTLR_ADV_ISO */

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	} else if (IS_SYNC_ISO_HANDLE(handle)) {
		struct lll_sync_iso_stream *sync_stream;
		struct ll_iso_datapath *dp;
		uint16_t stream_handle;

		if (!(path_dir & BIT(BT_HCI_DATAPATH_DIR_CTLR_TO_HOST))) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		stream_handle = LL_BIS_SYNC_IDX_FROM_HANDLE(handle);
		sync_stream = ull_sync_iso_stream_get(stream_handle);
		if (!sync_stream) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		dp = sync_stream->dp;
		if (dp) {
			sync_stream->dp = NULL;
			isoal_sink_destroy(dp->sink_hdl);
			ull_iso_datapath_release(dp);
		} else {
			/* Datapath was not previously set up */
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
#endif /* CONFIG_BT_CTLR_SYNC_ISO */

	} else {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	return 0;
}

#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
/* The sdu_alloc function is called before combining PDUs into an SDU. Here we
 * store the paylaod number associated with the first PDU, for unframed usecase.
 */
static isoal_status_t ll_iso_test_sdu_alloc(const struct isoal_sink *sink_ctx,
					    const struct isoal_pdu_rx *valid_pdu,
					    struct isoal_sdu_buffer *sdu_buffer)
{
	uint16_t handle;

	handle = sink_ctx->session.handle;

	if (IS_CIS_HANDLE(handle)) {
		if (!sink_ctx->session.framed) {
			struct ll_conn_iso_stream *cis;

			cis = ll_iso_stream_connected_get(sink_ctx->session.handle);
			LL_ASSERT(cis);

			/* For unframed, SDU counter is the payload number */
			cis->hdr.test_mode.rx.sdu_counter =
				(uint32_t)valid_pdu->meta->payload_number;
		}
	} else if (IS_SYNC_ISO_HANDLE(handle)) {
		if (!sink_ctx->session.framed) {
			struct lll_sync_iso_stream *sync_stream;
			uint16_t stream_handle;

			stream_handle = LL_BIS_SYNC_IDX_FROM_HANDLE(handle);
			sync_stream = ull_sync_iso_stream_get(stream_handle);
			LL_ASSERT(sync_stream);

			sync_stream->test_mode->sdu_counter =
				(uint32_t)valid_pdu->meta->payload_number;
		}
	}

	return sink_sdu_alloc_hci(sink_ctx, valid_pdu, sdu_buffer);
}

/* The sdu_emit function is called whenever an SDU is combined and ready to be sent
 * further in the data path. This injected implementation performs statistics on
 * the SDU and then discards it.
 */
static isoal_status_t ll_iso_test_sdu_emit(const struct isoal_sink             *sink_ctx,
					   const struct isoal_emitted_sdu_frag *sdu_frag,
					   const struct isoal_emitted_sdu      *sdu)
{
	struct ll_iso_rx_test_mode *test_mode_rx;
	isoal_sdu_len_t length;
	isoal_status_t status;
	struct net_buf *buf;
	uint32_t sdu_counter;
	uint16_t max_sdu;
	uint16_t handle;
	uint8_t framed;

	handle = sink_ctx->session.handle;
	buf = (struct net_buf *)sdu_frag->sdu.contents.dbuf;

	if (IS_CIS_HANDLE(handle)) {
		struct ll_conn_iso_stream *cis;

		cis = ll_iso_stream_connected_get(sink_ctx->session.handle);
		LL_ASSERT(cis);

		test_mode_rx = &cis->hdr.test_mode.rx;
		max_sdu = cis->c_max_sdu;
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	} else if (IS_SYNC_ISO_HANDLE(handle)) {
		struct lll_sync_iso_stream *sync_stream;
		struct ll_sync_iso_set *sync_iso;
		uint16_t stream_handle;

		stream_handle = LL_BIS_SYNC_IDX_FROM_HANDLE(handle);
		sync_stream = ull_sync_iso_stream_get(stream_handle);
		LL_ASSERT(sync_stream);

		sync_iso = ull_sync_iso_by_stream_get(stream_handle);

		test_mode_rx = sync_stream->test_mode;
		max_sdu = sync_iso->lll.max_sdu;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
	} else {
		/* Handle is out of range */
		status = ISOAL_STATUS_ERR_SDU_EMIT;
		net_buf_unref(buf);

		return status;
	}

	length = sink_ctx->sdu_production.sdu_written;
	framed = sink_ctx->session.framed;

	/* In BT_HCI_ISO_TEST_ZERO_SIZE_SDU mode, all SDUs must have length 0 and there is
	 * no sdu_counter field. In the other modes, the first 4 bytes must contain a
	 * packet counter, which is used as SDU counter. The sdu_counter is extracted
	 * regardless of mode as a sanity check, unless the length does not allow it.
	 */
	if (length >= ISO_TEST_PACKET_COUNTER_SIZE) {
		sdu_counter = sys_get_le32(buf->data);
	} else {
		sdu_counter = 0U;
	}

	switch (sdu_frag->sdu.status) {
	case ISOAL_SDU_STATUS_VALID:
		if (framed && test_mode_rx->sdu_counter == 0U) {
			/* BT 5.3, Vol 6, Part B, section 7.2:
			 * When using framed PDUs the expected value of the SDU counter
			 * shall be initialized with the value of the SDU counter of the
			 * first valid received SDU.
			 */
			test_mode_rx->sdu_counter = sdu_counter;
		}

		switch (test_mode_rx->payload_type) {
		case BT_HCI_ISO_TEST_ZERO_SIZE_SDU:
			if (length == 0) {
				test_mode_rx->received_cnt++;
			} else {
				test_mode_rx->failed_cnt++;
			}
			break;

		case BT_HCI_ISO_TEST_VARIABLE_SIZE_SDU:
			if ((length >= ISO_TEST_PACKET_COUNTER_SIZE) &&
			    (length <= max_sdu) &&
			    (sdu_counter == test_mode_rx->sdu_counter)) {
				test_mode_rx->received_cnt++;
			} else {
				test_mode_rx->failed_cnt++;
			}
			break;

		case BT_HCI_ISO_TEST_MAX_SIZE_SDU:
			if ((length == max_sdu) &&
			    (sdu_counter == test_mode_rx->sdu_counter)) {
				test_mode_rx->received_cnt++;
			} else {
				test_mode_rx->failed_cnt++;
			}
			break;

		default:
			LL_ASSERT(0);
			return ISOAL_STATUS_ERR_SDU_EMIT;
		}
		break;

	case ISOAL_SDU_STATUS_ERRORS:
	case ISOAL_SDU_STATUS_LOST_DATA:
		test_mode_rx->missed_cnt++;
		break;
	}

	/* In framed mode, we may start incrementing the SDU counter when rx_sdu_counter
	 * becomes non zero (initial state), or in case of zero-based counting, if zero
	 * is actually the first valid SDU counter received.
	 */
	if (framed && (test_mode_rx->sdu_counter ||
			(sdu_frag->sdu.status == ISOAL_SDU_STATUS_VALID))) {
		test_mode_rx->sdu_counter++;
	}

	status = ISOAL_STATUS_OK;
	net_buf_unref(buf);

	return status;
}

uint8_t ll_iso_receive_test(uint16_t handle, uint8_t payload_type)
{
	struct ll_iso_rx_test_mode *test_mode_rx;
	isoal_sink_handle_t sink_handle;
	struct ll_iso_datapath *dp;
	uint32_t sdu_interval;
	isoal_status_t err;

	struct ll_iso_datapath **stream_dp;

	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	uint16_t stream_handle;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
	uint16_t iso_interval;
	uint8_t framed;
	uint8_t role;
	uint8_t ft;
	uint8_t bn;

	if (IS_CIS_HANDLE(handle)) {
		struct ll_conn_iso_stream *cis;
		struct ll_conn_iso_group *cig;

		cis = ll_iso_stream_connected_get(handle);
		if (!cis) {
			/* CIS is not connected */
			return BT_HCI_ERR_UNKNOWN_CONN_ID;
		}

		if (cis->lll.rx.bn == 0) {
			/* CIS is not configured for RX */
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}

		test_mode_rx = &cis->hdr.test_mode.rx;
		stream_dp = &cis->hdr.datapath_out;
		cig = cis->group;

		if (cig->lll.role == BT_HCI_ROLE_PERIPHERAL) {
			/* peripheral */
			sdu_interval = cig->c_sdu_interval;
		} else {
			/* central */
			sdu_interval = cig->p_sdu_interval;
		}

		role = cig->lll.role;
		framed = cis->framed;
		bn = cis->lll.rx.bn;
		ft = cis->lll.rx.ft;
		iso_interval = cig->iso_interval;
		stream_sync_delay = cis->sync_delay;
		group_sync_delay = cig->sync_delay;
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	} else if (IS_SYNC_ISO_HANDLE(handle)) {
		/* Get the sync stream from the handle */
		struct lll_sync_iso_stream *sync_stream;
		struct ll_sync_iso_set *sync_iso;
		struct lll_sync_iso *lll_iso;

		stream_handle = LL_BIS_SYNC_IDX_FROM_HANDLE(handle);
		sync_stream = ull_sync_iso_stream_get(stream_handle);
		if (!sync_stream) {
			return BT_HCI_ERR_UNKNOWN_CONN_ID;
		}

		if (sync_stream->dp) {
			/* Data path already set up */
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		sync_iso = ull_sync_iso_by_stream_get(stream_handle);
		lll_iso = &sync_iso->lll;

		test_mode_rx = sync_stream->test_mode;
		stream_dp = &sync_stream->dp;

		/* BT Core v5.4 - Vol 6, Part B, Section 4.4.6.4:
		 * BIG_Sync_Delay = (Num_BIS – 1) × BIS_Spacing
		 *			+ (NSE – 1) × Sub_Interval + MPT.
		 */
		group_sync_delay = ull_big_sync_delay(lll_iso);
		stream_sync_delay = group_sync_delay - stream_handle * lll_iso->bis_spacing;

		role = ISOAL_ROLE_BROADCAST_SINK;
		framed = 0; /* FIXME: Get value from biginfo */
		bn = lll_iso->bn;
		ft = 0;
		sdu_interval = lll_iso->sdu_interval;
		iso_interval = lll_iso->iso_interval;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
	} else {
		/* Handle is out of range */
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (*stream_dp) {
		/* Data path already set up */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (payload_type > BT_HCI_ISO_TEST_MAX_SIZE_SDU) {
		return BT_HCI_ERR_INVALID_LL_PARAM;
	}

	/* Allocate and configure test datapath */
	dp = mem_acquire(&datapath_free);
	if (!dp) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	dp->path_dir = BT_HCI_DATAPATH_DIR_CTLR_TO_HOST;
	dp->path_id  = BT_HCI_DATAPATH_ID_HCI;

	*stream_dp = dp;
	memset(test_mode_rx, 0, sizeof(struct ll_iso_rx_test_mode));

	err = isoal_sink_create(handle, role, framed, bn, ft,
				sdu_interval, iso_interval,
				stream_sync_delay, group_sync_delay,
				ll_iso_test_sdu_alloc,
				ll_iso_test_sdu_emit,
				sink_sdu_write_hci, &sink_handle);
	if (err) {
		/* Error creating test source - cleanup source and
		 * datapath
		 */
		isoal_sink_destroy(sink_handle);
		ull_iso_datapath_release(dp);
		*stream_dp = NULL;

		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	dp->sink_hdl = sink_handle;
	isoal_sink_enable(sink_handle);

	/* Enable Receive Test Mode */
	test_mode_rx->enabled = 1;
	test_mode_rx->payload_type = payload_type;

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ll_iso_read_test_counters(uint16_t handle, uint32_t *received_cnt,
				  uint32_t *missed_cnt,
				  uint32_t *failed_cnt)
{
	struct ll_iso_rx_test_mode *test_mode_rx;

	*received_cnt = 0U;
	*missed_cnt   = 0U;
	*failed_cnt   = 0U;

	if (IS_CIS_HANDLE(handle)) {
		struct ll_conn_iso_stream *cis;

		cis = ll_iso_stream_connected_get(handle);
		if (!cis) {
			/* CIS is not connected */
			return BT_HCI_ERR_UNKNOWN_CONN_ID;
		}

		test_mode_rx = &cis->hdr.test_mode.rx;

	} else if (IS_SYNC_ISO_HANDLE(handle)) {
		/* Get the sync stream from the handle */
		struct lll_sync_iso_stream *sync_stream;
		uint16_t stream_handle;

		stream_handle = LL_BIS_SYNC_IDX_FROM_HANDLE(handle);
		sync_stream = ull_sync_iso_stream_get(stream_handle);
		if (!sync_stream) {
			return BT_HCI_ERR_UNKNOWN_CONN_ID;
		}

		test_mode_rx = sync_stream->test_mode;

	} else {
		/* Handle is out of range */
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (!test_mode_rx->enabled) {
		/* ISO receive Test is not active */
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	/* Return SDU statistics */
	*received_cnt = test_mode_rx->received_cnt;
	*missed_cnt   = test_mode_rx->missed_cnt;
	*failed_cnt   = test_mode_rx->failed_cnt;

	return BT_HCI_ERR_SUCCESS;
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
	uint8_t status;

	*tx_unacked_packets = 0;
	*tx_flushed_packets = 0;
	*tx_last_subevent_packets = 0;
	*retransmitted_packets = 0;
	*crc_error_packets = 0;
	*rx_unreceived_packets = 0;
	*duplicate_packets = 0;

	status = BT_HCI_ERR_SUCCESS;

	if (IS_CIS_HANDLE(handle)) {
		struct ll_conn_iso_stream *cis;

		cis = ll_iso_stream_connected_get(handle);

		if (!cis) {
			/* CIS is not connected */
			return BT_HCI_ERR_UNKNOWN_CONN_ID;
		}

		*tx_unacked_packets       = cis->hdr.link_quality.tx_unacked_packets;
		*tx_flushed_packets       = cis->hdr.link_quality.tx_flushed_packets;
		*tx_last_subevent_packets = cis->hdr.link_quality.tx_last_subevent_packets;
		*retransmitted_packets    = cis->hdr.link_quality.retransmitted_packets;
		*crc_error_packets        = cis->hdr.link_quality.crc_error_packets;
		*rx_unreceived_packets    = cis->hdr.link_quality.rx_unreceived_packets;
		*duplicate_packets        = cis->hdr.link_quality.duplicate_packets;

	} else if (IS_SYNC_ISO_HANDLE(handle)) {
		/* FIXME: Implement for sync receiver */
		status = BT_HCI_ERR_CMD_DISALLOWED;
	} else {
		/* Handle is out of range */
		status = BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	return status;
}
#endif /* CONFIG_BT_CTLR_READ_ISO_LINK_QUALITY */

#endif /* CONFIG_BT_CTLR_SYNC_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
static isoal_status_t ll_iso_test_pdu_release(struct node_tx_iso *node_tx,
					      const uint16_t handle,
					      const isoal_status_t status)
{
	/* Release back to memory pool */
	if (node_tx->link) {
		ll_iso_link_tx_release(node_tx->link);
	}
	ll_iso_tx_mem_release(node_tx);

	return ISOAL_STATUS_OK;
}

#if defined(CONFIG_BT_CTLR_CONN_ISO)
void ll_iso_transmit_test_send_sdu(uint16_t handle, uint32_t ticks_at_expire)
{
	isoal_source_handle_t source_handle;
	struct isoal_sdu_tx sdu;
	isoal_status_t err;
	uint8_t tx_buffer[ISO_TEST_TX_BUFFER_SIZE];
	uint64_t next_payload_number;
	uint16_t remaining_tx;
	uint32_t sdu_counter;

	if (IS_CIS_HANDLE(handle)) {
		struct ll_conn_iso_stream *cis;
		struct ll_conn_iso_group *cig;
		uint32_t rand_max_sdu;
		uint8_t event_offset;
		uint8_t max_sdu;
		uint8_t rand_8;

		cis = ll_iso_stream_connected_get(handle);
		LL_ASSERT(cis);

		if (!cis->hdr.test_mode.tx.enabled) {
			/* Transmit Test Mode not enabled */
			return;
		}

		cig = cis->group;
		source_handle = cis->hdr.datapath_in->source_hdl;

		max_sdu = IS_PERIPHERAL(cig) ? cis->p_max_sdu : cis->c_max_sdu;

		switch (cis->hdr.test_mode.tx.payload_type) {
		case BT_HCI_ISO_TEST_ZERO_SIZE_SDU:
			remaining_tx = 0;
			break;

		case BT_HCI_ISO_TEST_VARIABLE_SIZE_SDU:
			/* Randomize the length [4..max_sdu] */
			lll_rand_get(&rand_8, sizeof(rand_8));
			rand_max_sdu = rand_8 * (max_sdu - ISO_TEST_PACKET_COUNTER_SIZE);
			remaining_tx = ISO_TEST_PACKET_COUNTER_SIZE + (rand_max_sdu >> 8);
			break;

		case BT_HCI_ISO_TEST_MAX_SIZE_SDU:
			LL_ASSERT(max_sdu > ISO_TEST_PACKET_COUNTER_SIZE);
			remaining_tx = max_sdu;
			break;

		default:
			LL_ASSERT(0);
			return;
		}

		if (remaining_tx > ISO_TEST_TX_BUFFER_SIZE) {
			sdu.sdu_state = BT_ISO_START;
		} else {
			sdu.sdu_state = BT_ISO_SINGLE;
		}

		/* Configure SDU similarly to one delivered via HCI */
		sdu.packet_sn = 0;
		sdu.dbuf = tx_buffer;

		/* We must ensure sufficient time for ISO-AL to fragment SDU and
		 * deliver PDUs to the TX queue. By checking ull_ref_get, we
		 * know if we are within the subevents of an ISO event. If so,
		 * we can assume that we have enough time to deliver in the next
		 * ISO event. If we're not active within the ISO event, we don't
		 * know if there is enough time to deliver in the next event,
		 * and for safety we set the target to current event + 2.
		 *
		 * For FT > 1, we have the opportunity to retransmit in later
		 * event(s), in which case we have the option to target an
		 * earlier event (this or next) because being late does not
		 * instantly flush the payload.
		 */
		event_offset = ull_ref_get(&cig->ull) ? 1 : 2;
		if (cis->lll.tx.ft > 1) {
			/* FT > 1, target an earlier event */
			event_offset -= 1;
		}

		sdu.grp_ref_point = isoal_get_wrapped_time_us(cig->cig_ref_point,
						(event_offset * cig->iso_interval *
							ISO_INT_UNIT_US));
		sdu.target_event = cis->lll.event_count + event_offset;
		sdu.iso_sdu_length = remaining_tx;

		/* Send all SDU fragments */
		do {
			sdu.cntr_time_stamp = HAL_TICKER_TICKS_TO_US(ticks_at_expire);
			sdu.time_stamp = sdu.cntr_time_stamp;
			sdu.size = MIN(remaining_tx, ISO_TEST_TX_BUFFER_SIZE);
			memset(tx_buffer, 0, sdu.size);

			/* If this is the first fragment of a framed SDU, inject the SDU
			 * counter.
			 */
			if ((sdu.size >= ISO_TEST_PACKET_COUNTER_SIZE) &&
			    ((sdu.sdu_state == BT_ISO_START) || (sdu.sdu_state == BT_ISO_SINGLE))) {
				if (cis->framed) {
					sdu_counter = (uint32_t)cis->hdr.test_mode.tx.sdu_counter;
				} else {
					/* Unframed. Get the next payload counter.
					 *
					 * BT 5.3, Vol 6, Part B, Section 7.1:
					 * When using unframed PDUs, the SDU counter shall be equal
					 * to the payload counter.
					 */
					isoal_tx_unframed_get_next_payload_number(source_handle,
									&sdu,
									&next_payload_number);
					sdu_counter = (uint32_t)next_payload_number;
				}

				sys_put_le32(sdu_counter, tx_buffer);
			}

			/* Send to ISOAL */
			err = isoal_tx_sdu_fragment(source_handle, &sdu);
			LL_ASSERT(!err);

			remaining_tx -= sdu.size;

			if (remaining_tx > ISO_TEST_TX_BUFFER_SIZE) {
				sdu.sdu_state = BT_ISO_CONT;
			} else {
				sdu.sdu_state = BT_ISO_END;
			}
		} while (remaining_tx);

		cis->hdr.test_mode.tx.sdu_counter++;

	} else if (IS_ADV_ISO_HANDLE(handle)) {
		/* FIXME: Implement for broadcaster */
	} else {
		LL_ASSERT(0);
	}
}
#endif /* CONFIG_BT_CTLR_CONN_ISO */

uint8_t ll_iso_transmit_test(uint16_t handle, uint8_t payload_type)
{
	isoal_source_handle_t source_handle;
	struct ll_iso_datapath *dp;
	uint32_t sdu_interval;
	isoal_status_t err;
	uint8_t status;

	status = BT_HCI_ERR_SUCCESS;

	if (IS_CIS_HANDLE(handle)) {
		struct ll_conn_iso_stream *cis;
		struct ll_conn_iso_group *cig;

		cis = ll_iso_stream_connected_get(handle);
		if (!cis) {
			/* CIS is not connected */
			return BT_HCI_ERR_UNKNOWN_CONN_ID;
		}

		if (cis->lll.tx.bn == 0U) {
			/* CIS is not configured for TX */
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}

		if (cis->hdr.datapath_in) {
			/* Data path already set up */
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		if (payload_type > BT_HCI_ISO_TEST_MAX_SIZE_SDU) {
			return BT_HCI_ERR_INVALID_LL_PARAM;
		}

		/* Allocate and configure test datapath */
		dp = mem_acquire(&datapath_free);
		if (!dp) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		dp->path_dir = BT_HCI_DATAPATH_DIR_HOST_TO_CTLR;
		dp->path_id  = BT_HCI_DATAPATH_ID_HCI;

		cis->hdr.datapath_in = dp;
		cig = cis->group;

		sdu_interval = IS_PERIPHERAL(cig) ? cig->p_sdu_interval : cig->c_sdu_interval;

		/* Setup the test source */
		err = isoal_source_create(handle, cig->lll.role, cis->framed,
					  cis->lll.tx.bn, cis->lll.tx.ft,
					  cis->lll.tx.max_pdu, sdu_interval,
					  cig->iso_interval, cis->sync_delay,
					  cig->sync_delay, ll_iso_pdu_alloc,
					  ll_iso_pdu_write, ll_iso_pdu_emit,
					  ll_iso_test_pdu_release,
					  &source_handle);

		if (err) {
			/* Error creating test source - cleanup source and datapath */
			isoal_source_destroy(source_handle);
			ull_iso_datapath_release(dp);
			cis->hdr.datapath_in = NULL;

			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		dp->source_hdl = source_handle;
		isoal_source_enable(source_handle);

		/* Enable Transmit Test Mode */
		cis->hdr.test_mode.tx.enabled = 1;
		cis->hdr.test_mode.tx.payload_type = payload_type;

	} else if (IS_ADV_ISO_HANDLE(handle)) {
		struct lll_adv_iso_stream *stream;
		uint16_t stream_handle;

		stream_handle = LL_BIS_ADV_IDX_FROM_HANDLE(handle);
		stream = ull_adv_iso_stream_get(stream_handle);
		if (!stream) {
			return BT_HCI_ERR_UNKNOWN_CONN_ID;
		}

		/* FIXME: Implement use of common header in stream to enable code sharing
		 * between CIS and BIS for test commands (and other places).
		 */
		status = BT_HCI_ERR_CMD_DISALLOWED;
	} else {
		/* Handle is out of range */
		status = BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	return status;
}
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

uint8_t ll_iso_test_end(uint16_t handle, uint32_t *received_cnt,
			uint32_t *missed_cnt, uint32_t *failed_cnt)
{
	*received_cnt = 0U;
	*missed_cnt   = 0U;
	*failed_cnt   = 0U;

	if (IS_CIS_HANDLE(handle)) {
		struct ll_conn_iso_stream *cis;

		cis = ll_iso_stream_connected_get(handle);
		if (!cis) {
			/* CIS is not connected */
			return BT_HCI_ERR_UNKNOWN_CONN_ID;
		}

		if (!cis->hdr.test_mode.rx.enabled && !cis->hdr.test_mode.tx.enabled) {
			/* Test Mode is not active */
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}

		if (cis->hdr.test_mode.rx.enabled) {
			isoal_sink_destroy(cis->hdr.datapath_out->sink_hdl);
			ull_iso_datapath_release(cis->hdr.datapath_out);
			cis->hdr.datapath_out = NULL;

			/* Return SDU statistics */
			*received_cnt = cis->hdr.test_mode.rx.received_cnt;
			*missed_cnt   = cis->hdr.test_mode.rx.missed_cnt;
			*failed_cnt   = cis->hdr.test_mode.rx.failed_cnt;
		}

		if (cis->hdr.test_mode.tx.enabled) {
			/* Tear down source and datapath */
			isoal_source_destroy(cis->hdr.datapath_in->source_hdl);
			ull_iso_datapath_release(cis->hdr.datapath_in);
			cis->hdr.datapath_in = NULL;
		}

		/* Disable Test Mode */
		(void)memset(&cis->hdr.test_mode, 0U, sizeof(cis->hdr.test_mode));

	} else if (IS_ADV_ISO_HANDLE(handle)) {
		/* FIXME: Implement for broadcaster */
		return BT_HCI_ERR_CMD_DISALLOWED;

	} else if (IS_SYNC_ISO_HANDLE(handle)) {
		struct lll_sync_iso_stream *sync_stream;
		uint16_t stream_handle;

		stream_handle = LL_BIS_SYNC_IDX_FROM_HANDLE(handle);
		sync_stream = ull_sync_iso_stream_get(stream_handle);
		if (!sync_stream) {
			return BT_HCI_ERR_UNKNOWN_CONN_ID;
		}

		if (!sync_stream->test_mode->enabled || !sync_stream->dp) {
			/* Test Mode is not active */
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}

		isoal_sink_destroy(sync_stream->dp->sink_hdl);
		ull_iso_datapath_release(sync_stream->dp);
		sync_stream->dp = NULL;

		/* Return SDU statistics */
		*received_cnt = sync_stream->test_mode->received_cnt;
		*missed_cnt   = sync_stream->test_mode->missed_cnt;
		*failed_cnt   = sync_stream->test_mode->failed_cnt;

		(void)memset(&sync_stream->test_mode, 0U, sizeof(sync_stream->test_mode));

	} else {
		/* Handle is out of range */
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	return BT_HCI_ERR_SUCCESS;
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

int ll_iso_tx_mem_enqueue(uint16_t handle, void *node_tx, void *link)
{
	if (IS_ENABLED(CONFIG_BT_CTLR_CONN_ISO) &&
	    IS_CIS_HANDLE(handle)) {
		struct ll_conn_iso_stream *cis;

		cis = ll_conn_iso_stream_get(handle);
		memq_enqueue(link, node_tx, &cis->lll.memq_tx.tail);

	} else if (IS_ENABLED(CONFIG_BT_CTLR_ADV_ISO) &&
		   IS_ADV_ISO_HANDLE(handle)) {
		struct lll_adv_iso_stream *stream;
		uint16_t stream_handle;

		stream_handle = LL_BIS_ADV_IDX_FROM_HANDLE(handle);
		stream = ull_adv_iso_stream_get(stream_handle);
		memq_enqueue(link, node_tx, &stream->memq_tx.tail);

	} else {
		return -EINVAL;
	}

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

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
void ull_iso_lll_ack_enqueue(uint16_t handle, struct node_tx_iso *node_tx)
{
	struct ll_iso_datapath *dp = NULL;

	if (IS_ENABLED(CONFIG_BT_CTLR_CONN_ISO) && IS_CIS_HANDLE(handle)) {
		struct ll_conn_iso_stream *cis;

		cis = ll_conn_iso_stream_get(handle);
		dp  = cis->hdr.datapath_in;

		if (dp) {
			isoal_tx_pdu_release(dp->source_hdl, node_tx);
		} else {
			/* Race with Data Path remove */
			/* FIXME: ll_tx_ack_put is not LLL callable as it is
			 * used by ACL connections in ULL context to dispatch
			 * ack.
			 */
			ll_tx_ack_put(handle, (void *)node_tx);
			ll_rx_sched();
		}
	} else if (IS_ENABLED(CONFIG_BT_CTLR_ADV_ISO) && IS_ADV_ISO_HANDLE(handle)) {
		/* Process as TX ack. TODO: Can be unified with CIS and use
		 * ISOAL.
		 */
		/* FIXME: ll_tx_ack_put is not LLL callable as it is
		 * used by ACL connections in ULL context to dispatch
		 * ack.
		 */
		ll_tx_ack_put(handle, (void *)node_tx);
		ll_rx_sched();
	} else {
		LL_ASSERT(0);
	}
}

void ull_iso_lll_event_prepare(uint16_t handle, uint64_t event_count)
{
	if (IS_CIS_HANDLE(handle)) {
		struct ll_iso_datapath *dp = NULL;
		struct ll_conn_iso_stream *cis;

		cis = ll_iso_stream_connected_get(handle);

		if (cis) {
			dp  = cis->hdr.datapath_in;
		}

		if (dp) {
			isoal_tx_event_prepare(dp->source_hdl, event_count);
		}
	} else if (IS_ADV_ISO_HANDLE(handle)) {
		/* Send event deadline trigger to ISO-AL.
		 * TODO: Can be unified with CIS implementation.
		 */
	} else {
		LL_ASSERT(0);
	}
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

#if defined(CONFIG_BT_CTLR_CONN_ISO)
static void iso_rx_cig_ref_point_update(struct ll_conn_iso_group *cig,
					const struct ll_conn_iso_stream *cis,
					const struct node_rx_iso_meta  *meta)
{
	uint32_t cig_sync_delay;
	uint32_t cis_sync_delay;
	uint64_t event_count;
	uint8_t burst_number;
	uint8_t role;

	role = cig->lll.role;
	cig_sync_delay = cig->sync_delay;
	cis_sync_delay = cis->sync_delay;
	burst_number = cis->lll.rx.bn;
	event_count = cis->lll.event_count;

	if (role) {
		/* Peripheral */

		/* Check if this is the first payload received for this cis in
		 * this event
		 */
		if (meta->payload_number == (burst_number * event_count)) {
			/* Update the CIG reference point based on the CIS
			 * anchor point
			 */
			cig->cig_ref_point = isoal_get_wrapped_time_us(meta->timestamp,
						cis_sync_delay - cig_sync_delay);
		}
	}
}
#endif /* CONFIG_BT_CTLR_CONN_ISO */

static void iso_rx_demux(void *param)
{
#if defined(CONFIG_BT_CTLR_CONN_ISO) || \
	defined(CONFIG_BT_CTLR_SYNC_ISO)
	struct ll_iso_datapath *dp;
#endif  /* CONFIG_BT_CTLR_CONN_ISO || CONFIG_BT_CTLR_SYNC_ISO */
	struct node_rx_pdu *rx_pdu;
	struct node_rx_hdr *rx;
	memq_link_t *link;
	uint16_t handle;

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

				rx_pdu = (struct node_rx_pdu *)rx;
				handle = rx_pdu->hdr.handle;
				dp = NULL;

				if (false) {
#if defined(CONFIG_BT_CTLR_CONN_ISO)
				} else if (IS_CIS_HANDLE(handle)) {
					struct ll_conn_iso_stream *cis;
					struct ll_conn_iso_group *cig;

					cis = ll_conn_iso_stream_get(handle);
					cig = cis->group;
					dp  = cis->hdr.datapath_out;

					iso_rx_cig_ref_point_update(cig, cis,
								    &rx_pdu->hdr.rx_iso_meta);
#endif /* CONFIG_BT_CTLR_CONN_ISO */
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
				} else if (IS_SYNC_ISO_HANDLE(handle)) {
					struct lll_sync_iso_stream *sync_stream;
					uint16_t stream_handle;

					stream_handle = LL_BIS_SYNC_IDX_FROM_HANDLE(handle);
					sync_stream = ull_sync_iso_stream_get(stream_handle);
					dp = sync_stream ? sync_stream->dp : NULL;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
				}

#if defined(CONFIG_BT_CTLR_CONN_ISO) || defined(CONFIG_BT_CTLR_SYNC_ISO)
				if (dp && dp->path_id != BT_HCI_DATAPATH_ID_HCI) {
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
						isoal_rx_pdu_recombine(dp->sink_hdl, &pckt_meta);

					LL_ASSERT(err == ISOAL_STATUS_OK); /* TODO handle err */
				}
#endif /* CONFIG_BT_CTLR_CONN_ISO || CONFIG_BT_CTLR_SYNC_ISO */

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
			mem_release(rx, &mem_pool_iso_rx.free);
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
			mem_release(rx_free, &mem_pool_iso_rx.free);
			break;
		default:
			/* Ignore other types as node may have been initialized due to
			 * race with HCI reset.
			 */
			break;
		}
	}

	*node_rx = rx;

	RXFIFO_ALLOC(iso_rx, UINT8_MAX);
}
#endif /* CONFIG_BT_CTLR_SYNC_ISO) || CONFIG_BT_CTLR_CONN_ISO */

struct ll_iso_datapath *ull_iso_datapath_alloc(void)
{
	return mem_acquire(&datapath_free);
}

void ull_iso_datapath_release(struct ll_iso_datapath *dp)
{
	mem_release(dp, &datapath_free);
}

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
void ll_iso_link_tx_release(void *link)
{
	mem_release(link, &mem_link_iso_tx.free);
}

/**
 * Allocate a PDU from the LL and store the details in the given buffer. Allocation
 * is not expected to fail as there must always be sufficient PDU buffers. Any
 * failure will trigger the assert.
 * @param[in]  pdu_buffer Buffer to store PDU details in
 * @return     Error status of operation
 */
static isoal_status_t ll_iso_pdu_alloc(struct isoal_pdu_buffer *pdu_buffer)
{
	struct node_tx_iso *node_tx;

	node_tx = ll_iso_tx_mem_acquire();
	if (!node_tx) {
		LOG_ERR("Tx Buffer Overflow");
		/* TODO: Report overflow to HCI and remove assert
		 * data_buf_overflow(evt, BT_OVERFLOW_LINK_ISO)
		 */
		LL_ASSERT(0);
		return ISOAL_STATUS_ERR_PDU_ALLOC;
	}

	node_tx->link = NULL;

	/* node_tx handle will be required to emit the PDU later */
	pdu_buffer->handle = (void *)node_tx;
	pdu_buffer->pdu    = (void *)node_tx->pdu;

	/* Use TX buffer size as the limit here. Actual size will be decided in
	 * the ISOAL based on the minimum of the buffer size and the respective
	 * Max_PDU_C_To_P or Max_PDU_P_To_C.
	 */
	pdu_buffer->size = MAX(LL_BIS_OCTETS_TX_MAX, LL_CIS_OCTETS_TX_MAX);

	return ISOAL_STATUS_OK;
}

/**
 * Write the given SDU payload to the target PDU buffer at the given offset.
 * @param[in,out]  pdu_buffer  Target PDU buffer
 * @param[in]      pdu_offset  Offset / current write position within PDU
 * @param[in]      sdu_payload Location of source data
 * @param[in]      consume_len Length of data to copy
 * @return         Error status of write operation
 */
static isoal_status_t ll_iso_pdu_write(struct isoal_pdu_buffer *pdu_buffer,
				       const size_t  pdu_offset,
				       const uint8_t *sdu_payload,
				       const size_t  consume_len)
{
	ARG_UNUSED(pdu_offset);
	ARG_UNUSED(consume_len);

	LL_ASSERT(pdu_buffer);
	LL_ASSERT(pdu_buffer->pdu);
	LL_ASSERT(sdu_payload);

	if ((pdu_offset + consume_len) > pdu_buffer->size) {
		/* Exceeded PDU buffer */
		return ISOAL_STATUS_ERR_UNSPECIFIED;
	}

	/* Copy source to destination at given offset */
	memcpy(&pdu_buffer->pdu->payload[pdu_offset], sdu_payload, consume_len);

	return ISOAL_STATUS_OK;
}

/**
 * Emit the encoded node to the transmission queue
 * @param node_tx TX node to enqueue
 * @param handle  CIS/BIS handle
 * @return        Error status of enqueue operation
 */
static isoal_status_t ll_iso_pdu_emit(struct node_tx_iso *node_tx,
				      const uint16_t handle)
{
	memq_link_t *link;

	link = mem_acquire(&mem_link_iso_tx.free);
	LL_ASSERT(link);

	if (ll_iso_tx_mem_enqueue(handle, node_tx, link)) {
		return ISOAL_STATUS_ERR_PDU_EMIT;
	}

	return ISOAL_STATUS_OK;
}

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
/**
 * Release the given payload back to the memory pool.
 * @param node_tx TX node to release or forward
 * @param handle  CIS/BIS handle
 * @param status  Reason for release
 * @return        Error status of release operation
 */
static isoal_status_t ll_iso_pdu_release(struct node_tx_iso *node_tx,
					 const uint16_t handle,
					 const isoal_status_t status)
{
	if (status == ISOAL_STATUS_OK) {
		/* Process as TX ack, we are in LLL execution context here.
		 * status == ISOAL_STATUS_OK when an ISO PDU has been acked.
		 *
		 * Call Path:
		 *   ull_iso_lll_ack_enqueue() --> isoal_tx_pdu_release() -->
		 *   pdu_release() == ll_iso_pdu_release() (this function).
		 */
		/* FIXME: ll_tx_ack_put is not LLL callable as it is used by
		 * ACL connections in ULL context to dispatch ack.
		 */
		ll_tx_ack_put(handle, (void *)node_tx);
		ll_rx_sched();
	} else {
		/* Release back to memory pool, we are in Thread context
		 * Callers:
		 *   isoal_source_deallocate() with ISOAL_STATUS_ERR_PDU_EMIT
		 *   isoal_tx_pdu_emit with status != ISOAL_STATUS_OK
		 */
		if (node_tx->link) {
			ll_iso_link_tx_release(node_tx->link);
		}
		ll_iso_tx_mem_release(node_tx);
	}

	return ISOAL_STATUS_OK;
}
#endif /* CONFIG_BT_CTLR_CONN_ISO */
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

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
	mem_init(mem_iso_tx.pool, NODE_TX_BUFFER_SIZE, BT_CTLR_ISO_TX_BUFFERS,
		 &mem_iso_tx.free);

	/* Initialize tx link pool. */
	mem_init(mem_link_iso_tx.pool, sizeof(memq_link_t),
		 BT_CTLR_ISO_TX_BUFFERS, &mem_link_iso_tx.free);
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if BT_CTLR_ISO_STREAMS
	/* Initialize ISO Datapath pool */
	mem_init(datapath_pool, sizeof(struct ll_iso_datapath),
		 sizeof(datapath_pool) / sizeof(struct ll_iso_datapath), &datapath_free);
#endif /* BT_CTLR_ISO_STREAMS */

	/* Initialize ISO Adaptation Layer */
	isoal_init();

	return 0;
}
