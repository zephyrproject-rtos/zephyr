/****************************************************************************
**
** Copyright (C) 2015 Intel Corporation
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

/**
 * \addtogroup CborEncoding
 * @{
 */

/**
 * @deprecated
 *
 * Closes the CBOR container (array or map) provided by \a containerEncoder and
 * updates the CBOR stream provided by \a encoder. Both parameters must be the
 * same as were passed to cbor_encoder_create_array() or
 * cbor_encoder_create_map().
 *
 * Prior to version 0.5, cbor_encoder_close_container() did not check the
 * number of items added. Since that version, it does and now
 * cbor_encoder_close_container_checked() is no longer needed.
 *
 * \sa cbor_encoder_create_array(), cbor_encoder_create_map()
 */
CborError cbor_encoder_close_container_checked(CborEncoder *encoder, const CborEncoder *containerEncoder)
{
    return cbor_encoder_close_container(encoder, containerEncoder);
}

/** @} */
