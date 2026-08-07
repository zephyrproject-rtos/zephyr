/** @file
 * @brief HTTP response status codes
 */

/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_STATUS_H_
#define ZEPHYR_INCLUDE_NET_HTTP_STATUS_H_

/**
 * @brief HTTP response status codes
 * @defgroup http_status_codes HTTP response status codes
 * @since 3.3
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HTTP response status codes
 *
 * @note HTTP response status codes are subject to IANA approval.
 *
 * @see <a href="https://www.iana.org/assignments/http-status-codes">Hypertext Transfer Protocol (HTTP) Status Code Registry</a>
 * @see <a href="https://www.ietf.org/rfc/rfc9110.txt">RFC9110</a>
 * @see <a href="https://developer.mozilla.org/en-US/docs/Web/HTTP/Status">HTTP response status codes</a>
 */
enum http_status {
	HTTP_100_CONTINUE = 100, /**< Continue */
	HTTP_101_SWITCHING_PROTOCOLS = 101, /**< Switching Protocols */
	HTTP_102_PROCESSING = 102, /**< Processing */
	HTTP_103_EARLY_HINTS = 103, /**< Early Hints */
	HTTP_200_OK = 200, /**< OK */
	HTTP_201_CREATED = 201, /**< Created */
	HTTP_202_ACCEPTED = 202, /**< Accepted */
	HTTP_203_NON_AUTHORITATIVE_INFORMATION = 203, /**< Non-Authoritative Information */
	HTTP_204_NO_CONTENT = 204, /**< No Content */
	HTTP_205_RESET_CONTENT = 205, /**< Reset Content */
	HTTP_206_PARTIAL_CONTENT = 206, /**< Partial Content */
	HTTP_207_MULTI_STATUS = 207, /**< Multi-Status */
	HTTP_208_ALREADY_REPORTED = 208, /**< Already Reported */
	HTTP_226_IM_USED = 226, /**< IM Used */
	HTTP_300_MULTIPLE_CHOICES = 300, /**< Multiple Choices */
	HTTP_301_MOVED_PERMANENTLY = 301, /**< Moved Permanently */
	HTTP_302_FOUND = 302, /**< Found */
	HTTP_303_SEE_OTHER = 303, /**< See Other */
	HTTP_304_NOT_MODIFIED = 304, /**< Not Modified */
	HTTP_305_USE_PROXY = 305, /**< Use Proxy */
	HTTP_306_UNUSED = 306, /**< unused */
	HTTP_307_TEMPORARY_REDIRECT = 307, /**< Temporary Redirect */
	HTTP_308_PERMANENT_REDIRECT = 308, /**< Permanent Redirect */
	HTTP_400_BAD_REQUEST = 400, /**< Bad Request */
	HTTP_401_UNAUTHORIZED = 401, /**< Unauthorized */
	HTTP_402_PAYMENT_REQUIRED = 402, /**< Payment Required */
	HTTP_403_FORBIDDEN = 403, /**< Forbidden */
	HTTP_404_NOT_FOUND = 404, /**< Not Found */
	HTTP_405_METHOD_NOT_ALLOWED = 405, /**< Method Not Allowed */
	HTTP_406_NOT_ACCEPTABLE = 406, /**< Not Acceptable */
	HTTP_407_PROXY_AUTHENTICATION_REQUIRED = 407, /**< Proxy Authentication Required */
	HTTP_408_REQUEST_TIMEOUT = 408, /**< Request Timeout */
	HTTP_409_CONFLICT = 409, /**< Conflict */
	HTTP_410_GONE = 410, /**< Gone */
	HTTP_411_LENGTH_REQUIRED = 411, /**< Length Required */
	HTTP_412_PRECONDITION_FAILED = 412, /**< Precondition Failed */
	HTTP_413_PAYLOAD_TOO_LARGE = 413, /**< Payload Too Large */
	HTTP_414_URI_TOO_LONG = 414, /**< URI Too Long */
	HTTP_415_UNSUPPORTED_MEDIA_TYPE = 415, /**< Unsupported Media Type */
	HTTP_416_RANGE_NOT_SATISFIABLE = 416, /**< Range Not Satisfiable */
	HTTP_417_EXPECTATION_FAILED = 417, /**< Expectation Failed */
	HTTP_418_IM_A_TEAPOT = 418, /**< I'm a teapot */
	HTTP_421_MISDIRECTED_REQUEST = 421, /**< Misdirected Request */
	HTTP_422_UNPROCESSABLE_ENTITY = 422, /**< Unprocessable Entity */
	HTTP_423_LOCKED = 423, /**< Locked */
	HTTP_424_FAILED_DEPENDENCY = 424, /**< Failed Dependency */
	HTTP_425_TOO_EARLY = 425, /**< Too Early */
	HTTP_426_UPGRADE_REQUIRED = 426, /**< Upgrade Required */
	HTTP_428_PRECONDITION_REQUIRED = 428, /**< Precondition Required */
	HTTP_429_TOO_MANY_REQUESTS = 429, /**< Too Many Requests */
	HTTP_431_REQUEST_HEADER_FIELDS_TOO_LARGE = 431, /**< Request Header Fields Too Large */
	HTTP_451_UNAVAILABLE_FOR_LEGAL_REASONS = 451, /**< Unavailable For Legal Reasons */
	HTTP_500_INTERNAL_SERVER_ERROR = 500, /**< Internal Server Error */
	HTTP_501_NOT_IMPLEMENTED = 501, /**< Not Implemented */
	HTTP_502_BAD_GATEWAY = 502, /**< Bad Gateway */
	HTTP_503_SERVICE_UNAVAILABLE = 503, /**< Service Unavailable */
	HTTP_504_GATEWAY_TIMEOUT = 504, /**< Gateway Timeout */
	HTTP_505_HTTP_VERSION_NOT_SUPPORTED = 505, /**< HTTP Version Not Supported */
	HTTP_506_VARIANT_ALSO_NEGOTIATES = 506, /**< Variant Also Negotiates */
	HTTP_507_INSUFFICIENT_STORAGE = 507, /**< Insufficient Storage */
	HTTP_508_LOOP_DETECTED = 508, /**< Loop Detected */
	HTTP_510_NOT_EXTENDED = 510, /**< Not Extended */
	HTTP_511_NETWORK_AUTHENTICATION_REQUIRED = 511, /**< Network Authentication Required */
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
