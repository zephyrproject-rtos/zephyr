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


#ifndef CBORATTR_H
#define CBORATTR_H

#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <tinycbor/cbor.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This library wraps the tinycbor decoder with a attribute based decoder
 * suitable for decoding a binary version of json.  Specifically, the
 * contents of the cbor contains pairs of attributes.  where the attribute
 * is a key/value pair.  keys are always text strings, but values can be
 * many different things (enumerated below) */

typedef enum CborAttrType {
    CborAttrIntegerType = 1,
    CborAttrUnsignedIntegerType,
    CborAttrByteStringType,
    CborAttrTextStringType,
    CborAttrBooleanType,
    CborAttrFloatType,
    CborAttrDoubleType,
    CborAttrArrayType,
    CborAttrObjectType,
    CborAttrStructObjectType,
    CborAttrNullType,
} CborAttrType;

struct cbor_attr_t;

struct cbor_enum_t {
    char *name;
    long long int value;
};

struct cbor_array_t {
    CborAttrType element_type;
    union {
        struct {
            const struct cbor_attr_t *subtype;
            char *base;
            size_t stride;
        } objects;
        struct {
            char **ptrs;
            char *store;
            int storelen;
        } strings;
        struct {
            long long int *store;
        } integers;
        struct {
            long long unsigned int *store;
        } uintegers;
        struct {
            double *store;
        } reals;
        struct {
            bool *store;
        } booleans;
    } arr;
    int *count;
    int maxlen;
};

struct cbor_attr_t {
    char *attribute;
    CborAttrType type;
    union {
        long long int *integer;
        long long unsigned int *uinteger;
        double *real;
        float *fval;
        char *string;
        bool *boolean;
        struct byte_string {
            uint8_t *data;
            size_t *len;
        } bytestring;
        struct cbor_array_t array;
        size_t offset;
        struct cbor_attr_t *obj;
    } addr;
    union {
        long long int integer;
        double real;
        bool boolean;
        float fval;
    } dflt;
    size_t len;
    bool nodefault;
};

/*
 * Use the following macros to declare template initializers for
 * CborAttrStructObjectType arrays. Writing the equivalents out by hand is
 * error-prone.
 *
 * CBOR_STRUCT_OBJECT takes a structure name s, and a fieldname f in s.
 *
 * CBOR_STRUCT_ARRAY takes the name of a structure array, a pointer to a an
 * initializer defining the subobject type, and the address of an integer to
 * store the length in.
 */
#define CBORATTR_STRUCT_OBJECT(s, f)        .addr.offset = offsetof(s, f)
#define CBORATTR_STRUCT_ARRAY(a, e, n)                                  \
    .addr.array.element_type = CborAttrStructObjectType,                \
    .addr.array.arr.objects.subtype = e,                                \
    .addr.array.arr.objects.base = (char*)a,                            \
    .addr.array.arr.objects.stride = sizeof(a[0]),                      \
    .addr.array.count = n,                                              \
    .addr.array.maxlen = (int)(sizeof(a)/sizeof(a[0]))

#define CBORATTR_ATTR_UNNAMED (char *)(-1)

int cbor_read_object(struct CborValue *, const struct cbor_attr_t *);
int cbor_read_array(struct CborValue *, const struct cbor_array_t *);

int cbor_read_flat_attrs(const uint8_t *data, int len,
                         const struct cbor_attr_t *attrs);

#ifdef __cplusplus
}
#endif

#endif /* CBORATTR_H */

