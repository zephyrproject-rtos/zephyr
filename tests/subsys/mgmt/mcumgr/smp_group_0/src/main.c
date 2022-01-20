/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/byteorder.h>
#include <net/buf.h>
#include <mgmt/mgmt.h>
#include "os_mgmt/os_mgmt.h"


/* Request/response buffers */
extern struct net_buf_simple ut_mcumgr_req_buf;
extern struct net_buf_simple ut_mcumgr_rsp_buf;

/* The callback that is given to the ut_smp_req_mcumgr to be called when
 * response buffer is ready for processing. Function should access response
 * buffer via global pointer ut_mcumgr_res_buf and return EMGMT_ error
 * code.
 */
typedef int (*mcumgr_rsp_callback)(void);

/* The function directly returns result of calling smp_process_request_packet */
int ut_smp_req_to_mcumgr(struct net_buf_simple *nb, mcumgr_rsp_callback rps_cb);

static void dump_hex(char *title, uint8_t *p, size_t len)
{
	uint8_t *end = p + len;
	int i = 0;

	if (len == 0) {
		return;
	}
	TC_PRINT("HEX DUMP START: %s %p + %d\n", title, p, len);
	TC_PRINT("    | +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +a +b +c +d +e +f\n");
	while (end != p) {
		if (i % 16 == 0) {
			TC_PRINT("%04x| ", i);
		}
		TC_PRINT("%02x ", *p);
		++i;
		++p;
		if (i % 16 == 0) {
			TC_PRINT("\n");
		}
	}
	if (i % 16) {
		TC_PRINT("\n");
	}
	TC_PRINT("HEX DUMP END: %s\n", title);
}

/* This is callback that will be called by MCUMGR lib to pass response */
static int response_from_mcumgr(void)
{
	int rc = 0;

	return rc;
}

/* Not really a test, group registration */
void test_register_group_0(void)
{
	TC_PRINT("Registering group 0\n");
	os_mgmt_register_group();
}

/* Check response header against request header */
void check_mgmt_hdr_req_vs_res(void)
{
	const struct mgmt_hdr *req = (struct mgmt_hdr *)ut_mcumgr_req_buf.data;
	const struct mgmt_hdr *rsp = (struct mgmt_hdr *)ut_mcumgr_rsp_buf.data;

	zassert_true(ut_mcumgr_rsp_buf.len >= 8, "Response header too short");
	/* res.nh_op range is not checked on purpose: we assume that test sends correct one */
	zassert_equal(req->nh_op + 1, rsp->nh_op, "Expected opcode %d, got %d", req->nh_op + 1,
		     rsp->nh_op);
	zassert_equal(rsp->nh_flags, 0, "Expected 0 flags, got 0x%02x", rsp->nh_flags);
	zassert_equal(req->nh_group, rsp->nh_group, "Expected group %d, got %d", req->nh_group,
		     rsp->nh_group);
	zassert_equal(req->nh_seq, rsp->nh_seq, "Expected sequence number %d, got %d",
		     req->nh_seq, rsp->nh_seq);
	zassert_equal(req->nh_id, rsp->nh_id, "Expected sequence number %d, got %d", req->nh_id,
		     rsp->nh_id);
}

/* 0 length packet */
void test_nothing(void)
{
	const uint8_t too_short[] = { 0 };

	net_buf_simple_reset(&ut_mcumgr_req_buf);
	net_buf_simple_reset(&ut_mcumgr_rsp_buf);
	net_buf_simple_add_mem(&ut_mcumgr_req_buf, (void *)too_short, 0);

	int ret = ut_smp_req_to_mcumgr(&ut_mcumgr_req_buf, response_from_mcumgr);

	zassert_equal(MGMT_ERR_EOK, ret, "Expected MGMT_ERR_EOK (0) got %d", ret);
	zassert_equal(ut_mcumgr_rsp_buf.len, 0, "Unexpected modification of response buffer");
	dump_hex("response", ut_mcumgr_rsp_buf.data, ut_mcumgr_rsp_buf.len);
}

/* Too short to even parse header */
void test_too_short(void)
{
	const uint8_t too_short[] = { 0x02, 0x00, 0x00, 0x09 };

	net_buf_simple_reset(&ut_mcumgr_req_buf);
	net_buf_simple_reset(&ut_mcumgr_rsp_buf);
	net_buf_simple_add_mem(&ut_mcumgr_req_buf, (void *)too_short, ARRAY_SIZE(too_short));

	int ret = ut_smp_req_to_mcumgr(&ut_mcumgr_req_buf, response_from_mcumgr);

	zassert_equal(MGMT_ERR_ECORRUPT, ret, "Expected MGMT_ERR_ECORRUPT (9) got %d", ret);
	zassert_equal(ut_mcumgr_rsp_buf.len, 0, "Unexpected modification of response buffer");
	dump_hex("response", ut_mcumgr_rsp_buf.data, ut_mcumgr_rsp_buf.len);
}

/* Received Header only but with expected payload */
void test_header_only(void)
{
	const uint8_t header_only[] = { 0x02, 0x00, 0x00, 0x09, 0x00, 0x00, 0x42, 0x00 };

	net_buf_simple_reset(&ut_mcumgr_req_buf);
	net_buf_simple_reset(&ut_mcumgr_rsp_buf);
	net_buf_simple_add_mem(&ut_mcumgr_req_buf, (void *)header_only, ARRAY_SIZE(header_only));

	int ret = ut_smp_req_to_mcumgr(&ut_mcumgr_req_buf, response_from_mcumgr);

	zassert_equal(MGMT_ERR_ECORRUPT, ret, "Expected MGMT_ERR_ECORRUPT (9) got %d", ret);
	check_mgmt_hdr_req_vs_res();
	dump_hex("response", ut_mcumgr_rsp_buf.data, ut_mcumgr_rsp_buf.len);
}

/* TODO: Test for valid header with no payload */

/* Valid frame but command not registered */
void test_unregistered(void)
{
	const uint8_t group_0_echo[] = {
		0x02, 0x00, 0x00, 0x09, 0x00, 0x00, 0x42, 0x00, 0xa1, 0x61, 0x64, 0x65, 0x68, 0x65,
		0x6c, 0x6c, 0x6c };

	net_buf_simple_reset(&ut_mcumgr_req_buf);
	net_buf_simple_reset(&ut_mcumgr_rsp_buf);
	net_buf_simple_add_mem(&ut_mcumgr_req_buf, (void *)group_0_echo, ARRAY_SIZE(group_0_echo));

	int ret = ut_smp_req_to_mcumgr(&ut_mcumgr_req_buf, response_from_mcumgr);

	zassert_equal(MGMT_ERR_ENOTSUP, ret, "Expected MGMT_ERR_ENOTSUP (8) got %d", ret);
	check_mgmt_hdr_req_vs_res();
	dump_hex("response", ut_mcumgr_rsp_buf.data, ut_mcumgr_rsp_buf.len);
}

/* Valid echo */
void test_echo(void)
{
	const uint8_t group_0_echo[] = {
		0x02, 0x00, 0x00, 0x09, 0x00, 0x00, 0x42, 0x00, 0xa1, 0x61, 0x64, 0x65, 0x68, 0x65,
		0x6c, 0x6c, 0x6c };

	net_buf_simple_reset(&ut_mcumgr_req_buf);
	net_buf_simple_reset(&ut_mcumgr_rsp_buf);
	net_buf_simple_add_mem(&ut_mcumgr_req_buf, (void *)group_0_echo, ARRAY_SIZE(group_0_echo));

	int ret = ut_smp_req_to_mcumgr(&ut_mcumgr_req_buf, response_from_mcumgr);

	zassert_equal(0, ret, "Expected success (0) got %d", ret);
	check_mgmt_hdr_req_vs_res();
	dump_hex("response", ut_mcumgr_rsp_buf.data, ut_mcumgr_rsp_buf.len);
	/* TODO: Container parsing */
}

void test_main(void)
{
	ztest_test_suite(
		mcumgr_smp_group_0,
		ztest_unit_test(test_nothing),
		ztest_unit_test(test_too_short),
		ztest_unit_test(test_header_only),
		ztest_unit_test(test_unregistered),
		ztest_unit_test(test_register_group_0),
		ztest_unit_test(test_echo)
		);

	ztest_run_test_suite(mcumgr_smp_group_0);
}
