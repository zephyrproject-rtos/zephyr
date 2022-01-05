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

#include "pdu.h"

#include "lll.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "isoal.h"

#include "ull_sync_types.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_internal.h"
#include "ull_iso_internal.h"

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
#if BT_CTLR_ISO_STREAMS
static struct ll_iso_datapath datapath_pool[BT_CTLR_ISO_STREAMS];
#endif
static void *datapath_free;

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
static MFIFO_DEFINE(iso_tx, sizeof(struct lll_tx),
		    CONFIG_BT_CTLR_ISO_TX_BUFFERS);

static struct {
	void *free;
	uint8_t pool[CONFIG_BT_CTLR_ISO_TX_BUFFER_SIZE *
			CONFIG_BT_CTLR_ISO_TX_BUFFERS];
} mem_iso_tx;
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

	isoal_sink_handle_t sink_handle;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	uint8_t flush_timeout;
	uint16_t iso_interval;
	uint32_t sdu_interval;
	uint8_t  burst_number;
	isoal_status_t err;
	uint8_t role;

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
	if (stream->dp) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
#endif /* CONFIG_BT_CTLR_CONN_ISO */

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

	return 0;
}

uint8_t ll_remove_iso_path(uint16_t handle, uint8_t path_dir)
{
	struct ll_iso_datapath *dp;

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
	dp = stream->dp;
	if (dp) {
		stream->dp = NULL;
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
	struct lll_tx *tx;
	uint8_t idx;

	idx = MFIFO_ENQUEUE_GET(iso_tx, (void **) &tx);
	if (!tx) {
		return -ENOBUFS;
	}

	tx->handle = handle;
	tx->node = node_tx;

	MFIFO_ENQUEUE(iso_tx, idx);

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
	/* Re-initialize the Tx mfifo */
	MFIFO_INIT(iso_tx);
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void ull_iso_datapath_release(struct ll_iso_datapath *dp)
{
	mem_release(dp, &datapath_free);
}

static int init_reset(void)
{
#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	/* Initialize tx pool. */
	mem_init(mem_iso_tx.pool, CONFIG_BT_CTLR_ISO_TX_BUFFER_SIZE,
		 CONFIG_BT_CTLR_ISO_TX_BUFFERS, &mem_iso_tx.free);
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if BT_CTLR_ISO_STREAMS
	/* Initialize ISO Datapath pool */
	mem_init(datapath_pool, sizeof(struct ll_iso_datapath),
		 sizeof(datapath_pool) / sizeof(struct ll_iso_datapath), &datapath_free);
#endif

	return 0;
}
