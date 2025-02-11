/** @file
 *  @brief DNS Service Discovery
 */

/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_DNS_SD_H_
#define ZEPHYR_INCLUDE_NET_DNS_SD_H_

#include <stdint.h>
#include <zephyr/sys/byteorder.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DNS Service Discovery
 *
 * @details This API enables services to be advertised via DNS. To
 * advertise a service, system or application code should use
 * @ref DNS_SD_REGISTER_TCP_SERVICE or
 * @ref DNS_SD_REGISTER_UDP_SERVICE.
 *
 * @see <a href="https://tools.ietf.org/html/rfc6763">RFC 6763</a>
 *
 * @defgroup dns_sd DNS Service Discovery
 * @ingroup networking
 * @{
 */

/** RFC 1034 Section 3.1 */
#define DNS_SD_INSTANCE_MIN_SIZE 1
/** RFC 1034 Section 3.1, RFC 6763 Section 7.2 */
#define DNS_SD_INSTANCE_MAX_SIZE 63
/** RFC 6763 Section 7.2 - inclusive of underscore */
#define DNS_SD_SERVICE_MIN_SIZE 2
/** RFC 6763 Section 7.2 - inclusive of underscore */
#define DNS_SD_SERVICE_MAX_SIZE 16
/** RFC 6763 Section 4.1.2 */
#define DNS_SD_SERVICE_PREFIX '_'
/** RFC 6763 Section 4.1.2 - either _tcp or _udp (case insensitive) */
#define DNS_SD_PROTO_SIZE 4
/** ICANN Rules for TLD naming */
#define DNS_SD_DOMAIN_MIN_SIZE 2
/** RFC 1034 Section 3.1, RFC 6763 Section 7.2 */
#define DNS_SD_DOMAIN_MAX_SIZE 63

/**
 * Minimum number of segments in a fully-qualified name
 *
 * This represents FQN's of the form below
 * ```
 * <sn>._tcp.<domain>.
 * ```
 * Currently sub-types and service domains are unsupported and only the
 * "local" domain is supported. Specifically, that excludes the following:
 * ```
 * <sub>._sub.<sn>._tcp.<servicedomain>.<parentdomain>.
 * ```
 * @see <a href="https://datatracker.ietf.org/doc/html/rfc6763">RFC 6763</a>, Section 7.2.
 */
#define DNS_SD_MIN_LABELS 3
/**
 * Maximum number of segments in a fully-qualified name
 *
 * This represents FQN's of the form below
 * ```
 * <instance>.<sn>._tcp.<domain>.
 * ```
 *
 * Currently sub-types and service domains are unsupported and only the
 * "local" domain is supported. Specifically, that excludes the following:
 * ```
 * <sub>._sub.<sn>._tcp.<servicedomain>.<parentdomain>.
 * ```
 * @see <a href="https://datatracker.ietf.org/doc/html/rfc6763">RFC 6763</a>, Section 7.2.
 */
#define DNS_SD_MAX_LABELS 4

/**
 * @brief Register a service for DNS Service Discovery
 *
 * This macro should be used for advanced use cases. Two simple use cases are
 * when a custom @p _domain or a custom (non-standard) @p _proto is required.
 *
 * Another use case is when the port number is not preassigned. That could
 * be for a number of reasons, but the most common use case would be for
 * ephemeral port usage - i.e. when the service is bound using port number 0.
 * In that case, Zephyr (like other OS's) will simply choose an unused port.
 * When using ephemeral ports, it can be helpful to assign @p _port to the
 * @ref sockaddr_in.sin_port field of an IPv4 @ref sockaddr_in, or to the
 * @ref sockaddr_in6.sin6_port field of an IPv6 @ref sockaddr_in6.
 *
 * The service can be referenced using the @p _id variable.
 *
 * @param _id variable name for the DNS-SD service record
 * @param _instance name of the service instance such as "My HTTP Server"
 * @param _service name of the service, such as "_http"
 * @param _proto protocol used by the service - either "_tcp" or "_udp"
 * @param _domain the domain of the service, such as "local"
 * @param _text information for the DNS TXT record
 * @param _port a pointer to the port number that this service will use
 */
#define DNS_SD_REGISTER_SERVICE(_id, _instance, _service, _proto,	\
				_domain, _text, _port)			\
	static const STRUCT_SECTION_ITERABLE(dns_sd_rec, _id) = {	\
		.instance = _instance,					\
		.service = _service,					\
		.proto = _proto,					\
		.domain = _domain,					\
		.text = (const char *)_text,				\
		.text_size = sizeof(_text) - 1,				\
		.port = _port,						\
	}

/**
 * @brief Register a TCP service for DNS Service Discovery
 *
 * This macro can be used for service advertisement using DNS-SD.
 *
 * The service can be referenced using the @p id variable.
 *
 * Example (with TXT):
 * @code{.c}
 * #include <zephyr/net/dns_sd.h>
 * static const bar_txt[] = {
 *   "\x06" "path=/"
 *   "\x0f" "this=is the way"
 *   "\x0e" "foo or=foo not"
 *   "\x17" "this=has\0embedded\0nulls"
 *   "\x04" "true"
 * };
 * // Possibly use an ephemeral port
 * // Possibly only assign bar_port when the service is running
 * static uint16_t bar_port;
 * DNS_SD_REGISTER_TCP_SERVICE(bar, CONFIG_NET_HOSTNAME,
 *   "_bar", "local", bar_txt, &bar_port);
 * @endcode
 *
 * TXT records begin with a single length byte (hex-encoded)
 * and contain key=value pairs. Thus, the length of the key-value pair
 * must not exceed 255 bytes. Care must be taken to ensure that the
 * encoded length value is correct.
 *
 * For additional rules on TXT encoding, see RFC 6763, Section 6.

 * @param id variable name for the DNS-SD service record
 * @param instance name of the service instance such as "My HTTP Server"
 * @param service name of the service, such as "_http"
 * @param domain the domain of the service, such as "local"
 * @param text information for the DNS TXT record
 * @param port the port number that this service will use
 *
 * @see <a href="https://tools.ietf.org/html/rfc6763">RFC 6763</a>
 */
#define DNS_SD_REGISTER_TCP_SERVICE(id, instance, service, domain, text, \
				    port)				 \
	static const uint16_t id ## _port = sys_cpu_to_be16(port); \
	DNS_SD_REGISTER_SERVICE(id, instance, service, "_tcp", domain,	 \
				text, &id ## _port)

/**
 * @brief Register a UDP service for DNS Service Discovery
 *
 * This macro can be used for service advertisement using DNS-SD.
 *
 * The service can be referenced using the @p id variable.
 *
 * Example (no TXT):
 * @code{.c}
 * #include <zephyr/net/dns_sd.h>
 * #include <zephyr/sys/byteorder.h>
 * static const foo_port = sys_cpu_to_be16(4242);
 * DNS_SD_REGISTER_UDP_SERVICE(foo, CONFIG_NET_HOSTNAME,
 *   "_foo", DNS_SD_EMPTY_TXT, &foo_port);
 * @endcode
 *
 * @param id variable name for the DNS-SD service record
 * @param instance name of the service instance such as "My TFTP Server"
 * @param service name of the service, such as "_tftp"
 * @param domain the domain of the service, such as "local" or "zephyrproject.org"
 * @param text information for the DNS TXT record
 * @param port a pointer to the port number that this service will use
 *
 * @see <a href="https://tools.ietf.org/html/rfc6763">RFC 6763</a>
 */
#define DNS_SD_REGISTER_UDP_SERVICE(id, instance, service, domain, text, \
				    port)				 \
	static const uint16_t id ## _port = sys_cpu_to_be16(port); \
	DNS_SD_REGISTER_SERVICE(id, instance, service, "_udp", domain,	 \
				text, &id ## _port)

/** Empty DNS-SD TXT specifier */
#define DNS_SD_EMPTY_TXT dns_sd_empty_txt

/**
 * @brief DNS Service Discovery record
 *
 * This structure used in the implementation of RFC 6763 and should not
 * need to be accessed directly from application code.
 *
 * The @a port pointer must be non-NULL. When the value in @a port
 * is non-zero, the service is advertised as being on that particular
 * port. When the value in @a port is zero, then the service is not
 * advertised.
 *
 * Thus, it is possible for multiple services to advertise on a
 * particular port if they hard-code the port.
 *
 * @see <a href="https://tools.ietf.org/html/rfc6763">RFC 6763</a>
 */
struct dns_sd_rec {
	/** "<Instance>" - e.g. "My HTTP Server" */
	const char *instance;
	/** Top half of the "<Service>" such as "_http" */
	const char *service;
	/** Bottom half of the "<Service>" "_tcp" or "_udp" */
	const char *proto;
	/** "<Domain>" such as "local" or "zephyrproject.org" */
	const char *domain;
	/** DNS TXT record */
	const char *text;
	/** Size (in bytes) of the DNS TXT record  */
	size_t text_size;
	/** A pointer to the port number used by the service */
	const uint16_t *port;
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Empty TXT specifier for DNS-SD
 *
 * @internal
 */
extern const char dns_sd_empty_txt[1];
/**
 * @brief Wildcard Port specifier for DNS-SD
 *
 * @internal
 */
extern const uint16_t dns_sd_port_zero;

/** @endcond */

/**
 * @brief Obtain the size of DNS-SD TXT data
 *
 * @param rec the record to in question
 * @return the size of the text field
 */
static inline size_t dns_sd_txt_size(const struct dns_sd_rec *rec)
{
	return rec->text_size;
}

/**
 * @brief Check if @a rec is a DNS-SD Service Type Enumeration
 *
 * DNS-SD Service Type Enumeration is used by network tooling to
 * acquire a list of all mDNS-advertised services belonging to a
 * particular host on a particular domain.
 *
 * For example, for the domain '.local', the equivalent query
 * would be '_services._dns-sd._udp.local'.
 *
 * Currently, only the '.local' domain is supported.
 *
 * @see <a href="https://datatracker.ietf.org/doc/html/rfc6763#section-9">Service Type Enumeration, RFC 6763</a>.
 *
 * @param rec the record to in question
 * @return true if @a rec is a DNS-SD Service Type Enumeration
 */
bool dns_sd_is_service_type_enumeration(const struct dns_sd_rec *rec);

/**
 * @brief Create a wildcard filter for DNS-SD records
 *
 * @param filter a pointer to the filter to use
 */
void dns_sd_create_wildcard_filter(struct dns_sd_rec *filter);

/**
 * @}
 */

#ifdef __cplusplus
};
#endif

#endif /* ZEPHYR_INCLUDE_NET_DNS_SD_H_ */
