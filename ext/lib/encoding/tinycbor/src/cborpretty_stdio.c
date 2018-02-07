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

#include "cbor.h"
#include <stdarg.h>
#include <stdio.h>

static CborError cbor_fprintf(void *out, const char *fmt, ...)
{
    int n;

    va_list list;
    va_start(list, fmt);
    n = vfprintf((FILE *)out, fmt, list);
    va_end(list);

    return n < 0 ? CborErrorIO : CborNoError;
}

/**
 * \fn CborError cbor_value_to_pretty(FILE *out, const CborValue *value)
 *
 * Converts the current CBOR type pointed by \a value to its textual
 * representation and writes it to the \a out stream. If an error occurs, this
 * function returns an error code similar to CborParsing.
 *
 * \sa cbor_value_to_pretty_advance(), cbor_value_to_json_advance()
 */

/**
 * Converts the current CBOR type pointed by \a value to its textual
 * representation and writes it to the \a out stream. If an error occurs, this
 * function returns an error code similar to CborParsing.
 *
 * If no error ocurred, this function advances \a value to the next element.
 * Often, concatenating the text representation of multiple elements can be
 * done by appending a comma to the output stream.
 *
 * \sa cbor_value_to_pretty(), cbor_value_to_pretty_stream(), cbor_value_to_json_advance()
 */
CborError cbor_value_to_pretty_advance(FILE *out, CborValue *value)
{
    return cbor_value_to_pretty_stream(cbor_fprintf, out, value, CborPrettyDefaultFlags);
}

/**
 * Converts the current CBOR type pointed by \a value to its textual
 * representation and writes it to the \a out stream. If an error occurs, this
 * function returns an error code similar to CborParsing.
 *
 * The textual representation can be controlled by the \a flags parameter (see
 * CborPrettyFlags for more information).
 *
 * If no error ocurred, this function advances \a value to the next element.
 * Often, concatenating the text representation of multiple elements can be
 * done by appending a comma to the output stream.
 *
 * \sa cbor_value_to_pretty_stream(), cbor_value_to_pretty(), cbor_value_to_json_advance()
 */
CborError cbor_value_to_pretty_advance_flags(FILE *out, CborValue *value, int flags)
{
    return cbor_value_to_pretty_stream(cbor_fprintf, out, value, flags);
}

