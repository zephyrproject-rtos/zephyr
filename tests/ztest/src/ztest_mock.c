/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/types.h>
#include <string.h>
#include <stdio.h>

struct parameter {
	struct parameter *next;
	const char *fn;
	const char *name;
	uintptr_t value;
};

#ifndef KERNEL

#include <stdlib.h>

static void free_parameter(struct parameter *param)
{
	free(param);
}

static struct parameter *alloc_parameter(void)
{
	struct parameter *param;

	param = calloc(1, sizeof(struct parameter));
	if (!param) {
		PRINT("Failed to allocate mock parameter\n");
		ztest_test_fail();
	}

	return param;
}

void _init_mock(void)
{

}

#else

/*
 * FIXME: move to sys_io.h once the argument signature for bitmap has
 * been fixed to void* or similar GH-2825
 */
#define BITS_PER_UL (8 * sizeof(unsigned long int))
#define DEFINE_BITFIELD(name, bits)					\
	unsigned long int (name)[((bits) + BITS_PER_UL - 1) / BITS_PER_UL]

static inline
int sys_bitfield_find_first_clear(const unsigned long *bitmap,
				  unsigned int bits)
{
	unsigned int words = (bits + BITS_PER_UL - 1) / BITS_PER_UL;
	unsigned int cnt;
	unsigned int long neg_bitmap;

	/*
	 * By bitwise negating the bitmap, we are actually implemeting
	 * ffc (find first clear) using ffs (find first set).
	 */
	for (cnt = 0; cnt < words; cnt++) {
		neg_bitmap = ~bitmap[cnt];
		if (neg_bitmap == 0)	/* all full */
			continue;
		else if (neg_bitmap == ~0UL)	/* first bit */
			return cnt * BITS_PER_UL;
		else
			return cnt * BITS_PER_UL + __builtin_ffsl(neg_bitmap) - 1;
	}
	return -1;
}

static DEFINE_BITFIELD(params_allocation, CONFIG_ZTEST_PARAMETER_COUNT);
static struct parameter params[CONFIG_ZTEST_PARAMETER_COUNT];

static
void free_parameter(struct parameter *param)
{
	unsigned int allocation_index = param - params;

	if (param == NULL)
		return;
	__ASSERT(allocation_index < CONFIG_ZTEST_PARAMETER_COUNT,
		 "param %p given to free is not in the static buffer %p:%u",
		 param, params, CONFIG_ZTEST_PARAMETER_COUNT);
	sys_bitfield_clear_bit((mem_addr_t) params_allocation,
			       allocation_index);
}

static
struct parameter *alloc_parameter(void)
{
	int allocation_index;
	struct parameter *param;

	allocation_index = sys_bitfield_find_first_clear(
		params_allocation, CONFIG_ZTEST_PARAMETER_COUNT);
	if (allocation_index == -1) {
		printk("No more mock parameters available for allocation\n");
		ztest_test_fail();
	}
	sys_bitfield_set_bit((mem_addr_t) params_allocation, allocation_index);
	param = params + allocation_index;
	(void)memset(param, 0, sizeof(*param));
	return param;
}

void _init_mock(void)
{
}

#endif

static struct parameter *find_and_delete_value(struct parameter *param,
					       const char *fn,
					       const char *name)
{
	struct parameter *value;

	if (!param->next) {
		return NULL;
	}

	if (strcmp(param->next->name, name) || strcmp(param->next->fn, fn)) {
		return find_and_delete_value(param->next, fn, name);
	}

	value = param->next;
	param->next = param->next->next;
	value->next = NULL;

	return value;
}

static void insert_value(struct parameter *param, const char *fn,
			 const char *name, uintptr_t val)
{
	struct parameter *value;

	value = alloc_parameter();
	value->fn = fn;
	value->name = name;
	value->value = val;

	/* Seek to end of linked list to ensure correct discovery order in find_and_delete_value */
	while (param->next) {
		param = param->next;
	}

	/* Append to end of linked list */
	value->next = param->next;
	param->next = value;
}

static struct parameter parameter_list = { NULL, "", "", 0 };
static struct parameter return_value_list = { NULL, "", "", 0 };

void _ztest_expect_value(const char *fn, const char *name, uintptr_t val)
{
	insert_value(&parameter_list, fn, name, val);
}

void _ztest_check_expected_value(const char *fn, const char *name,
				 uintptr_t val)
{
	struct parameter *param;
	uintptr_t expected;

	param = find_and_delete_value(&parameter_list, fn, name);
	if (!param) {
		PRINT("Failed to find parameter %s for %s\n", name, fn);
		ztest_test_fail();
	}

	expected = param->value;
	free_parameter(param);

	if (expected != val) {
		/* We need to cast these values since the toolchain doesn't
		 * provide inttypes.h
		 */
		PRINT("%s received wrong value: Got %lu, expected %lu\n",
		      fn, (unsigned long)val, (unsigned long)expected);
		ztest_test_fail();
	}
}

void  _ztest_returns_value(const char *fn, uintptr_t value)
{
	insert_value(&return_value_list, fn, "", value);
}


uintptr_t _ztest_get_return_value(const char *fn)
{
	uintptr_t value;
	struct parameter *param = find_and_delete_value(&return_value_list,
							fn, "");

	if (!param) {
		PRINT("Failed to find return value for function %s\n", fn);
		ztest_test_fail();
	}

	value = param->value;
	free_parameter(param);

	return value;
}

static void free_param_list(struct parameter *param)
{
	struct parameter *next;

	while (param) {
		next = param->next;
		free_parameter(param);
		param = next;
	}
}

int _cleanup_mock(void)
{
	int fail = 0;

	if (parameter_list.next) {
		fail = 1;
	}
	if (return_value_list.next) {
		fail = 2;
	}

	free_param_list(parameter_list.next);
	free_param_list(return_value_list.next);

	parameter_list.next = NULL;
	return_value_list.next = NULL;

	return fail;
}
