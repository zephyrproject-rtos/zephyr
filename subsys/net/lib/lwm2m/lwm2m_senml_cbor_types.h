/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generated using zcbor version 0.8.1
 * https://github.com/zephyrproject-rtos/zcbor
 * Generated with a --default-max-qty of 99
 */

#ifndef LWM2M_SENML_CBOR_TYPES_H__
#define LWM2M_SENML_CBOR_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

enum lwm2m_senml_cbor_key {
	lwm2m_senml_cbor_key_bn = -2,
	lwm2m_senml_cbor_key_bt = -3,
	lwm2m_senml_cbor_key_n = 0,
	lwm2m_senml_cbor_key_t = 6,
	lwm2m_senml_cbor_key_vi = 2,
	lwm2m_senml_cbor_key_vf = 2,
	lwm2m_senml_cbor_key_vs = 3,
	lwm2m_senml_cbor_key_vb = 4,
	lwm2m_senml_cbor_key_vd = 8,
};

struct record_bn {
	struct zcbor_string record_bn;
};

struct record_bt {
	int64_t record_bt;
};

struct record_n {
	struct zcbor_string record_n;
};

struct record_t {
	int64_t record_t;
};

struct record_union_r {
	union {
		struct {
			int64_t union_vi;
		};
		struct {
			double union_vf;
		};
		struct {
			struct zcbor_string union_vs;
		};
		struct {
			bool union_vb;
		};
		struct {
			struct zcbor_string union_vd;
		};
		struct {
			struct zcbor_string union_vlo;
		};
	};
	enum {
		union_vi_c,
		union_vf_c,
		union_vs_c,
		union_vb_c,
		union_vd_c,
		union_vlo_c,
	} record_union_choice;
};

struct value_r {
	union {
		struct zcbor_string value_tstr;
		struct zcbor_string value_bstr;
		int64_t value_int;
		double value_float;
		bool value_bool;
	};
	enum {
		value_tstr_c,
		value_bstr_c,
		value_int_c,
		value_float_c,
		value_bool_c,
	} value_choice;
};

struct key_value_pair {
	int32_t key_value_pair_key;
	struct value_r key_value_pair;
};

struct record_key_value_pair_m {
	struct key_value_pair record_key_value_pair_m;
};

struct record {
	struct record_bn record_bn;
	bool record_bn_present;
	struct record_bt record_bt;
	bool record_bt_present;
	struct record_n record_n;
	bool record_n_present;
	struct record_t record_t;
	bool record_t_present;
	struct record_union_r record_union;
	bool record_union_present;
	struct record_key_value_pair_m record_key_value_pair_m[5];
	size_t record_key_value_pair_m_count;
};

struct lwm2m_senml {
	struct record lwm2m_senml_record_m[CONFIG_LWM2M_RW_SENML_CBOR_RECORDS];
	size_t lwm2m_senml_record_m_count;
};

#ifdef __cplusplus
}
#endif

#endif /* LWM2M_SENML_CBOR_TYPES_H__ */
