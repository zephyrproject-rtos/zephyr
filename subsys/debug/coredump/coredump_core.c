/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <kernel_internal.h>
#include <toolchain.h>
#include <debug/coredump.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include "coredump_internal.h"

#if defined(CONFIG_DEBUG_COREDUMP_BACKEND_LOGGING)
extern struct z_coredump_backend_api z_coredump_backend_logging;
static struct z_coredump_backend_api
	*backend_api = &z_coredump_backend_logging;
#elif defined(DEBUG_COREDUMP_BACKEND_NULL)
extern struct z_coredump_backend_api z_coredump_backend_null;
static struct z_coredump_backend_api
	*backend_api = &z_coredump_backend_null;
#else
#error "Need to select a coredump backend"
#endif

static int error;

static int dump_header(unsigned int reason)
{
	struct z_coredump_hdr_t hdr = {
		.id = {'Z', 'E'},
		.hdr_version = Z_COREDUMP_HDR_VER,
		.reason = sys_cpu_to_le16(reason),
	};

	if (sizeof(uintptr_t) == 8) {
		hdr.ptr_size_bits = 6; /* 2^6 = 64 */
	} else if (sizeof(uintptr_t) == 4) {
		hdr.ptr_size_bits = 5; /* 2^5 = 32 */
	} else {
		hdr.ptr_size_bits = 0; /* Unknown */
	}

	hdr.tgt_code = sys_cpu_to_le16(arch_coredump_tgt_code_get());

	return backend_api->buffer_output((uint8_t *)&hdr, sizeof(hdr));
}

static void dump_thread(struct k_thread *thread)
{
#ifdef CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_MIN
	uintptr_t end_addr;

	/*
	 * When dumping minimum information,
	 * the current thread struct and stack need to
	 * to be dumped so debugger can examine them.
	 */

	if (thread == NULL) {
		return;
	}

	end_addr = POINTER_TO_UINT(thread) + sizeof(*thread);

	z_coredump_memory_dump(POINTER_TO_UINT(thread), end_addr);

	end_addr = thread->stack_info.start + thread->stack_info.size;

	z_coredump_memory_dump(thread->stack_info.start, end_addr);
#endif
}

void process_memory_region_list(void)
{
#ifdef CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_LINKER_RAM
	unsigned int idx = 0;

	while (true) {
		struct z_coredump_memory_region_t *r =
			&z_coredump_memory_regions[idx];

		if (r->end == POINTER_TO_UINT(NULL)) {
			break;
		}

		z_coredump_memory_dump(r->start, r->end);

		idx++;
	}
#endif
}

void z_coredump(unsigned int reason, const z_arch_esf_t *esf,
		struct k_thread *thread)
{
	error = 0;

	z_coredump_start();

	dump_header(reason);

	if (esf != NULL) {
		arch_coredump_info_dump(esf);
	}

	if (thread != NULL) {
		dump_thread(thread);
	}

	process_memory_region_list();

	if (error != 0)	{
		z_coredump_error();
	}

	z_coredump_end();
}

void z_coredump_start(void)
{
	backend_api->start();
}

void z_coredump_end(void)
{
	backend_api->end();
}

void z_coredump_error(void)
{
	backend_api->error();
}

int z_coredump_buffer_output(uint8_t *buf, size_t buflen)
{
	int ret;

	/* Error encountered before, skip */
	if (error != 0) {
		return -EAGAIN;
	}

	if ((buf == NULL) || (buflen == 0)) {
		ret = -EINVAL;
	} else {
		error = backend_api->buffer_output(buf, buflen);
		ret = error;
	}

	return ret;
}

void z_coredump_memory_dump(uintptr_t start_addr, uintptr_t end_addr)
{
	struct z_coredump_mem_hdr_t m;
	size_t len;

	if ((start_addr == POINTER_TO_UINT(NULL)) ||
	    (end_addr == POINTER_TO_UINT(NULL))) {
		return;
	}

	if (start_addr >= end_addr) {
		return;
	}

	len = end_addr - start_addr;

	m.id = Z_COREDUMP_MEM_HDR_ID;
	m.hdr_version = Z_COREDUMP_MEM_HDR_VER;

	if (sizeof(uintptr_t) == 8) {
		m.start	= sys_cpu_to_le64(start_addr);
		m.end = sys_cpu_to_le64(end_addr);
	} else if (sizeof(uintptr_t) == 4) {
		m.start	= sys_cpu_to_le32(start_addr);
		m.end = sys_cpu_to_le32(end_addr);
	}

	z_coredump_buffer_output((uint8_t *)&m, sizeof(m));

	z_coredump_buffer_output((uint8_t *)start_addr, len);
}
