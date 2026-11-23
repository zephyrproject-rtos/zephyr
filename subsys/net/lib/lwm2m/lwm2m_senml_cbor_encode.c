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
#include "zcbor_encode.h"
#include "lwm2m_senml_cbor_encode.h"
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

static bool encode_repeated_record_bn(zcbor_state_t *state, const struct record_bn *input);
static bool encode_repeated_record_bt(zcbor_state_t *state, const struct record_bt *input);
static bool encode_repeated_record_n(zcbor_state_t *state, const struct record_n *input);
static bool encode_repeated_record_t(zcbor_state_t *state, const struct record_t *input);
static bool encode_repeated_record_union(zcbor_state_t *state, const struct record_union_r *input);
static bool encode_value(zcbor_state_t *state, const struct value_r *input);
static bool encode_key_value_pair(zcbor_state_t *state, const struct key_value_pair *input);
static bool encode_repeated_record_key_value_pair_m(zcbor_state_t *state,
						    const struct record_key_value_pair_m *input);
static bool encode_record(zcbor_state_t *state, const struct record *input);
static bool encode_lwm2m_senml(zcbor_state_t *state, const struct lwm2m_senml *input);

static bool encode_repeated_record_bn(zcbor_state_t *state, const struct record_bn *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_int32_put(state, (-2)))) &&
		     (zcbor_tstr_encode(state, (&(*input).record_bn)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_record_bt(zcbor_state_t *state, const struct record_bt *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_int32_put(state, (-3)))) &&
		     (zcbor_int64_encode(state, (&(*input).record_bt)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_record_n(zcbor_state_t *state, const struct record_n *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_uint32_put(state, (0)))) &&
		     (zcbor_tstr_encode(state, (&(*input).record_n)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_record_t(zcbor_state_t *state, const struct record_t *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_uint32_put(state, (6)))) &&
		     (zcbor_int64_encode(state, (&(*input).record_t)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_record_union(zcbor_state_t *state, const struct record_union_r *input)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;

	bool res = (((
		((*input).record_union_choice == union_vi_c)
			? (((zcbor_uint32_put(state, (2)))) &&
			   (zcbor_int64_encode(state, (&(*input).union_vi))))
			: (((*input).record_union_choice == union_vf_c)
				   ? (((zcbor_uint32_put(state, (2)))) &&
				      (zcbor_float64_encode(state, (&(*input).union_vf))))
				   : (((*input).record_union_choice == union_vs_c)
				      ? (((zcbor_uint32_put(state, (3)))) &&
					 (zcbor_tstr_encode(state, (&(*input).union_vs))))
				      : (((*input).record_union_choice == union_vb_c)
						 ? (((zcbor_uint32_put(state, (4)))) &&
						    (zcbor_bool_encode(
							    state, (&(*input).union_vb))))
						 : (((*input).record_union_choice ==
						     union_vd_c)
							    ? (((zcbor_uint32_put(state,
										  (8)))) &&
							       (zcbor_bstr_encode(
								       state,
								       (&(*input).union_vd))))
							    : (((*input).record_union_choice ==
								union_vlo_c)
								       ? (((zcbor_tstr_encode(
								  state,
								  ((tmp_str.value =
									(uint8_t *)"vlo",
								    tmp_str.len =
									sizeof("vlo") -
									    1,
								    &tmp_str))))) &&
								  (zcbor_tstr_encode(
									  state,
									  (&(*input).union_vlo))))
								       : false))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_value(zcbor_state_t *state, const struct value_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((
		((*input).value_choice == value_tstr_c)
			? ((zcbor_tstr_encode(state, (&(*input).value_tstr))))
			: (((*input).value_choice == value_bstr_c)
				   ? ((zcbor_bstr_encode(state, (&(*input).value_bstr))))
				   : (((*input).value_choice == value_int_c)
				      ? ((zcbor_int64_encode(state, (&(*input).value_int))))
				      : (((*input).value_choice == value_float_c)
						 ? ((zcbor_float64_encode(
						   state, (&(*input).value_float))))
						 : (((*input).value_choice == value_bool_c)
							    ? ((zcbor_bool_encode(
							      state,
							      (&(*input).value_bool))))
							    : false)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_key_value_pair(zcbor_state_t *state, const struct key_value_pair *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((((zcbor_int32_encode(state, (&(*input).key_value_pair_key)))) &&
		     (encode_value(state, (&(*input).key_value_pair)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_repeated_record_key_value_pair_m(zcbor_state_t *state,
						    const struct record_key_value_pair_m *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((encode_key_value_pair(state, (&(*input).record_key_value_pair_m)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_record(zcbor_state_t *state, const struct record *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((
		(zcbor_map_start_encode(state, ZCBOR_ARRAY_SIZE(input->record_key_value_pair_m)) &&
		 (((!(*input).record_bn_present ||
		    encode_repeated_record_bn(state, (&(*input).record_bn))) &&
		   (!(*input).record_bt_present ||
		    encode_repeated_record_bt(state, (&(*input).record_bt))) &&
		   (!(*input).record_n_present ||
		    encode_repeated_record_n(state, (&(*input).record_n))) &&
		   (!(*input).record_t_present ||
		    encode_repeated_record_t(state, (&(*input).record_t))) &&
		   (!(*input).record_union_present ||
		    encode_repeated_record_union(state, (&(*input).record_union))) &&
		   zcbor_multi_encode_minmax(
			   0, ZCBOR_ARRAY_SIZE(input->record_key_value_pair_m),
			   &(*input).record_key_value_pair_m_count,
			   (zcbor_encoder_t *)encode_repeated_record_key_value_pair_m, state,
			   (*&(*input).record_key_value_pair_m),
			   sizeof(struct record_key_value_pair_m))) ||
		  (zcbor_list_map_end_force_encode(state), false)) &&
		 zcbor_map_end_encode(state, ZCBOR_ARRAY_SIZE(input->record_key_value_pair_m)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_lwm2m_senml(zcbor_state_t *state, const struct lwm2m_senml *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res =
		(((zcbor_list_start_encode(state, ZCBOR_ARRAY_SIZE(input->lwm2m_senml_record_m)) &&
		   ((zcbor_multi_encode_minmax(
			    1, ZCBOR_ARRAY_SIZE(input->lwm2m_senml_record_m),
			    &(*input).lwm2m_senml_record_m_count, (zcbor_encoder_t *)encode_record,
			    state, (*&(*input).lwm2m_senml_record_m), sizeof(struct record))) ||
		    (zcbor_list_map_end_force_encode(state), false)) &&
		   zcbor_list_end_encode(state, ZCBOR_ARRAY_SIZE(input->lwm2m_senml_record_m)))));

	log_result(state, res, __func__);
	return res;
}

int cbor_encode_lwm2m_senml(uint8_t *payload, size_t payload_len, const struct lwm2m_senml *input,
			    size_t *payload_len_out)
{
	zcbor_state_t states[5];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
				    (zcbor_decoder_t *)encode_lwm2m_senml,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
