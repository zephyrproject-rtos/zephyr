/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>

#include <zephyr/drivers/virtualization/ivshmem.h>

#include <tracing_core.h>
#include <tracing_buffer.h>
#include <tracing_backend.h>

/*
 * Stream the CTF trace into an ivshmem-plain shared memory region so a host
 * tool can read it back after the run (e.g. the twister ivshmem sidecar),
 * which is useful on QEMU targets without semihosting.
 *
 * Layout written at offset 0 of the region (keep in sync with the host):
 *
 *   struct ivshmem_trace_header { magic, version, used, overflow }
 *
 * followed by the raw CTF byte stream. "used" is the total number of bytes
 * written including the header and is updated after every packet; the host
 * reads it once QEMU has exited, so the region is static by then. "overflow"
 * is set if the region filled up and trailing trace data was dropped.
 */

#define IVSHMEM_TRACE_MAGIC   0x4352545aU /* "ZTRC" */
#define IVSHMEM_TRACE_VERSION 1U

struct ivshmem_trace_header {
	uint32_t magic;
	uint32_t version;
	uint32_t used;
	uint32_t overflow;
};

static uint8_t *trace_region;
static size_t trace_size;
static uint32_t trace_pos;
static bool trace_overflow;

static void ivshmem_trace_header_write(uint32_t used, uint32_t overflow)
{
	volatile struct ivshmem_trace_header *hdr =
		(volatile struct ivshmem_trace_header *)trace_region;

	hdr->version = IVSHMEM_TRACE_VERSION;
	hdr->overflow = overflow;
	hdr->used = used;
	/* Write the magic last so the host never keys off a stale header. */
	hdr->magic = IVSHMEM_TRACE_MAGIC;
}

static void tracing_backend_ivshmem_output(const struct tracing_backend *backend,
					   uint8_t *data, uint32_t length)
{
	ARG_UNUSED(backend);

	if (trace_region == NULL || trace_overflow) {
		return;
	}

	if ((trace_pos + length) > trace_size) {
		trace_overflow = true;
		ivshmem_trace_header_write(trace_pos, 1U);
		return;
	}

	memcpy(trace_region + trace_pos, data, length);
	trace_pos += length;
	ivshmem_trace_header_write(trace_pos, 0U);
}

static void tracing_backend_ivshmem_init(void)
{
	const struct device *ivshmem = DEVICE_DT_GET_ONE(qemu_ivshmem);
	uintptr_t base;
	size_t size;

	if (!device_is_ready(ivshmem)) {
		printk("ivshmem device not ready, tracing backend disabled\n");
		return;
	}

	size = ivshmem_get_mem(ivshmem, &base);
	if (size <= sizeof(struct ivshmem_trace_header) || base == 0) {
		printk("no ivshmem region available for tracing\n");
		return;
	}

	trace_region = (uint8_t *)base;
	trace_size = size;
	trace_pos = sizeof(struct ivshmem_trace_header);
	trace_overflow = false;
	ivshmem_trace_header_write(trace_pos, 0U);
}

const struct tracing_backend_api tracing_backend_ivshmem_api = {
	.init = tracing_backend_ivshmem_init,
	.output = tracing_backend_ivshmem_output,
};

TRACING_BACKEND_DEFINE(tracing_backend_ivshmem, tracing_backend_ivshmem_api);
