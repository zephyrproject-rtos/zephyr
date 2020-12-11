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
#include <sys/byteorder.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DNS Service Discovery
 *
 * @details This API enables services to be advertised via DNS. To
 * advvertise a service, system or application code should use
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
 * @brief Register a service for DNS Service Discovery
 *
 * This macro should be used for advanced use cases. Two simple use cases are
 * when a custom @p domain or a custom (non-standard) @p proto is required.
 *
 * Another use case is when the port number is not preassigned. That could
 * be for a number of reasons, but the most common use case would be for
 * ephemeral port usage - i.e. when the service is bound using port number 0.
 * In that case, Zephyr (like other OS's) will simply choose an unused port.
 * When using ephemeral ports, it can be helpful to assign @p port to the
 * @ref sockaddr_in.sin_port field of an IPv4 @ref sockaddr_in, or to the
 * @ref sockaddr_in6.sin6_port field of an IPv6 @ref sockaddr_in6.
 *
 * The service can be referenced using the @p id variable.
 *
 * @param id variable name for the DNS-SD service record
 * @param instance name of the service instance such as "My HTTP Server"
 * @param service name of the service, such as "_http"
 * @param proto protocol used by the service - either "_tcp" or "_udp"
 * @param domain the domain of the service, such as "local"
 * @param text information for the DNS TXT record
 * @param port a pointer to the port number that this service will use
 */
#define DNS_SD_REGISTER_SERVICE(id, instance, service, proto, domain, \
				text, port)			      \
	static const Z_STRUCT_SECTION_ITERABLE(dns_sd_rec, id) = {    \
		instance,					      \
		service,					      \
		proto,						      \
		domain,						      \
		(const char *)text,				      \
		sizeof(text) - 1,				      \
		port						      \
	}

/**
 * @brief Register a TCP service for DNS Service Discovery
 *
 * This macro can be used for service advertisement using DNS-SD.
 *
 * The service can be referenced using the @p id variable.
 *
 * Example (with TXT):
 * @code{c}
 * #include <net/dns_sd.h>
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
 * @endcode{c}
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
 * @code{c}
 * #include <net/dns_sd.h>
 * #include <sys/byteorder.h>
 * static const foo_port = sys_cpu_to_be16(4242);
 * DNS_SD_REGISTER_UDP_SERVICE(foo, CONFIG_NET_HOSTNAME,
 *   "_foo", DNS_SD_EMPTY_TXT, &foo_port);
 * @endcode{c}
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

/** @cond INTERNAL_HIDDEN */

/**
 * @brief DNS Service Discovery record
 *
 * This structure used in the implementation of RFC 6763 and should not
 * need to be accessed directly from application code.
 *
 * The @a port pointer must be non-NULL. When the value in @a port
 * is non-zero, the service is advertized as being on that particular
 * port. When the value in @a port is zero, then the service is not
 * advertised.
 *
 * Thus, it is possible for multiple services to advertise on a
 * particular port if they hard-code the port.
 *
 * @internal
 *
 * @see <a href="https://tools.ietf.org/html/rfc6763">RFC 6763</a>
 */
struct dns_sd_rec {
	/** <Instance> - e.g. "My HTTP Server" */
	const char *instance;
	/** Top half of the <Service> such as "_http" */
	const char *service;
	/** Bottom half of the <Service> "_tcp" or "_udp" */
	const char *proto;
	/** <Domain> such as "local" or "zephyrproject.org" */
	const char *domain;
	/** DNS TXT record */
	const char *text;
	/** Size (in bytes) of the DNS TXT record  */
	size_t text_size;
	/** A pointer to the port number used by the service */
	const uint16_t *port;
};

/**
 * @brief Empty TXT specifier for DNS-SD
 *
 * @internal
 */
extern const char dns_sd_empty_txt[1];

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
 * @}
 */

#ifdef __cplusplus
};
#endif

#endif /* ZEPHYR_INCLUDE_NET_DNS_SD_H_ */
