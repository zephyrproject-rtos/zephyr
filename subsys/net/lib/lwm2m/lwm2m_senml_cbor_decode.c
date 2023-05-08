/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generated using zcbor version 0.7.0
 * https://github.com/zephyrproject-rtos/zcbor
 * Generated with a --default-max-qty of 99
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"
#include "lwm2m_senml_cbor_decode.h"

static bool decode_repeated_record_bn(zcbor_state_t *state, struct record_bn *result);
static bool decode_repeated_record_bt(zcbor_state_t *state, struct record_bt *result);
static bool decode_repeated_record_n(zcbor_state_t *state, struct record_n *result);
static bool decode_repeated_record_t(zcbor_state_t *state, struct record_t *result);
static bool decode_repeated_record_union(zcbor_state_t *state, struct record_union_ *result);
static bool decode_value(zcbor_state_t *state, struct value_ *result);
static bool decode_key_value_pair(zcbor_state_t *state, struct key_value_pair *result);
static bool decode_repeated_record__key_value_pair(zcbor_state_t *state,
						   struct record__key_value_pair *result);
static bool decode_record(zcbor_state_t *state, struct record *result);
static bool decode_lwm2m_senml(zcbor_state_t *state, struct lwm2m_senml *result);

static bool decode_repeated_record_bn(zcbor_state_t *state, struct record_bn *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_int32_expect(state, (-2)))) &&
			    (zcbor_tstr_decode(state, (&(*result)._record_bn)))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool decode_repeated_record_bt(zcbor_state_t *state, struct record_bt *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result =
		((((zcbor_int32_expect(state, (-3)))) &&
		  (zcbor_int64_decode(state, (&(*result)._record_bt))) &&
		  ((((*result)._record_bt >= INT64_MIN) && ((*result)._record_bt <= INT64_MAX)) ||
		   (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool decode_repeated_record_n(zcbor_state_t *state, struct record_n *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_uint32_expect(state, (0)))) &&
			    (zcbor_tstr_decode(state, (&(*result)._record_n)))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool decode_repeated_record_t(zcbor_state_t *state, struct record_t *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result =
		((((zcbor_uint32_expect(state, (6)))) &&
		  (zcbor_int64_decode(state, (&(*result)._record_t))) &&
		  ((((*result)._record_t >= INT64_MIN) && ((*result)._record_t <= INT64_MAX)) ||
		   (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool decode_repeated_record_union(zcbor_state_t *state, struct record_union_ *result)
{
	zcbor_print("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool tmp_result =
		(((zcbor_union_start_code(state) &&
		   (int_res = (((((zcbor_uint32_expect_union(state, (2)))) &&
				 (zcbor_int64_decode(state, (&(*result)._union_vi))) &&
				 ((((*result)._union_vi >= INT64_MIN) &&
				   ((*result)._union_vi <= INT64_MAX)) ||
				  (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false))) &&
				(((*result)._record_union_choice = _union_vi), true)) ||
			       ((((zcbor_uint32_expect_union(state, (2)))) &&
				 (zcbor_float_decode(state, (&(*result)._union_vf)))) &&
				(((*result)._record_union_choice = _union_vf), true)) ||
			       ((((zcbor_uint32_expect_union(state, (3)))) &&
				 (zcbor_tstr_decode(state, (&(*result)._union_vs)))) &&
				(((*result)._record_union_choice = _union_vs), true)) ||
			       ((((zcbor_uint32_expect_union(state, (4)))) &&
				 (zcbor_bool_decode(state, (&(*result)._union_vb)))) &&
				(((*result)._record_union_choice = _union_vb), true)) ||
			       ((((zcbor_uint32_expect_union(state, (8)))) &&
				 (zcbor_bstr_decode(state, (&(*result)._union_vd)))) &&
				(((*result)._record_union_choice = _union_vd), true)) ||
			       (zcbor_union_elem_code(state) &&
				((((zcbor_tstr_expect(
					  state, ((tmp_str.value = (uint8_t *)"vlo",
						   tmp_str.len = sizeof("vlo") - 1, &tmp_str))))) &&
				  (zcbor_tstr_decode(state, (&(*result)._union_vlo)))) &&
				 (((*result)._record_union_choice = _union_vlo), true)))),
		    zcbor_union_end_code(state), int_res))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool decode_value(zcbor_state_t *state, struct value_ *result)
{
	zcbor_print("%s\r\n", __func__);
	bool int_res;

	bool tmp_result =
		(((zcbor_union_start_code(state) &&
		   (int_res = ((((zcbor_tstr_decode(state, (&(*result)._value_tstr)))) &&
				(((*result)._value_choice = _value_tstr), true)) ||
			       (((zcbor_bstr_decode(state, (&(*result)._value_bstr)))) &&
				(((*result)._value_choice = _value_bstr), true)) ||
			       (((zcbor_int64_decode(state, (&(*result)._value_int))) &&
				 ((((*result)._value_int >= INT64_MIN) &&
				   ((*result)._value_int <= INT64_MAX)) ||
				  (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false))) &&
				(((*result)._value_choice = _value_int), true)) ||
			       (((zcbor_float_decode(state, (&(*result)._value_float)))) &&
				(((*result)._value_choice = _value_float), true)) ||
			       (((zcbor_bool_decode(state, (&(*result)._value_bool)))) &&
				(((*result)._value_choice = _value_bool), true))),
		    zcbor_union_end_code(state), int_res))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool decode_key_value_pair(zcbor_state_t *state, struct key_value_pair *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_int32_decode(state, (&(*result)._key_value_pair_key)))) &&
			    (decode_value(state, (&(*result)._key_value_pair)))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool decode_repeated_record__key_value_pair(zcbor_state_t *state,
						   struct record__key_value_pair *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = (((decode_key_value_pair(state, (&(*result)._record__key_value_pair)))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool decode_record(zcbor_state_t *state, struct record *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result =
		(((zcbor_map_start_decode(state) &&
		   ((zcbor_present_decode(&((*result)._record_bn_present),
					  (zcbor_decoder_t *)decode_repeated_record_bn, state,
					  (&(*result)._record_bn)) &&
		     zcbor_present_decode(&((*result)._record_bt_present),
					  (zcbor_decoder_t *)decode_repeated_record_bt, state,
					  (&(*result)._record_bt)) &&
		     zcbor_present_decode(&((*result)._record_n_present),
					  (zcbor_decoder_t *)decode_repeated_record_n, state,
					  (&(*result)._record_n)) &&
		     zcbor_present_decode(&((*result)._record_t_present),
					  (zcbor_decoder_t *)decode_repeated_record_t, state,
					  (&(*result)._record_t)) &&
		     zcbor_present_decode(&((*result)._record_union_present),
					  (zcbor_decoder_t *)decode_repeated_record_union, state,
					  (&(*result)._record_union)) &&
		     zcbor_multi_decode(0, ZCBOR_ARRAY_SIZE(result->_record__key_value_pair),
					&(*result)._record__key_value_pair_count,
					(zcbor_decoder_t *)decode_repeated_record__key_value_pair,
					state, (&(*result)._record__key_value_pair),
					sizeof(struct record__key_value_pair))) ||
		    (zcbor_list_map_end_force_decode(state), false)) &&
		   zcbor_map_end_decode(state))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool decode_lwm2m_senml(zcbor_state_t *state, struct lwm2m_senml *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result =
		(((zcbor_list_start_decode(state) &&
		   ((zcbor_multi_decode(
			    1, ZCBOR_ARRAY_SIZE(result->_lwm2m_senml__record),
			    &(*result)._lwm2m_senml__record_count, (zcbor_decoder_t *)decode_record,
			    state, (&(*result)._lwm2m_senml__record), sizeof(struct record))) ||
		    (zcbor_list_map_end_force_decode(state), false)) &&
		   zcbor_list_end_decode(state))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

int cbor_decode_lwm2m_senml(const uint8_t *payload, size_t payload_len, struct lwm2m_senml *result,
			    size_t *payload_len_out)
{
	zcbor_state_t states[5];

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), payload, payload_len, 1);

	bool ret = decode_lwm2m_senml(states, result);

	if (ret && (payload_len_out != NULL)) {
		*payload_len_out = MIN(payload_len, (size_t)states[0].payload - (size_t)payload);
	}

	if (!ret) {
		int err = zcbor_pop_error(states);

		zcbor_print("Return error: %d\r\n", err);
		return (err == ZCBOR_SUCCESS) ? ZCBOR_ERR_UNKNOWN : err;
	}
	return ZCBOR_SUCCESS;
}
