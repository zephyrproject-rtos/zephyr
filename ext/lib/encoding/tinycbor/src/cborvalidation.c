/****************************************************************************
**
** Copyright (C) 2017 Intel Corporation
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
**
****************************************************************************/

#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif
#ifndef __STDC_LIMIT_MACROS
#  define __STDC_LIMIT_MACROS 1
#endif

#include "cbor.h"
#include "cborinternal_p.h"
#include "compilersupport_p.h"
#include "utf8_p.h"

#include <string.h>

#include <float.h>
#ifndef CBOR_NO_FLOATING_POINT
#include <math.h>
#endif


#ifndef CBOR_PARSER_MAX_RECURSIONS
#  define CBOR_PARSER_MAX_RECURSIONS 1024
#endif

/**
 * \addtogroup CborParsing
 * @{
 */

/**
 * \enum CborValidationFlags
 * The CborValidationFlags enum contains flags that control the validation of a
 * CBOR stream.
 *
 * \value CborValidateBasic         Validates only the syntax correctedness of the stream.
 * \value CborValidateCanonical     Validates that the stream is in canonical format, according to
 *                                  RFC 7049 section 3.9.
 * \value CborValidateStrictMode    Performs strict validation, according to RFC 7049 section 3.10.
 * \value CborValidateStrictest     Attempt to perform the strictest validation we know of.
 *
 * \value CborValidateShortestIntegrals     (Canonical) Validate that integral numbers and lengths are
 *                                          enconded in their shortest form possible.
 * \value CborValidateShortestFloatingPoint (Canonical) Validate that floating-point numbers are encoded
 *                                          in their shortest form possible.
 * \value CborValidateShortestNumbers       (Canonical) Validate both integrals and floating-point numbers
 *                                          are in their shortest form possible.
 * \value CborValidateNoIndeterminateLength (Canonical) Validate that no string, array or map uses
 *                                          indeterminate length encoding.
 * \value CborValidateMapIsSorted           (Canonical & Strict mode) Validate that map keys appear in
 *                                          sorted order.
 * \value CborValidateMapKeysAreUnique      (Strict mode) Validate that map keys are unique.
 * \value CborValidateTagUse                (Strict mode) Validate that known tags are used with the
 *                                          correct types. This does not validate that the content of
 *                                          those types is syntactically correct.
 * \value CborValidateUtf8                  (Strict mode) Validate that text strings are appropriately
 *                                          encoded in UTF-8.
 * \value CborValidateMapKeysAreString      Validate that all map keys are text strings.
 * \value CborValidateNoUndefined           Validate that no elements of type "undefined" are present.
 * \value CborValidateNoTags                Validate that no tags are used.
 * \value CborValidateFiniteFloatingPoint   Validate that all floating point numbers are finite (no NaN or
 *                                          infinities are allowed).
 * \value CborValidateCompleteData          Validate that the stream is complete and there is no more data
 *                                          in the buffer.
 * \value CborValidateNoUnknownSimpleTypesSA Validate that all Standards Action simple types are registered
 *                                          with IANA.
 * \value CborValidateNoUnknownSimpleTypes  Validate that all simple types used are registered with IANA.
 * \value CborValidateNoUnknownTagsSA       Validate that all Standard Actions tags are registered with IANA.
 * \value CborValidateNoUnknownTagsSR       Validate that all Standard Actions and Specification Required tags
 *                                          are registered with IANA (see below for limitations).
 * \value CborValidateNoUnkonwnTags         Validate that all tags are registered with IANA
 *                                          (see below for limitations).
 *
 * \par Simple type registry
 * The CBOR specification requires that registration for use of the first 19
 * simple types must be done by way of Standards Action. The rest of the simple
 * types only require a specification. The official list can be obtained from
 *  https://www.iana.org/assignments/cbor-simple-values/cbor-simple-values.xhtml.
 *
 * \par
 * There are no registered simple types recognized by this release of TinyCBOR
 * (beyond those defined by RFC 7049).
 *
 * \par Tag registry
 * The CBOR specification requires that registration for use of the first 23
 * tags must be done by way of Standards Action. The next up to tag 255 only
 * require a specification. Finally, all other tags can be registered on a
 * first-come-first-serve basis. The official list can be ontained from
 *  https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml.
 *
 * \par
 * Given the variability of this list, TinyCBOR cannot recognize all tags
 * registered with IANA. Instead, the implementation only recognizes tags
 * that are backed by an RFC.
 *
 * \par
 * These are the tags known to the current TinyCBOR release:
<table>
  <tr>
    <th>Tag</th>
    <th>Data Item</th>
    <th>Semantics</th>
  </tr>
  <tr>
    <td>0</td>
    <td>UTF-8 text string</td>
    <td>Standard date/time string</td>
  </td>
  <tr>
    <td>1</td>
    <td>integer</td>
    <td>Epoch-based date/time</td>
  </td>
  <tr>
    <td>2</td>
    <td>byte string</td>
    <td>Positive bignum</td>
  </td>
  <tr>
    <td>3</td>
    <td>byte string</td>
    <td>Negative bignum</td>
  </td>
  <tr>
    <td>4</td>
    <td>array</td>
    <td>Decimal fraction</td>
  </td>
  <tr>
    <td>5</td>
    <td>array</td>
    <td>Bigfloat</td>
  </td>
  <tr>
    <td>16</td>
    <td>array</td>
    <td>COSE Single Recipient Encrypted Data Object (RFC 8152)</td>
  </td>
  <tr>
    <td>17</td>
    <td>array</td>
    <td>COSE Mac w/o Recipients Object (RFC 8152)</td>
  </td>
  <tr>
    <td>18</td>
    <td>array</td>
    <td>COSE Single Signer Data Object (RFC 8162)</td>
  </td>
  <tr>
    <td>21</td>
    <td>byte string, array, map</td>
    <td>Expected conversion to base64url encoding</td>
  </td>
  <tr>
    <td>22</td>
    <td>byte string, array, map</td>
    <td>Expected conversion to base64 encoding</td>
  </td>
  <tr>
    <td>23</td>
    <td>byte string, array, map</td>
    <td>Expected conversion to base16 encoding</td>
  </td>
  <tr>
    <td>24</td>
    <td>byte string</td>
    <td>Encoded CBOR data item</td>
  </td>
  <tr>
    <td>32</td>
    <td>UTF-8 text string</td>
    <td>URI</td>
  </td>
  <tr>
    <td>33</td>
    <td>UTF-8 text string</td>
    <td>base64url</td>
  </td>
  <tr>
    <td>34</td>
    <td>UTF-8 text string</td>
    <td>base64</td>
  </td>
  <tr>
    <td>35</td>
    <td>UTF-8 text string</td>
    <td>Regular expression</td>
  </td>
  <tr>
    <td>36</td>
    <td>UTF-8 text string</td>
    <td>MIME message</td>
  </td>
  <tr>
    <td>96</td>
    <td>array</td>
    <td>COSE Encrypted Data Object (RFC 8152)</td>
  </td>
  <tr>
    <td>97</td>
    <td>array</td>
    <td>COSE MACed Data Object (RFC 8152)</td>
  </td>
  <tr>
    <td>98</td>
    <td>array</td>
    <td>COSE Signed Data Object (RFC 8152)</td>
  </td>
  <tr>
    <td>55799</td>
    <td>any</td>
    <td>Self-describe CBOR</td>
  </td>
</table>
 */

struct KnownTagData { uint32_t tag; uint32_t types; };
static const struct KnownTagData knownTagData[] = {
    { 0, (uint8_t)CborTextStringType },
    { 1, (uint8_t)(CborIntegerType+1) },
    { 2, (uint8_t)CborByteStringType },
    { 3, (uint8_t)CborByteStringType },
    { 4, (uint8_t)CborArrayType },
    { 5, (uint8_t)CborArrayType },
    { 16, (uint8_t)CborArrayType },
    { 17, (uint8_t)CborArrayType },
    { 18, (uint8_t)CborArrayType },
    { 21, (uint8_t)CborByteStringType | ((uint8_t)CborArrayType << 8) | ((uint8_t)CborMapType << 16) },
    { 22, (uint8_t)CborByteStringType | ((uint8_t)CborArrayType << 8) | ((uint8_t)CborMapType << 16) },
    { 23, (uint8_t)CborByteStringType | ((uint8_t)CborArrayType << 8) | ((uint8_t)CborMapType << 16) },
    { 24, (uint8_t)CborByteStringType },
    { 32, (uint8_t)CborTextStringType },
    { 33, (uint8_t)CborTextStringType },
    { 34, (uint8_t)CborTextStringType },
    { 35, (uint8_t)CborTextStringType },
    { 36, (uint8_t)CborTextStringType },
    { 96, (uint8_t)CborArrayType },
    { 97, (uint8_t)CborArrayType },
    { 98, (uint8_t)CborArrayType },
    { 55799, 0U }
};

static CborError validate_value(CborValue *it, int flags, int recursionLeft);

static inline CborError validate_utf8_string(const void *ptr, size_t n)
{
    const uint8_t *buffer = (const uint8_t *)ptr;
    const uint8_t * const end = buffer + n;
    while (buffer < end) {
        uint32_t uc = get_utf8(&buffer, end);
        if (uc == ~0U)
            return CborErrorInvalidUtf8TextString;
    }
    return CborNoError;
}

static inline CborError validate_simple_type(uint8_t simple_type, int flags)
{
    /* At current time, all known simple types are those from RFC 7049,
     * which are parsed by the parser into different CBOR types.
     * That means that if we've got here, the type is unknown */
    if (simple_type < 32)
        return (flags & CborValidateNoUnknownSimpleTypesSA) ? CborErrorUnknownSimpleType : CborNoError;
    return (flags & CborValidateNoUnknownSimpleTypes) == CborValidateNoUnknownSimpleTypes ?
                CborErrorUnknownSimpleType : CborNoError;
}

static inline CborError validate_number(const CborValue *it, CborType type, int flags)
{
    CborError err = CborNoError;
    uint64_t value;
    int offset;

    if ((flags & CborValidateShortestIntegrals) == 0)
        return err;
    if (type >= CborHalfFloatType && type <= CborDoubleType)
        return err;     /* checked elsewhere */

    offset = it->offset;
    err = _cbor_value_extract_number(it->parser, &offset, &value);
    if (err)
        return err;

    size_t bytesUsed = (size_t)(offset - it->offset - 1);
    size_t bytesNeeded = 0;
    if (value >= Value8Bit)
        ++bytesNeeded;
    if (value > 0xffU)
        ++bytesNeeded;
    if (value > 0xffffU)
        bytesNeeded += 2;
    if (value > 0xffffffffU)
        bytesNeeded += 4;
    if (bytesNeeded < bytesUsed)
        return CborErrorOverlongEncoding;
    return CborNoError;
}

static inline CborError validate_tag(CborValue *it, CborTag tag, int flags, int recursionLeft)
{
    CborType type = cbor_value_get_type(it);
    const size_t knownTagCount = sizeof(knownTagData) / sizeof(knownTagData[0]);
    const struct KnownTagData *tagData = knownTagData;
    const struct KnownTagData * const knownTagDataEnd = knownTagData + knownTagCount;

    if (!recursionLeft)
        return CborErrorNestingTooDeep;
    if (flags & CborValidateNoTags)
        return CborErrorExcludedType;

    /* find the tag data, if any */
    for ( ; tagData != knownTagDataEnd; ++tagData) {
        if (tagData->tag < tag)
            continue;
        if (tagData->tag > tag)
            tagData = NULL;
        break;
    }
    if (tagData == knownTagDataEnd)
        tagData = NULL;

    if (flags & CborValidateNoUnknownTags && !tagData) {
        /* tag not found */
        if (flags & CborValidateNoUnknownTagsSA && tag < 24)
            return CborErrorUnknownTag;
        if ((flags & CborValidateNoUnknownTagsSR) == CborValidateNoUnknownTagsSR && tag < 256)
            return CborErrorUnknownTag;
        if ((flags & CborValidateNoUnknownTags) == CborValidateNoUnknownTags)
            return CborErrorUnknownTag;
    }

    if (flags & CborValidateTagUse && tagData && tagData->types) {
        uint32_t allowedTypes = tagData->types;

        /* correct Integer so it's not zero */
        if (type == CborIntegerType)
            type = (CborType)(type + 1);

        while (allowedTypes) {
            if ((uint8_t)(allowedTypes & 0xff) == type)
                break;
            allowedTypes >>= 8;
        }
        if (!allowedTypes)
            return CborErrorInappropriateTagForType;
    }

    return validate_value(it, flags, recursionLeft);
}

#ifndef CBOR_NO_FLOATING_POINT
static inline CborError validate_floating_point(CborValue *it, CborType type, int flags)
{
    CborError err;
    double val;
    float valf;
    uint16_t valf16;

    if (type != CborDoubleType) {
        if (type == CborFloatType) {
            err = cbor_value_get_float(it, &valf);
            val = valf;
        } else {
#  ifdef CBOR_NO_HALF_FLOAT_TYPE
            (void)val16;
            return CborErrorUnsupportedType;
#  else
            err = cbor_value_get_half_float(it, &valf16);
            val = decode_half(valf16);
#  endif
        }
    } else {
        err = cbor_value_get_double(it, &val);
    }
    cbor_assert(err == CborNoError);     /* can't fail */

    int r = fpclassify(val);
    if (r == FP_NAN || r == FP_INFINITE) {
        if (flags & CborValidateFiniteFloatingPoint)
            return CborErrorExcludedValue;
        if (flags & CborValidateShortestFloatingPoint) {
            if (type == CborDoubleType)
                return CborErrorOverlongEncoding;
#  ifndef CBOR_NO_HALF_FLOAT_TYPE
            if (type == CborFloatType)
                return CborErrorOverlongEncoding;
            if (r == FP_NAN && valf16 != 0x7e00)
                return CborErrorImproperValue;
            if (r == FP_INFINITE && valf16 != 0x7c00 && valf16 != 0xfc00)
                return CborErrorImproperValue;
#  endif
        }
    }

    if (flags & CborValidateShortestFloatingPoint && type > CborHalfFloatType) {
        if (type == CborDoubleType) {
            valf = (float)val;
            if ((double)valf == val)
                return CborErrorOverlongEncoding;
        }
#  ifndef CBOR_NO_HALF_FLOAT_TYPE
        if (type == CborFloatType) {
            valf16 = encode_half(valf);
            if (valf == decode_half(valf16))
                return CborErrorOverlongEncoding;
        }
#  endif
    }

    return CborNoError;
}
#endif

static CborError validate_container(CborValue *it, int containerType, int flags, int recursionLeft)
{
    CborError err;
    int previous = -1;
    int previous_end = -1;

    if (!recursionLeft)
        return CborErrorNestingTooDeep;

    while (!cbor_value_at_end(it)) {
        int current = it->offset;

        if (containerType == CborMapType) {
            if (flags & CborValidateMapKeysAreString) {
                CborType type = cbor_value_get_type(it);
                if (type == CborTagType) {
                    /* skip the tags */
                    CborValue copy = *it;
                    err = cbor_value_skip_tag(&copy);
                    if (err)
                        return err;
                    type = cbor_value_get_type(&copy);
                }
                if (type != CborTextStringType)
                    return CborErrorMapKeyNotString;
            }
        }

        err = validate_value(it, flags, recursionLeft);
        if (err)
            return err;

        if (containerType != CborMapType)
            continue;

        if (flags & CborValidateMapIsSorted) {
            if (previous != -1) {
                uint64_t len1, len2;
                int offset;

                /* extract the two lengths */
                offset = previous;
                _cbor_value_extract_number(it->parser, &offset, &len1);
                offset = current;
                _cbor_value_extract_number(it->parser, &offset, &len2);

                if (len1 > len2)
                    return CborErrorMapNotSorted;
                if (len1 == len2) {
                    int bytelen1 = (previous_end - previous);
                    int bytelen2 = (it->offset - current);
                    int i;

                    for (i = 0; i < (bytelen1 <= bytelen2 ? bytelen1 : bytelen2); i++) {
                        int r = it->parser->d->get8(it->parser->d, previous + i) -
                            it->parser->d->get8(it->parser->d, current + i);

                        if (r < 0) {
                            break;
                        }
                        if (r == 0 && bytelen1 != bytelen2)
                            r = bytelen1 < bytelen2 ? -1 : +1;
                        if (r > 0)
                            return CborErrorMapNotSorted;
                        if (r == 0 && (flags & CborValidateMapKeysAreUnique) == CborValidateMapKeysAreUnique)
                            return CborErrorMapKeysNotUnique;
                    }
                }
            }

            previous = current;
            previous_end = it->offset;
        }

        /* map: that was the key, so get the value */
        err = validate_value(it, flags, recursionLeft);
        if (err)
            return err;
    }
    return CborNoError;
}

static CborError validate_value(CborValue *it, int flags, int recursionLeft)
{
    CborError err;
    CborType type = cbor_value_get_type(it);

    if (cbor_value_is_length_known(it)) {
        err = validate_number(it, type, flags);
        if (err)
            return err;
    } else {
        if (flags & CborValidateNoIndeterminateLength)
            return CborErrorUnknownLength;
    }

    switch (type) {
    case CborArrayType:
    case CborMapType: {
        /* recursive type */
        CborValue recursed;
        err = cbor_value_enter_container(it, &recursed);
        if (!err)
            err = validate_container(&recursed, type, flags, recursionLeft - 1);
        if (err) {
            it->offset = recursed.offset;
            return err;
        }
        err = cbor_value_leave_container(it, &recursed);
        if (err)
            return err;
        return CborNoError;
    }

    case CborIntegerType: {
        uint64_t val;
        err = cbor_value_get_raw_integer(it, &val);
        cbor_assert(err == CborNoError);         /* can't fail */

        break;
    }

    case CborByteStringType:
    case CborTextStringType: {
        size_t n = 0;
        const void *ptr;

        err = _cbor_value_prepare_string_iteration(it);
        if (err)
            return err;

        while (1) {
            err = validate_number(it, type, flags);
            if (err)
                return err;

            err = _cbor_value_get_string_chunk(it, &ptr, &n, it);
            if (err)
                return err;
            if (!ptr)
                break;

            if (type == CborTextStringType && flags & CborValidateUtf8) {
                err = validate_utf8_string(ptr, n);
                if (err)
                    return err;
            }
        }

        return CborNoError;
    }

    case CborTagType: {
        CborTag tag;
        err = cbor_value_get_tag(it, &tag);
        cbor_assert(err == CborNoError);     /* can't fail */

        err = cbor_value_advance_fixed(it);
        if (err)
            return err;
        err = validate_tag(it, tag, flags, recursionLeft - 1);
        if (err)
            return err;

        return CborNoError;
    }

    case CborSimpleType: {
        uint8_t simple_type;
        err = cbor_value_get_simple_type(it, &simple_type);
        cbor_assert(err == CborNoError);     /* can't fail */
        err = validate_simple_type(simple_type, flags);
        if (err)
            return err;
        break;
    }

    case CborNullType:
    case CborBooleanType:
        break;

    case CborUndefinedType:
        if (flags & CborValidateNoUndefined)
            return CborErrorExcludedType;
        break;

    case CborHalfFloatType:
    case CborFloatType:
    case CborDoubleType: {
#ifdef CBOR_NO_FLOATING_POINT
        return CborErrorUnsupportedType;
#else
        err = validate_floating_point(it, type, flags);
        if (err)
            return err;
        break;
    }
#endif /* !CBOR_NO_FLOATING_POINT */

    case CborInvalidType:
        return CborErrorUnknownType;
    }

    err = cbor_value_advance_fixed(it);
    return err;
}

/**
 * Performs a full validation controlled by the \a flags options of the CBOR
 * stream pointed by \a it and returns the error it found. If no error was
 * found, it returns CborNoError and the application can iterate over the items
 * with certainty that no other errors will appear during parsing.
 *
 * If \a flags is CborValidateBasic, the result should be the same as
 * cbor_value_validate_basic().
 *
 * This function has the same timing and memory requirements as
 * cbor_value_advance() and cbor_value_validate_basic().
 *
 * \sa CborValidationFlags, cbor_value_validate_basic(), cbor_value_advance()
 */
CborError cbor_value_validate(const CborValue *it, int flags)
{
    CborValue value = *it;
    CborError err = validate_value(&value, flags, CBOR_PARSER_MAX_RECURSIONS);
    if (err)
        return err;
    if (flags & CborValidateCompleteData && it->offset != it->parser->end)
        return CborErrorGarbageAtEnd;
    return CborNoError;
}

/**
 * @}
 */
