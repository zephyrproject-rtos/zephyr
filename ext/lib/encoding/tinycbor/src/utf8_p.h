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

#ifndef CBOR_UTF8_H
#define CBOR_UTF8_H

#include "compilersupport_p.h"

#include <stdint.h>

static inline uint32_t get_utf8(const uint8_t **buffer, const uint8_t *end)
{
    uint32_t uc;
    ptrdiff_t n = end - *buffer;
    if (n == 0)
        return ~0U;

    uc = *(*buffer)++;
    if (uc < 0x80) {
        /* single-byte UTF-8 */
        return uc;
    }

    /* multi-byte UTF-8, decode it */
    int charsNeeded;
    uint32_t min_uc;
    if (unlikely(uc <= 0xC1))
        return ~0U;
    if (uc < 0xE0) {
        /* two-byte UTF-8 */
        charsNeeded = 2;
        min_uc = 0x80;
        uc &= 0x1f;
    } else if (uc < 0xF0) {
        /* three-byte UTF-8 */
        charsNeeded = 3;
        min_uc = 0x800;
        uc &= 0x0f;
    } else if (uc < 0xF5) {
        /* four-byte UTF-8 */
        charsNeeded = 4;
        min_uc = 0x10000;
        uc &= 0x07;
    } else {
        return ~0U;
    }

    if (n < charsNeeded - 1)
        return ~0U;

    /* first continuation character */
    uint8_t b = *(*buffer)++;
    if ((b & 0xc0) != 0x80)
        return ~0U;
    uc <<= 6;
    uc |= b & 0x3f;

    if (charsNeeded > 2) {
        /* second continuation character */
        b = *(*buffer)++;
        if ((b & 0xc0) != 0x80)
            return ~0U;
        uc <<= 6;
        uc |= b & 0x3f;

        if (charsNeeded > 3) {
            /* third continuation character */
            b = *(*buffer)++;
            if ((b & 0xc0) != 0x80)
                return ~0U;
            uc <<= 6;
            uc |= b & 0x3f;
        }
    }

    /* overlong sequence? surrogate pair? out or range? */
    if (uc < min_uc || uc - 0xd800U < 2048U || uc > 0x10ffff)
        return ~0U;

    return uc;
}

#endif // CBOR_UTF8_H
