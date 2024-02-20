/*
 * SPDX-License-Identifier: BSD-3-Claus
 */
/*
 *
 * Copyright (c) [2020] by InvenSense, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/** @brief    Custom definition for boolean type to avoid compiler disrepenrencies
 *	@{
 */

#ifndef _INV_BOOL_H_
#define _INV_BOOL_H_

typedef int inv_bool_t;

#ifndef __cplusplus

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#endif /* __cplusplus */

#endif /* _INV_BOOL_H_ */

/** @} */
