/*
 * Copyright (c) 2018
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <new>

#if __cplusplus < 201103L
#define NOEXCEPT
#else /* >= C++11 */
#define NOEXCEPT noexcept
#endif /* __cplusplus */

#if __cplusplus < 202002L
#define NODISCARD
#else
#define NODISCARD [[nodiscard]]
#endif /* __cplusplus */

NODISCARD void* operator new(size_t size)
{
	return malloc(size);
}

NODISCARD void* operator new[](size_t size)
{
	return malloc(size);
}

NODISCARD void* operator new(std::size_t size, const std::nothrow_t& tag) NOEXCEPT
{
	return malloc(size);
}

NODISCARD void* operator new[](std::size_t size, const std::nothrow_t& tag) NOEXCEPT
{
	return malloc(size);
}

#if __cplusplus >= 201703L
NODISCARD void* operator new(size_t size, std::align_val_t al)
{
	return aligned_alloc(static_cast<size_t>(al), size);
}

NODISCARD void* operator new[](std::size_t size, std::align_val_t al)
{
	return aligned_alloc(static_cast<size_t>(al), size);
}

NODISCARD void* operator new(std::size_t size, std::align_val_t al,
			     const std::nothrow_t&) NOEXCEPT
{
	return aligned_alloc(static_cast<size_t>(al), size);
}

NODISCARD void* operator new[](std::size_t size, std::align_val_t al,
			       const std::nothrow_t&) NOEXCEPT
{
	return aligned_alloc(static_cast<size_t>(al), size);
}
#endif /* __cplusplus >= 201703L */

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
