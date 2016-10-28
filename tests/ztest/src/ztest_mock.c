/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ztest.h>
#include <stdint.h>
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

static struct parameter params[CONFIG_ZTEST_PARAMETER_COUNT];
static struct k_fifo *fifo;

static void free_parameter(struct parameter *param)
{
	if (param) {
		k_fifo_put(fifo, param);
	}
}
static struct parameter *alloc_parameter(void)
{
	struct parameter *param;

	param = k_fifo_get(fifo, K_NO_WAIT);
	if (!param) {
		PRINT("Failed to allocate mock parameter\n");
		ztest_test_fail();
	}

	return param;
}

void _init_mock(void)
{
	int i;

	k_fifo_init(fifo);
	for (i = 0; i < CONFIG_ZTEST_PARAMETER_COUNT; i++) {

		k_fifo_put(fifo, &params[i]);
	}
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

	value = find_and_delete_value(param, fn, name);
	if (!value) {
		value = alloc_parameter();
	}

	value->fn = fn;
	value->name = name;
	value->value = val;

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
		      fn, (unsigned long)val,
		      (unsigned long)expected);
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
