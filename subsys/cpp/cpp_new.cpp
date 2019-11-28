/*
 * Copyright (c) 2018
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

void* operator new(size_t size)
{
	return malloc(size);
}

void* operator new[](size_t size)
{
	return malloc(size);
}

void operator delete(void* ptr) noexcept
{
	free(ptr);
}

void operator delete[](void* ptr) noexcept
{
	free(ptr);
}

#if (__cplusplus > 201103L)
void operator delete(void* ptr, size_t) noexcept
{
	free(ptr);
}

void operator delete[](void* ptr, size_t) noexcept
{
	free(ptr);
}
#endif // __cplusplus > 201103L
