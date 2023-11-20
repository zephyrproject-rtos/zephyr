/*
 * Copyright (c) 2022 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

FAKE_VALUE_FUNC(isoal_status_t,
		sink_sdu_alloc_test,
		const struct isoal_sink *,
		const struct isoal_pdu_rx *,
		struct isoal_sdu_buffer *);

static struct {
	struct isoal_sdu_buffer *out[6];
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
	isoal_test_debug_trace_func_call(__func__, "IN");

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

#define ZASSERT_ISOAL_SDU_ALLOC_TEST(_typ, _sink, _pdu)                                            \
	zassert_equal_ptr(_sink,                                                                   \
			  sink_sdu_alloc_test_fake.arg0_##_typ,                                    \
			  "\t\tExpected alloc sink at %p, got %p.",                                \
			  _sink,                                                                   \
			  sink_sdu_alloc_test_fake.arg0_##_typ);                                   \
	zassert_equal_ptr(_pdu,                                                                    \
			  sink_sdu_alloc_test_fake.arg1_##_typ,                                    \
			  "\t\tExpected alloc PDU buffer at %p, got %p.",                          \
			  _pdu,                                                                    \
			  sink_sdu_alloc_test_fake.arg1_##_typ)

#define ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(_expected)                                         \
	zassert_equal(_expected,                                                                   \
		      sink_sdu_alloc_test_fake.call_count,                                         \
		      "Expected alloc called %u times, actual %u.",                                \
		      _expected,                                                                   \
		      sink_sdu_alloc_test_fake.call_count)

FAKE_VALUE_FUNC(isoal_status_t,
		sink_sdu_emit_test,
		const struct isoal_sink *,
		const struct isoal_emitted_sdu_frag *,
		const struct isoal_emitted_sdu *);

/**
 * This handler is called by custom_sink_sdu_emit_test using the non pointer versions of the
 * function's arguments. The tests are asserting on the argument content, since sink_sdu_emit_test()
 * is called multiple times with the same pointer (but different content) this additional fake is
 * used to store the history of the content.
 */
FAKE_VOID_FUNC(sink_sdu_emit_test_handler,
	       struct isoal_sink,
	       struct isoal_emitted_sdu_frag,
	       struct isoal_emitted_sdu);

/**
 * Callback test fixture to be provided for RX sink creation. Emits provided
 * SDU in buffer
 * @param[in]  sink_ctx  Sink context provided by ISO-AL
 * @param[in]  valid_sdu SDU buffer and details of SDU to be emitted
 * @return               Status of operation
 */
static isoal_status_t custom_sink_sdu_emit_test(const struct isoal_sink *sink_ctx,
						const struct isoal_emitted_sdu_frag *sdu_frag,
						const struct isoal_emitted_sdu *sdu)
{
	isoal_test_debug_trace_func_call(__func__, "IN");

	isoal_test_debug_print_rx_sdu(sink_ctx, sdu_frag, sdu);
	sink_sdu_emit_test_handler(*sink_ctx, *sdu_frag, *sdu);

	return sink_sdu_emit_test_fake.return_val;
}

#define ZASSERT_ISOAL_SDU_EMIT_TEST(_typ,                                                          \
				    _sink,                                                         \
				    _state,                                                        \
				    _frag_sz,                                                      \
				    _frag_status,                                                  \
				    _timestamp,                                                    \
				    _sn,                                                           \
				    _dbuf,                                                         \
				    _dbuf_sz,                                                      \
				    _total_sz,                                                     \
				    _sdu_status)                                                   \
	zassert_equal_ptr(_sink,                                                                   \
			  sink_sdu_emit_test_fake.arg0_##_typ,                                     \
			  "\t\tExpected sink at %p, got %p.",                                      \
			  _sink,                                                                   \
			  sink_sdu_emit_test_fake.arg0_##_typ);                                    \
	zassert_equal(_state,                                                                      \
		      sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu_state,                       \
		      "\t\tExpected SDU state '%s', got '%s'.",                                    \
		      STATE_TO_STR(_state),                                                        \
		      STATE_TO_STR(sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu_state));        \
	zassert_equal(_frag_sz,                                                                    \
		      sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu_frag_size,                   \
		      "\t\tExpected SDU frag of size %u, got %u.",                                 \
		      _frag_sz,                                                                    \
		      sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu_frag_size);                  \
	zassert_equal(_frag_status,                                                                \
		      sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu.status,                      \
		      "\t\tExpected SDU with status '%s', got '%s'.",                              \
		      DU_ERR_TO_STR(_frag_status),                                                 \
		      DU_ERR_TO_STR(sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu.status));      \
	zassert_equal(_timestamp,                                                                  \
		      sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu.timestamp,                   \
		      "\t\tExpected SDU with timestamp %u, got %u.",                               \
		      _timestamp,                                                                  \
		      sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu.timestamp);                  \
	zassert_equal(_sn,                                                                         \
		      sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu.sn,                          \
		      "\t\tExpected SDU with sequence number %u, got  %u.",                        \
		      _sn,                                                                         \
		      sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu.sn);                         \
	zassert_equal_ptr(_dbuf,                                                                   \
			  sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu.contents.dbuf,           \
			  "\t\tExpected SDU data buffer at %p, got %p.",                           \
			  _dbuf,                                                                   \
			  sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu.contents.dbuf);          \
	zassert_equal(_dbuf_sz,                                                                    \
		      sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu.contents.size,               \
		      "\t\tExpected SDU data buffer of size %u, got %u.",                          \
		      _dbuf_sz,                                                                    \
		      sink_sdu_emit_test_handler_fake.arg1_##_typ.sdu.contents.size);              \
	zassert_equal(_total_sz,                                                                   \
		      sink_sdu_emit_test_handler_fake.arg2_##_typ.total_sdu_size,                  \
		      "\t\tExpected total size of SDU %u,got %u.",                                 \
		      _total_sz,                                                                   \
		      sink_sdu_emit_test_handler_fake.arg2_##_typ.total_sdu_size);                 \
	zassert_equal(_sdu_status,                                                                 \
		      sink_sdu_emit_test_handler_fake.arg2_##_typ.collated_status,                 \
		      "\t\tExpected SDU with status '%s', got '%s'.",                              \
		      DU_ERR_TO_STR(_sdu_status),                                                  \
		      DU_ERR_TO_STR(sink_sdu_emit_test_handler_fake.arg2_##_typ.collated_status))

#define ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(_expected)                                          \
	zassert_equal(_expected,                                                                   \
		      sink_sdu_emit_test_fake.call_count,                                          \
		      "Expected emit called %u times,  actual %u.",                                \
		      _expected,                                                                   \
		      sink_sdu_emit_test_fake.call_count)

FAKE_VALUE_FUNC(isoal_status_t, sink_sdu_write_test, void *, const uint8_t *, const size_t);
/**
 * Callback test fixture to be provided for RX sink creation. Writes provided
 * data into target SDU buffer.
 * @param  dbuf        SDU buffer (Includes current write location field)
 * @param  pdu_payload Current PDU being reassembled by ISO-AL
 * @param  consume_len Length of data to transfer
 * @return             Status of the operation
 */
static isoal_status_t
custom_sink_sdu_write_test(void *dbuf, const uint8_t *pdu_payload, const size_t consume_len)
{
	isoal_test_debug_trace_func_call(__func__, "IN");

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

#define ZASSERT_ISOAL_SDU_WRITE_TEST(_typ, _frag_buf, _payload_buf, _length)                       \
	zassert_equal_ptr(_frag_buf,                                                               \
			  sink_sdu_write_test_fake.arg0_##_typ,                                    \
			  "\t\tExpected write buffer at %p, got %p.",                              \
			  _frag_buf,                                                               \
			  sink_sdu_write_test_fake.arg0_##_typ);                                   \
	zassert_equal_ptr(_payload_buf,                                                            \
			  sink_sdu_write_test_fake.arg1_##_typ,                                    \
			  "\t\tExpected write source at %p, got %p.",                              \
			  _payload_buf,                                                            \
			  sink_sdu_write_test_fake.arg1_##_typ);                                   \
	zassert_equal(_length,                                                                     \
		      sink_sdu_write_test_fake.arg2_##_typ,                                        \
		      "\t\tExpected write length of %u, got %u.",                                  \
		      _length,                                                                     \
		      sink_sdu_write_test_fake.arg2_##_typ)

#define ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(_expected)                                         \
	zassert_equal(_expected,                                                                   \
		      sink_sdu_write_test_fake.call_count,                                         \
		     "Expected write called %u times,  actual %u.",                                \
		      _expected,                                                                   \
		      sink_sdu_write_test_fake.call_count)

/**
 * RX common setup before running tests
 * @param f Input configuration parameters
 */
static void isoal_test_rx_common_before(void *f)
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
static int32_t calc_rx_latency_by_role(uint8_t role,
				       uint8_t framed,
				       uint8_t flush_timeout,
				       uint32_t sdu_interval,
				       uint16_t iso_interval_int,
				       uint32_t stream_sync_delay,
				       uint32_t group_sync_delay)
{
	int32_t latency;
	uint32_t iso_interval;

	latency = 0;
	iso_interval = iso_interval_int * ISO_INT_UNIT_US;

	switch (role) {
	case ISOAL_ROLE_PERIPHERAL:
		if (framed) {
			latency = stream_sync_delay + sdu_interval + (flush_timeout * iso_interval);
		} else {
			latency = stream_sync_delay + ((flush_timeout - 1) * iso_interval);
		}
		break;

	case ISOAL_ROLE_CENTRAL:
		if (framed) {
			latency = stream_sync_delay - group_sync_delay;
		} else {
			latency = stream_sync_delay - group_sync_delay -
				  (((iso_interval / sdu_interval) - 1) * iso_interval);
		}
		break;

	case ISOAL_ROLE_BROADCAST_SINK:
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
	PRINT("\tFT %d\n\tISO Interval %dus\n\tSDU Interval %dus"
	      "\n\tStream Sync Delay %dus\n\tGroup Sync Delay %dus\n\n",
	      flush_timeout,
	      iso_interval,
	      sdu_interval,
	      stream_sync_delay,
	      group_sync_delay);
#endif

	return latency;
}

static uint32_t get_next_time_offset(uint32_t time_offset,
				     uint32_t iso_interval_us,
				     uint32_t sdu_interval_us,
				     bool next_event_expected)
{
	uint32_t result;

	if (time_offset > sdu_interval_us) {
		result = time_offset - sdu_interval_us;
#if defined(DEBUG_TEST)
		PRINT("Increment time offset for same event %lu --> %lu\n", time_offset, result);
#endif
		zassert_false(next_event_expected);
	} else {
		result = time_offset + iso_interval_us - sdu_interval_us;
#if defined(DEBUG_TEST)
		PRINT("Increment time offset for next event %lu --> %lu\n", time_offset, result);
#endif
		zassert_true(next_event_expected);
	}

	return result;
}

/**
 * @breif Wrapper to test time wrapping
 * @param  time_now  Current time value
 * @param  time_diff Time difference (signed)
 * @return           Wrapped time after difference
 */
static uint32_t isoal_get_wrapped_time_test(uint32_t time_now, int32_t time_diff)
{
	uint32_t result = isoal_get_wrapped_time_us(time_now, time_diff);

#if defined(DEBUG_TEST)
	PRINT("[isoal_get_wrapped_time_us] time_now %12lu time_diff %12ld result %lu\n",
	      time_now,
	      time_diff,
	      result);
#endif

	return result;
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
static isoal_sink_handle_t basic_rx_test_setup(uint16_t handle,
					       uint8_t role,
					       uint8_t framed,
					       uint8_t burst_number,
					       uint8_t flush_timeout,
					       uint32_t sdu_interval,
					       uint16_t iso_interval_int,
					       uint32_t stream_sync_delay,
					       uint32_t group_sync_delay)
{
	isoal_sink_handle_t sink_hdl;
	isoal_status_t err;

#if defined(DEBUG_TEST)
	PRINT("RX Test Setup:\n\tHandle 0x%04x\n\tRole %s\n\tFraming %s"
	      "\n\tBN %u\n\tFT %d\n\tISO Interval %dus\n\tSDU Interval %dus"
	      "\n\tStream Sync Delay %dus\n\tGroup Sync Delay %dus\n\n",
	      handle,
	      ROLE_TO_STR(role),
	      framed ? "Framed" : "Unframed",
	      burst_number,
	      flush_timeout,
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

	/* Create a sink based on global parameters */
	err = isoal_sink_create(handle,
				role,
				framed,
				burst_number,
				flush_timeout,
				sdu_interval,
				iso_interval_int,
				stream_sync_delay,
				group_sync_delay,
				sink_sdu_alloc_test,
				sink_sdu_emit_test,
				sink_sdu_write_test,
				&sink_hdl);
	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Enable newly created sink */
	isoal_sink_enable(sink_hdl);

	return sink_hdl;
}

/**
 * Test Suite  :   RX basic test
 *
 * Test creating and destroying sinks upto the maximum with randomized
 * configuration parameters.
 */
ZTEST(test_rx_basics, test_sink_isoal_test_create_destroy)
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
		sdu_interval_int = 1;
		iso_interval_int = 1;
		iso_interval = iso_interval_int * ISO_INT_UNIT_US;
		sdu_interval = sdu_interval_int * ISO_INT_UNIT_US;
		stream_sync_delay = iso_interval - 200;
		group_sync_delay = iso_interval - 50;
		latency = 0;

		ztest_set_assert_valid(false);

		for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SINKS; i++) {
			res = ISOAL_STATUS_ERR_UNSPECIFIED;
			sink_hdl[i] = 0xFF;

			pdus_per_sdu = (burst_number * sdu_interval) / iso_interval;

			switch (role) {
			case ISOAL_ROLE_PERIPHERAL:
			case ISOAL_ROLE_CENTRAL:
			case ISOAL_ROLE_BROADCAST_SINK:
				latency = calc_rx_latency_by_role(role,
								  framed,
								  flush_timeout,
								  sdu_interval,
								  iso_interval_int,
								  stream_sync_delay,
								  group_sync_delay);
				break;

			default:
				ztest_set_assert_valid(true);
				break;
			}

			res = isoal_sink_create(handle,
						role,
						framed,
						burst_number,
						flush_timeout,
						sdu_interval,
						iso_interval_int,
						stream_sync_delay,
						group_sync_delay,
						sink_sdu_alloc_test,
						sink_sdu_emit_test,
						sink_sdu_write_test,
						&sink_hdl[i]);

			zassert_equal(isoal_global.sink_allocated[sink_hdl[i]],
				      ISOAL_ALLOC_STATE_TAKEN,
				      "");

			zassert_equal(isoal_global.sink_state[sink_hdl[i]].session.pdus_per_sdu,
				      pdus_per_sdu,
				      "%s pdus_per_sdu %d should be %d for:\n\tBN %d\n\tFT %d"
				      "\n\tISO Interval %dus\n\tSDU Interval %dus\n\tStream Sync "
				      "Delay %dus"
				      "\n\tGroup Sync Delay %dus",
				      (framed ? "Framed" : "Unframed"),
				      isoal_global.sink_state[sink_hdl[i]].session.pdus_per_sdu,
				      pdus_per_sdu,
				      burst_number,
				      flush_timeout,
				      iso_interval,
				      sdu_interval,
				      stream_sync_delay,
				      group_sync_delay);

			if (framed) {
				zassert_equal(
					isoal_global.sink_state[sink_hdl[i]].session.sdu_sync_const,
					latency,
					"%s latency framed %d should be %d",
					ROLE_TO_STR(role),
					isoal_global.sink_state[sink_hdl[i]].session.sdu_sync_const,
					latency);
			} else {
				zassert_equal(
					isoal_global.sink_state[sink_hdl[i]].session.sdu_sync_const,
					latency,
					"%s latency unframed %d should be %d",
					ROLE_TO_STR(role),
					isoal_global.sink_state[sink_hdl[i]].session.sdu_sync_const,
					latency);
			}

			zassert_equal(res,
				      ISOAL_STATUS_OK,
				      "Sink %d in role %s creation failed!",
				      i,
				      ROLE_TO_STR(role));

			isoal_sink_enable(sink_hdl[i]);

			zassert_equal(isoal_global.sink_state[sink_hdl[i]].sdu_production.mode,
				      ISOAL_PRODUCTION_MODE_ENABLED,
				      "Sink %d in role %s enable failed!",
				      i,
				      ROLE_TO_STR(role));

			framed = !framed;
			burst_number++;
			flush_timeout = (flush_timeout % 3) + 1;
			sdu_interval_int++;
			iso_interval_int = iso_interval_int * sdu_interval_int;
			sdu_interval = (sdu_interval_int * ISO_INT_UNIT_US) - (framed ? 100 : 0);
			iso_interval = iso_interval_int * ISO_INT_UNIT_US;
			stream_sync_delay = iso_interval - (200 * i);
			group_sync_delay = iso_interval - 50;
		}

		/* Destroy in order */
		for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SINKS; i++) {
			isoal_sink_destroy(sink_hdl[i]);

			zassert_equal(isoal_global.sink_allocated[sink_hdl[i]],
				      ISOAL_ALLOC_STATE_FREE,
				      "Sink destruction failed!");

			zassert_equal(isoal_global.sink_state[sink_hdl[i]].sdu_production.mode,
				      ISOAL_PRODUCTION_MODE_DISABLED,
				      "Sink disable failed!");
		}
	}
}

/**
 * Test Suite  :   RX basic test
 *
 * Test error return on exceeding the maximum number of sinks available.
 */
ZTEST(test_rx_basics, test_sink_isoal_test_create_err)
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
	role = ISOAL_ROLE_PERIPHERAL;
	burst_number = 1;
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

	for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SINKS; i++) {
		res = isoal_sink_create(handle,
					role,
					framed,
					burst_number,
					flush_timeout,
					sdu_interval,
					iso_interval_int,
					stream_sync_delay,
					group_sync_delay,
					sink_sdu_alloc_test,
					sink_sdu_emit_test,
					sink_sdu_write_test,
					&sink_hdl[i]);

		zassert_equal(res,
			      ISOAL_STATUS_OK,
			      "Sink %d in role %s creation failed!",
			      i,
			      ROLE_TO_STR(role));
	}

	res = isoal_sink_create(handle,
				role,
				framed,
				burst_number,
				flush_timeout,
				sdu_interval,
				iso_interval_int,
				stream_sync_delay,
				group_sync_delay,
				sink_sdu_alloc_test,
				sink_sdu_emit_test,
				sink_sdu_write_test,
				&sink_hdl[CONFIG_BT_CTLR_ISOAL_SINKS]);

	zassert_equal(res,
		      ISOAL_STATUS_ERR_SINK_ALLOC,
		      "Sink creation did not return error as expected!");
}

/**
 * Test Suite  :   RX basic test
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size = 0;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* Disable the sink */
	isoal_sink_disable(sink_hdl);

	/* Send SDU in a single PDU */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of a single valid RX PDU into an SDU.
 *
 * Expected Sequence:
 * -- Total of 1 SDUs released across 1 events
 * -- Event 1: PDU0 Valid  SDU 0 Unframed Single
 *                  -----> SDU 0 Valid           (Released)
 *
 */
ZTEST(test_rx_unframed, test_rx_unframed_single_pdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 0 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size = 23;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* Send SDU in a single PDU */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	/* No padding PDUs expected, so move to waiting for start fragment */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests boundary conditions of time wrapping function
 */
ZTEST(test_rx_unframed, test_rx_time_wrapping)
{
	const uint32_t time_wrapping_point = ISOAL_TIME_WRAPPING_POINT_US;
	uint32_t expected_result;
	int32_t time_diff;
	uint32_t time_now;
	uint32_t result;

	/* Maximum negative difference from 0 */
	time_now = 0;
	time_diff = (time_wrapping_point == UINT32_MAX ? INT32_MIN : -ISOAL_TIME_WRAPPING_POINT_US);
	expected_result = ISOAL_TIME_WRAPPING_POINT_US + time_diff + 1;
	result = isoal_get_wrapped_time_test(time_now, time_diff);
	zassert_equal(result, expected_result, "%lu != %lu", result, expected_result);

	/* Maximum negative difference from maximum time */
	time_now = ISOAL_TIME_WRAPPING_POINT_US;
	time_diff = (time_wrapping_point == UINT32_MAX ? INT32_MIN : -ISOAL_TIME_WRAPPING_POINT_US);
	expected_result = ISOAL_TIME_WRAPPING_POINT_US + time_diff;
	result = isoal_get_wrapped_time_test(time_now, time_diff);
	zassert_equal(result, expected_result, "%lu != %lu", result, expected_result);

	/* Maximum positive difference from maximum time */
	time_now = ISOAL_TIME_WRAPPING_POINT_US;
	time_diff = (time_wrapping_point == UINT32_MAX ? INT32_MAX : ISOAL_TIME_WRAPPING_POINT_US);
	expected_result = time_diff - 1;
	result = isoal_get_wrapped_time_test(time_now, time_diff);
	zassert_equal(result, expected_result, "%lu != %lu", result, expected_result);
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests time wrapping in reassembly of a single valid RX PDU into an SDU.
 *
 * Expected Sequence:
 * -- Total of 1 SDUs released across 1 events
 * -- Event 1: PDU0 Valid  SDU 0 Unframed Single
 *                  -----> SDU 0 Valid           (Released)
 *
 */
ZTEST(test_rx_unframed, test_rx_unframed_single_pdu_ts_wrap1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 0 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);

	/* SDU time stamp should wrap back to 0 */
	pdu_timestamp = (ISOAL_TIME_WRAPPING_POINT_US - latency) + 1;
	sdu_timestamp = 0;

	seqn = 0;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size = 23;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* Send SDU in a single PDU */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	/* No padding PDUs expected, so move to waiting for start fragment */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests time wrapping in reassembly of a single valid RX PDU into an SDU.
 *
 * Expected Sequence:
 * -- Total of 1 SDUs released across 1 events
 * -- Event 1: PDU0 Valid  SDU 0 Unframed Single
 *                  -----> SDU 0 Valid           (Released)
 *
 */
ZTEST(test_rx_unframed, test_rx_unframed_single_pdu_ts_wrap2)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_CENTRAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 0 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);

	/* SDU time stamp should wrap back to max time */
	pdu_timestamp = (-latency) - 1;
	sdu_timestamp = ISOAL_TIME_WRAPPING_POINT_US;

	seqn = 0;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size = 23;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* Send SDU in a single PDU */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	/* No padding PDUs expected, so move to waiting for start fragment */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of two valid RX PDU into a single SDU.
 *
 * Expected Sequence:
 * -- Total of 1 SDUs released across 1 events
 * -- Event 1: PDU0 Valid  SDU 0 Unframed Start
 *             PDU1 Valid  SDU 0 Unframed End
 *                  -----> SDU 0 Valid           (Released)
 *
 */
ZTEST(test_rx_unframed, test_rx_unframed_dbl_pdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 0 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* Send PDU with start fragment */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	/* No padding PDUs expected, so move to waiting for start fragment */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	/* Send PDU with end fragment  */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(1);

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	/* As two PDUs per SDU, no padding is expected */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of three SDUs where the end of the first two were not seen
 *
 * Expected Sequence:
 * -- Total of 1 SDUs released across 1 events
 * -- Event 1: PDU0 Valid  SDU 0 Unframed   Start
 *             PDU1 Valid  SDU 0 Unframed   Cont.
 *                  -----> SDU 0 Bit Errors        (Released)
 *             PDU2 Valid  SDU 1 Unframed   Start
 *             PDU3 Valid  SDU 1 Unframed   Cont.
 *                  -----> SDU 1 Bit Errors        (Released)
 * -- Event 2: PDU4 Valid  SDU 2 Unframed   Single
 *                  -----> SDU 2 Valid             (Released)
 *
 */
ZTEST(test_rx_unframed, test_rx_unframed_dbl_split)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US / 2;
	BN = 4;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* SDU 0 - PDU 0 -----------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 53);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be not emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* SDU 0 - PDU 1 -----------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(1);

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* SDU 1 - PDU 2 -----------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency + sdu_interval);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 -------------------------------------------------------------*/
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(1);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* SDU 1 - PDU 3 -----------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 -------------------------------------------------------------*/
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(2);

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* SDU 2 - PDU 4 -----------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	pdu_timestamp = 9249 + ISO_INT_UNIT_US;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 2 -------------------------------------------------------------*/
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	/* Expecting padding PDU as PDUs per SDU is 2 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of one SDUs in five fragments
 *
 * Expected Sequence:
 * -- Total of 1 SDUs released across 1 events
 * -- Event 1: PDU0 Valid  SDU 0 Unframed   Start
 *             PDU1 Valid  SDU 0 Unframed   Cont.
 *             PDU2 Valid  SDU 0 Unframed   Cont.
 *             PDU3 Valid  SDU 0 Unframed   Cont.
 *             PDU4 Valid  SDU 0 Unframed   End
 *                  -----> SDU 0 Valid             (Released)
 *
 */
ZTEST(test_rx_unframed, test_rx_unframed_multi_split)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 5;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 0 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 53);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(1);

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(1);

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(1);

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(1);

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of one SDUs in five fragments where the SDU buffer size is
 * reached
 *
 * Expected Sequence:
 * -- Total of 1 SDUs released across 1 events
 * -- Event 1: PDU0 Valid  SDU 0 Unframed   Start
 *             PDU1 Valid  SDU 0 Unframed   Cont.
 *                  -----> SDU 0 Valid      Frag 1 (Released)
 *             PDU2 Valid  SDU 0 Unframed   Cont.
 *                  -----> SDU 0 Valid      Frag 2 (Released)
 *             PDU3 Valid  SDU 0 Unframed   Cont.
 *             PDU4 Valid  SDU 0 Unframed   End
 *                  -----> SDU 0 Valid      Frag 3 (Released)
 *
 */
ZTEST(test_rx_unframed, test_rx_unframed_multi_split_on_border)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[100];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 5;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 0 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 100);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = 40;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 17;
	sdu_size = 17;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 - Frag 1 ----------------------------------------------------*/
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 23;
	sdu_size += 23;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, 100);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 - Frag 1 ----------------------------------------------------*/
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(1);

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_START,                       /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 40;
	sdu_size = 40;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, 100);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 - Frag 2 ----------------------------------------------------*/
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_CONT,                        /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 - Frag 3 ----------------------------------------------------*/
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(2);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, 100);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 - Frag 3 ----------------------------------------------------*/
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(3);

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_END,                         /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of one SDUs sent in one PDU when the SDU fragment size is
 * small, resulting in multiple SDU fragments released during reassembly
 */
ZTEST(test_rx_unframed, test_rx_unframed_long_pdu_short_sdu)
{
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[2];
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct isoal_sdu_buffer sdu_buffer[2];
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_test_data_buffer(testdata, 40);
	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[0].size = 20;
	sdu_buffer[1].size = 20;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 40;
	sdu_size = 20;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, 40);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* SDU 1 */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[0],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(history[0],
				     &rx_sdu_frag_buf[0],     /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3], /* PDU payload */
				     20);                     /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[0],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_START,                       /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 2 */
	sdu_size = 20;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, 40);

	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[1],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(history[1],
				     &rx_sdu_frag_buf[1],          /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + 20], /* PDU payload */
				     20);                          /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[1],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_END,                         /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of two SDUs where the end fragment of the first was not seen
 */
ZTEST(test_rx_unframed, test_rx_unframed_dbl_pdu_prem)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of SDUs with PDU errors
 */
ZTEST(test_rx_unframed, test_rx_unframed_single_pdu_err)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 0;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_ERRORS,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	pdu_timestamp = 9249 + ISO_INT_UNIT_US;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	testdata_indx = testdata_size;
	testdata_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_LOST_DATA,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_PDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of SDUs where PDUs are not in sequence
 */
ZTEST(test_rx_unframed, test_rx_unframed_seq_err)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 43);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 Not transferred to ISO-AL ------------------------------------*/
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	/* PDU count will not have reached 3 as one PDU was not received, so
	 * last_pdu will not be set and the state should remain in Error
	 * Spooling.
	 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	pdu_timestamp = 9249 + ISO_INT_UNIT_US;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	/* Detecting the transition from an end fragment to a start fragment
	 * should have triggered the monitoring code to pull the state machine
	 * out of Eroor spooling and directly into the start of a new SDU. As
	 * this was not an end fragment, the next state should be continue.
	 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of  SDUs where PDUs are not in sequence with errors
 * Tests error prioritization
 */
ZTEST(test_rx_unframed, test_rx_unframed_seq_pdu_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 43);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 Not transferred to ISO-AL ------------------------------------*/
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	/* PDU status ISOAL_PDU_STATUS_ERRORS */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_ERRORS,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(1);

	/* SDU payload should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(1);

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	/* PDU count will not have reached 3 as one PDU was not received, so
	 * last_pdu will not be set and the state should remain in Error
	 * Spooling.
	 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	pdu_timestamp = 9249 + ISO_INT_UNIT_US;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	/* Detecting the transition from an end fragment to a start fragment
	 * should have triggered the monitoring code to pull the state machine
	 * out of Eroor spooling and directly into the start of a new SDU. As
	 * this was not an end fragment, the next state should be continue.
	 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 5 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	/* Expecting padding so state should be Error Spooling */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of  SDUs where PDUs are not in sequence with errors
 * Tests releasing and collating information for buffered SDUs when an error in
 * reception occurs.
 */
ZTEST(test_rx_unframed, test_rx_unframed_seq_pdu_err2)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[80];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 80);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = 40;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 40;
	sdu_size = 40;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, 50);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_LOST_DATA);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_START,                       /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 Not transferred to ISO-AL ------------------------------------*/
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 0;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, 50);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	/* PDU status ISOAL_PDU_STATUS_ERRORS */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_ERRORS,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(1);

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_END,                         /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	/* PDU count will not have reached 3 as one PDU was not received, so
	 * last_pdu will not be set and the state should remain in Error
	 * Spooling.
	 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	pdu_timestamp = 9249 + ISO_INT_UNIT_US;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	/* Detecting the transition from an end fragment to a start fragment
	 * should have triggered the monitoring code to pull the state machine
	 * out of Eroor spooling and directly into the start of a new SDU. As
	 * this was not an end fragment, the next state should be continue.
	 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 5 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	/* Expecting padding so state should be Error Spooling */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of  SDUs where valid PDUs are followed by padding
 */
ZTEST(test_rx_unframed, test_rx_unframed_padding)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 4;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 43);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	/* Expecting padding PDUs so should be in Error Spool state */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;

	/* PDU padding 1 */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;

	/* PDU padding 2 */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of  SDUs with padding where the end was not seen
 */
ZTEST(test_rx_unframed, test_rx_unframed_padding_no_end)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 33);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;

	/* PDU padding 1 */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	/* PDU padding 2 */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of  SDUs where only padding has been received without any
 * other valid PDUs.
 */
ZTEST(test_rx_unframed, test_rx_unframed_padding_only)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 0;
	sdu_size = 0;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU padding 1 */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       NULL,
				       0,
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(0);

	/* SDU should not be written to */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;

	/* PDU padding 2 */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       NULL,
				       0,
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(0);

	/* SDU should not be written to */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA,
					       ISOAL_SDU_STATUS_LOST_DATA);

	/* PDU padding 3 */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       NULL,
				       0,
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU should not be written to */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of  SDUs with padding where the end was not seen where
 * padding is leads the data (not an expected case).
 */
ZTEST(test_rx_unframed, test_rx_unframed_padding_leading)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	sdu_interval = ISO_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 33);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 0;
	sdu_size = 0;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(0);

	/* SDU should not be written to */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;

	/* PDU padding 1 */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(0);

	/* SDU should not be written to */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_size = 13;
	sdu_size = 13;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */

	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of a SDU where there is an error in the first PDU followed
 * by valid padding PDUs
 */
ZTEST(test_rx_unframed, test_rx_unframed_padding_error1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 13);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 0;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_ERRORS,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;

	/* PDU padding 1 */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;

	/* PDU padding 2 */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of a SDU where the second PDU is corrupted and appears to
 * be a padding PDU
 */
ZTEST(test_rx_unframed, test_rx_unframed_padding_error2)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 13);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	/* PDU with errors that appears as padding */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_ERRORS,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;

	/* PDU padding 1 */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of a SDU where only the padding PDU has errors
 */
ZTEST(test_rx_unframed, test_rx_unframed_padding_error3)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;

	/* PDU padding with errors */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_ERRORS,
				       &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of a zero length SDU
 */
ZTEST(test_rx_unframed, test_rx_unframed_zero_len_packet)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 13);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 0;
	sdu_size = 0;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of SDUs where PDUs are not in sequence followed by a zero
 * length SDU
 */
ZTEST(test_rx_unframed, test_rx_unframed_seq_err_zero_length)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 43);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 2000;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 Not transferred to ISO-AL ------------------------------------*/
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	/* PDU count will not have reached 3 as one PDU was not received, so
	 * last_pdu will not be set and the state should remain in Error
	 * Spooling.
	 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	payload_number++;
	seqn++;
	pdu_timestamp = 9249 + ISO_INT_UNIT_US;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	testdata_indx = testdata_size;
	sdu_size = 0;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_COMPLETE_END,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	/* Detecting the transition from an end fragment to a start fragment
	 * should have triggered the monitoring code to pull the state machine
	 * out of Eroor spooling and directly into the start of a new SDU. As
	 * this was not a zero length SDU, the next state should be Error
	 * Spooling to dispense with padding PDUs.
	 */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests reassembly of a SDU in two PDUs where the end was not seen and BN is
 * two which should return to FSM start after reassembly
 */
ZTEST(test_rx_unframed, test_rx_unframed_dbl_pdu_no_end)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX unframed PDU reassembly
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 13);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* Invalid LLID - Valid PDU*/
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_FRAMED,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* expecting an assertion */
	ztest_set_assert_valid(true);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	ztest_set_assert_valid(false);
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests assertion on receiving a PDU with an invalid LLID without errors as
 * the second PDU of the SDU
 */
ZTEST(test_rx_unframed, test_rx_unframed_dbl_pdu_invalid_llid2)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	/* Invalid LLID - Valid PDU */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_FRAMED,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Test recombine (Black Box) */
	/* Expecting an assertion */
	ztest_set_assert_valid(true);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	ztest_set_assert_valid(false);
}

/**
 * Test Suite  :   RX unframed PDU reassembly
 *
 * Tests receiving a PDU with an invalid LLID with errors. This should not
 * result in a assertion as it could happen if a RX reaches it's flush timeout.
 */
ZTEST(test_rx_unframed, test_rx_unframed_dbl_pdu_invalid_llid2_pdu_err)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ISO_INT_UNIT_US;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  false,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       false,             /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_unframed_pdu(PDU_BIS_LLID_START_CONTINUE,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_VALID,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf,                 /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3],          /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	/* Invalid LLID - Valid PDU */
	isoal_test_create_unframed_pdu(PDU_BIS_LLID_FRAMED,
				       &testdata[testdata_indx],
				       (testdata_size - testdata_indx),
				       payload_number,
				       pdu_timestamp,
				       ISOAL_PDU_STATUS_ERRORS,
				       &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(1);

	/* SDU payload should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(1);

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU
 */
ZTEST(test_rx_framed, test_rx_framed_single_pdu_single_sdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * ISO_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size = 23;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests time wrapping recombination of a single SDU from a single segmented PDU
 */
ZTEST(test_rx_framed, test_rx_framed_single_pdu_single_sdu_ts_wrap1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * ISO_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	pdu_timestamp = ISOAL_TIME_WRAPPING_POINT_US - latency + sdu_timeoffset + 1;
	sdu_timestamp = 0;
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size = 23;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests reverse time wrapping in reassembly of a single valid RX PDU into an
 * SDU.
 */
ZTEST(test_rx_framed, test_rx_framed_single_pdu_single_sdu_ts_wrap2)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_CENTRAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * ISO_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 23);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	pdu_timestamp = (-latency) + sdu_timeoffset - 1;
	sdu_timestamp = ISOAL_TIME_WRAPPING_POINT_US;
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size = 23;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from three segmented PDUs
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_single_sdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * ISO_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 33);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_cont(&testdata[testdata_indx],
							 (testdata_size - testdata_indx),
							 &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[1]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[2] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[2]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of two SDUs from three segmented PDUs
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[2];
	struct isoal_sdu_buffer sdu_buffer[2];
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[2];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[2];
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	isoal_sdu_cnt_t seqn;
	uint32_t sdu_interval;
	uint8_t testdata[46];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = (iso_interval_int * ISO_INT_UNIT_US);
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_test_data_buffer(testdata, 46);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[0] += 10;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size[1] = 13;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	pdu_data_loc[2] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */

	/* SDU 1 */
	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(history[1],
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[1]],
				     /* PDU payload */
				     10); /* Size */
	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 2 */
	seqn++;
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[1], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[2]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[1] += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[1], sdu_size[1]);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[1], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[3]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[1],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a zero length SDU
 */
ZTEST(test_rx_framed, test_rx_framed_zero_length_sdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[3];
	struct isoal_sdu_buffer sdu_buffer[3];
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[3];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[3];
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	isoal_sdu_cnt_t seqn[3];
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[2]);
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
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[0] = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[0] += 10;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[1] = seqn[0] + 1;
	testdata_indx = testdata_size;
	sdu_size[1] = 0;

	/* Zero length SDU */
	pdu_data_loc[2] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[2] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[2] = seqn[1] + 1;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[2] = 10;

	pdu_data_loc[3] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[2]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* SDU 1 */
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(history[1],
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[1]],
				     /* PDU payload */
				     10); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[0],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn[0],                            /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 2 */
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[1], sdu_size[1]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[1],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[1],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[1],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn[1],                            /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 3 */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[2], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[3]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[2] += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[2], sdu_size[2]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[2], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[3]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[2],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[2],                   /* Timestamp */
				    seqn[2],                            /* Seq. number */
				    sdu_buffer[2].dbuf,                 /* Buffer */
				    sdu_buffer[2].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU followed by
 * padding
 */
ZTEST(test_rx_framed, test_rx_framed_dbl_pdu_dbl_sdu_padding)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 33);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* PDU 2 -------------------------------------------------------------*/
	/* Padding PDU */
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;
	testdata_indx = testdata_size;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(1);

	/* SDU should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(1);

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(1);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[1]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests release of SDUs when receiving only padding PDUs
 *
 * Expected Sequence:
 * -- Total of 3 SDUs released across 2 events
 * -- Event 1: PDU0 Valid        Framed Padding
 *             PDU1 Valid        Framed Padding
 *             PDU2 Valid        Framed Padding
 *    Event 2: PDU3 Valid  SDU 3 Framed Single
 *                  -----> SDU 0 Lost           (Released)
 *                  -----> SDU 1 Lost           (Released)
 *                  -----> SDU 2 Lost           (Released)
 *                  -----> SDU 3 Valid          (Released)
 *             PDU4 N/A
 *             PDU5 N/A
 *
 */
ZTEST(test_rx_framed, test_rx_framed_padding_only)
{
	const uint8_t number_of_pdus = 3;
	const uint8_t testdata_size_max = 20;

	struct rx_sdu_frag_buffer rx_sdu_frag_buf[4];
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct isoal_sdu_buffer sdu_buffer[4];
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_data_loc;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[testdata_size_max];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / number_of_pdus) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 0 -------------------------------------------------------------*/
	/* Padding PDU */
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[2]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[3]);
	init_test_data_buffer(testdata, testdata_size_max);
	pdu_data_loc = 0;

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[2].dbuf = &rx_sdu_frag_buf[2];
	sdu_buffer[3].dbuf = &rx_sdu_frag_buf[3];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[2].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[3].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - iso_interval_us);
	seqn = 0;
	testdata_indx = testdata_size = 0;
	sdu_size = 0;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(0);

	/* SDU payload should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU payload should not be emitted */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* PDU 1 -------------------------------------------------------------*/
	/* Padding PDU */
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(0);

	/* SDU should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* PDU 2 -------------------------------------------------------------*/
	/* Padding PDU */
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(0);

	/* SDU should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* SDU 0 -------------------------------------------------------------*/
	/* Missing */

	/* SDU 1 -------------------------------------------------------------*/
	/* Missing */
	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);

	/* SDU 2 -------------------------------------------------------------*/
	/* Missing */
	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += iso_interval_us;

	testdata_indx = 0;
	testdata_size = testdata_size_max;
	sdu_size = testdata_size_max;

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, true);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							sdu_timeoffset,
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[2]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[3]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[0],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[0],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 1 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	sdu_timestamp += sdu_interval;
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[1],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[1],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 2 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	sdu_timestamp += sdu_interval;
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[2],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[2],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[2].dbuf,                 /* Buffer */
				    sdu_buffer[2].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 3 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[3], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc],
								       /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[3].dbuf,                 /* Buffer */
				    sdu_buffer[3].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests release of SDUs when receiving only padding PDUs
 *
 * Expected Sequence:
 * -- Total of 3 SDUs released across 2 events
 * -- Event 1: PDU0 Valid        Framed Padding
 *             PDU1 Valid        Framed Padding
 *             PDU2 Valid        Framed Padding
 *    Event 2: PDU3 Errors SDU 3 Framed Single
 *                  -----> SDU 0 Lost           (Released)
 *                  -----> SDU 1 Lost           (Released)
 *                  -----> SDU 2 Lost           (Released)
 *                  -----> SDU 3 Bit Errors     (Released)
 *             PDU4 N/A
 *             PDU5 N/A
 *
 */
ZTEST(test_rx_framed, test_rx_framed_padding_only_pdu_err)
{
	const uint8_t number_of_pdus = 3;
	const uint8_t testdata_size_max = 20;

	struct rx_sdu_frag_buffer rx_sdu_frag_buf[3];
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct isoal_sdu_buffer sdu_buffer[4];
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint16_t pdu_data_loc;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[testdata_size_max];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / number_of_pdus) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 0 -------------------------------------------------------------*/
	/* Padding PDU */
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[2]);
	init_test_data_buffer(testdata, testdata_size_max);
	pdu_data_loc = 0;

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[2].dbuf = &rx_sdu_frag_buf[2];
	sdu_buffer[3].dbuf = &rx_sdu_frag_buf[2];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[2].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[3].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - iso_interval_us);
	seqn = 0;
	testdata_indx = testdata_size = 0;
	sdu_size = 0;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(0);

	/* SDU payload should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU payload should not be emitted */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* PDU 1 -------------------------------------------------------------*/
	/* Padding PDU */
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(0);

	/* SDU should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* PDU 2 -------------------------------------------------------------*/
	/* Padding PDU */
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(0);

	/* SDU should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* SDU 0 -------------------------------------------------------------*/
	/* Missing */

	/* SDU 1 -------------------------------------------------------------*/
	/* Missing */
	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);

	/* SDU 2 -------------------------------------------------------------*/
	/* Missing */
	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	pdu_timestamp += iso_interval_us;

	testdata_indx = 0;
	testdata_size = testdata_size_max;
	sdu_size = testdata_size_max;

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, true);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_ERRORS,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							sdu_timeoffset,
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[2]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[3]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[0],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[0],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 1 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	sdu_timestamp += sdu_interval;
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[1],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[1],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 2 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	sdu_timestamp += sdu_interval;
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[2],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[2],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[2].dbuf,                 /* Buffer */
				    sdu_buffer[2].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */


	/* SDU 3 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	sdu_timestamp += sdu_interval;
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[3].dbuf,                 /* Buffer */
				    sdu_buffer[3].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU with errors,
 * followed by a valid PDU
 */
ZTEST(test_rx_framed, test_rx_framed_dbl_pdu_dbl_sdu_pdu_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 33);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	/* PDU will have errors. Time stamp is only an approximation */
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - iso_interval_us);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size = 0;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_ERRORS,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size = 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[1]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU with errors,
 * followed by a valid PDU
 */
ZTEST(test_rx_framed, test_rx_framed_dbl_pdu_dbl_sdu_pdu_err2)
{
	const uint8_t test_data_size = 33;
	const uint8_t max_sdu_burst = 2;

	struct rx_sdu_frag_buffer rx_sdu_frag_buf[max_sdu_burst];
	struct isoal_sdu_buffer sdu_buffer[max_sdu_burst];
	isoal_sdu_status_t collated_status[max_sdu_burst];
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	isoal_sdu_len_t sdu_size[max_sdu_burst];
	uint16_t total_sdu_size[max_sdu_burst];
	uint32_t sdu_timestamp[max_sdu_burst];
	uint8_t testdata[test_data_size];
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	init_test_data_buffer(testdata, test_data_size);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	/* PDU will have errors. Time stamp is only an approximation */
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - iso_interval_us);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 23;
	sdu_size[0] = 0;
	total_sdu_size[0] = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status[0] =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_LOST_DATA,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(0);

	/* SDU payload should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);

	payload_number++;

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[1] = 10;
	total_sdu_size[1] = COLLATED_RX_SDU_INFO(sdu_size[1], sdu_size[1]);
	collated_status[1] = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* SDU 0 -------------------------------------------------------------*/
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[0],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[0],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size[0],                  /* Total size */
				    collated_status[0]);                /* SDU status */

	/* SDU 1 -------------------------------------------------------------*/
	seqn++;
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(1);
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[1], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[1]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[1],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size[1],                  /* Total size */
				    collated_status[1]);                /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU with errors,
 * mainly focussing on the release of SDUs buffered before the PDU with errors
 * was received. The first PDU triggers a release of a filled SDU and the
 * reception of the second PDU is expected to trigger allocation and release of
 * a new SDU to indicate the error and the status information should be collated
 * accordingly.
 */
ZTEST(test_rx_framed, test_rx_framed_dbl_pdu_dbl_sdu_pdu_err3)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint16_t total_sdu_size;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[50];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * ISO_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 50);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = 35;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 35;
	sdu_size = 35;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, 35);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_PDU_STATUS_VALID, ISOAL_SDU_STATUS_LOST_DATA);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_START,                       /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;

	testdata_indx = testdata_size;
	testdata_size += 15;
	sdu_size = 0;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, 35);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_PDU_STATUS_LOST_DATA, ISOAL_PDU_STATUS_LOST_DATA);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_LOST_DATA,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_cont(&testdata[testdata_indx],
							 (testdata_size - testdata_indx),
							 &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_END,                         /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU with errors,
 * followed by a valid PDU
 */
ZTEST(test_rx_framed, test_rx_framed_dbl_pdu_dbl_sdu_seq_err1)
{
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[2];
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct isoal_sdu_buffer sdu_buffer[2];
	isoal_sdu_status_t collated_status[2];
	isoal_sink_handle_t sink_hdl;
	isoal_sdu_len_t sdu_size[2];
	uint16_t total_sdu_size[2];
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	uint32_t sdu_timestamp[2];
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 0 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_test_data_buffer(testdata, 33);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;
	total_sdu_size[0] = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status[0] = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size[0],                  /* Total size */
				    collated_status[0]);                /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* PDU 1 -------------------------------------------------------------*/
	/* Not transferred to the ISO-AL */
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);

	payload_number++;

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);

	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[0] = 0;
	total_sdu_size[0] = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status[0] =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);

	payload_number++;

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[1] = 10;
	total_sdu_size[1] = COLLATED_RX_SDU_INFO(sdu_size[1], sdu_size[1]);
	collated_status[1] = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* SDU 2 -------------------------------------------------------------*/
	seqn++;
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[1],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[1],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size[0],                  /* Total size */
				    collated_status[0]);                /* SDU status */

	/* SDU 3 -------------------------------------------------------------*/
	seqn++;
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[1], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[1]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[1],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size[1],                  /* Total size */
				    collated_status[1]);                /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from three segmented PDUs with errors
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_single_sdu_pdu_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = iso_interval_us + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 0 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 46);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - iso_interval_us);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 0;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_ERRORS,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_cont(&testdata[testdata_indx],
							 (testdata_size - testdata_indx),
							 &rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[2] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;
	pdu_timestamp += iso_interval_us;

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, true);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size = 13;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[3]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from three segmented PDUs with errors
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_single_sdu_pdu_err2)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 46);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_ERRORS,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_cont(&testdata[testdata_indx],
							 (testdata_size - testdata_indx),
							 &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[2] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size = 13;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[3]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from three segmented PDUs with errors
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_single_sdu_pdu_err3)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 46);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size += 10;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_cont(&testdata[testdata_indx],
							 (testdata_size - testdata_indx),
							 &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[1]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_ERRORS,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[2] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size = 13;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[3]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from three segmented PDUs with errors
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_single_sdu_seq_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 46);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	/* PDU not transferred to the ISO-AL */
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[2] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size = 13;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[3]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from three segmented PDUs with errors
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_single_sdu_pdu_seq_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 46);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	/* PDU not transferred to the ISO-AL */
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_ERRORS,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[2] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);

	payload_number++;

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn++;
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size = 13;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[3]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of two SDUs from three segmented PDUs
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu_pdu_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[2];
	struct isoal_sdu_buffer sdu_buffer[2];
	isoal_sdu_status_t collated_status[2];
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[2];
	uint16_t total_sdu_size[2];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[2];
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[63];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_test_data_buffer(testdata, 63);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	/* PDU will have errors. Time stamp is only an approximation */
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - iso_interval_us);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 0;
	total_sdu_size[0] = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status[0] = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_ERRORS,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* SDU 1 -------------------------------------------------------------*/
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(0);

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size[0],                  /* Total size */
				    collated_status[0]);                /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[0] += 10;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	testdata_indx = testdata_size;
	testdata_size += 17;
	sdu_size[1] = 17;

	pdu_data_loc[2] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* SDU 1 -------------------------------------------------------------*/
	/* SDU payload should not be written */

	/* SDU shold not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(1);

	/* SDU 2 -------------------------------------------------------------*/
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[1], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[2]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[1] += 10;
	total_sdu_size[1] = COLLATED_RX_SDU_INFO(sdu_size[1], sdu_size[1]);
	collated_status[1] = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* SDU 2 -------------------------------------------------------------*/
	seqn++;
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(2);

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[1], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[3]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[1],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size[1],                  /* Total size */
				    collated_status[1]);                /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));

	/* SDU 3 -------------------------------------------------------------*/
	/* Missing */
	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	sdu_size[1] = 0;
	total_sdu_size[1] = COLLATED_RX_SDU_INFO(sdu_size[1], sdu_size[1]);
	collated_status[1] =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);

	payload_number++;
	pdu_timestamp = 9249 + iso_interval_us;
	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, true);
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size[0] = 13;
	total_sdu_size[0] = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status[0] = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[4] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* SDU 3 -------------------------------------------------------------*/
	seqn++;
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[2],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[2],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[1],                        /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size[1],                  /* Total size */
				    collated_status[1]);                /* SDU status */

	/* SDU 4 -------------------------------------------------------------*/
	seqn++;
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[4]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size[0],                  /* Total size */
				    collated_status[0]);                /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests handling errors in the middle of a stream, where the errors occur
 * across the end of one SDU and into the start of the next SDU.
 *
 * Expected Sequence:
 * -- Total of 4 SDUs released across 2 events
 * -- Event 1: PDU0 Valid  SDU 0 Framed Start
 *             PDU1 Errors SDU 0 Framed End
 *                         SDU 1 Framed Start
 *                  -----> SDU 0 Bit Errors (Released)
 *             PDU2 Valid  SDU 1 Framed End
 *             Missing     SDU 2
 *    Event 2: PDU3 Valid  SDU 3 Framed Single
 *                  -----> SDU 1 Lost       (Released)
 *                  -----> SDU 2 Lost       (Released)
 *                  -----> SDU 3 Valid      (Released)
 *             PDU4 N/A
 *             PDU5 N/A
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu_pdu_err2)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[3];
	struct isoal_sdu_buffer sdu_buffer[3];
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[2];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[3];
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[63];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 0 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[2]);
	init_test_data_buffer(testdata, 63);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[2].dbuf = &rx_sdu_frag_buf[2];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[2].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	/* PDU with errors */
	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_ERRORS,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	testdata_indx = testdata_size;
	testdata_size += 17;
	sdu_size[1] = 0;

	pdu_data_loc[2] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 1 -------------------------------------------------------------*/
	/* A new SDU should not be allocated */

	/* SDU payload should not be written */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */

	/* SDU should not be written to */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* SDU 2 -------------------------------------------------------------*/
	/* Missing */
	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[2] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);

	payload_number++;
	pdu_timestamp = 9249 + (iso_interval_int * ISO_INT_UNIT_US);

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, true);
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size[0] = 13;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[4] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[2]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[1],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[1],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 2 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[2],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[2],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[2],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[2].dbuf,                 /* Buffer */
				    sdu_buffer[2].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 3 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[4]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests handling errors in the middle of a stream, where the errors occur
 * at the end of the SDU.
 *
 * Expected Sequence:
 * -- Total of 4 SDUs released across 2 events
 * -- Event 1: PDU0 Valid  SDU 0 Framed Start
 *             PDU1 Valid  SDU 0 Framed End
 *                         SDU 1 Framed Start
 *                  -----> SDU 0 Valid      (Released)
 *             PDU2 Errors SDU 1 Framed End
 *                  -----> SDU 1 Bit Errors (Released)
 *             Missing     SDU 2
 *    Event 2: PDU3 Valid  SDU 3 Framed Single
 *                  -----> SDU 2 Lost       (Released)
 *                  -----> SDU 3 Valid      (Released)
 *             PDU4 N/A
 *             PDU5 N/A
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu_pdu_err3)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[2];
	struct isoal_sdu_buffer sdu_buffer[2];
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[2];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[2];
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[63];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 0 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_test_data_buffer(testdata, 63);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[0] += 10;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	testdata_indx = testdata_size;
	testdata_size += 17;
	sdu_size[1] = 17;

	pdu_data_loc[2] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(history[1],
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[1]],
				     /* PDU payload */
				     10); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 1 -------------------------------------------------------------*/
	seqn++;
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[1],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[1], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[2]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	/* SDU size does not change */
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[1], sdu_size[1]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_ERRORS, ISOAL_SDU_STATUS_ERRORS);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_ERRORS,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[1],                        /* Frag size */
				    ISOAL_SDU_STATUS_ERRORS,            /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* SDU 2 -------------------------------------------------------------*/
	/* Missing */
	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);

	payload_number++;
	pdu_timestamp = 9249 + (iso_interval_int * ISO_INT_UNIT_US);

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, true);
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size[0] = 13;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[4] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 2 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[2],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[2],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 3 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[4]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests handling missing PDU (sequence) errors in the middle of a stream, where
 * the errors occur across the end of one SDU and into the start of the next
 * SDU.
 *
 * Expected Sequence:
 * -- Total of 4 SDUs released across 2 events
 * -- Event 1: PDU0 Valid   SDU 0 Framed Start
 *             PDU1 Missing SDU 0 Framed End
 *                          SDU 1 Framed Start
 *                  ----->  SDU 0 Valid      (Released)
 *             PDU2 Valid   SDU 1 Framed End
 *             Missing      SDU 2
 *    Event 2: PDU3 Valid   SDU 3 Framed Single
 *                  ----->  SDU 1 Lost       (Released)
 *                  ----->  SDU 2 Lost       (Released)
 *                  ----->  SDU 3 Valid      (Released)
 *             PDU4 N/A
 *             PDU5 N/A
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu_seq_err1)
{
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[3];
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct isoal_sdu_buffer sdu_buffer[3];
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	isoal_sdu_len_t sdu_size[2];
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[3];
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[63];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 0 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[2]);
	init_test_data_buffer(testdata, 63);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[2].dbuf = &rx_sdu_frag_buf[2];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[2].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	/* No change in SDU 1 size */

	/* SDU 1 -------------------------------------------------------------*/
	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);

	testdata_indx = testdata_size;
	testdata_size += 17;
	sdu_size[1] = 0;

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	/* SDU size does not change */
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(1);

	/* SDU payload should not be written */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(1);

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 1 -------------------------------------------------------------*/
	/* Lost */

	/* SDU 2 -------------------------------------------------------------*/
	/* Missing */
	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[2] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);

	payload_number++;
	pdu_timestamp = 9249 + (iso_interval_int * ISO_INT_UNIT_US);

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, true);
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size[0] = 13;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[4] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[2]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[1],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[1],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 2 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[2],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[2],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[2],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[2].dbuf,                 /* Buffer */
				    sdu_buffer[2].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 3 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[4]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests handling missing PDU (sequence) errors followed by bit errors in the
 * middle of a stream, where the errors occur across the end of one SDU and
 * into the start of the next SDU.
 *
 * Expected Sequence:
 * -- Total of 4 SDUs released across 2 events
 * -- Event 1: PDU0 Valid   SDU 0 Framed Start
 *             PDU1 Missing SDU 0 Framed End
 *                          SDU 1 Framed Start
 *                  ----->  SDU 0 Lost       (Released)
 *             PDU2 Errors  SDU 1 Framed End
 *             Missing      SDU 2
 *    Event 2: PDU3 Valid   SDU 3 Framed Single
 *                  ----->  SDU 1 Lost       (Released)
 *                  ----->  SDU 2 Lost       (Released)
 *                  ----->  SDU 3 Valid      (Released)
 *             PDU4 N/A
 *             PDU5 N/A
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu_pdu_seq_err1)
{
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[3];
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct isoal_sdu_buffer sdu_buffer[3];
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[2];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[3];
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[63];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[2]);
	init_test_data_buffer(testdata, 63);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[2].dbuf = &rx_sdu_frag_buf[2];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[2].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	/* No change in SDU 0 size */

	/* SDU 1 -------------------------------------------------------------*/
	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);

	testdata_indx = testdata_size;
	testdata_size += 17;
	sdu_size[1] = 0;

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	/* SDU size does not change */
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_ERRORS,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(1);

	/* SDU payload should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(1);

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 1 -------------------------------------------------------------*/
	/* Lost */

	/* SDU 2 -------------------------------------------------------------*/
	/* Missing */
	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[2] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);

	payload_number++;
	pdu_timestamp = 9249 + (iso_interval_int * ISO_INT_UNIT_US);

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, true);
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size[0] = 13;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[4] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[2]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[1],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[1],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 2 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[2],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[2],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[2],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[2].dbuf,                 /* Buffer */
				    sdu_buffer[2].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 3 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[4]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU which is
 * invalid as it contains multiple segments from the same SDU.
 */
ZTEST(test_rx_framed, test_rx_framed_single_invalid_pdu_single_sdu)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * ISO_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 25);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	testdata_indx = testdata_size;
	testdata_size += 5;
	sdu_size += 5;

	pdu_data_loc[1] = isoal_test_add_framed_pdu_cont(&testdata[testdata_indx],
							 (testdata_size - testdata_indx),
							 &rx_pdu_meta_buf.pdu_meta);

	testdata_indx = testdata_size;
	testdata_size += 7;
	sdu_size += 7;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	pdu_data_loc[2] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(history[0],
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     13); /* Size */

	ZASSERT_ISOAL_SDU_WRITE_TEST(history[1],
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[1]],
				     /* PDU payload */
				     5); /* Size */

	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[2]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of a single SDU from a single segmented PDU which is
 * invalid as it contains multiple segments from the same SDU with incorrect
 * header info
 */
ZTEST(test_rx_framed, test_rx_framed_single_invalid_pdu_single_sdu_hdr_err)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf;
	struct isoal_sdu_buffer sdu_buffer;
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size;
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint64_t payload_number;
	uint16_t total_sdu_size;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint32_t sdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[21];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = ((iso_interval_int * ISO_INT_UNIT_US) / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * ISO_INT_UNIT_US) - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf);
	init_test_data_buffer(testdata, 21);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer.dbuf = &rx_sdu_frag_buf;
	sdu_buffer.size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 3;
	sdu_size = 3;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_cont(&testdata[testdata_indx],
							 (testdata_size - testdata_indx),
							 &rx_pdu_meta_buf.pdu_meta);

	testdata_indx = testdata_size;
	testdata_size += 4;
	sdu_size += 4;

	pdu_data_loc[1] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	testdata_indx = testdata_size;
	testdata_size += 4;
	sdu_size += 4;

	pdu_data_loc[2] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	testdata_indx = testdata_size;
	testdata_size += 4;
	sdu_size += 4;

	pdu_data_loc[3] = isoal_test_add_framed_pdu_cont(&testdata[testdata_indx],
							 (testdata_size - testdata_indx),
							 &rx_pdu_meta_buf.pdu_meta);

	testdata_indx = testdata_size;
	testdata_size += 6;
	sdu_size += 6;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size, sdu_size);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	pdu_data_loc[4] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_ERR_UNSPECIFIED, "err = 0x%02x", err);

	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(history[0],
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     3); /* Size */

	ZASSERT_ISOAL_SDU_WRITE_TEST(history[1],
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[1]],
				     /* PDU payload */
				     4); /* Size */

	ZASSERT_ISOAL_SDU_WRITE_TEST(history[2],
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[2]],
				     /* PDU payload */
				     4); /* Size */

	ZASSERT_ISOAL_SDU_WRITE_TEST(history[3],
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[3]],
				     /* PDU payload */
				     4); /* Size */

	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf, /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[4]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size,                           /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp,                      /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer.dbuf,                    /* Buffer */
				    sdu_buffer.size,                    /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests handling an error in the length of an SDU seqment where the CRC check
 * does not fail.
 *
 * Expected Sequence:
 * -- Total of 2 SDUs released across 1 event
 * -- Event 1: PDU0 valid   SDU 0 Framed Start (Seg Error)
 *                  ----->  SDU 0 Lost         (Released)
 *             PDU1 Valid   SDU 0 Framed End
 *                          SDU 1 Framed Start
 *             PDU2 Valid   SDU 1 Framed End
 *                  ----->  SDU 1 Valid        (Released)
 *
 * Qualification:
 * IAL/CIS/FRA/PER/BI-01-C
 * IAL/CIS/FRA/PER/BI-02-C
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu_seg_err1)
{
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[2];
	struct isoal_sdu_buffer sdu_buffer[2];
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	isoal_sdu_len_t sdu_size[2];
	uint8_t iso_interval_int;
	uint32_t sdu_timestamp[2];
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	isoal_sdu_cnt_t seqn[2];
	uint64_t payload_number;
	uint16_t total_sdu_size;
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
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 0 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_test_data_buffer(testdata, 63);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - iso_interval_us);
	seqn[0] = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 0;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	/* PDU with errors */
	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set an invalid length and incomplete header */
	rx_pdu_meta_buf.pdu_meta.pdu->len = 3;

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn[0],                            /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[0] += 10;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn[1] = seqn[0] + 1;
	testdata_indx = testdata_size;
	testdata_size += 17;
	sdu_size[1] = 17;

	pdu_data_loc[2] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */

	/* SDU payload should not be written */

	/* SDU should not be emitted */

	/* SDU 1 -------------------------------------------------------------*/
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[1], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[2]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(1);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[1] += 10;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[1], sdu_size[1]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should not be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(2);

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[1], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[3]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[1],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn[1],                            /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}

/**
 * Test Suite  :   RX framed PDU recombination
 *
 * Tests recombination of two SDUs from three segmented PDUs where there is a
 * length error in the second SDU's segments that does not cause the CRC to
 * fail.
 *
 * Expected Sequence:
 * -- Total of 4 SDUs released across 2 event
 * -- Event 1: PDU0 valid   SDU 0 Framed Start
 *             PDU1 Valid   SDU 0 Framed End
 *                          SDU 1 Framed Start (Seg Error)
 *                  ----->  SDU 0 Valid        (Released)
 *                  ----->  SDU 1 Lost         (Released)
 *             PDU2 Valid   SDU 1 Framed End
 *             Missing      SDU 2
 * -- Event 2: PDU3 valid   SDU 3 Framed Single
 *                  ----->  SDU 2 Lost         (Released)
 *                  ----->  SDU 3 Valid        (Released)
 *             PDU4 N/A
 *             PDU5 N/A
 *
 * Qualification:
 * IAL/CIS/FRA/PER/BI-01-C
 * IAL/CIS/FRA/PER/BI-02-C
 */
ZTEST(test_rx_framed, test_rx_framed_trppl_pdu_dbl_sdu_seg_err2)
{
	struct rx_sdu_frag_buffer rx_sdu_frag_buf[2];
	struct rx_pdu_meta_buffer rx_pdu_meta_buf;
	struct isoal_sdu_buffer sdu_buffer[2];
	isoal_sdu_status_t collated_status;
	isoal_sink_handle_t sink_hdl;
	isoal_sdu_len_t sdu_size[2];
	uint32_t stream_sync_delay;
	uint32_t group_sync_delay;
	uint32_t sdu_timestamp[2];
	uint8_t iso_interval_int;
	uint16_t pdu_data_loc[5];
	uint32_t iso_interval_us;
	uint64_t payload_number;
	uint16_t total_sdu_size;
	uint32_t sdu_timeoffset;
	uint32_t pdu_timestamp;
	uint16_t testdata_indx;
	uint16_t testdata_size;
	uint32_t sdu_interval;
	isoal_sdu_cnt_t seqn;
	uint8_t testdata[63];
	isoal_status_t err;
	uint32_t latency;
	uint8_t role;
	uint8_t BN;
	uint8_t FT;

	/* Settings */
	role = ISOAL_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	iso_interval_us = iso_interval_int * ISO_INT_UNIT_US;
	sdu_interval = (iso_interval_us / 3) + 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = iso_interval_us - 200;
	group_sync_delay = iso_interval_us - 50;

	/* PDU 1 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[1]);
	init_test_data_buffer(testdata, 63);
	memset(pdu_data_loc, 0, sizeof(pdu_data_loc));

	sdu_buffer[0].dbuf = &rx_sdu_frag_buf[0];
	sdu_buffer[1].dbuf = &rx_sdu_frag_buf[1];
	sdu_buffer[0].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	sdu_buffer[1].size = TEST_RX_SDU_FRAG_PAYLOAD_MAX;
	payload_number = 1000 * BN;
	pdu_timestamp = 9249;
	latency = calc_rx_latency_by_role(role,
					  true,
					  FT,
					  sdu_interval,
					  iso_interval_int,
					  stream_sync_delay,
					  group_sync_delay);
	sdu_timeoffset = group_sync_delay - 50;
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);
	seqn = 0;
	testdata_indx = 0;
	testdata_size = 13;
	sdu_size[0] = 13;

	sink_hdl = basic_rx_test_setup(0xADAD,            /* Handle */
				       role,              /* Role */
				       true,              /* Framed */
				       BN,                /* BN */
				       FT,                /* FT */
				       sdu_interval,      /* SDU Interval */
				       iso_interval_int,  /* ISO Interval */
				       stream_sync_delay, /* Stream Sync Delay */
				       group_sync_delay); /* Group Sync Delay */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[0] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[0]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST_CALL_COUNT(0);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_CONTINUE,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_CONTINUE));

	/* PDU 2 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	sdu_size[0] += 10;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[1] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = sdu_timestamp[0] + sdu_interval;
	testdata_indx = testdata_size;
	testdata_size += 17;
	sdu_size[1] = 0;

	pdu_data_loc[2] = isoal_test_add_framed_pdu_start(&testdata[testdata_indx],
							  (testdata_size - testdata_indx),
							  sdu_timeoffset,
							  &rx_pdu_meta_buf.pdu_meta);

	/* Set an invalid length */
	rx_pdu_meta_buf.pdu_meta.pdu->len -= 5;

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 0 -------------------------------------------------------------*/
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);
	/* Test recombine (Black Box) */

	/* A new SDU should not be allocated */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[1]],
				     /* PDU payload */
				     10); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[0],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 1 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[1], sdu_size[1]);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should not be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(2);

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[1],                        /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 3 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);

	payload_number++;
	testdata_indx = testdata_size;
	testdata_size += 10;
	/* SDU size does not change */

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[3] = isoal_test_add_framed_pdu_end(&testdata[testdata_indx],
							(testdata_size - testdata_indx),
							&rx_pdu_meta_buf.pdu_meta);

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 2 -------------------------------------------------------------*/
	/* Missing */
	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, false);
	sdu_timestamp[1] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);

	/* SDU 1 -------------------------------------------------------------*/
	/* Test recombine (Black Box) */
	/* Should not allocate a new SDU */
	ZASSERT_ISOAL_SDU_ALLOC_TEST_CALL_COUNT(2);

	/* SDU should not be written to */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(2);

	/* SDU should not be emitted */
	ZASSERT_ISOAL_SDU_WRITE_TEST_CALL_COUNT(2);

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_ERR_SPOOL,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_ERR_SPOOL));

	/* PDU 4 -------------------------------------------------------------*/
	isoal_test_init_rx_pdu_buffer(&rx_pdu_meta_buf);
	isoal_test_init_rx_sdu_buffer(&rx_sdu_frag_buf[0]);

	payload_number++;
	pdu_timestamp = 9249 + (iso_interval_int * ISO_INT_UNIT_US);

	sdu_timeoffset = get_next_time_offset(sdu_timeoffset, iso_interval_us, sdu_interval, true);
	sdu_timestamp[0] = (uint32_t)((int64_t)pdu_timestamp + latency - sdu_timeoffset);

	testdata_indx = testdata_size;
	testdata_size += 13;
	sdu_size[0] = 13;

	isoal_test_create_framed_pdu_base(payload_number,
					  pdu_timestamp,
					  ISOAL_PDU_STATUS_VALID,
					  &rx_pdu_meta_buf.pdu_meta);
	pdu_data_loc[4] = isoal_test_add_framed_pdu_single(&testdata[testdata_indx],
							   (testdata_size - testdata_indx),
							   sdu_timeoffset,
							   &rx_pdu_meta_buf.pdu_meta);

	/* Set callback function return values */
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[1]);
	push_custom_sink_sdu_alloc_test_output_buffer(&sdu_buffer[0]);
	sink_sdu_alloc_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_write_test_fake.return_val = ISOAL_STATUS_OK;
	sink_sdu_emit_test_fake.return_val = ISOAL_STATUS_OK;

	err = isoal_rx_pdu_recombine(sink_hdl, &rx_pdu_meta_buf.pdu_meta);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 2 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(0, 0);
	collated_status =
		COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_LOST_DATA, ISOAL_SDU_STATUS_LOST_DATA);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(history[2],
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(history[2],
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    0,                                  /* Frag size */
				    ISOAL_SDU_STATUS_LOST_DATA,         /* Frag status */
				    sdu_timestamp[1],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[1].dbuf,                 /* Buffer */
				    sdu_buffer[1].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* SDU 3 -------------------------------------------------------------*/
	seqn++;
	total_sdu_size = COLLATED_RX_SDU_INFO(sdu_size[0], sdu_size[0]);
	collated_status = COLLATED_RX_SDU_INFO(ISOAL_SDU_STATUS_VALID, ISOAL_SDU_STATUS_VALID);
	/* Test recombine (Black Box) */
	/* A new SDU should be allocated */
	ZASSERT_ISOAL_SDU_ALLOC_TEST(val,
				     &isoal_global.sink_state[sink_hdl], /* Sink */
				     &rx_pdu_meta_buf.pdu_meta);         /* PDU */

	/* SDU payload should be written */
	ZASSERT_ISOAL_SDU_WRITE_TEST(val,
				     &rx_sdu_frag_buf[0], /* SDU buffer */
				     &rx_pdu_meta_buf.pdu[3 + pdu_data_loc[4]],
				     /* PDU payload */
				     (testdata_size - testdata_indx)); /* Size */

	/* SDU should be emitted */
	ZASSERT_ISOAL_SDU_EMIT_TEST(val,
				    &isoal_global.sink_state[sink_hdl], /* Sink */
				    BT_ISO_SINGLE,                      /* Frag state */
				    sdu_size[0],                        /* Frag size */
				    ISOAL_SDU_STATUS_VALID,             /* Frag status */
				    sdu_timestamp[0],                   /* Timestamp */
				    seqn,                               /* Seq. number */
				    sdu_buffer[0].dbuf,                 /* Buffer */
				    sdu_buffer[0].size,                 /* Buffer size */
				    total_sdu_size,                     /* Total size */
				    collated_status);                   /* SDU status */

	/* Test recombine (White Box) */
	zassert_equal(isoal_global.sink_state[sink_hdl].sdu_production.fsm,
		      ISOAL_START,
		      "FSM state %s should be %s!",
		      FSM_TO_STR(isoal_global.sink_state[sink_hdl].sdu_production.fsm),
		      FSM_TO_STR(ISOAL_START));
}
