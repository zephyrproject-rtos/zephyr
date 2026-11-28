/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * gcov runtime for code built with clang --coverage.
 *
 * Clang does not use the GCC gcov_info structures handled by coverage.c.
 * Instead, every instrumented object file registers a writeout and a reset
 * function through llvm_gcov_init(). The writeout function describes the
 * object's .gcda contents by calling the llvm_gcda_*() hooks below, which
 * compiler-rt implements on hosted targets (GCDAProfiling.c). This file
 * produces the same records as compiler-rt and emits each .gcda image with
 * the console or semihost transport of the GCC runtime, so the host side
 * (twister, gcovr, llvm-cov gcov) does not need to change.
 *
 * The hook signatures are the ones used since LLVM 12. Each writeout is
 * expected to describe a single object, which does not hold when objects
 * are merged, for example with LTO.
 */

#include <zephyr/kernel.h>
#include <zephyr/debug/gcov.h>
#include <zephyr/posix/fnmatch.h>
#include <zephyr/arch/common/semihost.h>
#include <zephyr/sys/atomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "coverage.h"

typedef void (*llvm_gcov_fn)(void);

#ifdef CONFIG_COVERAGE_SEMIHOST
/* Each .gcda image is built in this heap before it is written to the host. */
BUILD_ASSERT(CONFIG_COVERAGE_GCOV_HEAP_SIZE > 0,
	     "The semihost dump needs CONFIG_COVERAGE_GCOV_HEAP_SIZE > 0 with clang");
K_HEAP_DEFINE(gcov_heap, CONFIG_COVERAGE_GCOV_HEAP_SIZE);
#endif

static llvm_gcov_fn writeout_fns[CONFIG_COVERAGE_GCOV_LLVM_MAX_OBJECTS];
static llvm_gcov_fn reset_fns[CONFIG_COVERAGE_GCOV_LLVM_MAX_OBJECTS];
static size_t n_objects;
static size_t n_dropped;

/* Set while a dump is serializing; the hooks below share one state. */
static atomic_t dump_busy;

/* What the llvm_gcda_*() hooks do with the data of the current writeout. */
enum wr_mode {
	WR_MEASURE, /* only count the bytes */
	WR_FILL,    /* copy the bytes into wr.buf */
	WR_PRINT,   /* print the bytes as hex on the console */
};

/* State of the writeout currently being serialized. */
static struct {
	enum wr_mode mode;
	uint8_t *buf;
	size_t len;
	const char *filename; /* .gcda path recorded by the compiler */
	uint32_t gcov_version;
	uint32_t n_functions; /* instrumented functions in the object */
	bool has_data;        /* any non-zero arc counter */
} wr;

#if defined(CONFIG_COVERAGE_SEMIHOST) && defined(CONFIG_REBOOT)
#define GCOV_FILENAME_KEY 0x23A5B6C4
static char gcov_filename[512];
static __noinit uint32_t gcov_filename_key;
static __noinit uint32_t gcov_filename_idx;
#endif

static void put_u32(uint32_t v)
{
	const uint8_t *bytes = (const uint8_t *)&v;

	switch (wr.mode) {
	case WR_FILL:
		memcpy(wr.buf + wr.len, bytes, sizeof(v));
		break;
	case WR_PRINT:
		for (size_t i = 0; i < sizeof(v); i++) {
			printk("%02x", bytes[i]);
		}
		break;
	default:
		break;
	}
	wr.len += sizeof(v);
}

/* gcov stores 64-bit values as the low word followed by the high word. */
static void put_u64(uint64_t v)
{
	put_u32((uint32_t)v);
	put_u32((uint32_t)(v >> 32));
}

/* Called once per object from its constructor. */
void llvm_gcov_init(llvm_gcov_fn writeout, llvm_gcov_fn reset)
{
	if (n_objects < ARRAY_SIZE(writeout_fns)) {
		writeout_fns[n_objects] = writeout;
		reset_fns[n_objects] = reset;
		n_objects++;
	} else {
		n_dropped++;
	}
}

void llvm_gcda_start_file(const char *orig_filename, uint32_t version, uint32_t checksum)
{
	uint32_t c3 = (version >> 24) & 0xffU;
	uint32_t c2 = (version >> 16) & 0xffU;
	uint32_t c1 = (version >> 8) & 0xffU;
	const uint32_t ch_0 = (uint32_t)'0';
	const uint32_t ch_a = (uint32_t)'A';

	/* Same decoding as compiler-rt: "408*" is 48, "B01*" is 110. */
	wr.gcov_version = (c3 >= ch_a) ? ((c3 - ch_a) * 100U + (c2 - ch_0) * 10U + (c1 - ch_0))
				       : ((c3 - ch_0) * 10U + (c1 - ch_0));
	wr.filename = orig_filename;
	wr.len = 0;
	wr.n_functions = 0;
	wr.has_data = false;

	put_u32(GCOV_DATA_MAGIC);
	put_u32(version);
	put_u32(checksum);
}

void llvm_gcda_emit_function(uint32_t ident, uint32_t func_checksum, uint32_t cfg_checksum)
{
	bool extra_checksum = wr.gcov_version >= 47U;

	wr.n_functions++;
	put_u32(GCOV_TAG_FUNCTION);
	put_u32(extra_checksum ? 3U : 2U);
	put_u32(ident);
	put_u32(func_checksum);
	if (extra_checksum) {
		put_u32(cfg_checksum);
	}
}

void llvm_gcda_emit_arcs(uint32_t num_counters, uint64_t *counters)
{
	put_u32(GCOV_TAG_COUNTER_BASE);
	put_u32(num_counters * 2U);
	for (uint32_t i = 0; i < num_counters; i++) {
		put_u64(counters[i]);
		if (counters[i] != 0U) {
			wr.has_data = true;
		}
	}
}

void llvm_gcda_summary_info(void)
{
	const uint32_t runs = 1U;

	if (wr.gcov_version >= 90U) {
		put_u32(GCOV_TAG_OBJECT_SUMMARY);
		put_u32(2U);
		put_u32(runs);
		put_u32(0U); /* sum_max */
	} else {
		put_u32(GCOV_TAG_PROGRAM_SUMMARY);
		put_u32(3U);
		put_u32(0U);
		put_u32(0U);
		put_u32(runs);
	}
}

void llvm_gcda_end_file(void)
{
	/* EOF record: zero tag and zero length. */
	put_u32(0U);
	put_u32(0U);
}

/* Run the writeout function of object @p idx with the given mode. */
static void run_writeout(size_t idx, enum wr_mode mode, uint8_t *buf)
{
	wr.mode = mode;
	wr.buf = buf;
	writeout_fns[idx]();
	wr.mode = WR_MEASURE;
	wr.buf = NULL;
}

/*
 * Measure object @p idx and decide whether it is part of this dump. On return
 * wr.filename and wr.len describe the object.
 */
static bool should_dump(size_t idx, const char *tag)
{
	run_writeout(idx, WR_MEASURE, NULL);

	/*
	 * Objects excluded by -fprofile-filter-files still register a writeout
	 * function, but it describes no functions. Skip them.
	 */
	if (wr.n_functions == 0U) {
		return false;
	}

	/* For per-test (tagged) dumps skip objects with no coverage. */
	if ((tag != NULL) && !wr.has_data) {
		return false;
	}

	if ((strlen(CONFIG_COVERAGE_DUMP_PATH_EXCLUDE) > 0) &&
	    (fnmatch(CONFIG_COVERAGE_DUMP_PATH_EXCLUDE, wr.filename, 0) == 0)) {
		return false;
	}

	return true;
}

/*
 * Take the dump state. A dump started while another one is serializing, for
 * example from a fault handler, is refused instead of corrupting it.
 */
static bool dump_begin(void)
{
	if (!atomic_cas(&dump_busy, 0, 1)) {
		printk("\ngcov: dump already in progress\n");
		return false;
	}

#ifdef CONFIG_MULTITHREADING
	if (!k_is_in_isr()) {
		k_sched_lock();
	}
#endif

	if (n_dropped > 0U) {
		printk("\ngcov: %zu objects not registered, increase "
		       "CONFIG_COVERAGE_GCOV_LLVM_MAX_OBJECTS\n",
		       n_dropped);
	}

	return true;
}

static void dump_end(void)
{
#ifdef CONFIG_MULTITHREADING
	if (!k_is_in_isr()) {
		k_sched_unlock();
	}
#endif

	atomic_clear(&dump_busy);
}

#ifdef CONFIG_COVERAGE_DUMP
void gcov_coverage_dump_tagged(const char *tag)
{
	if (!dump_begin()) {
		return;
	}

	printk("\nGCOV_COVERAGE_DUMP_START");
	if (tag != NULL) {
		printk(" %s", tag);
	}

	for (size_t i = 0; i < n_objects; i++) {
		if (!should_dump(i, tag)) {
			continue;
		}

		/* The data is printed while it is serialized, no buffer needed. */
		printk("\n%c%s%c", FILE_START_INDICATOR, wr.filename, GCOV_DUMP_SEPARATOR);
		run_writeout(i, WR_PRINT, NULL);
	}

	printk("\nGCOV_COVERAGE_DUMP_END\n");

	dump_end();
}

void gcov_coverage_dump(void)
{
	gcov_coverage_dump_tagged(NULL);
}

#elif defined(CONFIG_COVERAGE_SEMIHOST)
void gcov_coverage_dump_tagged(const char *tag)
{
	if (!dump_begin()) {
		return;
	}

#if defined(CONFIG_REBOOT)
	/* See coverage.c: the reboot index only uniquifies untagged dumps. */
	if (tag == NULL) {
		if (gcov_filename_key != GCOV_FILENAME_KEY) {
			gcov_filename_key = GCOV_FILENAME_KEY;
			gcov_filename_idx = 0;
		} else {
			gcov_filename_idx += 1;
		}
	}
#endif

	for (size_t i = 0; i < n_objects; i++) {
		const char *filename;
		char *tagged_path = NULL;
		uint8_t *buffer;
		size_t size;
		int fd;
		long ret;

		if (!should_dump(i, tag)) {
			continue;
		}
		filename = wr.filename;
		size = wr.len;

#if defined(CONFIG_REBOOT)
		if (tag == NULL) {
			snprintk(gcov_filename, sizeof(gcov_filename), "%s.%u", filename,
				 gcov_filename_idx);
			filename = gcov_filename;
		}
#endif

		if (tag != NULL) {
			size_t path_size = strlen(filename) + strlen(tag) + sizeof("@@");

			tagged_path = k_heap_alloc(&gcov_heap, path_size, K_NO_WAIT);
			if (tagged_path == NULL) {
				printk("No memory available for tagged path\n");
				break;
			}
			snprintk(tagged_path, path_size, "%s@@%s", filename, tag);
			filename = tagged_path;
		}

		buffer = k_heap_alloc(&gcov_heap, size, K_NO_WAIT);
		if (buffer == NULL) {
			printk("No memory available to continue dump\n");
			k_heap_free(&gcov_heap, tagged_path);
			break;
		}

		run_writeout(i, WR_FILL, buffer);
		if (wr.len != size) {
			printk("Write Error on buffer\n");
			k_heap_free(&gcov_heap, buffer);
			k_heap_free(&gcov_heap, tagged_path);
			break;
		}

		fd = semihost_open(filename, SEMIHOST_OPEN_WB);
		if (fd < 0) {
			printk("Failed to open file: %s\n", filename);
			k_heap_free(&gcov_heap, buffer);
			k_heap_free(&gcov_heap, tagged_path);
			break;
		}

		/* SYS_WRITE returns the number of bytes not written. */
		ret = semihost_write(fd, buffer, size);
		semihost_close(fd);
		if (ret != 0) {
			printk("Failed to write data to file: %s\n", filename);
		}

		k_heap_free(&gcov_heap, buffer);
		k_heap_free(&gcov_heap, tagged_path);
		if (ret != 0) {
			break;
		}
	}

	dump_end();
}

void gcov_coverage_semihost(void)
{
	gcov_coverage_dump_tagged(NULL);
}
#endif /* CONFIG_COVERAGE_DUMP */

void gcov_reset_all_counts(void)
{
#ifdef CONFIG_MULTITHREADING
	k_sched_lock();
#endif

	for (size_t i = 0; i < n_objects; i++) {
		reset_fns[i]();
	}

#ifdef CONFIG_MULTITHREADING
	k_sched_unlock();
#endif
}

/*
 * Objects register themselves from GNU-style constructors, which the kernel
 * runs because COVERAGE_GCOV_LLVM selects STATIC_INIT_GNU. Running all
 * constructors here as well would run C++ static constructors twice.
 */
void gcov_static_init(void)
{
}
