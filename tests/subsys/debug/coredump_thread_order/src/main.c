/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/debug/coredump.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	if (reason != K_ERR_KERNEL_OOPS) {
		k_fatal_halt(reason);
	}
}

static void crash_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_oops();
}

static uintptr_t decode_address(uintptr_t address)
{
	if (sizeof(address) == sizeof(uint64_t)) {
		return sys_le64_to_cpu(address);
	}

	return sys_le32_to_cpu(address);
}

/** @brief The crashing thread precedes other threads, even in a truncated dump. */
ZTEST(coredump_thread_order, test_crashing_thread_first)
{
	static K_THREAD_STACK_DEFINE(crashing_stack, 2048);
	static K_THREAD_STACK_DEFINE(later_stack, 4096);

	static struct k_thread crashing_thread;
	static struct k_thread later_thread;
	uint8_t dump[sizeof(struct coredump_mem_hdr_t)];
	struct coredump_cmd_copy_arg copy = {
		.buffer = dump,
	};
	size_t offset = sizeof(struct coredump_hdr_t);
	uint32_t crashing_count = 0;
	uint32_t later_count = 0;
	uint32_t memory_count = 0;
	bool stack_found = false;
	int32_t size;

	k_thread_create(&crashing_thread, crashing_stack, K_THREAD_STACK_SIZEOF(crashing_stack),
			crash_entry, NULL, NULL, NULL, 0, 0, K_FOREVER);
	/* A newer thread is ahead of the crashing thread in the monitored list. */
	k_thread_create(&later_thread, later_stack, K_THREAD_STACK_SIZEOF(later_stack), crash_entry,
			NULL, NULL, NULL, 0, 0, K_FOREVER);
	k_thread_start(&crashing_thread);
	zassert_ok(k_thread_join(&crashing_thread, K_FOREVER));
	k_thread_abort(&later_thread);

	size = coredump_query(COREDUMP_QUERY_GET_STORED_DUMP_SIZE, NULL);
	zassert_true(size > 0, "No stored coredump");
	zassert_true(size <= CONFIG_DEBUG_COREDUMP_BACKEND_IN_MEMORY_SIZE);

	while (offset < size) {
		size_t header_size;
		size_t payload_size;

		copy.offset = offset;
		copy.length = MIN(sizeof(dump), size - offset);
		zassert_ok(coredump_cmd(COREDUMP_CMD_COPY_STORED_DUMP, &copy));

		if (dump[0] == COREDUMP_MEM_HDR_ID) {
			struct coredump_mem_hdr_t header;
			uintptr_t start;
			uintptr_t end;

			if (copy.length < sizeof(header)) {
				break;
			}
			memcpy(&header, dump, sizeof(header));
			start = decode_address(header.start);
			end = decode_address(header.end);
			zassert_true(end > start);
			header_size = sizeof(header);
			payload_size = end - start;

			if (memory_count == 0U) {
				zassert_equal(start, (uintptr_t)&crashing_thread,
					      "First memory block is not the crashing thread");
			}
			if (memory_count == 1U) {
				zassert_equal(start, crashing_thread.stack_info.start);
				zassert_equal(payload_size, crashing_thread.stack_info.size);
				zassert_true(payload_size <= size - offset - header_size,
					     "Crashing stack was truncated");
				stack_found = true;
			}
			crashing_count += start == (uintptr_t)&crashing_thread;
			later_count += start == (uintptr_t)&later_thread;
			memory_count++;
		} else {
			struct coredump_arch_hdr_t header;

			zassert_true(dump[0] == COREDUMP_ARCH_HDR_ID ||
				     dump[0] == THREADS_META_HDR_ID ||
				     dump[0] == COREDUMP_CPU_SNAPSHOT_HDR_ID);
			zassert_true(copy.length >= sizeof(header));
			memcpy(&header, dump, sizeof(header));
			header_size = dump[0] == COREDUMP_CPU_SNAPSHOT_HDR_ID
					      ? sizeof(struct coredump_cpu_snapshot_hdr_t)
					      : sizeof(header);
			payload_size = sys_le16_to_cpu(header.num_bytes);
		}

		if (header_size > size - offset || payload_size > size - offset - header_size) {
			break;
		}
		offset += header_size + payload_size;
	}

	zassert_true(stack_found);
	zassert_equal(crashing_count, 1U, "Crashing thread dumped more than once");
	if (CONFIG_DEBUG_COREDUMP_BACKEND_IN_MEMORY_SIZE == 4096) {
		zassert_equal(size, CONFIG_DEBUG_COREDUMP_BACKEND_IN_MEMORY_SIZE,
			      "Dump did not fill the backend");
	} else {
		zassert_equal(offset, size, "Unexpectedly truncated dump");
		zassert_equal(later_count, 1U, "Other thread omitted or duplicated");
	}
}

ZTEST_SUITE(coredump_thread_order, NULL, NULL, NULL, NULL, NULL);
