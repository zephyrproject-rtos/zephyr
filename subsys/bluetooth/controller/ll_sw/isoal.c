/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <zephyr/types.h>
#include <sys/types.h>
#include <toolchain.h>
#include <sys/util.h>

#include "util/memq.h"
#include "pdu.h"
#include "lll.h"
#include "isoal.h"

#define LOG_MODULE_NAME bt_ctlr_isoal
#include "common/log.h"
#include "hal/debug.h"

/* TODO this must be taken from a Kconfig */
#define ISOAL_SINKS_MAX   (4)

/** Allocation state */
typedef uint8_t isoal_alloc_state_t;
#define ISOAL_ALLOC_STATE_FREE            ((isoal_alloc_state_t) 0x00)
#define ISOAL_ALLOC_STATE_TAKEN           ((isoal_alloc_state_t) 0x01)

static struct
{
	isoal_alloc_state_t  sink_allocated[ISOAL_SINKS_MAX];
	struct isoal_sink    sink_state[ISOAL_SINKS_MAX];
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
	if (err) {
		return err;
	}

	return err;
}

/** Clean up and reinitialize */
isoal_status_t isoal_reset(void)
{
	isoal_status_t err = ISOAL_STATUS_OK;

	err = isoal_init_reset();
	if (err) {
		return err;
	}

	return err;
}

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
	for (i = 0; i < ISOAL_SINKS_MAX; i++) {
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
}

/**
 * @brief Create a new sink
 *
 * @param hdl[out]        Handle to new sink
 * @param cis[in]         Poiner to iso stream
 * @param sdu_alloc[in]   Callback of SDU allocator
 * @param sdu_emit[in]    Callback of SDU emitter
 * @return ISOAL_STATUS_OK if we could create a new sink; otherwise ISOAL_STATUS_ERR_SINK_ALLOC
 */
isoal_status_t isoal_sink_create(
	isoal_sink_handle_t      *hdl,
	struct ll_conn_iso_stream *cis,
	isoal_sink_sdu_alloc_cb  sdu_alloc,
	isoal_sink_sdu_emit_cb   sdu_emit,
	isoal_sink_sdu_write_cb  sdu_write)
{
	isoal_status_t err = ISOAL_STATUS_OK;

	/* Allocate a new sink */
	err = isoal_sink_allocate(hdl);
	if (err) {
		return err;
	}

	isoal_global.sink_state[*hdl].session.cis = cis;

	/* Remember the platform-specific callbacks */
	isoal_global.sink_state[*hdl].session.sdu_alloc = sdu_alloc;
	isoal_global.sink_state[*hdl].session.sdu_emit  = sdu_emit;
	isoal_global.sink_state[*hdl].session.sdu_write = sdu_write;

	/* Initialize running seq number to zero */
	isoal_global.sink_state[*hdl].session.seqn = 0;

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

static uint8_t isoal_rx_packet_classify(const struct isoal_pdu_rx *pdu_meta)
{
	/* LLID location is the same for both CIS and BIS PDUs */
	uint8_t llid = pdu_meta->pdu->cis.ll_id;

	return llid;
}

/* Obtain destination SDU */
static isoal_status_t isoal_rx_allocate_sdu(struct isoal_sink *sink,
					    const struct isoal_pdu_rx *pdu_meta)
{
	isoal_status_t err = ISOAL_STATUS_OK;
	struct isoal_sdu_produced *sdu = &sink->sdu_production.sdu;

	/* Allocate a SDU if the previous was filled (thus sent) */
	const bool sdu_complete = (sink->sdu_production.sdu_available == 0);

	if (sdu_complete) {
		/* Allocate new clean SDU buffer */
		err = sink->session.sdu_alloc(
			sink,
			pdu_meta,      /* [in]  PDU origin may determine buffer */
			&sdu->contents  /* [out] Updated with pointer and size */
		);

		/* Nothing has been written into buffer yet */
		sink->sdu_production.sdu_written   = 0;
		sink->sdu_production.sdu_available = sdu->contents.size;
		LL_ASSERT(sdu->contents.size > 0);

		/* Remember meta data */
		sdu->status = pdu_meta->meta->status;
		sdu->timestamp = pdu_meta->meta->timestamp;
		/* Get seq number from session counter, and increase SDU counter */
		sdu->seqn = sink->session.seqn++;
	}

	return err;
}

static isoal_status_t isoal_rx_try_emit_sdu(struct isoal_sink *sink, bool end_of_sdu)
{
	isoal_status_t err = ISOAL_STATUS_OK;
	struct isoal_sdu_produced *sdu = &sink->sdu_production.sdu;

	/* Emit a SDU */
	const bool sdu_complete = (sink->sdu_production.sdu_available == 0) || end_of_sdu;

	if (end_of_sdu) {
		sink->sdu_production.sdu_available = 0;
	}

	if (sdu_complete) {
		uint8_t next_state = BT_ISO_START;

		switch (sink->sdu_production.sdu_state) {
		case BT_ISO_START:
			if (end_of_sdu) {
				sink->sdu_production.sdu_state = BT_ISO_SINGLE;
				next_state = BT_ISO_START;
			} else {
				sink->sdu_production.sdu_state = BT_ISO_START;
				next_state = BT_ISO_CONT;
			}
			break;
		case BT_ISO_CONT:
			if (end_of_sdu) {
				sink->sdu_production.sdu_state  = BT_ISO_END;
				next_state = BT_ISO_START;
			} else {
				sink->sdu_production.sdu_state  = BT_ISO_CONT;
				next_state = BT_ISO_CONT;
			}
			break;
		case BT_ISO_END:
		case BT_ISO_SINGLE:
		default:
			LL_ASSERT(0);
			break;
		}
		err = sink->session.sdu_emit(sink, sdu);

		/* update next state */
		sink->sdu_production.sdu_state = next_state;
	}

	return err;
}

static isoal_status_t isoal_rx_append_to_sdu(struct isoal_sink *sink,
					     const struct isoal_pdu_rx *pdu_meta,
					     bool is_end_fragment)
{
	isoal_status_t err = ISOAL_STATUS_OK;
	const uint8_t *pdu_payload          = pdu_meta->pdu->cis.payload;
	/* Note length field in same location for both CIS and BIS */
	isoal_pdu_len_t packet_available    = pdu_meta->pdu->cis.length;

	/* TODO error handling and lost packet */

	/* While there is something left of the packet to consume */
	while (packet_available > 0) {
		const isoal_status_t err_alloc = isoal_rx_allocate_sdu(sink, pdu_meta);
		struct isoal_sdu_produced *sdu = &sink->sdu_production.sdu;

		err |= err_alloc;

		/*
		 * For this SDU we can only consume of packet, bounded by:
		 *   - What can fit in the destination SDU.
		 *   - What remains of the packet.
		 */
		const size_t consume_len = MIN(
			packet_available,
			sink->sdu_production.sdu_available
		);

		LL_ASSERT(sdu->contents.dbuf);
		LL_ASSERT(pdu_payload);

		sdu->status |= pdu_meta->meta->status;

		if (pdu_meta->meta->status == ISOAL_PDU_STATUS_VALID) {
			err |= sink->session.sdu_write(sdu->contents.dbuf,
						       pdu_payload,
						       consume_len);
		}
		pdu_payload += consume_len;
		sink->sdu_production.sdu_written   += consume_len;
		sink->sdu_production.sdu_available -= consume_len;
		packet_available                   -= consume_len;

		bool end_of_sdu = (packet_available == 0) && is_end_fragment;

		const isoal_status_t err_emit = isoal_rx_try_emit_sdu(sink, end_of_sdu);

		err |= err_emit;
	}

	LL_ASSERT(packet_available == 0);
	return err;
}


/**
 * @brief Consume a PDU: Copy contents into SDU(s) and emit to a sink
 * @details Destination sink may have an already partially built SDU
 *
 * @param sink[in,out]    Destination sink with bookkeeping state
 * @param pdu_meta[out]  PDU with meta information (origin, timing, status)
 *
 * @return TODO
 */
static isoal_status_t isoal_rx_packet_consume(struct isoal_sink *sink,
					      const struct isoal_pdu_rx *pdu_meta)
{
	isoal_pdu_cnt_t id, prev_id;
	isoal_status_t err = ISOAL_STATUS_OK;
	const uint8_t llid = isoal_rx_packet_classify(pdu_meta);

	switch (sink->sdu_production.fsm) {

	case ISOAL_START:
		/* State assumes First PDU of new SDU */
		sink->sdu_production.prev_pdu_id = pdu_meta->meta->payload_number;
		sink->sdu_production.sdu_state = BT_ISO_START;
		if (llid == PDU_BIS_LLID_START_CONTINUE) {
			/* PDU contains first fragment of an SDU */
			err |= isoal_rx_append_to_sdu(sink, pdu_meta, false);
			sink->sdu_production.fsm = ISOAL_CONTINUE;
		} else if (llid == PDU_BIS_LLID_COMPLETE_END) {
			/* PDU contains complete SDU */
			err |= isoal_rx_append_to_sdu(sink, pdu_meta, true);
			sink->sdu_production.fsm = ISOAL_START;
		} else {
			/* Unsupported case */
			LL_ASSERT(0);
			/* TODO error handling */
			break;
		}
		break;

	case ISOAL_CONTINUE:
		/* State assumes that at least one PDU has been seen of fragmented SDU */
		id = pdu_meta->meta->payload_number;
		prev_id = sink->sdu_production.prev_pdu_id;
		if (id != (prev_id+1)) {
			/* Id not as expexted, a PDU could have been lost */
			LL_ASSERT(0);
			/* TODO error handling? Mark SDU as incomplete? break? */
		}
		sink->sdu_production.prev_pdu_id = id;
		if (llid == PDU_BIS_LLID_START_CONTINUE) {
			/* PDU contains a continuation (neither start of end) fragment of SDU */
			err |= isoal_rx_append_to_sdu(sink, pdu_meta, false);
			sink->sdu_production.fsm = ISOAL_CONTINUE;
		} else if (llid == PDU_BIS_LLID_COMPLETE_END) {
			/* PDU contains end fragment of a fragmented SDU */
			err |= isoal_rx_append_to_sdu(sink, pdu_meta, true);
			sink->sdu_production.fsm = ISOAL_START;
		} else  {
			/* Unsupported case */
			LL_ASSERT(0);
			/* TODO error handling */
			break;
		}
		break;
	}

	return err;
}

/**
 * @brief Deep copy a PDU, recombine into SDU(s)
 * @details Recombination will occur individually for every enabled sink
 *
 * @param sink_hdl[in] Handle of destination sink
 * @param pdu_meta[in] PDU along with meta information (origin, timing, status)
 * @return TODO
 */
isoal_status_t isoal_rx_pdu_recombine(isoal_sink_handle_t sink_hdl,
				      const struct isoal_pdu_rx *pdu_meta)
{
	struct isoal_sink *sink = &isoal_global.sink_state[sink_hdl];
	isoal_status_t err = ISOAL_STATUS_ERR_SDU_ALLOC;

	if (sink->sdu_production.mode != ISOAL_PRODUCTION_MODE_DISABLED) {
		err = isoal_rx_packet_consume(sink, pdu_meta);
	}

	return err;
}
