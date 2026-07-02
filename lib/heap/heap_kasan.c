/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Lightweight heap KASAN for Zephyr sys_heap.
 *
 * Shadow map: 1 bit per ASAN_GRANULE bytes (bit=1 = poisoned).
 * Per-store checks via __asan_store* callbacks; bulk writes redirected via
 * -Dmemset=__asan_memset etc.  Non-instrumented code bypasses - intentional.
 */
#if defined(CONFIG_SYS_HEAP_KASAN_EXTENSIONS)
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#undef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/bitarray.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "heap.h"

#include <zephyr/sys/heap_kasan.h>

LOG_MODULE_DECLARE(os_heap, CONFIG_SYS_HEAP_LOG_LEVEL);

#define ASAN_GRANULE         SYS_HEAP_KASAN_GRANULE
#define ASAN_SLOTS_PER_CHUNK (CHUNK_UNIT / ASAN_GRANULE)

BUILD_ASSERT(IS_POWER_OF_TWO(SYS_HEAP_KASAN_GRANULE) && SYS_HEAP_KASAN_GRANULE <= CHUNK_UNIT,
	     "SYS_HEAP_KASAN_GRANULE must be a power of two <= CHUNK_UNIT");

static uintptr_t asan_heap_min = UINTPTR_MAX;
static uintptr_t asan_heap_max;

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
static Z_THREAD_LOCAL int asan_last_idx;
#endif

struct asan_reg_entry {
	uintptr_t base;
	uintptr_t span;
	uint32_t *bundles; /* cache of ba->bundles for fast check path */
	struct sys_heap *heap;
	sys_bitarray_t *ba;
};

static struct asan_reg_entry asan_reg[CONFIG_SYS_HEAP_KASAN_MAX_HEAPS];

static int asan_reg_count;

static void heap_kasan_init_heap(struct sys_heap *heap);

void heap_kasan_register(struct sys_heap *heap, sys_bitarray_t *ba)
{
	if (asan_reg_count >= CONFIG_SYS_HEAP_KASAN_MAX_HEAPS) {
		LOG_ERR("registration table full; increase CONFIG_SYS_HEAP_KASAN_MAX_HEAPS");
		return;
	}
	asan_reg[asan_reg_count].heap = heap;
	asan_reg[asan_reg_count].ba = ba;
	asan_reg_count++;
}

/* Called by sys_heap_init().
 * heap_kasan_register() must precede it, otherwise kasan_ba stays NULL.
 */
void heap_sanitizer_on_init(struct sys_heap *heap, void *mem, size_t bytes)
{
	ARG_UNUSED(mem);
	ARG_UNUSED(bytes);

	for (int i = 0; i < asan_reg_count; i++) {
		if (asan_reg[i].heap == heap) {
			heap->kasan_ba = asan_reg[i].ba;
			heap_kasan_init_heap(heap);
			asan_reg[i].base = (uintptr_t)chunk_buf(heap->heap);
			asan_reg[i].span = (uintptr_t)heap->heap->end_chunk * CHUNK_UNIT;
			asan_reg[i].bundles = heap->kasan_ba->bundles;
			return;
		}
	}
}

static ALWAYS_INLINE bool reg_contains(int i, uintptr_t addr)
{
	return asan_reg[i].base != 0 &&
	       addr >= asan_reg[i].base &&
	       (addr - asan_reg[i].base) < asan_reg[i].span;
}

/* Returns pointer to the asan_reg entry covering addr, or NULL. */
static const struct asan_reg_entry *find_reg_for_addr(uintptr_t addr)
{
	if (addr < asan_heap_min || addr >= asan_heap_max) {
		return NULL;
	}

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
	int cached = asan_last_idx;

	if (reg_contains(cached, addr)) {
		return &asan_reg[cached];
	}
#endif

	for (int i = 0; i < asan_reg_count; i++) {
		if (!reg_contains(i, addr)) {
			continue;
		}
#if defined(CONFIG_THREAD_LOCAL_STORAGE)
		asan_last_idx = i;
#endif
		return &asan_reg[i];
	}
	return NULL;
}

static void heap_kasan_init_heap(struct sys_heap *heap)
{
	struct z_heap *h = heap->heap;
	size_t asan_slots = h->end_chunk * ASAN_SLOTS_PER_CHUNK;

	__ASSERT(asan_slots <= heap->kasan_ba->num_bundles * 32U, "HEAP KASAN shadow undersized");

	heap->kasan_ba->num_bits = asan_slots;

	__builtin_memset(heap->kasan_ba->bundles, 0xFF,
			 heap->kasan_ba->num_bundles * sizeof(uint32_t));

	uintptr_t base = (uintptr_t)chunk_buf(h);
	uintptr_t heap_end = base + (uintptr_t)h->end_chunk * CHUNK_UNIT;

	if (base < asan_heap_min) {
		asan_heap_min = base;
	}
	if (heap_end > asan_heap_max) {
		asan_heap_max = heap_end;
	}
}

void heap_sanitizer_on_alloc(struct sys_heap *heap, void *mem, size_t bytes)
{
	if (heap->kasan_ba == NULL || bytes == 0) {
		return;
	}

	uintptr_t base = (uintptr_t)chunk_buf(heap->heap);
	size_t data_first_slot = ((uintptr_t)mem - base) / ASAN_GRANULE;
	size_t data_last_slot = ((uintptr_t)mem + bytes - 1 - base) / ASAN_GRANULE;
	size_t data_slots = data_last_slot - data_first_slot + 1;

	sys_bitarray_clear_region(heap->kasan_ba, data_slots, data_first_slot);
}

void heap_sanitizer_on_free(struct sys_heap *heap, void *mem, size_t bytes)
{
	if (heap->kasan_ba == NULL || bytes == 0) {
		return;
	}

	uintptr_t base = (uintptr_t)chunk_buf(heap->heap);
	size_t data_first_slot = ((uintptr_t)mem - base) / ASAN_GRANULE;
	size_t data_last_slot = ((uintptr_t)mem + bytes - 1 - base) / ASAN_GRANULE;
	size_t data_slots = data_last_slot - data_first_slot + 1;

	sys_bitarray_set_region(heap->kasan_ba, data_slots, data_first_slot);
}

void __weak heap_kasan_report(uintptr_t addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
	k_panic();
}

/* No lock by design: acquiring the bitarray spinlock on every store is too costly.
 * A torn read during concurrent alloc/free only causes a missed/spurious report,
 * and only when user code is already racing on the heap.
 */
static void check_write_range(uintptr_t addr, size_t size)
{
	if (size == 0) {
		return;
	}

	const struct asan_reg_entry *reg = find_reg_for_addr(addr);

	if (!reg) {
		return;
	}

	if (size > reg->span - (addr - reg->base)) {
		printk("KASAN POISON: addr=0x%lx size=%zu (write past heap end 0x%lx)\n",
		       (unsigned long)addr, size,
		       (unsigned long)(reg->base + reg->span));
		heap_kasan_report(addr, size);
		return;
	}

	size_t first_slot = (addr - reg->base) / ASAN_GRANULE;
	size_t last_slot = (addr + size - 1 - reg->base) / ASAN_GRANULE;
	size_t first_bidx = first_slot >> 5; /* 32 slots per bundle */
	size_t last_bidx = last_slot >> 5;
	uint32_t *bundles = reg->bundles;

	for (size_t bi = first_bidx; bi <= last_bidx; bi++) {
		uint32_t sb = bundles[bi];

		if (sb == 0u) {
			continue;
		}

		/* Mask out bits outside [first_slot, last_slot] at bundle edges. */
		if (bi == first_bidx) {
			sb &= (uint32_t)(0xFFFFFFFFu << (first_slot & 31u));
		}
		if (bi == last_bidx) {
			sb &= (uint32_t)(0xFFFFFFFFu >> (31u - (last_slot & 31u)));
		}
		if (sb == 0u) {
			continue;
		}

		size_t bit = (size_t)__builtin_ctz((unsigned int)sb);
		uintptr_t viol_addr = reg->base + ((bi << 5) | bit) * ASAN_GRANULE;

		printk("KASAN POISON: viol_addr=0x%lx viol_slot=%zu addr=0x%lx size=%zu "
		       "shadow=0x%08x heap_base=0x%lx\n",
		       (unsigned long)viol_addr, (bi << 5) | bit, (unsigned long)addr, size,
		       bundles[bi], (unsigned long)reg->base);
		heap_kasan_report(viol_addr, size);
		return;
	}
}

/* No lock by design: acquiring the bitarray spinlock on every store is too costly.
 * A torn read during concurrent alloc/free only causes a missed/spurious report,
 * and only when user code is already racing on the heap.
 */
static ALWAYS_INLINE void check_write(uintptr_t addr, size_t size)
{
	const struct asan_reg_entry *reg = find_reg_for_addr(addr);

	if (!reg) {
		return;
	}

	if (size > reg->span - (addr - reg->base)) {
		printk("KASAN POISON: addr=0x%lx size=%u (write past heap end 0x%lx)\n",
		       (unsigned long)addr, (unsigned int)size,
		       (unsigned long)(reg->base + reg->span));
		heap_kasan_report(addr, size);
		return;
	}

	size_t si = (addr - reg->base) / ASAN_GRANULE;
	size_t si_end = (addr + size - 1 - reg->base) / ASAN_GRANULE;
	size_t n_slots = si_end - si + 1;   /* shadow slots covered by write */
	size_t bidx = si >> 5;              /* 32 slots per bundle */
	size_t bit_off = si & 31u;          /* bit offset of si within bundle */
	uint32_t *bundles = reg->bundles;

	if (bit_off + n_slots <= 32u) {
		/* Single bundle: n_slots ones starting at bit_off. */
		uint32_t mask = (uint32_t)((0xFFFFFFFFu >> (32u - n_slots)) << bit_off);

		if (bundles[bidx] & mask) {
			printk("KASAN POISON: addr=0x%lx size=%u slot=%u "
			       "shadow=0x%08x heap_base=0x%lx\n",
			       (unsigned long)addr, (unsigned int)size, (unsigned int)si,
			       bundles[bidx], (unsigned long)reg->base);
			heap_kasan_report(addr, size);
		}
	} else {
		/* Two bundles: overflow slots spill into bundles[bidx+1]. */
		size_t overflow = bit_off + n_slots - 32u;
		uint32_t mask0 = (uint32_t)(0xFFFFFFFFu << bit_off);
		uint32_t mask1 = (uint32_t)((1u << overflow) - 1u);
		uint32_t sb0 = bundles[bidx] & mask0;
		uint32_t sb1 = bundles[bidx + 1] & mask1;

		if (sb0 || sb1) {
			printk("KASAN POISON: addr=0x%lx size=%u slot=%u "
			       "shadow=0x%08x heap_base=0x%lx\n",
			       (unsigned long)addr, (unsigned int)size, (unsigned int)si,
			       sb0 ? bundles[bidx] : bundles[bidx + 1],
			       (unsigned long)reg->base);
			heap_kasan_report(addr, size);
		}
	}
}

void __asan_load1_noabort(uintptr_t addr)
{
	ARG_UNUSED(addr);
}
void __asan_load2_noabort(uintptr_t addr)
{
	ARG_UNUSED(addr);
}
void __asan_load4_noabort(uintptr_t addr)
{
	ARG_UNUSED(addr);
}
void __asan_load8_noabort(uintptr_t addr)
{
	ARG_UNUSED(addr);
}
void __asan_load16_noabort(uintptr_t addr)
{
	ARG_UNUSED(addr);
}
void __asan_loadN_noabort(uintptr_t addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
}

/* -fsanitize=kernel-address emits _noabort variants in outline mode. */
void __asan_store1_noabort(uintptr_t addr)
{
	check_write(addr, 1);
}
void __asan_store2_noabort(uintptr_t addr)
{
	check_write(addr, 2);
}
void __asan_store4_noabort(uintptr_t addr)
{
	check_write(addr, 4);
}
void __asan_store8_noabort(uintptr_t addr)
{
	check_write(addr, 8);
}
void __asan_store16_noabort(uintptr_t addr)
{
	check_write(addr, 16);
}
void __asan_storeN_noabort(uintptr_t addr, size_t size)
{
	check_write_range(addr, size);
}

void __asan_init(void)
{
	/* required ABI symbol */
}
/* GCC requires one of v6/v7/v8 depending on toolchain release. */
void __asan_version_mismatch_check_v6(void)
{
	/* required ABI symbol */
}
void __asan_version_mismatch_check_v7(void)
{
	/* required ABI symbol */
}
void __asan_version_mismatch_check_v8(void)
{
	/* required ABI symbol */
}
void __asan_handle_no_return(void)
{
	/* required ABI symbol; no stack unwinding needed */
}

/* Stack-ASAN stubs - we only track heap memory. */
void __asan_alloca_poison(uintptr_t addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
}

void __asan_allocas_unpoison(uintptr_t top, uintptr_t bottom)
{
	ARG_UNUSED(top);
	ARG_UNUSED(bottom);
}

/* Bulk-write interceptors via -D redirects; __builtin_* avoids re-checking. */
void *__asan_memset(void *s, int c, size_t n)
{
	check_write_range((uintptr_t)s, n);
	return __builtin_memset(s, c, n);
}

void *__asan_memcpy(void *dst, const void *src, size_t n)
{
	check_write_range((uintptr_t)dst, n);
	return __builtin_memcpy(dst, src, n);
}

void *__asan_memmove(void *dst, const void *src, size_t n)
{
	check_write_range((uintptr_t)dst, n);
	return __builtin_memmove(dst, src, n);
}

char *__asan_strcpy(char *dst, const char *src)
{
	check_write_range((uintptr_t)dst, strlen(src) + 1);
	return strcpy(dst, src);
}

char *__asan_strncpy(char *dst, const char *src, size_t n)
{
	check_write_range((uintptr_t)dst, n);
	return strncpy(dst, src, n);
}

char *__asan_strcat(char *dst, const char *src)
{
	check_write_range((uintptr_t)(dst + strlen(dst)), strlen(src) + 1);
	return strcat(dst, src);
}

char *__asan_strncat(char *dst, const char *src, size_t n)
{
	size_t write_len = (n < SIZE_MAX) ? n + 1 : SIZE_MAX;

	check_write_range((uintptr_t)(dst + strlen(dst)), write_len);
	return strncat(dst, src, n);
}

int __asan_vsnprintf(char *dst, size_t n, const char *fmt, va_list ap)
{
	check_write_range((uintptr_t)dst, n);
	return vsnprintf(dst, n, fmt, ap);
}

int __asan_snprintf(char *dst, size_t n, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	int ret = __asan_vsnprintf(dst, n, fmt, ap);

	va_end(ap);
	return ret;
}

int __asan_vsprintf(char *dst, const char *fmt, va_list ap)
{
	va_list ap2;

	va_copy(ap2, ap);
	int needed = vsnprintf(NULL, 0, fmt, ap2);

	va_end(ap2);
	if (needed >= 0) {
		check_write_range((uintptr_t)dst, (size_t)needed + 1);
	}
	return vsprintf(dst, fmt, ap);
}

int __asan_sprintf(char *dst, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	int ret = __asan_vsprintf(dst, fmt, ap);

	va_end(ap);
	return ret;
}

#if defined(CONFIG_SYS_HEAP_KASAN_EXTENSIONS)
char *__asan_stpcpy(char *dst, const char *src)
{
	check_write_range((uintptr_t)dst, strlen(src) + 1);
	return stpcpy(dst, src);
}

char *__asan_stpncpy(char *dst, const char *src, size_t n)
{
	check_write_range((uintptr_t)dst, n);
	return stpncpy(dst, src, n);
}

size_t __asan_strlcpy(char *dst, const char *src, size_t siz)
{
	check_write_range((uintptr_t)dst, siz);
	return strlcpy(dst, src, siz);
}

size_t __asan_strlcat(char *dst, const char *src, size_t siz)
{
	check_write_range((uintptr_t)dst, siz);
	return strlcat(dst, src, siz);
}

/* Conservative check: actual write may be less if c is found early. */
void *__asan_memccpy(void *dst, const void *src, int c, size_t n)
{
	check_write_range((uintptr_t)dst, n);
	return memccpy(dst, src, c, n);
}

void *__asan_mempcpy(void *dst, const void *src, size_t n)
{
	check_write_range((uintptr_t)dst, n);
	__builtin_memcpy(dst, src, n);
	return (uint8_t *)dst + n;
}

char *__asan_fgets(char *buf, int size, FILE *stream)
{
	if (size <= 0) {
		return NULL;
	}
	check_write_range((uintptr_t)buf, (size_t)size);
	return fgets(buf, size, stream);
}
#endif /* CONFIG_SYS_HEAP_KASAN_EXTENSIONS */
