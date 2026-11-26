/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>

#include "helpers.h"

uint8_t req_buf[512];
uint8_t rsp_buf[512];

/* Build a request by prepending, so bindings are added last to first. */
size_t build_request(int32_t version, const char *community, uint8_t pdu_type, int32_t err_status,
		     int32_t err_index, const struct req_vb *vbs, size_t count, size_t *start)
{
	size_t s = sizeof(req_buf);
	size_t end = s;

	for (size_t n = count; n > 0; n--) {
		const struct req_vb *vb = &vbs[n - 1];
		size_t vb_end = s;

		switch (vb->value.tag) {
		case 0:
			zassert_ok(
				snmp_ber_prepend_octets(req_buf, &s, SNMP_BER_TAG_NULL, NULL, 0));
			break;
		case SNMP_BER_TAG_INTEGER:
			zassert_ok(snmp_ber_prepend_int32(req_buf, &s, vb->value.i32));
			break;
		case SNMP_BER_TAG_OCTET_STRING:
			zassert_ok(snmp_ber_prepend_octets(req_buf, &s, SNMP_BER_TAG_OCTET_STRING,
							   (const uint8_t *)vb->value.str,
							   strlen(vb->value.str)));
			break;
		default:
			zassert_ok(snmp_ber_prepend_octets(req_buf, &s, vb->value.tag, NULL, 0));
			break;
		}

		{
			/* Encode the name forward into scratch, then prepend it. */
			uint8_t scratch[64];
			struct snmp_ber_writer w;

			snmp_ber_writer_init(&w, scratch, sizeof(scratch));
			zassert_ok(snmp_ber_write_oid(&w, vb->oid, vb->oid_len));
			zassert_true(s >= w.pos);
			s -= w.pos;
			memcpy(&req_buf[s], scratch, w.pos);
		}

		zassert_ok(snmp_ber_prepend_header(req_buf, &s, SNMP_BER_TAG_SEQUENCE, vb_end - s));
	}

	zassert_ok(snmp_ber_prepend_header(req_buf, &s, SNMP_BER_TAG_SEQUENCE, end - s));
	zassert_ok(snmp_ber_prepend_int32(req_buf, &s, err_index));
	zassert_ok(snmp_ber_prepend_int32(req_buf, &s, err_status));
	zassert_ok(snmp_ber_prepend_int32(req_buf, &s, REQUEST_ID));
	zassert_ok(snmp_ber_prepend_header(req_buf, &s, pdu_type, end - s));
	zassert_ok(snmp_ber_prepend_octets(req_buf, &s, SNMP_BER_TAG_OCTET_STRING,
					   (const uint8_t *)community, strlen(community)));
	zassert_ok(snmp_ber_prepend_int32(req_buf, &s, version));
	zassert_ok(snmp_ber_prepend_header(req_buf, &s, SNMP_BER_TAG_SEQUENCE, end - s));

	*start = s;

	return end - s;
}

void parse_response(const uint8_t *data, size_t len, struct response *rsp)
{
	struct snmp_ber_reader outer, msg, pdu, list;
	const uint8_t *octets;
	size_t octets_len, pdu_len;
	uint8_t tag;

	memset(rsp, 0, sizeof(*rsp));

	snmp_ber_reader_init(&outer, data, len);
	zassert_ok(snmp_ber_read_sequence(&outer, SNMP_BER_TAG_SEQUENCE, &msg));
	zassert_true(snmp_ber_reader_at_end(&outer), "Trailing bytes after message");

	zassert_ok(snmp_ber_read_int32(&msg, &rsp->version));
	zassert_ok(snmp_ber_read_octets(&msg, SNMP_BER_TAG_OCTET_STRING, &octets, &octets_len));
	zassert_true(octets_len < sizeof(rsp->community));
	memcpy(rsp->community, octets, octets_len);
	rsp->community_len = octets_len;

	zassert_ok(snmp_ber_read_header(&msg, &tag, &pdu_len));
	zassert_equal(tag, SNMP_BER_TAG_RESPONSE, "PDU type 0x%02x is not a response", tag);
	snmp_ber_reader_init(&pdu, &msg.data[msg.pos], pdu_len);
	msg.pos += pdu_len;
	zassert_true(snmp_ber_reader_at_end(&msg), "Trailing bytes after PDU");

	zassert_ok(snmp_ber_read_int32(&pdu, &rsp->request_id));
	zassert_ok(snmp_ber_read_int32(&pdu, &rsp->error_status));
	zassert_ok(snmp_ber_read_int32(&pdu, &rsp->error_index));
	zassert_ok(snmp_ber_read_sequence(&pdu, SNMP_BER_TAG_SEQUENCE, &list));
	zassert_true(snmp_ber_reader_at_end(&pdu), "Trailing bytes after bindings");

	rsp->raw_vbs = list.data;
	rsp->raw_vbs_len = list.len;

	while (!snmp_ber_reader_at_end(&list)) {
		struct snmp_ber_reader vb;
		struct rsp_vb *out;

		zassert_true(rsp->vb_count < ARRAY_SIZE(rsp->vb), "Too many bindings");
		out = &rsp->vb[rsp->vb_count++];

		zassert_ok(snmp_ber_read_sequence(&list, SNMP_BER_TAG_SEQUENCE, &vb));
		zassert_ok(snmp_ber_read_oid(&vb, out->oid, ARRAY_SIZE(out->oid), &out->oid_len));
		zassert_ok(snmp_ber_peek_tag(&vb, &out->tag));

		/* An error response echoes the request's values as they were,
		 * including ones the codec rejects; expect_echo() checks them.
		 */
		if (rsp->error_status != SNMP_ERR_NO_ERROR) {
			zassert_ok(snmp_ber_skip(&vb));
			zassert_true(snmp_ber_reader_at_end(&vb), "Trailing bytes in binding");
			continue;
		}

		switch (out->tag) {
		case SNMP_BER_TAG_INTEGER:
			zassert_ok(snmp_ber_read_int32(&vb, &out->i32));
			break;
		case SNMP_BER_TAG_OCTET_STRING:
			zassert_ok(snmp_ber_read_octets(&vb, out->tag, &octets, &octets_len));
			/* Long values are truncated; str_len keeps the real length */
			memcpy(out->str, octets, MIN(octets_len, sizeof(out->str)));
			out->str_len = octets_len;
			break;
		case SNMP_BER_TAG_GAUGE32:
		case SNMP_BER_TAG_COUNTER32:
		case SNMP_BER_TAG_TIMETICKS:
			zassert_ok(snmp_ber_read_uint32(&vb, out->tag, &out->u32));
			break;
		case SNMP_BER_TAG_COUNTER64:
			zassert_ok(snmp_ber_read_uint64(&vb, out->tag, &out->u64));
			break;
		case SNMP_BER_TAG_OID:
			zassert_ok(snmp_ber_read_oid(&vb, out->val_oid, ARRAY_SIZE(out->val_oid),
						     &out->val_oid_len));
			break;
		default:
			zassert_ok(snmp_ber_read_empty(&vb, out->tag));
			break;
		}

		zassert_true(snmp_ber_reader_at_end(&vb), "Trailing bytes in binding");
	}
}

/* Send a request through the engine and parse the reply. Returns the reply
 * length, 0 when the engine dropped the request.
 */
size_t exchange(int32_t version, const char *community, uint8_t pdu_type, int32_t err_status,
		int32_t err_index, const struct req_vb *vbs, size_t count, struct response *rsp)
{
	size_t req_start, req_len, rsp_start, rsp_len;

	req_len = build_request(version, community, pdu_type, err_status, err_index, vbs, count,
				&req_start);

	memset(rsp_buf, 0xEE, sizeof(rsp_buf));
	rsp_len = snmp_process_message(&req_buf[req_start], req_len, rsp_buf, sizeof(rsp_buf),
				       &rsp_start);
	if (rsp_len == 0) {
		return 0;
	}

	zassert_true(rsp_start + rsp_len <= sizeof(rsp_buf));
	parse_response(&rsp_buf[rsp_start], rsp_len, rsp);

	zassert_equal(rsp->version, version, "Response version %d", rsp->version);
	zassert_equal(rsp->community_len, strlen(community));
	zassert_mem_equal(rsp->community, community, rsp->community_len);
	zassert_equal(rsp->request_id, REQUEST_ID, "Request id 0x%x", rsp->request_id);

	return rsp_len;
}

void expect_oid(const struct rsp_vb *vb, const uint32_t *oid, size_t len)
{
	zassert_equal(vb->oid_len, len, "Binding has %zu arcs, expected %zu", vb->oid_len, len);
	zassert_mem_equal(vb->oid, oid, len * sizeof(uint32_t), "Wrong binding name");
}

void expect_ok(const struct response *rsp, size_t count)
{
	zassert_equal(rsp->error_status, SNMP_ERR_NO_ERROR, "Error status %d index %d",
		      rsp->error_status, rsp->error_index);
	zassert_equal(rsp->error_index, 0);
	zassert_equal(rsp->vb_count, count, "%zu bindings, expected %zu", rsp->vb_count, count);
}

void expect_error(const struct response *rsp, int32_t status, int32_t index)
{
	zassert_equal(rsp->error_status, status, "Error status %d, expected %d", rsp->error_status,
		      status);
	zassert_equal(rsp->error_index, index, "Error index %d, expected %d", rsp->error_index,
		      index);
}

/* The bindings of an error response are those of the request, byte for byte. */
void expect_echo(const struct response *rsp, const struct req_vb *vbs, size_t count)
{
	zassert_equal(rsp->vb_count, count);

	for (size_t i = 0; i < count; i++) {
		expect_oid(&rsp->vb[i], vbs[i].oid, vbs[i].oid_len);
		zassert_equal(rsp->vb[i].tag,
			      (vbs[i].value.tag == 0) ? SNMP_BER_TAG_NULL : vbs[i].value.tag);
	}
}

void get_display_string(const uint32_t *oid, size_t len, char *out, size_t size)
{
	struct req_vb vb = {oid, len, {0}};
	struct response rsp;

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, &vb, 1, &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_OCTET_STRING);
	zassert_true(rsp.vb[0].str_len < size);
	memcpy(out, rsp.vb[0].str, rsp.vb[0].str_len);
	out[rsp.vb[0].str_len] = '\0';
}

int32_t get_int32(const uint32_t *oid, size_t len)
{
	struct req_vb vb = {oid, len, {0}};
	struct response rsp;

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, &vb, 1, &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_INTEGER);

	return rsp.vb[0].i32;
}
