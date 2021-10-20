/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_cborattr.h"

TEST_CASE_SELF(test_cborattr_encode_simple)
{
	const struct cbor_out_attr_t attrs[] = {
		{
			.attribute = "null",
			.val = {
				.type = CborAttrNullType,
			},
		},
		{
			.attribute = "bool",
			.val = {
				.type = CborAttrBooleanType,
				.boolean = true,
			},
		},
		{
			.attribute = "int",
			.val = {
				.type = CborAttrIntegerType,
				.integer = -99,
			},
		},
		{
			.attribute = "uint",
			.val = {
				.type = CborAttrUnsignedIntegerType,
				.integer = 8442,
			},
		},
		{
			.attribute = "float",
			.val = {
				.type = CborAttrFloatType,
				.fval = 8.0,
			},
		},
		{
			.attribute = "double",
			.val = {
				.type = CborAttrDoubleType,
				.real = 16.0,
			},
		},
		{
			.attribute = "bytes",
			.val = {
				.type = CborAttrByteStringType,
				.bytestring.data = (uint8_t[]) {1, 2, 3},
				.bytestring.len = 3,
			},
		},
		{
			.attribute = "str",
			.val = {
				.type = CborAttrTextStringType,
				.string = "mystr",
			},
		},
		{
			.attribute = "arr",
			.val = {
				.type = CborAttrArrayType,
				.array = {
					.elems = (struct cbor_out_val_t[]) {
						[0] = {
							.type = CborAttrUnsignedIntegerType,
							.integer = 4355,
						},
						[1] = {
							.type = CborAttrBooleanType,
							.integer = false,
						},
					},
					.len = 2,
				},
			},
		},
		{
			.attribute = "obj",
			.val = {
				.type = CborAttrObjectType,
				.obj = (struct cbor_out_attr_t[]) {
					{
						.attribute = "inner_str",
						.val = {
							.type = CborAttrTextStringType,
							.string = "mystr2",
						},
					},
					{
						.attribute = "inner_int",
						.val = {
							.type = CborAttrIntegerType,
							.integer = 123,
						},
					},
					{ 0 }
				},
			},
		},
		{ 0 }
	};
	const uint8_t exp[] = {
		0xbf, 0x64, 0x6e, 0x75, 0x6c, 0x6c, 0xf6, 0x64,
		0x62, 0x6f, 0x6f, 0x6c, 0xf5, 0x63, 0x69, 0x6e,
		0x74, 0x38, 0x62, 0x64, 0x75, 0x69, 0x6e, 0x74,
		0x19, 0x20, 0xfa, 0x65, 0x66, 0x6c, 0x6f, 0x61,
		0x74, 0xfa, 0x41, 0x00, 0x00, 0x00, 0x66, 0x64,
		0x6f, 0x75, 0x62, 0x6c, 0x65, 0xfb, 0x40, 0x30,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0x62,
		0x79, 0x74, 0x65, 0x73, 0x43, 0x01, 0x02, 0x03,
		0x63, 0x73, 0x74, 0x72, 0x65, 0x6d, 0x79, 0x73,
		0x74, 0x72, 0x63, 0x61, 0x72, 0x72, 0x82, 0x19,
		0x11, 0x03, 0xf4, 0x63, 0x6f, 0x62, 0x6a, 0xbf,
		0x69, 0x69, 0x6e, 0x6e, 0x65, 0x72, 0x5f, 0x73,
		0x74, 0x72, 0x66, 0x6d, 0x79, 0x73, 0x74, 0x72,
		0x32, 0x69, 0x69, 0x6e, 0x6e, 0x65, 0x72, 0x5f,
		0x69, 0x6e, 0x74, 0x18, 0x7b, 0xff, 0xff,
	};

	cborattr_test_util_encode(attrs, exp, sizeof(exp));
}
