/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Each segment header in a test would usually be written to when it is first
 * inserted and again when the segment is finalized.
 */
#define EXPECTED_SEG_HDR_WRITES (2)

/*------------------ PDU Allocation Callback ---------------------------------*/
/* Fake function */
FAKE_VALUE_FUNC(isoal_status_t, source_pdu_alloc_test, struct isoal_pdu_buffer *);

/* Customized buffering for data returned in pdu_buffer */
/* Queue for pdu_buffer return data */
static struct {
	struct isoal_pdu_buffer out[10];
	size_t buffer_size;
	size_t pos;

} custom_source_pdu_alloc_test_pdu_buffers;

/* Push to pdu_buffer queue */
static void push_custom_source_pdu_alloc_test_pdu_buffer(struct isoal_pdu_buffer *data)
{
	size_t buffer_size = custom_source_pdu_alloc_test_pdu_buffers.buffer_size;

	custom_source_pdu_alloc_test_pdu_buffers.out[buffer_size] = *data;
	zassert_true(custom_source_pdu_alloc_test_pdu_buffers.buffer_size <=
			     ARRAY_SIZE(custom_source_pdu_alloc_test_pdu_buffers.out),
		     "Maximum PDU buffers reached!!");

	custom_source_pdu_alloc_test_pdu_buffers.buffer_size++;
}

/* Customized routine for fake function */
static isoal_status_t custom_source_pdu_alloc_test(struct isoal_pdu_buffer *pdu_buffer)
{
	isoal_test_debug_trace_func_call(__func__, "IN");

	/* Return PDU buffer details as provided by the test */
	size_t pos = custom_source_pdu_alloc_test_pdu_buffers.pos;
	size_t buffer_size = custom_source_pdu_alloc_test_pdu_buffers.buffer_size;

	zassert_not_null(pdu_buffer);
	zassert_true(pos < buffer_size,
		     "No PDU buffers (Allocated %u, required %u)",
		     buffer_size,
		     pos);
	(void)memcpy(pdu_buffer,
		     &custom_source_pdu_alloc_test_pdu_buffers.out[pos],
		     sizeof(*pdu_buffer));

	custom_source_pdu_alloc_test_pdu_buffers.pos++;

	return source_pdu_alloc_test_fake.return_val;
}

#define SET_NEXT_PDU_ALLOC_BUFFER(_buf) push_custom_source_pdu_alloc_test_pdu_buffer(_buf)

#define PDU_ALLOC_TEST_RETURNS(_status) source_pdu_alloc_test_fake.return_val = _status

#define ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(_expected)                                               \
	zassert_equal(_expected,                                                                   \
		      source_pdu_alloc_test_fake.call_count,                                       \
		      "Expected %u got %u",                                                        \
		      _expected,                                                                   \
		      source_pdu_alloc_test_fake.call_count)

/*------------------ PDU Write Callback --------------------------------------*/
/* Fake function */
FAKE_VALUE_FUNC(isoal_status_t,
		source_pdu_write_test,
		struct isoal_pdu_buffer *,
		const size_t,
		const uint8_t *,
		const size_t);

FAKE_VOID_FUNC(source_pdu_write_test_handler, struct isoal_pdu_buffer);

/* Customized buffering for data returned in sdu_payload */
/* Queue for sdu_payload return data */
static struct {
	uint8_t out[20][TEST_TX_SDU_FRAG_PAYLOAD_MAX];
	size_t out_size[20];
	size_t buffer_size;
	size_t pos;

} custom_source_pdu_write_test_sdu_payloads;

/* Push to sdu_payload queue */
static void push_custom_source_pdu_write_test_sdu_payload(const uint8_t *data, const size_t length)
{
	size_t buffer_size = custom_source_pdu_write_test_sdu_payloads.buffer_size;

	zassert_true(length <= TEST_TX_SDU_FRAG_PAYLOAD_MAX,
		     "Length exceeds TEST_TX_SDU_FRAG_PAYLOAD_MAX");
	memcpy(&custom_source_pdu_write_test_sdu_payloads.out[buffer_size][0], data, length);
	custom_source_pdu_write_test_sdu_payloads.out_size[buffer_size] = length;
	zassert_true(buffer_size <= ARRAY_SIZE(custom_source_pdu_write_test_sdu_payloads.out),
		     "Maximum SDU payloads reached!!");

	custom_source_pdu_write_test_sdu_payloads.buffer_size++;
}

static void check_next_custom_source_pdu_write_test_sdu_payload(const uint8_t *data,
								const size_t length,
								const uint32_t line)
{
	size_t pos = custom_source_pdu_write_test_sdu_payloads.pos;
	size_t buffer_size = custom_source_pdu_write_test_sdu_payloads.buffer_size;

	zassert_true(pos < buffer_size, "%u exceeds received SDU payloads %u", pos, buffer_size);
	zassert_equal(length,
		      custom_source_pdu_write_test_sdu_payloads.out_size[pos],
		      "Expected %u != received %u",
		      length,
		      custom_source_pdu_write_test_sdu_payloads.out_size[pos]);
	for (size_t i = 0; i < custom_source_pdu_write_test_sdu_payloads.out_size[pos]; i++) {
		zassert_equal(custom_source_pdu_write_test_sdu_payloads.out[pos][i],
			      data[i],
			      "[Line %lu] deviation at index %u, expected %u, got %u",
			      line,
			      i,
			      data[i],
			      custom_source_pdu_write_test_sdu_payloads.out[pos][i]);
	}
	custom_source_pdu_write_test_sdu_payloads.pos++;
}

/* Customized routine for fake function */
static isoal_status_t custom_source_pdu_write_test(struct isoal_pdu_buffer *pdu_buffer,
						   const size_t pdu_offset,
						   const uint8_t *sdu_payload,
						   const size_t consume_len)
{
	isoal_test_debug_trace_func_call(__func__, "IN");

#if defined(DEBUG_TEST)
	zassert_not_null(pdu_buffer, "");
	zassert_not_null(sdu_payload, "");

	zassert_false((pdu_offset + consume_len) > pdu_buffer->size,
		      "Write size of %u at offset %u exceeds buffer!",
		      consume_len,
		      pdu_offset);

	/* Copy source to destination at given offset */
	memcpy(&pdu_buffer->pdu->payload[pdu_offset], sdu_payload, consume_len);
#endif

	/* Return SDU payload details as provided by the test */
	zassert_not_null(sdu_payload, NULL);

	source_pdu_write_test_handler(*pdu_buffer);

	push_custom_source_pdu_write_test_sdu_payload(sdu_payload, consume_len);

	return source_pdu_write_test_fake.return_val;
}

#define PDU_WRITE_TEST_RETURNS(_status) source_pdu_write_test_fake.return_val = _status;

#define ZASSERT_PDU_WRITE_TEST(_typ, _pdu_buffer, _pdu_offset, _sdu_payload, _consume_len)         \
	zassert_equal_ptr(_pdu_buffer.handle,                                                      \
			  source_pdu_write_test_handler_fake.arg0_##_typ.handle,                   \
			  "\t\t%p != %p",                                                          \
			  _pdu_buffer.handle,                                                      \
			  source_pdu_write_test_handler_fake.arg0_##_typ.handle);                  \
	zassert_equal_ptr(_pdu_buffer.pdu,                                                         \
			  source_pdu_write_test_handler_fake.arg0_##_typ.pdu,                      \
			  "\t\t%p != %p",                                                          \
			  _pdu_buffer.pdu,                                                         \
			  source_pdu_write_test_handler_fake.arg0_##_typ.pdu);                     \
	zassert_equal(_pdu_buffer.size,                                                            \
		      source_pdu_write_test_handler_fake.arg0_##_typ.size,                         \
		      "\t\t%u != %u",                                                              \
		      _pdu_buffer.size,                                                            \
		      source_pdu_write_test_handler_fake.arg0_##_typ.size);                        \
	zassert_equal(_pdu_offset,                                                                 \
		      source_pdu_write_test_fake.arg1_##_typ,                                      \
		      "\t\t%u != %u",                                                              \
		      _pdu_offset,                                                                 \
		      source_pdu_write_test_fake.arg1_##_typ);                                     \
	zassert_equal(_consume_len,                                                                \
		      source_pdu_write_test_fake.arg3_##_typ,                                      \
		      "\t\t%u != %u",                                                              \
		      _consume_len,                                                                \
		      source_pdu_write_test_fake.arg3_##_typ);                                     \
	check_next_custom_source_pdu_write_test_sdu_payload((const uint8_t *)_sdu_payload,         \
							    _consume_len, __LINE__)

#define ZASSERT_PDU_WRITE_TEST_CALL_COUNT(_expected)                                               \
	zassert_equal(_expected,                                                                   \
		      source_pdu_write_test_fake.call_count,                                       \
		      "Expected %u, got %u",                                                       \
		      _expected,                                                                   \
		      source_pdu_write_test_fake.call_count)

/*------------------ PDU Emit Callback --------------------------------------*/
/**
 * Emit the encoded node to the transmission queue
 * @param node_tx TX node to enqueue
 * @param handle  CIS/BIS handle
 * @return        Error status of enqueue operation
 */

/* Fake function */
FAKE_VALUE_FUNC(isoal_status_t, source_pdu_emit_test, struct node_tx_iso *, const uint16_t);

FAKE_VOID_FUNC(source_pdu_emit_test_handler, struct node_tx_iso, struct pdu_iso);

/* Customized routine for fake function */
static isoal_status_t custom_source_pdu_emit_test(struct node_tx_iso *node_tx,
						  const uint16_t handle)
{
	isoal_test_debug_trace_func_call(__func__, "IN");

	zassert_not_null(node_tx);
	zassert_not_null(node_tx->pdu);
	source_pdu_emit_test_handler(*node_tx, *((struct pdu_iso *)node_tx->pdu));

	isoal_test_debug_print_tx_pdu(node_tx);

	return source_pdu_emit_test_fake.return_val;
}

#define PDU_EMIT_TEST_RETURNS(_status) source_pdu_emit_test_fake.return_val = _status

#define ZASSERT_PDU_EMIT_TEST(_typ,                                                                \
			      _node_tx,                                                            \
			      _payload_count,                                                      \
			      _sdu_fragments,                                                      \
			      _ll_id,                                                              \
			      _length,                                                             \
			      _handle)                                                             \
	zassert_equal_ptr(_node_tx,                                                                \
			  source_pdu_emit_test_fake.arg0_##_typ,                                   \
			  "\t\t%p != %p",                                                          \
			  _node_tx,                                                                \
			  source_pdu_emit_test_fake.arg0_##_typ);                                  \
	zassert_equal(_payload_count,                                                              \
		      source_pdu_emit_test_handler_fake.arg0_##_typ.payload_count,                 \
		      "\t\t%u != %u",                                                              \
		      _payload_count,                                                              \
		      source_pdu_emit_test_handler_fake.arg0_##_typ.payload_count);                \
	zassert_equal(_sdu_fragments,                                                              \
		      source_pdu_emit_test_handler_fake.arg0_##_typ.sdu_fragments,                 \
		      "\t\t%u != %u",                                                              \
		      _sdu_fragments,                                                              \
		      source_pdu_emit_test_handler_fake.arg0_##_typ.sdu_fragments);                \
	zassert_equal(_ll_id,                                                                      \
		      source_pdu_emit_test_handler_fake.arg1_##_typ.ll_id,                         \
		      "\t\t%u != %u",                                                              \
		      _ll_id,                                                                      \
		      source_pdu_emit_test_handler_fake.arg1_##_typ.ll_id);                        \
	zassert_equal(_length,                                                                     \
		      source_pdu_emit_test_handler_fake.arg1_##_typ.len,                           \
		      "\t\t%u != %u",                                                              \
		      _length,                                                                     \
		      source_pdu_emit_test_handler_fake.arg1_##_typ.len);                          \
	zassert_equal(bt_iso_handle(_handle),                                                      \
		      source_pdu_emit_test_fake.arg1_##_typ,                                       \
		      "\t\t%08x != %08x",                                                          \
		      bt_iso_handle(_handle),                                                      \
		      source_pdu_emit_test_fake.arg1_##_typ)

#define ZASSERT_PDU_EMIT_TEST_CALL_COUNT(_expected)                                                \
	zassert_equal(_expected,                                                                   \
		      source_pdu_emit_test_fake.call_count,                                        \
		      "Expected %u, got %u",                                                       \
		      _expected,                                                                   \
		      source_pdu_emit_test_fake.call_count)

/*------------------ PDU Release Callback --------------------------------------*/
/**
 * Test releasing the given payload back to the memory pool.
 * @param node_tx TX node to release or forward
 * @param handle  CIS/BIS handle
 * @param status  Reason for release
 * @return        Error status of release operation
 */

/* Fake function */
FAKE_VALUE_FUNC(isoal_status_t,
		source_pdu_release_test,
		struct node_tx_iso *,
		const uint16_t,
		const isoal_status_t);

/* Customized routine for fake function */
static isoal_status_t custom_source_pdu_release_test(struct node_tx_iso *node_tx,
						     const uint16_t handle,
						     const isoal_status_t status)
{
	isoal_test_debug_trace_func_call(__func__, "IN");

	return source_pdu_release_test_fake.return_val;
}

#define PDU_RELEASE_TEST_RETURNS(_status) source_pdu_release_test_fake.return_val = _status

#define ZASSERT_PDU_RELEASE_TEST(_typ, _node_tx, _handle, _status)                                 \
	zassert_equal_ptr(_node_tx,                                                                \
			  source_pdu_release_test_fake.arg0_##_typ,                                \
			  "\t\t%p != %p",                                                          \
			  _node_tx,                                                                \
			  source_pdu_release_test_fake.arg0_##_typ);                               \
	zassert_equal(_handle,                                                                     \
		      source_pdu_release_test_fake.arg1_##_typ,                                    \
		      "\t\t%u != %u",                                                              \
		      _handle,                                                                     \
		      source_pdu_release_test_fake.arg1_##_typ);                                   \
	zassert_equal(_status,                                                                     \
		      source_pdu_release_test_fake.arg2_##_typ,                                    \
		      "\t\t%u != %u",                                                              \
		      _status,                                                                     \
		      source_pdu_release_test_fake.arg2_##_typ)

#define ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(_expected)                                             \
	zassert_equal(_expected,                                                                   \
		      source_pdu_release_test_fake.call_count,                                     \
		      "Expected %u, got %u",                                                       \
		      _expected,                                                                   \
		      source_pdu_release_test_fake.call_count)

/**
 * TX common setup before running tests
 * @param f Input configuration parameters
 */
static void isoal_test_tx_common_before(void *f)
{
	ARG_UNUSED(f);

	custom_source_pdu_alloc_test_pdu_buffers.buffer_size = 0;
	custom_source_pdu_alloc_test_pdu_buffers.pos = 0;
	custom_source_pdu_write_test_sdu_payloads.buffer_size = 0;
	custom_source_pdu_write_test_sdu_payloads.pos = 0;
	RESET_FAKE(source_pdu_alloc_test);
	RESET_FAKE(source_pdu_write_test);
	RESET_FAKE(source_pdu_write_test_handler);
	RESET_FAKE(source_pdu_emit_test);
	RESET_FAKE(source_pdu_emit_test_handler);
	RESET_FAKE(source_pdu_release_test);

	FFF_RESET_HISTORY();

	source_pdu_alloc_test_fake.custom_fake = custom_source_pdu_alloc_test;
	source_pdu_write_test_fake.custom_fake = custom_source_pdu_write_test;
	source_pdu_emit_test_fake.custom_fake = custom_source_pdu_emit_test;
	source_pdu_release_test_fake.custom_fake = custom_source_pdu_release_test;
}

/**
 * Wrapper totest time difference
 * @param  time_before Subtrahend
 * @param  time_after  Minuend
 * @param  result      Difference if valid
 * @return             Validity
 */
static bool isoal_get_time_diff_test(uint32_t time_before, uint32_t time_after, uint32_t *result)
{
	bool valid = isoal_get_time_diff(time_before, time_after, result);

#if defined(DEBUG_TEST)
	if (valid) {
		PRINT("[isoal_get_time_diff] time_before %12lu time_after %12lu result %lu\n",
		      time_before,
		      time_after,
		      *result);
	} else {
		PRINT("[isoal_get_time_diff] time_before %12lu time_after %12lu result INVALID\n",
		      time_before,
		      time_after);
	}
#endif

	return valid;
}

/**
 * Basic setup of a single source for any TX test
 * @param  handle            Stream handle
 * @param  role              Peripheral / Central / Broadcast
 * @param  framed            PDU framing
 * @param  burst_number      BN
 * @param  flush_timeout     FT
 * @param  max_octets        Max PDU Size
 * @param  sdu_interval      SDU Interval (us)
 * @param  iso_interval_int  ISO Interval (integer multiple of 1250us)
 * @param  stream_sync_delay CIS / BIS sync delay
 * @param  group_sync_delay  CIG / BIG sync delay
 * @return                   Newly created source handle
 */
static isoal_source_handle_t basic_tx_test_setup(uint16_t handle,
						 uint8_t role,
						 uint8_t framed,
						 uint8_t burst_number,
						 uint8_t flush_timeout,
						 uint8_t max_octets,
						 uint32_t sdu_interval,
						 uint16_t iso_interval_int,
						 uint32_t stream_sync_delay,
						 uint32_t group_sync_delay)
{
	isoal_source_handle_t source_hdl;
	isoal_status_t err;

#if defined(DEBUG_TEST)
	PRINT("TX Test Setup:\n\tHandle 0x%04x\n\tRole %s\n\tFraming %s"
	      "\n\tBN %u\n\tFT %d\n\tMax PDU %u\n\tISO Interval %dus"
	      "\n\tSDU Interval %dus\n\tStream Sync Delay %dus"
	      "\n\tGroup Sync Delay %dus\n\n",
	      handle,
	      ROLE_TO_STR(role),
	      framed ? "Framed" : "Unframed",
	      burst_number,
	      flush_timeout,
	      max_octets,
	      (iso_interval_int * ISO_INT_UNIT_US),
	      sdu_interval,
	      stream_sync_delay,
	      group_sync_delay);
#endif

	ztest_set_assert_valid(false);

	err = isoal_init();
	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	err = isoal_reset();
	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Create a source based on global parameters */
	err = isoal_source_create(handle,
				  role,
				  framed,
				  burst_number,
				  flush_timeout,
				  max_octets,
				  sdu_interval,
				  iso_interval_int,
				  stream_sync_delay,
				  group_sync_delay,
				  source_pdu_alloc_test,
				  source_pdu_write_test,
				  source_pdu_emit_test,
				  source_pdu_release_test,
				  &source_hdl);
	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Enable newly created source */
	isoal_source_enable(source_hdl);

	return source_hdl;
}

/**
 * Creates a SDU fragment according to the provided configuration.
 * @param[In]  sdu_state        Fragment type (Single / Start / Cont. / End)
 * @param[In]  dataptr          Test data to fill SDU payload
 * @param[In]  length           Length of SDU fragment
 * @param[In]  sdu_total_length Total size of the SDU
 * @param[In]  packet_number    SDU packet sequence number
 * @param[In]  timestamp        SDU timestamp at source
 * @param[In]  ref_point        CIG / BIG reference point
 * @param[In]  target_event     Event number requested
 * @param[Out] sdu_tx           SDU buffer
 */
static void isoal_test_create_sdu_fagment(uint8_t sdu_state,
					  uint8_t *dataptr,
					  uint16_t length,
					  uint16_t sdu_total_length,
					  uint16_t packet_number,
					  uint32_t timestamp,
					  uint32_t cntr_timestamp,
					  uint32_t ref_point,
					  uint64_t target_event,
					  struct isoal_sdu_tx *sdu_tx)
{
	sdu_tx->sdu_state = sdu_state;
	sdu_tx->packet_sn = packet_number;
	sdu_tx->iso_sdu_length = sdu_total_length;
	sdu_tx->time_stamp = timestamp;
	sdu_tx->cntr_time_stamp = cntr_timestamp;
	sdu_tx->grp_ref_point = ref_point;
	sdu_tx->target_event = target_event;
	memcpy(sdu_tx->dbuf, dataptr, length);
	sdu_tx->size = length;

	isoal_test_debug_print_tx_sdu(sdu_tx);
}

/**
 * Test Suite  :   TX basic test
 *
 * Test creating and destroying sources upto the maximum, with randomized
 * configuration parameters.
 */
ZTEST(test_tx_basics, test_source_isoal_test_create_destroy)
{
	isoal_sink_handle_t source_hdl[CONFIG_BT_CTLR_ISOAL_SOURCES];
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	uint8_t sdu_interval_int;
	uint8_t iso_interval_int;
	uint8_t flush_timeout;
	uint32_t iso_interval;
	uint32_t sdu_interval;
	uint8_t burst_number;
	uint8_t pdus_per_sdu;
	uint8_t max_octets;
	isoal_status_t res;
	uint16_t handle;
	bool framed;

	res = isoal_init();
	zassert_equal(res, ISOAL_STATUS_OK, "res = 0x%02x", res);

	res = isoal_reset();
	zassert_equal(res, ISOAL_STATUS_OK, "res = 0x%02x", res);

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
		max_octets = 40;
		sdu_interval_int = 1;
		iso_interval_int = 1;
		iso_interval = iso_interval_int * ISO_INT_UNIT_US;
		sdu_interval = sdu_interval_int * ISO_INT_UNIT_US;
		stream_sync_delay = iso_interval - 200;
		group_sync_delay = iso_interval - 50;

		ztest_set_assert_valid(false);

		for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SOURCES; i++) {
			res = ISOAL_STATUS_ERR_UNSPECIFIED;
			source_hdl[i] = 0xFF;

			pdus_per_sdu = (burst_number * sdu_interval) / iso_interval;

			res = isoal_source_create(handle,
						  role,
						  framed,
						  burst_number,
						  flush_timeout,
						  max_octets,
						  sdu_interval,
						  iso_interval_int,
						  stream_sync_delay,
						  group_sync_delay,
						  source_pdu_alloc_test,
						  source_pdu_write_test,
						  source_pdu_emit_test,
						  source_pdu_release_test,
						  &source_hdl[i]);

			zassert_equal(isoal_global.source_allocated[source_hdl[i]],
				      ISOAL_ALLOC_STATE_TAKEN,
				      "");

			zassert_equal(isoal_global.source_state[source_hdl[i]].session.pdus_per_sdu,
				      pdus_per_sdu,
				      "%s pdus_per_sdu %d should be %d for:\n\tBN %d"
				      "\n\tFT %d\n\tISO Interval %dus"
				      "\n\tSDU Interval %dus"
				      "\n\tStream Sync Delay %dus"
				      "\n\tGroup Sync Delay %dus",
				      (framed ? "Framed" : "Unframed"),
				      isoal_global.source_state[source_hdl[i]].session.pdus_per_sdu,
				      pdus_per_sdu,
				      burst_number,
				      flush_timeout,
				      iso_interval,
				      sdu_interval,
				      stream_sync_delay,
				      group_sync_delay);

			zassert_equal(res,
				      ISOAL_STATUS_OK,
				      "Source %d in role %s creation failed!",
				      i,
				      ROLE_TO_STR(role));

			isoal_source_enable(source_hdl[i]);

			zassert_equal(isoal_global.source_state[source_hdl[i]].pdu_production.mode,
				      ISOAL_PRODUCTION_MODE_ENABLED,
				      "Source %d in role %s enable failed!",
				      i,
				      ROLE_TO_STR(role));

			framed = !framed;
			burst_number++;
			flush_timeout = (flush_timeout % 3) + 1;
			max_octets += max_octets / 2;
			sdu_interval_int++;
			iso_interval_int = iso_interval_int * sdu_interval_int;
			sdu_interval = (sdu_interval_int * ISO_INT_UNIT_US) - (framed ? 100 : 0);
			iso_interval = iso_interval_int * ISO_INT_UNIT_US;
			stream_sync_delay = iso_interval - (200 * i);
			group_sync_delay = iso_interval - 50;
		}

		/* Destroy in order */
		for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SOURCES; i++) {
			isoal_source_destroy(source_hdl[i]);

			zassert_equal(isoal_global.source_allocated[source_hdl[i]],
				      ISOAL_ALLOC_STATE_FREE,
				      "Source destruction failed!");

			zassert_equal(isoal_global.source_state[source_hdl[i]].pdu_production.mode,
				      ISOAL_PRODUCTION_MODE_DISABLED,
				      "Source disable failed!");
		}
	}
}

/**
 * Test Suite  :   TX basic test
 *
 * Test error return on exceeding the maximum number of sources available.
 */
ZTEST(test_tx_basics, test_source_isoal_test_create_err)
{
	isoal_source_handle_t source_hdl[CONFIG_BT_CTLR_ISOAL_SOURCES + 1];
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint8_t flush_timeout;
	uint32_t sdu_interval;
	uint8_t burst_number;
	uint8_t max_octets;
	isoal_status_t res;
	uint16_t handle;
	uint8_t role;
	bool framed;

	handle = 0x8000;
	role = ISOAL_ROLE_PERIPHERAL;
	burst_number = 1;
	max_octets = 40;
	flush_timeout = 1;
	framed = false;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	stream_sync_delay = ISO_INT_UNIT_US - 200;
	group_sync_delay = ISO_INT_UNIT_US - 50;

	res = isoal_init();
	zassert_equal(res, ISOAL_STATUS_OK, "res = 0x%02x", res);

	res = isoal_reset();
	zassert_equal(res, ISOAL_STATUS_OK, "res = 0x%02x", res);

	for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SOURCES; i++) {
		res = isoal_source_create(handle,
					  role,
					  framed,
					  burst_number,
					  flush_timeout,
					  max_octets,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay,
					  source_pdu_alloc_test,
					  source_pdu_write_test,
					  source_pdu_emit_test,
					  source_pdu_release_test,
					  &source_hdl[i]);

		zassert_equal(res,
			      ISOAL_STATUS_OK,
			      "Source %d in role %s creation failed!",
			      i,
			      ROLE_TO_STR(role));
	}

	res = isoal_source_create(handle,
				  role,
				  framed,
				  burst_number,
				  flush_timeout,
				  max_octets,
				  sdu_interval,
				  iso_interval_int,
				  stream_sync_delay,
				  group_sync_delay,
				  source_pdu_alloc_test,
				  source_pdu_write_test,
				  source_pdu_emit_test,
				  source_pdu_release_test,
				  &source_hdl[CONFIG_BT_CTLR_ISOAL_SOURCES]);

	zassert_equal(res,
		      ISOAL_STATUS_ERR_SOURCE_ALLOC,
		      "Source creation did not return error as expected!");
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is less than the PDU buffer size
 */
ZTEST(test_tx_unframed, test_tx_unframed_1_sdu_1_frag_1_pdu_maxPDU)
{
	uint8_t testdata[TEST_TX_PDU_PAYLOAD_MAX - 5];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, TEST_TX_PDU_PAYLOAD_MAX - 5);
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	testdata_indx = 0;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX - 5;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 false,             /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* PDU 1 */
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(val,
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(val,
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Test PDU release */
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	isoal_tx_pdu_release(source_hdl, &tx_pdu_meta_buf.node_tx);

	ZASSERT_PDU_RELEASE_TEST(val,
				 &tx_pdu_meta_buf.node_tx,
				 isoal_global.source_state[source_hdl].session.handle,
				 ISOAL_STATUS_OK);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is greater than the PDU buffer size
 */
ZTEST(test_tx_unframed, test_tx_unframed_1_sdu_1_frag_1_pdu_bufSize)
{
	uint8_t testdata[TEST_TX_PDU_PAYLOAD_MAX];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, TEST_TX_PDU_PAYLOAD_MAX);
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX;
	testdata_indx = 0;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX;
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 false,             /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* PDU 1 */
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(val,
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(val,
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of a single SDU contained in a single fragment
 * into three PDUs where Max PDU is less than the PDU buffer size
 */
ZTEST(test_tx_unframed, test_tx_unframed_1_sdu_1_frag_3_pdu)
{
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint8_t testdata[100];
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, 100);
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = 100;
	testdata_indx = 0;
	testdata_size = 100;
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 false,             /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* PDU 1 */
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = max_octets;
	sdu_fragments = 0;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_START_CONTINUE,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 2 */
	payload_number++;
	sdu_read_loc += pdu_write_size;
	sdu_fragments = 0;

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_START_CONTINUE,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 3 */
	payload_number++;
	sdu_read_loc += pdu_write_size;
	pdu_write_size = 30;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[2],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of a single SDU contained in three fragments
 * into a single PDU where Max PDU is greater than the PDU buffer size
 */
ZTEST(test_tx_unframed, test_tx_unframed_1_sdu_3_frag_1_pdu)
{
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint8_t testdata[TEST_TX_PDU_PAYLOAD_MAX];
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, TEST_TX_PDU_PAYLOAD_MAX);
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX;
	testdata_indx = 0;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX / 3;
	payload_number = event_number * BN;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 false,             /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* PDU 1 */
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX / 3;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU Frag 2 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += TEST_TX_PDU_PAYLOAD_MAX / 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size += TEST_TX_PDU_PAYLOAD_MAX / 3;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */
	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(1);

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU Frag 3 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX;

	isoal_test_create_sdu_fagment(BT_ISO_END,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */
	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(1);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of a single SDU contained in three fragments
 * into two PDUs where Max PDU is greater than the PDU buffer size
 */
ZTEST(test_tx_unframed, test_tx_unframed_1_sdu_3_frag_2_pdu)
{
	struct tx_pdu_meta_buffer tx_pdu_meta_buf[2];
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer[2];
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint8_t testdata[TEST_TX_PDU_PAYLOAD_MAX * 2];
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, TEST_TX_PDU_PAYLOAD_MAX * 2);
	pdu_buffer[0].handle = (void *)&tx_pdu_meta_buf[0].node_tx;
	pdu_buffer[0].pdu = (struct pdu_iso *)tx_pdu_meta_buf[0].node_tx.pdu;
	pdu_buffer[0].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[1].handle = (void *)&tx_pdu_meta_buf[1].node_tx;
	pdu_buffer[1].pdu = (struct pdu_iso *)tx_pdu_meta_buf[1].node_tx.pdu;
	pdu_buffer[1].size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX * 2;
	testdata_indx = 0;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 false,             /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* PDU 1 */
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU Frag 2 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	/* PDU should not be allocated (Allocated for PDU 2) */
	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(2);

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_START_CONTINUE,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* PDU 2 */
	payload_number++;
	sdu_read_loc = pdu_write_size;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc);
	pdu_write_loc = 0;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(1);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU Frag 3 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX * 2;

	isoal_test_create_sdu_fagment(BT_ISO_END,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */
	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(2);

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests boundary conditions for the time difference function
 */
ZTEST(test_tx_unframed, test_tx_time_diff)
{
	uint32_t time_before;
	uint32_t time_after;
	uint32_t result;
	bool valid;

	result = 0;

	/* Check that the difference from maximum to 0 is 1 */
	time_before = ISOAL_TIME_WRAPPING_POINT_US;
	time_after = 0;
	valid = isoal_get_time_diff_test(time_before, time_after, &result);
	zassert_true(valid, NULL);
	zassert_equal(result, 1, "%lu != %lu", result, 1);

	/* Check that if time_before is ahead of time_after the result is
	 * invalid
	 */
	time_before = 0;
	time_after = ISOAL_TIME_WRAPPING_POINT_US;
	valid = isoal_get_time_diff_test(time_before, time_after, &result);
	zassert_false(valid, NULL);

	time_before = ISOAL_TIME_WRAPPING_POINT_US;
	time_after = ISOAL_TIME_WRAPPING_POINT_US - 1;
	valid = isoal_get_time_diff_test(time_before, time_after, &result);
	zassert_false(valid, NULL);

	time_before = 1;
	time_after = 0;
	valid = isoal_get_time_diff_test(time_before, time_after, &result);
	zassert_false(valid, NULL);

	time_before = ISOAL_TIME_MID_POINT_US;
	time_after = ISOAL_TIME_MID_POINT_US - 1;
	valid = isoal_get_time_diff_test(time_before, time_after, &result);
	zassert_false(valid, NULL);

	time_before = ISOAL_TIME_MID_POINT_US + 1;
	time_after = ISOAL_TIME_MID_POINT_US;
	valid = isoal_get_time_diff_test(time_before, time_after, &result);
	zassert_false(valid, NULL);

	time_before = ISOAL_TIME_MID_POINT_US + 1;
	time_after = ISOAL_TIME_MID_POINT_US - 1;
	valid = isoal_get_time_diff_test(time_before, time_after, &result);
	zassert_false(valid, NULL);

	/* Check valid results that are 0 */
	time_before = 0;
	time_after = 0;
	valid = isoal_get_time_diff_test(time_before, time_after, &result);
	zassert_true(valid, NULL);
	zassert_equal(result, 0, "%lu != %lu", result, 0);

	time_before = ISOAL_TIME_WRAPPING_POINT_US;
	time_after = ISOAL_TIME_WRAPPING_POINT_US;
	valid = isoal_get_time_diff_test(time_before, time_after, &result);
	zassert_true(valid, NULL);
	zassert_equal(result, 0, "%lu != %lu", result, 0);

	time_before = ISOAL_TIME_MID_POINT_US;
	time_after = ISOAL_TIME_MID_POINT_US;
	valid = isoal_get_time_diff_test(time_before, time_after, &result);
	zassert_true(valid, NULL);
	zassert_equal(result, 0, "%lu != %lu", result, 0);

	/* Check valid results across the mid-point */
	time_before = ISOAL_TIME_MID_POINT_US;
	time_after = ISOAL_TIME_MID_POINT_US + 1;
	valid = isoal_get_time_diff_test(time_before, time_after, &result);
	zassert_true(valid, NULL);
	zassert_equal(result, 1, "%lu != %lu", result, 1);

	time_before = ISOAL_TIME_MID_POINT_US - 1;
	time_after = ISOAL_TIME_MID_POINT_US;
	valid = isoal_get_time_diff_test(time_before, time_after, &result);
	zassert_true(valid, NULL);
	zassert_equal(result, 1, "%lu != %lu", result, 1);

	time_before = ISOAL_TIME_MID_POINT_US - 1;
	time_after = ISOAL_TIME_MID_POINT_US + 1;
	valid = isoal_get_time_diff_test(time_before, time_after, &result);
	zassert_true(valid, NULL);
	zassert_equal(result, 2, "%lu != %lu", result, 2);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is less than the PDU buffer size
 */
ZTEST(test_tx_unframed, test_tx_unframed_2_sdu_1_frag_2_pdu_ts_wrap1)
{
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	uint32_t tx_sync_timestamp_expected;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	uint32_t tx_sync_offset_expected;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint16_t tx_sync_seq_expected;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t tx_sync_timestamp;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t tx_sync_offset;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint8_t testdata[33];
	uint16_t tx_sync_seq;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, 33);
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = ISOAL_TIME_WRAPPING_POINT_US;
	ref_point = ISOAL_TIME_WRAPPING_POINT_US;
	sdu_total_size = 23;
	testdata_indx = 0;
	testdata_size = 23;
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = 23;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 false,             /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_packet_number++;
	sdu_timestamp = sdu_interval - 1;
	sdu_total_size = 10;
	testdata_indx = 0;
	testdata_size = 10;
	sdu_fragments = 0;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	payload_number++;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = testdata_size;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Check TX Sync info */
	tx_sync_seq_expected = 2;
	tx_sync_timestamp_expected = (iso_interval_int * ISO_INT_UNIT_US) - 1;
	tx_sync_offset_expected = 0;

	err = isoal_tx_get_sync_info(source_hdl, &tx_sync_seq, &tx_sync_timestamp, &tx_sync_offset);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
	zassert_equal(tx_sync_seq, tx_sync_seq_expected, "%u != %u", tx_sync_seq, 2);
	zassert_equal(tx_sync_timestamp, tx_sync_timestamp_expected, "%u != %u", tx_sync_seq, 2);
	zassert_equal(tx_sync_offset, tx_sync_offset_expected, "%u != %u", tx_sync_seq, 0);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of two SDUs containing three fragments each
 * into two PDUs each where Max PDU is greater than the PDU buffer size
 */
ZTEST(test_tx_unframed, test_tx_unframed_2_sdu_3_frag_4_pdu)
{
	struct tx_pdu_meta_buffer tx_pdu_meta_buf[2];
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer[2];
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint8_t testdata[TEST_TX_PDU_PAYLOAD_MAX * 2];
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 2;
	sdu_interval = ISO_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 4;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, TEST_TX_PDU_PAYLOAD_MAX * 2);
	pdu_buffer[0].handle = (void *)&tx_pdu_meta_buf[0].node_tx;
	pdu_buffer[0].pdu = (struct pdu_iso *)tx_pdu_meta_buf[0].node_tx.pdu;
	pdu_buffer[0].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[1].handle = (void *)&tx_pdu_meta_buf[1].node_tx;
	pdu_buffer[1].pdu = (struct pdu_iso *)tx_pdu_meta_buf[1].node_tx.pdu;
	pdu_buffer[1].size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX * 2;
	testdata_indx = 0;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 false,             /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments++;

	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(1);

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 1 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_START_CONTINUE,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* PDU 2 */
	payload_number++;
	sdu_read_loc = pdu_write_size;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc);
	pdu_write_loc = 0;
	sdu_fragments = 1;

	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(2);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(1);

	/* SDU 1 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX * 2;

	isoal_test_create_sdu_fagment(BT_ISO_END,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */
	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(2);

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, TEST_TX_PDU_PAYLOAD_MAX * 2);
	sdu_packet_number++;
	event_number = 2000;
	sdu_timestamp = 9249 + sdu_interval;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX * 2;
	testdata_indx = 0;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments = 0;

	isoal_test_create_sdu_fagment(BT_ISO_START,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	payload_number++;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments++;

	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(3);

	ZASSERT_PDU_WRITE_TEST(history[4],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(2);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	ZASSERT_PDU_WRITE_TEST(history[5],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[2],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_START_CONTINUE,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* PDU 4 */
	payload_number++;
	sdu_read_loc = pdu_write_size;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc);
	pdu_write_loc = 0;
	sdu_fragments = 1;

	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(4);

	ZASSERT_PDU_WRITE_TEST(history[6],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(3);

	/* SDU 2 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX * 2;

	isoal_test_create_sdu_fagment(BT_ISO_END,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 4 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */
	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(4);

	ZASSERT_PDU_WRITE_TEST(history[7],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[3],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of two SDUs containing three fragments each
 * into two PDUs each where Max PDU is greater than the PDU buffer size
 * with padding
 */
ZTEST(test_tx_unframed, test_tx_unframed_2_sdu_3_frag_4_pdu_padding)
{
	struct tx_pdu_meta_buffer tx_pdu_meta_buf[3];
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer[3];
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint8_t testdata[TEST_TX_PDU_PAYLOAD_MAX * 2];
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 2;
	sdu_interval = ISO_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 8;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[2]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, TEST_TX_PDU_PAYLOAD_MAX * 2);
	pdu_buffer[0].handle = (void *)&tx_pdu_meta_buf[0].node_tx;
	pdu_buffer[0].pdu = (struct pdu_iso *)tx_pdu_meta_buf[0].node_tx.pdu;
	pdu_buffer[0].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[1].handle = (void *)&tx_pdu_meta_buf[1].node_tx;
	pdu_buffer[1].pdu = (struct pdu_iso *)tx_pdu_meta_buf[1].node_tx.pdu;
	pdu_buffer[1].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[2].handle = (void *)&tx_pdu_meta_buf[2].node_tx;
	pdu_buffer[2].pdu = (struct pdu_iso *)tx_pdu_meta_buf[2].node_tx.pdu;
	pdu_buffer[2].size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX * 2;
	testdata_indx = 0;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 false,             /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[2]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[2]);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments++;

	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(1);

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 1 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_START_CONTINUE,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 2 */
	payload_number++;
	sdu_read_loc = pdu_write_size;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc);
	pdu_write_loc = 0;
	sdu_fragments = 1;

	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(2);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(1);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 1 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX * 2;

	isoal_test_create_sdu_fagment(BT_ISO_END,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Padding PDUs */
	/* Padding 1 */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	/* PDU should not be written to */
	ZASSERT_PDU_WRITE_TEST_CALL_COUNT(4);

	ZASSERT_PDU_EMIT_TEST(history[2],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_START_CONTINUE,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Padding 2 */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[2]);
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(4);

	/* PDU should not be written to */
	ZASSERT_PDU_WRITE_TEST_CALL_COUNT(4);

	ZASSERT_PDU_EMIT_TEST(history[3],
			      &tx_pdu_meta_buf[2].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_START_CONTINUE,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, TEST_TX_PDU_PAYLOAD_MAX * 2);
	sdu_packet_number++;
	event_number = 2000;
	sdu_timestamp = 9249 + sdu_interval;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX * 2;
	testdata_indx = 0;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments = 0;

	isoal_test_create_sdu_fagment(BT_ISO_START,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	payload_number++;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments++;

	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(5);

	ZASSERT_PDU_WRITE_TEST(history[4],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(4);

	/* PDU release not expected (No Error) */

	/* SDU 2 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	ZASSERT_PDU_WRITE_TEST(history[5],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[4],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_START_CONTINUE,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 4 */
	payload_number++;
	sdu_read_loc = pdu_write_size;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc);
	pdu_write_loc = 0;
	sdu_fragments = 1;

	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(6);

	ZASSERT_PDU_WRITE_TEST(history[6],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(5);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX * 2;

	isoal_test_create_sdu_fagment(BT_ISO_END,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 4 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[7],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[5],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* Padding PDUs */
	/* Padding 3 */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	/* PDU should not be written to */
	ZASSERT_PDU_WRITE_TEST_CALL_COUNT(8);

	ZASSERT_PDU_EMIT_TEST(history[6],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_START_CONTINUE,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* Padding 4 */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[2]);
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(8);

	/* PDU should not be written to */
	ZASSERT_PDU_WRITE_TEST_CALL_COUNT(8);

	ZASSERT_PDU_EMIT_TEST(history[7],
			      &tx_pdu_meta_buf[2].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_START_CONTINUE,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is less than the PDU buffer size,
 * followed by padding
 */
ZTEST(test_tx_unframed, test_tx_unframed_1_zero_sdu_1_frag_1_pdu_maxPDU_padding)
{
	struct tx_pdu_meta_buffer tx_pdu_meta_buf[3];
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer[3];
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint8_t testdata[1];
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[2]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, 1);
	pdu_buffer[0].handle = (void *)&tx_pdu_meta_buf[0].node_tx;
	pdu_buffer[0].pdu = (struct pdu_iso *)tx_pdu_meta_buf[0].node_tx.pdu;
	pdu_buffer[0].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[1].handle = (void *)&tx_pdu_meta_buf[1].node_tx;
	pdu_buffer[1].pdu = (struct pdu_iso *)tx_pdu_meta_buf[1].node_tx.pdu;
	pdu_buffer[1].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[2].handle = (void *)&tx_pdu_meta_buf[2].node_tx;
	pdu_buffer[2].pdu = (struct pdu_iso *)tx_pdu_meta_buf[2].node_tx.pdu;
	pdu_buffer[2].size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = 0;
	testdata_indx = 0;
	testdata_size = 0;
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = 0;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 false,             /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[2]);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	sdu_fragments = 1;

	/* PDU should not be written to */
	ZASSERT_PDU_WRITE_TEST_CALL_COUNT(0);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Padding PDUs */
	/* Padding 1 */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	/* PDU should not be written to */
	ZASSERT_PDU_WRITE_TEST_CALL_COUNT(0);

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_START_CONTINUE,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Padding 2 */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[2]);
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(3);

	/* PDU should not be written to */
	ZASSERT_PDU_WRITE_TEST_CALL_COUNT(0);

	ZASSERT_PDU_EMIT_TEST(history[2],
			      &tx_pdu_meta_buf[2].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_START_CONTINUE,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of a single SDU contained in a single fragment,
 * where PDU allocation fails
 */
ZTEST(test_tx_unframed, test_tx_unframed_1_sdu_1_frag_pdu_alloc_err)
{
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint8_t testdata[TEST_TX_PDU_PAYLOAD_MAX - 5];
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, TEST_TX_PDU_PAYLOAD_MAX - 5);
	pdu_buffer.handle = NULL;
	pdu_buffer.pdu = NULL;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	testdata_indx = 0;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = 0;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 false,             /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_ERR_PDU_ALLOC);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	ztest_set_assert_valid(true);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	ztest_set_assert_valid(false);

	zassert_equal(err, ISOAL_STATUS_ERR_PDU_ALLOC, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	sdu_fragments = 1;

	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(1);

	/* PDU should not be written to */
	ZASSERT_PDU_WRITE_TEST_CALL_COUNT(0);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is less than the PDU buffer size,
 * where PDU emit fails
 */
ZTEST(test_tx_unframed, test_tx_unframed_1_sdu_1_frag_pdu_emit_err)
{
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint8_t testdata[TEST_TX_PDU_PAYLOAD_MAX - 5];
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, TEST_TX_PDU_PAYLOAD_MAX - 5);
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	testdata_indx = 0;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX - 5;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 false,             /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_ERR_PDU_EMIT);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_ERR_PDU_EMIT, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	sdu_fragments = 1;

	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(1);

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	ZASSERT_PDU_RELEASE_TEST(
		history[0],
		&tx_pdu_meta_buf.node_tx,
		bt_iso_handle(isoal_global.source_state[source_hdl].session.handle),
		ISOAL_STATUS_ERR_PDU_EMIT);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of a single SDU contained in a single fragment
 * into a single PDU such that it does not insert a skew into the stream.
 */
ZTEST(test_tx_unframed, test_tx_unframed_4_sdu_1_frag_4_pdu_stream_loc)
{
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	uint32_t tx_sync_timestamp_expected;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	uint32_t tx_sync_offset_expected;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint16_t tx_sync_seq_expected;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t tx_sync_timestamp;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t tx_sync_offset;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint8_t testdata[53];
	uint16_t tx_sync_seq;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US / 2;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	/* Sets initial fragmentation status */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, sizeof(testdata));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	event_number = 2000;
	sdu_packet_number = (event_number * BN);
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = 23;
	testdata_indx = 0;
	testdata_size = 23;
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = 23;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 false,             /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Check TX Sync info */
	tx_sync_seq_expected = 1;
	tx_sync_timestamp_expected = ref_point;
	tx_sync_offset_expected = 0;

	err = isoal_tx_get_sync_info(source_hdl, &tx_sync_seq, &tx_sync_timestamp, &tx_sync_offset);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
	zassert_equal(tx_sync_seq, tx_sync_seq_expected, "%u != %u", tx_sync_seq, 2);
	zassert_equal(tx_sync_timestamp, tx_sync_timestamp_expected, "%u != %u", tx_sync_seq, 2);
	zassert_equal(tx_sync_offset, tx_sync_offset_expected, "%u != %u", tx_sync_seq, 0);

	/* SDU 2 Frag 1 ------------------------------------------------------*/
	/* Check correct position in stream based on the SDU packet number */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_packet_number += 29;
	sdu_timestamp += ((iso_interval_int * ISO_INT_UNIT_US) * 15) - sdu_interval;
	event_number += 15;
	ref_point += (iso_interval_int * ISO_INT_UNIT_US) * 15;
	sdu_total_size = 10;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_fragments = 0;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	payload_number += 29;
	pdu_write_loc = 0;
	sdu_read_loc = testdata_indx;
	pdu_write_size = (testdata_size - testdata_indx);
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Check TX Sync info */
	tx_sync_seq_expected += 29;
	tx_sync_timestamp_expected = ref_point - (iso_interval_int * ISO_INT_UNIT_US);
	tx_sync_offset_expected = 0;

	err = isoal_tx_get_sync_info(source_hdl, &tx_sync_seq, &tx_sync_timestamp, &tx_sync_offset);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
	zassert_equal(tx_sync_seq, tx_sync_seq_expected, "%u != %u", tx_sync_seq, 2);
	zassert_equal(tx_sync_timestamp, tx_sync_timestamp_expected, "%u != %u", tx_sync_seq, 2);
	zassert_equal(tx_sync_offset, tx_sync_offset_expected, "%u != %u", tx_sync_seq, 0);

	/* SDU 3 Frag 1 ------------------------------------------------------*/
	/* Check correct position in stream based on the SDU time stamp */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	/* Same SDU packet sequence number for testing */
	/* Time stamp just before the exact multiple of the SDU interval */
	sdu_timestamp += ((iso_interval_int * ISO_INT_UNIT_US) * 15) - 1;
	event_number += 15;
	ref_point += (iso_interval_int * ISO_INT_UNIT_US) * 15;
	sdu_total_size = 10;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_fragments = 0;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	payload_number += 30;
	pdu_write_loc = 0;
	sdu_read_loc = testdata_indx;
	pdu_write_size = (testdata_size - testdata_indx);
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[2],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Check TX Sync info */
	tx_sync_seq_expected += 30;
	tx_sync_timestamp_expected = ref_point - (iso_interval_int * ISO_INT_UNIT_US);
	tx_sync_offset_expected = 0;

	err = isoal_tx_get_sync_info(source_hdl, &tx_sync_seq, &tx_sync_timestamp, &tx_sync_offset);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
	zassert_equal(tx_sync_seq, tx_sync_seq_expected, "%u != %u", tx_sync_seq, 2);
	zassert_equal(tx_sync_timestamp, tx_sync_timestamp_expected, "%u != %u", tx_sync_seq, 2);
	zassert_equal(tx_sync_offset, tx_sync_offset_expected, "%u != %u", tx_sync_seq, 0);

	/* SDU 4 Frag 1 ------------------------------------------------------*/
	/* Check correct position in stream based on the SDU time stamp */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	/* Same SDU packet sequence number for testing */
	/* Time stamp just after the exact multiple of the SDU interval.
	 * +1 (reset to exact multiple of SDU interval from the last SDU)
	 * +1 (push the time stamp 1us beyond the multiple mark)
	 */
	sdu_timestamp += ((iso_interval_int * ISO_INT_UNIT_US) * 15) + 1 + 1;
	event_number += 15;
	ref_point += (iso_interval_int * ISO_INT_UNIT_US) * 15;
	sdu_total_size = 10;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_fragments = 0;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	payload_number += 30;
	pdu_write_loc = 0;
	sdu_read_loc = testdata_indx;
	pdu_write_size = (testdata_size - testdata_indx);
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	ZASSERT_PDU_EMIT_TEST(history[3],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_COMPLETE_END,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Check TX Sync info */
	tx_sync_seq_expected += 30;
	tx_sync_timestamp_expected = ref_point - (iso_interval_int * ISO_INT_UNIT_US);
	tx_sync_offset_expected = 0;

	err = isoal_tx_get_sync_info(source_hdl, &tx_sync_seq, &tx_sync_timestamp, &tx_sync_offset);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
	zassert_equal(tx_sync_seq, tx_sync_seq_expected, "%u != %u", tx_sync_seq, 2);
	zassert_equal(tx_sync_timestamp, tx_sync_timestamp_expected, "%u != %u", tx_sync_seq, 2);
	zassert_equal(tx_sync_offset, tx_sync_offset_expected, "%u != %u", tx_sync_seq, 0);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests framed event selection
 */
#define RUN_TX_FRAMED_FIND_CORRECT_TX_EVENT()                                  \
	out_sdus_skipped = isoal_tx_framed_find_correct_tx_event(source,   \
								 &tx_sdu_frag_buf.sdu_tx,  \
								 &out_payload_number,  \
								 &out_ref_point,  \
								 &out_time_offset);  \
										   \
	zassert_equal(out_payload_number, expect_payload_number, "%llu != %llu",  \
			out_payload_number, expect_payload_number);  \
	zassert_equal(out_ref_point, expect_ref_point, "%u != %u",  \
			out_ref_point, expect_ref_point);  \
	zassert_equal(out_time_offset, expect_time_offset, "%u != %u",  \
			out_time_offset, expect_time_offset);  \
	zassert_equal(out_sdus_skipped, expect_sdus_skipped, "%u .!= %u",  \
			out_sdus_skipped, expect_sdus_skipped)

ZTEST(test_tx_framed, test_tx_framed_find_correct_tx_event)
{
	const uint8_t number_of_pdus = 1;
	const uint8_t testdata_size_max = MAX_FRAMED_PDU_PAYLOAD(number_of_pdus);

	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_source_session *session;
	uint8_t testdata[testdata_size_max];
	isoal_sdu_len_t in_sdu_total_size;
	isoal_source_handle_t source_hdl;
	struct isoal_pdu_production *pp;
	uint64_t expect_payload_number;
	struct isoal_source *source;
	uint64_t out_payload_number;
	uint32_t expect_time_offset;
	uint8_t expect_sdus_skipped;
	uint32_t expected_timestamp;
	uint32_t stream_sync_delay;
	uint32_t in_cntr_timestamp;
	uint32_t group_sync_delay;
	uint64_t in_sdu_packet_sn;
	uint32_t in_sdu_timestamp;
	uint32_t expect_ref_point;
	uint64_t in_target_event;
	uint32_t iso_interval_us;
	uint8_t iso_interval_int;
	uint32_t out_time_offset;
	uint8_t out_sdus_skipped;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t out_ref_point;
	uint32_t sdu_interval;
	uint32_t in_ref_point;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = iso_interval_us + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	init_test_data_buffer(testdata, testdata_size_max);

	/* Create source */
	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	source      = &isoal_global.source_state[source_hdl];
	session     = &source->session;
	pp          = &source->pdu_production;

	in_sdu_total_size = testdata_size_max;
	testdata_indx = 0;
	testdata_size = testdata_size_max;

	/* Test    : Selection of event for first SDU where
	 *           -- Last SDU packet number is uninitialized
	 *           -- Last SDU time stamp is uninitialized
	 *           -- Payload number is uninitialized
	 *           -- Target event and reference point are one event ahead
	 *           -- Time stamp is valid
	 *           -- Time stamp indicates that target event is feasible
	 * Expected:
	 * -- Target event is used for transmission and calculations are based
	 *    on that
	 * -- Time offset is based on the SDUs time stamp
	 */
	in_sdu_packet_sn = 2000;
	in_target_event = 2000;
	in_sdu_timestamp = 9249;
	in_cntr_timestamp = in_sdu_timestamp + 200;
	in_ref_point = in_sdu_timestamp + iso_interval_us - 50;

	pp->initialized = 0U;
	session->tx_time_stamp = 0;
	session->tx_time_offset = 0;
	session->last_input_sn = 0;
	session->last_input_time_stamp = 0;
	pp->payload_number = 0;

	expect_sdus_skipped = 0;
	expect_payload_number = in_target_event * BN;
	expect_ref_point = in_ref_point;
	expected_timestamp = in_sdu_timestamp;
	expect_time_offset = expect_ref_point - expected_timestamp;

	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      in_sdu_total_size,
				      in_sdu_packet_sn,
				      in_sdu_timestamp,
				      in_cntr_timestamp,
				      in_ref_point,
				      in_target_event,
				      &tx_sdu_frag_buf.sdu_tx);

	RUN_TX_FRAMED_FIND_CORRECT_TX_EVENT();

	/* Test    : Selection of event for first SDU where
	 *           -- Last SDU packet number is uninitialized
	 *           -- Last SDU time stamp is uninitialized
	 *           -- Payload number ahead of target event
	 *           -- Target event and reference point are one event behind
	 *              current payload
	 *           -- Time stamp is valid
	 *           -- Time stamp indicates that target event is feasible
	 * Expected:
	 * -- Target event + 1 is selected based on the payload being ahead and
	 *    calculations are based on that reference
	 * -- Time offset is based on the SDUs time stamp
	 */
	in_sdu_packet_sn = 2000;
	in_target_event = 2000;
	in_sdu_timestamp = 9249;
	in_cntr_timestamp = in_sdu_timestamp + 200;
	in_ref_point = in_sdu_timestamp + iso_interval_us - 50;

	pp->initialized = 0U;
	session->tx_time_stamp = 0;
	session->tx_time_offset = 0;
	session->last_input_sn = 0;
	session->last_input_time_stamp = 0;
	pp->payload_number = (in_target_event + 1) * BN;

	expect_sdus_skipped = 0;
	expect_payload_number = (in_target_event + 1) * BN;
	expect_ref_point = in_ref_point + iso_interval_us;
	expected_timestamp = in_sdu_timestamp;
	expect_time_offset = expect_ref_point - expected_timestamp;

	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      in_sdu_total_size,
				      in_sdu_packet_sn,
				      in_sdu_timestamp,
				      in_cntr_timestamp,
				      in_ref_point,
				      in_target_event,
				      &tx_sdu_frag_buf.sdu_tx);

	RUN_TX_FRAMED_FIND_CORRECT_TX_EVENT();

	/* Test    : Selection of event for first SDU where
	 *           -- Last SDU packet number is uninitialized
	 *           -- Last SDU time stamp is uninitialized
	 *           -- Payload number ahead of target event
	 *           -- Target event and reference point are one event behind
	 *              current payload
	 *           -- Time stamp is invalid
	 *           -- Controller time stamp indicates that target event is
	 *              feasible
	 * Expected:
	 * -- Target event + 1 is selected based on the payload being ahead and
	 *    calculations are based on that reference
	 * -- Time offset is based on the controller's capture time
	 */
	in_sdu_packet_sn = 2000;
	in_target_event = 2000;
	in_sdu_timestamp = 0;
	in_cntr_timestamp = 9249 + 200;
	in_ref_point = in_cntr_timestamp + iso_interval_us - 50;

	pp->initialized = 0U;
	session->tx_time_stamp = 0;
	session->tx_time_offset = 0;
	session->last_input_sn = 0;
	session->last_input_time_stamp = 0;
	pp->payload_number = (in_target_event + 1) * BN;

	expect_sdus_skipped = 0;
	expect_payload_number = (in_target_event + 1) * BN;
	expect_ref_point = in_ref_point + iso_interval_us;
	expected_timestamp = in_cntr_timestamp;
	expect_time_offset = expect_ref_point - expected_timestamp;

	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      in_sdu_total_size,
				      in_sdu_packet_sn,
				      in_sdu_timestamp,
				      in_cntr_timestamp,
				      in_ref_point,
				      in_target_event,
				      &tx_sdu_frag_buf.sdu_tx);

	RUN_TX_FRAMED_FIND_CORRECT_TX_EVENT();

	/* Test    : Selection of event for a subsequent SDU where
	 *           -- Last SDU packet number is in sequence
	 *           -- Last SDU time stamp is in sequence
	 *           -- Payload number is in sequence
	 *           -- Target event and reference point are one event ahead of
	 *              current payload
	 *           -- Time stamp is valid
	 *           -- Time stamp indicates that target event is feasible
	 * Expected:
	 * -- Target event is selected based on the time stamp and calculations
	 *    are based on that reference
	 * -- Time offset is based on the SDUs time stamp
	 */
	in_sdu_packet_sn = 2000;
	in_target_event = 2000;
	in_sdu_timestamp = 9249;
	in_cntr_timestamp = 9249 + 200;
	in_ref_point = in_sdu_timestamp + iso_interval_us - 50;

	pp->initialized = 1U;
	session->tx_time_stamp = 0;
	session->tx_time_offset = 0;
	session->last_input_sn = in_sdu_packet_sn - 1;
	session->last_input_time_stamp = in_sdu_timestamp - sdu_interval;
	pp->payload_number = (in_target_event - 1) * BN;

	expect_sdus_skipped = 0;
	expect_payload_number = in_target_event * BN;
	expect_ref_point = in_ref_point;
	expected_timestamp = in_sdu_timestamp;
	expect_time_offset = expect_ref_point - expected_timestamp;

	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      in_sdu_total_size,
				      in_sdu_packet_sn,
				      in_sdu_timestamp,
				      in_cntr_timestamp,
				      in_ref_point,
				      in_target_event,
				      &tx_sdu_frag_buf.sdu_tx);

	RUN_TX_FRAMED_FIND_CORRECT_TX_EVENT();

	/* Test    : Selection of event for a subsequent SDU where
	 *           -- Last SDU packet number is not in sequence
	 *           -- Last SDU time stamp is not in sequence
	 *           -- Payload number is not in sequence
	 *           -- Target event and reference point are two events ahead
	 *           -- Time stamp is valid but at the border of the range
	 *           -- Time stamp indicates that target event - 1 is feasible
	 * Expected:
	 * -- Target event - 1 is selected based on the time stamp and
	 *    calculations are based on that reference
	 * -- Time offset is based on the SDUs time stamp
	 */
	in_sdu_packet_sn = 2000;
	in_target_event = 2001;
	in_sdu_timestamp = 9249;
	in_cntr_timestamp = 9249 + sdu_interval + iso_interval_us;
	in_ref_point = in_sdu_timestamp + (iso_interval_us * 2) - 50;

	pp->initialized = 1U;
	session->tx_time_stamp = 0;
	session->tx_time_offset = 0;
	session->last_input_sn = in_sdu_packet_sn - 3;
	session->last_input_time_stamp = in_sdu_timestamp - (sdu_interval * 2);
	pp->payload_number = (in_target_event - 2) * BN;

	expect_sdus_skipped = in_sdu_packet_sn - session->last_input_sn - 1;
	expect_payload_number = (in_target_event - 1) * BN;
	expect_ref_point = in_ref_point - iso_interval_us;
	expected_timestamp = in_sdu_timestamp;
	expect_time_offset = expect_ref_point - expected_timestamp;

	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      in_sdu_total_size,
				      in_sdu_packet_sn,
				      in_sdu_timestamp,
				      in_cntr_timestamp,
				      in_ref_point,
				      in_target_event,
				      &tx_sdu_frag_buf.sdu_tx);

	RUN_TX_FRAMED_FIND_CORRECT_TX_EVENT();

	/* Test    : Selection of event for a subsequent SDU where
	 *           -- Last SDU packet number is not in sequence
	 *           -- Last SDU time stamp is not in sequence
	 *           -- Payload number is not in sequence
	 *           -- Target event and reference point are two events ahead
	 *           -- Time stamp is invalid
	 * Expected:
	 * -- Target event is selected based on the time stamp calculated
	 *    from the difference between time stamps and calculations are based
	 *    on that reference
	 * -- Time offset is based on the SDUs time stamp
	 */
	in_sdu_packet_sn = 2000;
	in_target_event = 2001;
	in_sdu_timestamp = 9249;
	in_cntr_timestamp = 9249 + sdu_interval + iso_interval_us + 1;
	in_ref_point = in_sdu_timestamp + (iso_interval_us * 2) - 50;

	pp->initialized = 1U;
	session->tx_time_stamp = in_ref_point - iso_interval_us;
	session->tx_time_offset = session->tx_time_stamp -
					(in_sdu_timestamp - sdu_interval);
	session->last_input_sn = in_sdu_packet_sn - 3;
	session->last_input_time_stamp = in_sdu_timestamp - (sdu_interval * 2);
	pp->payload_number = (in_target_event - 2) * BN;

	expect_sdus_skipped = in_sdu_packet_sn - session->last_input_sn - 1;
	expect_payload_number = in_target_event * BN;
	expect_ref_point = in_ref_point;
	expected_timestamp = session->tx_time_stamp - session->tx_time_offset +
				(in_sdu_timestamp - session->last_input_time_stamp);
	expect_time_offset = expect_ref_point - expected_timestamp;

	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      in_sdu_total_size,
				      in_sdu_packet_sn,
				      in_sdu_timestamp,
				      in_cntr_timestamp,
				      in_ref_point,
				      in_target_event,
				      &tx_sdu_frag_buf.sdu_tx);

	RUN_TX_FRAMED_FIND_CORRECT_TX_EVENT();

	/* Test    : Selection of event for a subsequent SDU where
	 *           -- Last SDU packet number is not in sequence
	 *           -- Last SDU time stamp has been projected as part of a
	 *              burst
	 *           -- Payload number is not in sequence
	 *           -- Target event and reference point are two events ahead
	 *           -- Time stamp is invalid
	 *           -- Time stamp delta is invalid
	 * Expected:
	 * -- Target event + 1 is selected based on the time stamp calculated
	 *    from the difference in packet sn and calculations are based
	 *    on that reference
	 * -- Time offset is based on the SDUs time stamp
	 */
	in_sdu_packet_sn = 2000;
	in_target_event = 2001;
	in_sdu_timestamp = 9249;
	in_cntr_timestamp = 9249 + sdu_interval + iso_interval_us + 1;
	in_ref_point = in_sdu_timestamp + (iso_interval_us * 2) - 50;

	pp->initialized = 1U;
	session->tx_time_stamp = in_ref_point - iso_interval_us;
	session->tx_time_offset = session->tx_time_stamp -
					(in_sdu_timestamp + sdu_interval);
	session->last_input_sn = in_sdu_packet_sn - 1;
	session->last_input_time_stamp = in_sdu_timestamp + (sdu_interval * 2);
	pp->payload_number = (in_target_event - 2) * BN;

	expect_sdus_skipped = in_sdu_packet_sn - session->last_input_sn - 1;
	expect_payload_number = (in_target_event + 1) * BN;
	expect_ref_point = in_ref_point + iso_interval_us;
	expected_timestamp = session->tx_time_stamp - session->tx_time_offset + sdu_interval;
	expect_time_offset = expect_ref_point - expected_timestamp;

	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      in_sdu_total_size,
				      in_sdu_packet_sn,
				      in_sdu_timestamp,
				      in_cntr_timestamp,
				      in_ref_point,
				      in_target_event,
				      &tx_sdu_frag_buf.sdu_tx);

	RUN_TX_FRAMED_FIND_CORRECT_TX_EVENT();

	/* Test    : Selection of event for a subsequent SDU where
	 *           -- Last SDU packet number is in sequence
	 *           -- Last SDU time stamp has been projected as part of a
	 *              burst
	 *           -- Payload number is ahead of selected event
	 *           -- Target event and reference point are two events ahead
	 *           -- Time stamp is valid
	 *           -- Time stamp indicates that target event - 1 is feasible
	 * Expected:
	 * -- Target event -1 is selected based on the time stamp and
	 *    calculations are based on that reference
	 * -- Payload number continues from last
	 * -- Time offset is based on the SDUs time stamp
	 */
	in_sdu_packet_sn = 2000;
	in_target_event = 2001;
	in_sdu_timestamp = 9249;
	in_cntr_timestamp = 9249;
	in_ref_point = in_sdu_timestamp + (iso_interval_us * 2) - 50;

	pp->initialized = 1U;
	session->tx_time_stamp = 0;
	session->tx_time_offset = 0;
	session->last_input_sn = in_sdu_packet_sn - 1;
	session->last_input_time_stamp = in_sdu_timestamp - sdu_interval;
	pp->payload_number = ((in_target_event - 1) * BN) + 1;

	expect_sdus_skipped = in_sdu_packet_sn - session->last_input_sn - 1;
	expect_payload_number = pp->payload_number;
	expect_ref_point = in_ref_point - iso_interval_us;
	expected_timestamp = in_sdu_timestamp;
	expect_time_offset = expect_ref_point - expected_timestamp;

	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      in_sdu_total_size,
				      in_sdu_packet_sn,
				      in_sdu_timestamp,
				      in_cntr_timestamp,
				      in_ref_point,
				      in_target_event,
				      &tx_sdu_frag_buf.sdu_tx);

	RUN_TX_FRAMED_FIND_CORRECT_TX_EVENT();
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is less than the PDU buffer size
 */
ZTEST(test_tx_framed, test_tx_framed_1_sdu_1_frag_1_pdu_maxPDU)
{
	uint8_t testdata[TEST_TX_PDU_PAYLOAD_MAX - 5 -
			 (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	struct pdu_iso_sdu_sh seg_hdr[2];
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      TEST_TX_PDU_PAYLOAD_MAX - 5 -
				      (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	(void)memset(&seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Test PDU release */
	isoal_tx_pdu_release(source_hdl, &tx_pdu_meta_buf.node_tx);

	ZASSERT_PDU_RELEASE_TEST(history[0],
				 &tx_pdu_meta_buf.node_tx,
				 isoal_global.source_state[source_hdl].session.handle,
				 ISOAL_STATUS_OK);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is greater than the PDU buffer size
 */
ZTEST(test_tx_framed, test_tx_framed_1_sdu_1_frag_1_pdu_bufSize)
{
	uint8_t testdata[TEST_TX_PDU_PAYLOAD_MAX -
			 (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	struct pdu_iso_sdu_sh seg_hdr[2];
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      TEST_TX_PDU_PAYLOAD_MAX -
				      (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	(void)memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment
 * into three PDUs where Max PDU is less than the PDU buffer size. Also tests
 * endianness of the segment header.
 */
ZTEST(test_tx_framed, test_tx_framed_1_sdu_1_frag_3_pdu)
{
	uint8_t testdata[100 - ((3 * PDU_ISO_SEG_HDR_SIZE) + PDU_ISO_SEG_TIMEOFFSET_SIZE)];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	struct pdu_iso_sdu_sh seg_hdr[2 * 3];
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      100 - ((3 * PDU_ISO_SEG_HDR_SIZE) + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	(void)memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = 100 - ((3 * PDU_ISO_SEG_HDR_SIZE) + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size = 100 - ((3 * PDU_ISO_SEG_HDR_SIZE) + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	/* Test endianness */
	WRITE_BIT(((uint8_t *)&seg_hdr[0])[0], 0, 0); /* sc */
	WRITE_BIT(((uint8_t *)&seg_hdr[0])[0], 1, 0); /* cmplt */
	sys_put_le24(ref_point - sdu_timestamp, (uint8_t *)(&seg_hdr[0]) + PDU_ISO_SEG_HDR_SIZE);
	((uint8_t *)(&seg_hdr[0]))[1] = PDU_ISO_SEG_TIMEOFFSET_SIZE; /* len */
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = max_octets;
	sdu_fragments = 0;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	((uint8_t *)(&seg_hdr[1]))[1] += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 2 */
	payload_number++;
	WRITE_BIT(((uint8_t *)&seg_hdr[2])[0], 0, 1); /* sc */
	WRITE_BIT(((uint8_t *)&seg_hdr[2])[0], 1, 0); /* cmplt */
	sys_put_le24(0, (uint8_t *)(&seg_hdr[2]) + PDU_ISO_SEG_HDR_SIZE);
	((uint8_t *)(&seg_hdr[2]))[1] = 0; /* len */
	pdu_hdr_loc = 0;
	sdu_read_loc += (pdu_write_size - pdu_write_loc);
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE;
	pdu_write_size = max_octets;
	sdu_fragments = 0;

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[2],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_WRITE_TEST(history[4],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[3] = seg_hdr[2];
	((uint8_t *)(&seg_hdr[3]))[1] += (pdu_write_size - pdu_write_loc); /* len */

	ZASSERT_PDU_WRITE_TEST(history[5],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[3],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 3 */
	payload_number++;
	WRITE_BIT(((uint8_t *)&seg_hdr[4])[0], 0, 1); /* sc */
	WRITE_BIT(((uint8_t *)&seg_hdr[4])[0], 1, 0); /* cmplt */
	sys_put_le24(0, (uint8_t *)(&seg_hdr[4]) + PDU_ISO_SEG_HDR_SIZE);
	((uint8_t *)(&seg_hdr[4]))[1] = 0; /* len */
	pdu_hdr_loc = 0;
	sdu_read_loc += (pdu_write_size - pdu_write_loc);
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE;
	pdu_write_size =
		sdu_total_size -
		((2 * max_octets) - (2 * PDU_ISO_SEG_HDR_SIZE) - PDU_ISO_SEG_TIMEOFFSET_SIZE) +
		pdu_write_loc;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[6],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[4],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_WRITE_TEST(history[7],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[5] = seg_hdr[4];
	WRITE_BIT(((uint8_t *)&seg_hdr[5])[0], 1, 1); /* cmplt */
	((uint8_t *)(&seg_hdr[5]))[1] += (pdu_write_size - pdu_write_loc); /* len */

	ZASSERT_PDU_WRITE_TEST(history[8],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[5],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[2],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in three fragments
 * into a single PDU where Max PDU is greater than the PDU buffer size
 */
ZTEST(test_tx_framed, test_tx_framed_1_sdu_3_frag_1_pdu)
{
	uint8_t testdata[TEST_TX_PDU_PAYLOAD_MAX -
			 (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	struct pdu_iso_sdu_sh seg_hdr[2];
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      TEST_TX_PDU_PAYLOAD_MAX -
				      (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	(void)memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		(TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
		3;
	payload_number = event_number * BN;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size =
		((TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
		 3) +
		pdu_write_loc;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU Frag 2 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size +=
		(TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
		3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size +=
		((TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
		 3);
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */
	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(1);

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[4],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU Frag 3 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);

	isoal_test_create_sdu_fagment(BT_ISO_END,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */
	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(1);

	ZASSERT_PDU_WRITE_TEST(history[5],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[6],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in three fragments
 * into two PDUs where Max PDU is greater than the PDU buffer size
 */
ZTEST(test_tx_framed, test_tx_framed_1_sdu_3_frag_2_pdu)
{
	uint8_t testdata[(TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf[2];
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer[2];
	struct pdu_iso_sdu_sh seg_hdr[2 * 2];
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      (TEST_TX_PDU_PAYLOAD_MAX * 2) -
				      ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	(void)memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer[0].handle = (void *)&tx_pdu_meta_buf[0].node_tx;
	pdu_buffer[0].pdu = (struct pdu_iso *)tx_pdu_meta_buf[0].node_tx.pdu;
	pdu_buffer[0].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[1].handle = (void *)&tx_pdu_meta_buf[1].node_tx;
	pdu_buffer[1].pdu = (struct pdu_iso *)tx_pdu_meta_buf[1].node_tx.pdu;
	pdu_buffer[1].size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size = ((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			3;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	payload_number = event_number * BN;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = (((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			   ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			  3) +
			 pdu_write_loc;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU Frag 2 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += ((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			  ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	/* PDU should not be allocated */

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[4],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 2 */
	payload_number++;
	seg_hdr[2].sc = 1;
	seg_hdr[2].cmplt = 0;
	seg_hdr[2].timeoffset = 0;
	seg_hdr[2].len = 0;
	sdu_read_loc = (pdu_write_size - pdu_write_loc) + testdata_indx;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc) +
			 PDU_ISO_SEG_HDR_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[5],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[2],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_WRITE_TEST(history[6],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[3] = seg_hdr[2];
	seg_hdr[3].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[7],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[3],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(1);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU Frag 3 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) -
			((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE);

	isoal_test_create_sdu_fagment(BT_ISO_END,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */
	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(2);

	ZASSERT_PDU_WRITE_TEST(history[8],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[3].cmplt = 1;
	seg_hdr[3].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[9],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[3],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of two SDUs containing three fragments each
 * into two PDUs each where Max PDU is greater than the PDU buffer size
 */
ZTEST(test_tx_framed, test_tx_framed_2_sdu_3_frag_4_pdu)
{
	uint8_t testdata[(TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf[2];
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer[2];
	struct pdu_iso_sdu_sh seg_hdr[2 * 2];
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 2;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 4;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      (TEST_TX_PDU_PAYLOAD_MAX * 2) -
				      ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	(void)memset(&seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer[0].handle = (void *)&tx_pdu_meta_buf[0].node_tx;
	pdu_buffer[0].pdu = (struct pdu_iso *)tx_pdu_meta_buf[0].node_tx.pdu;
	pdu_buffer[0].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[1].handle = (void *)&tx_pdu_meta_buf[1].node_tx;
	pdu_buffer[1].pdu = (struct pdu_iso *)tx_pdu_meta_buf[1].node_tx.pdu;
	pdu_buffer[1].size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = 9249 + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size = ((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			3;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	payload_number = event_number * BN;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = (((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			   ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			  3) +
			 pdu_write_loc;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 1 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += ((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			  ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be allocated */

	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[4],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 2 */
	payload_number++;
	seg_hdr[2].sc = 1;
	seg_hdr[2].cmplt = 0;
	seg_hdr[2].timeoffset = 0;
	seg_hdr[2].len = 0;
	sdu_read_loc = (pdu_write_size - pdu_write_loc) + testdata_indx;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc) +
			 PDU_ISO_SEG_HDR_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[5],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[2],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_WRITE_TEST(history[6],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[3] = seg_hdr[2];
	seg_hdr[3].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[7],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[3],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(1);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 1 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) -
			((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE);

	isoal_test_create_sdu_fagment(BT_ISO_END,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */

	ZASSERT_PDU_WRITE_TEST(history[8],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[3].cmplt = 1;
	seg_hdr[3].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[9],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[3],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_packet_number++;
	event_number = 2000;
	sdu_timestamp = 9249 + sdu_interval;
	ref_point = 9249 + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size = ((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			3;
	sdu_fragments = 0;

	isoal_test_create_sdu_fagment(BT_ISO_START,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	payload_number++;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = (((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			   ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			  3) +
			 pdu_write_loc;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[10],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[11],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[12],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(2);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += ((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			  ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	/* PDU should not be allocated */

	ZASSERT_PDU_WRITE_TEST(history[13],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[14],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[2],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 4 */
	payload_number++;
	seg_hdr[2].sc = 1;
	seg_hdr[2].cmplt = 0;
	seg_hdr[2].timeoffset = 0;
	seg_hdr[2].len = 0;
	sdu_read_loc = (pdu_write_size - pdu_write_loc) + testdata_indx;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc) +
			 PDU_ISO_SEG_HDR_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[15],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[2],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_WRITE_TEST(history[16],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[3] = seg_hdr[2];
	seg_hdr[3].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[17],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[3],
			       PDU_ISO_SEG_HDR_SIZE);
	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(3);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) -
			((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE);

	isoal_test_create_sdu_fagment(BT_ISO_END,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 4 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */
	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(4);

	ZASSERT_PDU_WRITE_TEST(history[18],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[3].cmplt = 1;
	seg_hdr[3].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[19],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[3],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[3],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of two SDUs containing three fragments each
 * into two PDUs each where Max PDU is greater than the PDU buffer size
 * with padding
 */
ZTEST(test_tx_framed, test_tx_framed_2_sdu_3_frag_4_pdu_padding)
{
	const uint8_t number_of_pdus = 2;
	const uint8_t number_of_sdu_frags = 3;
	const uint8_t testdata_size_max = MAX_FRAMED_PDU_PAYLOAD(number_of_pdus);
	const uint8_t number_of_seg_hdr_buf = EXPECTED_SEG_HDR_WRITES * number_of_pdus;

	struct tx_pdu_meta_buffer tx_pdu_meta_buf[number_of_pdus];
	struct pdu_iso_sdu_sh seg_hdr[number_of_seg_hdr_buf];
	struct isoal_pdu_buffer pdu_buffer[number_of_pdus];
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	uint8_t testdata[testdata_size_max];
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 2;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 6;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, testdata_size_max);
	(void)memset(&seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer[0].handle = (void *)&tx_pdu_meta_buf[0].node_tx;
	pdu_buffer[0].pdu = (struct pdu_iso *)tx_pdu_meta_buf[0].node_tx.pdu;
	pdu_buffer[0].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[1].handle = (void *)&tx_pdu_meta_buf[1].node_tx;
	pdu_buffer[1].pdu = (struct pdu_iso *)tx_pdu_meta_buf[1].node_tx.pdu;
	pdu_buffer[1].size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = 9249 + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = testdata_size_max;
	testdata_indx = 0;
	testdata_size = testdata_size_max / number_of_sdu_frags;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	payload_number = event_number * BN;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = (testdata_size_max / number_of_sdu_frags) + pdu_write_loc;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 1 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += testdata_size_max / number_of_sdu_frags;

	isoal_test_create_sdu_fagment(BT_ISO_CONT,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	/* PDU should not be allocated */

	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[4],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 2 */
	payload_number++;
	seg_hdr[2].sc = 1;
	seg_hdr[2].cmplt = 0;
	seg_hdr[2].timeoffset = 0;
	seg_hdr[2].len = 0;
	sdu_read_loc = (pdu_write_size - pdu_write_loc) + testdata_indx;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc) +
			 PDU_ISO_SEG_HDR_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[5],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[2],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_WRITE_TEST(history[6],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[3] = seg_hdr[2];
	seg_hdr[3].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[7],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[3],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(1);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 1 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = testdata_size_max;

	isoal_test_create_sdu_fagment(BT_ISO_END,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */

	ZASSERT_PDU_WRITE_TEST(history[8],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[3].cmplt = 1;
	seg_hdr[3].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[9],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[3],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_packet_number++;
	event_number = 2000;
	sdu_timestamp = 9249 + sdu_interval;
	ref_point = 9249 + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = testdata_size_max;
	testdata_indx = 0;
	testdata_size = testdata_size_max / number_of_sdu_frags;
	sdu_fragments = 0;

	isoal_test_create_sdu_fagment(BT_ISO_START,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	payload_number++;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = (testdata_size_max / number_of_sdu_frags) + pdu_write_loc;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[10],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[11],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[12],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(2);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += testdata_size_max / number_of_sdu_frags;

	isoal_test_create_sdu_fagment(BT_ISO_CONT,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	/* PDU should not be allocated */

	ZASSERT_PDU_WRITE_TEST(history[13],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[14],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[2],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 4 */
	payload_number++;
	seg_hdr[2].sc = 1;
	seg_hdr[2].cmplt = 0;
	seg_hdr[2].timeoffset = 0;
	seg_hdr[2].len = 0;
	sdu_read_loc = (pdu_write_size - pdu_write_loc) + testdata_indx;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc) +
			 PDU_ISO_SEG_HDR_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[15],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[2],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_WRITE_TEST(history[16],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[3] = seg_hdr[2];
	seg_hdr[3].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[17],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[3],
			       PDU_ISO_SEG_HDR_SIZE);
	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(3);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = testdata_size_max;

	isoal_test_create_sdu_fagment(BT_ISO_END,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 4 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */
	ZASSERT_PDU_ALLOC_TEST_CALL_COUNT(4);

	ZASSERT_PDU_WRITE_TEST(history[18],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[3].cmplt = 1;
	seg_hdr[3].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[19],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[3],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[3],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Send Event Timeout ----------------------------------------------- */
	isoal_tx_event_prepare(source_hdl, event_number);

	/* PDU 5 (Padding) */
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	/* PDU should not be written to */
	ZASSERT_PDU_WRITE_TEST_CALL_COUNT(20);

	ZASSERT_PDU_EMIT_TEST(history[4],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 6 (Padding) */
	payload_number++;
	sdu_fragments = 0;

	/* PDU should not be written to */
	ZASSERT_PDU_WRITE_TEST_CALL_COUNT(20);

	ZASSERT_PDU_EMIT_TEST(history[5],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is greater than the PDU buffer size,
 * where the reference point has to be advanced due to the payload number not
 * matching the actual target event
 */
ZTEST(test_tx_framed, test_tx_framed_2_sdu_1_frag_2_pdu_refPoint2)
{
	uint8_t testdata[(TEST_TX_PDU_PAYLOAD_MAX -
			  (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) *
			 2];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	struct pdu_iso_sdu_sh seg_hdr[2];
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(
		testdata,
		(TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) *
			2);
	(void)memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	memset(seg_hdr, 0, sizeof(seg_hdr));
	sdu_packet_number++;
	sdu_timestamp = 9249 + sdu_interval;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	/* Advance the target event and the reference point to what it should be */
	event_number++;
	ref_point += iso_interval_int * ISO_INT_UNIT_US;
	payload_number++;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[4],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[5],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is greater than the PDU buffer size,
 * where the reference point has to be advanced as it is earlier than the
 * time stamp
 */
ZTEST(test_tx_framed, test_tx_framed_1_sdu_1_frag_1_pdu_refPoint3)
{
	uint8_t testdata[(TEST_TX_PDU_PAYLOAD_MAX -
			  (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) *
			 2];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	struct pdu_iso_sdu_sh seg_hdr[2];
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(
		testdata,
		(TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) *
			2);
	(void)memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	/* Advance the target event and the reference point to what it should be */
	event_number++;
	ref_point += iso_interval_int * ISO_INT_UNIT_US;
	payload_number = event_number * BN;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is greater than the PDU buffer size,
 * where the reference point has to be advanced due to the payload number not
 * matching the actual target event with a focus on the wrapping point of the
 * controller's clock
 */
ZTEST(test_tx_framed, test_tx_framed_2_sdu_1_frag_2_pdu_ts_wrap1)
{
	uint8_t testdata[(TEST_TX_PDU_PAYLOAD_MAX -
			  (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) *
			 2];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	struct pdu_iso_sdu_sh seg_hdr[2];
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(
		testdata,
		(TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) *
			2);
	(void)memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = ISOAL_TIME_WRAPPING_POINT_US;
	ref_point = 100;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = 101;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	memset(seg_hdr, 0, sizeof(seg_hdr));
	sdu_packet_number++;
	sdu_timestamp = sdu_interval - 1;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	/* Advance the target event and the reference point to what it should be */
	event_number++;
	ref_point += iso_interval_int * ISO_INT_UNIT_US;
	payload_number++;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[4],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[5],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);
	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is less than the PDU buffer size
 */
ZTEST(test_tx_framed, test_tx_framed_1_zero_sdu_1_frag_1_pdu_maxPDU)
{
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	struct pdu_iso_sdu_sh seg_hdr[2];
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint8_t testdata[1];
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, 1);
	(void)memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size = 0;
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = pdu_write_loc;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Send Event Timeout ----------------------------------------------- */
	isoal_tx_event_prepare(source_hdl, event_number);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment
 * into a single PDU followed by padding
 */
ZTEST(test_tx_framed, test_tx_framed_1_zero_sdu_1_frag_1_pdu_padding)
{
	struct tx_pdu_meta_buffer tx_pdu_meta_buf[3];
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer[3];
	isoal_source_handle_t source_hdl;
	struct pdu_iso_sdu_sh seg_hdr[2];
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint8_t testdata[1];
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[2]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, 1);
	(void)memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer[0].handle = (void *)&tx_pdu_meta_buf[0].node_tx;
	pdu_buffer[0].pdu = (struct pdu_iso *)tx_pdu_meta_buf[0].node_tx.pdu;
	pdu_buffer[0].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[1].handle = (void *)&tx_pdu_meta_buf[1].node_tx;
	pdu_buffer[1].pdu = (struct pdu_iso *)tx_pdu_meta_buf[1].node_tx.pdu;
	pdu_buffer[1].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[2].handle = (void *)&tx_pdu_meta_buf[2].node_tx;
	pdu_buffer[2].pdu = (struct pdu_iso *)tx_pdu_meta_buf[2].node_tx.pdu;
	pdu_buffer[2].size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size = 0;
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[2]);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = pdu_write_loc;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Send Event Timeout ----------------------------------------------- */
	isoal_tx_event_prepare(source_hdl, event_number);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 2 (Padding) */
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	/* PDU should not be written to */
	ZASSERT_PDU_WRITE_TEST_CALL_COUNT(2);

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 3 (Padding) */
	payload_number++;
	sdu_fragments = 0;

	/* PDU should not be written to */
	ZASSERT_PDU_WRITE_TEST_CALL_COUNT(2);

	ZASSERT_PDU_EMIT_TEST(history[2],
			      &tx_pdu_meta_buf[2].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment,
 * where PDU allocation fails
 */
ZTEST(test_tx_framed, test_tx_framed_1_sdu_1_frag_pdu_alloc_err)
{
	uint8_t testdata[TEST_TX_PDU_PAYLOAD_MAX - 5 -
			 (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	struct pdu_iso_sdu_sh seg_hdr[2];
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      TEST_TX_PDU_PAYLOAD_MAX - 5 -
				      (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	(void)memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = NULL;
	pdu_buffer.pdu = NULL;
	pdu_buffer.size = 0;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_ERR_PDU_ALLOC);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	ztest_set_assert_valid(true);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	ztest_set_assert_valid(false);

	zassert_equal(err, ISOAL_STATUS_ERR_PDU_ALLOC, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	sdu_fragments = 1;

	/* PDU should not be written to */
	ZASSERT_PDU_WRITE_TEST_CALL_COUNT(0);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Emit Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is less than the PDU buffer size,
 * where PDU emit fails
 */
ZTEST(test_tx_framed, test_tx_framed_1_sdu_1_frag_pdu_emit_err)
{
	uint8_t testdata[TEST_TX_PDU_PAYLOAD_MAX - 5 -
			 (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	struct pdu_iso_sdu_sh seg_hdr[2];
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      TEST_TX_PDU_PAYLOAD_MAX - 5 -
				      (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	(void)memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_ERR_PDU_EMIT);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_ERR_PDU_EMIT, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	ZASSERT_PDU_RELEASE_TEST(
		history[0],
		&tx_pdu_meta_buf.node_tx,
		bt_iso_handle(isoal_global.source_state[source_hdl].session.handle),
		ISOAL_STATUS_ERR_PDU_EMIT);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment
 * into a single PDU, relying on the ISO event deadline to release the PDU.
 */
ZTEST(test_tx_framed, test_tx_framed_2_sdu_1_frag_pdu_timeout)
{
	uint8_t testdata[40];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	struct pdu_iso_sdu_sh seg_hdr[2];
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint16_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 800;
	sdu_interval = 500000;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, 40);
	(void)memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 0;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size = 10;
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = pdu_write_loc + testdata_size;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be  emitted */

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Test PDU release */
	/* Simulate interleaving by setting context active flag */
	isoal_global.source_state[source_hdl].context_active = true;
	isoal_tx_event_prepare(source_hdl, event_number);
	isoal_global.source_state[source_hdl].context_active = false;

	/* SDU 2 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	memset(seg_hdr, 0, sizeof(seg_hdr));
	sdu_packet_number++;
	sdu_timestamp = 9249 + sdu_interval;
	sdu_total_size = 10;
	testdata_indx = testdata_size;
	testdata_size += 10;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 - Seg 2 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 10 + PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_write_loc = pdu_hdr_loc + PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = testdata_indx;
	pdu_write_size = pdu_write_loc + 10;
	sdu_fragments++;

	/* PDU should not be allocated */

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[4],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[5],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 3 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	memset(seg_hdr, 0, sizeof(seg_hdr));
	event_number++;
	sdu_packet_number++;
	sdu_timestamp = sdu_timestamp + sdu_interval;
	ref_point = ref_point + (iso_interval_int * ISO_INT_UNIT_US);
	sdu_total_size = 20;
	testdata_indx = testdata_size;
	testdata_size += 20;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 - Seg 2 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = testdata_indx;
	pdu_write_size = pdu_write_loc + sdu_total_size;
	sdu_fragments = 1;
	payload_number++;

	ZASSERT_PDU_WRITE_TEST(history[6],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[7],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[8],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU emit not expected */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(1);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Test PDU release */
	isoal_tx_event_prepare(source_hdl, event_number);

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests that consecutive events are used irrespective of the target event info
 * as long as they are feasible.
 */
ZTEST(test_tx_framed, test_tx_framed_event_utilization_1)
{
	const uint8_t number_of_pdus = 3;
	const uint8_t sdu_fragment_data_size = 25;
	const uint8_t testdata_size_max = sdu_fragment_data_size * 4;
	/* Two SDUs and one that would overflow into a new PDU */
	const uint8_t number_of_seg_hdr_buf = 3;

	struct tx_pdu_meta_buffer tx_pdu_meta_buf[number_of_pdus];
	struct pdu_iso_sdu_sh seg_hdr[number_of_seg_hdr_buf];
	struct isoal_pdu_buffer pdu_buffer[number_of_pdus];
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	uint8_t testdata[testdata_size_max];
	isoal_source_handle_t source_hdl;
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_end;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint64_t pdu_event_number;
	uint8_t iso_interval_int;
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint32_t pdu_ref_point;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = ISO_INT_UNIT_US - 50;            /* Less than an ISO interval */
	max_octets = TEST_TX_PDU_PAYLOAD_MAX;
	BN = 2;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* SDU 0 -------------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[2]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, testdata_size_max);
	(void)memset(&seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer[0].handle = (void *)&tx_pdu_meta_buf[0].node_tx;
	pdu_buffer[0].pdu = (struct pdu_iso *)tx_pdu_meta_buf[0].node_tx.pdu;
	pdu_buffer[0].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[1].handle = (void *)&tx_pdu_meta_buf[1].node_tx;
	pdu_buffer[1].pdu = (struct pdu_iso *)tx_pdu_meta_buf[1].node_tx.pdu;
	pdu_buffer[1].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[2].handle = (void *)&tx_pdu_meta_buf[2].node_tx;
	pdu_buffer[2].pdu = (struct pdu_iso *)tx_pdu_meta_buf[2].node_tx.pdu;
	pdu_buffer[2].size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 0;
	event_number = 5;
	pdu_event_number = event_number;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + iso_interval_us;
	pdu_ref_point = ref_point;
	sdu_total_size = sdu_fragment_data_size;
	testdata_indx = 0;
	testdata_size = sdu_fragment_data_size;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[1]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[2]);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer[0]);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 0 */
	payload_number = event_number * BN;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = pdu_ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_end = sdu_fragment_data_size + pdu_write_loc;
	sdu_fragments++;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_end - pdu_write_loc));

	seg_hdr[0].cmplt = 1;
	seg_hdr[0].len += (pdu_write_end - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 1 -------------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_packet_number++;
	event_number += 2;
	ref_point += iso_interval_us * 2;
	sdu_timestamp += sdu_interval;
	testdata_indx = testdata_size;
	testdata_size += sdu_fragment_data_size;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 10 */
	pdu_hdr_loc = pdu_write_end;
	seg_hdr[1].sc = 0;
	seg_hdr[1].cmplt = 0;
	seg_hdr[1].timeoffset = pdu_ref_point - sdu_timestamp;
	seg_hdr[1].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_write_loc = pdu_write_end + (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	pdu_write_end = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[4],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_end - pdu_write_loc));

	/* PDU should not be allocated */

	seg_hdr[1].len += (pdu_write_end - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[5],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_end,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* PDU 11 */
	payload_number++;
	seg_hdr[2].sc = 1;
	seg_hdr[2].cmplt = 0;
	seg_hdr[2].timeoffset = 0;
	seg_hdr[2].len = 0;
	sdu_read_loc += (pdu_write_end - pdu_write_loc);
	pdu_hdr_loc = 0;
	pdu_write_end = testdata_size - testdata_indx - (pdu_write_end - pdu_write_loc) +
			 PDU_ISO_SEG_HDR_SIZE;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[6],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[2],
			       PDU_ISO_SEG_HDR_SIZE);

	ZASSERT_PDU_WRITE_TEST(history[7],
			       pdu_buffer[1],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_end - pdu_write_loc));

	seg_hdr[2].cmplt = 1;
	seg_hdr[2].len += (pdu_write_end - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[8],
			       pdu_buffer[1],
			       pdu_hdr_loc,
			       &seg_hdr[2],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(1);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 -------------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_packet_number++;
	event_number += 2;
	ref_point += iso_interval_us * 2;
	sdu_timestamp += sdu_interval;
	testdata_indx = testdata_size;
	testdata_size += sdu_fragment_data_size;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 11 */

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_end,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* PDU 12 */
	payload_number++;
	pdu_event_number++;
	pdu_ref_point += iso_interval_us;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = pdu_ref_point - sdu_timestamp;
	seg_hdr[0].len = 3;
	sdu_read_loc = testdata_indx;
	pdu_hdr_loc = 0;
	pdu_write_end = testdata_size - testdata_indx  + PDU_ISO_SEG_HDR_SIZE +
			PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[9],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[10],
			       pdu_buffer[0],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_end - pdu_write_loc));

	seg_hdr[0].cmplt = 1;
	seg_hdr[0].len += (pdu_write_end - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[11],
			       pdu_buffer[0],
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(2);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Send Event Timeout ----------------------------------------------- */
	isoal_tx_event_prepare(source_hdl, pdu_event_number - 1);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(2);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 3 -------------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[2]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_packet_number++;
	event_number += 2;
	ref_point += iso_interval_us * 2;
	sdu_timestamp += sdu_interval;
	sdu_total_size = sdu_fragment_data_size;
	testdata_indx = testdata_size;
	testdata_size += sdu_fragment_data_size;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 12 */
	ZASSERT_PDU_EMIT_TEST(history[2],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_end,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 13 */
	payload_number++;

	/* Padding PDU */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(4);
	ZASSERT_PDU_EMIT_TEST(history[3],
			      &tx_pdu_meta_buf[1].node_tx,
			      payload_number,
			      0,
			      PDU_BIS_LLID_FRAMED,
			      0,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* PDU 14 */
	payload_number++;
	pdu_event_number++;
	pdu_ref_point += iso_interval_us;
	seg_hdr[1].sc = 0;
	seg_hdr[1].cmplt = 0;
	seg_hdr[1].timeoffset = pdu_ref_point - sdu_timestamp;
	seg_hdr[1].len = 3;
	sdu_read_loc = testdata_indx;
	pdu_hdr_loc = 0;
	pdu_write_end = testdata_size - testdata_indx + PDU_ISO_SEG_HDR_SIZE +
			PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[12],
			       pdu_buffer[2],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);

	ZASSERT_PDU_WRITE_TEST(history[13],
			       pdu_buffer[2],
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_end - pdu_write_loc));

	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_end - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[14],
			       pdu_buffer[2],
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(4);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Send Event Timeout ----------------------------------------------- */
	isoal_tx_event_prepare(source_hdl, pdu_event_number);


	ZASSERT_PDU_EMIT_TEST(history[4],
			      &tx_pdu_meta_buf[2].node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_end,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU 5 */
	payload_number++;

	/* Padding PDU */
	ZASSERT_PDU_EMIT_TEST(history[5],
			      &tx_pdu_meta_buf[0].node_tx,
			      payload_number,
			      0,
			      PDU_BIS_LLID_FRAMED,
			      0,
			      isoal_global.source_state[source_hdl].session.handle);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);
}

/**
 * Test Suite  :   TX framed EBQ test IAL-CIS-FRA-PER-BV07C
 *
 * Tests packing multiple SDU segments into the same PDU and release on event
 * timeout.
 */
ZTEST(test_tx_framed_ebq, test_tx_framed_cis_fra_per_bv07c)
{
	uint8_t testdata[40];
	struct tx_pdu_meta_buffer tx_pdu_meta_buf;
	struct tx_sdu_frag_buffer tx_sdu_frag_buf;
	struct isoal_pdu_buffer pdu_buffer;
	isoal_source_handle_t source_hdl;
	struct pdu_iso_sdu_sh seg_hdr[2];
	isoal_sdu_len_t sdu_total_size;
	isoal_pdu_len_t pdu_write_size;
	uint32_t stream_sync_delay;
	uint64_t sdu_packet_number;
	uint32_t group_sync_delay;
	uint16_t iso_interval_int;
	uint64_t payload_number;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_write_loc;
	uint16_t sdu_read_loc;
	uint64_t event_number;
	uint32_t sdu_interval;
	uint8_t sdu_fragments;
	uint16_t pdu_hdr_loc;
	uint32_t ref_point;
	isoal_status_t err;
	uint8_t max_octets;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 800;
	sdu_interval = 500000;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, 40);
	(void)memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 0;
	event_number = 0;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * ISO_INT_UNIT_US) - 50;
	sdu_total_size = 10;
	testdata_indx = 0;
	testdata_size = 10;
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,            /* Handle */
					 role,              /* Role */
					 true,              /* Framed */
					 BN,                /* BN */
					 FT,                /* FT */
					 max_octets,        /* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	SET_NEXT_PDU_ALLOC_BUFFER(&pdu_buffer);
	PDU_ALLOC_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_WRITE_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_EMIT_TEST_RETURNS(ISOAL_STATUS_OK);
	PDU_RELEASE_TEST_RETURNS(ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = pdu_write_loc + testdata_size;
	sdu_fragments = 1;

	ZASSERT_PDU_WRITE_TEST(history[0],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[1],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[2],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU should not be  emitted */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* SDU 2 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	memset(seg_hdr, 0, sizeof(seg_hdr));
	sdu_packet_number++;
	sdu_timestamp = sdu_timestamp + sdu_interval;
	sdu_total_size = 10;
	testdata_indx = testdata_size;
	testdata_size += 10;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 - Seg 2 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 10 + PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_write_loc = pdu_hdr_loc + PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = testdata_indx;
	pdu_write_size = pdu_write_loc + 10;
	sdu_fragments++;

	/* PDU should not be allocated */

	ZASSERT_PDU_WRITE_TEST(history[3],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[4],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[5],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU emit not expected */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(0);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Test PDU release */
	isoal_tx_event_prepare(source_hdl, event_number);

	ZASSERT_PDU_EMIT_TEST(history[0],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);

	/* SDU 3 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	memset(seg_hdr, 0, sizeof(seg_hdr));
	event_number++;
	sdu_packet_number++;
	sdu_timestamp = sdu_timestamp + sdu_interval;
	ref_point = ref_point + (iso_interval_int * ISO_INT_UNIT_US);
	sdu_total_size = 20;
	testdata_indx = testdata_size;
	testdata_size += 20;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE,
				      &testdata[testdata_indx],
				      (testdata_size - testdata_indx),
				      sdu_total_size,
				      sdu_packet_number,
				      sdu_timestamp,
				      sdu_timestamp,
				      ref_point,
				      event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 - Seg 2 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].len = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = testdata_indx;
	pdu_write_size = pdu_write_loc + sdu_total_size;
	sdu_fragments = 1;
	payload_number++;

	ZASSERT_PDU_WRITE_TEST(history[6],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[0],
			       (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));

	ZASSERT_PDU_WRITE_TEST(history[7],
			       pdu_buffer,
			       pdu_write_loc,
			       &testdata[sdu_read_loc],
			       (pdu_write_size - pdu_write_loc));

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].len += (pdu_write_size - pdu_write_loc);

	ZASSERT_PDU_WRITE_TEST(history[8],
			       pdu_buffer,
			       pdu_hdr_loc,
			       &seg_hdr[1],
			       PDU_ISO_SEG_HDR_SIZE);

	/* PDU emit not expected */
	ZASSERT_PDU_EMIT_TEST_CALL_COUNT(1);

	/* PDU release not expected (No Error) */
	ZASSERT_PDU_RELEASE_TEST_CALL_COUNT(0);

	/* Test PDU release */
	isoal_tx_event_prepare(source_hdl, event_number);

	ZASSERT_PDU_EMIT_TEST(history[1],
			      &tx_pdu_meta_buf.node_tx,
			      payload_number,
			      sdu_fragments,
			      PDU_BIS_LLID_FRAMED,
			      pdu_write_size,
			      isoal_global.source_state[source_hdl].session.handle);
}
