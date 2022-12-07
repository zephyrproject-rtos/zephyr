/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <stdlib.h>

#include <zephyr/types.h>
#include <sys/types.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "util/memq.h"

#include "hal/ccm.h"
#include "hal/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "ll.h"
#include "lll.h"
#include "lll_conn_iso.h"
#include "lll_iso_tx.h"
#include "isoal.h"
#include "ull_iso_types.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_ctlr_isoal, LOG_LEVEL_DBG);

#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
/* Given the minimum payload, this defines the minimum number of bytes that
 * should be  remaining in a TX PDU such that it would make inserting a new
 * segment worthwhile during the segmentation process.
 * [Payload (min) + Segmentation Header + Time Offset]
 */
#define ISOAL_TX_SEGMENT_MIN_SIZE         (CONFIG_BT_CTLR_ISO_TX_SEG_PLAYLOAD_MIN +                \
					   PDU_ISO_SEG_HDR_SIZE +                                  \
					   PDU_ISO_SEG_TIMEOFFSET_SIZE)
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

/* Defined the wrapping point and mid point in the range of time input values,
 * which depend on range of the controller's clock in microseconds.
 */
#define ISOAL_TIME_WRAPPING_POINT_US      (HAL_TICKER_TICKS_TO_US(HAL_TICKER_CNTR_MASK))
#define ISOAL_TIME_MID_POINT_US           (ISOAL_TIME_WRAPPING_POINT_US / 2)
#define ISOAL_TIME_SPAN_FULL_US           (ISOAL_TIME_WRAPPING_POINT_US + 1)
#define ISOAL_TIME_SPAN_HALF_US           (ISOAL_TIME_SPAN_FULL_US / 2)

/** Allocation state */
typedef uint8_t isoal_alloc_state_t;
#define ISOAL_ALLOC_STATE_FREE            ((isoal_alloc_state_t) 0x00)
#define ISOAL_ALLOC_STATE_TAKEN           ((isoal_alloc_state_t) 0x01)

struct
{
#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	isoal_alloc_state_t sink_allocated[CONFIG_BT_CTLR_ISOAL_SINKS];
	struct isoal_sink   sink_state[CONFIG_BT_CTLR_ISOAL_SINKS];
#endif /* CONFIG_BT_CTLR_SYNC_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	isoal_alloc_state_t source_allocated[CONFIG_BT_CTLR_ISOAL_SOURCES];
	struct isoal_source source_state[CONFIG_BT_CTLR_ISOAL_SOURCES];
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */
} isoal_global;

/**
 * @brief Internal reset
 * Zero-init entire ISO-AL state
 */
static isoal_status_t isoal_init_reset(void)
{
	memset(&isoal_global, 0, sizeof(isoal_global));
	return ISOAL_STATUS_OK;
}

/**
 * @brief  Initialize ISO-AL
 */
isoal_status_t isoal_init(void)
{
	isoal_status_t err = ISOAL_STATUS_OK;

	err = isoal_init_reset();

	return err;
}

/** Clean up and reinitialize */
isoal_status_t isoal_reset(void)
{
	isoal_status_t err = ISOAL_STATUS_OK;

	err = isoal_init_reset();

	return err;
}

/**
 * @brief Wraps given time within the range of 0 to ISOAL_TIME_WRAPPING_POINT_US
 * @param  time_now  Current time value
 * @param  time_diff Time difference (signed)
 * @return           Wrapped time after difference
 */
static uint32_t isoal_get_wrapped_time_us(uint32_t time_now_us, int32_t time_diff_us)
{
	LL_ASSERT(time_now_us <= ISOAL_TIME_WRAPPING_POINT_US);

	uint32_t result = ((uint64_t)time_now_us + ISOAL_TIME_SPAN_FULL_US + time_diff_us) %
				((uint64_t)ISOAL_TIME_SPAN_FULL_US);

	return result;
}

#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
/**
 * @brief Find free sink from statically-sized pool and allocate it
 * @details Implemented as linear search since pool is very small
 *
 * @param hdl[out]  Handle to sink
 * @return ISOAL_STATUS_OK if we could allocate; otherwise ISOAL_STATUS_ERR_SINK_ALLOC
 */
static isoal_status_t isoal_sink_allocate(isoal_sink_handle_t *hdl)
{
	isoal_sink_handle_t i;

	/* Very small linear search to find first free */
	for (i = 0; i < CONFIG_BT_CTLR_ISOAL_SINKS; i++) {
		if (isoal_global.sink_allocated[i] == ISOAL_ALLOC_STATE_FREE) {
			isoal_global.sink_allocated[i] = ISOAL_ALLOC_STATE_TAKEN;
			*hdl = i;
			return ISOAL_STATUS_OK;
		}
	}

	return ISOAL_STATUS_ERR_SINK_ALLOC; /* All entries were taken */
}

/**
 * @brief Mark a sink as being free to allocate again
 * @param hdl[in]  Handle to sink
 */
static void isoal_sink_deallocate(isoal_sink_handle_t hdl)
{
	isoal_global.sink_allocated[hdl] = ISOAL_ALLOC_STATE_FREE;
	(void)memset(&isoal_global.sink_state[hdl], 0, sizeof(struct isoal_sink));
}

/**
 * @brief Create a new sink
 *
 * @param handle[in]            Connection handle
 * @param role[in]              Peripheral, Central or Broadcast
 * @param framed[in]            Framed case
 * @param burst_number[in]      Burst Number
 * @param flush_timeout[in]     Flush timeout
 * @param sdu_interval[in]      SDU interval
 * @param iso_interval[in]      ISO interval
 * @param stream_sync_delay[in] CIS / BIS sync delay
 * @param group_sync_delay[in]  CIG / BIG sync delay
 * @param sdu_alloc[in]         Callback of SDU allocator
 * @param sdu_emit[in]          Callback of SDU emitter
 * @param sdu_write[in]         Callback of SDU byte writer
 * @param hdl[out]              Handle to new sink
 *
 * @return ISOAL_STATUS_OK if we could create a new sink; otherwise ISOAL_STATUS_ERR_SINK_ALLOC
 */
isoal_status_t isoal_sink_create(
	uint16_t                 handle,
	uint8_t                  role,
	uint8_t                  framed,
	uint8_t                  burst_number,
	uint8_t                  flush_timeout,
	uint32_t                 sdu_interval,
	uint16_t                 iso_interval,
	uint32_t                 stream_sync_delay,
	uint32_t                 group_sync_delay,
	isoal_sink_sdu_alloc_cb  sdu_alloc,
	isoal_sink_sdu_emit_cb   sdu_emit,
	isoal_sink_sdu_write_cb  sdu_write,
	isoal_sink_handle_t      *hdl)
{
	uint32_t iso_interval_us;
	isoal_status_t err;

	/* ISO interval in units of time requires the integer (iso_interval)
	 * to be multiplied by 1250us.
	 */
	iso_interval_us = iso_interval * ISO_INT_UNIT_US;

	/* Allocate a new sink */
	err = isoal_sink_allocate(hdl);
	if (err) {
		return err;
	}

	struct isoal_sink_session *session = &isoal_global.sink_state[*hdl].session;

	session->handle = handle;
	session->framed = framed;
	session->sdu_interval = sdu_interval;
	session->burst_number = burst_number;

	/* Todo: Next section computing various constants, should potentially be a
	 * function in itself as a number of the dependencies could be changed while
	 * a connection is active.
	 */

	session->pdus_per_sdu = (burst_number * sdu_interval) /
		iso_interval_us;

	/* Computation of transport latency (constant part)
	 *
	 * Unframed case:
	 *
	 * C->P: SDU_Synchronization_Reference =
	 *   CIS reference anchor point + CIS_Sync_Delay + (FT_C_To_P - 1) * ISO_Interval
	 *
	 * P->C: SDU_Synchronization_Reference =
	 *   CIS reference anchor point + CIS_Sync_Delay - CIG_Sync_Delay -
	 *   ((ISO_Interval / SDU interval)-1) * SDU interval
	 *
	 * BIS: SDU_Synchronization_Reference =
	 *    BIG reference anchor point + BIG_Sync_Delay
	 *
	 * Framed case:
	 *
	 * C->P: SDU_Synchronization_Reference =
	 *   CIS Reference Anchor point +
	 *   CIS_Sync_Delay + SDU_Interval_C_To_P + FT_C_To_P * ISO_Interval -
	 *   Time_Offset
	 *
	 * P->C: synchronization reference SDU = CIS reference anchor point +
	 *   CIS_Sync_Delay - CIG_Sync_Delay - Time_Offset
	 *
	 * BIS: SDU_Synchronization_Reference =
	 *   BIG reference anchor point +
	 *   BIG_Sync_Delay + SDU_interval + ISO_Interval - Time_Offset.
	 */
	if (role == BT_CONN_ROLE_PERIPHERAL) {
		isoal_global.sink_state[*hdl].session.latency_unframed =
			stream_sync_delay + ((flush_timeout - 1) * iso_interval_us);

		isoal_global.sink_state[*hdl].session.latency_framed =
			stream_sync_delay + sdu_interval + (flush_timeout * iso_interval_us);
	} else if (role == BT_CONN_ROLE_CENTRAL) {
		isoal_global.sink_state[*hdl].session.latency_unframed =
			stream_sync_delay - group_sync_delay -
			(((iso_interval_us / sdu_interval) - 1) * iso_interval_us);

		isoal_global.sink_state[*hdl].session.latency_framed =
			stream_sync_delay - group_sync_delay;
	} else if (role == BT_ROLE_BROADCAST) {
		isoal_global.sink_state[*hdl].session.latency_unframed =
			group_sync_delay;

		isoal_global.sink_state[*hdl].session.latency_framed =
			group_sync_delay + sdu_interval + iso_interval_us;

	} else {
		LL_ASSERT(0);
	}

	/* Remember the platform-specific callbacks */
	session->sdu_alloc = sdu_alloc;
	session->sdu_emit  = sdu_emit;
	session->sdu_write = sdu_write;

	/* Initialize running seq number to zero */
	session->seqn = 0;

	return err;
}

/**
 * @brief Get reference to configuration struct
 *
 * @param hdl[in]   Handle to new sink
 * @return Reference to parameter struct, to be configured by caller
 */
struct isoal_sink_config *isoal_get_sink_param_ref(isoal_sink_handle_t hdl)
{
	LL_ASSERT(hdl < CONFIG_BT_CTLR_ISOAL_SINKS);
	LL_ASSERT(isoal_global.sink_allocated[hdl] == ISOAL_ALLOC_STATE_TAKEN);

	return &isoal_global.sink_state[hdl].session.param;
}

/**
 * @brief Atomically enable latch-in of packets and SDU production
 * @param hdl[in]  Handle of existing instance
 */
void isoal_sink_enable(isoal_sink_handle_t hdl)
{
	/* Reset bookkeeping state */
	memset(&isoal_global.sink_state[hdl].sdu_production, 0,
	       sizeof(isoal_global.sink_state[hdl].sdu_production));

	/* Atomically enable */
	isoal_global.sink_state[hdl].sdu_production.mode = ISOAL_PRODUCTION_MODE_ENABLED;
}

/**
 * @brief Atomically disable latch-in of packets and SDU production
 * @param hdl[in]  Handle of existing instance
 */
void isoal_sink_disable(isoal_sink_handle_t hdl)
{
	/* Atomically disable */
	isoal_global.sink_state[hdl].sdu_production.mode = ISOAL_PRODUCTION_MODE_DISABLED;
}

/**
 * @brief Disable and deallocate existing sink
 * @param hdl[in]  Handle of existing instance
 */
void isoal_sink_destroy(isoal_sink_handle_t hdl)
{
	/* Atomic disable */
	isoal_sink_disable(hdl);

	/* Permit allocation anew */
	isoal_sink_deallocate(hdl);
}

/* Obtain destination SDU */
static isoal_status_t isoal_rx_allocate_sdu(struct isoal_sink *sink,
					    const struct isoal_pdu_rx *pdu_meta)
{
	struct isoal_sink_session *session;
	struct isoal_sdu_production *sp;
	struct isoal_sdu_produced *sdu;
	isoal_status_t err;

	err = ISOAL_STATUS_OK;
	session = &sink->session;
	sp = &sink->sdu_production;
	sdu = &sp->sdu;

	/* Allocate a SDU if the previous was filled (thus sent) */
	const bool sdu_complete = (sp->sdu_available == 0);

	if (sdu_complete) {
		/* Allocate new clean SDU buffer */
		err = session->sdu_alloc(
			sink,
			pdu_meta,      /* [in]  PDU origin may determine buffer */
			&sdu->contents  /* [out] Updated with pointer and size */
		);

		if (err == ISOAL_STATUS_OK) {
			sp->sdu_allocated = true;
		}

		/* Nothing has been written into buffer yet */
		sp->sdu_written   = 0;
		sp->sdu_available = sdu->contents.size;
		LL_ASSERT(sdu->contents.size > 0);

		/* Get seq number from session counter */
		sdu->seqn = session->seqn;
	}

	return err;
}

/**
 * @brief  Depending of whether the configuration is enabled, this will either
 *         buffer and collate information for the SDU across all fragments
 *         before emitting the batch of fragments, or immediately release the
 *         fragment.
 * @param  sink       Point to the sink context structure
 * @param  end_of_sdu Indicates if this is the end fragment of an SDU or forced
 *                    release on an error
 * @return            Status of operation
 */
static isoal_status_t isoal_rx_buffered_emit_sdu(struct isoal_sink *sink, bool end_of_sdu)
{
	struct isoal_emitted_sdu_frag sdu_frag;
	struct isoal_emitted_sdu sdu_status;
	struct isoal_sink_session *session;
	struct isoal_sdu_production *sp;
	struct isoal_sdu_produced *sdu;
	bool emit_sdu_current;
	isoal_status_t err;

	err = ISOAL_STATUS_OK;
	session = &sink->session;
	sp = &sink->sdu_production;
	sdu = &sp->sdu;

	/* Initialize current SDU fragment buffer */
	sdu_frag.sdu_state = sp->sdu_state;
	sdu_frag.sdu_frag_size = sp->sdu_written;
	sdu_frag.sdu = *sdu;

	sdu_status.total_sdu_size = sdu_frag.sdu_frag_size;
	sdu_status.collated_status = sdu_frag.sdu.status;
	emit_sdu_current = true;

#if defined(ISOAL_BUFFER_RX_SDUS_ENABLE)
	uint16_t next_write_indx;
	bool sdu_list_empty;
	bool emit_sdu_list;
	bool sdu_list_max;
	bool sdu_list_err;

	next_write_indx = sp->sdu_list.next_write_indx;
	sdu_list_max = (next_write_indx >= CONFIG_BT_CTLR_ISO_RX_SDU_BUFFERS);
	sdu_list_empty = (next_write_indx == 0);

	/* There is an error in the sequence of SDUs if the current SDU fragment
	 * is not an end fragment and either the list at capacity or the current
	 * fragment is not a continuation (i.e. it is a start of a new SDU).
	 */
	sdu_list_err = !end_of_sdu &&
		(sdu_list_max ||
		(!sdu_list_empty && sdu_frag.sdu_state != BT_ISO_CONT));

	/* Release the current fragment if it is the end of the SDU or if it is
	 * not the starting fragment of a multi-fragment SDU.
	 */
	emit_sdu_current = end_of_sdu || (sdu_list_empty && sdu_frag.sdu_state != BT_ISO_START);

	/* Flush the buffered SDUs if this is an end fragment either on account
	 * of reaching the end of the SDU or on account of an error or if
	 * there is an error in the sequence of buffered fragments.
	 */
	emit_sdu_list = emit_sdu_current || sdu_list_err;

	/* Total size is cleared if the current fragment is not being emitted
	 * or if there is an error in the sequence of fragments.
	 */
	if (!emit_sdu_current || sdu_list_err) {
		sdu_status.total_sdu_size = 0;
		sdu_status.collated_status = (sdu_list_err ? ISOAL_SDU_STATUS_LOST_DATA :
						ISOAL_SDU_STATUS_VALID);
	}

	if (emit_sdu_list && next_write_indx > 0) {
		if (!sdu_list_err) {
			/* Collated information is not reliable if there is an
			 * error in the sequence of the fragments.
			 */
			for (uint8_t i = 0; i < next_write_indx; i++) {
				sdu_status.total_sdu_size +=
					sp->sdu_list.list[i].sdu_frag_size;
				if (sp->sdu_list.list[i].sdu.status == ISOAL_SDU_STATUS_LOST_DATA ||
					sdu_status.collated_status == ISOAL_SDU_STATUS_LOST_DATA) {
					sdu_status.collated_status = ISOAL_SDU_STATUS_LOST_DATA;
				} else {
					sdu_status.collated_status |=
						sp->sdu_list.list[i].sdu.status;
				}
			}
		}

		for (uint8_t i = 0; i < next_write_indx; i++) {
			err |= session->sdu_emit(sink, &sp->sdu_list.list[i],
						&sdu_status);
		}

		next_write_indx = sp->sdu_list.next_write_indx  = 0;
	}
#endif /* ISOAL_BUFFER_RX_SDUS_ENABLE */

	if (emit_sdu_current) {
		if (sdu_frag.sdu_state == BT_ISO_SINGLE) {
			sdu_status.total_sdu_size = sdu_frag.sdu_frag_size;
			sdu_status.collated_status = sdu_frag.sdu.status;
		}

		err |= session->sdu_emit(sink, &sdu_frag, &sdu_status);

#if defined(ISOAL_BUFFER_RX_SDUS_ENABLE)
	} else if (next_write_indx < CONFIG_BT_CTLR_ISO_RX_SDU_BUFFERS) {
		sp->sdu_list.list[next_write_indx++] = sdu_frag;
		sp->sdu_list.next_write_indx = next_write_indx;
#endif /* ISOAL_BUFFER_RX_SDUS_ENABLE */
	} else {
		/* Unreachable */
		LL_ASSERT(0);
	}

	return err;
}

static isoal_status_t isoal_rx_try_emit_sdu(struct isoal_sink *sink, bool end_of_sdu)
{
	struct isoal_sdu_production *sp;
	struct isoal_sdu_produced *sdu;
	isoal_status_t err;

	err = ISOAL_STATUS_OK;
	sp = &sink->sdu_production;
	sdu = &sp->sdu;

	/* Emit a SDU */
	const bool sdu_complete = (sp->sdu_available == 0) || end_of_sdu;

	if (end_of_sdu) {
		sp->sdu_available = 0;
	}

	if (sdu_complete) {
		uint8_t next_state = BT_ISO_START;

		switch (sp->sdu_state) {
		case BT_ISO_START:
			if (end_of_sdu) {
				sp->sdu_state = BT_ISO_SINGLE;
				next_state = BT_ISO_START;
			} else {
				sp->sdu_state = BT_ISO_START;
				next_state = BT_ISO_CONT;
			}
			break;
		case BT_ISO_CONT:
			if (end_of_sdu) {
				sp->sdu_state  = BT_ISO_END;
				next_state = BT_ISO_START;
			} else {
				sp->sdu_state  = BT_ISO_CONT;
				next_state = BT_ISO_CONT;
			}
			break;
		}
		sdu->status = sp->sdu_status;

		err = isoal_rx_buffered_emit_sdu(sink, end_of_sdu);
		sp->sdu_allocated = false;

		/* update next state */
		sink->sdu_production.sdu_state = next_state;
	}

	return err;
}

static isoal_status_t isoal_rx_append_to_sdu(struct isoal_sink *sink,
					     const struct isoal_pdu_rx *pdu_meta,
					     uint8_t offset,
					     uint8_t length,
					     bool is_end_fragment,
					     bool is_padding)
{
	isoal_pdu_len_t packet_available;
	const uint8_t *pdu_payload;
	bool handle_error_case;
	isoal_status_t err;

	/* Might get an empty packed due to errors, we will need to terminate
	 * and send something up anyhow
	 */
	packet_available = length;
	handle_error_case = (is_end_fragment && (packet_available == 0));

	pdu_payload = pdu_meta->pdu->payload + offset;
	LL_ASSERT(pdu_payload);

	/* While there is something left of the packet to consume */
	err = ISOAL_STATUS_OK;
	while ((packet_available > 0) || handle_error_case) {
		isoal_status_t err_alloc;
		struct isoal_sdu_production *sp;
		struct isoal_sdu_produced *sdu;

		err_alloc = ISOAL_STATUS_OK;
		if (!is_padding) {
			/* A new SDU should only be allocated if the current is
			 * not padding. Covers a situation where the end
			 * fragment was not received.
			 */
			err_alloc = isoal_rx_allocate_sdu(sink, pdu_meta);
		}

		sp = &sink->sdu_production;
		sdu = &sp->sdu;

		err |= err_alloc;

		/*
		 * For this SDU we can only consume of packet, bounded by:
		 *   - What can fit in the destination SDU.
		 *   - What remains of the packet.
		 */
		const size_t consume_len = MIN(
			packet_available,
			sp->sdu_available
		);

		if (consume_len > 0) {
			const struct isoal_sink_session *session = &sink->session;

			err |= session->sdu_write(sdu->contents.dbuf,
						  pdu_payload,
						  consume_len);
			pdu_payload += consume_len;
			sp->sdu_written   += consume_len;
			sp->sdu_available -= consume_len;
			packet_available  -= consume_len;
		}
		bool end_of_sdu = (packet_available == 0) && is_end_fragment;

		isoal_status_t err_emit = ISOAL_STATUS_OK;

		if (sp->sdu_allocated) {
			/* SDU should be emitted only if it was allocated */
			err_emit = isoal_rx_try_emit_sdu(sink, end_of_sdu);
		}

		handle_error_case = false;
		err |= err_emit;
	}

	return err;
}


/**
 * @brief Consume an unframed PDU: Copy contents into SDU(s) and emit to a sink
 * @details Destination sink may have an already partially built SDU
 *
 * @param sink[in,out]    Destination sink with bookkeeping state
 * @param pdu_meta[out]  PDU with meta information (origin, timing, status)
 *
 * @return Status
 */
static isoal_status_t isoal_rx_unframed_consume(struct isoal_sink *sink,
						const struct isoal_pdu_rx *pdu_meta)
{
	struct isoal_sink_session *session;
	struct isoal_sdu_production *sp;
	struct node_rx_iso_meta *meta;
	struct pdu_iso *pdu;
	bool end_of_packet;
	uint8_t next_state;
	isoal_status_t err;
	bool pdu_padding;
	uint8_t length;
	bool last_pdu;
	bool pdu_err;
	bool seq_err;
	uint8_t llid;

	sp = &sink->sdu_production;
	session = &sink->session;
	meta = pdu_meta->meta;
	pdu = pdu_meta->pdu;

	err = ISOAL_STATUS_OK;
	next_state = ISOAL_START;

	/* If status is not ISOAL_PDU_STATUS_VALID, length and LLID cannot be trusted */
	llid = pdu_meta->pdu->ll_id;
	pdu_err = (pdu_meta->meta->status != ISOAL_PDU_STATUS_VALID);
	length = pdu_meta->pdu->len;
	/* A zero length PDU with LLID 0b01 (PDU_BIS_LLID_START_CONTINUE) would be a padding PDU.
	 * However if there are errors in the PDU, it could be an incorrectly receive non-padding
	 * PDU. Therefore only consider a PDU with errors as padding if received after the end
	 * fragment is seen when padding PDUs are expected.
	 */
	pdu_padding = (length == 0) && (llid == PDU_BIS_LLID_START_CONTINUE) &&
		      (!pdu_err || sp->fsm == ISOAL_ERR_SPOOL);
	seq_err = (meta->payload_number != (sp->prev_pdu_id+1));

	/* If there are no buffers available, the PDUs received by the ISO-AL
	 * may not be in sequence even though this is expected for unframed rx.
	 * It would be necessary to exit the ISOAL_ERR_SPOOL state as the PDU
	 * count and as a result the last_pdu detection is no longer reliable.
	 */
	if (sp->fsm == ISOAL_ERR_SPOOL) {
		if ((!pdu_err && !seq_err &&
			/* Previous sequence error should have move to the
			 * ISOAL_ERR_SPOOL state and emitted the SDU in production. No
			 * PDU error so LLID and length are reliable and no sequence
			 * error so this PDU is the next in order.
			 */
			((sp->prev_pdu_is_end || sp->prev_pdu_is_padding) &&
				((llid == PDU_BIS_LLID_START_CONTINUE && length > 0) ||
					(llid == PDU_BIS_LLID_COMPLETE_END && length == 0))))
			/* Detected a start of a new SDU as the last PDU was an end
			 * fragment or padding and the current is the start of a new SDU
			 * (either filled or zero length). Move to ISOAL_START
			 * immediately.
			 */

			|| (meta->payload_number % session->pdus_per_sdu == 0)) {
			/* Based on the payload number, this should be the start
			 * of a new SDU.
			 */
			sp->fsm = ISOAL_START;
		}
	}

	if (sp->fsm == ISOAL_START) {
		struct isoal_sdu_produced *sdu;
		uint32_t anchorpoint;
		uint16_t sdu_offset;
		int32_t latency;

		sp->sdu_status = ISOAL_SDU_STATUS_VALID;
		sp->sdu_state = BT_ISO_START;
		sp->pdu_cnt = 1;
		session->seqn++;
		seq_err = false;

		/* The incoming time stamp for each PDU is expected to be the
		 * CIS / BIS reference anchor point. SDU reference point is
		 * reconstructed by adding the precalculated latency constant.
		 *
		 * BT Core V5.3 : Vol 6 Low Energy Controller : Part G IS0-AL:
		 * 3.2.2 SDU synchronization reference using unframed PDUs:
		 *
		 * The CIS reference anchor point is computed excluding any
		 * retransmissions or missed subevents and shall be set to the
		 * start of the isochronous event in which the first PDU
		 * containing the SDU could have been transferred.
		 *
		 * The BIG reference anchor point is the anchor point of the BIG
		 * event that the PDU is associated with.
		 */
		anchorpoint = meta->timestamp;
		latency = session->latency_unframed;
		sdu = &sp->sdu;
		sdu->timestamp = isoal_get_wrapped_time_us(anchorpoint, latency);

		/* If there are multiple SDUs in an ISO interval
		 * (SDU interval < ISO Interval) every SDU after the first
		 * should add an SDU interval to the time stamp.
		 *
		 * BT Core V5.3 : Vol 6 Low Energy Controller : Part G IS0-AL:
		 * 3.2.2 SDU synchronization reference using unframed PDUs:
		 *
		 * All PDUs belonging to a burst as defined by the configuration
		 * of BN have the same reference anchor point. When multiple
		 * SDUs have the same reference anchor point, the first SDU uses
		 * the reference anchor point timing. Each subsequent SDU
		 * increases the SDU synchronization reference timing with one
		 * SDU interval.
		 */
		sdu_offset = (meta->payload_number % session->burst_number) / session->pdus_per_sdu;
		sdu->timestamp = isoal_get_wrapped_time_us(sdu->timestamp,
					sdu_offset * session->sdu_interval);
	} else {
		sp->pdu_cnt++;
	}

	last_pdu = (sp->pdu_cnt == session->pdus_per_sdu);
	end_of_packet = (llid == PDU_BIS_LLID_COMPLETE_END) || last_pdu || pdu_err;

	switch (sp->fsm) {
	case ISOAL_START:
	case ISOAL_CONTINUE:
		if (pdu_err || seq_err) {
			/* PDU contains errors */
			if (last_pdu) {
				/* Last PDU all done */
				next_state = ISOAL_START;
			} else {
				next_state = ISOAL_ERR_SPOOL;
			}
		} else if (llid == PDU_BIS_LLID_START_CONTINUE) {
			/* PDU contains a continuation (neither start of end) fragment of SDU */
			if (last_pdu) {
				/* last pdu in sdu, but end fragment not seen, emit with error */
				next_state = ISOAL_START;
			} else {
				next_state = ISOAL_CONTINUE;
			}
		} else if (llid == PDU_BIS_LLID_COMPLETE_END) {
			/* PDU contains end fragment of a fragmented SDU */
			if (last_pdu) {
				/* Last PDU all done */
				next_state = ISOAL_START;
			} else {
				/* Padding after end fragment to follow */
				next_state = ISOAL_ERR_SPOOL;
			}
		} else  {
			/* Unsupported case */
			err = ISOAL_STATUS_ERR_UNSPECIFIED;
			LOG_ERR("Invalid unframed LLID (%d)", llid);
			LL_ASSERT(0);
		}
		break;

	case ISOAL_ERR_SPOOL:
		/* State assumes that at end fragment or err has been seen,
		 * now just consume the rest
		 */
		if (last_pdu) {
			/* Last padding seen, restart */
			next_state = ISOAL_START;
		} else {
			next_state = ISOAL_ERR_SPOOL;
		}
		break;

	}

	/* Update error state */
	/* Prioritisation:
	 * (1) Sequence Error should set the ISOAL_SDU_STATUS_LOST_DATA status
	 *     as data is missing and this will trigger the HCI to discard any
	 *     data received.
	 *
	 *     BT Core V5.3 : Vol 4 HCI I/F : Part G HCI Func. Spec.:
	 *     5.4.5 HCI ISO Data packets
	 *     If Packet_Status_Flag equals 0b10 then PB_Flag shall equal 0b10.
	 *     When Packet_Status_Flag is set to 0b10 in packets from the Controller to the
	 *     Host, there is no data and ISO_SDU_Length shall be set to zero.
	 *
	 * (2) Any error status received from the LL via the PDU status should
	 *     set the relevant error conditions.
	 *
	 * (3) Missing end fragment handling.
	 */
	if (seq_err) {
		sp->sdu_status |= ISOAL_SDU_STATUS_LOST_DATA;
	} else if (pdu_err && !pdu_padding) {
		sp->sdu_status |= meta->status;
	} else if (last_pdu && (llid != PDU_BIS_LLID_COMPLETE_END) &&
				(sp->fsm  != ISOAL_ERR_SPOOL)) {
		/* END fragment never seen */
		sp->sdu_status |= ISOAL_SDU_STATUS_ERRORS;
	}

	/* Append valid PDU to SDU */
	if (sp->fsm != ISOAL_ERR_SPOOL && (!pdu_padding || end_of_packet)) {
		err |= isoal_rx_append_to_sdu(sink, pdu_meta, 0,
					      length, end_of_packet,
					      pdu_padding);
	}

	/* Update next state */
	sp->fsm = next_state;
	sp->prev_pdu_id = meta->payload_number;
	sp->prev_pdu_is_end = !pdu_err && llid == PDU_BIS_LLID_COMPLETE_END;
	sp->prev_pdu_is_padding = !pdu_err && pdu_padding;

	return err;
}

/* Check a given segment for errors */
static isoal_sdu_status_t isoal_check_seg_header(struct pdu_iso_sdu_sh *seg_hdr,
						   uint8_t pdu_size_remaining)
{
	if (!seg_hdr) {
		/* Segment header is null */
		return ISOAL_SDU_STATUS_ERRORS;
	}

	if (pdu_size_remaining >= PDU_ISO_SEG_HDR_SIZE &&
		pdu_size_remaining >= PDU_ISO_SEG_HDR_SIZE + seg_hdr->len) {

		/* Valid if there is sufficient data for the segment header and
		 * there is sufficient data for the required length of the
		 * segment
		 */
		return ISOAL_SDU_STATUS_VALID;
	}

	/* Data is missing from the PDU */
	return ISOAL_SDU_STATUS_LOST_DATA;
}

/**
 * @brief Consume a framed PDU: Copy contents into SDU(s) and emit to a sink
 * @details Destination sink may have an already partially built SDU
 *
 * @param sink[in,out]   Destination sink with bookkeeping state
 * @param pdu_meta[out]  PDU with meta information (origin, timing, status)
 *
 * @return Status
 */
static isoal_status_t isoal_rx_framed_consume(struct isoal_sink *sink,
					      const struct isoal_pdu_rx *pdu_meta)
{
	struct isoal_sink_session *session;
	struct isoal_sdu_production *sp;
	struct isoal_sdu_produced *sdu;
	struct pdu_iso_sdu_sh *seg_hdr;
	struct node_rx_iso_meta *meta;
	uint32_t anchorpoint;
	uint8_t *end_of_pdu;
	uint32_t timeoffset;
	isoal_status_t err;
	uint8_t next_state;
	uint32_t timestamp;
	bool pdu_padding;
	int32_t latency;
	bool pdu_err;
	bool seq_err;
	bool seg_err;

	sp = &sink->sdu_production;
	session = &sink->session;
	meta = pdu_meta->meta;
	sdu = &sp->sdu;

	err = ISOAL_STATUS_OK;
	next_state = ISOAL_START;
	pdu_err = (pdu_meta->meta->status != ISOAL_PDU_STATUS_VALID);
	pdu_padding = (pdu_meta->pdu->len == 0);

	if (sp->fsm == ISOAL_START) {
		seq_err = false;
	} else {
		seq_err = (meta->payload_number != (sp->prev_pdu_id + 1));
	}

	end_of_pdu = ((uint8_t *) pdu_meta->pdu->payload) + pdu_meta->pdu->len - 1;
	seg_hdr = (pdu_err || seq_err || pdu_padding) ? NULL :
			(struct pdu_iso_sdu_sh *) pdu_meta->pdu->payload;

	seg_err = false;
	if (seg_hdr && isoal_check_seg_header(seg_hdr, pdu_meta->pdu->len) ==
								ISOAL_SDU_STATUS_LOST_DATA) {
		seg_err = true;
		seg_hdr = NULL;
	}

	while (seg_hdr) {
		bool append = true;
		const uint8_t sc    = seg_hdr->sc;
		const uint8_t cmplt = seg_hdr->cmplt;

		if (sp->fsm == ISOAL_START) {
			sp->sdu_status = ISOAL_SDU_STATUS_VALID;
			sp->sdu_state  = BT_ISO_START;
			session->seqn++;
		}

		switch (sp->fsm) {
		case ISOAL_START:
			timeoffset = seg_hdr->timeoffset;
			anchorpoint = meta->timestamp;
			latency = session->latency_framed;
			timestamp = isoal_get_wrapped_time_us(anchorpoint, latency - timeoffset);

			if (!sc && !cmplt) {
				/* The start of a new SDU, where not all SDU data is included in
				 * the current PDU, and additional PDUs are required to complete
				 * the SDU.
				 */
				sdu->timestamp = timestamp;
				next_state = ISOAL_CONTINUE;
			} else if (!sc && cmplt) {
				/* The start of a new SDU that contains the full SDU data in the
				 * current PDU.
				 */
				sdu->timestamp = timestamp;
				next_state = ISOAL_START;
			} else {
				/* Unsupported case */
				err = ISOAL_STATUS_ERR_UNSPECIFIED;
			}
			break;

		case ISOAL_CONTINUE:
			if (sc && !cmplt) {
				/* The continuation of a previous SDU. The SDU payload is appended
				 * to the previous data and additional PDUs are required to
				 * complete the SDU.
				 */
				next_state = ISOAL_CONTINUE;
			} else if (sc && cmplt) {
				/* The continuation of a previous SDU.
				 * Frame data is appended to previously received SDU data and
				 * completes in the current PDU.
				 */
				next_state = ISOAL_START;
			} else {
				/* Unsupported case */
				err = ISOAL_STATUS_ERR_UNSPECIFIED;
			}
			break;

		case ISOAL_ERR_SPOOL:
			/* In error state, search for valid next start of SDU */
			timeoffset = seg_hdr->timeoffset;
			anchorpoint = meta->timestamp;
			latency = session->latency_framed;
			timestamp = isoal_get_wrapped_time_us(anchorpoint, latency - timeoffset);

			if (!sc && !cmplt) {
				/* The start of a new SDU, where not all SDU data is included in
				 * the current PDU, and additional PDUs are required to complete
				 * the SDU.
				 */
				sdu->timestamp = timestamp;
				next_state = ISOAL_CONTINUE;
			} else if (!sc && cmplt) {
				/* The start of a new SDU that contains the full SDU data in the
				 * current PDU.
				 */
				sdu->timestamp = timestamp;
				next_state = ISOAL_START;
			} else {
				/* Start not found yet, stay in Error state */
				append = false;
				next_state = ISOAL_ERR_SPOOL;
			}

			/* TODO: Confirm if the sequence number must be updated even for an SDU
			 * with errors.
			 * BT Core V5.3 : Vol 6 Low Energy Controller : Part G IS0-AL:
			 * 2 ISOAL Features :
			 * The sequence number shall be incremented for each SDU_Interval,
			 * whether or not an SDU was received from or sent to the upper layer.
			 */

			if (next_state != ISOAL_ERR_SPOOL) {
				/* While in the Error state, received a valid start of the next SDU,
				 * so SDU status and sequence number should be updated.
				 */
				sp->sdu_status = ISOAL_SDU_STATUS_VALID;
				/* sp->sdu_state will be set by next_state decided above */
				session->seqn++;
			}
			break;
		}

		if (append) {
			/* Calculate offset of first payload byte from SDU based on assumption
			 * of No time_offset in header
			 */
			uint8_t offset = ((uint8_t *) seg_hdr) + PDU_ISO_SEG_HDR_SIZE -
					 pdu_meta->pdu->payload;
			uint8_t length = seg_hdr->len;

			if (!sc) {
				/* time_offset included in header, don't copy offset field to SDU */
				offset = offset + PDU_ISO_SEG_TIMEOFFSET_SIZE;
				length = length - PDU_ISO_SEG_TIMEOFFSET_SIZE;
			}

			/* Todo: check if effective len=0 what happens then?
			 * We should possibly be able to send empty packets with only time stamp
			 */

			err |= isoal_rx_append_to_sdu(sink, pdu_meta, offset, length, cmplt, false);
		}

		/* Update next state */
		sp->fsm = next_state;

		/* Find next segment header, set to null if past end of PDU */
		seg_hdr = (struct pdu_iso_sdu_sh *) (((uint8_t *) seg_hdr) +
						     seg_hdr->len + PDU_ISO_SEG_HDR_SIZE);

		if (((uint8_t *) seg_hdr) > end_of_pdu) {
			seg_hdr = NULL;
		} else if (isoal_check_seg_header(seg_hdr,
				(uint8_t)(end_of_pdu + 1 - ((uint8_t *) seg_hdr))) ==
								ISOAL_SDU_STATUS_LOST_DATA) {
			seg_err = true;
			seg_hdr = NULL;
		}
	}

	if (pdu_err || seq_err || seg_err) {
		/* When one or more ISO Data PDUs are not received, the receiving device may
		 * discard all SDUs affected by the missing PDUs. Any partially received SDU
		 * may also be discarded.
		 */
		next_state = ISOAL_ERR_SPOOL;

		/* This maps directly to the HCI ISO Data packet Packet_Status_Flag by way of the
		 * sdu_status in the SDU emitted.
		 * BT Core V5.3 : Vol 4 HCI I/F : Part G HCI Func. Spec.:
		 * 5.4.5 HCI ISO Data packets : Table 5.2 :
		 * Packet_Status_Flag (in packets sent by the Controller)
		 *   0b00  Valid data. The complete SDU was received correctly.
		 *   0b01  Possibly invalid data. The contents of the ISO_SDU_Fragment may contain
		 *         errors or part of the SDU may be missing. This is reported as "data with
		 *         possible errors".
		 *   0b10  Part(s) of the SDU were not received correctly. This is reported as
		 *         "lost data".
		 */
		if (seq_err || seg_err) {
			sp->sdu_status |= ISOAL_SDU_STATUS_LOST_DATA;
		} else if (pdu_err) {
			sp->sdu_status |= meta->status;
		}

		if (sp->fsm == ISOAL_START) {
			/* Sequence number should be incremented if an error
			 * occurs at the beginning.
			 */
			session->seqn++;

			if (sdu->timestamp == 0) {
				/* Last timestamp is not valid so set an
				 * approximate timestamp
				 */
				anchorpoint = meta->timestamp;
				latency = session->latency_framed;
				timestamp = isoal_get_wrapped_time_us(anchorpoint, latency);

				sdu->timestamp = timestamp;
			} else {
				/* Advance the timestamp by an SDU interval */
				sdu->timestamp += session->sdu_interval;
			}
		}

		/* Flush current SDU with error if any */
		err |= isoal_rx_append_to_sdu(sink, pdu_meta, 0, 0, true, false);

		/* Update next state */
		sink->sdu_production.fsm = next_state;
	}

	sp->prev_pdu_id = meta->payload_number;

	return err;
}

/**
 * @brief Deep copy a PDU, recombine into SDU(s)
 * @details Recombination will occur individually for every enabled sink
 *
 * @param sink_hdl[in] Handle of destination sink
 * @param pdu_meta[in] PDU along with meta information (origin, timing, status)
 * @return Status
 */
isoal_status_t isoal_rx_pdu_recombine(isoal_sink_handle_t sink_hdl,
				      const struct isoal_pdu_rx *pdu_meta)
{
	struct isoal_sink *sink = &isoal_global.sink_state[sink_hdl];
	isoal_status_t err = ISOAL_STATUS_OK;

	if (sink && sink->sdu_production.mode != ISOAL_PRODUCTION_MODE_DISABLED) {
		if (sink->session.framed) {
			err = isoal_rx_framed_consume(sink, pdu_meta);
		} else {
			err = isoal_rx_unframed_consume(sink, pdu_meta);
		}
	}

	return err;
}
#endif /* CONFIG_BT_CTLR_SYNC_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
/**
 * @brief Check if a time difference calculation is valid and return the difference.
 * @param  time_before Subtrahend
 * @param  time_after  Minuend
 * @param  result      Difference if valid
 * @return             Validity - valid if time_after leads time_before with
 *                                consideration for wrapping such that the
 *                                difference can be calculated.
 */
static bool isoal_get_time_diff(uint32_t time_before, uint32_t time_after, uint32_t *result)
{
	bool valid = false;

	LL_ASSERT(time_before <= ISOAL_TIME_WRAPPING_POINT_US);
	LL_ASSERT(time_after <= ISOAL_TIME_WRAPPING_POINT_US);

	if (time_before > time_after) {
		if (time_before >= ISOAL_TIME_MID_POINT_US &&
			time_after <= ISOAL_TIME_MID_POINT_US) {
			if ((time_before - time_after) <=  ISOAL_TIME_SPAN_HALF_US) {
				/* Time_before is after time_after and the result is invalid. */
			} else {
				/* time_after has wrapped */
				*result = time_after + ISOAL_TIME_SPAN_FULL_US - time_before;
				valid = true;
			}
		}

		/* Time_before is after time_after and the result is invalid. */
	} else {
		/* Time_before <= time_after */
		*result = time_after - time_before;
		if (*result <=  ISOAL_TIME_SPAN_HALF_US) {
			/* result is valid  if it is within half the maximum
			 * time span.
			 */
			valid = true;
		} else {
			/* time_before has wrapped and the calculation is not
			 * valid as time_before is ahead of time_after.
			 */
		}
	}

	return valid;
}

/**
 * @brief Find free source from statically-sized pool and allocate it
 * @details Implemented as linear search since pool is very small
 *
 * @param hdl[out]  Handle to source
 * @return ISOAL_STATUS_OK if we could allocate; otherwise ISOAL_STATUS_ERR_SOURCE_ALLOC
 */
static isoal_status_t isoal_source_allocate(isoal_source_handle_t *hdl)
{
	isoal_source_handle_t i;

	/* Very small linear search to find first free */
	for (i = 0; i < CONFIG_BT_CTLR_ISOAL_SOURCES; i++) {
		if (isoal_global.source_allocated[i] == ISOAL_ALLOC_STATE_FREE) {
			isoal_global.source_allocated[i] = ISOAL_ALLOC_STATE_TAKEN;
			*hdl = i;
			return ISOAL_STATUS_OK;
		}
	}

	return ISOAL_STATUS_ERR_SOURCE_ALLOC; /* All entries were taken */
}

/**
 * @brief Mark a source as being free to allocate again
 * @param hdl[in]  Handle to source
 */
static void isoal_source_deallocate(isoal_source_handle_t hdl)
{
	struct isoal_pdu_production *pp;
	struct isoal_source *source;

	source = &isoal_global.source_state[hdl];
	pp = &source->pdu_production;

	if (pp->pdu_available > 0) {
		/* There is a pending PDU that should be released */
		if (source && source->session.pdu_release) {
			source->session.pdu_release(pp->pdu.contents.handle,
						    source->session.handle,
						    ISOAL_STATUS_ERR_PDU_EMIT);
		}
	}

	isoal_global.source_allocated[hdl] = ISOAL_ALLOC_STATE_FREE;
	(void)memset(&isoal_global.source_state[hdl], 0, sizeof(struct isoal_source));
}

/**
 * @brief      Check if a provided handle is valid
 * @param[in]  hdl Input handle for validation
 * @return         Handle valid / not valid
 */
static isoal_status_t isoal_check_source_hdl_valid(isoal_source_handle_t hdl)
{
	if (hdl < CONFIG_BT_CTLR_ISOAL_SOURCES &&
		isoal_global.source_allocated[hdl] == ISOAL_ALLOC_STATE_TAKEN) {
		return ISOAL_STATUS_OK;
	}

	LOG_ERR("Invalid source handle (0x%02x)", hdl);

	return ISOAL_STATUS_ERR_UNSPECIFIED;
}

/**
 * @brief Create a new source
 *
 * @param handle[in]            Connection handle
 * @param role[in]              Peripheral, Central or Broadcast
 * @param framed[in]            Framed case
 * @param burst_number[in]      Burst Number
 * @param flush_timeout[in]     Flush timeout
 * @param max_octets[in]        Maximum PDU size (Max_PDU_C_To_P / Max_PDU_P_To_C)
 * @param sdu_interval[in]      SDU interval
 * @param iso_interval[in]      ISO interval
 * @param stream_sync_delay[in] CIS / BIS sync delay
 * @param group_sync_delay[in]  CIG / BIG sync delay
 * @param pdu_alloc[in]         Callback of PDU allocator
 * @param pdu_write[in]         Callback of PDU byte writer
 * @param pdu_emit[in]          Callback of PDU emitter
 * @param pdu_release[in]       Callback of PDU deallocator
 * @param hdl[out]              Handle to new source
 *
 * @return ISOAL_STATUS_OK if we could create a new sink; otherwise ISOAL_STATUS_ERR_SOURCE_ALLOC
 */
isoal_status_t isoal_source_create(
	uint16_t                    handle,
	uint8_t                     role,
	uint8_t                     framed,
	uint8_t                     burst_number,
	uint8_t                     flush_timeout,
	uint8_t                     max_octets,
	uint32_t                    sdu_interval,
	uint16_t                    iso_interval,
	uint32_t                    stream_sync_delay,
	uint32_t                    group_sync_delay,
	isoal_source_pdu_alloc_cb   pdu_alloc,
	isoal_source_pdu_write_cb   pdu_write,
	isoal_source_pdu_emit_cb    pdu_emit,
	isoal_source_pdu_release_cb pdu_release,
	isoal_source_handle_t       *hdl)
{
	isoal_status_t err;

	/* Allocate a new source */
	err = isoal_source_allocate(hdl);
	if (err) {
		return err;
	}

	struct isoal_source_session *session = &isoal_global.source_state[*hdl].session;

	session->handle = handle;
	session->framed = framed;
	session->burst_number = burst_number;
	session->iso_interval = iso_interval;

	/* Todo: Next section computing various constants, should potentially be a
	 * function in itself as a number of the dependencies could be changed while
	 * a connection is active.
	 */

	/* Note: sdu_interval unit is uS, iso_interval is a multiple of 1.25mS */
	session->pdus_per_sdu = (burst_number * sdu_interval) /
		((uint32_t)iso_interval * ISO_INT_UNIT_US);
	/* Set maximum PDU size */
	session->max_pdu_size = max_octets;

	/* Remember the platform-specific callbacks */
	session->pdu_alloc   = pdu_alloc;
	session->pdu_write   = pdu_write;
	session->pdu_emit    = pdu_emit;
	session->pdu_release = pdu_release;

	/* TODO: Constant need to be updated */

	/* Initialize running seq number to zero */
	session->seqn = 0;

	return err;
}

/**
 * @brief Get reference to configuration struct
 *
 * @param hdl[in]   Handle to new source
 * @return Reference to parameter struct, to be configured by caller
 */
struct isoal_source_config *isoal_get_source_param_ref(isoal_source_handle_t hdl)
{
	LL_ASSERT(hdl < CONFIG_BT_CTLR_ISOAL_SOURCES);
	LL_ASSERT(isoal_global.source_allocated[hdl] == ISOAL_ALLOC_STATE_TAKEN);

	return &isoal_global.source_state[hdl].session.param;
}

/**
 * @brief Atomically enable latch-in of packets and PDU production
 * @param hdl[in]  Handle of existing instance
 */
void isoal_source_enable(isoal_source_handle_t hdl)
{
	/* Reset bookkeeping state */
	memset(&isoal_global.source_state[hdl].pdu_production, 0,
	       sizeof(isoal_global.source_state[hdl].pdu_production));

	/* Atomically enable */
	isoal_global.source_state[hdl].pdu_production.mode = ISOAL_PRODUCTION_MODE_ENABLED;
}

/**
 * @brief Atomically disable latch-in of packets and PDU production
 * @param hdl[in]  Handle of existing instance
 */
void isoal_source_disable(isoal_source_handle_t hdl)
{
	/* Atomically disable */
	isoal_global.source_state[hdl].pdu_production.mode = ISOAL_PRODUCTION_MODE_DISABLED;
}

struct isoal_source *isoal_source_get(isoal_source_handle_t hdl)
{
	return &isoal_global.source_state[hdl];
}

/**
 * @brief Disable and deallocate existing source
 * @param hdl[in]  Handle of existing instance
 */
void isoal_source_destroy(isoal_source_handle_t hdl)
{
	/* Atomic disable */
	isoal_source_disable(hdl);

	/* Permit allocation anew */
	isoal_source_deallocate(hdl);
}

/**
 * Queue the PDU in production in the relevant LL transmit queue. If the
 * attmept to release the PDU fails, the buffer linked to the PDU will be released
 * and it will not be possible to retry the emit operation on the same PDU.
 * @param[in]  source_ctx        ISO-AL source reference for this CIS / BIS
 * @param[in]  produced_pdu      PDU in production
 * @param[in]  pdu_ll_id         LLID to be set indicating the type of fragment
 * @param[in]  sdu_fragments     Nummber of SDU HCI fragments consumed
 * @param[in]  payload_number    CIS / BIS payload number
 * @param[in]  payload_size      Length of the data written to the PDU
 * @return     Error status of the operation
 */
static isoal_status_t isoal_tx_pdu_emit(const struct isoal_source *source_ctx,
					const struct isoal_pdu_produced *produced_pdu,
					const uint8_t pdu_ll_id,
					const uint8_t sdu_fragments,
					const uint64_t payload_number,
					const isoal_pdu_len_t payload_size)
{
	struct node_tx_iso *node_tx;
	isoal_status_t status;
	uint16_t handle;

	/* Retrieve CIS / BIS handle */
	handle = bt_iso_handle(source_ctx->session.handle);

	/* Retrieve Node handle */
	node_tx = produced_pdu->contents.handle;
	/* Under race condition with isoal_source_deallocate() */
	if (!node_tx) {
		return ISOAL_STATUS_ERR_PDU_EMIT;
	}

	/* Set payload number */
	node_tx->payload_count = payload_number & 0x7fffffffff;
	node_tx->sdu_fragments = sdu_fragments;
	/* Set PDU LLID */
	produced_pdu->contents.pdu->ll_id = pdu_ll_id;
	/* Set PDU length */
	produced_pdu->contents.pdu->len = (uint8_t)payload_size;

	/* Attempt to enqueue the node towards the LL */
	status = source_ctx->session.pdu_emit(node_tx, handle);

	if (status != ISOAL_STATUS_OK) {
		/* If it fails, the node will be released and no further attempt
		 * will be possible
		 */
		LOG_ERR("Failed to enqueue node (%p)", node_tx);
		source_ctx->session.pdu_release(node_tx, handle, status);
	}

	return status;
}

/* Allocates a new PDU only if the previous PDU was emitted */
static isoal_status_t isoal_tx_allocate_pdu(struct isoal_source *source,
					    const struct isoal_sdu_tx *tx_sdu)
{
	struct isoal_source_session *session;
	struct isoal_pdu_production *pp;
	struct isoal_pdu_produced *pdu;
	isoal_status_t err;

	err = ISOAL_STATUS_OK;
	session = &source->session;
	pp = &source->pdu_production;
	pdu = &pp->pdu;

	/* Allocate a PDU if the previous was filled (thus sent) */
	const bool pdu_complete = (pp->pdu_available == 0);

	if (pdu_complete) {
		/* Allocate new PDU buffer */
		err = session->pdu_alloc(
			&pdu->contents  /* [out] Updated with pointer and size */
		);

		if (err) {
			pdu->contents.handle = NULL;
			pdu->contents.pdu    = NULL;
			pdu->contents.size   = 0;
		}

		/* Get maximum buffer available */
		const size_t available_len = MIN(
			session->max_pdu_size,
			pdu->contents.size
		);

		/* Nothing has been written into buffer yet */
		pp->pdu_written   = 0;
		pp->pdu_available = available_len;
		LL_ASSERT(available_len > 0);

		pp->pdu_cnt++;
	}

	return err;
}

/**
 * Attempt to emit the PDU in production if it is complete.
 * @param[in]  source      ISO-AL source reference
 * @param[in]  force_emit  Request PDU emit
 * @param[in]  pdu_ll_id   LLID / PDU fragment type as Start, Cont, End, Single (Unframed) or Framed
 * @return     Error status of operation
 */
static isoal_status_t isoal_tx_try_emit_pdu(struct isoal_source *source,
					    bool force_emit,
					    uint8_t pdu_ll_id)
{
	struct isoal_pdu_production *pp;
	struct isoal_pdu_produced   *pdu;
	isoal_status_t err;

	err = ISOAL_STATUS_OK;
	pp = &source->pdu_production;
	pdu = &pp->pdu;

	/* Emit a PDU */
	const bool pdu_complete = (pp->pdu_available == 0) || force_emit;

	if (force_emit) {
		pp->pdu_available = 0;
	}

	if (pdu_complete) {
		/* Emit PDU and increment the payload number */
		err = isoal_tx_pdu_emit(source, pdu, pdu_ll_id,
					pp->sdu_fragments,
					pp->payload_number,
					pp->pdu_written);
		pp->payload_number++;
		pp->sdu_fragments = 0;
	}

	return err;
}

/**
 * @brief Fragment received SDU and produce unframed PDUs
 * @details Destination source may have an already partially built PDU
 *
 * @param source[in,out] Destination source with bookkeeping state
 * @param tx_sdu[in]     SDU with packet boundary information
 *
 * @return Status
 */
static isoal_status_t isoal_tx_unframed_produce(struct isoal_source *source,
						const struct isoal_sdu_tx *tx_sdu)
{
	struct isoal_source_session *session;
	isoal_sdu_len_t packet_available;
	struct isoal_pdu_production *pp;
	const uint8_t *sdu_payload;
	bool zero_length_sdu;
	isoal_status_t err;
	bool padding_pdu;
	uint8_t ll_id;

	session     = &source->session;
	pp          = &source->pdu_production;
	padding_pdu = false;
	err         = ISOAL_STATUS_OK;

	packet_available = tx_sdu->size;
	sdu_payload = tx_sdu->dbuf;
	LL_ASSERT(sdu_payload);

	zero_length_sdu = (packet_available == 0 &&
		tx_sdu->sdu_state == BT_ISO_SINGLE);

	if (tx_sdu->sdu_state == BT_ISO_START ||
		tx_sdu->sdu_state == BT_ISO_SINGLE) {
		/* Initialize to info provided in SDU */
		uint32_t actual_grp_ref_point = tx_sdu->grp_ref_point;
		uint64_t actual_event = tx_sdu->target_event;

		/* Start of a new SDU */

		/* Update sequence number for received SDU
		 *
		 * BT Core V5.3 : Vol 6 Low Energy Controller : Part G IS0-AL:
		 * 2 ISOAL Features :
		 * SDUs received by the ISOAL from the upper layer shall be
		 * given a sequence number which is initialized to 0 when the
		 * CIS or BIS is created.
		 *
		 * NOTE: The upper layer may synchronize its sequence number
		 * with the sequence number in the ISOAL once the Datapath is
		 * configured and the link is established.
		 */
		session->seqn++;

		/* Update payload counter in case time has passed since last
		 * SDU. This should mean that event count * burst number should
		 * be greater than the current payload number. In the event of
		 * an SDU interval smaller than the ISO interval, multiple SDUs
		 * will be sent in the same event. As such the current payload
		 * number should be retained. Payload numbers are indexed at 0
		 * and valid until the PDU is emitted.
		 */
		pp->payload_number = MAX(pp->payload_number,
			(tx_sdu->target_event * session->burst_number));

		/* Get actual event for this payload number */
		actual_event = pp->payload_number / session->burst_number;

		/* Get group reference point for this PDU based on the actual
		 * event being set. This might introduce some errors as the
		 * group refernce point for future events could drift. However
		 * as the time offset calculation requires an absolute value,
		 * this seems to be the best candidate. As the actual group
		 * refereence point is 32-bits, it is expected that advancing
		 * the reference point will cause it to wrap around.
		 */
		if (actual_event > tx_sdu->target_event) {
			actual_grp_ref_point = isoal_get_wrapped_time_us(tx_sdu->grp_ref_point,
				((actual_event - tx_sdu->target_event) * session->iso_interval *
					ISO_INT_UNIT_US));
		}

		/* Store timing info for TX Sync command */
		session->tx_time_stamp = actual_grp_ref_point;
		/* BT Core V5.3 : Vol 4 HCI : Part E HCI Functional Spec:
		 * 7.8.96 LE Read ISO TX Sync Command:
		 * When the Connection_Handle identifies a CIS or BIS that is
		 * transmitting unframed PDUs the value of Time_Offset returned
		 * shall be zero
		 * Relies on initialization value being 0.
		 */

		/* Reset PDU fragmentation count for this SDU */
		pp->pdu_cnt = 0;

		/* The start of an unframed SDU will always be in a new PDU.
		 * There cannot be any other fragments packed.
		 */
		pp->sdu_fragments = 0;
	}

	/* PDUs should be created until the SDU fragment has been fragmented or
	 * if this is the last fragment of the SDU, until the required padding
	 * PDU(s) are sent.
	 */
	while ((err == ISOAL_STATUS_OK) &&
		((packet_available > 0) || padding_pdu || zero_length_sdu)) {
		const isoal_status_t err_alloc = isoal_tx_allocate_pdu(source, tx_sdu);
		struct isoal_pdu_produced  *pdu = &pp->pdu;
		err |= err_alloc;

		/*
		 * For this PDU we can only consume of packet, bounded by:
		 *   - What can fit in the destination PDU.
		 *   - What remains of the packet.
		 */
		const size_t consume_len = MIN(
			packet_available,
			pp->pdu_available
		);

		/* End of the SDU fragment has been reached when the last of the
		 * SDU is packed into a PDU.
		 */
		bool end_of_sdu_frag = !padding_pdu &&
				((consume_len > 0 && consume_len == packet_available) ||
					zero_length_sdu);

		if (consume_len > 0) {
			err |= session->pdu_write(&pdu->contents,
						  pp->pdu_written,
						  sdu_payload,
						  consume_len);
			sdu_payload       += consume_len;
			pp->pdu_written   += consume_len;
			pp->pdu_available -= consume_len;
			packet_available  -= consume_len;
		}

		if (end_of_sdu_frag) {
			/* Each PDU will carry the number of completed SDU
			 * fragments contained in that PDU.
			 */
			pp->sdu_fragments++;
		}

		/* End of the SDU is reached at the end of the last SDU fragment
		 * or if this is a single fragment SDU
		 */
		bool end_of_sdu = (packet_available == 0) &&
				((tx_sdu->sdu_state == BT_ISO_SINGLE) ||
					(tx_sdu->sdu_state == BT_ISO_END));

		/* Decide PDU type
		 *
		 * BT Core V5.3 : Vol 6 Low Energy Controller : Part G IS0-AL:
		 * 2.1 Unframed PDU :
		 * LLID 0b00 PDU_BIS_LLID_COMPLETE_END:
		 * (1) When the payload of the ISO Data PDU contains the end
		 *     fragment of an SDU.
		 * (2) When the payload of the ISO Data PDU contains a complete
		 *     SDU.
		 * (3) When an SDU contains zero length data, the corresponding
		 *     PDU shall be of zero length and the LLID field shall be
		 *     set to 0b00.
		 *
		 * LLID 0b01 PDU_BIS_LLID_COMPLETE_END:
		 * (1) When the payload of the ISO Data PDU contains a start or
		 *     a continuation fragment of an SDU.
		 * (2) When the ISO Data PDU is used as padding.
		 */
		ll_id = PDU_BIS_LLID_COMPLETE_END;
		if (!end_of_sdu || padding_pdu) {
			ll_id = PDU_BIS_LLID_START_CONTINUE;
		}

		const isoal_status_t err_emit = isoal_tx_try_emit_pdu(source, end_of_sdu, ll_id);

		err |= err_emit;

		/* Send padding PDU(s) if required
		 *
		 * BT Core V5.3 : Vol 6 Low Energy Controller : Part G IS0-AL:
		 * 2.1 Unframed PDU :
		 * Each SDU shall generate BN ÷ (ISO_Interval ÷ SDU_Interval)
		 * fragments. If an SDU generates less than this number of
		 * fragments, empty payloads shall be used to make up the
		 * number.
		 */
		padding_pdu = (end_of_sdu && (pp->pdu_cnt < session->pdus_per_sdu));
		zero_length_sdu = false;
	}

	return err;
}

/**
 * @brief  Inserts a segmentation header at the current write point in the PDU
 *         under production.
 * @param  source              source handle
 * @param  sc                  start / continuation bit value to be written
 * @param  cmplt               complete bit value to be written
 * @param  time_offset         value of time offset to be written
 * @return                     status
 */
static isoal_status_t isoal_insert_seg_header_timeoffset(struct isoal_source *source,
							 const bool sc,
							 const bool cmplt,
							 const uint32_t time_offset)
{
	struct isoal_source_session *session;
	struct isoal_pdu_production *pp;
	struct isoal_pdu_produced *pdu;
	struct pdu_iso_sdu_sh seg_hdr;
	isoal_status_t err;
	uint8_t write_size;

	session    = &source->session;
	pp         = &source->pdu_production;
	pdu        = &pp->pdu;
	write_size = PDU_ISO_SEG_HDR_SIZE + (sc ? 0 : PDU_ISO_SEG_TIMEOFFSET_SIZE);

	memset(&seg_hdr, 0, sizeof(seg_hdr));

	/* Check if there is enough space left in the PDU. This should not fail
	 * as the calling should also check before requesting insertion of a
	 * new header.
	 */
	if (pp->pdu_available < write_size) {

		return ISOAL_STATUS_ERR_UNSPECIFIED;
	}

	seg_hdr.sc = sc;
	seg_hdr.cmplt = cmplt;
	seg_hdr.len = sc ? 0 : PDU_ISO_SEG_TIMEOFFSET_SIZE;

	if (!sc) {
		seg_hdr.timeoffset = time_offset;
	}

	/* Store header */
	pp->seg_hdr_sc = seg_hdr.sc;
	pp->seg_hdr_length = seg_hdr.len;

	/* Save location of last segmentation header so that it can be updated
	 * as data is written.
	 */
	pp->last_seg_hdr_loc = pp->pdu_written;
	/* Write to PDU */
	err = session->pdu_write(&pdu->contents,
				  pp->pdu_written,
				  (uint8_t *) &seg_hdr,
				  write_size);
	pp->pdu_written   += write_size;
	pp->pdu_available -= write_size;

	return err;
}

/**
 * @breif  Updates the cmplt flag and length in the last segmentation header written
 * @param  source     source handle
 * @param  cmplt      ew value for complete flag
 * param   add_length length to add
 * @return            status
 */
static isoal_status_t isoal_update_seg_header_cmplt_length(struct isoal_source *source,
							   const bool cmplt,
							   const uint8_t add_length)
{
	struct isoal_source_session *session;
	struct isoal_pdu_production *pp;
	struct isoal_pdu_produced *pdu;
	struct pdu_iso_sdu_sh seg_hdr;

	session    = &source->session;
	pp         = &source->pdu_production;
	pdu        = &pp->pdu;
	memset(&seg_hdr, 0, sizeof(seg_hdr));

	seg_hdr.sc = pp->seg_hdr_sc;

	/* Update the complete flag and length */
	seg_hdr.cmplt = cmplt;
	pp->seg_hdr_length += add_length;
	seg_hdr.len = pp->seg_hdr_length;


	/* Re-write the segmentation header at the same location */
	return session->pdu_write(&pdu->contents,
				  pp->last_seg_hdr_loc,
				  (uint8_t *) &seg_hdr,
				  PDU_ISO_SEG_HDR_SIZE);
}

/**
 * @brief Fragment received SDU and produce framed PDUs
 * @details Destination source may have an already partially built PDU
 *
 * @param source[in,out] Destination source with bookkeeping state
 * @param tx_sdu[in]     SDU with packet boundary information
 *
 * @return Status
 */
static isoal_status_t isoal_tx_framed_produce(struct isoal_source *source,
						const struct isoal_sdu_tx *tx_sdu)
{
	struct isoal_source_session *session;
	struct isoal_pdu_production *pp;
	isoal_sdu_len_t packet_available;
	const uint8_t *sdu_payload;
	uint32_t time_offset;
	bool zero_length_sdu;
	isoal_status_t err;
	bool padding_pdu;
	uint8_t ll_id;

	session     = &source->session;
	pp          = &source->pdu_production;
	padding_pdu = false;
	err         = ISOAL_STATUS_OK;
	time_offset = 0;

	packet_available = tx_sdu->size;
	sdu_payload      = tx_sdu->dbuf;
	LL_ASSERT(sdu_payload);

	zero_length_sdu = (packet_available == 0 &&
		tx_sdu->sdu_state == BT_ISO_SINGLE);

	if (tx_sdu->sdu_state == BT_ISO_START ||
		tx_sdu->sdu_state == BT_ISO_SINGLE) {
		/* Start of a new SDU */

		/* Initialize to info provided in SDU */
		uint32_t actual_grp_ref_point = tx_sdu->grp_ref_point;
		uint64_t actual_event = tx_sdu->target_event;
		bool time_diff_valid = false;
		uint32_t time_diff = 0;

		/* Update sequence number for received SDU
		 *
		 * BT Core V5.3 : Vol 6 Low Energy Controller : Part G IS0-AL:
		 * 2 ISOAL Features :
		 * SDUs received by the ISOAL from the upper layer shall be
		 * given a sequence number which is initialized to 0 when the
		 * CIS or BIS is created.
		 *
		 * NOTE: The upper layer may synchronize its sequence number
		 * with the sequence number in the ISOAL once the Datapath is
		 * configured and the link is established.
		 */
		session->seqn++;

		/* Reset PDU production state */
		pp->pdu_state = BT_ISO_START;

		/* Update payload counter in case time has passed since the last
		 * SDU. This should mean that event count * burst number should
		 * be greater than the current payload number. In the event of
		 * an SDU interval smaller than the ISO interval, multiple SDUs
		 * will be sent in the same event. As such the current payload
		 * number should be retained. Payload numbers are indexed at 0
		 * and valid until the PDU is emitted.
		 */
		pp->payload_number = MAX(pp->payload_number,
			(tx_sdu->target_event * session->burst_number));

		/* Get actual event for this payload number */
		actual_event = pp->payload_number / session->burst_number;

		/* Get group reference point for this PDU based on the actual
		 * event being set. This might introduce some errors as the
		 * group refernce point for future events could drift. However
		 * as the time offset calculation requires an absolute value,
		 * this seems to be the best candidate.
		 */
		if (actual_event > tx_sdu->target_event) {
			actual_grp_ref_point = isoal_get_wrapped_time_us(tx_sdu->grp_ref_point,
				((actual_event - tx_sdu->target_event) * session->iso_interval *
					ISO_INT_UNIT_US));
		}

		/* Check if time stamp on packet is later than the group
		 * reference point and adjust targets. This could happen if the
		 * SDU has been time-stampped at the controller when received
		 * via HCI.
		 *
		 * BT Core V5.3 : Vol 6 Low Energy Controller : Part G IS0-AL:
		 * 3.1 Time_Offset in framed PDUs :
		 * The Time_Offset shall be a positive value.
		 */
		if (!isoal_get_time_diff(tx_sdu->time_stamp, actual_grp_ref_point, &time_diff) ||
			time_diff == 0) {
			/* Advance target to next event */
			actual_event++;
			actual_grp_ref_point = isoal_get_wrapped_time_us(actual_grp_ref_point,
							session->iso_interval * ISO_INT_UNIT_US);

			/* Set payload number */
			pp->payload_number = actual_event * session->burst_number;
		}

		/* Calculate the time offset */
		time_diff_valid = isoal_get_time_diff(tx_sdu->time_stamp,
					actual_grp_ref_point, &time_diff);
		LL_ASSERT(time_diff_valid);
		LL_ASSERT(time_diff > 0);
		/* Time difference must be less than the maximum possible
		 * time-offset of 24-bits.
		 */
		LL_ASSERT(time_diff <= 0x00FFFFFF);
		time_offset = time_diff;

		/* Store timing info for TX Sync command */
		session->tx_time_stamp = actual_grp_ref_point;
		session->tx_time_offset = time_offset;

		/* Reset PDU fragmentation count for this SDU */
		pp->pdu_cnt = 0;
	}

	/* PDUs should be created until the SDU fragment has been fragmented or if
	 * this is the last fragment of the SDU, until the required padding PDU(s)
	 * are sent.
	 */
	while ((err == ISOAL_STATUS_OK) &&
		((packet_available > 0) || padding_pdu || zero_length_sdu)) {
		const isoal_status_t err_alloc = isoal_tx_allocate_pdu(source, tx_sdu);
		struct isoal_pdu_produced  *pdu = &pp->pdu;

		err |= err_alloc;

		if (pp->pdu_state == BT_ISO_START) {
			/* Start of a new SDU. Segmentation header and time-offset
			 * should be inserted.
			 */
			err |= isoal_insert_seg_header_timeoffset(source,
								false, false,
								time_offset);
			pp->pdu_state = BT_ISO_CONT;
		} else if (!padding_pdu && pp->pdu_state == BT_ISO_CONT && pp->pdu_written == 0) {
			/* Continuing an SDU in a new PDU. Segmentation header
			 * alone should be inserted.
			 */
			err |= isoal_insert_seg_header_timeoffset(source,
								true, false,
								0);
		}

		/*
		 * For this PDU we can only consume of packet, bounded by:
		 *   - What can fit in the destination PDU.
		 *   - What remains of the packet.
		 */
		const size_t consume_len = MIN(
			packet_available,
			pp->pdu_available
		);

		/* End of the SDU fragment has been reached when the last of the
		 * SDU is packed into a PDU.
		 */
		bool end_of_sdu_frag = !padding_pdu &&
				((consume_len > 0 && consume_len == packet_available) ||
					zero_length_sdu);

		if (consume_len > 0) {
			err |= session->pdu_write(&pdu->contents,
						  pp->pdu_written,
						  sdu_payload,
						  consume_len);
			sdu_payload       += consume_len;
			pp->pdu_written   += consume_len;
			pp->pdu_available -= consume_len;
			packet_available  -= consume_len;
		}

		if (end_of_sdu_frag) {
			/* Each PDU will carry the number of completed SDU
			 * fragments contained in that PDU.
			 */
			pp->sdu_fragments++;
		}

		/* End of the SDU is reached at the end of the last SDU fragment
		 * or if this is a single fragment SDU
		 */
		bool end_of_sdu = (packet_available == 0) &&
				((tx_sdu->sdu_state == BT_ISO_SINGLE) ||
					(tx_sdu->sdu_state == BT_ISO_END));
		/* Update complete flag in last segmentation header */
		err |= isoal_update_seg_header_cmplt_length(source, end_of_sdu, consume_len);

		/* LLID is fixed for framed PDUs */
		ll_id = PDU_BIS_LLID_FRAMED;

		/* If there isn't sufficient usable space then release the
		 * PDU when the end of the SDU is reached, instead of waiting
		 * for the next SDU.
		 */
		bool release_pdu = end_of_sdu && (pp->pdu_available <= ISOAL_TX_SEGMENT_MIN_SIZE);
		const isoal_status_t err_emit = isoal_tx_try_emit_pdu(source, release_pdu, ll_id);

		err |= err_emit;

		/* TODO: Send padding PDU(s) if required
		 *
		 * BT Core V5.3 : Vol 6 Low Energy Controller : Part G IS0-AL:
		 * 2 ISOAL Features :
		 * Padding is required when the data does not add up to the
		 * configured number of PDUs that are specified in the BN
		 * parameter per CIS or BIS event.
		 *
		 * When padding PDUs as opposed to null PDUs are required for
		 * framed production is not clear.
		 */
		padding_pdu = false;
		zero_length_sdu = false;
	}

	return err;
}

/**
 * @brief  Handle preparation of the given source before commencing TX on the
 *         specified event (only for framed sources)
 * @param  source_hdl   Handle of source to prepare
 * @param  event_count  Event number source should be prepared for
 * @return              Status of operation
 */
static isoal_status_t isoal_tx_framed_event_prepare_handle(isoal_source_handle_t source_hdl,
							   uint64_t event_count)
{
	struct isoal_source_session *session;
	struct isoal_pdu_production *pp;
	struct isoal_source *source;
	uint64_t last_event_payload;
	isoal_status_t err;

	err = ISOAL_STATUS_OK;

	source = &isoal_global.source_state[source_hdl];
	session = &source->session;
	pp = &source->pdu_production;
	last_event_payload = (session->burst_number * (event_count + 1)) - 1;

	if (pp->pdu_available > 0 &&
		pp->payload_number <= last_event_payload) {
		/* Pending PDU that should be released for framed TX */
		err = isoal_tx_try_emit_pdu(source, true, PDU_BIS_LLID_FRAMED);
	}

	if (pp->payload_number < last_event_payload + 1) {
		pp->payload_number = last_event_payload + 1;
	}

	return err;
}

/**
 * @brief Deep copy a SDU, fragment into PDU(s)
 * @details Fragmentation will occur individually for every enabled source
 *
 * @param source_hdl[in] Handle of destination source
 * @param tx_sdu[in]     SDU along with packet boudary state
 * @return Status
 */
isoal_status_t isoal_tx_sdu_fragment(isoal_source_handle_t source_hdl,
				     struct isoal_sdu_tx *tx_sdu)
{
	struct isoal_source_session *session;
	struct isoal_source *source;
	isoal_status_t err;

	source = &isoal_global.source_state[source_hdl];
	session = &source->session;
	err = ISOAL_STATUS_ERR_PDU_ALLOC;

	/* Set source context active to mutually exclude ISO Event prepare
	 * kick.
	 */
	source->context_active = true;

	if (source->pdu_production.mode != ISOAL_PRODUCTION_MODE_DISABLED) {
		/* BT Core V5.3 : Vol 6 Low Energy Controller : Part G IS0-AL:
		 * 2 ISOAL Features :
		 * (1) Unframed PDUs shall only be used when the ISO_Interval
		 *     is equal to or an integer multiple of the SDU_Interval
		 *     and a constant time offset alignment is maintained
		 *     between the SDU generation and the timing in the
		 *     isochronous transport.
		 * (2) When the Host requests the use of framed PDUs, the
		 *     Controller shall use framed PDUs.
		 */
		if (source->session.framed) {
			err = isoal_tx_framed_produce(source, tx_sdu);
		} else {
			err = isoal_tx_unframed_produce(source, tx_sdu);
		}
	}

	source->context_active = false;

	if (source->timeout_trigger) {
		source->timeout_trigger = false;
		if (session->framed) {
			isoal_tx_framed_event_prepare_handle(source_hdl,
						source->timeout_event_count);
		}
	}

	return err;
}

void isoal_tx_pdu_release(isoal_source_handle_t source_hdl,
			  struct node_tx_iso *node_tx)
{
	struct isoal_source *source = &isoal_global.source_state[source_hdl];

	if (source && source->session.pdu_release) {
		source->session.pdu_release(node_tx, source->session.handle,
					    ISOAL_STATUS_OK);
	}
}

/**
 * @brief  Get information required for HCI_LE_Read_ISO_TX_Sync
 * @param  source_hdl Source handle linked to handle provided in HCI message
 * @param  seq        Packet Sequence number of last SDU
 * @param  timestamp  CIG / BIG reference point of last SDU
 * @param  offset     Time-offset (Framed) / 0 (Unframed) of last SDU
 * @return            Operation status
 */
isoal_status_t isoal_tx_get_sync_info(isoal_source_handle_t source_hdl,
				      uint16_t *seq,
				      uint32_t *timestamp,
				      uint32_t *offset)
{
	if (isoal_check_source_hdl_valid(source_hdl) == ISOAL_STATUS_OK) {
		struct isoal_source_session *session;

		session = &isoal_global.source_state[source_hdl].session;

		/* BT Core V5.3 : Vol 4 HCI : Part E HCI Functional Spec:
		 * 7.8.96 LE Read ISO TX Sync Command:
		 * If the Host issues this command before an SDU had been transmitted by
		 * the Controller, then Controller shall return the error code Command
		 * Disallowed.
		 */
		if (session->seqn > 0) {
			*seq = session->seqn;
			*timestamp = session->tx_time_stamp;
			*offset = session->tx_time_offset;
			return ISOAL_STATUS_OK;
		}
	}

	return ISOAL_STATUS_ERR_UNSPECIFIED;
}

/**
 * @brief  Incoming prepare request before commencing TX for the specified
 *         event
 * @param  source_hdl   Handle of source to prepare
 * @param  event_count  Event number source should be prepared for
 * @return              Status of operation
 */
void isoal_tx_event_prepare(isoal_source_handle_t source_hdl,
			    uint64_t event_count)
{
	struct isoal_source_session *session;
	struct isoal_source *source;

	source = &isoal_global.source_state[source_hdl];
	session = &source->session;

	/* Store prepare timeout information and check if fragmentation context
	 * is active.
	 */
	source->timeout_event_count = event_count;
	source->timeout_trigger = true;
	if (source->context_active) {
		return;
	}
	source->timeout_trigger = false;

	if (session->framed) {
		isoal_tx_framed_event_prepare_handle(source_hdl, event_count);
	}
}

#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */
