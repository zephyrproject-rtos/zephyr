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

#include <string.h>
#include <zephyr/types.h>
#include <ztest.h>
#include <ztest_error_hook.h>

#include <stdio.h>
#include <stdlib.h>

/* Include the DUT */
#include "ll_sw/isoal.c"

/*
 * Note: sdu_interval unit is uS, iso_interval is a multiple of 1.25mS
 *
 * pdus_per_sdu = burst_number * (sdu_interval / (iso_interval * 1250));
 *
 */

/* Global vars to be used at setup */
isoal_sink_handle_t sink_hdl;
isoal_status_t err;
uint16_t handle;
uint32_t stream_sync_delay = 1;
uint32_t group_sync_delay = 1;
uint16_t iso_interval = 1;
uint32_t sdu_interval = (3 * 1250);
uint8_t  burst_number = 1;
uint8_t  flush_timeout = 1;
uint8_t  role = BT_CONN_ROLE_PERIPHERAL;
uint8_t  framed;

uint64_t payload_number = 2000;

/* This should point at start of reference PDU content when emit is called */
uint8_t *pdu_ref;

/* Sink as referenced by sink_hdl */
struct isoal_sink *sink;

/*
 * PDU defines and helper functions
 */
#define TESTDATA_MAX_LEN 60
uint8_t testdata[TESTDATA_MAX_LEN] = {
	0xBF, 0x9B, 0xD1, 0x7D, 0x9E, 0xE0, 0xB9, 0xA4, 0x71, 0xE6,
	0x80, 0xA7, 0x59, 0xAD, 0xB0, 0x98, 0xB3, 0x95, 0x83, 0x6B,
	0xF8, 0xFE, 0xCB, 0xA1, 0xE9, 0x7A, 0xDD, 0x86, 0x68, 0x50,
	0x77, 0x6E, 0xF2, 0x5C, 0xD7, 0x53, 0x62, 0x56, 0x74, 0x89,
	0xCE, 0xC5, 0xAA, 0x65, 0x8C, 0xFF, 0xF5, 0x8F, 0xDA, 0xBC,
	0xB6, 0xEC, 0x5F, 0xE3, 0xD4, 0xEF, 0xC8, 0x92, 0xC2, 0xFB
};

uint8_t pdu_data[(TESTDATA_MAX_LEN+2)];
struct node_rx_iso_meta meta;
struct isoal_pdu_rx pdu_meta = {&meta, (struct pdu_iso *) &pdu_data};

#define LLID_TO_STR(llid) (llid == PDU_BIS_LLID_COMPLETE_END ? "COMPLETE_END" : \
	(llid == PDU_BIS_LLID_START_CONTINUE ? "START_CONT" : \
		(llid == PDU_BIS_LLID_FRAMED ? "FRAMED" : \
			(llid == PDU_BIS_LLID_CTRL ? "CTRL" : "?????"))))

#define STATE_TO_STR(s) (s == BT_ISO_SINGLE ? "SINGLE" : \
	(s == BT_ISO_START ? "START" : \
		(s == BT_ISO_CONT ? "CONT" : \
			(s == BT_ISO_END ? "END" : "???"))))

#define DEBUG_TEST 1

#if defined(DEBUG_TEST)
void debug_print_pdu(void)
{
	uint8_t len = pdu_data[1];

	PRINT("PDU %04d : %10s %02d: ", (uint32_t) meta.payload_number,
		LLID_TO_STR(pdu_data[0]), len);

	for (int i = 0; i < len; i++) {
		PRINT("0x%0x ", pdu_data[2+i]);
	}
	PRINT("\n");
}

void debug_print_sdu(const struct isoal_sink *sink_ctx, uint8_t *buf)
{
	uint16_t len = sink_ctx->sdu_production.sdu_written;

	PRINT("\n");
	PRINT("SDU %04d : %10s %02d: ", (uint32_t) sink->session.seqn,
		STATE_TO_STR(sink_ctx->sdu_production.sdu_state), len);
	for (int i = 0; i < len; i++) {
		PRINT("0x%0x ", buf[i]);
	}
	PRINT("\n");
	PRINT("\n");
}
#else
void debug_print_pdu(void) {}
void debug_print_sdu(const struct isoal_sink *sink_ctx, uint8_t *buf) {}
#endif

/**
 * @brief   Construct Test meta pdu - unframed
 * @details Based on parameters construct a meta pdu datastructure by
 *          by copying supplied data pointed to by dataptr
 *
 * @param llid    Packet type for header
 * @param dataptr Pointer to source test data
 * @param length  Length of PDU raw data to copy
 * @param payload_number Payload number
 * @param timestamp      Time of reception
 * @param status         (OK/not OK)
 *
 */
void construct_pdu_unframed(uint8_t llid, uint8_t *dataptr, uint8_t length,
			    uint64_t payload_number, uint32_t timestamp, uint8_t  status)
{
	/* Clear PDU raw data */
	memset(pdu_data, 0, TESTDATA_MAX_LEN);

	meta.payload_number = payload_number;
	meta.timestamp =      timestamp;
	meta.status =         status;

	pdu_data[0] = llid;
	pdu_data[1] = length;

	memcpy(&pdu_data[2], dataptr, length);

	debug_print_pdu();
}


struct pdu_iso_sdu_sh *seg_hdr;

/**
 * @brief   Construct Test meta pdu - start of framed pdu
 * @details Based on parameters construct a meta pdu datastructure by
 *          by copying supplied data pointed to by dataptr
 *
 * @param cmplt   Set to 1 if this PDU is last PDU of SDU
 * @param dataptr Pointer to source test data
 * @param length  Length of PDU raw data to copy, i.e. payload
 * @param payload_number Payload number
 * @param timestamp      Time of reception
 * @param status         (OK/not OK)
 */
void construct_pdu_framed_start(uint8_t sc, uint8_t cmplt, uint8_t *dataptr, uint8_t length,
				uint64_t payload_number, uint32_t timestamp, uint8_t  status)
{
	/* Clear PDU raw data */
	memset(pdu_data, 0, TESTDATA_MAX_LEN);

	meta.payload_number = payload_number;
	meta.timestamp =      timestamp;
	meta.status =         status;

	pdu_data[0] = PDU_BIS_LLID_FRAMED;
	if (length > 0) {
		pdu_data[1] = length  + PDU_ISO_SEG_HDR_SIZE +
				(sc ? 0 : PDU_ISO_SEG_TIMEOFFSET_SIZE);
	} else {
		/* padding packet */
		pdu_data[1] = 0;
	}

	seg_hdr = (struct pdu_iso_sdu_sh *) pdu_meta.pdu->payload;
	seg_hdr->sc = sc & 0x1;
	seg_hdr->cmplt = cmplt & 0x1;
	/* Note, timeoffset only available in first segment of sdu */
	seg_hdr->length = length + (sc ? 0 : PDU_ISO_SEG_TIMEOFFSET_SIZE);
	seg_hdr->timeoffset = 0x123456;
	uint8_t *plp = ((uint8_t *) seg_hdr) + PDU_ISO_SEG_HDR_SIZE +
			(sc ? 0 : PDU_ISO_SEG_TIMEOFFSET_SIZE);
	memcpy(plp, dataptr, length);
	debug_print_pdu();

	/* advance pointer to point at next segment header */
	seg_hdr = (struct pdu_iso_sdu_sh *) (((uint8_t *) seg_hdr) +
		seg_hdr->length + PDU_ISO_SEG_HDR_SIZE);
}

/**
 * @brief   Construct Test meta pdu - framed (add segment)
 * @details Based on parameters construct a meta pdu datastructure by
 *          by copying supplied data pointed to by dataptr
 *          Note: construct_pdu_framed_start must be called
 *                on first segment, i.e. before calling this
 *                function.
 *
 * @param cmplt   Set to 1 if this PDU is last PDU of SDU
 * @param dataptr Pointer to source test data
 * @param length  Length of segment raw data to copy, i.e. payload
 *
 */
void construct_pdu_framed_cont(uint8_t cmplt, uint8_t *dataptr, uint8_t length)
{
	pdu_data[1] += length  + PDU_ISO_SEG_HDR_SIZE; /* Increase total length of PDU */
	seg_hdr->sc = 1;
	seg_hdr->cmplt =  cmplt & 0x1;
	seg_hdr->length = length;

	uint8_t *plp = ((uint8_t *) seg_hdr) + PDU_ISO_SEG_HDR_SIZE;

	memcpy(plp, dataptr, length);
	debug_print_pdu();

	seg_hdr = (struct pdu_iso_sdu_sh *) (((uint8_t *) seg_hdr) +
		seg_hdr->length + PDU_ISO_SEG_HDR_SIZE);
}



/*
 * SDU defines and helper functions
 */
#define SDU_BUF_MAX_LEN 256

uint8_t sdu_buf[SDU_BUF_MAX_LEN];
isoal_sdu_len_t sdu_buf_len = SDU_BUF_MAX_LEN;
uint16_t sdu_buf_idx;
bool sdu_emit_expected = true;

/**
 * @brief   Reset SDU buffer
 *
 */
void clear_sdu_buf(void)
{
	memset(sdu_buf, 0, SDU_BUF_MAX_LEN);
	sdu_buf_idx = 0;
	sdu_emit_expected = true;
}

/*
 * Callbacks stubs for ISO AL, self checking SDU against ref data
 */
isoal_status_t sink_sdu_alloc_test(const struct isoal_sink    *sink_ctx,
				   const struct isoal_pdu_rx  *valid_pdu,
				   struct isoal_sdu_buffer    *sdu_buffer)
{
	ARG_UNUSED(sink_ctx);
	ARG_UNUSED(valid_pdu);

	sdu_buffer->dbuf = sdu_buf;
	sdu_buffer->size = sdu_buf_len;

	return ISOAL_STATUS_OK;
}

isoal_status_t sink_sdu_emit_test(const struct isoal_sink         *sink_ctx,
				  const struct isoal_sdu_produced *valid_sdu)
{
	uint8_t *buf;

	buf = (uint8_t *) valid_sdu->contents.dbuf;
	int res = memcmp(buf, pdu_ref, sink_ctx->sdu_production.sdu_written);
	debug_print_sdu(sink_ctx, buf);
	zassert_equal(res, 0, "len=%u buf[0]=0x%x ref[0]=0x%0x",
		      sink_ctx->sdu_production.sdu_written, buf[0], pdu_ref[0]);

	/* Advance reference pointer, this will be needed when a PDU is split over multiple SDUs */
	pdu_ref += sink_ctx->sdu_production.sdu_written;
	zassert_true(sdu_emit_expected, "");
	clear_sdu_buf();
	return ISOAL_STATUS_OK;
}

isoal_status_t sink_sdu_write_test(void *dbuf,
				   const uint8_t *pdu_payload,
				   const size_t consume_len)
{
	uint8_t *buf = (uint8_t *) dbuf;

	memcpy(&buf[sdu_buf_idx], pdu_payload, consume_len);
	sdu_buf_idx += consume_len;

	return ISOAL_STATUS_OK;
}

/**
 * @brief   Test setup, can be called before running other test functions
 */
void test_setup(void)
{
	err = isoal_init();
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	err = isoal_reset();
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	/* Create a sink based on global parameters */
	err = isoal_sink_create(handle, role, framed,
				burst_number, flush_timeout,
				sdu_interval, iso_interval,
				stream_sync_delay, group_sync_delay,
				sink_sdu_alloc_test, sink_sdu_emit_test, sink_sdu_write_test,
				&sink_hdl);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	sink = &isoal_global.sink_state[sink_hdl];

	/* Enable newly created sink */
	isoal_sink_enable(sink_hdl);

	payload_number = 2000;
}

/**
 * @brief   Test unframed single PDU in a single SDU
 */
void test_unframed_single_pdu(void)
{
	clear_sdu_buf();

	pdu_ref = &testdata[0];
	construct_pdu_unframed(PDU_BIS_LLID_COMPLETE_END, pdu_ref, 23, payload_number++, 9249, 0);

	/* Test recombine */
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 23, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Test unframed double PDU in a single SDU
 */
void test_unframed_dbl_pdu(void)
{
	clear_sdu_buf();

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[2], 5,
			       payload_number++, 10000, 0);
	/* Test recombine */
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 5, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	construct_pdu_unframed(PDU_BIS_LLID_COMPLETE_END, &testdata[2+5], 7,
			       payload_number++, 20000, 0);
	/* Test recombine, should now trigger emit since this is last PDU in SDU */
	pdu_ref = &testdata[2];
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 5+7, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Test unframed PDUs, multiple SDU
 */
void test_unframed_dbl_split(void)
{
	clear_sdu_buf();

	/* Assume SDU buffer len 10 (sdu_buf_len=10) */

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[0], 4,
			       payload_number++, 10000, 0);
	/* Test recombine */
	pdu_ref = &testdata[0];
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 4, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[4], 6,
			       payload_number++, 20000, 0);
	/* Test recombine, should now trigger emit since this is last PDU in SDU */
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);


	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[10], 7,
			       payload_number++, 20000, 0);
	pdu_ref = &testdata[10];
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 7, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[10+7], 1,
			       payload_number++, 20000, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 8, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	construct_pdu_unframed(PDU_BIS_LLID_COMPLETE_END, &testdata[10+7+1], 2,
			       payload_number++, 20000, 0);
	/* Test recombine, should now trigger emit since this is last PDU in SDU */
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Test unframed 5 PDUs of a single SDU
 */
void test_unframed_multi_split(void)
{
	clear_sdu_buf();

	/*
	 * Assumes SDU buffer len 10 (sdu_buf_len=10)
	 * PDUs per SDU interval should be 5
	 *
	 */

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[0], 10,
			       payload_number++, 10000, 0);
	pdu_ref = &testdata[0];
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[10], 10,
			       payload_number++, 20000, 0);
	pdu_ref = &testdata[10];
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[20], 10,
			       payload_number++, 30000, 0);
	pdu_ref = &testdata[20];
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[30], 10,
			       payload_number++, 40000, 0);
	pdu_ref = &testdata[30];
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	construct_pdu_unframed(PDU_BIS_LLID_COMPLETE_END, &testdata[40], 10,
			       payload_number++, 50000, 0);
	pdu_ref = &testdata[40];
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Single PDU split over two SDU fragments
 */
void test_unframed_long_pdu_short_sdu(void)
{
	/* Assume SDU buffer len 5 (sdu_buf_len=5) */

	clear_sdu_buf();
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[0], 10,
			       payload_number++, 10000, 0);
	/* Test recombine */
	pdu_ref = &testdata[0];
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* should only see 5 written as two SDUs 5 each has been generated */
	zassert_equal(sink->sdu_production.sdu_written, 5, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Test create and destroy sink
 */
void test_sink_create_destroy(void)
{
	isoal_sink_handle_t hdl[CONFIG_BT_CTLR_ISOAL_SINKS];
	struct isoal_sink_config *config_ptr;

	err = isoal_init();
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	err = isoal_reset();
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	uint8_t dummy_role = BT_CONN_ROLE_CENTRAL;

	for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SINKS; i++) {
		/* Create a sink based on global parameters */
		err = isoal_sink_create(handle, dummy_role, framed,
					burst_number, flush_timeout,
					sdu_interval, iso_interval,
					stream_sync_delay, group_sync_delay,
					sink_sdu_alloc_test, sink_sdu_emit_test,
					sink_sdu_write_test, &hdl[i]);
		zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

		isoal_sink_enable(hdl[i]);

		config_ptr = isoal_get_sink_param_ref(hdl[i]);
		zassert_not_null(config_ptr, "");


		dummy_role = (dummy_role + 1) % (BT_ROLE_BROADCAST + 1);
	}

	for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SINKS; i++) {
		/* Destroy sink */
		isoal_sink_destroy(hdl[i]);
	}
}

/**
 * @brief   Test over allocation of sinks
 */
void test_sink_create_err(void)
{
	err = isoal_init();
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	err = isoal_reset();
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	isoal_sink_handle_t hdl;

	for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SINKS; i++) {
		/* Create a sink based on global parameters */
		err = isoal_sink_create(handle, role, framed,
					burst_number, flush_timeout,
					sdu_interval, iso_interval,
					stream_sync_delay, group_sync_delay,
					sink_sdu_alloc_test, sink_sdu_emit_test,
					sink_sdu_write_test, &hdl);
		zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

		isoal_sink_enable(hdl);
	}

	/* Should be out of sinks, allocation should generate an error */
	err = isoal_sink_create(handle, role, framed,
				burst_number, flush_timeout,
				sdu_interval, iso_interval,
				stream_sync_delay, group_sync_delay,
				sink_sdu_alloc_test, sink_sdu_emit_test, sink_sdu_write_test,
				&hdl);
	zassert_equal(err, ISOAL_STATUS_ERR_SINK_ALLOC, "");
}

/**
 * @brief  Two PDUs per SDU sent should result in an error when params are set as One PDU per SDU
 */
void test_unframed_dbl_pdu_prem(void)
{
	clear_sdu_buf();

	pdu_ref = &testdata[2];
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[2], 5,
			       payload_number++, 10000, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 5, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_ERRORS, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* TBD: It is debatable if first PDU should result in error but second should not */
	construct_pdu_unframed(PDU_BIS_LLID_COMPLETE_END, &testdata[2+5], 7,
			       payload_number++, 20000, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 7, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}


/**
 * @brief   Test unframed single PDU in a single SDU, PDU with error
 */
void test_unframed_single_pdu_err(void)
{
	clear_sdu_buf();

	pdu_ref = &testdata[0];
	construct_pdu_unframed(PDU_BIS_LLID_COMPLETE_END, pdu_ref, 23, payload_number++, 9249, 1);

	/* Test recombine */
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting 0 data to be written but with error status */
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_ERRORS, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Trigger payload number sequence error path
 */
void test_unframed_seq_err(void)
{
	clear_sdu_buf();

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[0], 3,
			       payload_number++, 10000, 0);
	/* Test recombine */
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 3, "written=%u",
		      sink->sdu_production.sdu_written);

	construct_pdu_unframed(PDU_BIS_LLID_COMPLETE_END, &testdata[3], 7, 123123, 20000, 0);
	/* Test recombine, should now trigger emit since this is last PDU in SDU */
	pdu_ref = &testdata[0];
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	/* Expecting data to be written but with error status */
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_LOST_DATA,
		      "sdu_status=0x%x", sink->sdu_production.sdu_status);
}

/**
 * @brief   Trigger payload number sequence error path, with pdu erros
 */
void test_unframed_seq_pdu_err(void)
{
	clear_sdu_buf();

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[0], 3,
			       payload_number++, 10000, 0);
	/* Test recombine */
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 3, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	construct_pdu_unframed(PDU_BIS_LLID_COMPLETE_END, &testdata[3], 7, 123123, 20000, 1);
	/* Test recombine, should now trigger emit since this is last PDU in SDU */
	pdu_ref = &testdata[0];
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	/* Expecting no new data to be written but with error status */
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 3, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_ERRORS, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}


/**
 * @brief   Exercise padding pdu path
 */
void test_unframed_padding(void)
{
	clear_sdu_buf();

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[2], 5,
			       payload_number++, 10000, 0);
	/* Test recombine */
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 5, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	construct_pdu_unframed(PDU_BIS_LLID_COMPLETE_END, &testdata[2+5], 7,
			       payload_number++, 20000, 0);
	/* Test recombine, should now trigger emit since this is last PDU in SDU */
	pdu_ref = &testdata[2];
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 5+7, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* first padding in SDU interval */
	pdu_ref = &testdata[0];
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, pdu_ref, 0 /* len = 0 => padding */,
			       payload_number++, 923749, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting 0 data to be written, with no error status */
	zassert_equal(sink->sdu_production.sdu_written, 5+7+0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Second and last padding in SDU interval, end not seen should result in error */
	pdu_ref = &testdata[0];
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, pdu_ref, 0 /* len = 0 => padding */,
			       payload_number++, 923750, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting 0 data to be written but with error status */
	zassert_equal(sink->sdu_production.sdu_written, 5+7+0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Padding packets but no end packet received at end of SDU interval
 */
void test_unframed_padding_no_end(void)
{
	clear_sdu_buf();

	/* Assumes 3 PDUs per SDU interval */

	/* first padding in SDU interval */
	pdu_ref = &testdata[0];
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, pdu_ref, 0 /* len = 0 => padding pdu */,
			       payload_number++, 923749, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting 0 data to be written, with no error status */
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Second padding in SDU interval */
	pdu_ref = &testdata[0];
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, pdu_ref, 0 /* len = 0 => padding pdu */,
			       payload_number++, 923750, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting 0 data to be written, with no error status */
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Third and last padding in SDU interval, end not seen should result in error */
	pdu_ref = &testdata[0];
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, pdu_ref, 0 /* len = 0 => padding */,
			       payload_number++, 923751, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting 0 data to be written but with error status */
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_ERRORS, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Padding PDU with error status received in the ISOAL_START state (beginning of SDU)
 */
void test_unframed_padding_error1(void)
{
	clear_sdu_buf();

	/* Assumes 3 PDUs per SDU interval */

	/* Padding PDU with errors seen before the end of an SDU should be considered a non-padding
	 * PDU with incorrect length or LLID on account of the errors. This should result in an SDU
	 * with errors being emitted.
	 */
	pdu_ref = &testdata[0];
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, pdu_ref, 0 /* len = 0 => padding pdu */,
			       payload_number++, 923752, 1 /* pdu error */);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting no additional data to be written, but PDU should not be considered padding */
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_ERRORS, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Second padding in SDU interval */
	sdu_emit_expected = false;
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, pdu_ref, 0 /* len = 0 => padding pdu */,
			       payload_number++, 923750, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting 0 data to be written, with no error status */
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_ERRORS, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Third and last padding in SDU interval */
	sdu_emit_expected = false;
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, pdu_ref, 0 /* len = 0 => padding */,
			       payload_number++, 923751, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting 0 data to be written but with error status */
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_ERRORS, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Padding PDU with error status received in the ISOAL_CONTINUE state (middle of SDU)
 */
void test_unframed_padding_error2(void)
{
	clear_sdu_buf();

	/* Assumes 3 PDUs per SDU interval */

	/* Send PDU with start fragment without any errors */
	pdu_ref = &testdata[0];
	sdu_emit_expected = false;
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, pdu_ref, 5,
			       payload_number++, 923753, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 5, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Second padding in SDU interval but with errors. A padding PDU with errors seen before
	 * the end of an SDU should be considered a non-padding PDU with incorrect length or LLID
	 * on account of the errors. This should result in an SDU with errors being emitted.
	 */
	sdu_emit_expected = true;
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, pdu_ref, 0 /* len = 0 => padding pdu */,
			       payload_number++, 923754, 1 /* pdu error */);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting no additional data to be written, but PDU should not be considered padding */
	zassert_equal(sink->sdu_production.sdu_written, 5, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_ERRORS, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Third and last padding in SDU interval */
	sdu_emit_expected = false;
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, pdu_ref, 0 /* len = 0 => padding */,
			       payload_number++, 923751, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting 0 data to be written but with error status */
	zassert_equal(sink->sdu_production.sdu_written, 5, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_ERRORS, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

void test_unframed_padding_error3(void)
{
	clear_sdu_buf();

	/* Assumes 3 PDUs per SDU interval */

	/* Send PDU with start fragment without any errors */
	pdu_ref = &testdata[0];
	sdu_emit_expected = false;
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, pdu_ref, 5,
			       payload_number++, 923755, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 5, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Send PDU with end fragment without any errors */
	sdu_emit_expected = true;
	construct_pdu_unframed(PDU_BIS_LLID_COMPLETE_END, &testdata[5], 5,
			       payload_number++, 923756, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Send padding PDU with error. Even though there are errors, as it is a padding PDU
	 * received after the end fragment is seen, this should be considered a padding PDU and
	 * discarded.
	 */
	sdu_emit_expected = false;
	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, pdu_ref, 0 /* len = 0 => padding pdu */,
			       payload_number++, 923757, 1 /* pdu error */);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting no additional data to be written and  PDU should be considered padding */
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Zero length packet but not padding
 */
void test_unframed_zero_len_packet(void)
{
	clear_sdu_buf();

	/* TBD: This should potentially result in an error, zero length packet but not error
	 * Might need a fix in DUT and check below should be changed from
	 * ISOAL_SDU_STATUS_VALID to ISOAL_SDU_STATUS_ERRORS
	 */
	pdu_ref = &testdata[0];
	construct_pdu_unframed(PDU_BIS_LLID_COMPLETE_END, pdu_ref,
			       0 /* len = 0, but llid does not make it padding */,
			       payload_number++, 923751, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting 0 data to be written but with error status */
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Two packets in 2 packet SDU interval, but no end packet, should result in error
 */
void test_unframed_dbl_packet_no_end(void)
{
	/* Test assumes two PDUs per SDU interval */
	clear_sdu_buf();

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[2], 5,
			       payload_number++, 10000, 0);
	/* Test recombine */
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 5, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[2+5], 7,
			       payload_number++, 20000, 0);
	/* Test recombine, should now trigger emit since this is last PDU in SDU */
	pdu_ref = &testdata[2];
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 5+7, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_ERRORS, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}


/**
 * @brief   Trigger assert in isoal_sink_create
 */
void test_trig_assert_isoal_sink_create(void)
{
	err = isoal_init();
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	err = isoal_reset();
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);

	ztest_set_assert_valid(true);
	/* Create a sink based on global parameters */
	err = isoal_sink_create(handle, 99 /* Faulty role param to trigger assert */, framed,
				burst_number, flush_timeout,
				sdu_interval, iso_interval,
				stream_sync_delay, group_sync_delay,
				sink_sdu_alloc_test, sink_sdu_emit_test, sink_sdu_write_test,
				&sink_hdl);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	sink = &isoal_global.sink_state[sink_hdl];

	/* Enable newly created sink */
	isoal_sink_enable(sink_hdl);

	ztest_set_assert_valid(false);
}

/**
 * @brief   Trigger assert in isoal_rx_pdu_recombine, pdu is first pdu (state=ISOAL_START)
 */
void test_trig_assert_isoal_rx_pdu_recombine_first(void)
{
	clear_sdu_buf();

	pdu_ref = &testdata[0];
	construct_pdu_unframed(99 /*Faulty llid to trigger assert */, pdu_ref, 23, 1234, 92749, 0);

	/* Test recombine, should trigger Assert */
	ztest_set_assert_valid(true);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	ztest_set_assert_valid(false);
}

/**
 * @brief   Trigger assert in isoal_rx_pdu_recombine, faulty pdu is second PDU
 *          (state=ISOAL_CONTINUE)
 */
void test_trig_assert_isoal_rx_pdu_recombine_second(void)
{
	clear_sdu_buf();

	construct_pdu_unframed(PDU_BIS_LLID_START_CONTINUE, &testdata[2], 5,
			       payload_number++, 10000, 0);
	/* Test recombine */
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 5, "written=%u",
		      sink->sdu_production.sdu_written);

	pdu_ref = &testdata[0];
	construct_pdu_unframed(99 /*Faulty llid to trigger assert */, &testdata[2+5], 7,
			       payload_number++, 20000, 0);
	/* Test recombine, should trigger Assert */
	ztest_set_assert_valid(true);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	ztest_set_assert_valid(false);
}


/**
 * @brief   Trigger assert in isoal_get_sink_param_ref, faulty pdu is second PDU
 *          (state=ISOAL_CONTINUE)
 */
void test_trig_assert_isoal_get_sink_param_ref(void)
{
	/* should trigger Assert */
	ztest_set_assert_valid(true);
	volatile struct isoal_sink_config *config_ptr = isoal_get_sink_param_ref(99);

	zassert_not_null(config_ptr, "");
	ztest_set_assert_valid(false);
}

/**
 * @brief   Test unframed single PDU in a single SDU
 */
void test_unframed_disabled_sink(void)
{
	clear_sdu_buf();

	pdu_ref = &testdata[0];
	construct_pdu_unframed(PDU_BIS_LLID_COMPLETE_END, pdu_ref, 23, payload_number++, 92349, 0);

	/* disable sink */
	isoal_sink_disable(sink_hdl);

	/* Test recombine */
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "");
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		      sink->sdu_production.sdu_written);
}


/**
 * @brief   Test framed single PDU in a single SDU
 */
void test_framed_single_pdu(void)
{

	/* Single PDU, 1 segment */
	clear_sdu_buf();
	pdu_ref = &testdata[0];
	construct_pdu_framed_start(0, 1, pdu_ref, 10, payload_number++, 1000, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Single PDU, 2 segments */
	clear_sdu_buf();
	pdu_ref = &testdata[0];
	construct_pdu_framed_start(0, 0, pdu_ref, 7, payload_number++, 2000, 0);
	construct_pdu_framed_cont(1, &testdata[0+7], 3);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Single PDU, 3 segments */
	clear_sdu_buf();
	pdu_ref = &testdata[0];
	construct_pdu_framed_start(0, 0, pdu_ref, 4, payload_number++, 3000, 0);
	construct_pdu_framed_cont(0, &testdata[0+4], 6);
	construct_pdu_framed_cont(1, &testdata[0+4+6], 5);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 15, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Test framed double PDU in a single SDU
 */
void test_framed_double_pdu(void)
{
	/* Single PDU, 2 segments */
	clear_sdu_buf();
	pdu_ref = &testdata[0];
	construct_pdu_framed_start(0, 0, pdu_ref, 7, payload_number++, 2000, 0);
	construct_pdu_framed_cont(0, &testdata[0+7], 3);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Single PDU, 3 segments */
	construct_pdu_framed_start(1, 0, &testdata[10], 4, payload_number++, 3000, 0);
	construct_pdu_framed_cont(0, &testdata[10+4], 6);
	construct_pdu_framed_cont(1, &testdata[10+4+6], 5);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 25, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Test unframed single PDU in a single SDU, PDU with error
 */
void test_framed_single_pdu_err(void)
{
	/* Single PDU, 1 segment */
	clear_sdu_buf();
	pdu_ref = &testdata[0];
	construct_pdu_framed_start(0, 1, pdu_ref, 10, payload_number++, 1000, 1 /* error */);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* Expecting 0 data to be written but with error status */
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		     sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_ERRORS, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Test unframed single PDU in a single SDU, PDU with error
 */
void test_framed_dbl_pdu_err(void)
{
	/* Single PDU, 2 segments */
	clear_sdu_buf();
	pdu_ref = &testdata[0];
	construct_pdu_framed_start(0, 0, pdu_ref, 7, payload_number++, 2000, 1 /* error */);
	construct_pdu_framed_cont(0, &testdata[0+7], 3);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* SDU should be flushed on error, thus expect 0 written bytes */
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_ERRORS, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Single PDU, 3 segments */
	construct_pdu_framed_start(1, 0, &testdata[10], 4, payload_number++, 3000, 0);
	construct_pdu_framed_cont(0, &testdata[10+4], 6);
	construct_pdu_framed_cont(1, &testdata[10+4+6], 5);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	/* SDU should be flushed on error, thus expect 0 written bytes */
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_ERRORS, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Test framed double PDU in a single SDU
 */
void test_framed_seq_err(void)
{
	/* Single PDU, 2 segments */
	clear_sdu_buf();
	pdu_ref = &testdata[0];
	construct_pdu_framed_start(0, 0, pdu_ref, 7, payload_number++, 2000, 0);
	construct_pdu_framed_cont(0, &testdata[0+7], 3);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Single PDU, 3 segments */
	construct_pdu_framed_start(1, 0, &testdata[10], 4,
				   288456 /* not subsequent packet */, 3000, 0);
	construct_pdu_framed_cont(0, &testdata[10+4], 6);
	construct_pdu_framed_cont(1, &testdata[10+4+6], 5);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_LOST_DATA,
		      "sdu_status=0x%x", sink->sdu_production.sdu_status);
}

/**
 * @brief   PDU padding framed
 */
void test_framed_padding(void)
{
	/* Single PDU, 1 segment */
	clear_sdu_buf();
	pdu_ref = &testdata[0];
	construct_pdu_framed_start(0, 1, pdu_ref, 0, payload_number++, 1000, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	/* expect PDU to be dropped thus no SDU write */
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 0, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Test framed PDU, seq error and PDU error
 */
void test_framed_pdu_seq_err1(void)
{
	/* Single PDU, 2 segments */
	clear_sdu_buf();
	pdu_ref = &testdata[0];
	construct_pdu_framed_start(0, 0, pdu_ref, 7, payload_number++, 2000, 0);
	construct_pdu_framed_cont(0, &testdata[0+7], 3);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Single PDU, 3 segments */
	construct_pdu_framed_start(1, 0, &testdata[10], 4,
				   288456 /* not subsequent packet */, 3000, 1 /* PDU error */);
	construct_pdu_framed_cont(0, &testdata[10+4], 6);
	construct_pdu_framed_cont(1, &testdata[10+4+6], 5);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	/* PDU error should have priority in the SDU status */
	zassert_equal(sink->sdu_production.sdu_status,
		      ISOAL_SDU_STATUS_ERRORS,
		      "sdu_status=0x%x", sink->sdu_production.sdu_status);
}

/**
 * @brief   Test framed PDU, seq error
 */
void test_framed_pdu_seq_err2(void)
{
	/* Single PDU, 2 segments */
	clear_sdu_buf();
	pdu_ref = &testdata[0];
	construct_pdu_framed_start(0, 0, pdu_ref, 7, payload_number++, 2000, 0);
	construct_pdu_framed_cont(0, &testdata[0+7], 3);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Single PDU, 3 segments */
	construct_pdu_framed_start(1, 0, &testdata[10], 4,
				   288456 /* not subsequent packet */, 3000, 0 /* No PDU error */);
	construct_pdu_framed_cont(0, &testdata[10+4], 6);
	construct_pdu_framed_cont(1, &testdata[10+4+6], 5);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status,
		      ISOAL_SDU_STATUS_LOST_DATA,
		      "sdu_status=0x%x", sink->sdu_production.sdu_status);
}

/**
 * @brief   Test error in ISOAL_START state
 */
void test_framed_error1(void)
{

	/* Single PDU, 1 segment */
	clear_sdu_buf();
	pdu_ref = &testdata[0];
	construct_pdu_framed_start(1, 0, pdu_ref, 10, payload_number++, 1000, 0);
	ztest_set_assert_valid(true);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	ztest_set_assert_valid(false);
	zassert_equal(err, ISOAL_STATUS_ERR_UNSPECIFIED, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}

/**
 * @brief   Test error in ISOAL_CONTINUE state
 */
void test_framed_error2(void)
{

	/* Single PDU, 2 segments */
	clear_sdu_buf();
	pdu_ref = &testdata[0];
	construct_pdu_framed_start(0, 0, pdu_ref, 10, payload_number++, 2000, 0);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	zassert_equal(err, ISOAL_STATUS_OK, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 10, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);

	/* Single PDU, 3 segments */
	construct_pdu_framed_start(0, 1, &testdata[10], 15, payload_number++, 3000, 0);
	ztest_set_assert_valid(true);
	err = isoal_rx_pdu_recombine(sink_hdl, &pdu_meta);
	ztest_set_assert_valid(false);
	zassert_equal(err, ISOAL_STATUS_ERR_UNSPECIFIED, "err=0x%02x", err);
	zassert_equal(sink->sdu_production.sdu_written, 25, "written=%u",
		      sink->sdu_production.sdu_written);
	zassert_equal(sink->sdu_production.sdu_status, ISOAL_SDU_STATUS_VALID, "sdu_status=0x%x",
		      sink->sdu_production.sdu_status);
}


void test_main(void)
{

	/* UNFRAMED TEST CASES */

	sdu_buf_len = SDU_BUF_MAX_LEN;
	ztest_test_suite(test_basic,
		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_single_pdu),
		ztest_unit_test(test_unframed_dbl_pdu)
	);
	ztest_run_test_suite(test_basic);


	sdu_buf_len = 5;
	ztest_test_suite(test_basic2,
		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_long_pdu_short_sdu)
	);
	ztest_run_test_suite(test_basic2);


	sdu_buf_len = SDU_BUF_MAX_LEN;
	ztest_test_suite(test_sink,
		ztest_unit_test(test_sink_create_destroy),
		ztest_unit_test(test_sink_create_err)
	);
	ztest_run_test_suite(test_sink);


	ztest_test_suite(test_pck_err,
		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_single_pdu_err)
	);
	ztest_run_test_suite(test_pck_err);


	ztest_test_suite(test_seq_err,
		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_seq_err),

		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_seq_pdu_err)
	);
	ztest_run_test_suite(test_seq_err);


	sdu_interval = (4 * 1250); /* 4 PDUs per SDU interval */
	ztest_test_suite(test_padding,
		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_padding)
	);
	ztest_run_test_suite(test_padding);


	sdu_interval = (3 * 1250); /* Three PDUs per SDU interval */
	ztest_test_suite(test_padding_err,
		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_padding_no_end),
		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_padding_error1),
		ztest_unit_test(test_unframed_padding_error2),
		ztest_unit_test(test_unframed_padding_error3),
		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_zero_len_packet)
	);
	ztest_run_test_suite(test_padding_err);


	sdu_interval = (2 * 1250); /* Two PDUs per SDU interval */
	ztest_test_suite(test_no_end,
		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_dbl_packet_no_end)
	);
	ztest_run_test_suite(test_no_end);


	sdu_buf_len = 10;
	sdu_interval = (5 * 1250); /* 5 PDUs per SDU interval */
	ztest_test_suite(test_split_sdu,
		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_dbl_split),
		ztest_unit_test(test_unframed_multi_split)
	);
	ztest_run_test_suite(test_split_sdu);


	sdu_buf_len = SDU_BUF_MAX_LEN;
	sdu_interval = (1 * 1250); /* One PDU per SDU interval */
	ztest_test_suite(test2,
		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_single_pdu),
		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_dbl_pdu_prem)
	);
	ztest_run_test_suite(test2);


	ztest_test_suite(test3,
		ztest_unit_test(test_setup),
		ztest_unit_test(test_unframed_disabled_sink)
	);
	ztest_run_test_suite(test3);


	/* Trig asserts to get code coverage (negative testcases) */
	sdu_interval = (3 * 1250);
	ztest_test_suite(test_assert_err,
		ztest_unit_test(test_trig_assert_isoal_sink_create),

		ztest_unit_test(test_setup),
		ztest_unit_test(test_trig_assert_isoal_rx_pdu_recombine_first),

		ztest_unit_test(test_setup),
		ztest_unit_test(test_trig_assert_isoal_rx_pdu_recombine_second),

		ztest_unit_test(test_trig_assert_isoal_get_sink_param_ref)
	);
	ztest_run_test_suite(test_assert_err);


	/* FRAMED TEST CASES */
	sdu_buf_len = SDU_BUF_MAX_LEN;
	sdu_interval = (4 * 1250); /* 4 PDUs per SDU interval */
	framed = 1;
	ztest_test_suite(test_framed,
		ztest_unit_test(test_setup),
		ztest_unit_test(test_framed_single_pdu),
		ztest_unit_test(test_setup),
		ztest_unit_test(test_framed_double_pdu),
		ztest_unit_test(test_setup),
		ztest_unit_test(test_framed_single_pdu_err),
		ztest_unit_test(test_setup),
		ztest_unit_test(test_framed_dbl_pdu_err),
		ztest_unit_test(test_framed_single_pdu),
		ztest_unit_test(test_setup),
		ztest_unit_test(test_framed_dbl_pdu_err),
		ztest_unit_test(test_framed_double_pdu),
		ztest_unit_test(test_setup),
		ztest_unit_test(test_framed_seq_err),
		ztest_unit_test(test_setup),
		ztest_unit_test(test_framed_padding),
		ztest_unit_test(test_setup),
		ztest_unit_test(test_framed_pdu_seq_err1),
		ztest_unit_test(test_setup),
		ztest_unit_test(test_framed_pdu_seq_err2)
	);
	ztest_run_test_suite(test_framed);

	ztest_test_suite(test_framed_errors,
		ztest_unit_test(test_setup),
		ztest_unit_test(test_framed_error1),
		ztest_unit_test(test_setup),
		ztest_unit_test(test_framed_error2)
	);
	ztest_run_test_suite(test_framed_errors);
}
