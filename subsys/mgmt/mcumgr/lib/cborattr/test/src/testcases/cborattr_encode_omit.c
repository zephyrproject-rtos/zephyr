/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
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

    cborattr_test_util_encode(attrs, exp, sizeof exp);
}
