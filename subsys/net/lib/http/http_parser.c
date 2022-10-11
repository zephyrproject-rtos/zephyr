/* SPDX-License-Identifier: MIT */

/* Based on src/http/ngx_http_parse.c from NGINX copyright Igor Sysoev
 *
 * Additional changes are licensed under the same terms as NGINX and
 * copyright Joyent, Inc. and other Node contributors. All rights reserved.
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
#include <zephyr/net/http/parser.h>
#include <zephyr/sys/__assert.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <zephyr/toolchain.h>

#ifndef ULLONG_MAX
# define ULLONG_MAX ((uint64_t) -1) /* 2^64-1 */
#endif

#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef BIT_AT
# define BIT_AT(a, i)                                                \
	(!!((unsigned int) (a)[(unsigned int) (i) >> 3] &                  \
	 (1 << ((unsigned int) (i) & 7))))
#endif

#ifndef ELEM_AT
# define ELEM_AT(a, i, v) ((unsigned int) (i) < ARRAY_SIZE(a) ? (a)[(i)] : (v))
#endif

#define SET_ERRNO(e)	(parser->http_errno = (e))
#define CURRENT_STATE() p_state
#define UPDATE_STATE(V) (p_state = (enum state)(V))

#ifdef __GNUC__
# define LIKELY(X) __builtin_expect(!!(X), 1)
# define UNLIKELY(X) __builtin_expect(!!(X), 0)
#else
# define LIKELY(X) (X)
# define UNLIKELY(X) (X)
#endif

/* Set the mark FOR; non-destructive if mark is already set */
#define MARK(FOR)                                                          \
do {                                                                       \
	if (!FOR##_mark) {                                                 \
		FOR##_mark = p;                                            \
	}                                                                  \
} while (0)

/* Don't allow the total size of the HTTP headers (including the status
 * line) to exceed HTTP_MAX_HEADER_SIZE.  This check is here to protect
 * embedders against denial-of-service attacks where the attacker feeds
 * us a never-ending header that the embedder keeps buffering.
 *
 * This check is arguably the responsibility of embedders but we're doing
 * it on the embedder's behalf because most won't bother and this way we
 * make the web a little safer.  HTTP_MAX_HEADER_SIZE is still far bigger
 * than any reasonable request or response so this should never affect
 * day-to-day operation.
 */
static inline
int count_header_size(struct http_parser *parser, int bytes)
{
	parser->nread += bytes;

	if (UNLIKELY(parser->nread > (HTTP_MAX_HEADER_SIZE))) {
		parser->http_errno = HPE_HEADER_OVERFLOW;
		return -1;
	}

	return 0;
}

#define PROXY_CONNECTION "proxy-connection"
#define CONNECTION "connection"
#define CONTENT_LENGTH "content-length"
#define TRANSFER_ENCODING "transfer-encoding"
#define UPGRADE "upgrade"
#define CHUNKED "chunked"
#define KEEP_ALIVE "keep-alive"
#define CLOSE "close"


static const
char *method_strings[] = {"DELETE", "GET", "HEAD", "POST", "PUT", "CONNECT",
			  "OPTIONS", "TRACE", "COPY", "LOCK", "MKCOL", "MOVE",
			  "PROPFIND", "PROPPATCH", "SEARCH", "UNLOCK", "BIND",
			  "REBIND", "UNBIND", "ACL", "REPORT", "MKACTIVITY",
			  "CHECKOUT", "MERGE", "M-SEARCH", "NOTIFY",
			  "SUBSCRIBE", "UNSUBSCRIBE", "PATCH", "PURGE",
			  "MKCALENDAR", "LINK", "UNLINK"};


/* Tokens as defined by rfc 2616. Also lowercases them.
 *        token       = 1*<any CHAR except CTLs or separators>
 *     separators     = "(" | ")" | "<" | ">" | "@"
 *                    | "," | ";" | ":" | "\" | <">
 *                    | "/" | "[" | "]" | "?" | "="
 *                    | "{" | "}" | SP | HT
 */
static const char tokens[256] = {
/*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
	0,       0,       0,       0,       0,       0,       0,       0,
/*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
	0,       0,       0,       0,       0,       0,       0,       0,
/*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
	0,       0,       0,       0,       0,       0,       0,       0,
/*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
	0,       0,       0,       0,       0,       0,       0,       0,
/*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
	0,      '!',      0,      '#',     '$',     '%',     '&',    '\'',
/*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
	0,       0,      '*',     '+',      0,      '-',     '.',      0,
/*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
	'0',     '1',     '2',     '3',     '4',     '5',     '6',     '7',
/*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
	'8',     '9',      0,       0,       0,       0,       0,       0,
/*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
	0,      'a',     'b',     'c',     'd',     'e',     'f',     'g',
/*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
	'h',     'i',     'j',     'k',     'l',     'm',     'n',     'o',
/*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
	'p',     'q',     'r',     's',     't',     'u',     'v',     'w',
/*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
	'x',     'y',     'z',      0,       0,       0,      '^',     '_',
/*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
	'`',     'a',     'b',     'c',     'd',     'e',     'f',     'g',
/* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
	'h',     'i',     'j',     'k',     'l',     'm',     'n',     'o',
/* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
	'p',     'q',     'r',     's',     't',     'u',     'v',     'w',
/* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
	'x',     'y',     'z',      0,      '|',      0,      '~',       0
};


static const
int8_t unhex[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};


#define PARSING_HEADER(state) (state <= s_headers_done)

enum header_states {
	h_general = 0,
	h_C,
	h_CO,
	h_CON,
	h_matching_connection,
	h_matching_proxy_connection,
	h_matching_content_length,
	h_matching_transfer_encoding,
	h_matching_upgrade,
	h_connection,
	h_content_length,
	h_transfer_encoding,
	h_upgrade,
	h_matching_transfer_encoding_chunked,
	h_matching_connection_token_start,
	h_matching_connection_keep_alive,
	h_matching_connection_close,
	h_matching_connection_upgrade,
	h_matching_connection_token,
	h_transfer_encoding_chunked,
	h_connection_keep_alive,
	h_connection_close,
	h_connection_upgrade
};

static inline
int cb_notify(struct http_parser *parser, enum state *current_state, http_cb cb,
	      int cb_error, size_t *parsed, size_t already_parsed)
{
	__ASSERT_NO_MSG(HTTP_PARSER_ERRNO(parser) == HPE_OK);

	if (cb == NULL) {
		return 0;
	}

	parser->state = *current_state;
	if (UNLIKELY(cb(parser) != 0)) {
		SET_ERRNO(cb_error);
	}
	*current_state = parser->state;
	/* We either errored above or got paused; get out */
	if (UNLIKELY(HTTP_PARSER_ERRNO(parser) != HPE_OK)) {
		*parsed = already_parsed;
		return -HTTP_PARSER_ERRNO(parser);
	}

	return 0;
}

static inline
int cb_data(struct http_parser *parser, http_data_cb cb, int cb_error,
	    enum state *current_state, size_t *parsed, size_t already_parsed,
	    const char **mark, size_t len)
{
	int rc;

	__ASSERT_NO_MSG(HTTP_PARSER_ERRNO(parser) == HPE_OK);
	if (*mark == NULL) {
		return 0;
	}
	if (cb == NULL) {
		goto lb_end;
	}

	parser->state = *current_state;
	rc = cb(parser, *mark, len);
	if (UNLIKELY(rc != 0)) {
		SET_ERRNO(cb_error);
	}
	*current_state = parser->state;
	/* We either errored above or got paused; get out */
	if (UNLIKELY(HTTP_PARSER_ERRNO(parser) != HPE_OK)) {
		*parsed = already_parsed;
		return -HTTP_PARSER_ERRNO(parser);
	}
lb_end:
	*mark = NULL;

	return 0;
}


/* Macros for character classes; depends on strict-mode  */
#define CR                  '\r'
#define LF                  '\n'
#define LOWER(c)            (unsigned char)(c | 0x20)
#define IS_ALPHA(c)         (LOWER(c) >= 'a' && LOWER(c) <= 'z')
#define IS_NUM(c)           ((c) >= '0' && (c) <= '9')
#define IS_ALPHANUM(c)      (IS_ALPHA(c) || IS_NUM(c))
#define IS_HEX(c)           (IS_NUM(c) || (LOWER(c) >= 'a' && LOWER(c) <= 'f'))

#define IS_MARK(c)		((c) == '-' || (c) == '_' || (c) == '.' || \
				 (c) == '!' || (c) == '~' || (c) == '*' || \
				 (c) == '\'' || (c) == '(' || (c) == ')')

#define IS_USERINFO_CHAR(c) (IS_ALPHANUM(c) || IS_MARK(c) || (c) == '%' || \
				(c) == ';' || (c) == ':' || (c) == '&' ||  \
				(c) == '=' || (c) == '+' || (c) == '$' ||  \
				(c) == ',')

#define STRICT_TOKEN(c)     (tokens[(unsigned char)c])

#ifdef HTTP_PARSER_STRICT
#define TOKEN(c)            (tokens[(unsigned char)c])
#define IS_HOST_CHAR(c)     (IS_ALPHANUM(c) || (c) == '.' || (c) == '-')
#else
#define TOKEN(c)            ((c == ' ') ? ' ' : tokens[(unsigned char)c])
#define IS_HOST_CHAR(c)                                                        \
	(IS_ALPHANUM(c) || (c) == '.' || (c) == '-' || (c) == '_')
#endif

/**
 * Verify that a char is a valid visible (printable) US-ASCII
 * character or %x80-FF
 **/
#define IS_HEADER_CHAR(ch)                                                     \
	(ch == CR || ch == LF || ch == 9 || \
	 ((unsigned char)ch > 31 && ch != 127))

#define start_state (parser->type == HTTP_REQUEST ? s_start_req : s_start_res)


#ifdef HTTP_PARSER_STRICT
static inline
int strict_check(struct http_parser *parser, int c)
{
	if (c) {
		parser->http_errno = HPE_STRICT;
		return -1;
	}
	return 0;
}

# define NEW_MESSAGE() (http_should_keep_alive(parser) ? start_state : s_dead)
#else
static inline
int strict_check(struct http_parser *parser, int c)
{
	(void)parser;
	(void)c;
	return 0;
}
# define NEW_MESSAGE() start_state
#endif

static struct {
	const char *name;
	const char *description;
} http_strerror_tab[] = {
	{"HPE_OK", "success"},
	{"HPE_CB_message_begin", "the on_message_begin callback failed"},
	{"HPE_CB_url", "the on_url callback failed"},
	{"HPE_CB_header_field", "the on_header_field callback failed"},
	{"HPE_CB_header_value", "the on_header_value callback failed"},
	{"HPE_CB_headers_complete", "the on_headers_complete callback failed"},
	{"HPE_CB_body", "the on_body callback failed"},
	{"HPE_CB_message_complete", "the on_message_complete callback failed"},
	{"HPE_CB_status", "the on_status callback failed"},
	{"HPE_CB_chunk_header", "the on_chunk_header callback failed"},
	{"HPE_CB_chunk_complete", "the on_chunk_complete callback failed"},
	{"HPE_INVALID_EOF_STATE", "stream ended at an unexpected time"},
	{"HPE_HEADER_OVERFLOW", "too many header bytes seen; "
				"overflow detected"},
	{"HPE_CLOSED_CONNECTION", "data received after completed connection: "
				  "close message"},
	{"HPE_INVALID_VERSION", "invalid HTTP version"},
	{"HPE_INVALID_STATUS", "invalid HTTP status code"},
	{"HPE_INVALID_METHOD", "invalid HTTP method"},
	{"HPE_INVALID_URL", "invalid URL"},
	{"HPE_INVALID_HOST", "invalid host"},
	{"HPE_INVALID_PORT", "invalid port"},
	{"HPE_INVALID_PATH", "invalid path"},
	{"HPE_INVALID_QUERY_STRING", "invalid query string"},
	{"HPE_INVALID_FRAGMENT", "invalid fragment"},
	{"HPE_LF_EXPECTED", "LF character expected"},
	{"HPE_INVALID_HEADER_TOKEN", "invalid character in header"},
	{"HPE_INVALID_CONTENT_LENGTH", "invalid character in content-length "
				       "header"},
	{"HPE_UNEXPECTED_CONTENT_LENGTH", "unexpected content-length header"},
	{"HPE_INVALID_CHUNK_SIZE", "invalid character in chunk size header"},
	{"HPE_INVALID_CONSTANT", "invalid constant string"},
	{"HPE_INVALID_INTERNAL_STATE", "encountered unexpected internal state"},
	{"HPE_STRICT", "strict mode assertion failed"},
	{"HPE_PAUSED", "parser is paused"},
	{"HPE_UNKNOWN", "an unknown error occurred"}
};

int http_message_needs_eof(const struct http_parser *parser);

static
int parser_header_state(struct http_parser *parser, char ch, char c)
{
	int cond1;

	switch (parser->header_state) {
	case h_general:
		break;

	case h_C:
		parser->index++;
		parser->header_state = (c == 'o' ? h_CO : h_general);
		break;

	case h_CO:
		parser->index++;
		parser->header_state = (c == 'n' ? h_CON : h_general);
		break;

	case h_CON:
		parser->index++;
		switch (c) {
		case 'n':
			parser->header_state = h_matching_connection;
			break;
		case 't':
			parser->header_state = h_matching_content_length;
			break;
		default:
			parser->header_state = h_general;
			break;
		}
		break;

	/* connection */

	case h_matching_connection:
		parser->index++;
		cond1 = parser->index > sizeof(CONNECTION) - 1;
		if (cond1 || c != CONNECTION[parser->index]) {
			parser->header_state = h_general;
		} else if (parser->index == sizeof(CONNECTION) - 2) {
			parser->header_state = h_connection;
		}
		break;

	/* proxy-connection */

	case h_matching_proxy_connection:
		parser->index++;
		cond1 = parser->index > sizeof(PROXY_CONNECTION) - 1;
		if (cond1 || c != PROXY_CONNECTION[parser->index]) {
			parser->header_state = h_general;
		} else if (parser->index == sizeof(PROXY_CONNECTION) - 2) {
			parser->header_state = h_connection;
		}
		break;

	/* content-length */

	case h_matching_content_length:
		parser->index++;
		cond1 = parser->index > sizeof(CONTENT_LENGTH) - 1;
		if (cond1 || c != CONTENT_LENGTH[parser->index]) {
			parser->header_state = h_general;
		} else if (parser->index == sizeof(CONTENT_LENGTH) - 2) {
			parser->header_state = h_content_length;
		}
		break;

	/* transfer-encoding */

	case h_matching_transfer_encoding:
		parser->index++;
		cond1 = parser->index > sizeof(TRANSFER_ENCODING) - 1;
		if (cond1 || c != TRANSFER_ENCODING[parser->index]) {
			parser->header_state = h_general;
		} else if (parser->index == sizeof(TRANSFER_ENCODING) - 2) {
			parser->header_state = h_transfer_encoding;
		}
		break;

	/* upgrade */

	case h_matching_upgrade:
		parser->index++;
		cond1 = parser->index > sizeof(UPGRADE) - 1;
		if (cond1 || c != UPGRADE[parser->index]) {
			parser->header_state = h_general;
		} else if (parser->index == sizeof(UPGRADE) - 2) {
			parser->header_state = h_upgrade;
		}
		break;

	case h_connection:
	case h_content_length:
	case h_transfer_encoding:
	case h_upgrade:
		if (ch != ' ') {
			parser->header_state = h_general;
		}
		break;

	default:
		__ASSERT_NO_MSG(0 && "Unknown header_state");
		break;
	}
	return 0;
}

static
int header_states(struct http_parser *parser, const char *data, size_t len,
		  const char **ptr, enum state *p_state,
		  enum header_states *header_state, char ch, char c)
{
	enum header_states h_state = *header_state;
	const char *p = *ptr;
	int cond1;

	switch (h_state) {
	case h_general: {
		size_t limit = data + len - p;
		const char *p_cr;
		const char *p_lf;

		limit = MIN(limit, HTTP_MAX_HEADER_SIZE);
		p_cr = (const char *)memchr(p, CR, limit);
		p_lf = (const char *)memchr(p, LF, limit);
		if (p_cr != NULL) {
			if (p_lf != NULL && p_cr >= p_lf) {
				p = p_lf;
			} else {
				p = p_cr;
			}
		} else if (UNLIKELY(p_lf != NULL)) {
			p = p_lf;
		} else {
			p = data + len;
		}
		--p;

		break;
	}

	case h_connection:
	case h_transfer_encoding:
		__ASSERT_NO_MSG(0 && "Shouldn't get here.");
		break;

	case h_content_length: {
		uint64_t t;
		uint64_t value;

		if (ch == ' ') {
			break;
		}

		if (UNLIKELY(!IS_NUM(ch))) {
			SET_ERRNO(HPE_INVALID_CONTENT_LENGTH);
			parser->header_state = h_state;
			return -1;
		}

		t = parser->content_length;
		t *= 10U;
		t += ch - '0';

		/* Overflow? Test against a conservative limit for simplicity */
		value = (ULLONG_MAX - 10) / 10;

		if (UNLIKELY(value < parser->content_length)) {
			SET_ERRNO(HPE_INVALID_CONTENT_LENGTH);
			parser->header_state = h_state;
			return -1;
		}

		parser->content_length = t;
		break;
	}

	/* Transfer-Encoding: chunked */
	case h_matching_transfer_encoding_chunked:
		parser->index++;
		cond1 = parser->index > sizeof(CHUNKED) - 1;
		if (cond1 || c != CHUNKED[parser->index]) {
			h_state = h_general;
		} else if (parser->index == sizeof(CHUNKED) - 2) {
			h_state = h_transfer_encoding_chunked;
		}
		break;

	case h_matching_connection_token_start:
		/* looking for 'Connection: keep-alive' */
		if (c == 'k') {
			h_state = h_matching_connection_keep_alive;
			/* looking for 'Connection: close' */
		} else if (c == 'c') {
			h_state = h_matching_connection_close;
		} else if (c == 'u') {
			h_state = h_matching_connection_upgrade;
		} else if (STRICT_TOKEN(c)) {
			h_state = h_matching_connection_token;
		} else if (c == ' ' || c == '\t') {
			/* Skip lws */
		} else {
			h_state = h_general;
		}
		break;

	/* looking for 'Connection: keep-alive' */
	case h_matching_connection_keep_alive:
		parser->index++;
		cond1 = parser->index > sizeof(KEEP_ALIVE) - 1;
		if (cond1 || c != KEEP_ALIVE[parser->index]) {
			h_state = h_matching_connection_token;
		} else if (parser->index == sizeof(KEEP_ALIVE) - 2) {
			h_state = h_connection_keep_alive;
		}
		break;

	/* looking for 'Connection: close' */
	case h_matching_connection_close:
		parser->index++;
		cond1 = parser->index > sizeof(CLOSE) - 1;
		if (cond1 || c != CLOSE[parser->index]) {
			h_state = h_matching_connection_token;
		} else if (parser->index == sizeof(CLOSE) - 2) {
			h_state = h_connection_close;
		}
		break;

	/* looking for 'Connection: upgrade' */
	case h_matching_connection_upgrade:
		parser->index++;
		cond1 = parser->index > sizeof(UPGRADE) - 1;
		if (cond1 || c != UPGRADE[parser->index]) {
			h_state = h_matching_connection_token;
		} else if (parser->index == sizeof(UPGRADE) - 2) {
			h_state = h_connection_upgrade;
		}
		break;

	case h_matching_connection_token:
		if (ch == ',') {
			h_state = h_matching_connection_token_start;
			parser->index = 0U;
		}
		break;

	case h_transfer_encoding_chunked:
		if (ch != ' ') {
			h_state = h_general;
		}
		break;

	case h_connection_keep_alive:
	case h_connection_close:
	case h_connection_upgrade:
		if (ch == ',') {
			if (h_state == h_connection_keep_alive) {
				parser->flags |= F_CONNECTION_KEEP_ALIVE;
			} else if (h_state == h_connection_close) {
				parser->flags |= F_CONNECTION_CLOSE;
			} else if (h_state == h_connection_upgrade) {
				parser->flags |= F_CONNECTION_UPGRADE;
			}
			h_state = h_matching_connection_token_start;
			parser->index = 0U;
		} else if (ch != ' ') {
			h_state = h_matching_connection_token;
		}
		break;

	default:
		*p_state = s_header_value;
		h_state = h_general;
		break;
	}

	*header_state = h_state;
	*ptr = p;

	return 0;
}

static
int zero_content_length(struct http_parser *parser,
			const struct http_parser_settings *settings,
			enum state *current_state, size_t *parsed,
			const char *p, const char *data)
{
	enum state p_state = *current_state;
	int rc;

	if (parser->content_length == 0U) {
		/* Content-Length header given but zero:
		 * Content-Length: 0\r\n
		 */
		UPDATE_STATE(NEW_MESSAGE());

		rc = cb_notify(parser, &p_state, settings->on_message_complete,
			       HPE_CB_message_complete, parsed, p - data + 1);
		if (rc != 0) {
			return rc;
		}

	} else if (parser->content_length != ULLONG_MAX) {
		/* Content-Length header given and
		 * non-zero
		 */
		UPDATE_STATE(s_body_identity);
	} else {
		if (!http_message_needs_eof(parser)) {
			/* Assume content-length 0 -
			 * read the next
			 */
			UPDATE_STATE(NEW_MESSAGE());

			rc = cb_notify(parser, &p_state,
				       settings->on_message_complete,
				       HPE_CB_message_complete, parsed,
				       p - data + 1);
			if (rc != 0) {
				return rc;
			}

		} else {
			/* Read body until EOF */
			UPDATE_STATE(s_body_identity_eof);
		}
	}

	*current_state = p_state;

	return 0;
}

static
int parser_execute(struct http_parser *parser,
		   const struct http_parser_settings *settings,
		   const char *data, size_t len, size_t *parsed)
{
	const unsigned int lenient = parser->lenient_http_headers;
	enum state p_state = (enum state) parser->state;
	const char *p = data;
	const char *header_field_mark = 0;
	const char *header_value_mark = 0;
	const char *url_mark = 0;
	const char *body_mark = 0;
	const char *status_mark = 0;
	int8_t unhex_val;
	int rc;
	char ch;
	char c;

	*parsed = 0;

	/* We're in an error state. Don't bother doing anything. */
	if (HTTP_PARSER_ERRNO(parser) != HPE_OK) {
		return 0;
	}

	if (len == 0) {
		switch (CURRENT_STATE()) {
		case s_body_identity_eof:
			/* Use of CALLBACK_NOTIFY() here would erroneously
			 * return 1 byte read if
			 * we got paused.
			 */
			cb_notify(parser, &p_state,
				  settings->on_message_complete,
				  HPE_CB_message_complete, parsed, p - data);
			return 0;

		case s_dead:
		case s_start_req_or_res:
		case s_start_res:
		case s_start_req:
			return 0;

		default:
			SET_ERRNO(HPE_INVALID_EOF_STATE);
			return 1;
		}
	}


	if (CURRENT_STATE() == s_header_field) {
		header_field_mark = data;
	}
	if (CURRENT_STATE() == s_header_value) {
		header_value_mark = data;
	}
	switch (CURRENT_STATE()) {
	case s_req_path:
	case s_req_schema:
	case s_req_schema_slash:
	case s_req_schema_slash_slash:
	case s_req_server_start:
	case s_req_server:
	case s_req_server_with_at:
	case s_req_query_string_start:
	case s_req_query_string:
	case s_req_fragment_start:
	case s_req_fragment:
		url_mark = data;
		break;
	case s_res_status:
		status_mark = data;
		break;
	default:
		break;
	}

	for (p = data; p != data + len; p++) {
		ch = *p;

		if (PARSING_HEADER(CURRENT_STATE())) {
			rc = count_header_size(parser, 1);
			if (rc != 0) {
				goto error;
			}
		}

reexecute:
		switch (CURRENT_STATE()) {

		case s_dead:
			/* this state is used after a 'Connection: close'
			 * message
			 * the parser will error out if it reads another message
			 */
			if (LIKELY(ch == CR || ch == LF)) {
				break;
			}

			SET_ERRNO(HPE_CLOSED_CONNECTION);
			goto error;

		case s_start_req_or_res: {
			if (ch == CR || ch == LF) {
				break;
			}
			parser->flags = 0U;
			parser->content_length = ULLONG_MAX;

			if (ch == 'H') {
				UPDATE_STATE(s_res_or_resp_H);

				rc = cb_notify(parser, &p_state,
					       settings->on_message_begin,
					       HPE_CB_message_begin,
					       parsed, p - data + 1);
				if (rc != 0) {
					return rc;
				}
			} else {
				parser->type = HTTP_REQUEST;
				UPDATE_STATE(s_start_req);
				goto reexecute;
			}

			break;
		}

		case s_res_or_resp_H:
			if (ch == 'T') {
				parser->type = HTTP_RESPONSE;
				UPDATE_STATE(s_res_HT);
			} else {
				if (UNLIKELY(ch != 'E')) {
					SET_ERRNO(HPE_INVALID_CONSTANT);
					goto error;
				}

				parser->type = HTTP_REQUEST;
				parser->method = HTTP_HEAD;
				parser->index = 2U;
				UPDATE_STATE(s_req_method);
			}
			break;

		case s_start_res: {
			parser->flags = 0U;
			parser->content_length = ULLONG_MAX;

			switch (ch) {
			case 'H':
				UPDATE_STATE(s_res_H);
				break;

			case CR:
			case LF:
				break;

			default:
				SET_ERRNO(HPE_INVALID_CONSTANT);
				goto error;
			}


			rc = cb_notify(parser, &p_state,
				       settings->on_message_begin,
				       HPE_CB_message_begin, parsed,
				       p - data + 1);
			if (rc != 0) {
				return rc;
			}
			break;
		}

		case s_res_H:
			rc = strict_check(parser, ch != 'T');
			if (rc != 0) {
				goto error;
			}
			UPDATE_STATE(s_res_HT);
			break;

		case s_res_HT:
			rc = strict_check(parser, ch != 'T');
			if (rc != 0) {
				goto error;
			}
			UPDATE_STATE(s_res_HTT);
			break;

		case s_res_HTT:
			rc = strict_check(parser, ch != 'P');
			if (rc != 0) {
				goto error;
			}
			UPDATE_STATE(s_res_HTTP);
			break;

		case s_res_HTTP:
			rc = strict_check(parser, ch != '/');
			if (rc != 0) {
				goto error;
			}
			UPDATE_STATE(s_res_first_http_major);
			break;

		case s_res_first_http_major:
			if (UNLIKELY(ch < '0' || ch > '9')) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_major = ch - '0';
			UPDATE_STATE(s_res_http_major);
			break;

		/* major HTTP version or dot */
		case s_res_http_major: {
			if (ch == '.') {
				UPDATE_STATE(s_res_first_http_minor);
				break;
			}

			if (!IS_NUM(ch)) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_major *= 10U;
			parser->http_major += ch - '0';

			if (UNLIKELY(parser->http_major > 999)) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			break;
		}

		/* first digit of minor HTTP version */
		case s_res_first_http_minor:
			if (UNLIKELY(!IS_NUM(ch))) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_minor = ch - '0';
			UPDATE_STATE(s_res_http_minor);
			break;

		/* minor HTTP version or end of request line */
		case s_res_http_minor: {
			if (ch == ' ') {
				UPDATE_STATE(s_res_first_status_code);
				break;
			}

			if (UNLIKELY(!IS_NUM(ch))) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_minor *= 10U;
			parser->http_minor += ch - '0';

			if (UNLIKELY(parser->http_minor > 999)) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			break;
		}

		case s_res_first_status_code: {
			if (!IS_NUM(ch)) {
				if (ch == ' ') {
					break;
				}

				SET_ERRNO(HPE_INVALID_STATUS);
				goto error;
			}
			parser->status_code = ch - '0';
			UPDATE_STATE(s_res_status_code);
			break;
		}

		case s_res_status_code: {
			if (!IS_NUM(ch)) {
				/* Numeric status only */
				if ((ch == CR) || (ch == LF)) {
					const char *no_status_txt = "";

					rc = cb_data(parser,
					     settings->on_status,
					     HPE_CB_status, &p_state, parsed,
					     p - data + 1, &no_status_txt, 0);
					if (rc != 0) {
						return rc;
					}
				}

				switch (ch) {
				case ' ':
					UPDATE_STATE(s_res_status_start);
					break;
				case CR:
					UPDATE_STATE(s_res_line_almost_done);
					break;
				case LF:
					UPDATE_STATE(s_header_field_start);
					break;
				default:
					SET_ERRNO(HPE_INVALID_STATUS);
					goto error;
				}
				break;
			}

			parser->status_code *= 10U;
			parser->status_code += ch - '0';

			if (UNLIKELY(parser->status_code > 999)) {
				SET_ERRNO(HPE_INVALID_STATUS);
				goto error;
			}

			break;
		}

		case s_res_status_start: {
			if (!status_mark && ((ch == CR) || (ch == LF))) {
				/* Numeric status only */
				const char *no_status_txt = "";

				rc = cb_data(parser,
					settings->on_status,
					HPE_CB_status, &p_state, parsed,
					p - data + 1, &no_status_txt, 0);
				if (rc != 0) {
					return rc;
				}
			}
			if (ch == CR) {
				UPDATE_STATE(s_res_line_almost_done);
				break;
			}

			if (ch == LF) {
				UPDATE_STATE(s_header_field_start);
				break;
			}

			MARK(status);
			UPDATE_STATE(s_res_status);
			parser->index = 0U;
			break;
		}

		case s_res_status:
			if (ch == CR) {
				UPDATE_STATE(s_res_line_almost_done);
				rc = cb_data(parser, settings->on_status,
					     HPE_CB_status, &p_state, parsed,
					     p - data + 1, &status_mark,
					     p - status_mark);
				if (rc != 0) {
					return rc;
				}
				break;
			}

			if (ch == LF) {
				UPDATE_STATE(s_header_field_start);
				rc = cb_data(parser, settings->on_status,
					     HPE_CB_status, &p_state, parsed,
					     p - data + 1, &status_mark,
					     p - status_mark);
				if (rc != 0) {
					return rc;
				}

				break;
			}

			break;

		case s_res_line_almost_done:
			rc = strict_check(parser, ch != LF);
			if (rc != 0) {
				goto error;
			}
			UPDATE_STATE(s_header_field_start);
			break;

		case s_start_req: {
			if (ch == CR || ch == LF) {
				break;
			}
			parser->flags = 0U;
			parser->content_length = ULLONG_MAX;

			if (UNLIKELY(!IS_ALPHA(ch))) {
				SET_ERRNO(HPE_INVALID_METHOD);
				goto error;
			}

			parser->method = (enum http_method) 0;
			parser->index = 1U;
			switch (ch) {
			case 'A':
				parser->method = HTTP_ACL;
				break;
			case 'B':
				parser->method = HTTP_BIND;
				break;
			case 'C':
				parser->method = HTTP_CONNECT;
				/* or COPY, CHECKOUT */
				break;
			case 'D':
				parser->method = HTTP_DELETE;
				break;
			case 'G':
				parser->method = HTTP_GET;
				break;
			case 'H':
				parser->method = HTTP_HEAD;
				break;
			case 'L':
				parser->method = HTTP_LOCK; /* or LINK */
				break;
			case 'M':
				parser->method =
					HTTP_MKCOL;
				/* or MOVE, MKACTIVITY, MERGE, M-SEARCH,
				 * MKCALENDAR
				 */
				break;
			case 'N':
				parser->method = HTTP_NOTIFY;
				break;
			case 'O':
				parser->method = HTTP_OPTIONS;
				break;
			case 'P':
				parser->method = HTTP_POST;
				/* or PROPFIND|PROPPATCH|PUT|PATCH|PURGE */
				break;
			case 'R':
				parser->method = HTTP_REPORT; /* or REBIND */
				break;
			case 'S':
				parser->method = HTTP_SUBSCRIBE; /* or SEARCH */
				break;
			case 'T':
				parser->method = HTTP_TRACE;
				break;
			case 'U':
				parser->method = HTTP_UNLOCK;
				/* or UNSUBSCRIBE, UNBIND, UNLINK */
				break;
			default:
				SET_ERRNO(HPE_INVALID_METHOD);
				goto error;
			}
			UPDATE_STATE(s_req_method);


			rc = cb_notify(parser, &p_state,
				       settings->on_message_begin,
				       HPE_CB_message_begin, parsed,
				       p - data + 1);
			if (rc != 0) {
				return rc;
			}
			break;
		}

		case s_req_method: {
			const char *matcher;

			if (UNLIKELY(ch == '\0')) {
				SET_ERRNO(HPE_INVALID_METHOD);
				goto error;
			}

			matcher = method_strings[parser->method];
			if (ch == ' ' && matcher[parser->index] == '\0') {
				UPDATE_STATE(s_req_spaces_before_url);
			} else if (ch == matcher[parser->index]) {
				; /* nada */
			} else if (IS_ALPHA(ch)) {

				uint64_t sw_option = parser->method << 16 |
						     parser->index << 8 | ch;
				switch (sw_option) {
				case (HTTP_POST << 16 | 1 << 8 | 'U'):
					parser->method = HTTP_PUT;
					break;
				case (HTTP_POST << 16 | 1 << 8 | 'A'):
					parser->method = HTTP_PATCH;
					break;
				case (HTTP_CONNECT << 16 | 1 << 8 | 'H'):
					parser->method = HTTP_CHECKOUT;
					break;
				case (HTTP_CONNECT << 16 | 2 << 8 | 'P'):
					parser->method = HTTP_COPY;
					break;
				case (HTTP_MKCOL << 16 | 1 << 8 | 'O'):
					parser->method = HTTP_MOVE;
					break;
				case (HTTP_MKCOL << 16 | 1 << 8 | 'E'):
					parser->method = HTTP_MERGE;
					break;
				case (HTTP_MKCOL << 16 | 2 << 8 | 'A'):
					parser->method = HTTP_MKACTIVITY;
					break;
				case (HTTP_MKCOL << 16 | 3 << 8 | 'A'):
					parser->method = HTTP_MKCALENDAR;
					break;
				case (HTTP_SUBSCRIBE << 16 | 1 << 8 | 'E'):
					parser->method = HTTP_SEARCH;
					break;
				case (HTTP_REPORT << 16 | 2 << 8 | 'B'):
					parser->method = HTTP_REBIND;
					break;
				case (HTTP_POST << 16 | 1 << 8 | 'R'):
					parser->method = HTTP_PROPFIND;
					break;
				case (HTTP_PROPFIND << 16 | 4 << 8 | 'P'):
					parser->method = HTTP_PROPPATCH;
					break;
				case (HTTP_PUT << 16 | 2 << 8 | 'R'):
					parser->method = HTTP_PURGE;
					break;
				case (HTTP_LOCK << 16 | 1 << 8 | 'I'):
					parser->method = HTTP_LINK;
					break;
				case (HTTP_UNLOCK << 16 | 2 << 8 | 'S'):
					parser->method = HTTP_UNSUBSCRIBE;
					break;
				case (HTTP_UNLOCK << 16 | 2 << 8 | 'B'):
					parser->method = HTTP_UNBIND;
					break;
				case (HTTP_UNLOCK << 16 | 3 << 8 | 'I'):
					parser->method = HTTP_UNLINK;
					break;
				default:
					parser->http_errno = HPE_INVALID_METHOD;
					goto error;
				}
			} else if (ch == '-' &&
					parser->index == 1U &&
					parser->method == HTTP_MKCOL) {
				parser->method = HTTP_MSEARCH;
			} else {
				SET_ERRNO(HPE_INVALID_METHOD);
				goto error;
			}

			++parser->index;
			break;
		}

		case s_req_spaces_before_url: {
			if (ch == ' ') {
				break;
			}

			MARK(url);
			if (parser->method == HTTP_CONNECT) {
				UPDATE_STATE(s_req_server_start);
			}

			UPDATE_STATE(parse_url_char(CURRENT_STATE(), ch));
			if (UNLIKELY(CURRENT_STATE() == s_dead)) {
				SET_ERRNO(HPE_INVALID_URL);
				goto error;
			}

			break;
		}

		case s_req_schema:
		case s_req_schema_slash:
		case s_req_schema_slash_slash:
		case s_req_server_start: {
			switch (ch) {
			/* No whitespace allowed here */
			case ' ':
			case CR:
			case LF:
				SET_ERRNO(HPE_INVALID_URL);
				goto error;
			default:
				UPDATE_STATE
					(parse_url_char(CURRENT_STATE(), ch));
				if (UNLIKELY(CURRENT_STATE() == s_dead)) {
					SET_ERRNO(HPE_INVALID_URL);
					goto error;
				}
			}

			break;
		}

		case s_req_server:
		case s_req_server_with_at:
		case s_req_path:
		case s_req_query_string_start:
		case s_req_query_string:
		case s_req_fragment_start:
		case s_req_fragment: {
			switch (ch) {
			case ' ':
				UPDATE_STATE(s_req_http_start);
				rc = cb_data(parser, settings->on_url,
					     HPE_CB_url, &p_state, parsed,
					     p - data + 1, &url_mark,
					     p - url_mark);
				if (rc != 0) {
					return rc;
				}

				break;
			case CR:
			case LF:
				parser->http_major = 0U;
				parser->http_minor = 9U;
				UPDATE_STATE((ch == CR) ?
					     s_req_line_almost_done :
					     s_header_field_start);
				rc = cb_data(parser, settings->on_url,
					     HPE_CB_url, &p_state, parsed,
					     p - data + 1, &url_mark,
					     p - url_mark);
				if (rc != 0) {
					return rc;
				}

				break;
			default:
				UPDATE_STATE
					(parse_url_char(CURRENT_STATE(), ch));
				if (UNLIKELY(CURRENT_STATE() == s_dead)) {
					SET_ERRNO(HPE_INVALID_URL);
					goto error;
				}
			}
			break;
		}

		case s_req_http_start:
			switch (ch) {
			case 'H':
				UPDATE_STATE(s_req_http_H);
				break;
			case ' ':
				break;
			default:
				SET_ERRNO(HPE_INVALID_CONSTANT);
				goto error;
			}
			break;

		case s_req_http_H:
			rc = strict_check(parser, ch != 'T');
			if (rc != 0) {
				goto error;
			}
			UPDATE_STATE(s_req_http_HT);
			break;

		case s_req_http_HT:
			rc = strict_check(parser, ch != 'T');
			if (rc != 0) {
				goto error;
			}
			UPDATE_STATE(s_req_http_HTT);
			break;

		case s_req_http_HTT:
			rc = strict_check(parser, ch != 'P');
			if (rc != 0) {
				goto error;
			}
			UPDATE_STATE(s_req_http_HTTP);
			break;

		case s_req_http_HTTP:
			rc = strict_check(parser, ch != '/');
			if (rc != 0) {
				goto error;
			}
			UPDATE_STATE(s_req_first_http_major);
			break;

		/* first digit of major HTTP version */
		case s_req_first_http_major:
			if (UNLIKELY(ch < '1' || ch > '9')) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_major = ch - '0';
			UPDATE_STATE(s_req_http_major);
			break;

		/* major HTTP version or dot */
		case s_req_http_major: {
			if (ch == '.') {
				UPDATE_STATE(s_req_first_http_minor);
				break;
			}

			if (UNLIKELY(!IS_NUM(ch))) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_major *= 10U;
			parser->http_major += ch - '0';

			if (UNLIKELY(parser->http_major > 999)) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			break;
		}

		/* first digit of minor HTTP version */
		case s_req_first_http_minor:
			if (UNLIKELY(!IS_NUM(ch))) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_minor = ch - '0';
			UPDATE_STATE(s_req_http_minor);
			break;

		/* minor HTTP version or end of request line */
		case s_req_http_minor: {
			if (ch == CR) {
				UPDATE_STATE(s_req_line_almost_done);
				break;
			}

			if (ch == LF) {
				UPDATE_STATE(s_header_field_start);
				break;
			}

			/* XXX allow spaces after digit? */

			if (UNLIKELY(!IS_NUM(ch))) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_minor *= 10U;
			parser->http_minor += ch - '0';

			if (UNLIKELY(parser->http_minor > 999)) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			break;
		}

		/* end of request line */
		case s_req_line_almost_done: {
			if (UNLIKELY(ch != LF)) {
				SET_ERRNO(HPE_LF_EXPECTED);
				goto error;
			}

			UPDATE_STATE(s_header_field_start);
			break;
		}

		case s_header_field_start: {
			if (ch == CR) {
				UPDATE_STATE(s_headers_almost_done);
				break;
			}

			if (ch == LF) {
				/* they might be just sending \n instead of \r\n
				 * so this would be
				 * the second \n to denote the end of headers
				 */
				UPDATE_STATE(s_headers_almost_done);
				goto reexecute;
			}

			c = TOKEN(ch);

			if (UNLIKELY(!c)) {
				SET_ERRNO(HPE_INVALID_HEADER_TOKEN);
				goto error;
			}

			MARK(header_field);

			parser->index = 0U;
			UPDATE_STATE(s_header_field);

			switch (c) {
			case 'c':
				parser->header_state = h_C;
				break;

			case 'p':
				parser->header_state =
					h_matching_proxy_connection;
				break;

			case 't':
				parser->header_state =
					h_matching_transfer_encoding;
				break;

			case 'u':
				parser->header_state = h_matching_upgrade;
				break;

			default:
				parser->header_state = h_general;
				break;
			}
			break;
		}

		case s_header_field: {
			const char *start = p;

			for (; p != data + len; p++) {
				ch = *p;
				c = TOKEN(ch);

				if (!c) {
					break;
				}
				parser_header_state(parser, ch, c);
			}

			rc = count_header_size(parser, p - start);
			if (rc != 0) {
				goto error;
			}

			if (p == data + len) {
				--p;
				break;
			}

			if (ch == ':') {
				UPDATE_STATE(s_header_value_discard_ws);
				rc = cb_data(parser, settings->on_header_field,
					     HPE_CB_header_field, &p_state,
					     parsed, p - data + 1,
					     &header_field_mark,
					     p - header_field_mark);
				if (rc != 0) {
					return rc;
				}

				break;
			}

			SET_ERRNO(HPE_INVALID_HEADER_TOKEN);
			goto error;
		}

		case s_header_value_discard_ws:
			if (ch == ' ' || ch == '\t') {
				break;
			}

			if (ch == CR) {
				UPDATE_STATE
					(s_header_value_discard_ws_almost_done);
				break;
			}

			if (ch == LF) {
				UPDATE_STATE(s_header_value_discard_lws);
				break;
			}

			__fallthrough;

		case s_header_value_start: {
			MARK(header_value);

			UPDATE_STATE(s_header_value);
			parser->index = 0U;

			c = LOWER(ch);

			switch (parser->header_state) {
			case h_upgrade:
				parser->flags |= F_UPGRADE;
				parser->header_state = h_general;
				break;

			case h_transfer_encoding:
				/* looking for 'Transfer-Encoding: chunked' */
				if ('c' == c) {
					parser->header_state =
					   h_matching_transfer_encoding_chunked;
				} else {
					parser->header_state = h_general;
				}
				break;

			case h_content_length:
				if (UNLIKELY(!IS_NUM(ch))) {
					SET_ERRNO(HPE_INVALID_CONTENT_LENGTH);
					goto error;
				}

				if (parser->flags & F_CONTENTLENGTH) {
					SET_ERRNO
						(HPE_UNEXPECTED_CONTENT_LENGTH);
					goto error;
				}

				parser->flags |= F_CONTENTLENGTH;
				parser->content_length = ch - '0';
				break;

			case h_connection:
				/* looking for 'Connection: keep-alive' */
				if (c == 'k') {
					parser->header_state =
					h_matching_connection_keep_alive;
					/* looking for 'Connection: close' */
				} else if (c == 'c') {
					parser->header_state =
						h_matching_connection_close;
				} else if (c == 'u') {
					parser->header_state =
						h_matching_connection_upgrade;
				} else {
					parser->header_state =
						h_matching_connection_token;
				}
				break;

			/* Multi-value `Connection` header */
			case h_matching_connection_token_start:
				break;

			default:
				parser->header_state = h_general;
				break;
			}
			break;
		}

		case s_header_value: {
			const char *start = p;
			enum header_states h_state =
				(enum header_states)parser->header_state;

			for (; p != data + len; p++) {
				ch = *p;
				if (ch == CR) {
					UPDATE_STATE(s_header_almost_done);
					parser->header_state = h_state;
					rc = cb_data(parser,
						     settings->on_header_value,
						     HPE_CB_header_value,
						     &p_state, parsed,
						     p - data + 1,
						     &header_value_mark,
						     p - header_value_mark);
					if (rc != 0) {
						return rc;
					}

					break;
				}

				if (ch == LF) {
					UPDATE_STATE(s_header_almost_done);
					rc = count_header_size(parser,
							       p - start);
					if (rc != 0) {
						goto error;
					}
					parser->header_state = h_state;
					rc = cb_data(parser,
						     settings->on_header_value,
						     HPE_CB_header_value,
						     &p_state, parsed, p - data,
						     &header_value_mark,
						     p - header_value_mark);
					if (rc != 0) {
						return rc;
					}

					goto reexecute;
				}

				if (!lenient && !IS_HEADER_CHAR(ch)) {
					SET_ERRNO(HPE_INVALID_HEADER_TOKEN);
					goto error;
				}

				c = LOWER(ch);

				rc = header_states(parser, data, len, &p,
						   &p_state, &h_state, ch, c);
				if (rc != 0) {
					goto error;
				}

			}
			parser->header_state = h_state;

			rc = count_header_size(parser, p - start);
			if (rc != 0) {
				goto error;
			}

			if (p == data + len) {
				--p;
			}
			break;
		}

		case s_header_almost_done: {
			if (UNLIKELY(ch != LF)) {
				SET_ERRNO(HPE_LF_EXPECTED);
				goto error;
			}

			UPDATE_STATE(s_header_value_lws);
			break;
		}

		case s_header_value_lws: {
			if (ch == ' ' || ch == '\t') {
				UPDATE_STATE(s_header_value_start);
				goto reexecute;
			}

			/* finished the header */
			switch (parser->header_state) {
			case h_connection_keep_alive:
				parser->flags |= F_CONNECTION_KEEP_ALIVE;
				break;
			case h_connection_close:
				parser->flags |= F_CONNECTION_CLOSE;
				break;
			case h_transfer_encoding_chunked:
				parser->flags |= F_CHUNKED;
				break;
			case h_connection_upgrade:
				parser->flags |= F_CONNECTION_UPGRADE;
				break;
			default:
				break;
			}

			UPDATE_STATE(s_header_field_start);
			goto reexecute;
		}

		case s_header_value_discard_ws_almost_done: {
			rc = strict_check(parser, ch != LF);
			if (rc != 0) {
				goto error;
			}
			UPDATE_STATE(s_header_value_discard_lws);
			break;
		}

		case s_header_value_discard_lws: {
			if (ch == ' ' || ch == '\t') {
				UPDATE_STATE(s_header_value_discard_ws);
				break;
			}
			switch (parser->header_state) {
			case h_connection_keep_alive:
				parser->flags |=
					F_CONNECTION_KEEP_ALIVE;
				break;
			case h_connection_close:
				parser->flags |= F_CONNECTION_CLOSE;
				break;
			case h_connection_upgrade:
				parser->flags |= F_CONNECTION_UPGRADE;
				break;
			case h_transfer_encoding_chunked:
				parser->flags |= F_CHUNKED;
				break;
			default:
				break;
			}

			/* header value was empty */
			MARK(header_value);
			UPDATE_STATE(s_header_field_start);
			rc = cb_data(parser, settings->on_header_value,
				     HPE_CB_header_value, &p_state, parsed,
				     p - data, &header_value_mark,
				     p - header_value_mark);
			if (rc != 0) {
				return rc;
			}

			goto reexecute;
		}

		case s_headers_almost_done: {
			rc = strict_check(parser, ch != LF);
			if (rc != 0) {
				goto error;
			}

			if (parser->flags & F_TRAILING) {
				/* End of a chunked request */
				UPDATE_STATE(s_message_done);

				rc = cb_notify(parser, &p_state,
					       settings->on_chunk_complete,
					       HPE_CB_chunk_complete, parsed,
					       p - data);
				if (rc != 0) {
					return rc;
				}
				goto reexecute;
			}

			/* Cannot use chunked encoding and a content-length
			 * header together per the HTTP specification.
			 */
			if ((parser->flags & F_CHUNKED) &&
					(parser->flags & F_CONTENTLENGTH)) {
				SET_ERRNO(HPE_UNEXPECTED_CONTENT_LENGTH);
				goto error;
			}

			UPDATE_STATE(s_headers_done);

			/* Set this here so that on_headers_complete() callbacks
			 * can see it
			 */
			int flags = (F_UPGRADE | F_CONNECTION_UPGRADE);

			parser->upgrade =
				((parser->flags & flags) == flags ||
				 parser->method == HTTP_CONNECT);

			/* Here we call the headers_complete callback. This is
			 * somewhat
			 * different than other callbacks because if the user
			 * returns 1, we
			 * will interpret that as saying that this message has
			 * no body. This
			 * is needed for the annoying case of receiving a
			 * response to a HEAD
			 * request.
			 *
			 * We'd like to use CALLBACK_NOTIFY_NOADVANCE() here but
			 * we cannot, so
			 * we have to simulate it by handling a change in errno
			 * below.
			 */
			if (settings->on_headers_complete) {
				switch (settings->on_headers_complete(parser)) {
				case 0:
					break;

				case 2:
					parser->upgrade = 1U;
					__fallthrough;

				case 1:
					parser->flags |= F_SKIPBODY;
					break;

				default:
					SET_ERRNO(HPE_CB_headers_complete);

					parser->state = CURRENT_STATE();
					*parsed = p - data;
					return -HTTP_PARSER_ERRNO(parser);
				}
			}

			if (HTTP_PARSER_ERRNO(parser) != HPE_OK) {
				parser->state = CURRENT_STATE();
				*parsed = p - data;
				return -HTTP_PARSER_ERRNO(parser);
			}

			goto reexecute;
		}

		case s_headers_done: {
			int hasBody;

			rc = strict_check(parser, ch != LF);
			if (rc != 0) {
				goto error;
			}

			parser->nread = 0U;

			hasBody = parser->flags & F_CHUNKED ||
				  (parser->content_length > 0 &&
				   parser->content_length != ULLONG_MAX);
			if (parser->upgrade &&
				(parser->method == HTTP_CONNECT ||
				 (parser->flags & F_SKIPBODY) || !hasBody)) {
				/* Exit, the rest of the message is in a
				 * different protocol.
				 */
				UPDATE_STATE(NEW_MESSAGE());

				rc = cb_notify(parser, &p_state,
					       settings->on_message_complete,
					       HPE_CB_message_complete, parsed,
					       p - data + 1);
				if (rc != 0) {
					return rc;
				}
				parser->state = CURRENT_STATE();
				*parsed = p - data + 1;
				return 0;
			}

			if (parser->flags & F_SKIPBODY) {
				UPDATE_STATE(NEW_MESSAGE());

				rc = cb_notify(parser, &p_state,
					       settings->on_message_complete,
					       HPE_CB_message_complete, parsed,
					       p - data + 1);
				if (rc != 0) {
					return rc;
				}

			} else if (parser->flags & F_CHUNKED) {
				/* chunked encoding - ignore Content-Length
				 * header
				 */
				UPDATE_STATE(s_chunk_size_start);
			} else {
				rc = zero_content_length(parser, settings,
							 &p_state, parsed, p,
							 data);
				if (rc != 0) {
					return rc;
				}
			}

			break;
		}

		case s_body_identity: {
			uint64_t to_read = MIN(parser->content_length,
					       (uint64_t) ((data + len) - p));

			__ASSERT_NO_MSG(parser->content_length != 0U
			       && parser->content_length != ULLONG_MAX);

			/* The difference between advancing content_length and
			 * p is because
			 * the latter will automatically advance on the next
			 * loop
			 * iteration.
			 * Further, if content_length ends up at 0, we want to
			 * see the last
			 * byte again for our message complete callback.
			 */
			MARK(body);
			parser->content_length -= to_read;
			p += to_read - 1;

			if (parser->content_length == 0U) {
				UPDATE_STATE(s_message_done);

				/* Mimic CALLBACK_DATA_NOADVANCE() but with one
				 * extra byte.
				 *
				 * The alternative to doing this is to wait for
				 * the next byte to
				 * trigger the data callback, just as in every
				 * other case. The
				 * problem with this is that this makes it
				 * difficult for the test
				 * harness to distinguish between
				 * complete-on-EOF and
				 * complete-on-length. It's not clear that this
				 * distinction is
				 * important for applications, but let's keep it
				 * for now.
				 */

				rc = cb_data(parser, settings->on_body,
					     HPE_CB_body, &p_state, parsed,
					     p - data, &body_mark,
					     p - body_mark + 1);
				if (rc != 0) {
					return rc;
				}

				goto reexecute;
			}

			break;
		}

		/* read until EOF */
		case s_body_identity_eof:
			MARK(body);
			p = data + len - 1;

			break;

		case s_message_done:
			UPDATE_STATE(NEW_MESSAGE());

			rc = cb_notify(parser, &p_state,
				       settings->on_message_complete,
				       HPE_CB_message_complete, parsed,
				       p - data + 1);
			if (rc != 0) {
				return rc;
			}
			if (parser->upgrade) {
				/* Exit, the rest of the message is in a
				 * different protocol.
				 */
				parser->state = CURRENT_STATE();
				*parsed = p - data + 1;
				return 0;
			}
			break;

		case s_chunk_size_start: {
			__ASSERT_NO_MSG(parser->nread == 1U);
			__ASSERT_NO_MSG(parser->flags & F_CHUNKED);

			unhex_val = unhex[(unsigned char)ch];
			if (UNLIKELY(unhex_val == -1)) {
				SET_ERRNO(HPE_INVALID_CHUNK_SIZE);
				goto error;
			}

			parser->content_length = unhex_val;
			UPDATE_STATE(s_chunk_size);
			break;
		}

		case s_chunk_size: {
			uint64_t t;

			__ASSERT_NO_MSG(parser->flags & F_CHUNKED);

			if (ch == CR) {
				UPDATE_STATE(s_chunk_size_almost_done);
				break;
			}

			unhex_val = unhex[(unsigned char)ch];

			if (unhex_val == -1) {
				if (ch == ';' || ch == ' ') {
					UPDATE_STATE(s_chunk_parameters);
					break;
				}

				SET_ERRNO(HPE_INVALID_CHUNK_SIZE);
				goto error;
			}

			t = parser->content_length;
			t *= 16U;
			t += unhex_val;

			/* Overflow? Test against a conservative limit for
			 * simplicity.
			 */
			uint64_t ulong_value = (ULLONG_MAX - 16) / 16;

			if (UNLIKELY(ulong_value < parser->content_length)) {
				SET_ERRNO(HPE_INVALID_CONTENT_LENGTH);
				goto error;
			}

			parser->content_length = t;
			break;
		}

		case s_chunk_parameters: {
			__ASSERT_NO_MSG(parser->flags & F_CHUNKED);
			/* just ignore this shit. TODO check for overflow */
			if (ch == CR) {
				UPDATE_STATE(s_chunk_size_almost_done);
				break;
			}
			break;
		}

		case s_chunk_size_almost_done: {
			__ASSERT_NO_MSG(parser->flags & F_CHUNKED);

			rc = strict_check(parser, ch != LF);
			if (rc != 0) {
				goto error;
			}

			parser->nread = 0U;

			if (parser->content_length == 0U) {
				parser->flags |= F_TRAILING;
				UPDATE_STATE(s_header_field_start);
			} else {
				UPDATE_STATE(s_chunk_data);
			}

			rc = cb_notify(parser, &p_state,
				       settings->on_chunk_header,
				       HPE_CB_chunk_header, parsed,
				       p - data + 1);
			if (rc != 0) {
				return rc;
			}
			break;
		}

		case s_chunk_data: {
			uint64_t to_read = MIN(parser->content_length,
					       (uint64_t) ((data + len) - p));

			__ASSERT_NO_MSG(parser->flags & F_CHUNKED);
			__ASSERT_NO_MSG(parser->content_length != 0U
					&& parser->content_length != ULLONG_MAX);

			/* See the explanation in s_body_identity for why the
			 * content
			 * length and data pointers are managed this way.
			 */
			MARK(body);
			parser->content_length -= to_read;
			p += to_read - 1;

			if (parser->content_length == 0U) {
				UPDATE_STATE(s_chunk_data_almost_done);
			}

			break;
		}

		case s_chunk_data_almost_done:
			__ASSERT_NO_MSG(parser->flags & F_CHUNKED);
			__ASSERT_NO_MSG(parser->content_length == 0U);
			rc = strict_check(parser, ch != CR);
			if (rc != 0) {
				goto error;
			}
			UPDATE_STATE(s_chunk_data_done);
			rc = cb_data(parser, settings->on_body, HPE_CB_body,
				     &p_state, parsed, p - data + 1, &body_mark,
				     p - body_mark);
			if (rc != 0) {
				return rc;
			}

			break;

		case s_chunk_data_done:
			__ASSERT_NO_MSG(parser->flags & F_CHUNKED);
			rc = strict_check(parser, ch != LF);
			if (rc != 0) {
				goto error;
			}
			parser->nread = 0U;
			UPDATE_STATE(s_chunk_size_start);

			rc = cb_notify(parser, &p_state,
				       settings->on_chunk_complete,
				       HPE_CB_chunk_complete, parsed,
				       p - data + 1);
			if (rc != 0) {
				return rc;
			}
			break;

		default:
			__ASSERT_NO_MSG(0 && "unhandled state");
			SET_ERRNO(HPE_INVALID_INTERNAL_STATE);
			goto error;
		}
	}

	/* Run callbacks for any marks that we have leftover after we ran our of
	 * bytes. There should be at most one of these set, so it's OK to invoke
	 * them in series (unset marks will not result in callbacks).
	 *
	 * We use the NOADVANCE() variety of callbacks here because 'p' has
	 * already
	 * overflowed 'data' and this allows us to correct for the off-by-one
	 * that
	 * we'd otherwise have (since CALLBACK_DATA() is meant to be run with a
	 * 'p'
	 * value that's in-bounds).
	 */

	__ASSERT_NO_MSG(((header_field_mark ? 1 : 0) +
			(header_value_mark ? 1 : 0) +
			(url_mark ? 1 : 0)  +
			(body_mark ? 1 : 0) +
			(status_mark ? 1 : 0)) <= 1);

	rc = cb_data(parser, settings->on_header_field, HPE_CB_header_field,
		     &p_state, parsed, p - data, &header_field_mark,
		     p - header_field_mark);
	if (rc != 0) {
		return rc;
	}
	rc = cb_data(parser, settings->on_header_value, HPE_CB_header_value,
		     &p_state, parsed, p - data, &header_value_mark,
		     p - header_value_mark);
	if (rc != 0) {
		return rc;
	}
	rc = cb_data(parser, settings->on_url, HPE_CB_url, &p_state, parsed,
		     p - data, &url_mark, p - url_mark);
	if (rc != 0) {
		return rc;
	}
	rc = cb_data(parser, settings->on_body, HPE_CB_body, &p_state, parsed,
		     p - data, &body_mark, p - body_mark);
	if (rc != 0) {
		return rc;
	}
	rc = cb_data(parser, settings->on_status, HPE_CB_status, &p_state,
		     parsed, p - data, &status_mark, p - status_mark);
	if (rc != 0) {
		return rc;
	}

	parser->state = CURRENT_STATE();
	*parsed = len;
	return 0;

error:
	if (HTTP_PARSER_ERRNO(parser) == HPE_OK) {
		SET_ERRNO(HPE_UNKNOWN);
	}

	parser->state = CURRENT_STATE();
	*parsed = p - data; /* Error */
	return -HTTP_PARSER_ERRNO(parser);
}

size_t http_parser_execute(struct http_parser *parser,
			   const struct http_parser_settings *settings,
			   const char *data, size_t len)
{
	size_t parsed;

	parser_execute(parser, settings, data, len, &parsed);
	return parsed;
}

/* Does the parser need to see an EOF to find the end of the message? */
int http_message_needs_eof(const struct http_parser *parser)
{
	if (parser->type == HTTP_REQUEST) {
		return 0;
	}

	/* See RFC 2616 section 4.4 */
	if (parser->status_code / 100 == 1U || /* 1xx e.g. Continue */
			parser->status_code == 204U ||     /* No Content */
			parser->status_code == 304U ||     /* Not Modified */
			parser->flags & F_SKIPBODY) {     /* response to a HEAD
							   * request
							   */
		return 0;
	}

	if ((parser->flags & F_CHUNKED) ||
			parser->content_length != ULLONG_MAX) {
		return 0;
	}

	return 1;
}


int http_should_keep_alive(const struct http_parser *parser)
{
	if (parser->http_major > 0 && parser->http_minor > 0) {
		/* HTTP/1.1 */
		if (parser->flags & F_CONNECTION_CLOSE) {
			return 0;
		}
	} else {
		/* HTTP/1.0 or earlier */
		if (!(parser->flags & F_CONNECTION_KEEP_ALIVE)) {
			return 0;
		}
	}

	return !http_message_needs_eof(parser);
}


const char *http_method_str(enum http_method m)
{
	return ELEM_AT(method_strings, m, "<unknown>");
}


void http_parser_init(struct http_parser *parser, enum http_parser_type t)
{
	void *data = parser->data; /* preserve application data */

	(void)memset(parser, 0, sizeof(*parser));
	parser->data = data;
	parser->type = t;
	parser->state =
		(t == HTTP_REQUEST ? s_start_req :
		 (t == HTTP_RESPONSE ? s_start_res : s_start_req_or_res));
	parser->http_errno = HPE_OK;
}

void http_parser_settings_init(struct http_parser_settings *settings)
{
	(void)memset(settings, 0, sizeof(*settings));
}

const char *http_errno_name(enum http_errno err)
{
	__ASSERT_NO_MSG(((size_t) err) < ARRAY_SIZE(http_strerror_tab));

	return http_strerror_tab[err].name;
}

const char *http_errno_description(enum http_errno err)
{
	__ASSERT_NO_MSG(((size_t) err) < ARRAY_SIZE(http_strerror_tab));

	return http_strerror_tab[err].description;
}

void http_parser_pause(struct http_parser *parser, int paused)
{
	/* Users should only be pausing/unpausing a parser that is not in an
	 * error
	 * state. In non-debug builds, there's not much that we can do about
	 * this
	 * other than ignore it.
	 */
	if (HTTP_PARSER_ERRNO(parser) == HPE_OK ||
			HTTP_PARSER_ERRNO(parser) == HPE_PAUSED) {
		SET_ERRNO((paused) ? HPE_PAUSED : HPE_OK);
	} else {
		__ASSERT_NO_MSG(0 && "Attempting to pause parser in error state");
	}
}

int http_body_is_final(const struct http_parser *parser)
{
	return parser->state == s_message_done;
}

unsigned long http_parser_version(void)
{
	return HTTP_PARSER_VERSION_MAJOR * 0x10000 |
	       HTTP_PARSER_VERSION_MINOR * 0x00100 |
	       HTTP_PARSER_VERSION_PATCH * 0x00001;
}
