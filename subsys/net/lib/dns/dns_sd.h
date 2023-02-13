/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DNS_SD_H_
#define DNS_SD_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/net/dns_sd.h>
#include <zephyr/net/net_ip.h>

#include "dns_pack.h"

/* TODO: Move these into Kconfig */
#define DNS_SD_PTR_TTL 4500
#define DNS_SD_TXT_TTL 4500
#define DNS_SD_SRV_TTL 120
#define DNS_SD_A_TTL 120
#define DNS_SD_AAAA_TTL 120

#define DNS_SD_PTR_MASK (NS_CMPRSFLGS << 8)

#ifdef __cplusplus
extern "C" {
#endif

#define DNS_SD_FOREACH(it) \
	STRUCT_SECTION_FOREACH(dns_sd_rec, it)

/**
 * @brief Extract labels from a DNS-SD PTR query
 *
 * ```
 *            <sn>._tcp.<domain>.
 * <instance>.<sn>._tcp.<domain>.
 * ```
 *
 * Currently sub-types and service domains are unsupported and only the
 * "local" domain is supported. Specifically, that excludes the following:
 * ```
 * <sub>._sub.<sn>._tcp.<servicedomain>.<parentdomain>.
 * ```
 *
 * @param query a pointer to the start of the query
 * @param query_size the number of bytes contained in the query
 * @param[out] record the DNS-SD record to initialize and populate
 * @param label array of pointers to suitably sized buffers
 * @param size array of sizes for each buffer in @p label
 * @param[inout] n number of elements in @p label and @p size
 *
 * @return on success, number of bytes read from @p query
 * @return on failure, a negative errno value
 *
 * @see <a href="https://datatracker.ietf.org/doc/html/rfc6763">RFC 6763</a>, Section 7.2.
 */
int dns_sd_query_extract(const uint8_t *query, size_t query_size, struct dns_sd_rec *record,
			 char **label, size_t *size, size_t *n);

/**
 * @brief Extract the Service, Protocol, and Domain from a DNS-SD PTR query
 *
 * This function zero-initializes @p record and populates the appropriate
 * fields so that @p record may be subsequently passed to @ref dns_sd_rec_match.
 *
 * If a query with a supported format is found, the function returns the
 * length of the initial, variable-length portion of the query.
 *
 * For example, if the query begins with "._foo._tcp.local.", where
 * the '.' character represents a length of the subsequent string value,
 * then this function will return 17.
 *
 * @param query a pointer to the start of the query
 * @param query_size the number of bytes contained in the query
 * @param[out] record the DNS-SD record to initialize and populate
 * @param service buffer to store the null-terminated service
 * @param service_size the size of @p service
 * @param proto buffer to store the null-terminated proto
 * @param proto_size the size of @p proto
 * @param domain buffer to store the null-terminated domain
 * @param domain_size the size of @p domain
 * @return on success, a positive number representing length of the query
 * @return on failure, a negative errno value
 */
__deprecated
int dns_sd_extract_service_proto_domain(const uint8_t *query,
	size_t query_size, struct dns_sd_rec *record, char *service,
	size_t service_size, char *proto, size_t proto_size,
	char *domain, size_t domain_size);

/**
 * @brief See if the DNS SD @p filter matches the @p record
 *
 * The fields in @p filter should be populated with filter elements to
 * identify a possible match. If pointer fields are set to NULL, they
 * act as a wildcard in the matching process. I.e. they will match
 * anything. Similarly, the @ref dns_sd_rec.port field may be set to 0
 * to be used as a wildcard.
 *
 * The @ref dns_sd_rec.text and @ref dns_ds_rec.text_size fields
 * are not included in the matching process.
 *
 * For example, the filter below can be used to match any
 * "_http._tcp" records.
 *
 * @code{c}
 * const struct dns_sd_rec *it;
 * struct dns_sd_rec filter = {
 *     // match any instance
 *     .instance = NULL,
 *     // match records with service "_http"
 *     .service = "_http",
 *     // match records with protocol "_tcp"
 *     .proto = "_tcp",
 *     // match any domain
 *     .domain = NULL,
 *     // match any port
 *     .port = 0,
 * };
 *
 * DNS_SD_FOREACH(it) {
 *     if (dns_sd_rec_match(it, filter)) {
 *         // found a match!
 *     }
 * }
 * @endcode{c}
 *
 * @param record The reference DNS-SD record
 * @param filter The DNS-SD record filter
 *
 * @return true if the @p record matches the @p filter
 * @return false if @p record is not a match for @p filter
 * @return false if either @p record or @p filter are invalid
 */
bool dns_sd_rec_match(const struct dns_sd_rec *record,
	const struct dns_sd_rec *filter);

/**
 * @brief Handle a DNS PTR Query with DNS Service Discovery
 *
 * This function should be called once for each DNS-SD record that
 * matches a particular DNS PTR query.
 *
 * If there is no IPv4 address to advertise, then @p addr4 should be
 * NULL.
 *
 * If there is no IPv6 address to advertise, then @p addr6 should be
 * NULL.
 *
 * @param inst the DNS-SD record to advertise
 * @param addr4 pointer to the IPv4 address
 * @param addr6 pointer to the IPv6 address
 * @param buf output buffer
 * @param buf_size size of the output buffer
 *
 * @return on success, number of bytes written to @p buf
 * @return on failure, a negative errno value
 */
int dns_sd_handle_ptr_query(const struct dns_sd_rec *inst,
	const struct in_addr *addr4, const struct in6_addr *addr6,
	uint8_t *buf, uint16_t buf_size);

/**
 * @brief Handle a Service Type Enumeration with DNS Service Discovery
 *
 * This function should be called once for each type of advertised service.
 *
 * @param service the DNS-SD service to advertise
 * @param addr4 pointer to the IPv4 address
 * @param addr6 pointer to the IPv6 address
 * @param buf output buffer
 * @param buf_size size of the output buffer
 *
 * @return on success, number of bytes written to @p buf
 * @return on failure, a negative errno value
 */
int dns_sd_handle_service_type_enum(const struct dns_sd_rec *service,
	const struct in_addr *addr4, const struct in6_addr *addr6,
	uint8_t *buf, uint16_t buf_size);

#ifdef __cplusplus
};
#endif

#endif /* DNS_SD_H_ */
