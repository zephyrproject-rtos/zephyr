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

#define _BSD_SOURCE 1
#define _DEFAULT_SOURCE 1
#ifndef __STDC_LIMIT_MACROS
#  define __STDC_LIMIT_MACROS 1
#endif

#include "cbor.h"
#include "compilersupport_p.h"
#include "cborinternal_p.h"
#include "utf8_p.h"

#include <inttypes.h>
#include <float.h>
#ifndef CBOR_NO_FLOATING_POINT
#include <math.h>
#endif
#include <string.h>

/**
 * \defgroup CborPretty Converting CBOR to text
 * \brief Group of functions used to convert CBOR to text form.
 *
 * This group contains two functions that are can be used to convert one
 * CborValue object to a text representation. This module attempts to follow
 * the recommendations from RFC 7049 section 6 "Diagnostic Notation", though it
 * has a few differences. They are noted below.
 *
 * TinyCBOR does not provide a way to convert from the text representation back
 * to encoded form. To produce a text form meant to be parsed, CborToJson is
 * recommended instead.
 *
 * Either of the functions in this section will attempt to convert exactly one
 * CborValue object to text. Those functions may return any error documented
 * for the functions for CborParsing. In addition, if the C standard library
 * stream functions return with error, the text conversion will return with
 * error CborErrorIO.
 *
 * These functions also perform UTF-8 validation in CBOR text strings. If they
 * encounter a sequence of bytes that not permitted in UTF-8, they will return
 * CborErrorInvalidUtf8TextString. That includes encoding of surrogate points
 * in UTF-8.
 *
 * \warning The output type produced by these functions is not guaranteed to
 * remain stable. A future update of TinyCBOR may produce different output for
 * the same input and parsers may be unable to handle them.
 *
 * \sa CborParsing, CborToJson, cbor_parser_init()
 */

/**
 * \addtogroup CborPretty
 * @{
 * <h2 class="groupheader">Text format</h2>
 *
 * As described in RFC 7049 section 6 "Diagnostic Notation", the format is
 * largely borrowed from JSON, but modified to suit CBOR's different data
 * types. TinyCBOR makes further modifications to distinguish different, but
 * similar values.
 *
 * CBOR values are currently encoded as follows:
 * \par Integrals (unsigned and negative)
 *      Base-10 (decimal) text representation of the value
 * \par Byte strings:
 *      <tt>"h'"</tt> followed by the Base16 (hex) representation of the binary data, followed by an ending quote (')
 * \par Text strings:
 *      C-style escaped string in quotes, with C11/C++11 escaping of Unicode codepoints above U+007F.
 * \par Tags:
 *      Tag value, with the tagged value in parentheses. No special encoding of the tagged value is performed.
 * \par Simple types:
 *      <tt>"simple(nn)"</tt> where \c nn is the simple value
 * \par Null:
 *      \c null
 * \par Undefined:
 *      \c undefined
 * \par Booleans:
 *      \c true or \c false
 * \par Floating point:
 *      If NaN or infinite, the actual words \c NaN or \c infinite.
 *      Otherwise, the decimal representation with as many digits as necessary to ensure no loss of information.
 *      By default, float values are suffixed by "f" and half-float values suffixed by "f16" (doubles have no suffix).
 *      If the CborPrettyNumericEncodingIndicators flag is active, the values instead are encoded following the
 *      Section 6 recommended encoding indicators: float values are suffixed with "_2" and half-float with "_1".
 *      A dot is always present.
 * \par Arrays:
 *      Comma-separated list of elements, enclosed in square brackets ("[" and "]").
 * \par Maps:
 *      Comma-separated list of key-value pairs, with the key and value separated
 *      by a colon (":"), enclosed in curly braces ("{" and "}").
 *
 * The CborPrettyFlags enumerator contains flags to control some aspects of the
 * encoding:
 * \par String fragmentation
 *      When the CborPrettyShowStringFragments option is active, text and byte
 *      strings that are transmitted in fragments are shown instead inside
 *      parentheses ("(" and ")") with no preceding number and each fragment is
 *      displayed individually. If a tag precedes the string, then the output
 *      will contain a double set of parentheses. If the option is not active,
 *      the fragments are merged together and the display will not show any
 *      difference from a string transmitted with determinate length.
 * \par Encoding indicators
 *      Numbers and lengths in CBOR can be encoded in multiple representations.
 *      If the CborPrettyIndicateOverlongNumbers option is active, numbers
 *      and lengths that are transmitted in a longer encoding than necessary
 *      will be indicated, by appending an underscore ("_") to either the
 *      number or the opening bracket or brace, followed by a number
 *      indicating the CBOR additional information: 0 for 1 byte, 1 for 2
 *      bytes, 2 for 4 bytes and 3 for 8 bytes.
 *      If the CborPrettyIndicateIndetermineLength option is active, maps,
 *      arrays and strings encoded with indeterminate length will be marked by
 *      an underscore after the opening bracket or brace or the string (if not
 *      showing fragments), without a number after it.
 */

/**
 * \enum CborPrettyFlags
 * The CborPrettyFlags enum contains flags that control the conversion of CBOR to text format.
 *
 * \value CborPrettyNumericEncodingIndicators   Use numeric encoding indicators instead of textual for float and half-float.
 * \value CborPrettyTextualEncodingIndicators   Use textual encoding indicators for float ("f") and half-float ("f16").
 * \value CborPrettyIndicateIndetermineLength   Indicate when a map or array has indeterminate length.
 * \value CborPrettyIndicateOverlongNumbers     Indicate when a number or length was encoded with more bytes than needed.
 * \value CborPrettyShowStringFragments         If the byte or text string is transmitted in chunks, show each individually.
 * \value CborPrettyMergeStringFragment         Merge all chunked byte or text strings and display them in a single entry.
 * \value CborPrettyDefaultFlags       Default conversion flags.
 */

static void printRecursionLimit(CborStreamFunction stream, void *out)
{
    stream(out, "<nesting too deep, recursion stopped>");
}

static CborError hexDump(CborStreamFunction stream, void *out, const void *ptr, size_t n)
{
    const uint8_t *buffer = (const uint8_t *)ptr;
    CborError err = CborNoError;
    while (n-- && !err)
        err = stream(out, "%02" PRIx8, *buffer++);

    return err;
}

/* This function decodes buffer as UTF-8 and prints as escaped UTF-16.
 * On UTF-8 decoding error, it returns CborErrorInvalidUtf8TextString */
static CborError utf8EscapedDump(CborStreamFunction stream, void *out, const void *ptr, size_t n)
{
    const uint8_t *buffer = (const uint8_t *)ptr;
    const uint8_t * const end = buffer + n;
    CborError err = CborNoError;

    while (buffer < end && !err) {
        uint32_t uc = get_utf8(&buffer, end);
        if (uc == ~0U)
            return CborErrorInvalidUtf8TextString;

        if (uc < 0x80) {
            /* single-byte UTF-8 */
            if (uc < 0x7f && uc >= 0x20 && uc != '\\' && uc != '"') {
                err = stream(out, "%c", (char)uc);
                continue;
            }

            /* print as an escape sequence */
            char escaped = (char)uc;
            switch (uc) {
            case '"':
            case '\\':
                break;
            case '\b':
                escaped = 'b';
                break;
            case '\f':
                escaped = 'f';
                break;
            case '\n':
                escaped = 'n';
                break;
            case '\r':
                escaped = 'r';
                break;
            case '\t':
                escaped = 't';
                break;
            default:
                goto print_utf16;
            }
            err = stream(out, "\\%c", escaped);
            continue;
        }

        /* now print the sequence */
        if (uc > 0xffffU) {
            /* needs surrogate pairs */
            err = stream(out, "\\u%04" PRIX32 "\\u%04" PRIX32,
                         (uc >> 10) + 0xd7c0,    /* high surrogate */
                         (uc % 0x0400) + 0xdc00);
        } else {
print_utf16:
            /* no surrogate pair needed */
            err = stream(out, "\\u%04" PRIX32, uc);
        }
    }
    return err;
}

static const char *resolve_indicator(const CborValue *it, int flags)
{
    static const char indicators[8][3] = {
        "_0", "_1", "_2", "_3",
        "", "", "",             /* these are not possible */
        "_"
    };
    const char *no_indicator = indicators[5];   /* empty string */
    uint8_t additional_information;
    uint8_t expected_information;
    uint64_t value;
    CborError err;
    int offset;
    uint8_t val;

    if (it->offset == it->parser->end)
        return NULL;    /* CborErrorUnexpectedEOF */

    val = it->parser->d->get8(it->parser->d, it->offset);

    additional_information = (val & SmallValueMask);
    if (additional_information < Value8Bit)
        return no_indicator;

    /* determine whether to show anything */
    if ((flags & CborPrettyIndicateIndetermineLength) &&
            additional_information == IndefiniteLength)
        return indicators[IndefiniteLength - Value8Bit];
    if ((flags & CborPrettyIndicateOverlongNumbers) == 0)
        return no_indicator;

    offset = it->offset;

    err = _cbor_value_extract_number(it->parser, &offset, &value);
    if (err)
        return NULL;    /* CborErrorUnexpectedEOF */

    expected_information = Value8Bit - 1;
    if (value >= Value8Bit)
        ++expected_information;
    if (value > 0xffU)
        ++expected_information;
    if (value > 0xffffU)
        ++expected_information;
    if (value > 0xffffffffU)
        ++expected_information;
    return expected_information == additional_information ?
                no_indicator :
                indicators[additional_information - Value8Bit];
}

static const char *get_indicator(const CborValue *it, int flags)
{
    return resolve_indicator(it, flags);
}

static CborError value_to_pretty(CborStreamFunction stream, void *out, CborValue *it, int flags, int recursionsLeft);
static CborError container_to_pretty(CborStreamFunction stream, void *out, CborValue *it, CborType containerType,
                                     int flags, int recursionsLeft)
{
    if (!recursionsLeft) {
        printRecursionLimit(stream, out);
        return CborNoError; /* do allow the dumping to continue */
    }

    const char *comma = "";
    CborError err = CborNoError;
    while (!cbor_value_at_end(it) && !err) {
        err = stream(out, "%s", comma);
        comma = ", ";

        if (!err)
            err = value_to_pretty(stream, out, it, flags, recursionsLeft);

        if (containerType == CborArrayType)
            continue;

        /* map: that was the key, so get the value */
        if (!err)
            err = stream(out, ": ");
        if (!err)
            err = value_to_pretty(stream, out, it, flags, recursionsLeft);
    }
    return err;
}

static CborError value_to_pretty(CborStreamFunction stream, void *out, CborValue *it, int flags, int recursionsLeft)
{
    CborError err = CborNoError;
    CborType type = cbor_value_get_type(it);
    switch (type) {
    case CborArrayType:
    case CborMapType: {
        /* recursive type */
        CborValue recursed;
        const char *indicator = get_indicator(it, flags);
        const char *space = *indicator ? " " : indicator;

        err = stream(out, "%c%s%s", type == CborArrayType ? '[' : '{', indicator, space);
        if (err)
            return err;

        err = cbor_value_enter_container(it, &recursed);
        if (err) {
            it->offset = recursed.offset;
            return err;       /* parse error */
        }
        err = container_to_pretty(stream, out, &recursed, type, flags, recursionsLeft - 1);
        if (err) {
            it->offset = recursed.offset;
            return err;       /* parse error */
        }
        err = cbor_value_leave_container(it, &recursed);
        if (err)
            return err;       /* parse error */

        return stream(out, type == CborArrayType ? "]" : "}");
    }

    case CborIntegerType: {
        uint64_t val;
        cbor_value_get_raw_integer(it, &val);    /* can't fail */

        if (cbor_value_is_unsigned_integer(it)) {
            err = stream(out, "%" PRIu64, val);
        } else {
            /* CBOR stores the negative number X as -1 - X
             * (that is, -1 is stored as 0, -2 as 1 and so forth) */
            if (++val) {                /* unsigned overflow may happen */
                err = stream(out, "-%" PRIu64, val);
            } else {
                /* overflown
                 *   0xffff`ffff`ffff`ffff + 1 =
                 * 0x1`0000`0000`0000`0000 = 18446744073709551616 (2^64) */
                err = stream(out, "-18446744073709551616");
            }
        }
        if (!err)
            err = stream(out, "%s", get_indicator(it, flags));
        break;
    }

    case CborByteStringType:
    case CborTextStringType: {
        size_t n = 0;
        const void *ptr;
        bool showingFragments = (flags & CborPrettyShowStringFragments) && !cbor_value_is_length_known(it);
        const char *separator = "";
        char close = '\'';
        char open[3] = "h'";
        const char *indicator = NULL;

        if (type == CborTextStringType) {
            close = open[0] = '"';
            open[1] = '\0';
        }

        if (showingFragments) {
            err = stream(out, "(_ ");
            if (!err)
                err = _cbor_value_prepare_string_iteration(it);
        } else {
            err = stream(out, "%s", open);
        }

        while (!err) {
            if (showingFragments || indicator == NULL) {
                /* any iteration, except the second for a non-chunked string */
                indicator = resolve_indicator(it, flags);
            }

            err = _cbor_value_get_string_chunk(it, &ptr, &n, it);
            if (!ptr)
                break;

            if (!err && showingFragments)
                err = stream(out, "%s%s", separator, open);
            if (!err)
                err = (type == CborByteStringType ?
                           hexDump(stream, out, ptr, n) :
                           utf8EscapedDump(stream, out, ptr, n));
            if (!err && showingFragments) {
                err = stream(out, "%c%s", close, indicator);
                separator = ", ";
            }
        }

        if (!err) {
            if (showingFragments)
                err = stream(out, ")");
            else
                err = stream(out, "%c%s", close, indicator);
        }
        return err;
    }

    case CborTagType: {
        CborTag tag;
        cbor_value_get_tag(it, &tag);       /* can't fail */
        err = stream(out, "%" PRIu64 "%s(", tag, get_indicator(it, flags));
        if (!err)
            err = cbor_value_advance_fixed(it);
        if (!err && recursionsLeft)
            err = value_to_pretty(stream, out, it, flags, recursionsLeft - 1);
        else if (!err)
            printRecursionLimit(stream, out);
        if (!err)
            err = stream(out, ")");
        return err;
    }

    case CborSimpleType: {
        /* simple types can't fail and can't have overlong encoding */
        uint8_t simple_type;
        cbor_value_get_simple_type(it, &simple_type);
        err = stream(out, "simple(%" PRIu8 ")", simple_type);
        break;
    }

    case CborNullType:
        err = stream(out, "null");
        break;

    case CborUndefinedType:
        err = stream(out, "undefined");
        break;

    case CborBooleanType: {
        bool val;
        cbor_value_get_boolean(it, &val);       /* can't fail */
        err = stream(out, val ? "true" : "false");
        break;
    }
#ifndef CBOR_NO_FLOATING_POINT
    case CborDoubleType: {
        const char *suffix;
        double val;
        int r;
        if (false) {
            float f;
    case CborFloatType:
            cbor_value_get_float(it, &f);
            val = f;
            suffix = flags & CborPrettyNumericEncodingIndicators ? "_2" : "f";
#ifndef CBOR_NO_HALF_FLOAT_TYPE
        } else if (false) {
            uint16_t f16;
    case CborHalfFloatType:
            cbor_value_get_half_float(it, &f16);
            val = decode_half(f16);
            suffix = flags & CborPrettyNumericEncodingIndicators ? "_1" : "f16";
#endif
        } else {
            cbor_value_get_double(it, &val);
            suffix = "";
        }

        if ((flags & CborPrettyNumericEncodingIndicators) == 0) {
            r = fpclassify(val);
            if (r == FP_NAN || r == FP_INFINITE)
                suffix = "";
        }

        uint64_t ival = (uint64_t)fabs(val);
        if (ival == fabs(val)) {
            /* this double value fits in a 64-bit integer, so show it as such
             * (followed by a floating point suffix, to disambiguate) */
            err = stream(out, "%s%" PRIu64 ".%s", val < 0 ? "-" : "", ival, suffix);
        } else {
            /* this number is definitely not a 64-bit integer */
            err = stream(out, "%." DBL_DECIMAL_DIG_STR "g%s", val, suffix);
        }
        break;
    }
#endif
    case CborInvalidType:
        err = stream(out, "invalid");
        if (err)
            return err;
    }

    if (!err)
        err = cbor_value_advance_fixed(it);
    return err;
}

/**
 * Converts the current CBOR type pointed by \a value to its textual
 * representation and writes it to the stream by calling the \a streamFunction.
 * If an error occurs, this function returns an error code similar to
 * CborParsing.
 *
 * The textual representation can be controlled by the \a flags parameter (see
 * CborPrettyFlags for more information).
 *
 * If no error ocurred, this function advances \a value to the next element.
 * Often, concatenating the text representation of multiple elements can be
 * done by appending a comma to the output stream.
 *
 * The \a streamFunction function will be called with the \a token value as the
 * first parameter and a printf-style format string as the second, with a variable
 * number of further parameters.
 *
 * \sa cbor_value_to_pretty(), cbor_value_to_json_advance()
 */
CborError cbor_value_to_pretty_stream(CborStreamFunction streamFunction, void *token, CborValue *value, int flags)
{
    return value_to_pretty(streamFunction, token, value, flags, CBOR_PARSER_MAX_RECURSIONS);
}

/** @} */
