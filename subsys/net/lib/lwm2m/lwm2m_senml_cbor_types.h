/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generated using zcbor version 0.7.0
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

/** Which value for --default-max-qty this file was created with.
 *
 *  The define is used in the other generated file to do a build-time
 *  compatibility check.
 *
 *  See `zcbor --help` for more information about --default-max-qty
 */
#define DEFAULT_MAX_QTY CONFIG_LWM2M_RW_SENML_CBOR_RECORDS

struct record_bn {
	struct zcbor_string _record_bn;
};

struct record_bt {
	int64_t _record_bt;
};

struct record_n {
	struct zcbor_string _record_n;
};

struct record_t {
	int64_t _record_t;
};

/* The union members and enum members have the same names.
 * This is intentional so we need to ignore -Wshadow to avoid
 * compiler complaining about them.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"

struct record_union_ {
	union {
		struct {
			int64_t _union_vi;
		};
		struct {
			double _union_vf;
		};
		struct {
			struct zcbor_string _union_vs;
		};
		struct {
			bool _union_vb;
		};
		struct {
			struct zcbor_string _union_vd;
		};
		struct {
			struct zcbor_string _union_vlo;
		};
	};
	enum {
		_union_vi,
		_union_vf,
		_union_vs,
		_union_vb,
		_union_vd,
		_union_vlo,
	} _record_union_choice;
};

struct value_ {
	union {
		struct zcbor_string _value_tstr;
		struct zcbor_string _value_bstr;
		int64_t _value_int;
		double _value_float;
		bool _value_bool;
	};
	enum {
		_value_tstr,
		_value_bstr,
		_value_int,
		_value_float,
		_value_bool,
	} _value_choice;
};

#pragma GCC diagnostic pop

struct key_value_pair {
	int32_t _key_value_pair_key;
	struct value_ _key_value_pair;
};

struct record__key_value_pair {
	struct key_value_pair _record__key_value_pair;
};

struct record {
	struct record_bn _record_bn;
	bool _record_bn_present;
	struct record_bt _record_bt;
	bool _record_bt_present;
	struct record_n _record_n;
	bool _record_n_present;
	struct record_t _record_t;
	bool _record_t_present;
	struct record_union_ _record_union;
	bool _record_union_present;
	struct record__key_value_pair _record__key_value_pair[5];
	size_t _record__key_value_pair_count;
};

struct lwm2m_senml {
	struct record _lwm2m_senml__record[DEFAULT_MAX_QTY];
	size_t _lwm2m_senml__record_count;
};

#ifdef __cplusplus
}
#endif

#endif /* LWM2M_SENML_CBOR_TYPES_H__ */
