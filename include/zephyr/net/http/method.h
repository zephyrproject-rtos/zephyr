/** @file
 * @brief HTTP request methods
 */

/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_METHOD_H_
#define ZEPHYR_INCLUDE_NET_HTTP_METHOD_H_

/**
 * @brief HTTP request methods
 * @defgroup http_methods HTTP request methods
 * @ingroup networking
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief HTTP Request Methods */
enum http_method {
	HTTP_DELETE = 0, /**< DELETE */
	HTTP_GET = 1, /**< GET */
	HTTP_HEAD = 2, /**< HEAD */
	HTTP_POST = 3, /**< POST */
	HTTP_PUT = 4, /**< PUT */
	HTTP_CONNECT = 5, /**< CONNECT */
	HTTP_OPTIONS = 6, /**< OPTIONS */
	HTTP_TRACE = 7, /**< TRACE */
	HTTP_COPY = 8, /**< COPY */
	HTTP_LOCK = 9, /**< LOCK */
	HTTP_MKCOL = 10, /**< MKCOL */
	HTTP_MOVE = 11, /**< MOVE */
	HTTP_PROPFIND = 12, /**< PROPFIND */
	HTTP_PROPPATCH = 13, /**< PROPPATCH */
	HTTP_SEARCH = 14, /**< SEARCH */
	HTTP_UNLOCK = 15, /**< UNLOCK */
	HTTP_BIND = 16, /**< BIND */
	HTTP_REBIND = 17, /**< REBIND */
	HTTP_UNBIND = 18, /**< UNBIND */
	HTTP_ACL = 19, /**< ACL */
	HTTP_REPORT = 20, /**< REPORT */
	HTTP_MKACTIVITY = 21, /**< MKACTIVITY */
	HTTP_CHECKOUT = 22, /**< CHECKOUT */
	HTTP_MERGE = 23, /**< MERGE */
	HTTP_MSEARCH = 24, /**< MSEARCH */
	HTTP_NOTIFY = 25, /**< NOTIFY */
	HTTP_SUBSCRIBE = 26, /**< SUBSCRIBE */
	HTTP_UNSUBSCRIBE = 27, /**< UNSUBSCRIBE */
	HTTP_PATCH = 28, /**< PATCH */
	HTTP_PURGE = 29, /**< PURGE */
	HTTP_MKCALENDAR = 30, /**< MKCALENDAR */
	HTTP_LINK = 31, /**< LINK */
	HTTP_UNLINK = 32, /**< UNLINK */

	/** @cond INTERNAL_HIDDEN */
	HTTP_METHOD_END_VALUE /* keep this the last value */
	/** @endcond */
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
