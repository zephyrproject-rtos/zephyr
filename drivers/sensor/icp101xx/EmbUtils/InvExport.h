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

#ifndef _INV_IDD_EXPORT_H_
#define _INV_IDD_EXPORT_H_

#if defined(_WIN32)
#if !defined(INV_EXPORT) && defined(INV_DO_DLL_EXPORT)
#define INV_EXPORT __declspec(dllexport)
#elif !defined(INV_EXPORT) && defined(INV_DO_DLL_IMPORT)
#define INV_EXPORT __declspec(dllimport)
#endif
#endif

#if !defined(INV_EXPORT)
#define INV_EXPORT
#endif

#endif /* _INV_IDD_EXPORT_H_ */
