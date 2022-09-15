/*
 * Copyright (c) 2018
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#if __cplusplus < 201103L
#define NOEXCEPT
#else /* >= C++11 */
#define NOEXCEPT noexcept
#endif /* __cplusplus */

void* operator new(size_t size)
{
	return malloc(size);
}

void* operator new[](size_t size)
{
	return malloc(size);
}

void* operator new(std::size_t size, const std::nothrow_t& tag) NOEXCEPT
{
	return malloc(size);
}

void* operator new[](std::size_t size, const std::nothrow_t& tag) NOEXCEPT
{
	return malloc(size);
}

void operator delete(void* ptr) NOEXCEPT
{
	free(ptr);
}

void operator delete[](void* ptr) NOEXCEPT
{
	free(ptr);
}

#if (__cplusplus > 201103L)
void operator delete(void* ptr, size_t) NOEXCEPT
{
	free(ptr);
}

void operator delete[](void* ptr, size_t) NOEXCEPT
{
	free(ptr);
}
#endif // __cplusplus > 201103L
