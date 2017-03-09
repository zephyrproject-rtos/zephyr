/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>

#include <dns_pack.h>

#define MAX_BUF_SIZE	512
/* RFC 1035, 4.1.1. Header section format */
#define DNS_HEADER_SIZE	12

static uint8_t buf[MAX_BUF_SIZE];
static uint16_t buf_len;

static uint8_t qname[MAX_BUF_SIZE];
static uint16_t qname_len;


/* Domain: www.zephyrproject.org
 * Type: standard query (IPv4)
 * Transaction ID: 0xda0f
 * Recursion desired
 */
static uint8_t query_ipv4[] = { 0xda, 0x0f, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x03, 0x77, 0x77, 0x77,
				0x0d, 0x7a, 0x65, 0x70, 0x68, 0x79, 0x72, 0x70,
				0x72, 0x6f, 0x6a, 0x65, 0x63, 0x74, 0x03, 0x6f,
				0x72, 0x67, 0x00, 0x00, 0x01, 0x00, 0x01 };

#define DNAME1 "www.zephyrproject.org"
static uint16_t tid1 = 0xda0f;

static int eval_query(const char *dname, uint16_t tid, enum dns_rr_type type,
		      uint8_t *expected, uint16_t expected_len)
{
	uint8_t *question;
	int rc;

	rc = dns_msg_pack_qname(&qname_len, qname, MAX_BUF_SIZE, dname);
	if (rc != 0) {
		goto lb_exit;
	}

	rc = dns_msg_pack_query(buf, &buf_len, MAX_BUF_SIZE, qname, qname_len,
				tid, type);
	if (rc != 0) {
		goto lb_exit;
	}

	if (dns_unpack_header_id(buf) != tid) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* This is a query */
	if (dns_header_qr(buf) != DNS_QUERY) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* This is a query (standard query) */
	if (dns_header_opcode(buf) != DNS_QUERY) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Authoritative Answer must be 0 for a Query */
	if (dns_header_aa(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* TrunCation is always 0 */
	if (dns_header_tc(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Recursion Desired is always 1 */
	if (dns_header_rd(buf) != 1) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Recursion Available is always 0 */
	if (dns_header_ra(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Z is always 0 */
	if (dns_header_z(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Response code must be 0 (no error) */
	if (dns_header_rcode(buf) != DNS_HEADER_NOERROR) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Question counter must be 1 */
	if (dns_header_qdcount(buf) != 1) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Answer counter must be 0 */
	if (dns_header_ancount(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Name server resource records counter must be 0 */
	if (dns_header_nscount(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Additional records counter must be 0 */
	if (dns_header_arcount(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	question = buf + DNS_HEADER_SIZE;

	/* QClass */
	if (dns_unpack_query_qclass(question + qname_len) != DNS_CLASS_IN) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* QType */
	if (dns_unpack_query_qtype(question + qname_len) != type) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* compare with the expected result */
	if (buf_len != expected_len) {
		rc = -EINVAL;
		goto lb_exit;
	}

	if (memcmp(expected, buf, buf_len) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

lb_exit:
	return rc;
}

void test_dns_query(void)
{
	int rc;

	rc = eval_query(DNAME1, tid1, DNS_RR_TYPE_A,
			query_ipv4, sizeof(query_ipv4));
	assert_equal(rc, 0, "Query test failed for domain: "DNAME1);

	rc = eval_query(NULL, tid1, DNS_RR_TYPE_A,
			query_ipv4, sizeof(query_ipv4));
	assert_not_equal(rc, 0, "Query test with invalid domain name failed");

	rc = eval_query(DNAME1, tid1, DNS_RR_TYPE_AAAA,
			query_ipv4, sizeof(query_ipv4));
	assert_not_equal(rc, 0, "Query test for IPv4 with RR type AAAA failed");

	rc = eval_query(DNAME1, tid1 + 1, DNS_RR_TYPE_A,
			query_ipv4, sizeof(query_ipv4));
	assert_not_equal(rc, 0, "Query test with invalid ID failed");
}

void test_main(void)
{
	ztest_test_suite(dns_tests, ztest_unit_test(test_dns_query));

	ztest_run_test_suite(dns_tests);
}
