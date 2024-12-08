/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generated using zcbor version 0.9.0
 * https://github.com/zephyrproject-rtos/zcbor
 * Generated with a --default-max-qty of 99
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"
#include "lwm2m_senml_cbor_decode.h"
#include "zcbor_print.h"

#define log_result(state, result, func)                                                            \
	do {                                                                                       \
		if (!result) {                                                                     \
			zcbor_trace_file(state);                                                   \
			zcbor_log("%s error: %s\r\n", func,                                        \
				  zcbor_error_str(zcbor_peek_error(state)));                       \
		} else {                                                                           \
			zcbor_log("%s success\r\n", func);                                         \
		}                                                                                  \
	} while (0)

static bool decode_repeated_record_bn(zcbor_state_t *state, struct record_bn *result);
static bool decode_repeated_record_bt(zcbor_state_t *state, struct record_bt *result);
static bool decode_repeated_record_n(zcbor_state_t *state, struct record_n *result);
static bool decode_repeated_record_t(zcbor_state_t *state, struct record_t *result);
static bool decode_repeated_record_union(zcbor_state_t *state, struct record_union_r *result);
static bool decode_value(zcbor_state_t *state, struct value_r *result);
static bool decode_key_value_pair(zcbor_state_t *state, struct key_value_pair *result);
static bool decode_repeated_record_key_value_pair_m(zcbor_state_t *state,
						    struct record_key_value_pair_m *result);
static bool decode_record(zcbor_state_t *state, struct record *result);
static bool decode_lwm2m_senml(zcbor_state_t *state, struct lwm2m_senml *result);

static bool decode_repeated_record_bn(zcbor_state_t *state, struct record_bn *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_int32_expect(state, (-2)))) &&
		     (zcbor_tstr_decode(state, (&(*result).record_bn)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_record_bt(zcbor_state_t *state, struct record_bt *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_int32_expect(state, (-3)))) &&
		     (zcbor_int64_decode(state, (&(*result).record_bt)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_record_n(zcbor_state_t *state, struct record_n *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_uint32_expect(state, (0)))) &&
		     (zcbor_tstr_decode(state, (&(*result).record_n)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_record_t(zcbor_state_t *state, struct record_t *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_uint32_expect(state, (6)))) &&
		     (zcbor_int64_decode(state, (&(*result).record_t)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_record_union(zcbor_state_t *state, struct record_union_r *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool res = (((zcbor_union_start_code(state) &&
		      (int_res = (((((zcbor_uint32_expect_union(state, (2)))) &&
				    (zcbor_int64_decode(state, (&(*result).union_vi)))) &&
				   (((*result).record_union_choice = union_vi_c), true)) ||
				  ((((zcbor_uint32_expect_union(state, (2)))) &&
				    (zcbor_float_decode(state, (&(*result).union_vf)))) &&
				   (((*result).record_union_choice = union_vf_c), true)) ||
				  ((((zcbor_uint32_expect_union(state, (3)))) &&
				    (zcbor_tstr_decode(state, (&(*result).union_vs)))) &&
				   (((*result).record_union_choice = union_vs_c), true)) ||
				  ((((zcbor_uint32_expect_union(state, (4)))) &&
				    (zcbor_bool_decode(state, (&(*result).union_vb)))) &&
				   (((*result).record_union_choice = union_vb_c), true)) ||
				  ((((zcbor_uint32_expect_union(state, (8)))) &&
				    (zcbor_bstr_decode(state, (&(*result).union_vd)))) &&
				   (((*result).record_union_choice = union_vd_c), true)) ||
				  (zcbor_union_elem_code(state) &&
				   ((((zcbor_tstr_expect(state, ((tmp_str.value = (uint8_t *)"vlo",
								  tmp_str.len = sizeof("vlo") - 1,
								  &tmp_str))))) &&
				     (zcbor_tstr_decode(state, (&(*result).union_vlo)))) &&
				    (((*result).record_union_choice = union_vlo_c), true)))),
		       zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_value(zcbor_state_t *state, struct value_r *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_union_start_code(state) &&
		      (int_res = ((((zcbor_tstr_decode(state, (&(*result).value_tstr)))) &&
				   (((*result).value_choice = value_tstr_c), true)) ||
				  (((zcbor_bstr_decode(state, (&(*result).value_bstr)))) &&
				   (((*result).value_choice = value_bstr_c), true)) ||
				  (((zcbor_int64_decode(state, (&(*result).value_int)))) &&
				   (((*result).value_choice = value_int_c), true)) ||
				  (zcbor_union_elem_code(state) &&
				   (((zcbor_float_decode(state, (&(*result).value_float)))) &&
				    (((*result).value_choice = value_float_c), true))) ||
				  (((zcbor_bool_decode(state, (&(*result).value_bool)))) &&
				   (((*result).value_choice = value_bool_c), true))),
		       zcbor_union_end_code(state), int_res))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_key_value_pair(zcbor_state_t *state, struct key_value_pair *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_int32_decode(state, (&(*result).key_value_pair_key)))) &&
		     (decode_value(state, (&(*result).key_value_pair)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_repeated_record_key_value_pair_m(zcbor_state_t *state,
						    struct record_key_value_pair_m *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((decode_key_value_pair(state, (&(*result).record_key_value_pair_m)))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_record(zcbor_state_t *state, struct record *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res =
		(((zcbor_map_start_decode(state) &&
		   ((zcbor_present_decode(&((*result).record_bn_present),
					  (zcbor_decoder_t *)decode_repeated_record_bn, state,
					  (&(*result).record_bn)) &&
		     zcbor_present_decode(&((*result).record_bt_present),
					  (zcbor_decoder_t *)decode_repeated_record_bt, state,
					  (&(*result).record_bt)) &&
		     zcbor_present_decode(&((*result).record_n_present),
					  (zcbor_decoder_t *)decode_repeated_record_n, state,
					  (&(*result).record_n)) &&
		     zcbor_present_decode(&((*result).record_t_present),
					  (zcbor_decoder_t *)decode_repeated_record_t, state,
					  (&(*result).record_t)) &&
		     zcbor_present_decode(&((*result).record_union_present),
					  (zcbor_decoder_t *)decode_repeated_record_union, state,
					  (&(*result).record_union)) &&
		     zcbor_multi_decode(0, ZCBOR_ARRAY_SIZE(result->record_key_value_pair_m),
					&(*result).record_key_value_pair_m_count,
					(zcbor_decoder_t *)decode_repeated_record_key_value_pair_m,
					state, (*&(*result).record_key_value_pair_m),
					sizeof(struct record_key_value_pair_m))) ||
		    (zcbor_list_map_end_force_decode(state), false)) &&
		   zcbor_map_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_repeated_record_key_value_pair_m(state,
							(*&(*result).record_key_value_pair_m));
		decode_repeated_record_bn(state, (&(*result).record_bn));
		decode_repeated_record_bt(state, (&(*result).record_bt));
		decode_repeated_record_n(state, (&(*result).record_n));
		decode_repeated_record_t(state, (&(*result).record_t));
		decode_repeated_record_union(state, (&(*result).record_union));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_lwm2m_senml(zcbor_state_t *state, struct lwm2m_senml *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res =
		(((zcbor_list_start_decode(state) &&
		   ((zcbor_multi_decode(
			    1, ZCBOR_ARRAY_SIZE(result->lwm2m_senml_record_m),
			    &(*result).lwm2m_senml_record_m_count, (zcbor_decoder_t *)decode_record,
			    state, (*&(*result).lwm2m_senml_record_m), sizeof(struct record))) ||
		    (zcbor_list_map_end_force_decode(state), false)) &&
		   zcbor_list_end_decode(state))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		decode_record(state, (*&(*result).lwm2m_senml_record_m));
	}

	log_result(state, res, __func__);
	return res;
}

int cbor_decode_lwm2m_senml(const uint8_t *payload, size_t payload_len, struct lwm2m_senml *result,
			    size_t *payload_len_out)
{
	zcbor_state_t states[5];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_lwm2m_senml,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
