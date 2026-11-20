/*
 * Copyright (c) 2026 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LLEXT_TEST_CPP_EXT_HPP
#define LLEXT_TEST_CPP_EXT_HPP

/* Hide the dynamic type from the compiler, so calls go through the vtable */
template <typename T> static inline T *opaque(T *p)
{
	__asm__ volatile("" : "+r"(p));
	return p;
}

#endif /* LLEXT_TEST_CPP_EXT_HPP */
