/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "coverage.h"

K_HEAP_DEFINE(gcov_heap, CONFIG_COVERAGE_GCOV_HEAP_SIZE);

static struct gcov_info *gcov_info_head;

/**
 * Is called by gcc-generated constructor code for each object file compiled
 * with -fprofile-arcs.
 */
void __gcov_init(struct gcov_info *info)
{
	info->next = gcov_info_head;
	gcov_info_head = info;
}

void __gcov_merge_add(gcov_type *counters, unsigned int n_counters)
{
	/* Unused. */
}

void __gcov_exit(void)
{
	/* Unused. */
}

/**
 * print_u8 - Print 8 bit of gcov data
 */
static inline void print_u8(uint8_t v)
{
	printk("%02x", v);
}

/**
 * print_u32 - Print 32 bit of gcov data
 */
static inline void print_u32(uint32_t v)
{
	uint8_t *ptr = (uint8_t *)&v;

	print_u8(*ptr);
	print_u8(*(ptr+1));
	print_u8(*(ptr+2));
	print_u8(*(ptr+3));
}

/**
 * write_u64 - Store 64 bit data on a buffer and return the size
 */

static inline void write_u64(void *buffer, size_t *off, uint64_t v)
{
	if (buffer != NULL) {
		memcpy((uint8_t *)buffer + *off, (uint8_t *)&v, sizeof(v));
	} else {
		print_u32(*((uint32_t *)&v));
		print_u32(*(((uint32_t *)&v) + 1));
	}
	*off = *off + sizeof(uint64_t);
}

/**
 * write_u32 - Store 32 bit data on a buffer and return the size
 */
static inline void write_u32(void *buffer, size_t *off, uint32_t v)
{
	if (buffer != NULL) {
		memcpy((uint8_t *)buffer + *off, (uint8_t *)&v, sizeof(v));
	} else {
		print_u32(v);
	}
	*off = *off + sizeof(uint32_t);
}

size_t gcov_calculate_buff_size(struct gcov_info *info)
{
	uint32_t iter;
	uint32_t iter_1;
	uint32_t iter_2;

	/* Few fixed values at the start: magic number,
	 * version, stamp, and checksum.
	 */
#ifdef GCOV_12_FORMAT
	uint32_t size = sizeof(uint32_t) * 4;
#else
	uint32_t size = sizeof(uint32_t) * 3;
#endif

	for (iter = 0U; iter < info->n_functions; iter++) {
		/* space for TAG_FUNCTION and FUNCTION_LENGTH
		 * ident
		 * lineno_checksum
		 * cfg_checksum
		 */
		size += (sizeof(uint32_t) * 5);

		for (iter_1 = 0U; iter_1 < GCOV_COUNTERS; iter_1++) {
			if (!info->merge[iter_1]) {
				continue;
			}

			/*  for function counter and number of values  */
			size += (sizeof(uint32_t) * 2);

			for (iter_2 = 0U;
			     iter_2 < info->functions[iter]->ctrs->num;
			     iter_2++) {

				/* Iter for values which is uint64_t */
				size += (sizeof(uint64_t));
			}

		}


	}

	return size;
}

/**
 * gcov_to_gcda - convert from gcov data set (info) to
 * .gcda file format.
 * This buffer will now have info similar to a regular gcda
 * format.
 */
size_t gcov_to_gcda(uint8_t *buffer, struct gcov_info *info)
{
	struct gcov_fn_info *functions;
	struct gcov_ctr_info *counters_per_func;
	uint32_t iter_functions;
	uint32_t iter_counts;
	uint32_t iter_counter_values;
	size_t buffer_write_position = 0;

	/* File header. */
	write_u32(buffer,
		      &buffer_write_position,
		      GCOV_DATA_MAGIC);

	write_u32(buffer,
		      &buffer_write_position,
		      info->version);

	write_u32(buffer,
		      &buffer_write_position,
		      info->stamp);

#ifdef GCOV_12_FORMAT
	write_u32(buffer,
		      &buffer_write_position,
		      info->checksum);
#endif

	for (iter_functions = 0U;
	     iter_functions < info->n_functions;
	     iter_functions++) {

		functions = info->functions[iter_functions];


		write_u32(buffer,
			      &buffer_write_position,
			      GCOV_TAG_FUNCTION);

		write_u32(buffer,
			      &buffer_write_position,
			      GCOV_TAG_FUNCTION_LENGTH);

		write_u32(buffer,
			      &buffer_write_position,
			      functions->ident);

		write_u32(buffer,
			      &buffer_write_position,
			      functions->lineno_checksum);

		write_u32(buffer,
			      &buffer_write_position,
			      functions->cfg_checksum);

		counters_per_func = functions->ctrs;

		for (iter_counts = 0U;
		     iter_counts < GCOV_COUNTERS;
		     iter_counts++) {

			if (!info->merge[iter_counts]) {
				continue;
			}

			write_u32(buffer,
				      &buffer_write_position,
				      GCOV_TAG_FOR_COUNTER(iter_counts));

#ifdef GCOV_12_FORMAT
			/* GCOV 12 counts the length by bytes */
			write_u32(buffer,
				      &buffer_write_position,
				      counters_per_func->num * 2U * 4);
#else
			write_u32(buffer,
				      &buffer_write_position,
				      counters_per_func->num * 2U);
#endif

			for (iter_counter_values = 0U;
			     iter_counter_values < counters_per_func->num;
			     iter_counter_values++) {

				write_u64(buffer,
					      &buffer_write_position,
					      counters_per_func->values[iter_counter_values]);
			}

			counters_per_func++;
		}
	}
	return buffer_write_position;
}

void dump_on_console_start(const char *filename)
{
	printk("\n%c", FILE_START_INDICATOR);
	while (*filename != '\0') {
		printk("%c", *filename++);
	}
	printk("%c", GCOV_DUMP_SEPARATOR);
}

void dump_on_console_data(char *ptr, size_t len)
{
	if (ptr != NULL) {
		for (size_t iter = 0U; iter < len; iter++) {
			print_u8((uint8_t)*ptr++);
		}
	}
}

/**
 * Retrieves gcov coverage data and sends it over the given interface.
 */
void gcov_coverage_dump(void)
{
	uint8_t *buffer;
	size_t size;
	size_t written_size;
	struct gcov_info *gcov_list_first = gcov_info_head;
	struct gcov_info *gcov_list = gcov_info_head;

	k_sched_lock();
	printk("\nGCOV_COVERAGE_DUMP_START");
	while (gcov_list) {

		dump_on_console_start(gcov_list->filename);
		size = gcov_calculate_buff_size(gcov_list);

		buffer = k_heap_alloc(&gcov_heap, size, K_NO_WAIT);
		if (CONFIG_COVERAGE_GCOV_HEAP_SIZE > 0 && !buffer) {
			printk("No Mem available to continue dump\n");
			goto coverage_dump_end;
		}

		written_size = gcov_to_gcda(buffer, gcov_list);
		if (written_size != size) {
			printk("Write Error on buff\n");
			goto coverage_dump_end;
		}

		dump_on_console_data(buffer, size);

		k_heap_free(&gcov_heap, buffer);
		gcov_list = gcov_list->next;
		if (gcov_list_first == gcov_list) {
			goto coverage_dump_end;
		}
	}
coverage_dump_end:
	printk("\nGCOV_COVERAGE_DUMP_END\n");
	k_sched_unlock();
	return;
}

struct gcov_info *gcov_get_list_head(void)
{
	/* Locking someway before getting this is recommended. */
	return gcov_info_head;
}

/* Initialize the gcov by calling the required constructors */
void gcov_static_init(void)
{
	extern uintptr_t __init_array_start, __init_array_end;
	uintptr_t func_pointer_start = (uintptr_t) &__init_array_start;
	uintptr_t func_pointer_end = (uintptr_t) &__init_array_end;

	while (func_pointer_start < func_pointer_end) {
		void (**p)(void);
		/* get function pointer */
		p = (void (**)(void)) func_pointer_start;
		(*p)();
		func_pointer_start += sizeof(p);
	}
}
