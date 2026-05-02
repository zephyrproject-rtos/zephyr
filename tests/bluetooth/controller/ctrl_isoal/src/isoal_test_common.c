/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/cntr.h"
#include "hal/ticker.h"

#include "util/memq.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "ll.h"
#include "lll.h"
#include "lll_conn_iso.h"
#include "lll_iso_tx.h"
#include "isoal.h"

#include "isoal_test_common.h"
#include "isoal_test_debug.h"

#define ULL_TIME_WRAPPING_POINT_US	(HAL_TICKER_TICKS_TO_US_64BIT(HAL_TICKER_CNTR_MASK))
#define ULL_TIME_SPAN_FULL_US		(ULL_TIME_WRAPPING_POINT_US + 1)

/**
 * Intializes a RX PDU buffer
 * @param[in] buf Pointer to buffer structure
 */
void isoal_test_init_rx_pdu_buffer(struct rx_pdu_meta_buffer *buf)
{
	memset(buf, 0, sizeof(struct rx_pdu_meta_buffer));
	buf->pdu_meta.meta = &buf->meta;
	buf->pdu_meta.pdu = (struct pdu_iso *) &buf->pdu[0];
}

/**
 * Initializes a RX SDU buffer
 * @param[in] buf Pointer to buffer structure
 */
void isoal_test_init_rx_sdu_buffer(struct rx_sdu_frag_buffer *buf)
{
	memset(buf, 0, sizeof(struct rx_sdu_frag_buffer));
}

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
void isoal_test_create_unframed_pdu(uint8_t llid,
				uint8_t *dataptr,
				uint8_t length,
				uint64_t payload_number,
				uint32_t timestamp,
				uint8_t  status,
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
	pdu_meta->pdu->len = length;
	memcpy(pdu_meta->pdu->payload, dataptr, length);

	isoal_test_debug_print_rx_pdu(pdu_meta);
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
uint16_t isoal_test_insert_segment(bool sc, bool cmplt, uint32_t time_offset, uint8_t *dataptr,
				uint8_t length, struct isoal_pdu_rx *pdu_meta)
{
	uint8_t seg_hdr[PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE];
	uint16_t pdu_payload_size;
	uint8_t hdr_write_size;
	uint16_t pdu_data_loc;

	pdu_payload_size = pdu_meta->pdu->len + length + PDU_ISO_SEG_HDR_SIZE +
			(sc ? 0 : PDU_ISO_SEG_TIMEOFFSET_SIZE);
	hdr_write_size = PDU_ISO_SEG_HDR_SIZE + (sc ? 0 : PDU_ISO_SEG_TIMEOFFSET_SIZE);
	memset(&seg_hdr, 0, sizeof(seg_hdr));

	zassert_true(pdu_payload_size <= TEST_RX_PDU_PAYLOAD_MAX,
		"pdu_payload_size (%d)", pdu_payload_size);

	/* Write header independent of endian dependent structures */
	WRITE_BIT(seg_hdr[0], 0, sc); /* sc */
	WRITE_BIT(seg_hdr[0], 1, cmplt); /* cmplt */
	seg_hdr[1] = length + (sc ? 0 : PDU_ISO_SEG_TIMEOFFSET_SIZE);

	if (!sc) {
		sys_put_le24(time_offset, &seg_hdr[PDU_ISO_SEG_HDR_SIZE]);
	}

	memcpy(&pdu_meta->pdu->payload[pdu_meta->pdu->len], &seg_hdr, hdr_write_size);
	pdu_meta->pdu->len += hdr_write_size;

	memcpy(&pdu_meta->pdu->payload[pdu_meta->pdu->len], dataptr, length);
	pdu_data_loc = pdu_meta->pdu->len;
	pdu_meta->pdu->len += length;

	isoal_test_debug_print_rx_pdu(pdu_meta);

	return pdu_data_loc;
}

/**
 * Create and fill in base information for a framed PDU
 * @param[In]     payload_number Payload number (Meta Information)
 * @param[In]     timestamp      Adjusted RX time stamp (CIS anchorpoint)
 * @param[In]     status         PDU error status
 * @param[In/Out] pdu_meta       PDU structure including meta information
 */
void isoal_test_create_framed_pdu_base(uint64_t payload_number, uint32_t timestamp, uint8_t  status,
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
	pdu_meta->pdu->len = 0;

	isoal_test_debug_print_rx_pdu(pdu_meta);
}

/**
 * Adds a single SDU framed segment to the given PDU
 * @param[In]     dataptr     Pointer to data to fill in segment
 * @param[In]     length      Length of data
 * @param[In]     time_offset Time offset
 * @param[In/Out] pdu_meta    PDU structure including meta information
 * @return                    PDU data location index
 */
uint16_t isoal_test_add_framed_pdu_single(uint8_t *dataptr, uint8_t length, uint32_t time_offset,
					struct isoal_pdu_rx *pdu_meta)
{
	zassert_not_null(pdu_meta, "");
	zassert_not_null(pdu_meta->meta, "");
	zassert_not_null(pdu_meta->pdu, "");

	return isoal_test_insert_segment(false, true, time_offset, dataptr, length, pdu_meta);
}

/**
 * Adds a starting SDU framed segment to the given PDU
 * @param[In]     dataptr     Pointer to data to fill in segment
 * @param[In]     length      Length of data
 * @param[In]     time_offset Time offset
 * @param[In/Out] pdu_meta    PDU structure including meta information
 * @return                    PDU data location index
 */
uint16_t isoal_test_add_framed_pdu_start(uint8_t *dataptr, uint8_t length, uint32_t time_offset,
					struct isoal_pdu_rx *pdu_meta)
{
	zassert_not_null(pdu_meta, "");
	zassert_not_null(pdu_meta->meta, "");
	zassert_not_null(pdu_meta->pdu, "");

	return isoal_test_insert_segment(false, false, time_offset, dataptr, length, pdu_meta);
}

/**
 * Adds a continuation SDU framed segment to the given PDU
 * @param[In]     dataptr     Pointer to data to fill in segment
 * @param[In]     length      Length of data
 * @param[In/Out] pdu_meta    PDU structure including meta information
 * @return                    PDU data location index
 */
uint16_t isoal_test_add_framed_pdu_cont(uint8_t *dataptr,
					uint8_t length,
					struct isoal_pdu_rx *pdu_meta)
{
	zassert_not_null(pdu_meta, "");
	zassert_not_null(pdu_meta->meta, "");
	zassert_not_null(pdu_meta->pdu, "");

	return isoal_test_insert_segment(true, false, 0, dataptr, length, pdu_meta);
}

/**
 * Adds an end SDU framed segment to the given PDU
 * @param[In]     dataptr     Pointer to data to fill in segment
 * @param[In]     length      Length of data
 * @param[In/Out] pdu_meta    PDU structure including meta information
 * @return                    PDU data location index
 */
uint16_t isoal_test_add_framed_pdu_end(uint8_t *dataptr,
				       uint8_t length,
				       struct isoal_pdu_rx *pdu_meta)
{
	zassert_not_null(pdu_meta, "");
	zassert_not_null(pdu_meta->meta, "");
	zassert_not_null(pdu_meta->pdu, "");

	return isoal_test_insert_segment(true, true, 0, dataptr, length, pdu_meta);
}

/**
 * Intializes a TX PDU buffer
 * @param[in] buf Pointer to buffer structure
 */
void isoal_test_init_tx_pdu_buffer(struct tx_pdu_meta_buffer *buf)
{
	memset(buf, 0, sizeof(struct tx_pdu_meta_buffer));
}

/**
 * Initializes a TX SDU buffer
 * @param[in] buf Pointer to buffer structure
 */
void isoal_test_init_tx_sdu_buffer(struct tx_sdu_frag_buffer *buf)
{
	memset(buf, 0, sizeof(struct tx_sdu_frag_buffer));
	buf->sdu_tx.dbuf = buf->sdu_payload;
}

/**
 * Initialize the given test data buffer with a ramp pattern
 * @param buf  Test data buffer pointer
 * @param size Length of test data
 */
void init_test_data_buffer(uint8_t *buf, uint16_t size)
{
	zassert_not_null(buf, "");

	for (uint16_t i = 0; i < size; i++) {
		buf[i] = (uint8_t)(i & 0x00FF);
	}
}

/**
 * @brief Wraps given time within the range of 0 to ULL_TIME_WRAPPING_POINT_US
 * @param  time_now  Current time value
 * @param  time_diff Time difference (signed)
 * @return           Wrapped time after difference
 */
uint32_t ull_get_wrapped_time_us(uint32_t time_now_us, int32_t time_diff_us)
{
	uint32_t result = ((uint64_t)time_now_us + ULL_TIME_SPAN_FULL_US + time_diff_us) %
				((uint64_t)ULL_TIME_SPAN_FULL_US);

	return result;
}
