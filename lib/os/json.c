/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/__assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/types.h>

#include <zephyr/data/json.h>

struct json_obj_key_value {
	const char *key;
	size_t key_len;
	struct json_token value;
};

static bool lexer_consume(struct json_lexer *lex, struct json_token *tok,
			  enum json_tokens empty_token)
{
	if (lex->tok.type == empty_token) {
		return false;
	}

	*tok = lex->tok;
	lex->tok.type = empty_token;

	return true;
}

static bool lexer_next(struct json_lexer *lex, struct json_token *tok)
{
	while (lex->state) {
		if (lexer_consume(lex, tok, JSON_TOK_NONE)) {
			return true;
		}

		lex->state = lex->state(lex);
	}

	return lexer_consume(lex, tok, JSON_TOK_EOF);
}

static void *lexer_json(struct json_lexer *lex);

static void emit(struct json_lexer *lex, enum json_tokens token)
{
	lex->tok.type = token;
	lex->tok.start = lex->start;
	lex->tok.end = lex->pos;
	lex->start = lex->pos;
}

static int next(struct json_lexer *lex)
{
	if (lex->pos >= lex->end) {
		lex->pos = lex->end + 1;

		return '\0';
	}

	return *lex->pos++;
}

static void ignore(struct json_lexer *lex)
{
	lex->start = lex->pos;
}

static void backup(struct json_lexer *lex)
{
	lex->pos--;
}

static int peek(struct json_lexer *lex)
{
	int chr = next(lex);

	backup(lex);

	return chr;
}

static void *lexer_string(struct json_lexer *lex)
{
	ignore(lex);

	while (true) {
		int chr = next(lex);

		if (chr == '\0') {
			emit(lex, JSON_TOK_ERROR);
			return NULL;
		}

		if (chr == '\\') {
			switch (next(lex)) {
			case '"':
			case '\\':
			case '/':
			case 'b':
			case 'f':
			case 'n':
			case 'r':
			case 't':
				continue;
			case 'u':
				if (isxdigit(next(lex)) == 0) {
					goto error;
				}

				if (isxdigit(next(lex)) == 0) {
					goto error;
				}

				if (isxdigit(next(lex)) == 0) {
					goto error;
				}

				if (isxdigit(next(lex)) == 0) {
					goto error;
				}

				break;
			default:
				goto error;
			}
		}

		if (chr == '"') {
			backup(lex);
			emit(lex, JSON_TOK_STRING);

			next(lex);
			ignore(lex);

			return lexer_json;
		}
	}

error:
	emit(lex, JSON_TOK_ERROR);
	return NULL;
}

static int accept_run(struct json_lexer *lex, const char *run)
{
	for (; *run; run++) {
		if (next(lex) != *run) {
			return -EINVAL;
		}
	}

	return 0;
}

static void *lexer_boolean(struct json_lexer *lex)
{
	backup(lex);

	switch (next(lex)) {
	case 't':
		if (!accept_run(lex, "rue")) {
			emit(lex, JSON_TOK_TRUE);
			return lexer_json;
		}
		break;
	case 'f':
		if (!accept_run(lex, "alse")) {
			emit(lex, JSON_TOK_FALSE);
			return lexer_json;
		}
		break;
	}

	emit(lex, JSON_TOK_ERROR);
	return NULL;
}

static void *lexer_null(struct json_lexer *lex)
{
	if (accept_run(lex, "ull") < 0) {
		emit(lex, JSON_TOK_ERROR);
		return NULL;
	}

	emit(lex, JSON_TOK_NULL);
	return lexer_json;
}

static void *lexer_number(struct json_lexer *lex)
{
	while (true) {
		int chr = next(lex);

		if (isdigit(chr) != 0 || chr == '.') {
			continue;
		}

		backup(lex);
		emit(lex, JSON_TOK_NUMBER);

		return lexer_json;
	}
}

static void *lexer_json(struct json_lexer *lex)
{
	while (true) {
		int chr = next(lex);

		switch (chr) {
		case '\0':
			emit(lex, JSON_TOK_EOF);
			return NULL;
		case '}':
		case '{':
		case '[':
		case ']':
		case ',':
		case ':':
			emit(lex, (enum json_tokens)chr);
			return lexer_json;
		case '"':
			return lexer_string;
		case 'n':
			return lexer_null;
		case 't':
		case 'f':
			return lexer_boolean;
		case '-':
			if (isdigit(peek(lex)) != 0) {
				return lexer_number;
			}

			__fallthrough;
		default:
			if (isspace(chr) != 0) {
				ignore(lex);
				continue;
			}

			if (isdigit(chr) != 0) {
				return lexer_number;
			}

			emit(lex, JSON_TOK_ERROR);
			return NULL;
		}
	}
}

static void lexer_init(struct json_lexer *lex, char *data, size_t len)
{
	lex->state = lexer_json;
	lex->start = data;
	lex->pos = data;
	lex->end = data + len;
	lex->tok.type = JSON_TOK_NONE;
}

static int obj_init(struct json_obj *json, char *data, size_t len)
{
	struct json_token tok;

	lexer_init(&json->lex, data, len);

	if (!lexer_next(&json->lex, &tok)) {
		return -EINVAL;
	}

	if (tok.type != JSON_TOK_OBJECT_START) {
		return -EINVAL;
	}

	return 0;
}

static int arr_init(struct json_obj *json, char *data, size_t len)
{
	struct json_token tok;

	lexer_init(&json->lex, data, len);

	if (!lexer_next(&json->lex, &tok)) {
		return -EINVAL;
	}

	if (tok.type != JSON_TOK_ARRAY_START) {
		return -EINVAL;
	}

	return 0;
}

static int element_token(enum json_tokens token)
{
	switch (token) {
	case JSON_TOK_OBJECT_START:
	case JSON_TOK_ARRAY_START:
	case JSON_TOK_STRING:
	case JSON_TOK_NUMBER:
	case JSON_TOK_FLOAT:
	case JSON_TOK_OPAQUE:
	case JSON_TOK_OBJ_ARRAY:
	case JSON_TOK_TRUE:
	case JSON_TOK_FALSE:
		return 0;
	default:
		return -EINVAL;
	}
}

static int obj_next(struct json_obj *json,
		    struct json_obj_key_value *kv)
{
	struct json_token tok;

	if (!lexer_next(&json->lex, &tok)) {
		return -EINVAL;
	}

	/* Match end of object or next key */
	switch (tok.type) {
	case JSON_TOK_OBJECT_END:
		kv->key = NULL;
		kv->key_len = 0;
		kv->value = tok;

		return 0;
	case JSON_TOK_COMMA:
		if (!lexer_next(&json->lex, &tok)) {
			return -EINVAL;
		}

		if (tok.type != JSON_TOK_STRING) {
			return -EINVAL;
		}

		__fallthrough;
	case JSON_TOK_STRING:
		kv->key = tok.start;
		kv->key_len = (size_t)(tok.end - tok.start);
		break;
	default:
		return -EINVAL;
	}

	/* Match : after key */
	if (!lexer_next(&json->lex, &tok)) {
		return -EINVAL;
	}

	if (tok.type != JSON_TOK_COLON) {
		return -EINVAL;
	}

	/* Match value */
	if (!lexer_next(&json->lex, &kv->value)) {
		return -EINVAL;
	}

	return element_token(kv->value.type);
}

static int arr_next(struct json_obj *json, struct json_token *value)
{
	if (!lexer_next(&json->lex, value)) {
		return -EINVAL;
	}

	if (value->type == JSON_TOK_ARRAY_END) {
		return 0;
	}

	if (value->type == JSON_TOK_COMMA) {
		if (!lexer_next(&json->lex, value)) {
			return -EINVAL;
		}
	}

	return element_token(value->type);
}

static int decode_num(const struct json_token *token, int32_t *num)
{
	/* FIXME: strtod() is not available in newlib/minimal libc,
	 * so using strtol() here.
	 */
	char *endptr;
	char prev_end;

	prev_end = *token->end;
	*token->end = '\0';

	errno = 0;
	*num = strtol(token->start, &endptr, 10);

	*token->end = prev_end;

	if (errno != 0) {
		return -errno;
	}

	if (endptr != token->end) {
		return -EINVAL;
	}

	return 0;
}

static bool equivalent_types(enum json_tokens type1, enum json_tokens type2)
{
	if (type1 == JSON_TOK_TRUE || type1 == JSON_TOK_FALSE) {
		return type2 == JSON_TOK_TRUE || type2 == JSON_TOK_FALSE;
	}

	if (type1 == JSON_TOK_NUMBER && type2 == JSON_TOK_FLOAT) {
		return true;
	}

	if (type1 == JSON_TOK_STRING && type2 == JSON_TOK_OPAQUE) {
		return true;
	}

	if (type1 == JSON_TOK_ARRAY_START && type2 == JSON_TOK_OBJ_ARRAY) {
		return true;
	}

	return type1 == type2;
}

static int64_t obj_parse(struct json_obj *obj,
			 const struct json_obj_descr *descr, size_t descr_len,
			 void *val);
static int arr_parse(struct json_obj *obj,
		     const struct json_obj_descr *elem_descr,
		     size_t max_elements, void *field, void *val);

static int arr_data_parse(struct json_obj *obj, struct json_obj_token *val);

static int64_t decode_value(struct json_obj *obj,
			    const struct json_obj_descr *descr,
			    struct json_token *value, void *field, void *val)
{

	if (!equivalent_types(value->type, descr->type)) {
		return -EINVAL;
	}

	switch (descr->type) {
	case JSON_TOK_OBJECT_START:
		return obj_parse(obj, descr->object.sub_descr,
				 descr->object.sub_descr_len,
				 field);
	case JSON_TOK_ARRAY_START:
		return arr_parse(obj, descr->array.element_descr,
				 descr->array.n_elements, field, val);
	case JSON_TOK_OBJ_ARRAY: {
		struct json_obj_token *obj_token = field;

		obj_token->start = value->start;
		return arr_data_parse(obj, obj_token);
	}

	case JSON_TOK_FALSE:
	case JSON_TOK_TRUE: {
		bool *v = field;

		*v = value->type == JSON_TOK_TRUE;

		return 0;
	}
	case JSON_TOK_NUMBER: {
		int32_t *num = field;

		return decode_num(value, num);
	}
	case JSON_TOK_OPAQUE:
	case JSON_TOK_FLOAT: {
		struct json_obj_token *obj_token = field;

		obj_token->start = value->start;
		obj_token->length = value->end - value->start;
		return 0;
	}
	case JSON_TOK_STRING: {
		char **str = field;

		*value->end = '\0';
		*str = value->start;

		return 0;
	}
	default:
		return -EINVAL;
	}
}

static ptrdiff_t get_elem_size(const struct json_obj_descr *descr)
{
	switch (descr->type) {
	case JSON_TOK_NUMBER:
		return sizeof(int32_t);
	case JSON_TOK_OPAQUE:
	case JSON_TOK_FLOAT:
	case JSON_TOK_OBJ_ARRAY:
		return sizeof(struct json_obj_token);
	case JSON_TOK_STRING:
		return sizeof(char *);
	case JSON_TOK_TRUE:
	case JSON_TOK_FALSE:
		return sizeof(bool);
	case JSON_TOK_ARRAY_START: {
		ptrdiff_t size;

		size = descr->array.n_elements * get_elem_size(descr->array.element_descr);
		/* Consider additional item count field for array objects */
		if (descr->field_name_len > 0) {
			size = size + sizeof(size_t);
		}

		return size;
	}
	case JSON_TOK_OBJECT_START: {
		ptrdiff_t total = 0;
		size_t i;

		for (i = 0; i < descr->object.sub_descr_len; i++) {
			total += get_elem_size(&descr->object.sub_descr[i]);
		}

		return ROUND_UP(total, 1 << descr->align_shift);
	}
	default:
		return -EINVAL;
	}
}

static int arr_parse(struct json_obj *obj,
		     const struct json_obj_descr *elem_descr,
		     size_t max_elements, void *field, void *val)
{
	void *value = val;
	size_t *elements = (size_t *)((char *)value + elem_descr->offset);
	ptrdiff_t elem_size;
	void *last_elem;
	struct json_token tok;

	/* For nested arrays, skip parent descriptor to get elements */
	if (elem_descr->type == JSON_TOK_ARRAY_START) {
		elem_descr = elem_descr->array.element_descr;
	}

	*elements = 0;
	elem_size = get_elem_size(elem_descr);
	last_elem = (char *)field + elem_size * max_elements;

	__ASSERT_NO_MSG(elem_size > 0);

	while (!arr_next(obj, &tok)) {
		if (tok.type == JSON_TOK_ARRAY_END) {
			return 0;
		}

		if (field == last_elem) {
			return -ENOSPC;
		}

		/* For nested arrays, update value to current field,
		 * so it matches descriptor's offset to length field
		 */
		if (elem_descr->type == JSON_TOK_ARRAY_START) {
			value = field;
		}

		if (decode_value(obj, elem_descr, &tok, field, value) < 0) {
			return -EINVAL;
		}

		(*elements)++;
		field = (char *)field + elem_size;
	}

	return -EINVAL;
}

static int arr_data_parse(struct json_obj *obj, struct json_obj_token *val)
{
	bool string_state = false;
	int array_in_array = 1;

	/* Init length to zero */
	val->length = 0;

	while (obj->lex.pos != obj->lex.end) {
		if (string_state) {
			if (*obj->lex.pos == JSON_TOK_STRING) {
				string_state = false;
			}
		} else {
			if (*obj->lex.pos == JSON_TOK_ARRAY_END) {
				array_in_array--;
				if (array_in_array == 0) {
					/* Set array data length + 1 object end */
					val->length = obj->lex.pos - val->start + 1;
					/* Init Lexer that Object Parse can be finished properly */
					obj->lex.state = lexer_json;
					/* Move position to before array end */
					obj->lex.pos--;
					obj->lex.tok.end = obj->lex.pos;
					obj->lex.tok.start = val->start;
					obj->lex.tok.type = JSON_TOK_NONE;
					return 0;
				}
			} else if (*obj->lex.pos == JSON_TOK_STRING) {
				string_state = true;
			} else if (*obj->lex.pos == JSON_TOK_ARRAY_START) {
				/* arrary in array update structure count */
				array_in_array++;
			}
		}
		obj->lex.pos++;
	}

	return -EINVAL;
}

static int64_t obj_parse(struct json_obj *obj, const struct json_obj_descr *descr,
			 size_t descr_len, void *val)
{
	struct json_obj_key_value kv;
	int64_t decoded_fields = 0;
	size_t i;
	int ret;

	while (!obj_next(obj, &kv)) {
		if (kv.value.type == JSON_TOK_OBJECT_END) {
			return decoded_fields;
		}

		for (i = 0; i < descr_len; i++) {
			void *decode_field = (char *)val + descr[i].offset;

			/* Field has been decoded already, skip */
			if (decoded_fields & ((int64_t)1 << i)) {
				continue;
			}

			/* Check if it's the i-th field */
			if (kv.key_len != descr[i].field_name_len) {
				continue;
			}

			if (memcmp(kv.key, descr[i].field_name,
				   descr[i].field_name_len)) {
				continue;
			}

			/* Store the decoded value */
			ret = decode_value(obj, &descr[i], &kv.value,
					   decode_field, val);
			if (ret < 0) {
				return ret;
			}

			decoded_fields |= (int64_t)1<<i;
			break;
		}
	}

	return -EINVAL;
}

int64_t json_obj_parse(char *payload, size_t len,
		       const struct json_obj_descr *descr, size_t descr_len,
		       void *val)
{
	struct json_obj obj;
	int64_t ret;

	__ASSERT_NO_MSG(descr_len < (sizeof(ret) * CHAR_BIT - 1));

	ret = obj_init(&obj, payload, len);
	if (ret < 0) {
		return ret;
	}

	return obj_parse(&obj, descr, descr_len, val);
}

int json_arr_parse(char *payload, size_t len,
		   const struct json_obj_descr *descr, void *val)
{
	struct json_obj arr;
	int ret;

	ret = arr_init(&arr, payload, len);
	if (ret < 0) {
		return ret;
	}

	void *ptr = (char *)val + descr->offset;

	return arr_parse(&arr, descr->array.element_descr,
			 descr->array.n_elements, ptr, val);
}


int json_arr_separate_object_parse_init(struct json_obj *json, char *payload, size_t len)
{
	return arr_init(json, payload, len);
}

int json_arr_separate_parse_object(struct json_obj *json, const struct json_obj_descr *descr,
			  size_t descr_len, void *val)
{
	struct json_token tok;

	if (!lexer_next(&json->lex, &tok)) {
		return -EINVAL;
	}

	if (tok.type == JSON_TOK_ARRAY_END) {
		return 0;
	} else if (tok.type == JSON_TOK_COMMA) {
		if (!lexer_next(&json->lex, &tok)) {
			return -EINVAL;
		}
	}

	if (tok.type != JSON_TOK_OBJECT_START) {
		return -EINVAL;
	}

	return obj_parse(json, descr, descr_len, val);
}

static char escape_as(char chr)
{
	switch (chr) {
	case '"':
		return '"';
	case '\\':
		return '\\';
	case '\b':
		return 'b';
	case '\f':
		return 'f';
	case '\n':
		return 'n';
	case '\r':
		return 'r';
	case '\t':
		return 't';
	}

	return 0;
}

static int json_escape_internal(const char *str,
				json_append_bytes_t append_bytes,
				void *data)
{
	const char *cur;
	int ret = 0;

	for (cur = str; ret == 0 && *cur; cur++) {
		char escaped = escape_as(*cur);

		if (escaped) {
			char bytes[2] = { '\\', escaped };

			ret = append_bytes(bytes, 2, data);
		} else {
			ret = append_bytes(cur, 1, data);
		}
	}

	return ret;
}

size_t json_calc_escaped_len(const char *str, size_t len)
{
	size_t escaped_len = len;
	size_t pos;

	for (pos = 0; pos < len; pos++) {
		if (escape_as(str[pos])) {
			escaped_len++;
		}
	}

	return escaped_len;
}

ssize_t json_escape(char *str, size_t *len, size_t buf_size)
{
	char *next; /* Points after next character to escape. */
	char *dest; /* Points after next place to write escaped character. */
	size_t escaped_len = json_calc_escaped_len(str, *len);

	if (escaped_len == *len) {
		/*
		 * If no escape is necessary, there is nothing to do.
		 */
		return 0;
	}

	if (escaped_len >= buf_size) {
		return -ENOMEM;
	}

	/*
	 * By walking backwards in the buffer from the end positions
	 * of both the original and escaped strings, we avoid using
	 * extra space. Characters in the original string are
	 * overwritten only after they have already been escaped.
	 */
	str[escaped_len] = '\0';
	for (next = &str[*len], dest = &str[escaped_len]; next != str;) {
		char next_c = *(--next);
		char escape = escape_as(next_c);

		if (escape) {
			*(--dest) = escape;
			*(--dest) = '\\';
		} else {
			*(--dest) = next_c;
		}
	}
	*len = escaped_len;

	return 0;
}

static int encode(const struct json_obj_descr *descr, const void *val,
		  json_append_bytes_t append_bytes, void *data);

static int arr_encode(const struct json_obj_descr *elem_descr,
		      const void *field, const void *val,
		      json_append_bytes_t append_bytes, void *data)
{
	ptrdiff_t elem_size;
	/*
	 * NOTE: Since an element descriptor's offset isn't meaningful
	 * (array elements occur at multiple offsets in `val'), we use
	 * its space in elem_descr to store the offset to the field
	 * containing the number of elements.
	 */
	size_t n_elem = *(size_t *)((char *)val + elem_descr->offset);
	size_t i;
	int ret;

	ret = append_bytes("[", 1, data);
	if (ret < 0) {
		return ret;
	}

	/* For nested arrays, skip parent descriptor to get elements */
	if (elem_descr->type == JSON_TOK_ARRAY_START) {
		elem_descr = elem_descr->array.element_descr;
	}

	elem_size = get_elem_size(elem_descr);

	for (i = 0; i < n_elem; i++) {
		/*
		 * Though "field" points at the next element in the
		 * array which we need to encode, the value in
		 * elem_descr->offset is actually the offset of the
		 * length field in the "parent" struct containing the
		 * array.
		 *
		 * To patch things up, we lie to encode() about where
		 * the field is by exactly the amount it will offset
		 * it. This is a size optimization for struct
		 * json_obj_descr: the alternative is to keep a
		 * separate field next to element_descr which is an
		 * offset to the length field in the parent struct,
		 * but that would add a size_t to every descriptor.
		 */
		ret = encode(elem_descr, (char *)field - elem_descr->offset,
			     append_bytes, data);
		if (ret < 0) {
			return ret;
		}

		if (i < n_elem - 1) {
			ret = append_bytes(",", 1, data);
			if (ret < 0) {
				return ret;
			}
		}

		field = (char *)field + elem_size;
	}

	return append_bytes("]", 1, data);
}

static int str_encode(const char **str, json_append_bytes_t append_bytes,
		      void *data)
{
	int ret;

	ret = append_bytes("\"", 1, data);
	if (ret < 0) {
		return ret;
	}

	ret = json_escape_internal(*str, append_bytes, data);
	if (!ret) {
		return append_bytes("\"", 1, data);
	}

	return ret;
}

static int num_encode(const int32_t *num, json_append_bytes_t append_bytes,
		      void *data)
{
	char buf[3 * sizeof(int32_t)];
	int ret;

	ret = snprintk(buf, sizeof(buf), "%d", *num);
	if (ret < 0) {
		return ret;
	}
	if (ret >= (int)sizeof(buf)) {
		return -ENOMEM;
	}

	return append_bytes(buf, (size_t)ret, data);
}

static int float_ascii_encode(struct json_obj_token *num, json_append_bytes_t append_bytes,
		      void *data)
{

	return append_bytes(num->start, num->length, data);
}

static int opaque_string_encode(struct json_obj_token *opaque, json_append_bytes_t append_bytes,
		      void *data)
{
	int ret;

	ret = append_bytes("\"", 1, data);
	if (ret < 0) {
		return ret;
	}

	ret = append_bytes(opaque->start, opaque->length, data);
	if (ret < 0) {
		return ret;
	}

	return append_bytes("\"", 1, data);
}

static int bool_encode(const bool *value, json_append_bytes_t append_bytes,
		       void *data)
{
	if (*value) {
		return append_bytes("true", 4, data);
	}

	return append_bytes("false", 5, data);
}

static int encode(const struct json_obj_descr *descr, const void *val,
		  json_append_bytes_t append_bytes, void *data)
{
	void *ptr = (char *)val + descr->offset;

	switch (descr->type) {
	case JSON_TOK_FALSE:
	case JSON_TOK_TRUE:
		return bool_encode(ptr, append_bytes, data);
	case JSON_TOK_STRING:
		return str_encode(ptr, append_bytes, data);
	case JSON_TOK_ARRAY_START:
		return arr_encode(descr->array.element_descr, ptr,
				  val, append_bytes, data);
	case JSON_TOK_OBJECT_START:
		return json_obj_encode(descr->object.sub_descr,
				       descr->object.sub_descr_len,
				       ptr, append_bytes, data);
	case JSON_TOK_NUMBER:
		return num_encode(ptr, append_bytes, data);
	case JSON_TOK_FLOAT:
		return float_ascii_encode(ptr, append_bytes, data);
	case JSON_TOK_OPAQUE:
		return opaque_string_encode(ptr, append_bytes, data);
	default:
		return -EINVAL;
	}
}

int json_obj_encode(const struct json_obj_descr *descr, size_t descr_len,
		    const void *val, json_append_bytes_t append_bytes,
		    void *data)
{
	size_t i;
	int ret;

	ret = append_bytes("{", 1, data);
	if (ret < 0) {
		return ret;
	}

	for (i = 0; i < descr_len; i++) {
		ret = str_encode((const char **)&descr[i].field_name,
				 append_bytes, data);
		if (ret < 0) {
			return ret;
		}

		ret = append_bytes(":", 1, data);
		if (ret < 0) {
			return ret;
		}

		ret = encode(&descr[i], val, append_bytes, data);
		if (ret < 0) {
			return ret;
		}

		if (i < descr_len - 1) {
			ret = append_bytes(",", 1, data);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return append_bytes("}", 1, data);
}

int json_arr_encode(const struct json_obj_descr *descr, const void *val,
		    json_append_bytes_t append_bytes, void *data)
{
	void *ptr = (char *)val + descr->offset;

	return arr_encode(descr->array.element_descr, ptr, val, append_bytes,
			  data);
}

struct appender {
	char *buffer;
	size_t used;
	size_t size;
};

static int append_bytes_to_buf(const char *bytes, size_t len, void *data)
{
	struct appender *appender = data;

	if (len >= appender->size - appender->used) {
		return -ENOMEM;
	}

	memcpy(appender->buffer + appender->used, bytes, len);
	appender->used += len;
	appender->buffer[appender->used] = '\0';

	return 0;
}

int json_obj_encode_buf(const struct json_obj_descr *descr, size_t descr_len,
			const void *val, char *buffer, size_t buf_size)
{
	struct appender appender = { .buffer = buffer, .size = buf_size };

	return json_obj_encode(descr, descr_len, val, append_bytes_to_buf,
			       &appender);
}

int json_arr_encode_buf(const struct json_obj_descr *descr, const void *val,
			char *buffer, size_t buf_size)
{
	struct appender appender = { .buffer = buffer, .size = buf_size };

	return json_arr_encode(descr, val, append_bytes_to_buf, &appender);
}

static int measure_bytes(const char *bytes, size_t len, void *data)
{
	ssize_t *total = data;

	*total += (ssize_t)len;

	ARG_UNUSED(bytes);

	return 0;
}

ssize_t json_calc_encoded_len(const struct json_obj_descr *descr,
			      size_t descr_len, const void *val)
{
	ssize_t total = 0;
	int ret;

	ret = json_obj_encode(descr, descr_len, val, measure_bytes, &total);
	if (ret < 0) {
		return ret;
	}

	return total;
}

ssize_t json_calc_encoded_arr_len(const struct json_obj_descr *descr,
				  const void *val)
{
	ssize_t total = 0;
	int ret;

	ret = json_arr_encode(descr, val, measure_bytes, &total);
	if (ret < 0) {
		return ret;
	}

	return total;
}
