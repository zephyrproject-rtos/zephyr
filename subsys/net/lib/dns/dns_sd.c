/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <net/net_context.h>
#include <net/net_core.h>
#include <net/dns_sd.h>
#include <sys/util.h>
#include <zephyr.h>

#include "dns_pack.h"
#include "dns_sd.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(net_dns_sd, CONFIG_DNS_SD_LOG_LEVEL);

const char dns_sd_empty_txt[1];

#ifndef CONFIG_NET_TEST

static size_t service_proto_size(const struct dns_sd_rec *inst);
static bool label_is_valid(const char *label, size_t label_size);
static int add_a_record(const struct dns_sd_rec *inst, uint32_t ttl,
			uint16_t host_offset, uint32_t addr,
			uint8_t *buf,
			uint16_t buf_offset, uint16_t buf_size);
static int add_ptr_record(const struct dns_sd_rec *inst, uint32_t ttl,
			  uint8_t *buf, uint16_t buf_offset,
			  uint16_t buf_size,
			  uint16_t *service_offset,
			  uint16_t *instance_offset,
			  uint16_t *domain_offset);
static int add_txt_record(const struct dns_sd_rec *inst, uint32_t ttl,
			  uint16_t instance_offset, uint8_t *buf,
			  uint16_t buf_offset, uint16_t buf_size);
static int add_aaaa_record(const struct dns_sd_rec *inst, uint32_t ttl,
			   uint16_t host_offset, const uint8_t addr[16],
			   uint8_t *buf, uint16_t buf_offset,
			   uint16_t buf_size);
static int add_srv_record(const struct dns_sd_rec *inst, uint32_t ttl,
			  uint16_t instance_offset,
			  uint16_t domain_offset,
			  uint8_t *buf, uint16_t buf_offset,
			  uint16_t buf_size,
			  uint16_t *host_offset);
static bool rec_is_valid(const struct dns_sd_rec *inst);

#endif /* CONFIG_NET_TEST */

/**
 * Calculate the size of a DNS-SD service
 *
 * This macro calculates the size of the DNS-SD service for a DNS
 * Resource Record (RR).
 *
 * For example, if there is a service called 'My Foo'._http._tcp.local.,
 * then the returned size is 18. That is broken down as shown below.
 *
 * - 1 byte for the size of "_http"
 * - 5 bytes for the value of "_http"
 * - 1 byte for the size of "_tcp"
 * - 4 bytes for the value of "_tcp"
 * - 1 byte for the size of "local"
 * - 5 bytes for the value of "local"
 * - 1 byte for the trailing NUL terminator '\0'
 *
 * @param ref the DNS-SD record
 * @return the size of the DNS-SD service for a DNS Resource Record
 */
size_t service_proto_size(const struct dns_sd_rec *ref)
{
	return 0
	       + DNS_LABEL_LEN_SIZE + strlen(ref->service)
	       + DNS_LABEL_LEN_SIZE + strlen(ref->proto)
	       + DNS_LABEL_LEN_SIZE + strlen(ref->domain)
	       + DNS_LABEL_LEN_SIZE
	;
}

/**
 * Check Label Validity according to RFC 1035, Section 3.5
 *
 * <label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]
 * <ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>
 * <let-dig-hyp> ::= <let-dig> | -
 * <let-dig> ::= <letter> | <digit>
 * <letter> ::= [a-zA-Z]
 * <digit> ::= [0-9]
 */
bool label_is_valid(const char *label, size_t label_size)
{
	size_t i;

	if (label == NULL) {
		return false;
	}

	if (label_size == 0) {
		/* automatically calculate the length of the string */
		label_size = strlen(label);
	}

	if (label_size < DNS_LABEL_MIN_SIZE ||
	    label_size > DNS_LABEL_MAX_SIZE) {
		return false;
	}

	for (i = 0; i < label_size; ++i) {
		if (isalpha((int)label[i])) {
			continue;
		}

		if (i > 0) {
			if (isdigit((int)label[i])) {
				continue;
			}

			if ('-' == label[i]) {
				continue;
			}
		}

		return false;
	}

	return true;
}

static bool instance_is_valid(const char *instance)
{
	size_t i;
	size_t instance_size;

	if (instance == NULL) {
		NET_DBG("label is NULL");
		return false;
	}

	instance_size = strlen(instance);
	if (instance_size < DNS_SD_INSTANCE_MIN_SIZE) {
		NET_DBG("label '%s' is too small (%zu, min: %u)",
			instance, instance_size,
			DNS_SD_INSTANCE_MIN_SIZE);
		return false;
	}

	if (instance_size > DNS_SD_INSTANCE_MAX_SIZE) {
		NET_DBG("label '%s' is too big (%zu, max: %u)",
			instance, instance_size,
			DNS_SD_INSTANCE_MAX_SIZE);
		return false;
	}

	for (i = 0; i < instance_size; ++i) {
		/* RFC 6763 Section 4.1.1 */
		if (instance[i] <= 0x1f ||
		    instance[i] == 0x7f) {
			NET_DBG(
				"instance '%s' contains illegal byte 0x%02x",
				instance, instance[i]);
			return false;
		}
	}

	return instance_size;
}

static bool service_is_valid(const char *service)
{
	size_t service_size;

	if (service == NULL) {
		NET_DBG("label is NULL");
		return false;
	}

	service_size = strlen(service);
	if (service_size < DNS_SD_SERVICE_MIN_SIZE) {
		NET_DBG("label '%s' is too small (%zu, min: %u)",
			service, service_size, DNS_SD_SERVICE_MIN_SIZE);
		return false;
	}

	if (service_size > DNS_SD_SERVICE_MAX_SIZE) {
		NET_DBG("label '%s' is too big (%zu, max: %u)",
			service, service_size, DNS_SD_SERVICE_MAX_SIZE);
		return false;
	}

	if (service[0] != DNS_SD_SERVICE_PREFIX) {
		NET_DBG("service '%s' invalid (no leading underscore)",
			service);
		return false;
	}

	if (!label_is_valid(&service[1], service_size - 1)) {
		NET_DBG("service '%s' contains invalid characters",
			service);
		return false;
	}

	return service_size;
}

static bool proto_is_valid(const char *proto)
{
	size_t proto_size;

	if (proto == NULL) {
		NET_DBG("label is NULL");
		return false;
	}

	proto_size = strlen(proto);
	if (proto_size != DNS_SD_PROTO_SIZE) {
		NET_DBG("label '%s' wrong size (%zu, exp: %u)",
			proto, proto_size, DNS_SD_PROTO_SIZE);
		return false;
	}

	if (!(strncasecmp("_tcp", proto, DNS_SD_PROTO_SIZE) == 0 ||
	      strncasecmp("_udp", proto, DNS_SD_PROTO_SIZE) == 0)) {
		/* RFC 1034 Section 3.1 */
		NET_DBG("proto '%s' is invalid (not _tcp or _udp)",
			proto);
		return false;
	}

	return proto_size;
}

static bool domain_is_valid(const char *domain)
{
	size_t domain_size;

	if (domain == NULL) {
		NET_DBG("label is NULL");
		return false;
	}

	domain_size = strlen(domain);
	if (domain_size < DNS_SD_DOMAIN_MIN_SIZE) {
		NET_DBG("label '%s' is too small (%zu, min: %u)",
			domain, domain_size, DNS_SD_DOMAIN_MIN_SIZE);
		return false;
	}

	if (domain_size > DNS_SD_DOMAIN_MAX_SIZE) {
		NET_DBG("label '%s' is too big (%zu, max: %u)",
			domain, domain_size, DNS_SD_DOMAIN_MAX_SIZE);
		return false;
	}

	if (!label_is_valid(domain, domain_size)) {
		NET_DBG("domain '%s' contains invalid characters",
			domain);
		return false;
	}

	return domain_size;
}

/**
 * Check DNS SD Record for validity
 *
 * Our records are in the form <Instance>.<Service>.<Proto>.<Domain>
 *
 * Currently, <Subdomain>.<Domain> services are not supported.
 */
bool rec_is_valid(const struct dns_sd_rec *inst)
{
	return true
	       && inst != NULL
	       && instance_is_valid(inst->instance)
	       && service_is_valid(inst->service)
	       && proto_is_valid(inst->proto)
	       && domain_is_valid(inst->domain)
	       && inst->text != NULL
	       && inst->port != NULL
	;
}

int add_a_record(const struct dns_sd_rec *inst, uint32_t ttl,
		 uint16_t host_offset, uint32_t addr, uint8_t *buf,
		 uint16_t buf_offset, uint16_t buf_size)
{
	uint16_t total_size;
	struct dns_rr *rr;
	struct dns_a_rdata *rdata;
	uint16_t inst_offs;
	uint16_t offset = buf_offset;

	if ((DNS_SD_PTR_MASK & host_offset) != 0) {
		NET_DBG("offset %u too big for message compression",
			host_offset);
		return -E2BIG;
	}

	/* First, calculate that there is enough space in the buffer */
	total_size =
		/* pointer to .<Instance>.local. */
		2 + sizeof(*rr) + sizeof(*rdata);

	if (offset > buf_size || total_size >= buf_size - offset) {
		NET_DBG("Buffer too small. required: %u available: %d",
			total_size, (int)buf_size - (int)offset);
		return -ENOSPC;
	}

	/* insert a pointer to the instance + service name */
	inst_offs = host_offset;
	inst_offs |= DNS_SD_PTR_MASK;
	inst_offs = htons(inst_offs);
	memcpy(&buf[offset], &inst_offs, sizeof(inst_offs));
	offset += sizeof(inst_offs);

	rr = (struct dns_rr *)&buf[offset];
	rr->type = htons(DNS_RR_TYPE_A);
	rr->class_ = htons(DNS_CLASS_IN | DNS_CLASS_FLUSH);
	rr->ttl = htonl(ttl);
	rr->rdlength = htons(sizeof(*rdata));
	offset += sizeof(*rr);

	rdata = (struct dns_a_rdata *)&buf[offset];
	rdata->address = htonl(addr);
	offset += sizeof(*rdata);

	__ASSERT_NO_MSG(total_size == offset - buf_offset);

	return offset - buf_offset;
}

int add_ptr_record(const struct dns_sd_rec *inst, uint32_t ttl,
		   uint8_t *buf, uint16_t buf_offset, uint16_t buf_size,
		   uint16_t *service_offset, uint16_t *instance_offset,
		   uint16_t *domain_offset)
{
	uint8_t i;
	int name_size;
	struct dns_rr *rr;
	uint16_t svc_offs;
	uint16_t inst_offs;
	uint16_t dom_offs;
	size_t label_size;
	uint16_t sp_size;
	uint16_t offset = buf_offset;
	const char *labels[] = {
		inst->instance,
		inst->service,
		inst->proto,
		inst->domain,
	};

	/* First, ensure that labels and full name are within spec */
	if (!rec_is_valid(inst)) {
		return -EINVAL;
	}

	sp_size = service_proto_size(inst);

	/*
	 * Next, calculate that there is enough space in the buffer.
	 *
	 * We require that this is the first time names will appear in the
	 * DNS message. Message Compression is used in subsequent
	 * calculations.
	 *
	 * That is the reason there is an output variable for
	 * service_offset and instance_offset.
	 *
	 * For more information on DNS Message Compression, see
	 * RFC 1035, Section 4.1.4.
	 */
	name_size =
		/* uncompressed. e.g. "._foo._tcp.local." */
		sp_size +
		sizeof(*rr)
		/* compressed e.g. .My Foo" followed by (DNS_SD_PTR_MASK | 0x0abc) */
		+ 1 + strlen(inst->instance) + 2;

	if (offset > buf_size || name_size >= buf_size - offset) {
		NET_DBG("Buffer too small. required: %u available: %d",
			name_size, (int)buf_size - (int)offset);
		return -ENOSPC;
	}

	svc_offs = offset;
	if ((svc_offs & DNS_SD_PTR_MASK) != 0) {
		NET_DBG("offset %u too big for message compression",
			svc_offs);
		return -E2BIG;
	}

	inst_offs = offset + sp_size + sizeof(*rr);
	if ((inst_offs & DNS_SD_PTR_MASK) != 0) {
		NET_DBG("offset %u too big for message compression",
			inst_offs);
		return -E2BIG;
	}

	dom_offs = offset + sp_size - 1 -
		   strlen(inst->domain) - 1;

	/* Finally, write output with confidence that doing so is safe */

	*service_offset = svc_offs;
	*instance_offset = inst_offs;
	*domain_offset = dom_offs;

	/* copy the service name. e.g. "._foo._tcp.local." */
	for (i = 1; i < ARRAY_SIZE(labels); ++i) {
		label_size = strlen(labels[i]);
		buf[offset++] = strlen(labels[i]);
		memcpy(&buf[offset], labels[i], label_size);
		offset += label_size;
		if (i == ARRAY_SIZE(labels) - 1) {
			/* terminator */
			buf[offset++] = '\0';
		}
	}

	__ASSERT_NO_MSG(svc_offs + sp_size == offset);

	rr = (struct dns_rr *)&buf[offset];
	rr->type = htons(DNS_RR_TYPE_PTR);
	rr->class_ = htons(DNS_CLASS_IN);
	rr->ttl = htonl(ttl);
	rr->rdlength = htons(
		DNS_LABEL_LEN_SIZE +
		strlen(inst->instance)
		+ DNS_POINTER_SIZE);
	offset += sizeof(*rr);

	__ASSERT_NO_MSG(inst_offs == offset);

	/* copy the instance size, value, and add a pointer */
	label_size = strlen(inst->instance);
	buf[offset++] = label_size;
	memcpy(&buf[offset], inst->instance, label_size);
	offset += label_size;

	svc_offs |= DNS_SD_PTR_MASK;
	svc_offs = htons(svc_offs);
	memcpy(&buf[offset], &svc_offs, sizeof(svc_offs));
	offset += sizeof(svc_offs);

	__ASSERT_NO_MSG(name_size == offset - buf_offset);

	return offset - buf_offset;
}

int add_txt_record(const struct dns_sd_rec *inst, uint32_t ttl,
		   uint16_t instance_offset, uint8_t *buf,
		   uint16_t buf_offset, uint16_t buf_size)
{
	uint16_t total_size;
	struct dns_rr *rr;
	uint16_t inst_offs;
	uint16_t offset = buf_offset;

	if ((DNS_SD_PTR_MASK & instance_offset) != 0) {
		NET_DBG("offset %u too big for message compression",
			instance_offset);
		return -E2BIG;
	}

	/* First, calculate that there is enough space in the buffer */
	total_size =
		/* pointer to .<Instance>.<Service>.<Protocol>.local. */
		DNS_POINTER_SIZE + sizeof(*rr) + dns_sd_txt_size(inst);

	if (offset > buf_size || total_size >= buf_size - offset) {
		NET_DBG("Buffer too small. required: %u available: %d",
			total_size, (int)buf_size - (int)offset);
		return -ENOSPC;
	}

	/* insert a pointer to the instance + service name */
	inst_offs = instance_offset;
	inst_offs |= DNS_SD_PTR_MASK;
	inst_offs = htons(inst_offs);
	memcpy(&buf[offset], &inst_offs, sizeof(inst_offs));
	offset += sizeof(inst_offs);

	rr = (struct dns_rr *)&buf[offset];
	rr->type = htons(DNS_RR_TYPE_TXT);
	rr->class_ = htons(DNS_CLASS_IN | DNS_CLASS_FLUSH);
	rr->ttl = htonl(ttl);
	rr->rdlength = htons(dns_sd_txt_size(inst));
	offset += sizeof(*rr);

	memcpy(&buf[offset], inst->text, dns_sd_txt_size(inst));
	offset += dns_sd_txt_size(inst);

	__ASSERT_NO_MSG(total_size == offset - buf_offset);

	return offset - buf_offset;
}

int add_aaaa_record(const struct dns_sd_rec *inst, uint32_t ttl,
		    uint16_t host_offset, const uint8_t addr[16],
		    uint8_t *buf, uint16_t buf_offset, uint16_t buf_size)
{
	uint16_t total_size;
	struct dns_rr *rr;
	struct dns_aaaa_rdata *rdata;
	uint16_t inst_offs;
	uint16_t offset = buf_offset;

	if ((DNS_SD_PTR_MASK & host_offset) != 0) {
		NET_DBG("offset %u too big for message compression",
			host_offset);
		return -E2BIG;
	}

	/* First, calculate that there is enough space in the buffer */
	total_size =
		/* pointer to .<Instance>.local. */
		DNS_POINTER_SIZE + sizeof(*rr) + sizeof(*rdata);

	if (offset > buf_size || total_size >= buf_size - offset) {
		NET_DBG("Buffer too small. required: %u available: %d",
			total_size, (int)buf_size - (int)offset);
		return -ENOSPC;
	}

	/* insert a pointer to the instance + service name */
	inst_offs = host_offset;
	inst_offs |= DNS_SD_PTR_MASK;
	inst_offs = htons(inst_offs);
	memcpy(&buf[offset], &inst_offs, sizeof(inst_offs));
	offset += sizeof(inst_offs);

	rr = (struct dns_rr *)&buf[offset];
	rr->type = htons(DNS_RR_TYPE_AAAA);
	rr->class_ = htons(DNS_CLASS_IN | DNS_CLASS_FLUSH);
	rr->ttl = htonl(ttl);
	rr->rdlength = htons(sizeof(*rdata));
	offset += sizeof(*rr);

	rdata = (struct dns_aaaa_rdata *)&buf[offset];
	memcpy(rdata->address, addr, sizeof(*rdata));
	offset += sizeof(*rdata);

	__ASSERT_NO_MSG(total_size == offset - buf_offset);

	return offset - buf_offset;
}

int add_srv_record(const struct dns_sd_rec *inst, uint32_t ttl,
		   uint16_t instance_offset, uint16_t domain_offset,
		   uint8_t *buf, uint16_t buf_offset, uint16_t buf_size,
		   uint16_t *host_offset)
{
	uint16_t total_size;
	struct dns_rr *rr;
	struct dns_srv_rdata *rdata;
	size_t label_size;
	uint16_t inst_offs;
	uint16_t offset = buf_offset;

	if ((DNS_SD_PTR_MASK & instance_offset) != 0) {
		NET_DBG("offset %u too big for message compression",
			instance_offset);
		return -E2BIG;
	}

	if ((DNS_SD_PTR_MASK & domain_offset) != 0) {
		NET_DBG("offset %u too big for message compression",
			domain_offset);
		return -E2BIG;
	}

	/* First, calculate that there is enough space in the buffer */
	total_size =
		/* pointer to .<Instance>.<Service>.<Protocol>.local. */
		DNS_POINTER_SIZE + sizeof(*rr)
		+ sizeof(*rdata)
		/* .<Instance> */
		+ DNS_LABEL_LEN_SIZE
		+ strlen(inst->instance)
		/* pointer to .local. */
		+ DNS_POINTER_SIZE;

	if (offset > buf_size || total_size >= buf_size - offset) {
		NET_DBG("Buffer too small. required: %u available: %d",
			total_size, (int)buf_size - (int)offset);
		return -ENOSPC;
	}

	/* insert a pointer to the instance + service name */
	inst_offs = instance_offset;
	inst_offs |= DNS_SD_PTR_MASK;
	inst_offs = htons(inst_offs);
	memcpy(&buf[offset], &inst_offs, sizeof(inst_offs));
	offset += sizeof(inst_offs);

	rr = (struct dns_rr *)&buf[offset];
	rr->type = htons(DNS_RR_TYPE_SRV);
	rr->class_ = htons(DNS_CLASS_IN | DNS_CLASS_FLUSH);
	rr->ttl = htonl(ttl);
	/* .<Instance>.local. */
	rr->rdlength = htons(sizeof(*rdata) + DNS_LABEL_LEN_SIZE
			     + strlen(inst->instance) +
			     DNS_POINTER_SIZE);
	offset += sizeof(*rr);

	rdata = (struct dns_srv_rdata *)&buf[offset];
	rdata->priority = 0;
	rdata->weight = 0;
	rdata->port = *(inst->port);
	offset += sizeof(*rdata);

	*host_offset = offset;

	label_size = strlen(inst->instance);
	buf[offset++] = label_size;
	memcpy(&buf[offset], inst->instance, label_size);
	offset += label_size;

	domain_offset |= DNS_SD_PTR_MASK;
	domain_offset = htons(domain_offset);
	memcpy(&buf[offset], &domain_offset, sizeof(domain_offset));
	offset += sizeof(domain_offset);

	__ASSERT_NO_MSG(total_size == offset - buf_offset);

	return offset - buf_offset;
}

#ifndef CONFIG_NET_TEST
static bool port_in_use_sockaddr(uint16_t proto, uint16_t port,
	const struct sockaddr *addr)
{
	const struct sockaddr_in any = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
	};
	const struct sockaddr_in6 any6 = {
		.sin6_family = AF_INET6,
		.sin6_addr = in6addr_any,
	};
	const struct sockaddr *anyp =
		(addr->sa_family == AF_INET)
		? (const struct sockaddr *) &any
		: (const struct sockaddr *) &any6;

	return
		net_context_port_in_use(proto, port, addr)
		|| net_context_port_in_use(proto, port, anyp);
}

static bool port_in_use(uint16_t proto, uint16_t port, const struct in_addr *addr4,
	const struct in6_addr *addr6)
{
	bool r;
	struct sockaddr sa;

	if (addr4 != NULL) {
		net_sin(&sa)->sin_family = AF_INET;
		net_sin(&sa)->sin_addr = *addr4;

		r = port_in_use_sockaddr(proto, port, &sa);
		if (r) {
			return true;
		}
	}

	if (addr6 != NULL) {
		net_sin6(&sa)->sin6_family = AF_INET6;
		net_sin6(&sa)->sin6_addr = *addr6;

		r = port_in_use_sockaddr(proto, port, &sa);
		if (r) {
			return true;
		}
	}

	return false;
}
#else /* CONFIG_NET_TEST */
static inline bool port_in_use(uint16_t proto, uint16_t port, const struct in_addr *addr4,
	const struct in6_addr *addr6)
{
	ARG_UNUSED(port);
	ARG_UNUSED(addr4);
	ARG_UNUSED(addr6);
	return true;
}
#endif /* CONFIG_NET_TEST */

int dns_sd_handle_ptr_query(const struct dns_sd_rec *inst,
	const struct in_addr *addr4, const struct in6_addr *addr6,
	uint8_t *buf, uint16_t buf_size)
{
	/*
	 * RFC 6763 Section 12.1
	 *
	 * When including a DNS-SD Service Instance Enumeration or Selective
	 * Instance Enumeration (subtype) PTR record in a response packet, the
	 * server/responder SHOULD include the following additional records:
	 *
	 * o  The SRV record(s) named in the PTR rdata.
	 * o  The TXT record(s) named in the PTR rdata.
	 * o  All address records (type "A" and "AAAA") named in the SRV rdata.
	 *	contain the SRV record(s), the TXT record(s), and the address
	 *      records (A or AAAA)
	 */

	uint16_t instance_offset;
	uint16_t service_offset;
	uint16_t domain_offset;
	uint16_t host_offset;
	uint16_t proto;
	uint16_t offset = sizeof(struct dns_header);
	struct dns_header *rsp = (struct dns_header *)buf;
	uint32_t tmp;
	int r;

	memset(rsp, 0, sizeof(*rsp));

	if (!rec_is_valid(inst)) {
		return -EINVAL;
	}

	if (*(inst->port) == 0) {
		NET_DBG("Ephemeral port %u for %s.%s.%s.%s "
			"not initialized", ntohs(*(inst->port)),
			inst->instance, inst->service, inst->proto,
			inst->domain);
		return -EHOSTDOWN;
	}

	if (strncmp("_tcp", inst->proto, DNS_SD_PROTO_SIZE) == 0) {
		proto = IPPROTO_TCP;
	} else if (strncmp("_udp", inst->proto, DNS_SD_PROTO_SIZE) == 0) {
		proto = IPPROTO_UDP;
	} else {
		NET_DBG("invalid protocol %s", inst->proto);
		return -EINVAL;
	}

	if (!port_in_use(proto, ntohs(*(inst->port)), addr4, addr6)) {
		/* Service is not yet bound, so do not advertise */
		return -EHOSTDOWN;
	}

	/* first add the answer record */
	r = add_ptr_record(inst, DNS_SD_PTR_TTL, buf, offset,
			   buf_size - offset,
			   &service_offset, &instance_offset,
			   &domain_offset);
	if (r < 0) {
		return r; /* LCOV_EXCL_LINE */
	}

	rsp->ancount++;
	offset += r;

	/* then add the additional records */
	r = add_txt_record(inst, DNS_SD_TXT_TTL, instance_offset, buf,
			   offset,
			   buf_size - offset);
	if (r < 0) {
		return r; /* LCOV_EXCL_LINE */
	}

	rsp->arcount++;
	offset += r;

	r = add_srv_record(inst, DNS_SD_SRV_TTL, instance_offset,
			   domain_offset,
			   buf, offset, buf_size - offset, &host_offset);
	if (r < 0) {
		return r; /* LCOV_EXCL_LINE */
	}

	rsp->arcount++;
	offset += r;

	if (addr6 != NULL) {
		r = add_aaaa_record(inst, DNS_SD_AAAA_TTL, host_offset,
				    addr6->s6_addr,
				    buf, offset,
				    buf_size - offset); /* LCOV_EXCL_LINE */
		if (r < 0) {
			return r;                       /* LCOV_EXCL_LINE */
		}

		rsp->arcount++;
		offset += r;
	}

	if (addr4 != NULL) {
		tmp = htonl(*(addr4->s4_addr32));
		r = add_a_record(inst, DNS_SD_A_TTL, host_offset,
				 tmp, buf, offset,
				 buf_size - offset);
		if (r < 0) {
			return r; /* LCOV_EXCL_LINE */
		}

		rsp->arcount++;
		offset += r;
	}

	/* Set the Response and AA bits */
	rsp->flags = htons(BIT(15) | BIT(10));
	rsp->ancount = htons(rsp->ancount);
	rsp->arcount = htons(rsp->arcount);

	return offset;
}

/* TODO: dns_sd_handle_srv_query() */
/* TODO: dns_sd_handle_txt_query() */

bool dns_sd_rec_match(const struct dns_sd_rec *record,
		      const struct dns_sd_rec *filter)
{
	size_t i;
	const char *rec_label;
	const char *filt_label;

	static bool (*checkers[])(const char *) = {
		instance_is_valid,
		service_is_valid,
		proto_is_valid,
		domain_is_valid,
	};

	static const char *names[] = {
		"instance",
		"service",
		"protocol",
		"domain",
	};

	if (!rec_is_valid(record)) {
		LOG_WRN("DNS SD record at %p is invalid", record);
		return false;
	}

	if (filter == NULL) {
		return false;
	}

	/* Deref only after it is deemed safe to do so */
	const char *const pairs[] = {
		record->instance, filter->instance,
		record->service, filter->service,
		record->proto, filter->proto,
		record->domain, filter->domain,
	};

	BUILD_ASSERT(ARRAY_SIZE(pairs) == 2 * ARRAY_SIZE(checkers));
	BUILD_ASSERT(ARRAY_SIZE(names) == ARRAY_SIZE(checkers));

	for (i = 0; i < ARRAY_SIZE(checkers); ++i) {
		rec_label = pairs[2 * i];
		filt_label = pairs[2 * i + 1];

		/* check for the "wildcard" pointer */
		if (filt_label != NULL) {
			if (!checkers[i](filt_label)) {
				LOG_WRN("invalid %s label: '%s'",
					names[i], filt_label);
				return false;
			}

			if (strncasecmp(rec_label, filt_label,
					DNS_LABEL_MAX_SIZE) != 0) {
				return false;
			}
		}
	}

	/* check for the "wildcard" port */
	if (filter->port != NULL && *(record->port) != *(filter->port)) {
		return false;
	}

	return true;
}

int dns_sd_extract_service_proto_domain(const uint8_t *query,
	size_t query_size, struct dns_sd_rec *record, char *service,
	size_t service_size, char *proto, size_t proto_size, char *domain,
	size_t domain_size)
{
	uint16_t offs;
	uint8_t label_size;

	if (query == NULL || record == NULL || service == NULL
	    || proto == NULL || domain == NULL) {
		NET_DBG("one or more arguments are NULL");
		return -EINVAL;
	}

	if (query_size <= DNS_MSG_HEADER_SIZE
	    || service_size < DNS_SD_SERVICE_MAX_SIZE + 1
	    || proto_size < DNS_SD_PROTO_SIZE + 1
	    || domain_size < DNS_SD_DOMAIN_MAX_SIZE + 1
	    ) {
		NET_DBG("one or more size arguments are too small");
		return -EINVAL;
	}

	memset(record, 0, sizeof(*record));
	offs = DNS_MSG_HEADER_SIZE;

	/* Copy service label to '\0'-terminated buffer */
	label_size = query[offs];
	if (label_size == 0  || label_size > service_size - 1
	    || offs + label_size > query_size) {
		NET_DBG("could not get service");
		return -EINVAL;
	} else {
		strncpy(service, &query[offs + 1], label_size);
		service[label_size] = '\0';
		offs += label_size + 1;
	}

	/* Copy proto label to '\0'-terminated buffer */
	label_size = query[offs];
	if (label_size == 0 || label_size > proto_size - 1
	    || offs + label_size > query_size) {
		NET_DBG("could not get proto for '%s...'", log_strdup(service));
		return -EINVAL;
	} else {
		strncpy(proto, &query[offs + 1], label_size);
		proto[label_size] = '\0';
		offs += label_size + 1;
	}

	/* Copy domain label to '\0'-terminated buffer */
	label_size = query[offs];
	if (label_size == 0 || label_size > domain_size - 1
	    || offs + label_size > query_size) {
		NET_DBG("could not get domain for '%s.%s...'",
			log_strdup(service), log_strdup(proto));
		return -EINVAL;
	} else {
		strncpy(domain, &query[offs + 1], label_size);
		domain[label_size] = '\0';
		offs += label_size + 1;
	}

	/* Check that we have reached the DNS terminator */
	if (query[offs] != 0) {
		NET_DBG("ignoring request for '%s.%s.%s...'",
			log_strdup(service),
			log_strdup(proto),
			log_strdup(domain));
		return -EINVAL;
	}

	offs++;
	record->service = service;
	record->proto = proto;
	record->domain = domain;

	return offs;
}
