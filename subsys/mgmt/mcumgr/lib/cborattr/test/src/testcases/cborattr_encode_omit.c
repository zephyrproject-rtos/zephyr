/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_cborattr.h"

TEST_CASE_SELF(test_cborattr_encode_omit)
{
	/* Omit everything except the "str" value. */
	const struct cbor_out_attr_t attrs[11] = {
		{
			.attribute = "null",
			.val = {
				.type = CborAttrNullType,
			},
			.omit = true,
		},
		{
			.attribute = "bool",
			.val = {
				.type = CborAttrBooleanType,
				.boolean = true,
			},
			.omit = true,
		},
		{
			.attribute = "int",
			.val = {
				.type = CborAttrIntegerType,
				.integer = -99,
			},
			.omit = true,
		},
		{
			.attribute = "uint",
			.val = {
				.type = CborAttrUnsignedIntegerType,
				.integer = 8442,
			},
			.omit = true,
		},
		{
			.attribute = "float",
			.val = {
				.type = CborAttrFloatType,
				.fval = 8.0,
			},
			.omit = true,
		},
		{
			.attribute = "double",
			.val = {
				.type = CborAttrDoubleType,
				.real = 16.0,
			},
			.omit = true,
		},
		{
			.attribute = "bytes",
			.val = {
				.type = CborAttrByteStringType,
				.bytestring.data = (uint8_t[]) {1, 2, 3},
				.bytestring.len = 3,
			},
			.omit = true,
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
			.omit = true,
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
			.omit = true,
		},
		{ 0 }
	};
	const uint8_t exp[] = {
		0xbf, 0x63, 0x73, 0x74, 0x72, 0x65, 0x6d, 0x79,
		0x73, 0x74, 0x72, 0xff,
	};

	cborattr_test_util_encode(attrs, exp, sizeof(exp));
}
