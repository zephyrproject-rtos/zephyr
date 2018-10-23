/*
 * Copyright (c) 2018
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_LIB_CPLUSPLUS)
#include <new>
#endif // CONFIG_LIB_CPLUSPLUS
#include <kernel.h>

void* operator new(size_t size)
{
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	void* ptr = k_malloc(size);
#if defined(__cpp_exceptions) && defined(CONFIG_LIB_CPLUSPLUS)
	if (!ptr)
		throw std::bad_alloc();
#endif
	return ptr;
#else
	ARG_UNUSED(size);
	return NULL;
#endif
}

void* operator new[](size_t size)
{
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	void* ptr = k_malloc(size);
#if defined(__cpp_exceptions) && defined(CONFIG_LIB_CPLUSPLUS)
	if (!ptr)
		throw std::bad_alloc();
#endif
	return ptr;
#else
	ARG_UNUSED(size);
	return NULL;
#endif
}

void operator delete(void* ptr) noexcept
{
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	k_free(ptr);
#else
	ARG_UNUSED(ptr);
#endif
}

void operator delete[](void* ptr) noexcept
{
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	k_free(ptr);
#else
	ARG_UNUSED(ptr);
#endif
}

#if defined(CONFIG_LIB_CPLUSPLUS)
void* operator new(size_t size, const std::nothrow_t&) noexcept
{
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	return k_malloc(size);
#else
	ARG_UNUSED(size);
	return NULL;
#endif
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept
{
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	return k_malloc(size);
#else
	ARG_UNUSED(size);
	return NULL;
#endif
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept
{
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	k_free(ptr);
#else
	ARG_UNUSED(ptr);
#endif
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	k_free(ptr);
#else
	ARG_UNUSED(ptr);
#endif
}
#endif // CONFIG_LIB_CPLUSPLUS

#if (__cplusplus > 201103L)
void operator delete(void* ptr, size_t) noexcept
{
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	k_free(ptr);
#else
	ARG_UNUSED(ptr);
#endif
}

void operator delete[](void* ptr, size_t) noexcept
{
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	k_free(ptr);
#else
	ARG_UNUSED(ptr);
#endif
}
#endif // __cplusplus > 201103L
