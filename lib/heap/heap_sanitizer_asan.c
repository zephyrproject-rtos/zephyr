/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * AddressSanitizer (ASAN) runtime backend for the sys_heap sanitizer hooks.
 *
 * Implements the heap_sanitizer_on_{init,alloc,free} seam (see heap_sanitizer.h) on top
 * of the toolchain's Address Sanitizer manual poisoning runtime (GCC's libasan
 * or Clang/LLVM's compiler-rt), for use on native simulator based platforms.
 * The whole heap buffer is poisoned at init; an allocation unpoisons exactly
 * the user-requested region and a free re-poisons the usable region. Accesses
 * to freed, never-allocated or out-of-bounds heap memory are therefore
 * reported by ASAN with a full backtrace and shadow dump.
 *
 * The heap implementation itself is compiled with -fno-sanitize=address (see
 * CMakeLists.txt) so it can freely access the chunk headers, free-list links
 * and canaries that live inside the poisoned regions. Because that leaves
 * __SANITIZE_ADDRESS__ undefined in the heap translation units, the
 * convenience macros from <sanitizer/asan_interface.h> would compile away;
 * the runtime entry points (__asan_{,un}poison_memory_region) are therefore
 * called directly. They still update the ASAN shadow because the runtime is
 * linked into the instrumented application.
 *
 * Chunk headers and internal metadata are poisoned once at init and never
 * unpoisoned, so they act as permanent redzones: an overflow from one
 * allocation into a neighbour's header (even a live one) and an underflow
 * into an allocation's own header are both caught. This relies on the
 * big-heap chunk format (enforced via Kconfig) to keep the user pointer and
 * the end of the usable region 8-byte aligned, matching ASAN's shadow
 * granularity.
 */

#include <sanitizer/asan_interface.h>

#include "heap_sanitizer.h"

void heap_sanitizer_on_init(struct sys_heap *heap, void *mem, size_t bytes)
{
	ARG_UNUSED(heap);

	__asan_poison_memory_region(mem, bytes);
}

void heap_sanitizer_on_alloc(struct sys_heap *heap, void *mem, size_t bytes)
{
	ARG_UNUSED(heap);

	/*
	 * Unpoison only the user-requested region. The slack between the
	 * requested size and the usable size stays poisoned, so writing past
	 * the requested size is reported as an overflow.
	 */
	__asan_unpoison_memory_region(mem, bytes);
}

void heap_sanitizer_on_free(struct sys_heap *heap, void *mem, size_t bytes)
{
	ARG_UNUSED(heap);

	__asan_poison_memory_region(mem, bytes);
}
