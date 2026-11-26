/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief URL components
 */

#ifndef ZEPHYR_INCLUDE_NET_URL_H_
#define ZEPHYR_INCLUDE_NET_URL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup net_url URL components
 * @since 4.6
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/** A component of a URL, located in the string it was parsed from. */
struct net_url_field {
	/** Offset of the component in net_url.base. */
	uint16_t off;
	/** Length of the component, 0 when the URL has none. */
	uint16_t len;
};

/** The components of a URL. */
struct net_url {
	/** The URL the components are located in. */
	const char *base;
	/** Scheme, without the "://". */
	struct net_url_field scheme;
	/** User information, without the "@". */
	struct net_url_field userinfo;
	/** Host name or IP literal, without the brackets of an IPv6 literal. */
	struct net_url_field host;
	/** Path, up to the "?" or "#". */
	struct net_url_field path;
	/** Query, without the "?". */
	struct net_url_field query;
	/** Fragment, without the "#". */
	struct net_url_field fragment;
	/** Port number, 0 when the URL has none or names port 0 explicitly. */
	uint16_t port;
};

/**
 * @brief Split a URL into its components.
 *
 * The components are offsets into @p url, which must stay valid and
 * unchanged while @p out is used.
 *
 * @param url NUL-terminated URL.
 * @param out Components of the URL.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p url or @p out is NULL, or @p url is not a URL.
 */
int net_url_parse(const char *url, struct net_url *out);

/**
 * @brief Compare the scheme of a URL, ignoring case.
 *
 * @param url Parsed URL.
 * @param scheme Scheme to compare with, without the "://".
 *
 * @return true when the URL has @p scheme as scheme.
 */
bool net_url_scheme_is(const struct net_url *url, const char *scheme);

/**
 * @brief Copy a component into a NUL-terminated buffer.
 *
 * An absent component yields an empty string.
 *
 * @param url Parsed URL.
 * @param field Component of @p url to copy.
 * @param dst Destination buffer.
 * @param dst_len Size of @p dst.
 *
 * @retval 0 Success.
 * @retval -ENOBUFS The component and its terminator do not fit in @p dst.
 */
int net_url_field_copy(const struct net_url *url, const struct net_url_field *field, char *dst,
		       size_t dst_len);

/**
 * @brief Copy the request target of a URL into a NUL-terminated buffer.
 *
 * The request target is the path followed by "?" and the query when there is
 * one, as sent in an HTTP request line. An empty path is written as "/".
 *
 * @param url Parsed URL.
 * @param dst Destination buffer.
 * @param dst_len Size of @p dst.
 *
 * @retval 0 Success.
 * @retval -ENOBUFS The target and its terminator do not fit in @p dst.
 */
int net_url_target_copy(const struct net_url *url, char *dst, size_t dst_len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_URL_H_ */
