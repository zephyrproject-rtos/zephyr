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

/**
 * Test allocation of a new PDU
 * @param[in]  pdu_buffer Buffer to store PDU details in
 * @return     Error status of operation
 */
static isoal_status_t source_pdu_alloc_test(struct isoal_pdu_buffer *pdu_buffer)
{
	isoal_test_debug_trace_func_call(__func__, "IN");

	zassert_not_null(pdu_buffer, "");
	/* Return SDU buffer details as provided by the test */
	ztest_copy_return_data(pdu_buffer, sizeof(*pdu_buffer));

	return ztest_get_return_value();
}

/**
 * Test writing the given SDU payload to the target PDU buffer at the given
 * offset.
 * @param[in,out]  pdu_buffer  Target PDU buffer
 * @param[in]      pdu_offset  Offset / current write position within PDU
 * @param[in]      sdu_payload Location of source data
 * @param[in]      consume_len Length of data to copy
 * @return         Error status of write operation
 */
static isoal_status_t source_pdu_write_test(struct isoal_pdu_buffer *pdu_buffer,
					    const size_t pdu_offset, const uint8_t *sdu_payload,
					    const size_t consume_len)
{
	isoal_test_debug_trace_func_call(__func__, "IN");

#if defined(DEBUG_TEST)
	zassert_not_null(pdu_buffer, "");
	zassert_not_null(sdu_payload, "");

	zassert_false((pdu_offset + consume_len) > pdu_buffer->size,
		      "Write size of %d exceeds buffer!", consume_len);

	/* Copy source to destination at given offset */
	memcpy(&pdu_buffer->pdu->payload[pdu_offset], sdu_payload, consume_len);
#endif

	ztest_check_expected_data(pdu_buffer, sizeof(*pdu_buffer));
	ztest_check_expected_value(pdu_offset);
	ztest_check_expected_value(consume_len);
	ztest_check_expected_data(sdu_payload, consume_len);

	return ztest_get_return_value();
}

/**
 * Emit the encoded node to the transmission queue
 * @param node_tx TX node to enqueue
 * @param handle  CIS/BIS handle
 * @return        Error status of enqueue operation
 */
static isoal_status_t source_pdu_emit_test(struct node_tx_iso *node_tx, const uint16_t handle)
{
	isoal_test_debug_trace_func_call(__func__, "IN");

	struct pdu_iso *pdu;

	isoal_test_debug_print_tx_pdu(node_tx);

	ztest_check_expected_value(node_tx);
	ztest_check_expected_value(node_tx->payload_count);
	ztest_check_expected_value(node_tx->sdu_fragments);

	pdu = (struct pdu_iso *)node_tx->pdu;
	ztest_check_expected_value(pdu->ll_id);
	ztest_check_expected_value(pdu->length);

	ztest_check_expected_value(handle);

	return ztest_get_return_value();
}

/**
 * Test releasing the given payload back to the memory pool.
 * @param node_tx TX node to release or forward
 * @param handle  CIS/BIS handle
 * @param status  Reason for release
 * @return        Error status of release operation
 */
static isoal_status_t source_pdu_release_test(struct node_tx_iso *node_tx, const uint16_t handle,
					      const isoal_status_t status)
{
	isoal_test_debug_trace_func_call(__func__, "IN");

	ztest_check_expected_value(node_tx);
	ztest_check_expected_value(handle);
	ztest_check_expected_value(status);

	return ztest_get_return_value();
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
static isoal_source_handle_t basic_tx_test_setup(uint16_t handle, uint8_t role, uint8_t framed,
						 uint8_t burst_number, uint8_t flush_timeout,
						 uint8_t max_octets, uint32_t sdu_interval,
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
	      handle, ROLE_TO_STR(role), framed ? "Framed" : "Unframed", burst_number,
	      flush_timeout, max_octets, (iso_interval_int * CONN_INT_UNIT_US), sdu_interval,
	      stream_sync_delay, group_sync_delay);
#endif

	ztest_set_assert_valid(false);

	err = isoal_init();
	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	err = isoal_reset();
	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Create a source based on global parameters */
	err = isoal_source_create(handle, role, framed, burst_number, flush_timeout, max_octets,
				  sdu_interval, iso_interval_int, stream_sync_delay,
				  group_sync_delay, source_pdu_alloc_test, source_pdu_write_test,
				  source_pdu_emit_test, source_pdu_release_test, &source_hdl);
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
static void isoal_test_create_sdu_fagment(uint8_t sdu_state, uint8_t *dataptr, uint16_t length,
					  uint16_t sdu_total_length, uint16_t packet_number,
					  uint32_t timestamp, uint32_t ref_point,
					  uint64_t target_event, struct isoal_sdu_tx *sdu_tx)
{
	sdu_tx->sdu_state = sdu_state;
	sdu_tx->packet_sn = packet_number;
	sdu_tx->iso_sdu_length = sdu_total_length;
	sdu_tx->time_stamp = timestamp;
	sdu_tx->grp_ref_point = ref_point;
	sdu_tx->target_event = target_event;
	memcpy(sdu_tx->dbuf, dataptr, length);
	sdu_tx->size = length;

	isoal_test_debug_print_tx_sdu(sdu_tx);
}

/**
 * Test Suite  :   TX basic test
 *
 * Test creating and destroying sources upto the maximum with randomized
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
		iso_interval = iso_interval_int * CONN_INT_UNIT_US;
		sdu_interval = sdu_interval_int * CONN_INT_UNIT_US;
		stream_sync_delay = iso_interval - 200;
		group_sync_delay = iso_interval - 50;

		ztest_set_assert_valid(false);

		for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SOURCES; i++) {
			res = ISOAL_STATUS_ERR_UNSPECIFIED;
			source_hdl[i] = 0xFF;

			pdus_per_sdu = (burst_number * sdu_interval) / iso_interval;

			res = isoal_source_create(
				handle, role, framed, burst_number, flush_timeout, max_octets,
				sdu_interval, iso_interval_int, stream_sync_delay, group_sync_delay,
				source_pdu_alloc_test, source_pdu_write_test, source_pdu_emit_test,
				source_pdu_release_test, &source_hdl[i]);

			zassert_equal(isoal_global.source_allocated[source_hdl[i]],
				      ISOAL_ALLOC_STATE_TAKEN, "");

			zassert_equal(isoal_global.source_state[source_hdl[i]].session.pdus_per_sdu,
				      pdus_per_sdu, "%s pdus_per_sdu %d should be %d for:\n\tBN %d"
						    "\n\tFT %d\n\tISO Interval %dus"
						    "\n\tSDU Interval %dus"
						    "\n\tStream Sync Delay %dus"
						    "\n\tGroup Sync Delay %dus",
				      (framed ? "Framed" : "Unframed"),
				      isoal_global.source_state[source_hdl[i]].session.pdus_per_sdu,
				      pdus_per_sdu, burst_number, flush_timeout, iso_interval,
				      sdu_interval, stream_sync_delay, group_sync_delay);

			zassert_equal(res, ISOAL_STATUS_OK, "Source %d in role %s creation failed!",
				      i, ROLE_TO_STR(role));

			isoal_source_enable(source_hdl[i]);

			zassert_equal(isoal_global.source_state[source_hdl[i]].pdu_production.mode,
				      ISOAL_PRODUCTION_MODE_ENABLED,
				      "Source %d in role %s enable failed!", i, ROLE_TO_STR(role));

			zassert_not_null(isoal_get_source_param_ref(source_hdl[i]), "");

			framed = !framed;
			burst_number++;
			flush_timeout = (flush_timeout % 3) + 1;
			max_octets += max_octets / 2;
			sdu_interval_int++;
			iso_interval_int = iso_interval_int * sdu_interval_int;
			sdu_interval = (sdu_interval_int * CONN_INT_UNIT_US) - (framed ? 100 : 0);
			iso_interval = iso_interval_int * CONN_INT_UNIT_US;
			stream_sync_delay = iso_interval - (200 * i);
			group_sync_delay = iso_interval - 50;
		}

		/* Destroy in order */
		for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SOURCES; i++) {
			isoal_source_destroy(source_hdl[i]);

			zassert_equal(isoal_global.source_allocated[source_hdl[i]],
				      ISOAL_ALLOC_STATE_FREE, "Source destruction failed!");

			zassert_equal(isoal_global.source_state[source_hdl[i]].pdu_production.mode,
				      ISOAL_PRODUCTION_MODE_DISABLED, "Source disable failed!");
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
	role = BT_CONN_ROLE_PERIPHERAL;
	burst_number = 1;
	max_octets = 40;
	flush_timeout = 1;
	framed = false;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	stream_sync_delay = CONN_INT_UNIT_US - 200;
	group_sync_delay = CONN_INT_UNIT_US - 50;

	res = isoal_init();
	zassert_equal(res, ISOAL_STATUS_OK, "res = 0x%02x", res);

	res = isoal_reset();
	zassert_equal(res, ISOAL_STATUS_OK, "res = 0x%02x", res);

	for (int i = 0; i < CONFIG_BT_CTLR_ISOAL_SOURCES; i++) {
		res = isoal_source_create(
			handle, role, framed, burst_number, flush_timeout, max_octets, sdu_interval,
			iso_interval_int, stream_sync_delay, group_sync_delay,
			source_pdu_alloc_test, source_pdu_write_test, source_pdu_emit_test,
			source_pdu_release_test, &source_hdl[i]);

		zassert_equal(res, ISOAL_STATUS_OK, "Source %d in role %s creation failed!", i,
			      ROLE_TO_STR(role));
	}

	res = isoal_source_create(handle, role, framed, burst_number, flush_timeout, max_octets,
				  sdu_interval, iso_interval_int, stream_sync_delay,
				  group_sync_delay, source_pdu_alloc_test, source_pdu_write_test,
				  source_pdu_emit_test, source_pdu_release_test,
				  &source_hdl[CONFIG_BT_CTLR_ISOAL_SOURCES]);

	zassert_equal(res, ISOAL_STATUS_ERR_SOURCE_ALLOC,
		      "Source creation did not return error as expected!");
}

/**
 * Test Suite  :   TX basic test
 *
 * Test assertion when attempting to retrieve source params for an invalid source
 * handle.
 */
ZTEST(test_rx_basics, test_source_invalid_ref)
{
	ztest_set_assert_valid(true);

	isoal_get_source_param_ref(99);

	ztest_set_assert_valid(false);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is less than the PDU buffer size
 */
ZTEST(test_tx_unframed, test_tx_unframed_1_sdu_1_frag_1_pdu_maxPDU)
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

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
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	testdata_indx = 0;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX - 5;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 false,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_COMPLETE_END);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test PDU release */
	ztest_expect_value(source_pdu_release_test, node_tx, (void *)&tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_release_test, handle,
			   isoal_global.source_state[source_hdl].session.handle);
	ztest_expect_value(source_pdu_release_test, status, ISOAL_STATUS_OK);
	ztest_returns_value(source_pdu_release_test, ISOAL_STATUS_OK);

	isoal_tx_pdu_release(source_hdl, &tx_pdu_meta_buf.node_tx);
}

/**
 * Test Suite  :   TX unframed SDU fragmentation
 *
 * Tests fragmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is greater than the PDU buffer size
 */
ZTEST(test_tx_unframed, test_tx_unframed_1_sdu_1_frag_1_pdu_bufSize)
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

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
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX;
	testdata_indx = 0;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX;
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 false,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_COMPLETE_END);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

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
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = 100;
	testdata_indx = 0;
	testdata_size = 100;
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 false,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = max_octets;
	sdu_fragments = 0;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_START_CONTINUE);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU 2 */
	payload_number++;
	sdu_read_loc += pdu_write_size;
	sdu_fragments = 0;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_START_CONTINUE);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU 3 */
	payload_number++;
	sdu_read_loc += pdu_write_size;
	pdu_write_size = 30;
	sdu_fragments++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_COMPLETE_END);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

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
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX;
	testdata_indx = 0;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX / 3;
	payload_number = event_number * BN;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 false,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX / 3;
	sdu_fragments++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU Frag 2 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += TEST_TX_PDU_PAYLOAD_MAX / 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size += TEST_TX_PDU_PAYLOAD_MAX / 3;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU Frag 3 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX;

	isoal_test_create_sdu_fagment(BT_ISO_END, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_COMPLETE_END);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

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
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX * 2;
	testdata_indx = 0;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 false,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[0]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU Frag 2 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[0].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_START_CONTINUE);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	/* PDU 2 */
	payload_number++;
	sdu_read_loc = pdu_write_size;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc);
	pdu_write_loc = 0;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[1]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU Frag 3 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX * 2;

	isoal_test_create_sdu_fagment(BT_ISO_END, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[1].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_COMPLETE_END);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 2;
	sdu_interval = CONN_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 4;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

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
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX * 2;
	testdata_indx = 0;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 false,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[0]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[0].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_START_CONTINUE);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	/* PDU 2 */
	payload_number++;
	sdu_read_loc = pdu_write_size;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc);
	pdu_write_loc = 0;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[1]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX * 2;

	isoal_test_create_sdu_fagment(BT_ISO_END, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[1].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_COMPLETE_END);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

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

	isoal_test_create_sdu_fagment(BT_ISO_START, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	payload_number++;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[0]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 2 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[0].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_START_CONTINUE);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	/* PDU 4 */
	payload_number++;
	sdu_read_loc = pdu_write_size;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc);
	pdu_write_loc = 0;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[1]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 2 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX * 2;

	isoal_test_create_sdu_fagment(BT_ISO_END, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 4 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[1].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_COMPLETE_END);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 2;
	sdu_interval = CONN_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 8;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

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
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX * 2;
	testdata_indx = 0;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 false,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[0]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[0].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_START_CONTINUE);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	/* PDU 2 */
	payload_number++;
	sdu_read_loc = pdu_write_size;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc);
	pdu_write_loc = 0;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[1]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX * 2;

	isoal_test_create_sdu_fagment(BT_ISO_END, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[1].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_COMPLETE_END);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	/* Padding PDUs */
	/* Padding 1 */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[0]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	/* PDU should not be written to */

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[0].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_START_CONTINUE);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	/* Padding 2 */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[2]);
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[2]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	/* PDU should not be written to */

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[2].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_START_CONTINUE);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

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

	isoal_test_create_sdu_fagment(BT_ISO_START, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	payload_number++;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;
	sdu_fragments++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[0]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 2 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += (TEST_TX_PDU_PAYLOAD_MAX * 2) / 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[0].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_START_CONTINUE);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	/* PDU 4 */
	payload_number++;
	sdu_read_loc = pdu_write_size;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc);
	pdu_write_loc = 0;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[1]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 2 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX * 2;

	isoal_test_create_sdu_fagment(BT_ISO_END, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 4 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[1].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_COMPLETE_END);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	/* Padding PDUs */
	/* Padding 3 */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[0]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	/* PDU should not be written to */

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[0].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_START_CONTINUE);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	/* Padding 4 */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[2]);
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[2]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	/* PDU should not be written to */

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[2].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_START_CONTINUE);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

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
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = 0;
	testdata_indx = 0;
	testdata_size = 0;
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = 0;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 false,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[0]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	/* PDU should not be written to */

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[0].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_COMPLETE_END);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	/* Padding PDUs */
	/* Padding 1 */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[1]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	/* PDU should not be written to */

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[1].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_START_CONTINUE);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	/* Padding 2 */
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[2]);
	payload_number++;
	pdu_write_size = 0;
	sdu_fragments = 0;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[2]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	/* PDU should not be written to */

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[2].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_START_CONTINUE);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

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
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	testdata_indx = 0;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = 0;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 false,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_ERR_PDU_ALLOC);

	/* PDU should not be written to */

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	ztest_set_assert_valid(true);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	ztest_set_assert_valid(false);

	zassert_equal(err, ISOAL_STATUS_ERR_PDU_ALLOC, "err = 0x%02x", err);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

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
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	testdata_indx = 0;
	testdata_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	payload_number = event_number * BN;
	pdu_write_loc = 0;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX - 5;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 false,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test fragmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_COMPLETE_END);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_ERR_PDU_EMIT);

	ztest_expect_value(source_pdu_release_test, node_tx, (void *)&tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_release_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_expect_value(source_pdu_release_test, status, ISOAL_STATUS_ERR_PDU_EMIT);
	ztest_returns_value(source_pdu_release_test, ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_ERR_PDU_EMIT, "err = 0x%02x", err);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      TEST_TX_PDU_PAYLOAD_MAX - 5 -
				      (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 true,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test PDU release */
	ztest_expect_value(source_pdu_release_test, node_tx, (void *)&tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_release_test, handle,
			   isoal_global.source_state[source_hdl].session.handle);
	ztest_expect_value(source_pdu_release_test, status, ISOAL_STATUS_OK);
	ztest_returns_value(source_pdu_release_test, ISOAL_STATUS_OK);

	isoal_tx_pdu_release(source_hdl, &tx_pdu_meta_buf.node_tx);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is greater than the PDU buffer size
 */
ZTEST(test_tx_framed, test_tx_framed_1_sdu_1_frag_1_pdu_bufSize)
{
	uint8_t testdata
		[TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)];
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE +
								   PDU_ISO_SEG_TIMEOFFSET_SIZE));
	memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 true,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment
 * into three PDUs where Max PDU is less than the PDU buffer size
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      100 - ((3 * PDU_ISO_SEG_HDR_SIZE) + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = 100 - ((3 * PDU_ISO_SEG_HDR_SIZE) + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size = 100 - ((3 * PDU_ISO_SEG_HDR_SIZE) + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 true,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = max_octets;
	sdu_fragments = 0;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU 2 */
	payload_number++;
	seg_hdr[2].sc = 1;
	seg_hdr[2].cmplt = 0;
	seg_hdr[2].timeoffset = 0;
	seg_hdr[2].length = 0;
	pdu_hdr_loc = 0;
	sdu_read_loc += (pdu_write_size - pdu_write_loc);
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE;
	pdu_write_size = max_octets;
	sdu_fragments = 0;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[2]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[3] = seg_hdr[2];
	seg_hdr[3].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[3]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU 3 */
	payload_number++;
	seg_hdr[4].sc = 1;
	seg_hdr[4].cmplt = 0;
	seg_hdr[4].timeoffset = 0;
	seg_hdr[4].length = 0;
	pdu_hdr_loc = 0;
	sdu_read_loc += (pdu_write_size - pdu_write_loc);
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE;
	pdu_write_size = sdu_total_size - ((2 * max_octets) - (2 * PDU_ISO_SEG_HDR_SIZE) -
					   PDU_ISO_SEG_TIMEOFFSET_SIZE) +
			 pdu_write_loc;
	sdu_fragments++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[4]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[5] = seg_hdr[4];
	seg_hdr[5].cmplt = 1;
	seg_hdr[5].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[5]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in three fragments
 * into a single PDU where Max PDU is greater than the PDU buffer size
 */
ZTEST(test_tx_framed, test_tx_framed_1_sdu_3_frag_1_pdu)
{
	uint8_t testdata
		[TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)];
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE +
								   PDU_ISO_SEG_TIMEOFFSET_SIZE));
	memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		(TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
		3;
	payload_number = event_number * BN;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 true,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size =
		((TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
		 3) +
		pdu_write_loc;
	sdu_fragments++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU Frag 2 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size +=
		(TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
		3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

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

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU Frag 3 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);

	isoal_test_create_sdu_fagment(BT_ISO_END, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 2;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      (TEST_TX_PDU_PAYLOAD_MAX * 2) -
				      ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	pdu_buffer[0].handle = (void *)&tx_pdu_meta_buf[0].node_tx;
	pdu_buffer[0].pdu = (struct pdu_iso *)tx_pdu_meta_buf[0].node_tx.pdu;
	pdu_buffer[0].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[1].handle = (void *)&tx_pdu_meta_buf[1].node_tx;
	pdu_buffer[1].pdu = (struct pdu_iso *)tx_pdu_meta_buf[1].node_tx.pdu;
	pdu_buffer[1].size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size = ((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			3;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 true,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	payload_number = event_number * BN;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = (((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			   ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			  3) +
			 pdu_write_loc;
	sdu_fragments++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[0]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU Frag 2 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += ((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			  ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[0].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU 2 */
	payload_number++;
	seg_hdr[2].sc = 1;
	seg_hdr[2].cmplt = 0;
	seg_hdr[2].timeoffset = 0;
	seg_hdr[2].length = 0;
	sdu_read_loc = (pdu_write_size - pdu_write_loc) + testdata_indx;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc) +
			 PDU_ISO_SEG_HDR_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[1]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[2]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[3] = seg_hdr[2];
	seg_hdr[3].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[3]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	/* PDU should not be emitted */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU Frag 3 --------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) -
			((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE);

	isoal_test_create_sdu_fagment(BT_ISO_END, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[3].cmplt = 1;
	seg_hdr[3].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[3]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[1].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 2;
	sdu_interval = CONN_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 4;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      (TEST_TX_PDU_PAYLOAD_MAX * 2) -
				      ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	pdu_buffer[0].handle = (void *)&tx_pdu_meta_buf[0].node_tx;
	pdu_buffer[0].pdu = (struct pdu_iso *)tx_pdu_meta_buf[0].node_tx.pdu;
	pdu_buffer[0].size = TEST_TX_PDU_PAYLOAD_MAX;
	pdu_buffer[1].handle = (void *)&tx_pdu_meta_buf[1].node_tx;
	pdu_buffer[1].pdu = (struct pdu_iso *)tx_pdu_meta_buf[1].node_tx.pdu;
	pdu_buffer[1].size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = 9249 + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size = ((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			3;
	sdu_fragments = 0;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 true,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_START, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	payload_number = event_number * BN;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = (((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			   ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			  3) +
			 pdu_write_loc;
	sdu_fragments++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[0]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += ((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			  ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[0].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU 2 */
	payload_number++;
	seg_hdr[2].sc = 1;
	seg_hdr[2].cmplt = 0;
	seg_hdr[2].timeoffset = 0;
	seg_hdr[2].length = 0;
	sdu_read_loc = (pdu_write_size - pdu_write_loc) + testdata_indx;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc) +
			 PDU_ISO_SEG_HDR_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[1]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[2]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[3] = seg_hdr[2];
	seg_hdr[3].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[3]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	/* PDU should not be emitted */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 1 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) -
			((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE);

	isoal_test_create_sdu_fagment(BT_ISO_END, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[3].cmplt = 1;
	seg_hdr[3].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[3]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[1].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 2 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[0]);
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf[1]);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_packet_number++;
	event_number = 2000;
	sdu_timestamp = 9249 + sdu_interval;
	ref_point = 9249 + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size = ((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			 ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			3;
	sdu_fragments = 0;

	isoal_test_create_sdu_fagment(BT_ISO_START, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	payload_number++;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = (((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			   ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			  3) +
			 pdu_write_loc;
	sdu_fragments++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[0]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 2 Frag 2 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx += testdata_size;
	testdata_size += ((TEST_TX_PDU_PAYLOAD_MAX * 2) -
			  ((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE)) /
			 3;

	isoal_test_create_sdu_fagment(BT_ISO_CONT, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 3 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[0]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[0].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU 4 */
	payload_number++;
	seg_hdr[2].sc = 1;
	seg_hdr[2].cmplt = 0;
	seg_hdr[2].timeoffset = 0;
	seg_hdr[2].length = 0;
	sdu_read_loc = (pdu_write_size - pdu_write_loc) + testdata_indx;
	pdu_write_size = testdata_size - testdata_indx - (pdu_write_size - pdu_write_loc) +
			 PDU_ISO_SEG_HDR_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer[1]);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[2]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[3] = seg_hdr[2];
	seg_hdr[3].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[3]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	/* PDU release not expected (No Error) */

	/* PDU should not be emitted */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 2 Frag 3 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	sdu_timestamp += 10;
	testdata_indx = testdata_size;
	testdata_size = (TEST_TX_PDU_PAYLOAD_MAX * 2) -
			((PDU_ISO_SEG_HDR_SIZE * 2) + PDU_ISO_SEG_TIMEOFFSET_SIZE);

	isoal_test_create_sdu_fagment(BT_ISO_END, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 4 */
	pdu_write_loc = pdu_write_size;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_read_loc = testdata_indx;
	sdu_fragments++;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[3].cmplt = 1;
	seg_hdr[3].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer[1]);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[3]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf[1].node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
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
	PRINT("Framed padding not implemented.\n");
	ztest_test_skip();
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
	uint8_t testdata
		[(TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) *
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, (TEST_TX_PDU_PAYLOAD_MAX -
					 (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) *
						2);
	memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 true,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

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

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 2 */
	/* Advance the target event and the reference point to what it should be */
	event_number++;
	ref_point += iso_interval_int * CONN_INT_UNIT_US;
	payload_number++;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
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
	uint8_t testdata
		[(TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) *
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, (TEST_TX_PDU_PAYLOAD_MAX -
					 (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE)) *
						2);
	memset(seg_hdr, 0, sizeof(seg_hdr));
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

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 true,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	/* Advance the target event and the reference point to what it should be */
	event_number++;
	ref_point += iso_interval_int * CONN_INT_UNIT_US;
	payload_number = event_number * BN;
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 3;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, 1);
	memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size = 0;
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 true,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = pdu_write_loc;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be emitted */

	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Send Event Timeout ----------------------------------------------- */
	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));

	isoal_tx_event_prepare(source_hdl, event_number);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      TEST_TX_PDU_PAYLOAD_MAX - 5 -
				      (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = NULL;
	pdu_buffer.pdu = NULL;
	pdu_buffer.size = 0;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 true,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_ERR_PDU_ALLOC);

	/* PDU should not be written to */

	/* PDU should not be emitted */

	/* PDU release not expected (No Emit Error) */

	ztest_set_assert_valid(true);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	ztest_set_assert_valid(false);

	zassert_equal(err, ISOAL_STATUS_ERR_PDU_ALLOC, "err = 0x%02x", err);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 1;
	sdu_interval = CONN_INT_UNIT_US + 50;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX - 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU Frag 1 --------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata,
			      TEST_TX_PDU_PAYLOAD_MAX - 5 -
				      (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE));
	memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 2000;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size =
		TEST_TX_PDU_PAYLOAD_MAX - 5 - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 true,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = TEST_TX_PDU_PAYLOAD_MAX - 5;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_ERR_PDU_EMIT);

	ztest_expect_value(source_pdu_release_test, node_tx, (void *)&tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_release_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_expect_value(source_pdu_release_test, status, ISOAL_STATUS_ERR_PDU_EMIT);
	ztest_returns_value(source_pdu_release_test, ISOAL_STATUS_OK);

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_ERR_PDU_EMIT, "err = 0x%02x", err);
}

/**
 * Test Suite  :   TX framed SDU segmentation
 *
 * Tests segmentation of a single SDU contained in a single fragment
 * into a single PDU where Max PDU is less than the PDU buffer size,
 * where PDU emit fails
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 800;
	sdu_interval = 500000;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, 40);
	memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 0;
	event_number = 2000;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size =
		TEST_TX_PDU_PAYLOAD_MAX - (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	testdata_indx = 0;
	testdata_size = 10;
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 true,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = pdu_write_loc + testdata_size;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be  emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

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

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 - Seg 2 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 10 + PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_write_loc = pdu_hdr_loc + PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = testdata_indx;
	pdu_write_size = pdu_write_loc + 10;
	sdu_fragments++;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 3 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	memset(seg_hdr, 0, sizeof(seg_hdr));
	event_number++;
	sdu_packet_number++;
	sdu_timestamp = sdu_timestamp + sdu_interval;
	ref_point = ref_point + (iso_interval_int * CONN_INT_UNIT_US);
	sdu_total_size = 20;
	testdata_indx = testdata_size;
	testdata_size += 20;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 - Seg 2 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = testdata_indx;
	pdu_write_size = pdu_write_loc + sdu_total_size;
	sdu_fragments = 1;
	payload_number++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU emit not expected */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test PDU release */

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	isoal_tx_event_prepare(source_hdl, event_number);
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
	role = BT_CONN_ROLE_PERIPHERAL;
	iso_interval_int = 800;
	sdu_interval = 500000;
	max_octets = TEST_TX_PDU_PAYLOAD_MAX + 5;
	BN = 1;
	FT = 1;
	stream_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 200;
	group_sync_delay = (iso_interval_int * CONN_INT_UNIT_US) - 50;

	/* SDU 1 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	init_test_data_buffer(testdata, 40);
	memset(seg_hdr, 0, sizeof(seg_hdr));
	pdu_buffer.handle = (void *)&tx_pdu_meta_buf.node_tx;
	pdu_buffer.pdu = (struct pdu_iso *)tx_pdu_meta_buf.node_tx.pdu;
	pdu_buffer.size = TEST_TX_PDU_PAYLOAD_MAX;
	sdu_packet_number = 0;
	event_number = 0;
	sdu_timestamp = 9249;
	ref_point = sdu_timestamp + (iso_interval_int * CONN_INT_UNIT_US) - 50;
	sdu_total_size = 10;
	testdata_indx = 0;
	testdata_size = 10;
	payload_number = event_number * BN;

	source_hdl = basic_tx_test_setup(0xADAD,	    /* Handle */
					 role,		    /* Role */
					 true,		    /* Framed */
					 BN,		    /* BN */
					 FT,		    /* FT */
					 max_octets,	/* max_octets */
					 sdu_interval,      /* SDU Interval */
					 iso_interval_int,  /* ISO Interval */
					 stream_sync_delay, /* Stream Sync Delay */
					 group_sync_delay); /* Group Sync Delay */

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = 0;
	pdu_write_size = pdu_write_loc + testdata_size;
	sdu_fragments = 1;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU should not be  emitted */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* SDU 2 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	memset(seg_hdr, 0, sizeof(seg_hdr));
	sdu_packet_number++;
	sdu_timestamp = sdu_timestamp + sdu_interval;
	sdu_total_size = 10;
	testdata_indx = testdata_size;
	testdata_size += 10;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 - Seg 2 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 10 + PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_write_loc = pdu_hdr_loc + PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = testdata_indx;
	pdu_write_size = pdu_write_loc + 10;
	sdu_fragments++;

	/* PDU should not be allocated */

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU emit not expected */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test PDU release */

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	isoal_tx_event_prepare(source_hdl, event_number);

	/* SDU 3 Frag 1 ------------------------------------------------------*/
	isoal_test_init_tx_pdu_buffer(&tx_pdu_meta_buf);
	isoal_test_init_tx_sdu_buffer(&tx_sdu_frag_buf);
	memset(seg_hdr, 0, sizeof(seg_hdr));
	event_number++;
	sdu_packet_number++;
	sdu_timestamp = sdu_timestamp + sdu_interval;
	ref_point = ref_point + (iso_interval_int * CONN_INT_UNIT_US);
	sdu_total_size = 20;
	testdata_indx = testdata_size;
	testdata_size += 20;

	isoal_test_create_sdu_fagment(BT_ISO_SINGLE, &testdata[testdata_indx],
				      (testdata_size - testdata_indx), sdu_total_size,
				      sdu_packet_number, sdu_timestamp, ref_point, event_number,
				      &tx_sdu_frag_buf.sdu_tx);

	/* Test segmentation (Black Box) */
	/* Valid PDUs */
	/* PDU 1 - Seg 2 */
	seg_hdr[0].sc = 0;
	seg_hdr[0].cmplt = 0;
	seg_hdr[0].timeoffset = ref_point - sdu_timestamp;
	seg_hdr[0].length = PDU_ISO_SEG_TIMEOFFSET_SIZE;
	pdu_hdr_loc = 0;
	pdu_write_loc = PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE;
	sdu_read_loc = testdata_indx;
	pdu_write_size = pdu_write_loc + sdu_total_size;
	sdu_fragments = 1;
	payload_number++;

	ztest_return_data(source_pdu_alloc_test, pdu_buffer, &pdu_buffer);
	ztest_returns_value(source_pdu_alloc_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[0]);
	ztest_expect_value(source_pdu_write_test, consume_len,
			   PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &testdata[sdu_read_loc]);
	ztest_expect_value(source_pdu_write_test, consume_len, (pdu_write_size - pdu_write_loc));
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	seg_hdr[1] = seg_hdr[0];
	seg_hdr[1].cmplt = 1;
	seg_hdr[1].length += (pdu_write_size - pdu_write_loc);
	ztest_expect_data(source_pdu_write_test, pdu_buffer, &pdu_buffer);
	ztest_expect_value(source_pdu_write_test, pdu_offset, pdu_hdr_loc);
	ztest_expect_data(source_pdu_write_test, sdu_payload, &seg_hdr[1]);
	ztest_expect_value(source_pdu_write_test, consume_len, PDU_ISO_SEG_HDR_SIZE);
	ztest_returns_value(source_pdu_write_test, ISOAL_STATUS_OK);

	/* PDU emit not expected */

	/* PDU release not expected (No Error) */

	err = isoal_tx_sdu_fragment(source_hdl, &tx_sdu_frag_buf.sdu_tx);

	zassert_equal(err, ISOAL_STATUS_OK, "err = 0x%02x", err);

	/* Test PDU release */

	ztest_expect_value(source_pdu_emit_test, node_tx, &tx_pdu_meta_buf.node_tx);
	ztest_expect_value(source_pdu_emit_test, node_tx->payload_count, payload_number);
	ztest_expect_value(source_pdu_emit_test, node_tx->sdu_fragments, sdu_fragments);
	ztest_expect_value(source_pdu_emit_test, pdu->ll_id, PDU_BIS_LLID_FRAMED);
	ztest_expect_value(source_pdu_emit_test, pdu->length, pdu_write_size);
	ztest_expect_value(source_pdu_emit_test, handle,
			   bt_iso_handle(isoal_global.source_state[source_hdl].session.handle));
	ztest_returns_value(source_pdu_emit_test, ISOAL_STATUS_OK);

	isoal_tx_event_prepare(source_hdl, event_number);
}
