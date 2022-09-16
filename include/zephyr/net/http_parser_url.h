/* SPDX-License-Identifier: MIT */

/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#ifndef ZEPHYR_INCLUDE_NET_HTTP_PARSER_URL_H_
#define ZEPHYR_INCLUDE_NET_HTTP_PARSER_URL_H_

#include <sys/types.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/net/http_parser_state.h>

#ifdef __cplusplus
extern "C" {
#endif

enum http_parser_url_fields {
	  UF_SCHEMA           = 0
	, UF_HOST             = 1
	, UF_PORT             = 2
	, UF_PATH             = 3
	, UF_QUERY            = 4
	, UF_FRAGMENT         = 5
	, UF_USERINFO         = 6
	, UF_MAX              = 7
};

/* Result structure for http_parser_url_fields().
 *
 * Callers should index into field_data[] with UF_* values iff field_set
 * has the relevant (1 << UF_*) bit set. As a courtesy to clients (and
 * because we probably have padding left over), we convert any port to
 * a uint16_t.
 */
struct http_parser_url {
	uint16_t field_set;           /* Bitmask of (1 << UF_*) values */
	uint16_t port;                /* Converted UF_PORT string */

	struct {
		uint16_t off;               /* Offset into buffer in which field
					  * starts
					  */
		uint16_t len;               /* Length of run in buffer */
	} field_data[UF_MAX];
};

enum state parse_url_char(enum state s, const char ch);

/* Initialize all http_parser_url members to 0 */
void http_parser_url_init(struct http_parser_url *u);

/* Parse a URL; return nonzero on failure */
int http_parser_parse_url(const char *buf, size_t buflen,
			  int is_connect, struct http_parser_url *u);

#ifdef __cplusplus
}
#endif
#endif
