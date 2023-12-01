/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/ztest.h>

#include <zephyr/net/dns_sd.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_pkt.h>

#include "dns_pack.h"
#include "dns_sd.h"

#define BUFSZ 256
#define IP_ADDR(a, b, c, d) ((uint32_t)		  \
			     0			  \
			     | ((a & 0xff) << 24) \
			     | ((b & 0xff) << 16) \
			     | ((c & 0xff) <<  8) \
			     | ((d & 0xff) <<  0) \
			     )

extern bool label_is_valid(const char *label, size_t label_size);
extern int add_a_record(const struct dns_sd_rec *inst, uint32_t ttl,
			uint16_t host_offset, uint32_t addr,
			uint8_t *buf,
			uint16_t buf_offset, uint16_t buf_size);
extern int add_ptr_record(const struct dns_sd_rec *inst, uint32_t ttl,
			  uint8_t *buf, uint16_t buf_offset,
			  uint16_t buf_size,
			  uint16_t *service_offset,
			  uint16_t *instance_offset,
			  uint16_t *domain_offset);
extern int add_txt_record(const struct dns_sd_rec *inst, uint32_t ttl,
			  uint16_t instance_offset, uint8_t *buf,
			  uint16_t buf_offset, uint16_t buf_size);
extern int add_aaaa_record(const struct dns_sd_rec *inst, uint32_t ttl,
			   uint16_t host_offset, const uint8_t addr[16],
			   uint8_t *buf, uint16_t buf_offset,
			   uint16_t buf_size);
extern int add_srv_record(const struct dns_sd_rec *inst, uint32_t ttl,
			  uint16_t instance_offset,
			  uint16_t domain_offset,
			  uint8_t *buf, uint16_t buf_offset,
			  uint16_t buf_size,
			  uint16_t *host_offset);
extern size_t service_proto_size(const struct dns_sd_rec *ref);
extern bool rec_is_valid(const struct dns_sd_rec *ref);
extern int setup_dst_addr(struct net_context *ctx, sa_family_t family,
			  struct sockaddr *dst, socklen_t *dst_len);


/** Text for advertised service */
static const uint8_t nasxxxxxx_text[] = "\x06" "path=/";

/** A completely invalid record */
DNS_SD_REGISTER_SERVICE(invalid_dns_sd_record, NULL, NULL,
	NULL, NULL, NULL, NULL);

/* initialized to zero for illustrative purposes */
static uint16_t nonconst_port;
DNS_SD_REGISTER_SERVICE(nasxxxxxx_ephemeral, "NASXXXXXX", "_http",
	"_tcp", "local", nasxxxxxx_text, &nonconst_port);

/** Sample advertised service */
#define CONST_PORT 8080
DNS_SD_REGISTER_TCP_SERVICE(nasxxxxxx, "NASXXXXXX", "_http",
	"local", nasxxxxxx_text, CONST_PORT);

/** Buffer for  DNS queries */
static uint8_t create_query_buf[BUFSZ];

/**
 * @brief Create a DNS query
 *
 * @param inst the service instance to query
 * @param the type of DNS Resource Record to query
 * @param[out] size the size of the resulting query
 *
 * @return on success a pointer to the buffer containing the query
 * @return NULL on failure
 */
static uint8_t *create_query(const struct dns_sd_rec *inst,
			     uint8_t rr_type, size_t *size)
{
	uint16_t offs = 0;
	uint8_t label_size;
	uint16_t sp_size = service_proto_size(inst);

	uint16_t expected_req_buf_size = 0
					 + sizeof(struct dns_header)
					 + sp_size
					 + sizeof(struct dns_query);

	struct dns_header *hdr =
		(struct dns_header *)&create_query_buf[0];

	hdr->id = htons(0);
	hdr->qdcount = htons(1);
	offs += sizeof(struct dns_header);

	label_size = strlen(inst->service);
	create_query_buf[offs++] = label_size;
	memcpy(&create_query_buf[offs], inst->service, label_size);
	offs += label_size;

	label_size = strlen(inst->proto);
	create_query_buf[offs++] = label_size;
	memcpy(&create_query_buf[offs], inst->proto, label_size);
	offs += label_size;

	label_size = strlen(inst->domain);
	create_query_buf[offs++] = label_size;
	memcpy(&create_query_buf[offs], inst->domain, label_size);
	offs += label_size;

	create_query_buf[offs++] = '\0';

	struct dns_query *query =
		(struct dns_query *)&create_query_buf[offs];
	query->type = htons(rr_type);
	query->class_ = htons(DNS_CLASS_IN);
	offs += sizeof(struct dns_query);

	zassert_equal(expected_req_buf_size, offs,
		      "sz: %zu offs: %u", expected_req_buf_size, offs);

	*size = offs;

	return create_query_buf;
}

/** Test for @ref label_is_valid */
ZTEST(dns_sd, test_label_is_valid)
{
	zassert_equal(false, label_is_valid(NULL,
					    DNS_LABEL_MIN_SIZE), "");
	zassert_equal(false, label_is_valid("",
					    DNS_LABEL_MIN_SIZE - 1),
		      "");
	zassert_equal(false, label_is_valid(
			      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
			      DNS_LABEL_MAX_SIZE + 1), "");
	zassert_equal(true,  label_is_valid("a",
					    DNS_LABEL_MIN_SIZE), "");
	zassert_equal(true,  label_is_valid(
			      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
			      DNS_LABEL_MAX_SIZE), "");
	zassert_equal(false, label_is_valid("9abc", 4), "");
	zassert_equal(true,  label_is_valid("a9bc", 4), "");
	zassert_equal(false, label_is_valid("-abc", 4), "");
	zassert_equal(true,  label_is_valid("a-bc", 4), "");
	zassert_equal(true,  label_is_valid("A-Bc", 4), "");
}

/** Test for @ref dns_sd_rec_is_valid */
ZTEST(dns_sd, test_dns_sd_rec_is_valid)
{
	DNS_SD_REGISTER_TCP_SERVICE(name_min,
				"x",
				"_x",
				"xx",
				DNS_SD_EMPTY_TXT,
				CONST_PORT);
	zassert_equal(true, rec_is_valid(&name_min), "");

	DNS_SD_REGISTER_TCP_SERVICE(name_max,
				"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
				"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
				"_xxxxxxxxxxxxxxx",
				"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
				"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
				DNS_SD_EMPTY_TXT,
				CONST_PORT);
	zassert_equal(true, rec_is_valid(&name_max), "");

	DNS_SD_REGISTER_TCP_SERVICE(label_too_small,
				"x",
				"_",
				"xx",
				DNS_SD_EMPTY_TXT,
				CONST_PORT);
	zassert_equal(false, rec_is_valid(&label_too_small), "");

	DNS_SD_REGISTER_TCP_SERVICE(label_too_big,
				"x",
				"_x",
				"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
				"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
				DNS_SD_EMPTY_TXT,
				CONST_PORT);
	zassert_equal(false, rec_is_valid(&label_too_big), "");

	DNS_SD_REGISTER_TCP_SERVICE(invalid_instance,
				"abc" "\x01" "def",
				"_x",
				"xx",
				DNS_SD_EMPTY_TXT,
				CONST_PORT);
	zassert_equal(false, rec_is_valid(&invalid_instance), "");

	DNS_SD_REGISTER_TCP_SERVICE(invalid_service_prefix,
				"x",
				"xx",
				"xx",
				DNS_SD_EMPTY_TXT,
				CONST_PORT);
	zassert_equal(false, rec_is_valid(
			      &invalid_service_prefix), "");

	DNS_SD_REGISTER_TCP_SERVICE(invalid_service,
				"x",
				"_x.y",
				"xx",
				DNS_SD_EMPTY_TXT,
				CONST_PORT);
	zassert_equal(false, rec_is_valid(&invalid_service), "");

	DNS_SD_REGISTER_SERVICE(invalid_proto,
				"x",
				"_y",
				"_wtf",
				"xx",
				DNS_SD_EMPTY_TXT,
				&nonconst_port);
	zassert_equal(false, rec_is_valid(&invalid_proto), "");

	/* We do not currently support subdomains */
	DNS_SD_REGISTER_TCP_SERVICE(invalid_domain,
				"x",
				"_x",
				"x.y",
				DNS_SD_EMPTY_TXT,
				CONST_PORT);
	zassert_equal(false, rec_is_valid(&invalid_domain), "");

	zassert_equal(true, rec_is_valid(&nasxxxxxx), "");
}

/** Test for @ref creqte_query */
ZTEST(dns_sd, test_create_query)
{
	size_t actual_query_size = -1;
	uint8_t *actual_query = create_query(&nasxxxxxx,
					     DNS_RR_TYPE_PTR,
					     &actual_query_size);
	uint8_t expected_query[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x05, 0x5f, 0x68, 0x74,
		0x74, 0x70, 0x04, 0x5f, 0x74, 0x63, 0x70, 0x05,
		0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00, 0x00, 0x0c,
		0x00, 0x01
	};
	size_t expected_query_size = sizeof(expected_query);

	zassert_equal(actual_query_size, expected_query_size, "");
	zassert_mem_equal(expected_query, actual_query,
			  MIN(actual_query_size, expected_query_size),
			  "");
}

/** Test for @ref add_ptr_record */
ZTEST(dns_sd, test_add_ptr_record)
{
	const uint32_t ttl = DNS_SD_PTR_TTL;
	const uint32_t offset = sizeof(struct dns_header);

	uint16_t service_offset = -1;
	uint16_t instance_offset = -1;
	uint16_t domain_offset = -1;

	static uint8_t actual_buf[BUFSZ];
	static const uint8_t expected_buf[] = {
		0x05, 0x5f, 0x68, 0x74, 0x74, 0x70, 0x04, 0x5f,
		0x74, 0x63, 0x70, 0x05, 0x6c, 0x6f, 0x63, 0x61,
		0x6c, 0x00, 0x00, 0x0c, 0x00, 0x01, 0x00, 0x00,
		0x11, 0x94, 0x00, 0x0c, 0x09, 0x4e, 0x41, 0x53,
		0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0xc0, 0x0c,
	};
	int expected_int = sizeof(expected_buf);

	int actual_int = add_ptr_record(&nasxxxxxx, ttl,
					actual_buf, offset,
					sizeof(actual_buf),
					&service_offset,
					&instance_offset,
					&domain_offset);

	zassert_equal(actual_int, expected_int, "");

	zassert_equal(instance_offset, 40, "");
	zassert_equal(domain_offset, 23, "");

	memmove(actual_buf, actual_buf + offset, actual_int);
	zassert_mem_equal(actual_buf, expected_buf,
			  MIN(actual_int, expected_int), "");

	/* dns_sd_rec_is_valid failure */
	DNS_SD_REGISTER_TCP_SERVICE(null_label,
				NULL,
				"_x",
				"xx",
				DNS_SD_EMPTY_TXT,
				CONST_PORT);
	zassert_equal(-EINVAL, add_ptr_record(&null_label, ttl,
					      actual_buf, offset,
					      actual_int,
					      &service_offset,
					      &instance_offset,
					      &domain_offset), "");

	/* buffer too small failure */
	zassert_equal(-ENOSPC, add_ptr_record(&nasxxxxxx, ttl,
					      actual_buf, offset, 0,
					      &service_offset,
					      &instance_offset,
					      &domain_offset), "");

	/* offset too big for message compression (service) */
	zassert_equal(-E2BIG, add_ptr_record(&nasxxxxxx, ttl,
					     actual_buf, DNS_SD_PTR_MASK,
					     0xffff, &service_offset,
					     &instance_offset,
					     &domain_offset), "");

	/* offset too big for message compression (instance) */
	zassert_equal(-E2BIG, add_ptr_record(&nasxxxxxx, ttl,
					     actual_buf, 0x3fff,
					     0xffff, &service_offset,
					     &instance_offset,
					     &domain_offset), "");
}

/** Test for @ref add_txt_record */
ZTEST(dns_sd, test_add_txt_record)
{
	const uint32_t ttl = DNS_SD_TXT_TTL;
	const uint32_t offset = 0;
	const uint16_t instance_offset = 0x28;

	static uint8_t actual_buf[BUFSZ];
	static const uint8_t expected_buf[] = {
		0xc0, 0x28, 0x00, 0x10, 0x80, 0x01, 0x00, 0x00,
		0x11, 0x94, 0x00, 0x07, 0x06, 0x70, 0x61, 0x74,
		0x68, 0x3d, 0x2f
	};
	int expected_int = sizeof(expected_buf);

	int actual_int = add_txt_record(&nasxxxxxx, ttl,
					instance_offset, actual_buf,
					offset,
					sizeof(actual_buf));

	zassert_equal(actual_int, expected_int, "");

	zassert_mem_equal(actual_buf, expected_buf, MIN(actual_int,
							expected_int),
			  "");

	/* too big for message compression */
	zassert_equal(-E2BIG,
		      add_txt_record(&nasxxxxxx, ttl, DNS_SD_PTR_MASK,
				     actual_buf, offset,
				     sizeof(actual_buf)), "");

	/* buffer too small */
	zassert_equal(-ENOSPC, add_txt_record(&nasxxxxxx, ttl, offset,
					      actual_buf, offset,
					      0), "");
}

/** Test for @ref add_srv_record */
ZTEST(dns_sd, test_add_srv_record)
{
	const uint32_t ttl = DNS_SD_SRV_TTL;
	const uint32_t offset = 0;
	const uint16_t instance_offset = 0x28;
	const uint16_t domain_offset = 0x17;

	uint16_t host_offset = -1;
	static uint8_t actual_buf[BUFSZ];
	static const uint8_t expected_buf[] = {
		0xc0, 0x28, 0x00, 0x21, 0x80, 0x01, 0x00, 0x00,
		0x00, 0x78, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00,
		0x1f, 0x90, 0x09, 0x4e, 0x41, 0x53, 0x58, 0x58,
		0x58, 0x58, 0x58, 0x58, 0xc0, 0x17
	};

	int expected_int = sizeof(expected_buf);
	int actual_int = add_srv_record(&nasxxxxxx, ttl,
					instance_offset, domain_offset,
					actual_buf,
					offset, sizeof(actual_buf),
					&host_offset);

	zassert_equal(actual_int, expected_int, "");

	zassert_equal(host_offset, 18, "");

	zassert_mem_equal(actual_buf, expected_buf,
			  MIN(actual_int, expected_int), "");

	/* offset too big for message compression (instance) */
	zassert_equal(-E2BIG,
		      add_srv_record(&nasxxxxxx, ttl, DNS_SD_PTR_MASK,
				     domain_offset,
				     actual_buf, offset,
				     sizeof(actual_buf),
				     &host_offset), "");

	/* offset too big for message compression (domain) */
	zassert_equal(-E2BIG, add_srv_record(&nasxxxxxx, ttl,
					     instance_offset,
					     DNS_SD_PTR_MASK,
					     actual_buf, offset,
					     sizeof(actual_buf),
					     &host_offset), "");

	/* buffer too small */
	zassert_equal(-ENOSPC, add_srv_record(&nasxxxxxx, ttl,
					      instance_offset,
					      domain_offset,
					      actual_buf,
					      offset, 0,
					      &host_offset), "");
}

/** Test for @ref add_a_record */
ZTEST(dns_sd, test_add_a_record)
{
	const uint32_t ttl = DNS_SD_A_TTL;
	const uint32_t offset = 0;
	const uint16_t host_offset = 0x59;
	/* this one is made up */
	const uint32_t addr = IP_ADDR(177, 5, 240, 13);

	static uint8_t actual_buf[BUFSZ];
	static const uint8_t expected_buf[] = {
		0xc0, 0x59, 0x00, 0x01, 0x80, 0x01, 0x00, 0x00,
		0x00, 0x78, 0x00, 0x04, 0xb1, 0x05, 0xf0, 0x0d,
	};

	int expected_int = sizeof(expected_buf);
	int actual_int = add_a_record(&nasxxxxxx, ttl, host_offset,
				      addr, actual_buf, offset,
				      sizeof(actual_buf));

	zassert_equal(actual_int, expected_int, "");

	zassert_mem_equal(actual_buf, expected_buf,
			  MIN(actual_int, expected_int), "");

	/* test offset too large */
	zassert_equal(-E2BIG,
		      add_a_record(&nasxxxxxx, ttl, DNS_SD_PTR_MASK,
				   addr, actual_buf, offset,
				   sizeof(actual_buf)), "");

	/* test buffer too small */
	zassert_equal(-ENOSPC, add_a_record(&nasxxxxxx, ttl,
					    host_offset, addr,
					    actual_buf, offset,
					    0), "");
}

/** Test for @ref add_aaaa_record */
ZTEST(dns_sd, test_add_aaaa_record)
{
	const uint32_t ttl = DNS_SD_AAAA_TTL;
	const uint32_t offset = 0;
	const uint16_t host_offset = 0x59;
	/* this one is made up */
	const uint8_t addr[16] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
	};

	static uint8_t actual_buf[BUFSZ];
	static const uint8_t expected_buf[] = {
		0xc0, 0x59, 0x00, 0x1c, 0x80, 0x01, 0x00, 0x00,
		0x00, 0x78, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x01,
	};

	int expected_int = sizeof(expected_buf);
	int actual_int = add_aaaa_record(&nasxxxxxx, ttl, host_offset,
					 addr, actual_buf, offset,
					 sizeof(actual_buf));

	zassert_equal(actual_int, expected_int, "");

	zassert_mem_equal(actual_buf, expected_buf,
			  MIN(actual_int, expected_int), "");

	/* offset too large for message compression */
	zassert_equal(-E2BIG,
		      add_aaaa_record(&nasxxxxxx, ttl, DNS_SD_PTR_MASK,
				      addr, actual_buf,
				      offset,
				      sizeof(actual_buf)), "");

	/* buffer too small */
	zassert_equal(-ENOSPC,
		      add_aaaa_record(&nasxxxxxx, ttl, host_offset,
				      addr, actual_buf,
				      offset, 0), "");
}

/** Test for @ref dns_sd_handle_ptr_query */
ZTEST(dns_sd, test_dns_sd_handle_ptr_query)
{
	struct in_addr addr = {
		.s_addr = htonl(IP_ADDR(177, 5, 240, 13)),
	};
	static uint8_t actual_rsp[512];
	static uint8_t expected_rsp[] = {
		0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x03, 0x05, 0x5f, 0x68, 0x74,
		0x74, 0x70, 0x04, 0x5f, 0x74, 0x63, 0x70, 0x05,
		0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00, 0x00, 0x0c,
		0x00, 0x01, 0x00, 0x00, 0x11, 0x94, 0x00, 0x0c,
		0x09, 0x4e, 0x41, 0x53, 0x58, 0x58, 0x58, 0x58,
		0x58, 0x58, 0xc0, 0x0c, 0xc0, 0x28, 0x00, 0x10,
		0x80, 0x01, 0x00, 0x00, 0x11, 0x94, 0x00, 0x07,
		0x06, 0x70, 0x61, 0x74, 0x68, 0x3d, 0x2f, 0xc0,
		0x28, 0x00, 0x21, 0x80, 0x01, 0x00, 0x00, 0x00,
		0x78, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x1f,
		0x90, 0x09, 0x4e, 0x41, 0x53, 0x58, 0x58, 0x58,
		0x58, 0x58, 0x58, 0xc0, 0x17, 0xc0, 0x59, 0x00,
		0x01, 0x80, 0x01, 0x00, 0x00, 0x00, 0x78, 0x00,
		0x04, 0xb1, 0x05, 0xf0, 0x0d,
	};
	int expected_int = sizeof(expected_rsp);
	int actual_int = dns_sd_handle_ptr_query(&nasxxxxxx,
						 &addr,
						 NULL,
						 &actual_rsp[0],
						 sizeof(actual_rsp) -
						 sizeof(struct dns_header));

	zassert_true(actual_int > 0,
		     "dns_sd_handle_ptr_query() failed (%d)",
		     actual_int);

	zassert_equal(actual_int, expected_int, "act: %d exp: %d", actual_int, expected_int);

	zassert_mem_equal(actual_rsp, expected_rsp,
			  MIN(actual_int, expected_int), "");

	/* show non-advertisement for uninitialized port */
	nonconst_port = 0;
	zassert_equal(-EHOSTDOWN, dns_sd_handle_ptr_query(&nasxxxxxx_ephemeral,
		&addr, NULL, &actual_rsp[0], sizeof(actual_rsp) -
		sizeof(struct dns_header)), "port zero should not "
		"produce any DNS-SD query response");

	/* show advertisement for initialized port */
	nonconst_port = CONST_PORT;
	expected_int = sizeof(expected_rsp);
	zassert_equal(expected_int, dns_sd_handle_ptr_query(&nasxxxxxx_ephemeral,
		&addr, NULL, &actual_rsp[0], sizeof(actual_rsp) -
		sizeof(struct dns_header)), "");

	zassert_equal(-EINVAL, dns_sd_handle_ptr_query(
		&invalid_dns_sd_record,
		&addr, NULL, &actual_rsp[0], sizeof(actual_rsp) -
		sizeof(struct dns_header)), "");
}

/** Test for @ref dns_sd_handle_ptr_query */
ZTEST(dns_sd, test_dns_sd_handle_service_type_enum)
{
	DNS_SD_REGISTER_TCP_SERVICE(chromecast,
				"Chromecast-abcd",
				"_googlecast",
				"local",
				DNS_SD_EMPTY_TXT,
				CONST_PORT);

	struct in_addr addr = {
		.s_addr = htonl(IP_ADDR(177, 5, 240, 13)),
	};
	static uint8_t actual_rsp[512];
	static uint8_t expected_rsp[] = {
		0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
		0x00, 0x00, 0x00, 0x09, 0x5f, 0x73, 0x65, 0x72, 0x76,
		0x69, 0x63, 0x65, 0x73, 0x07, 0x5f, 0x64, 0x6e, 0x73,
		0x2d, 0x73, 0x64, 0x04, 0x5f, 0x75, 0x64, 0x70, 0x05,
		0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00, 0x00, 0x0c, 0x00,
		0x01, 0x00, 0x00, 0x11, 0x94, 0x00, 0x13, 0x0b, 0x5f,
		0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x63, 0x61, 0x73,
		0x74, 0x04, 0x5f, 0x74, 0x63, 0x70, 0xc0, 0x23,
	};
	int expected_int = sizeof(expected_rsp);
	int actual_int = dns_sd_handle_service_type_enum(&chromecast,
						 &addr,
						 NULL,
						 &actual_rsp[0],
						 sizeof(actual_rsp) -
						 sizeof(struct dns_header));

	zassert_true(actual_int > 0, "dns_sd_handle_service_type_enum() failed (%d)", actual_int);

	zassert_equal(actual_int, expected_int, "act: %d exp: %d", actual_int, expected_int);

	zassert_mem_equal(actual_rsp, expected_rsp, MIN(actual_int, expected_int), "");

	/* show non-advertisement for uninitialized port */
	nonconst_port = 0;
	zassert_equal(-EHOSTDOWN,
		      dns_sd_handle_service_type_enum(&nasxxxxxx_ephemeral, &addr, NULL,
				&actual_rsp[0], sizeof(actual_rsp) - sizeof(struct dns_header)),
		      "port zero should not "
		      "produce any DNS-SD query response");

	zassert_equal(-EINVAL,
		      dns_sd_handle_service_type_enum(&invalid_dns_sd_record, &addr, NULL,
			  &actual_rsp[0], sizeof(actual_rsp) - sizeof(struct dns_header)),
		      "");
}

/** Test @ref dns_sd_rec_match */
ZTEST(dns_sd, test_dns_sd_rec_match)
{
	DNS_SD_REGISTER_TCP_SERVICE(record,
				    "NGINX",
				    "_http",
				    "local",
				    DNS_SD_EMPTY_TXT,
				    CONST_PORT);

	static const struct dns_sd_rec filter_ok = {
		.instance = NULL,
		.service = "_http",
		.proto = "_tcp",
		.domain = NULL,
		.text = NULL,
		.text_size = 0,
		.port = NULL,
	};

	static const struct dns_sd_rec filter_nok = {
		.instance = NULL,
		.service = "_wtftp",
		.proto = "_udp",
		.domain = NULL,
		.text = NULL,
		.text_size = 0,
		.port = NULL,
	};

	zassert_equal(false, dns_sd_rec_match(NULL, NULL), "");
	zassert_equal(false, dns_sd_rec_match(NULL, &filter_ok), "");
	zassert_equal(false, dns_sd_rec_match(&record, NULL), "");
	zassert_equal(false, dns_sd_rec_match(&record, &filter_nok), "");
	zassert_equal(true, dns_sd_rec_match(&record, &filter_ok), "");
}

/** Test @ref setup_dst_addr */
ZTEST(dns_sd, test_setup_dst_addr)
{
	int ret;
	struct net_if *iface;
	struct sockaddr dst;
	socklen_t dst_len;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	/* IPv4 case */
	struct net_context *ctx_v4;
	struct in_addr addr_v4_expect = { { { 224, 0, 0, 251 } } };

	memset(&dst, 0, sizeof(struct sockaddr));

	ret = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &ctx_v4);
	zassert_equal(ret, 0, "Create IPv4 UDP context failed");

	zassert_equal(0, setup_dst_addr(ctx_v4, AF_INET, &dst, &dst_len), "");
	zassert_equal(255, ctx_v4->ipv4_mcast_ttl, "");
	zassert_true(net_ipv4_addr_cmp(&addr_v4_expect,
				       &net_sin(&dst)->sin_addr), "");
	zassert_equal(8, dst_len, "");

#if defined(CONFIG_NET_IPV6)
	/* IPv6 case */
	struct net_context *ctx_v6;
	struct in6_addr addr_v6_expect = { { { 0xff, 0x02, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0xfb } } };

	memset(&dst, 0, sizeof(struct sockaddr));

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &ctx_v6);
	zassert_equal(ret, 0, "Create IPv6 UDP context failed");

	zassert_equal(0, setup_dst_addr(ctx_v6, AF_INET6, &dst, &dst_len), "");
	zassert_equal(255, ctx_v6->ipv6_mcast_hop_limit, "");
	zassert_true(net_ipv6_addr_cmp(&addr_v6_expect,
				       &net_sin6(&dst)->sin6_addr), "");
	zassert_equal(24, dst_len, "");
#endif

	/* Unknown family case */

	struct net_context *ctx_xx;

	ret = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &ctx_xx);
	zassert_equal(ret, 0, "Create IPV4 udp context failed");

	zassert_equal(-EPFNOSUPPORT,
		      setup_dst_addr(ctx_xx, AF_PACKET, &dst, &dst_len), "");
}

/** test for @ref dns_sd_is_service_type_enumeration */
ZTEST(dns_sd, test_is_service_type_enumeration)
{
	static const struct dns_sd_rec filter_ok = {
		.instance = "_services",
		.service = "_dns-sd",
		.proto = "_udp",
		/* TODO: support additional service domains */
		.domain = "local",
		.text = dns_sd_empty_txt,
		.text_size = sizeof(dns_sd_empty_txt),
		.port = &dns_sd_port_zero,
	};

	zassert_true(dns_sd_is_service_type_enumeration(&filter_ok), "");

	static const struct dns_sd_rec filter_nok = {
		/* not a service_type_enumeration */
		.instance = "_serv1c3s",   .service = "_dns-sd",
		.proto = "_udp",	   .domain = "local",
		.text = dns_sd_empty_txt,  .text_size = sizeof(dns_sd_empty_txt),
		.port = &dns_sd_port_zero,
	};

	zassert_false(dns_sd_is_service_type_enumeration(&filter_nok), "");
}

ZTEST(dns_sd, test_extract_service_type_enumeration)
{
	static const uint8_t query[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x09, 0x5f,
		0x73, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x73, 0x07, 0x5f, 0x64, 0x6e, 0x73, 0x2d,
		0x73, 0x64, 0x04, 0x5f, 0x75, 0x64, 0x70, 0x05, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00,
	};

	struct dns_sd_rec record;
	char instance[DNS_SD_INSTANCE_MAX_SIZE + 1];
	char service[DNS_SD_SERVICE_MAX_SIZE + 1];
	char proto[DNS_SD_PROTO_SIZE + 1];
	char domain[DNS_SD_DOMAIN_MAX_SIZE + 1];
	char *label[4];
	size_t size[] = {
		ARRAY_SIZE(instance),
		ARRAY_SIZE(service),
		ARRAY_SIZE(proto),
		ARRAY_SIZE(domain),
	};
	size_t n = ARRAY_SIZE(label);

	BUILD_ASSERT(ARRAY_SIZE(label) == ARRAY_SIZE(size), "");

	/*
	 * work around for bug in compliance scripts which say that the array
	 * should be static const (incorrect)
	 */
	label[0] = instance;
	label[1] = service;
	label[2] = proto;
	label[3] = domain;

	zassert_equal(ARRAY_SIZE(query),
		      dns_sd_query_extract(query, ARRAY_SIZE(query), &record, label, size, &n),
		      "failed to extract service type enumeration");

	zassert_true(dns_sd_is_service_type_enumeration(&record), "");
}

ZTEST(dns_sd, test_wildcard_comparison)
{
	size_t n_matches = 0;
	size_t n_records = 0;
	struct dns_sd_rec filter;

	dns_sd_create_wildcard_filter(&filter);

	DNS_SD_FOREACH(record) {
		if (!rec_is_valid(record)) {
			continue;
		}

		++n_records;
	}

	DNS_SD_FOREACH(record) {
		if (!rec_is_valid(record)) {
			continue;
		}

		if (dns_sd_rec_match(record, &filter)) {
			++n_matches;
		}
	}

	zassert_true(n_records > 0, "there must be > 0 records");
	zassert_equal(n_matches, n_records, "wildcard filter does not match "
		"all records: n_records: %zu n_matches: %zu", n_records, n_matches);
}

ZTEST_SUITE(dns_sd, NULL, NULL, NULL, NULL, NULL);
