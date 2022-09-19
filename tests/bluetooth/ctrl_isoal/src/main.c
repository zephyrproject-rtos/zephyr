/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Run this test from zephyr directory as:
 *
 *     ./scripts/twister --coverage -p native_posix -v -T tests/bluetooth/ctrl_isoal/
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/fff.h>
#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

/* Include the DUT */
#include "ll_sw/isoal.c"

DEFINE_FFF_GLOBALS;

/* #define DEBUG_TEST			(1) */
/* #define DEBUG_TRACE			(1) */

#define TEST_RX_PDU_PAYLAOD_MAX (40)
#define TEST_RX_PDU_SIZE	(TEST_RX_PDU_PAYLAOD_MAX + 2)

#define TEST_RX_SDU_FRAG_PAYLOAD_MAX (100)

#define LLID_TO_STR(llid)                                                                          \
	(llid == PDU_BIS_LLID_COMPLETE_END                                                         \
		 ? "COMPLETE_END"                                                                  \
		 : (llid == PDU_BIS_LLID_START_CONTINUE                                            \
			    ? "START_CONT"                                                         \
			    : (llid == PDU_BIS_LLID_FRAMED                                         \
				       ? "FRAMED"                                                  \
				       : (llid == PDU_BIS_LLID_CTRL ? "CTRL" : "?????"))))

#define DU_ERR_TO_STR(err)                                                                         \
	(err == 1 ? "Bit Errors" : (err == 2 ? "Data Lost" : (err == 0 ? "OK" : "Undefined!")))

#define STATE_TO_STR(s)                                                                            \
	(s == BT_ISO_SINGLE                                                                        \
		 ? "SINGLE"                                                                        \
		 : (s == BT_ISO_START                                                              \
			    ? "START"                                                              \
			    : (s == BT_ISO_CONT ? "CONT" : (s == BT_ISO_END ? "END" : "???"))))

#define ROLE_TO_STR(s)                                                                             \
	(s == BT_ROLE_BROADCAST                                                                    \
		 ? "Broadcast"                                                                     \
		 : (role == BT_CONN_ROLE_PERIPHERAL                                                \
			    ? "Peripheral"                                                         \
			    : (role == BT_CONN_ROLE_CENTRAL ? "Central" : "Undefined")))

#define FSM_TO_STR(s)                                                                              \
	(s == ISOAL_START ? "START"                                                                \
			  : (s == ISOAL_CONTINUE ? "CONTINUE"                                      \
						 : (s == ISOAL_ERR_SPOOL ? "ERR SPOOL" : "???")))

struct rx_pdu_meta_buffer {
	struct isoal_pdu_rx pdu_meta;
	struct node_rx_iso_meta meta;
	uint8_t pdu[TEST_RX_PDU_SIZE];
};

struct rx_sdu_frag_buffer {
	uint16_t write_loc;
	uint8_t sdu[TEST_RX_SDU_FRAG_PAYLOAD_MAX];
};

/**
 * Intializes a RX PDU buffer
 * @param[in] buf Pointer to buffer structure
 */
static void init_rx_pdu_buffer(struct rx_pdu_meta_buffer *buf)
{
	memset(buf, 0, sizeof(struct rx_pdu_meta_buffer));
	buf->pdu_meta.meta = &buf->meta;
	buf->pdu_meta.pdu = (struct pdu_iso *)&buf->pdu[0];
}

/**
 * Initializes a RX SDU buffer
 * @param[in] buf Pointer to buffer structure
 */
static void init_rx_sdu_buffer(struct rx_sdu_frag_buffer *buf)
{
	memset(buf, 0, sizeof(struct rx_sdu_frag_buffer));
}

#if defined(DEBUG_TEST)
/**
 * Print contents of PDU
 * @param[in] pdu_meta Pointer to PDU structure including meta information
 */
static void debug_print_rx_pdu(struct isoal_pdu_rx *pdu_meta)
{
	zassert_not_null(pdu_meta, "");

	PRINT("PDU %04d (%10d) : %12s [%10s] %03d: ", (uint32_t)pdu_meta->meta->payload_number,
	      (uint32_t)pdu_meta->meta->timestamp, LLID_TO_STR(pdu_meta->pdu->ll_id),
	      DU_ERR_TO_STR(pdu_meta->meta->status), pdu_meta->pdu->length);

	for (int i = 0; i < pdu_meta->pdu->length; i++) {
		PRINT("%02x ", pdu_meta->pdu->payload[i]);
	}

	PRINT("\n");
}

/**
 * Print contents of RX SDU
 * @param[in] sink_ctx Sink context provided when SDU is emitted
 * @param[in] buf      SDU data buffer pointer
 */
static void debug_print_rx_sdu(const struct isoal_sink *sink_ctx, uint8_t *buf)
{
	zassert_not_null(sink_ctx, "");
	zassert_not_null(buf, "");

	uint16_t len = sink_ctx->sdu_production.sdu_written;

	PRINT("\n");
	PRINT("SDU %04d (%10d) : %12s [%10s] %03d: ", sink_ctx->sdu_production.sdu.seqn,
	      sink_ctx->sdu_production.sdu.timestamp,
	      STATE_TO_STR(sink_ctx->sdu_production.sdu_state),
	      DU_ERR_TO_STR(sink_ctx->sdu_production.sdu.status), len);
	for (int i = 0; i < len; i++) {
		PRINT("%02x ", buf[i]);
	}
	PRINT("\n");
	PRINT("\n");
}
#else  /* DEBUG_TEST */
static void debug_print_rx_pdu(struct isoal_pdu_rx *pdu_meta) {}
static void debug_print_rx_sdu(const struct isoal_sink *sink_ctx, uint8_t *buf) {}
#endif /* DEBUG_TEST */

#if defined(DEBUG_TRACE)
/**
 * Print function trace
 * @param func   Function name
 * @param status Status
 */
static void debug_trace_func_call(const uint8_t *func, const uint8_t *status)
{
	PRINT("\n");
	PRINT("%s :: %s\n", func, status);
}
#else  /* DEBUG_TRACE */
static void debug_trace_func_call(const uint8_t *func, const uint8_t *status) {}
#endif /* DEBUG_TRACE */

/**
 * Creates an unframed PDU fragment according to provided parameters
 * @param[in]  llid           LLID as Start / Continue or Complete / End
 * @param[in]  dataptr        Test data to fill PDU payload
 * @param[in]  length         Length of PDU payload
 * @param[in]  payload_number Payload number (Meta Information)
 * @param[in]  timestamp      PDU reception Time (Meta Information)
 * @param[in]  status         PDU data validity
 * @param[out] pdu_meta       PDU buffer including meta structure
 */
static void create_unframed_pdu(uint8_t llid, uint8_t *dataptr, uint8_t length,
				uint64_t payload_number, uint32_t timestamp, uint8_t status,
				struct isoal_pdu_rx *pdu_meta)
{
	zassert_not_null(pdu_meta, "");
	zassert_not_null(pdu_meta->meta, "");
	zassert_not_null(pdu_meta->pdu, "");

	memset(pdu_meta->meta, 0, sizeof(*pdu_meta->meta));
	memset(pdu_meta->pdu, 0, sizeof(*pdu_meta->pdu));

	pdu_meta->meta->payload_number = payload_number;
	pdu_meta->meta->timestamp = timestamp;
	pdu_meta->meta->status = status;

	pdu_meta->pdu->ll_id = llid;
	pdu_meta->pdu->length = length;
	memcpy(pdu_meta->pdu->payload, dataptr, length);

	debug_print_rx_pdu(pdu_meta);
}

/**
 * Insert a new segment in the given PDU
 * @param[In]     sc          !Start / Continuation bit
 * @param[In]     cmplt       Complete bit
 * @param[In]     time_offset Time Offset (us)
 * @param[In]     dataptr     Pointer to data to fill in segment
 * @param[In]     length      Length of data
 * @param[In/Out] pdu_meta    PDU structure including meta information
 * @return                    PDU data location index
 */
static uint16_t insert_segment(bool sc, bool cmplt, uint32_t time_offset, uint8_t *dataptr,
			       uint8_t length, struct isoal_pdu_rx *pdu_meta)
{
	struct pdu_iso_sdu_sh seg_hdr;
	uint16_t pdu_payload_size;
	uint8_t hdr_write_size;
	uint16_t pdu_data_loc;

	pdu_payload_size = pdu_meta->pdu->length + length + PDU_ISO_SEG_HDR_SIZE +
			   (sc ? 0 : PDU_ISO_SEG_TIMEOFFSET_SIZE);
	hdr_write_size = PDU_ISO_SEG_HDR_SIZE + (sc ? 0 : PDU_ISO_SEG_TIMEOFFSET_SIZE);
	memset(&seg_hdr, 0, sizeof(seg_hdr));

	zassert_true(pdu_payload_size <= TEST_RX_PDU_PAYLAOD_MAX, "pdu_payload_size (%d)",
		     pdu_payload_size);

	seg_hdr.sc = sc;
	seg_hdr.cmplt = cmplt;
	seg_hdr.length = length + (sc ? 0 : PDU_ISO_SEG_TIMEOFFSET_SIZE);

	if (!sc) {
		seg_hdr.timeoffset = time_offset;
	}

	memcpy(&pdu_meta->pdu->payload[pdu_meta->pdu->length], &seg_hdr, hdr_write_size);
	pdu_meta->pdu->length += hdr_write_size;

	memcpy(&pdu_meta->pdu->payload[pdu_meta->pdu->length], dataptr, length);
	pdu_data_loc = pdu_meta->pdu->length;
	pdu_meta->pdu->length += length;

	debug_print_rx_pdu(pdu_meta);

	return pdu_data_loc;
}

/**
 * Create and fill in base information for a framed PDU
 * @param[In]     payload_number Payload number (Meta Information)
 * @param[In]     timestamp      Adjusted RX time stamp (CIS anchorpoint)
 * @param[In]     status         PDU error status
 * @param[In/Out] pdu_meta       PDU structure including meta information
 */
static void create_framed_pdu_base(uint64_t payload_number, uint32_t timestamp, uint8_t status,
				   struct isoal_pdu_rx *pdu_meta)
{
	zassert_not_null(pdu_meta, "");
	zassert_not_null(pdu_meta->meta, "");
	zassert_not_null(pdu_meta->pdu, "");

	memset(pdu_meta->meta, 0, sizeof(*pdu_meta->meta));
	memset(pdu_meta->pdu, 0, sizeof(*pdu_meta->pdu));

	pdu_meta->meta->payload_number = payload_number;
	pdu_meta->meta->timestamp = timestamp;
	pdu_meta->meta->status = status;

	pdu_meta->pdu->ll_id = PDU_BIS_LLID_FRAMED;
	pdu_meta->pdu->length = 0;

	debug_print_rx_pdu(pdu_meta);
}

/**
 * Adds a single SDU framed segment to the given PDU
 * @param[In]     dataptr     Pointer to data to fill in segment
 * @param[In]     length      Length of data
 * @param[In]     time_offset Time offset
 * @param[In/Out] pdu_meta    PDU structure including meta information
 * @return                    PDU data location index
 */
static uint16_t add_framed_pdu_single(uint8_t *dataptr, uint8_t length, uint32_t time_offset,
				      struct isoal_pdu_rx *pdu_meta)
{
	zassert_not_null(pdu_meta, "");
	zassert_not_null(pdu_meta->meta, "");
	zassert_not_null(pdu_meta->pdu, "");

	return insert_segment(false, true, time_offset, dataptr, length, pdu_meta);
}

/**
 * Adds a starting SDU framed segment to the given PDU
 * @param[In]     dataptr     Pointer to data to fill in segment
 * @param[In]     length      Length of data
 * @param[In]     time_offset Time offset
 * @param[In/Out] pdu_meta    PDU structure including meta information
 * @return                    PDU data location index
 */
static uint16_t add_framed_pdu_start(uint8_t *dataptr, uint8_t length, uint32_t time_offset,
				     struct isoal_pdu_rx *pdu_meta)
{
	zassert_not_null(pdu_meta, "");
	zassert_not_null(pdu_meta->meta, "");
	zassert_not_null(pdu_meta->pdu, "");

	return insert_segment(false, false, time_offset, dataptr, length, pdu_meta);
}

/**
 * Adds a continuation SDU framed segment to the given PDU
 * @param[In]     dataptr     Pointer to data to fill in segment
 * @param[In]     length      Length of data
 * @param[In/Out] pdu_meta    PDU structure including meta information
 * @return                    PDU data location index
 */
static uint16_t add_framed_pdu_cont(uint8_t *dataptr, uint8_t length, struct isoal_pdu_rx *pdu_meta)
{
	zassert_not_null(pdu_meta, "");
	zassert_not_null(pdu_meta->meta, "");
	zassert_not_null(pdu_meta->pdu, "");

	return insert_segment(true, false, 0, dataptr, length, pdu_meta);
}

/**
 * Adds an end SDU framed segment to the given PDU
 * @param[In]     dataptr     Pointer to data to fill in segment
 * @param[In]     length      Length of data
 * @param[In/Out] pdu_meta    PDU structure including meta information
 * @return                    PDU data location index
 */
static uint16_t add_framed_pdu_end(uint8_t *dataptr, uint8_t length, struct isoal_pdu_rx *pdu_meta)
{
	zassert_not_null(pdu_meta, "");
	zassert_not_null(pdu_meta->meta, "");
	zassert_not_null(pdu_meta->pdu, "");

	return insert_segment(true, true, 0, dataptr, length, pdu_meta);
}

FAKE_VALUE_FUNC(isoal_status_t, sink_sdu_alloc_test, const struct isoal_sink *,
		const struct isoal_pdu_rx *, struct isoal_sdu_buffer *);

static struct {
	struct isoal_sdu_buffer *out[5];
	size_t buffer_size;
	size_t pos;

} custom_sink_sdu_alloc_test_output_buffer;

static void push_custom_sink_sdu_alloc_test_output_buffer(struct isoal_sdu_buffer *buf)
{
	custom_sink_sdu_alloc_test_output_buffer
		.out[custom_sink_sdu_alloc_test_output_buffer.buffer_size++] = buf;
	zassert_true(custom_sink_sdu_alloc_test_output_buffer.buffer_size <=
			     ARRAY_SIZE(custom_sink_sdu_alloc_test_output_buffer.out),
		     NULL);
}
/**
 * Callback test fixture to be provided for RX sink creation. Allocates a new
 * SDU buffer.
 * @param[in]  sink_ctx   Sink context provided by ISO-AL
 * @param[in]  valid_pdu  PDU currently being reassembled
 * @param[out] sdu_buffer SDU buffer information return structure
 * @return                Status of operation
 */
static isoal_status_t custom_sink_sdu_alloc_test(const struct isoal_sink *sink_ctx,
						 const struct isoal_pdu_rx *valid_pdu,
						 struct isoal_sdu_buffer *sdu_buffer)
{
	debug_trace_func_call(__func__, "IN");

	/* Return SDU buffer details as provided by the test */
	zassert_not_null(sdu_buffer, NULL);
	zassert_true(custom_sink_sdu_alloc_test_output_buffer.pos <
			     custom_sink_sdu_alloc_test_output_buffer.buffer_size,
		     NULL);
	memcpy(sdu_buffer,
	       custom_sink_sdu_alloc_test_output_buffer
		       .out[custom_sink_sdu_alloc_test_output_buffer.pos++],
	       sizeof(*sdu_buffer));

	return sink_sdu_alloc_test_fake.return_val;
}

FAKE_VALUE_FUNC(isoal_status_t, sink_sdu_emit_test, const struct isoal_sink *,
		const struct isoal_sdu_produced *);

/**
 * This handler is called by custom_sink_sdu_emit_test using the non pointer versions of the
 * function's arguments. The tests are asserting on the argument content, since sink_sdu_emit_test()
 * is called multiple times with the same pointer (but different content) this additional fake is
 * used to store the history of the content.
 */
FAKE_VOID_FUNC(sink_sdu_emit_test_handler, struct isoal_sink, struct isoal_sdu_produced);

/**
 * Callback test fixture to be provided for RX sink creation. Emits provided
 * SDU in buffer
 * @param[in]  sink_ctx  Sink context provided by ISO-AL
 * @param[in]  valid_sdu SDU buffer and details of SDU to be emitted
 * @return               Status of operation
 */
static isoal_status_t custom_sink_sdu_emit_test(const struct isoal_sink *sink_ctx,
						const struct isoal_sdu_produced *valid_sdu)
{
	debug_trace_func_call(__func__, "IN");

	debug_print_rx_sdu(sink_ctx, ((struct rx_sdu_frag_buffer *)valid_sdu->contents.dbuf)->sdu);
	sink_sdu_emit_test_handler(*sink_ctx, *valid_sdu);

	return sink_sdu_emit_test_fake.return_val;
}

FAKE_VALUE_FUNC(isoal_status_t, sink_sdu_write_test, void *, const uint8_t *, const size_t);
/**
 * Callback test fixture to be provided for RX sink creation. Writes provided
 * data into target SDU buffer.
 * @param  dbuf        SDU buffer (Includes current write location field)
 * @param  pdu_payload Current PDU being reassembled by ISO-AL
 * @param  consume_len Length of data to transfer
 * @return             Status of the operation
 */
static isoal_status_t custom_sink_sdu_write_test(void *dbuf, const uint8_t *pdu_payload,
						 const size_t consume_len)
{
	debug_trace_func_call(__func__, "IN");

#if defined(DEBUG_TEST)
	zassert_not_null(dbuf, "");
	zassert_not_null(pdu_payload, "");

	struct rx_sdu_frag_buffer *rx_sdu_frag_buf;

	rx_sdu_frag_buf = (struct rx_sdu_frag_buffer *)dbuf;
	memcpy(&rx_sdu_frag_buf->sdu[rx_sdu_frag_buf->write_loc], pdu_payload, consume_len);
	rx_sdu_frag_buf->write_loc += consume_len;
#endif

	return sink_sdu_write_test_fake.return_val;
}

/**
 * Cacluate RX latency based on role and framing
 * @param  role              Peripheral / Central / Broadcast
 * @param  framed            PDU framing (Framed / Unframed)
 * @param  flush_timeout     FT
 * @param  sdu_interval      SDU Interval (us)
 * @param  iso_interval_int  ISO Interval (Integer multiple of 1250us)
 * @param  stream_sync_delay CIS / BIS sync delay
 * @param  group_sync_delay  CIG / BIG sync delay
 * @return                   Latency (signed)
 */
static int32_t calc_rx_latency_by_role(uint8_t role, uint8_t framed, uint8_t flush_timeout,
				       uint32_t sdu_interval, uint16_t iso_interval_int,
				       uint32_t stream_sync_delay, uint32_t group_sync_delay)
{
	int32_t latency;
	uint32_t iso_interval;

	latency = 0;
	iso_interval = iso_interval_int * CONN_INT_UNIT_US;

	switch (role) {
	case BT_CONN_ROLE_PERIPHERAL:
		if (framed) {
			latency = stream_sync_delay + sdu_interval + (flush_timeout * iso_interval);
		} else {
			latency = stream_sync_delay + ((flush_timeout - 1) * iso_interval);
		}
		break;

	case BT_CONN_ROLE_CENTRAL:
		if (framed) {
			latency = stream_sync_delay - group_sync_delay;
		} else {
			latency = stream_sync_delay - group_sync_delay -
				  (((iso_interval / sdu_interval) - 1) * iso_interval);
		}
		break;

	case BT_ROLE_BROADCAST:
		if (framed) {
			latency = group_sync_delay + sdu_interval + iso_interval;
		} else {
			latency = group_sync_delay;
		}
		break;

	default:
		zassert_unreachable("Invalid role!");
		break;
	}

#if defined(DEBUG_TEST)
	PRINT("Latency %s calculated %dus.\n", framed ? "framed" : "unframed", latency);
	PRINT("\tFT %d\n\tISO Interval %dus\n\tSDU Interval %dus\n\tStream Sync Delay %dus\n"
	      "\tGroup Sync Delay %dus\n\n",
	      flush_timeout, iso_interval, sdu_interval, stream_sync_delay, group_sync_delay);
#endif

	return latency;
}

/**
 * Basic setup of a single sink for any RX test
 * @param  handle            Stream handle
 * @param  role              Peripheral / Central / Broadcast
 * @param  framed            PDU framing
 * @param  burst_number      BN
 * @param  flush_timeout     FT
 * @param  sdu_interval      SDU Interval (us)
 * @param  iso_interval_int  ISO Interval (integer multiple of 1250us)
 * @param  stream_sync_delay CIS / BIS sync delay
 * @param  group_sync_delay  CIG / BIG sync delay
 * @return                   Newly created sink handle
 */
static isoal_sink_handle_t basic_rx_test_setup(uint16_t handle, uint8_t role, uint8_t framed,
					       uint8_t burst_number, uint8_t flush_timeout,
					       uint32_t sdu_interval, uint16_t iso_interval_int,
					       uint32_t stream_sync_delay,
					       uint32_t group_sync_delay)
{
	isoal_sink_handle_t sink_hdl;
	isoal_status_t err;

	ztest_set_assert_valid(false);

	err = isoal_init();
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	err = isoal_reset();
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Create a sink based on global parameters */
	err = isoal_sink_create(handle, role, framed, burst_number, flush_timeout, sdu_interval,
				iso_interval_int, stream_sync_delay, group_sync_delay,
				sink_sdu_alloc_test, sink_sdu_emit_test, sink_sdu_write_test,
				&sink_hdl);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Enable newly created sink */
	isoal_sink_enable(sink_hdl);

	return sink_hdl;
}

/**
 * Initialize the given test data buffer with a ramp pattern
 * @param buf  Test data buffer pointer
 * @param size Length of test data
 */
static void init_test_data_buffer(uint8_t *buf, uint16_t size)
{
	zassert_not_null(buf, "");

	for (uint16_t i = 0; i < size; i++) {
		buf[i] = (uint8_t)(i & 0x00FF);
	}
}

/**
 * Test Suite	:	RX basic test
 *
 * Test creating and destroying sinks upto the maximum with randomized
 * configuration parameters.
 */
ZTEST(test_rx_basics, test_sink_create_destroy)
{
	isoal_sink_handle_t sink_hdl[CONFIG_BT_CTLR_ISOAL_SINKS];
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	uint8_t sdu_interval_int;
	uint8_t iso_interval_int;
	uint8_t flush_timeout;
	uint32_t iso_interval;
	uint32_t sdu_interval;
	uint8_t burst_number;
	uint8_t pdus_per_sdu;
	isoal_status_t res;
	uint16_t handle;
	int32_t latency;
	bool framed;

	res = isoal_init();
	zassert_equal(res, ISOAL_STATUS_OK, "res=0x%02x", res);

	res = isoal_reset();
	zassert_equal(res, ISOAL_STATUS_OK, "res=0x%02x", res);

	for (int role = 0; role <= 3; role++) {
		/* 0 Central
		 * 1 Peripheral
		 * 2 Broadcast
		 * 3 Undefined
		 */
		handle = 0x8000;
		burst_number = 0;
		flush_timeout = 1;
		framed = false;
		sdu_interval_int = 1;
		iso_interval_int = 1;
		iso_interval = iso_interval_int * CONN_INT_UNIT_US;
		sdu_interval = sdu_interval_int * CONN_INT_UNIT_US;
		stream_sync_delay = iso_interval - 200;
		group_sync_delay = iso_interval - 50;
		latency = 0;

		ztest_set_assert_valid(false);

		for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SINKS; i++) {
			res = ISOAL_STATUS_ERR_UNSPECIFIED;
			sink_hdl[i] = 0xFF;

			pdus_per_sdu = (burst_number * sdu_interval) / iso_interval;

			switch (role) {
			case BT_CONN_ROLE_PERIPHERAL:
			case BT_CONN_ROLE_CENTRAL:
			case BT_ROLE_BROADCAST:
				latency = calc_rx_latency_by_role(
					role, framed, flush_timeout, sdu_interval, iso_interval_int,
					stream_sync_delay, group_sync_delay);
				break;

			default:
				ztest_set_assert_valid(true);
				break;
			}

			res = isoal_sink_create(handle, role, framed, burst_number, flush_timeout,
						sdu_interval, iso_interval_int, stream_sync_delay,
						group_sync_delay, sink_sdu_alloc_test,
						sink_sdu_emit_test, sink_sdu_write_test,
						&sink_hdl[i]);

			zassert_equal(isoal_global.sink_allocated[sink_hdl[i]],
				      ISOAL_ALLOC_STATE_TAKEN, "");

			zassert_equal(isoal_global.sink_state[sink_hdl[i]].session.pdus_per_sdu,
				      pdus_per_sdu,
				      "%s pdus_per_sdu %d should be %d for:\n\tBN %d\n\tFT %d\n"
				      "\tISO Interval %dus\n\tSDU Interval %dus\n"
				      "\tStream Sync Delay %dus\n\tGroup Sync Delay %dus",
				      (framed ? "Framed" : "Unframed"),
				      isoal_global.sink_state[sink_hdl[i]].session.pdus_per_sdu,
				      pdus_per_sdu, burst_number, flush_timeout, iso_interval,
				      sdu_interval, stream_sync_delay, group_sync_delay);

			if (framed) {
				zassert_equal(
					isoal_global.sink_state[sink_hdl[i]].session.latency_framed,
					latency, "%s latency framed %d should be %d",
					ROLE_TO_STR(role),
					isoal_global.sink_state[sink_hdl[i]].session.latency_framed,
					latency);
			} else {
				zassert_equal(isoal_global.sink_state[sink_hdl[i]]
						      .session.latency_unframed,
					      latency, "%s latency unframed %d should be %d",
					      ROLE_TO_STR(role),
					      isoal_global.sink_state[sink_hdl[i]]
						      .session.latency_unframed,
					      latency);
			}

			zassert_equal(res, ISOAL_STATUS_OK, "Sink %d in role %s creation failed!",
				      i, ROLE_TO_STR(role));

			framed = !framed;
			burst_number++;
			flush_timeout = (flush_timeout % 3) + 1;
			sdu_interval_int++;
			iso_interval_int = iso_interval_int * sdu_interval_int;
			sdu_interval = (sdu_interval_int * CONN_INT_UNIT_US) - (framed ? 100 : 0);
			iso_interval = iso_interval_int * CONN_INT_UNIT_US;
			stream_sync_delay = iso_interval - (200 * i);
			group_sync_delay = iso_interval - 50;
		}

		/* Destroy in order */
		for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SINKS; i++) {
			isoal_sink_destroy(sink_hdl[i]);

			zassert_equal(isoal_global.sink_allocated[sink_hdl[i]],
				      ISOAL_ALLOC_STATE_FREE, "Sink destruction failed!");

			zassert_equal(isoal_global.sink_state[sink_hdl[i]].sdu_production.mode,
				      ISOAL_PRODUCTION_MODE_DISABLED, "Sink disable failed!");
		}
	}
}

/**
 * Test Suite	:	RX basic test
 *
 * Test error return on exceeding the maximum number of sinks available.
 */
ZTEST(test_rx_basics, test_sink_create_err)
{
	isoal_sink_handle_t sink_hdl[CONFIG_BT_CTLR_ISOAL_SINKS + 1];
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint8_t flush_timeout;
	uint32_t sdu_interval;
	uint8_t burst_number;
	isoal_status_t res;
	uint16_t handle;
	uint8_t role;
	bool framed;

	handle = 0x8000;
	role = BT_CONN_ROLE_PERIPHERAL;
	burst_number = 1;
	flush_timeout = 1;
	framed = false;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	stream_sync_delay = CONN_INT_UNIT_US - 200;
	group_sync_delay = CONN_INT_UNIT_US - 50;

	res = isoal_init();
	zassert_equal(res, ISOAL_STATUS_OK, "res=0x%02x", res);

	res = isoal_reset();
	zassert_equal(res, ISOAL_STATUS_OK, "res=0x%02x", res);

	for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SINKS; i++) {
		res = isoal_sink_create(handle, role, framed, burst_number, flush_timeout,
					sdu_interval, iso_interval_int, stream_sync_delay,
					group_sync_delay, sink_sdu_alloc_test, sink_sdu_emit_test,
					sink_sdu_write_test, &sink_hdl[i]);

		zassert_equal(res, ISOAL_STATUS_OK, "Sink %d in role %s creation failed!", i,
			      ROLE_TO_STR(role));
	}

	res = isoal_sink_create(handle, role, framed, burst_number, flush_timeout, sdu_interval,
				iso_interval_int, stream_sync_delay, group_sync_delay,
				sink_sdu_alloc_test, sink_sdu_emit_test, sink_sdu_write_test,
				&sink_hdl[CONFIG_BT_CTLR_ISOAL_SINKS]);

	zassert_equal(res, ISOAL_STATUS_ERR_SINK_ALLOC,
		      "Sink creation did not return error as expected!");
}

/**
 * Test Suite	:	RX basic test
 *
 * Test assertion when attempting to retrieve sink params for an invalid sink
 * handle.
 */
ZTEST(test_rx_basics, test_sink_invalid_ref)
{
	ztest_set_assert_valid(true);

	isoal_get_sink_param_ref(99);

	ztest_set_assert_valid(false);
}

/**
 * Test Suite	:	RX basic test
 *
 * Test error return when receiving PDUs for a disabled sink.
 */
ZTEST(test_rx_basics, test_sink_disable)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[23];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	latency = payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size = 0;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* Disable the sink */
	isoal_sink_disable(sink_hdl);

	/* Send SDU in a single PDU */
	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of a single valid RX PDU into an SDU.
 */
ZTEST(test_rx_unframed, test_rx_unframed_single_pdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[23];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	latency = payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size = 23;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* Send SDU in a single PDU */
	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should be emitted as it is complete */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	/* No padding PDUs expected, so to waiting for start fragment */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);

	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of two valid RX PDU into a single SDU.
 */
ZTEST(test_rx_unframed, test_rx_unframed_dbl_pdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[23];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* Send PDU with start fragment */
	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	/* Next state should wait for continuation or end */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	/* Send PDU with end fragment  */
	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	/* As two PDUs per SDU, no padding is expected */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of three SDUs where the end of the first two were not seen
 */
ZTEST(test_rx_unframed, test_rx_unframed_dbl_split)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[53];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US / 2;
	BN = 4;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU 1 - PDU 1 -----------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 53);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);

	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* SDU 1 - PDU 2 -----------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU emitted with errors as end fragment was not seen */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* SDU 2 - PDU 3 -----------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	pdu_timestamp += 200;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* SDU 2 - PDU 4 */
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU emitted with errors as end fragment was not seen */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* SDU 3 - PDU 5 -----------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	pdu_timestamp = 9249 + CONN_INT_UNIT_US;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	/* Expecting padding PDU as PDUs per SDU is 2 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of one SDUs in five fragments
 */
ZTEST(test_rx_unframed, test_rx_unframed_multi_split)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[53];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 5;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 53);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 5 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of one SDUs sent in one PDU when the SDU fragment size is
 * small, resulting in multiple SDU fragments released during reassembly
 */
ZTEST(test_rx_unframed, test_rx_unframed_long_pdu_short_sdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[40];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 40);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = 20;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 40;
	sdu_size = 20;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* SDU 1 */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU 2 */
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	sdu_size = 20;

	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_history[0], NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_history[0], NULL);
	zassert_equal(20, sink_sdu_write_test_fake.arg2_history[0]);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_history[1], NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + 20], sink_sdu_write_test_fake.arg1_history[1],
			  NULL);
	zassert_equal(20, sink_sdu_write_test_fake.arg2_history[1]);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl],
			  sink_sdu_emit_test_fake.arg0_history[0], NULL);
	zassert_equal(BT_ISO_START,
		      sink_sdu_emit_test_handler_fake.arg0_history[0].sdu_production.sdu_state,
		      NULL);
	zassert_equal(sdu_size,
		      sink_sdu_emit_test_handler_fake.arg0_history[0].sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID,
		      sink_sdu_emit_test_handler_fake.arg1_history[0].status, NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_history[0].timestamp,
		      NULL);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_history[0].seqn);
	zassert_equal(sdu_buffer.dbuf,
		      sink_sdu_emit_test_handler_fake.arg1_history[0].contents.dbuf, NULL);
	zassert_equal(sdu_buffer.size,
		      sink_sdu_emit_test_handler_fake.arg1_history[0].contents.size, NULL);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl],
			  sink_sdu_emit_test_fake.arg0_history[1], NULL);
	zassert_equal(BT_ISO_END,
		      sink_sdu_emit_test_handler_fake.arg0_history[1].sdu_production.sdu_state,
		      NULL);
	zassert_equal(sdu_size,
		      sink_sdu_emit_test_handler_fake.arg0_history[1].sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID,
		      sink_sdu_emit_test_handler_fake.arg1_history[1].status, NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_history[1].timestamp,
		      NULL);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_history[1].seqn);
	zassert_equal(sdu_buffer.dbuf,
		      sink_sdu_emit_test_handler_fake.arg1_history[1].contents.dbuf, NULL);
	zassert_equal(sdu_buffer.size,
		      sink_sdu_emit_test_handler_fake.arg1_history[1].contents.size, NULL);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of two SDUs where the end fragment of the first was not seen
 */
ZTEST(test_rx_unframed, test_rx_unframed_dbl_pdu_prem)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[23];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	pdu_timestamp += 200;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of SDUs with PDU errors
 */
ZTEST(test_rx_unframed, test_rx_unframed_single_pdu_err)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[23];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_ERRORS, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	pdu_timestamp = 9249 + CONN_INT_UNIT_US;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_LOST_DATA, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_LOST_DATA, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of SDUs where PDUs are not in sequence
 */
ZTEST(test_rx_unframed, test_rx_unframed_seq_err)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[43];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 43);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 Not transferred to ISO-AL ------------------------------------*/
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	/* PDU count will not have reached 3 as one PDU was not received, so
	 * last_pdu will not be set and the state should remain in Error
	 * Spooling.
	 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_LOST_DATA, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	pdu_timestamp = 9249 + CONN_INT_UNIT_US;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	/* Detecting the transition from an end fragment to a start fragment
	 * should have triggered the monitoring code to pull the state machine
	 * out of Eroor spooling and directly into the start of a new SDU. As
	 * this was not an end fragment, the next state should be continue.
	 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of  SDUs where PDUs are not in sequence with errors
 * Tests error prioritization
 */
ZTEST(test_rx_unframed, test_rx_unframed_seq_pdu_err)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[43];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 43);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 Not transferred to ISO-AL ------------------------------------*/
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	/* PDU status ISOAL_PDU_STATUS_ERRORS */
	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_ERRORS, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* Lost data should be higher priority */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	/* PDU count will not have reached 3 as one PDU was not received, so
	 * last_pdu will not be set and the state should remain in Error
	 * Spooling.
	 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_LOST_DATA, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	pdu_timestamp = 9249 + CONN_INT_UNIT_US;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	/* Detecting the transition from an end fragment to a start fragment
	 * should have triggered the monitoring code to pull the state machine
	 * out of Eroor spooling and directly into the start of a new SDU. As
	 * this was not an end fragment, the next state should be continue.
	 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 5 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	/* Expecting padding so state should be Error Spooling */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of  SDUs where valid PDUs are followed by padding
 */
ZTEST(test_rx_unframed, test_rx_unframed_padding)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[43];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 4;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 43);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	/* Expecting padding PDUs so should be in Error Spool state */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;

	/* PDU padding 1 */
	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;

	/* PDU padding 2 */
	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of  SDUs with padding where the end was not seen
 */
ZTEST(test_rx_unframed, test_rx_unframed_padding_no_end)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[33];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 33);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;

	/* PDU padding 1 */
	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;

	/* PDU padding 2 */
	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU emitted with errors as end fragment was not seen */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of a SDU where there is an error in the first PDU followed
 * by valid padding PDUs
 */
ZTEST(test_rx_unframed, test_rx_unframed_padding_error1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[13];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 13);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_ERRORS, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;

	/* PDU padding 1 */
	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;

	/* PDU padding 2 */
	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of a SDU where the second PDU is corrupted and appears to
 * be a padding PDU
 */
ZTEST(test_rx_unframed, test_rx_unframed_padding_error2)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[13];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 13);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;

	/* PDU with errors that appears as padding */
	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_ERRORS, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;

	/* PDU padding 1 */
	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of a SDU where only the padding PDU has errors
 */
ZTEST(test_rx_unframed, test_rx_unframed_padding_error3)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[23];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;

	/* PDU padding with errors */
	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_ERRORS, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of a zero length SDU
 */
ZTEST(test_rx_unframed, test_rx_unframed_zero_len_packet)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[13];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 13);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 0;
	sdu_size = 0;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be written to */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests reassembly of a SDU in two PDUs where the end was not seen and BN is
 * two which should return to FSM start after reassembly
 */
ZTEST(test_rx_unframed, test_rx_unframed_dbl_pdu_no_end)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[23];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests assertion on receiving a PDU with an invalid LLID without errors as
 * the first PDU of the SDU
 */
ZTEST(test_rx_unframed, test_rx_unframed_dbl_pdu_invalid_llid1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[13];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 13);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* Invalid LLID - Valid PDU*/
	create_unframed_pdu(PDU_BIS_LLID_FRAMED, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* expecting an assertion */
	ztest_set_assert_valid(true);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	ztest_set_assert_valid(false);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests assertion on receiving a PDU with an invalid LLID without errors as
 * the second PDU of the SDU
 */
ZTEST(test_rx_unframed, test_rx_unframed_dbl_pdu_invalid_llid2)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[23];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	/* Invalid LLID - Valid PDU */
	create_unframed_pdu(PDU_BIS_LLID_FRAMED, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Expecting an assertion */
	ztest_set_assert_valid(true);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	ztest_set_assert_valid(false);
}

/**
 * Test Suite	:	RX unframed PDU reassembly
 *
 * Tests receiving a PDU with an invalid LLID with errors. This should not
 * result in a assertion as it could happen if a RX reaches it's flush timeout.
 */
ZTEST(test_rx_unframed, test_rx_unframed_dbl_pdu_invalid_llid2_pdu_err)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[23];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, false, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       false,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_VALID, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	/* Invalid LLID - Valid PDU */
	create_unframed_pdu(PDU_BIS_LLID_FRAMED, &testdata[testdata_indx],
			    (testdata_size - testdata_indx), payload_number, pdu_timestamp,
			    ISOAL_PDU_STATUS_ERRORS, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU emitted with errors */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2], sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU
 */
ZTEST(test_rx_framed, test_rx_framed_single_pdu_single_sdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[23];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size = 23;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of a single SDU from three segmented PDUs
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_single_sdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[33];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 33);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] =
		add_framed_pdu_cont(&testdata[testdata_indx], (testdata_size - testdata_indx),
				    &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[1]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[2] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[2]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of two SDUs from three segmented PDUs
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[2];
	struct isoal_sdu_buffer sdu_buffer[2];
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[2];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[2];
	uint16_t pdu_data_loc[5];
	isoal_sdu_cnt_t seqn[2];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	uint8_t testdata[46];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_test_data_buffer(testdata, 46);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[0] = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf[0], sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[0] += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[1] = seqn[0] + 1;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size[1] = 13;

	pdu_data_loc[2] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* SDU 1 */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU 2 */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU 2 should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf[0], sink_sdu_write_test_fake.arg0_history[1], NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[1]],
			  sink_sdu_write_test_fake.arg1_history[1], NULL);
	zassert_equal(10, sink_sdu_write_test_fake.arg2_history[1]);
	zassert_equal_ptr(&rx_sdu_frag_buf[1], sink_sdu_write_test_fake.arg0_history[2], NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[2]],
			  sink_sdu_write_test_fake.arg1_history[2], NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_history[2],
		      NULL);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[0],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[0], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[0], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[0].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[0].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[1] += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&rx_sdu_frag_buf[1], sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[3]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[1],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[1], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[1], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[1].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[1].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of a zero length SDU
 */
ZTEST(test_rx_framed, test_rx_framed_zero_length_sdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[3];
	struct isoal_sdu_buffer sdu_buffer[3];
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[3];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[3];
	uint16_t pdu_data_loc[5];
	isoal_sdu_cnt_t seqn[3];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	uint8_t testdata[46];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[2]);
	init_test_data_buffer(testdata, 46);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[2].dbuf = &rx_sdu_frag_buf[2];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[2].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[0] = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf[0], sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[0] += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[1] = seqn[0] + 1;
	testdata_indx = testdata_size;
	sdu_size[1] = 0;

	/* Zero length SDU */
	pdu_data_loc[2] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp[2] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[2] = seqn[1] + 1;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[2] = 10;

	pdu_data_loc[3] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* SDU 1 */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU 2 */
	/* Zero length SDU */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be written to */

	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU 3 */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[2]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU 2 should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf[0], sink_sdu_write_test_fake.arg0_history[1], NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[1]],
			  sink_sdu_write_test_fake.arg1_history[1], NULL);
	zassert_equal(10, sink_sdu_write_test_fake.arg2_history[1]);
	zassert_equal_ptr(&rx_sdu_frag_buf[2], sink_sdu_write_test_fake.arg0_history[2], NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[3]],
			  sink_sdu_write_test_fake.arg1_history[2], NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_history[2],
		      NULL);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl],
			  sink_sdu_emit_test_fake.arg0_history[0], NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_history[0].sdu_production.sdu_state,
		      NULL);
	zassert_equal(sdu_size[0],
		      sink_sdu_emit_test_handler_fake.arg0_history[0].sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID,
		      sink_sdu_emit_test_handler_fake.arg1_history[0].status, NULL);
	zassert_equal(sdu_timestamp[0], sink_sdu_emit_test_handler_fake.arg1_history[0].timestamp,
		      NULL);
	zassert_equal(seqn[0], sink_sdu_emit_test_handler_fake.arg1_history[0].seqn);
	zassert_equal(sdu_buffer[0].dbuf,
		      sink_sdu_emit_test_handler_fake.arg1_history[0].contents.dbuf, NULL);
	zassert_equal(sdu_buffer[0].size,
		      sink_sdu_emit_test_handler_fake.arg1_history[0].contents.size, NULL);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl],
			  sink_sdu_emit_test_fake.arg0_history[1], NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_history[1].sdu_production.sdu_state,
		      NULL);
	zassert_equal(sdu_size[1],
		      sink_sdu_emit_test_handler_fake.arg0_history[1].sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID,
		      sink_sdu_emit_test_handler_fake.arg1_history[1].status, NULL);
	zassert_equal(sdu_timestamp[1], sink_sdu_emit_test_handler_fake.arg1_history[1].timestamp,
		      NULL);
	zassert_equal(seqn[1], sink_sdu_emit_test_handler_fake.arg1_history[1].seqn);
	zassert_equal(sdu_buffer[1].dbuf,
		      sink_sdu_emit_test_handler_fake.arg1_history[1].contents.dbuf, NULL);
	zassert_equal(sdu_buffer[1].size,
		      sink_sdu_emit_test_handler_fake.arg1_history[1].contents.size, NULL);

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[2] += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&rx_sdu_frag_buf[2], sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[3]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[2],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[2], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[2], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[2].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[2].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU followed by
 * padding
 */
ZTEST(test_rx_framed, test_rx_framed_dbl_pdu_dbl_sdu_padding)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[33];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 33);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 2 -------------------------------------------------------------*/
	/* Padding PDU */
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* SDU should not be allocated */

	/* SDU should not be written */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;
	pdu_timestamp += 200;

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[1]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU with errors,
 * followed by a valid PDU
 */
ZTEST(test_rx_framed, test_rx_framed_dbl_pdu_dbl_sdu_pdu_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[33];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 33);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = 0;
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size = 0;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_ERRORS,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be written to */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;
	pdu_timestamp += 200;

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[1]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU with errors,
 * followed by a valid PDU
 */
ZTEST(test_rx_framed, test_rx_framed_dbl_pdu_dbl_sdu_pdu_err2)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[33];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 33);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = 0;
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size = 0;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_LOST_DATA,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be written to */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_LOST_DATA, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;
	pdu_timestamp += 200;

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[1]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU with errors,
 * followed by a valid PDU
 */
ZTEST(test_rx_framed, test_rx_framed_dbl_pdu_dbl_sdu_seq_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[33];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 33);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 2 -------------------------------------------------------------*/
	/* Not transferred to the ISO-AL */
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;
	pdu_timestamp += 200;

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	/* ISO-AL has no concept of time and is unable to detect than an SDU
	 * has been lost. Sequence number does not increment.
	 * seqn = seqn;
	 */
	testdata_indx = testdata_size;
	testdata_size += 10;

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;
	pdu_timestamp += 200;

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[1]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of a single SDU from three segmented PDUs with errors
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_single_sdu_pdu_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[46];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 46);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = 0;
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 0;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_ERRORS,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be written to */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] =
		add_framed_pdu_cont(&testdata[testdata_indx], (testdata_size - testdata_indx),
				    &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[2] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;
	pdu_timestamp += 200;

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size = 13;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[3]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of a single SDU from three segmented PDUs with errors
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_single_sdu_pdu_err2)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[46];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 46);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_ERRORS,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] =
		add_framed_pdu_cont(&testdata[testdata_indx], (testdata_size - testdata_indx),
				    &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[2] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;
	pdu_timestamp += 200;

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size = 13;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[3]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of a single SDU from three segmented PDUs with errors
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_single_sdu_pdu_err3)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[46];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 46);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] =
		add_framed_pdu_cont(&testdata[testdata_indx], (testdata_size - testdata_indx),
				    &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[1]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_ERRORS,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[2] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;
	pdu_timestamp += 200;

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size = 13;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[3]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of a single SDU from three segmented PDUs with errors
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_single_sdu_seq_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[46];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 46);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	/* PDU not transferred to the ISO-AL */
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[2] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_LOST_DATA, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;
	pdu_timestamp += 200;

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size = 13;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[3]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of a single SDU from three segmented PDUs with errors
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_single_sdu_pdu_seq_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[46];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 46);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	/* PDU not transferred to the ISO-AL */
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_ERRORS,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[2] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_LOST_DATA, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;
	pdu_timestamp += 200;

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size = 13;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[3]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of two SDUs from three segmented PDUs
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu_pdu_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[2];
	struct isoal_sdu_buffer sdu_buffer[2];
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[2];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[2];
	uint16_t pdu_data_loc[5];
	isoal_sdu_cnt_t seqn[2];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	uint8_t testdata[63];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_test_data_buffer(testdata, 63);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = 0;
	seqn[0] = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 0;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_ERRORS,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be written to */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[0],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[0], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[0], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[0].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[0].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[0] += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[1] = seqn[0] + 1;
	testdata_indx = testdata_size;
	testdata_size += 17;
	sdu_size[1] = 17;

	pdu_data_loc[2] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* SDU 1 */
	/* SDU should not be written to */

	/* SDU shold not be emitted */

	/* SDU 2 */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU 2 should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf[1], sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[2]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[1] += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&rx_sdu_frag_buf[1], sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[3]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[1],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[1], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[1], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[1].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[1].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);

	payload_number++;
	pdu_timestamp = 9249 + (iso_interval_int * CONN_INT_UNIT_US);

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[0] = seqn[1] + 1;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size[0] = 13;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[4] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[4]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[0],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[0], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[0], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[0].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[0].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of two SDUs from three segmented PDUs
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu_pdu_err2)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[2];
	struct isoal_sdu_buffer sdu_buffer[2];
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[2];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[2];
	uint16_t pdu_data_loc[5];
	isoal_sdu_cnt_t seqn[2];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	uint8_t testdata[63];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_test_data_buffer(testdata, 63);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[0] = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf[0], sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;

	/* PDU with errors */
	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_ERRORS,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[1] = seqn[0];
	testdata_indx = testdata_size;
	testdata_size += 17;
	sdu_size[1] = 0;

	pdu_data_loc[2] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* SDU 1 */
	/* SDU should not be written to */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU 2 */
	/* Should not be allocated */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[0],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[0], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[0], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[0].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[0].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);

	payload_number++;
	pdu_timestamp = 9249 + (iso_interval_int * CONN_INT_UNIT_US);

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[0] = seqn[1] + 1;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size[0] = 13;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[4] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[4]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[0],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[0], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[0], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[0].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[0].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of two SDUs from three segmented PDUs
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu_pdu_err3)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[2];
	struct isoal_sdu_buffer sdu_buffer[2];
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[2];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[2];
	uint16_t pdu_data_loc[5];
	isoal_sdu_cnt_t seqn[2];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	uint8_t testdata[63];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_test_data_buffer(testdata, 63);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[0] = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf[0], sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[0] += 10;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[1] = seqn[0] + 1;
	testdata_indx = testdata_size;
	testdata_size += 17;
	sdu_size[1] = 17;

	pdu_data_loc[2] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* SDU 1 */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU 2 */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU 2 should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf[0], sink_sdu_write_test_fake.arg0_history[1], NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[1]],
			  sink_sdu_write_test_fake.arg1_history[1], NULL);
	zassert_equal(10, sink_sdu_write_test_fake.arg2_history[1]);
	zassert_equal_ptr(&rx_sdu_frag_buf[1], sink_sdu_write_test_fake.arg0_history[2], NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[2]],
			  sink_sdu_write_test_fake.arg1_history[2], NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_history[2],
		      NULL);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[0],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[0], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[0], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[0].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[0].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	/* SDU size does not change */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_ERRORS,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[1],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_ERRORS, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[1], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[1], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[1].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[1].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);

	payload_number++;
	pdu_timestamp = 9249 + (iso_interval_int * CONN_INT_UNIT_US);

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[0] = seqn[1] + 1;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size[0] = 13;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[4] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[4]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[0],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[0], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[0], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[0].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[0].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of two SDUs from three segmented PDUs with errors
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu_seq_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[2];
	struct isoal_sdu_buffer sdu_buffer[2];
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[2];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[2];
	uint16_t pdu_data_loc[5];
	isoal_sdu_cnt_t seqn[2];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	uint8_t testdata[63];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_test_data_buffer(testdata, 63);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[0] = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf[0], sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	/* No change in SDU 1 size */

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	/* ISO-AL has no concept of time and is unable to detect than an SDU
	 * has been lost. Sequence number does not increment.
	 */
	seqn[1] = seqn[0];
	testdata_indx = testdata_size;
	testdata_size += 17;
	sdu_size[1] = 0;

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	/* SDU size does not change */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU 1 emitted with errors, SDU 2 lost */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[0],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_LOST_DATA, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[0], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[0], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[0].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[0].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);

	payload_number++;
	pdu_timestamp = 9249 + (iso_interval_int * CONN_INT_UNIT_US);

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[0] = seqn[1] + 1;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size[0] = 13;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[4] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[4]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[0],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[0], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[0], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[0].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[0].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of two SDUs from three segmented PDUs with errors
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu_pdu_seq_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[2];
	struct isoal_sdu_buffer sdu_buffer[2];
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[2];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[2];
	uint16_t pdu_data_loc[5];
	isoal_sdu_cnt_t seqn[2];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	uint8_t testdata[63];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_test_data_buffer(testdata, 63);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[0] = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf[0], sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	/* PDU 2 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	/* No change in SDU 1 size */

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	/* ISO-AL has no concept of time and is unable to detect than an SDU
	 * has been lost. Sequence number does not increment.
	 */
	seqn[1] = seqn[0];
	testdata_indx = testdata_size;
	testdata_size += 17;
	sdu_size[1] = 0;

	/* PDU 3 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += 200;
	testdata_indx = testdata_size;
	testdata_size += 10;
	/* SDU size does not change */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_ERRORS,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU 1 emitted with errors, SDU 2 lost */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[0],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_LOST_DATA, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[0], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[0], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[0].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[0].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);

	/* PDU 4 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);

	payload_number++;
	pdu_timestamp = 9249 + (iso_interval_int * CONN_INT_UNIT_US);

	sdu_timeoffset =
		sdu_timeoffset - sdu_interval > 0
			? sdu_timeoffset - sdu_interval
			: sdu_timeoffset + (iso_interval_int * CONN_INT_UNIT_US) - sdu_interval;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[0] = seqn[1] + 1;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size[0] = 13;

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[4] =
		add_framed_pdu_single(&testdata[testdata_indx], (testdata_size - testdata_indx),
				      sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);
	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_val, NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[4]],
			  sink_sdu_write_test_fake.arg1_val, NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_val);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size[0],
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written, NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp[0], sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn[0], sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer[0].dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer[0].size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

/**
 * Test Suite	:	RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU which is
 * invalid as it contains multiple segments from the same SDU.
 */
ZTEST(test_rx_framed, test_rx_framed_single_invalid_pdu_single_sdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[25];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * CONN_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	init_rx_pdu_buffer(&rx_pdu_meta_buf);
	init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 25);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role, true, FT, sdu_interval, iso_interval_int,
					  stream_sync_delay, group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 1;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,		  /* Handle */
				       role,		  /* Role */
				       true,		  /* Framed */
				       BN,		  /* BN */
				       FT,		  /* FT */
				       sdu_interval,	  /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	create_framed_pdu_base(payload_number, pdu_timestamp, ISOAL_PDU_STATUS_VALID,
			       &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] =
		add_framed_pdu_start(&testdata[testdata_indx], (testdata_size - testdata_indx),
				     sdu_timeoffset, &rx_pdu_meta_buf.pdu_meta);

	testdata_indx = testdata_size;
	testdata_size += 5;
	sdu_size += 5;

	pdu_data_loc[1] =
		add_framed_pdu_cont(&testdata[testdata_indx], (testdata_size - testdata_indx),
				    &rx_pdu_meta_buf.pdu_meta);

	testdata_indx = testdata_size;
	testdata_size += 7;
	sdu_size += 7;

	pdu_data_loc[2] =
		add_framed_pdu_end(&testdata[testdata_indx], (testdata_size - testdata_indx),
				   &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;

	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm, ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_alloc_test_fake.arg0_val,
			  NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu_meta, sink_sdu_alloc_test_fake.arg1_val, NULL);

	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_history[0], NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[0]],
			  sink_sdu_write_test_fake.arg1_history[0], NULL);
	zassert_equal(13, sink_sdu_write_test_fake.arg2_history[0]);

	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_history[1], NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[1]],
			  sink_sdu_write_test_fake.arg1_history[1], NULL);
	zassert_equal(5, sink_sdu_write_test_fake.arg2_history[1]);

	zassert_equal_ptr(&rx_sdu_frag_buf, sink_sdu_write_test_fake.arg0_history[2], NULL);
	zassert_equal_ptr(&rx_pdu_meta_buf.pdu[2 + pdu_data_loc[2]],
			  sink_sdu_write_test_fake.arg1_history[2], NULL);
	zassert_equal((testdata_size - testdata_indx), sink_sdu_write_test_fake.arg2_history[2],
		      NULL);

	zassert_equal_ptr(&isoal_global.sink_state[sink_hdl], sink_sdu_emit_test_fake.arg0_val,
			  NULL);
	zassert_equal(BT_ISO_SINGLE,
		      sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_state, NULL);
	zassert_equal(sdu_size, sink_sdu_emit_test_handler_fake.arg0_val.sdu_production.sdu_written,
		      NULL);
	zassert_equal(ISOAL_SDU_STATUS_VALID, sink_sdu_emit_test_handler_fake.arg1_val.status,
		      NULL);
	zassert_equal(sdu_timestamp, sink_sdu_emit_test_handler_fake.arg1_val.timestamp);
	zassert_equal(seqn, sink_sdu_emit_test_handler_fake.arg1_val.seqn);
	zassert_equal(sdu_buffer.dbuf, sink_sdu_emit_test_handler_fake.arg1_val.contents.dbuf,
		      NULL);
	zassert_equal(sdu_buffer.size, sink_sdu_emit_test_handler_fake.arg1_val.contents.size,
		      NULL);
}

static void common_before(void *f)
{
	ARG_UNUSED(f);

	custom_sink_sdu_alloc_test_output_buffer.buffer_size = 0;
	custom_sink_sdu_alloc_test_output_buffer.pos = 0;
	RESET_FAKE(sink_sdu_alloc_test);
	RESET_FAKE(sink_sdu_write_test);
	RESET_FAKE(sink_sdu_emit_test);
	RESET_FAKE(sink_sdu_emit_test_handler);

	FFF_RESET_HISTORY();

	sink_sdu_alloc_test_fake.custom_fake = custom_sink_sdu_alloc_test;
	sink_sdu_write_test_fake.custom_fake = custom_sink_sdu_write_test;
	sink_sdu_emit_test_fake.custom_fake = custom_sink_sdu_emit_test;
}

ZTEST_SUITE(test_rx_basics, NULL, NULL, common_before, NULL, NULL);
ZTEST_SUITE(test_rx_unframed, NULL, NULL, common_before, NULL, NULL);
ZTEST_SUITE(test_rx_framed, NULL, NULL, common_before, NULL, NULL);
