/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <stdio.h>

struct parameter {
	sys_snode_t node;
	const char *fn;
	const char *name;
	uintptr_t value;
};

#ifndef KERNEL

#include <stdlib.h>
#include <stdarg.h>

static void free_parameter(struct parameter *param)
{
	free(param);
}

static struct parameter *alloc_parameter(void)
{
	struct parameter *param;

	param = calloc(1, sizeof(struct parameter));
	if (!param) {
		PRINT_DATA("Failed to allocate mock parameter\n");
		ztest_test_fail();
	}

	return param;
}

void z_init_mock(void)
{
}

void printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void vprintk(const char *fmt, va_list ap)
{
	vprintf(fmt, ap);
}

int snprintk(char *str, size_t size, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vsnprintf(str, size, fmt, ap);
	va_end(ap);

	return ret;
}

#else

/*
 * FIXME: move to sys_io.h once the argument signature for bitmap has
 * been fixed to void* or similar GH-2825
 */
#define DEFINE_BITFIELD(name, bits) unsigned long(name)[DIV_ROUND_UP(bits, BITS_PER_LONG)]

static inline int sys_bitfield_find_first_clear(const unsigned long *bitmap,
						const unsigned int bits)
{
	const size_t words = DIV_ROUND_UP(bits, BITS_PER_LONG);
	size_t cnt;
	unsigned int long neg_bitmap;

	/*
	 * By bitwise negating the bitmap, we are actually implementing
	 * ffc (find first clear) using ffs (find first set).
	 */
	for (cnt = 0; cnt < words; cnt++) {
		neg_bitmap = ~bitmap[cnt];
		if (neg_bitmap == 0) {
			/* All full. Try next word. */
			continue;
		} else if (neg_bitmap == ~0UL) {
			/* First bit is free */
			return cnt * BITS_PER_LONG;
		} else {
			const unsigned int bit =
				(cnt * BITS_PER_LONG) + __builtin_ffsl(neg_bitmap) - 1;
			/* Ensure first free bit is within total bits count */
			if (bit < bits) {
				return bit;
			} else {
				return -1;
			}
		}
	}
	return -1;
}

static DEFINE_BITFIELD(params_allocation, CONFIG_ZTEST_PARAMETER_COUNT);
static struct parameter params[CONFIG_ZTEST_PARAMETER_COUNT];

static void free_parameter(struct parameter *param)
{
	unsigned int allocation_index = param - params;

	if (param == NULL) {
		return;
	}
	__ASSERT(allocation_index < CONFIG_ZTEST_PARAMETER_COUNT,
		 "param %p given to free is not in the static buffer %p:%u", param, params,
		 CONFIG_ZTEST_PARAMETER_COUNT);
	sys_bitfield_clear_bit((mem_addr_t)params_allocation, allocation_index);
}

static struct parameter *alloc_parameter(void)
{
	int allocation_index;
	struct parameter *param;

	allocation_index =
		sys_bitfield_find_first_clear(params_allocation, CONFIG_ZTEST_PARAMETER_COUNT);
	if (allocation_index == -1) {
		printk("No more mock parameters available for allocation\n");
		ztest_test_fail();
	}
	sys_bitfield_set_bit((mem_addr_t)params_allocation, allocation_index);
	param = params + allocation_index;
	(void)memset(param, 0, sizeof(*param));
	return param;
}

void z_init_mock(void)
{
}

#endif

static struct parameter *find_and_delete_value(sys_slist_t *list, const char *fn, const char *name)
{
	struct parameter *param;
	sys_snode_t *prev = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(list, param, node) {
		if (!strcmp(param->name, name) && !strcmp(param->fn, fn)) {
			sys_slist_remove(list, prev, &param->node);
			return param;
		}
		prev = &param->node;
	}
	return NULL;
}

static void insert_value(sys_slist_t *list, const char *fn, const char *name, uintptr_t val)
{
	struct parameter *value = alloc_parameter();

	value->fn = fn;
	value->name = name;
	value->value = val;
	sys_slist_append(list, &value->node);
}

static sys_slist_t parameter_list = SYS_SLIST_STATIC_INIT(&parameter_list);
static sys_slist_t return_value_list = SYS_SLIST_STATIC_INIT(&return_value_list);

void z_ztest_expect_value(const char *fn, const char *name, uintptr_t val)
{
	insert_value(&parameter_list, fn, name, val);
}

void z_ztest_check_expected_value(const char *fn, const char *name, uintptr_t val)
{
	struct parameter *param;
	uintptr_t expected;

	param = find_and_delete_value(&parameter_list, fn, name);
	if (!param) {
		PRINT_DATA("Failed to find parameter %s for %s\n", name, fn);
		ztest_test_fail();
	}

	expected = param->value;
	free_parameter(param);

	if (expected != val) {
		/* We need to cast these values since the toolchain doesn't
		 * provide inttypes.h
		 */
		PRINT_DATA("%s:%s received wrong value: Got %lu, expected %lu\n", fn, name,
			   (unsigned long)val, (unsigned long)expected);
		ztest_test_fail();
	}
}

void z_ztest_expect_data(const char *fn, const char *name, void *val)
{
	insert_value(&parameter_list, fn, name, (uintptr_t)val);
}

void z_ztest_check_expected_data(const char *fn, const char *name, void *data, uint32_t length)
{
	struct parameter *param;
	void *expected;

	param = find_and_delete_value(&parameter_list, fn, name);
	if (!param) {
		PRINT_DATA("Failed to find parameter %s for %s\n", name, fn);
		/* No return from this function but for coverity reasons
		 * put a return after to avoid the warning of a null
		 * dereference of param below.
		 */
		ztest_test_fail();
		return;
	}

	expected = (void *)param->value;
	free_parameter(param);

	if (expected == NULL && data != NULL) {
		PRINT_DATA("%s:%s received data while expected null pointer\n", fn, name);
		ztest_test_fail();
	} else if (data == NULL && expected != NULL) {
		PRINT_DATA("%s:%s received null pointer while expected data\n", fn, name);
		ztest_test_fail();
	} else if (data != NULL) {
		if (memcmp(data, expected, length) != 0) {
			PRINT_DATA("%s:%s data provided doesn't match\n", fn, name);
			ztest_test_fail();
		}
	}
}

void z_ztest_return_data(const char *fn, const char *name, void *val)
{
	insert_value(&parameter_list, fn, name, (uintptr_t)val);
}

void z_ztest_copy_return_data(const char *fn, const char *name, void *data, uint32_t length)
{
	struct parameter *param;
	void *return_data;

	if (data == NULL) {
		PRINT_DATA("%s:%s received null pointer\n", fn, name);
		ztest_test_fail();
		return;
	}

	param = find_and_delete_value(&parameter_list, fn, name);
	if (!param) {
		PRINT_DATA("Failed to find parameter %s for %s\n", name, fn);
		memset(data, 0, length);
		ztest_test_fail();
	} else {
		return_data = (void *)param->value;
		free_parameter(param);
		memcpy(data, return_data, length);
	}
}

void z_ztest_returns_value(const char *fn, uintptr_t value)
{
	insert_value(&return_value_list, fn, "", value);
}

uintptr_t z_ztest_get_return_value(const char *fn)
{
	uintptr_t value;
	struct parameter *param = find_and_delete_value(&return_value_list, fn, "");

	if (!param) {
		PRINT_DATA("Failed to find return value for function %s\n", fn);
		ztest_test_fail();
	}

	value = param->value;
	free_parameter(param);

	return value;
}

static void free_param_list(sys_slist_t *list)
{
	sys_snode_t *node;

	while ((node = sys_slist_get(list)) != NULL) {
		free_parameter(CONTAINER_OF(node, struct parameter, node));
	}
}

int z_cleanup_mock(void)
{
	int fail = 0;
	sys_snode_t *head;

	head = sys_slist_peek_head(&parameter_list);
	if (head != NULL) {
		struct parameter *first = CONTAINER_OF(head, struct parameter, node);

		PRINT_DATA("Parameter not used by mock: %s:%s\n", first->fn, first->name);
		fail = 1;
	}
	head = sys_slist_peek_head(&return_value_list);
	if (head != NULL) {
		struct parameter *first = CONTAINER_OF(head, struct parameter, node);

		PRINT_DATA("Return value not used by mock: %s\n", first->fn);
		fail = 2;
	}

	free_param_list(&parameter_list);
	free_param_list(&return_value_list);

	return fail;
}
