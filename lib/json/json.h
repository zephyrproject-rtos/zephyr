/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __JSON_H
#define __JSON_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

enum json_tokens {
	JSON_TOK_NONE = '_',
	JSON_TOK_OBJECT_START = '{',
	JSON_TOK_OBJECT_END = '}',
	JSON_TOK_STRING = '"',
	JSON_TOK_COLON = ':',
	JSON_TOK_COMMA = ',',
	JSON_TOK_NUMBER = '0',
	JSON_TOK_TRUE = 't',
	JSON_TOK_FALSE = 'f',
	JSON_TOK_NULL = 'n',
	JSON_TOK_ERROR = '!',
	JSON_TOK_EOF = '\0',
};

struct json_obj_descr {
	const char *field_name;
	size_t field_name_len;
	size_t offset;

	/* Valid values here: JSON_TOK_STRING, JSON_TOK_NUMBER,
	 * JSON_TOK_TRUE, JSON_TOK_FALSE. (All others ignored.)
	 */
	enum json_tokens type;
};

/**
 * @brief Parses the JSON-encoded object pointer to by @param json, with
 * size @param len, according to the descriptor pointed to by @param descr.
 * Values are stored in a struct pointed to by @param val.  Set up the
 * descriptor like this:
 *
 *    struct s { int foo; char *bar; }
 *    struct json_obj_descr descr[] = {
 *       { .field_name = "foo",
 *         .field_name_len = 3,
 *         .offset = offsetof(struct s, foo),
 *         .type = JSON_TOK_NUMBER },
 *       { .field_name = "bar",
 *         .field_name_len = 3,
 *         .offset = offsetof(struct s, bar),
 *         .type = JSON_TOK_STRING }
 *    };
 *
 * Since this parser is designed for machine-to-machine communications,
 * some liberties were taken to simplify the design: (1) strings are not
 * unescaped; (2) no UTF-8 validation is performed; (3) only integer
 * numbers are supported; (4) nested objects are not supported, including
 * arrays and objects within objects.
 *
 * @param json Pointer to JSON-encoded value to be parsed
 *
 * @param len Length of JSON-encoded value
 *
 * @param descr Pointer to the descriptor array
 *
 * @param descr_len Number of elements in the descriptor array. Must be less
 * than 31 due to implementation detail reasons (if more fields are
 * necessary, use two descriptors)
 *
 * @param val Pointer to the struct to hold the decoded values
 *
 * @return < 0 if error, bitmap of decoded fields on success (bit 0
 * is set if first field in the descriptor has been properly decoded, etc).
 */
int json_obj_parse(char *json, size_t len,
	const struct json_obj_descr *descr, size_t descr_len,
	void *val);

/**
 * @brief Escapes the string so it can be used to encode JSON objects
 *
 * @param str The string to escape; the escape string is stored the
 * buffer pointed to by this parameter
 *
 * @param len Points to a size_t containing the size before and after
 * the escaping process
 *
 * @param buf_size The size of buffer str points to
 *
 * @return 0 if string has been escaped properly, or -ENOMEM if there
 * was not enough space to escape the buffer
 */
ssize_t json_escape(char *str, size_t *len, size_t buf_size);

/**
 * @brief Calculates the JSON-escaped string length
 *
 * @param str The string to analyze
 *
 * @param len String size
 *
 * @return The length str would have if it were escaped
 */
size_t json_calc_escaped_len(const char *str, size_t len);

#endif /* __JSON_H */
