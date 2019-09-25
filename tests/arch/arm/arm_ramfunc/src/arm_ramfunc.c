/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <linker/linker-defs.h>
#include <syscall_handler.h>

static volatile int test_flag;

__ramfunc static void arm_ram_function(void)
{
	test_flag = 1;
}

void test_arm_ramfunc(void)
{
	int init_flag, post_flag;

	init_flag = test_flag;
	zassert_true(init_flag == 0, "Test flag not initialized to zero");

	/* Verify that the .ramfunc section is not empty, it is located
	 * inside SRAM, and that arm_ram_function(.) is located inside
	 * the .ramfunc section.
	 */
	zassert_true((u32_t)&_ramfunc_ram_size != 0,
		".ramfunc linker section is empty");
	zassert_true(((u32_t)&_ramfunc_ram_start >= (u32_t)&_image_ram_start)
			&& ((u32_t)&_ramfunc_ram_end < (u32_t)&_image_ram_end),
			".ramfunc linker section not in RAM");
	zassert_true(
		(((u32_t)&_ramfunc_ram_start) <= (u32_t)arm_ram_function) &&
		(((u32_t)&_ramfunc_ram_end) > (u32_t)arm_ram_function),
		"arm_ram_function not loaded into .ramfunc");

	/* If we build with User Mode support, verify that the
	 * arm_ram_function(.) is user (read) accessible.
	 */
#if defined(CONFIG_USERSPACE)
	zassert_true(z_arch_buffer_validate((void *)&_ramfunc_ram_start,
			(size_t)&_ramfunc_ram_size, 0) == 0 /* Success */,
		".ramfunc section not user accessible");
#endif /* CONFIG_USERSPACE */

	/* Execute the function from SRAM. */
	arm_ram_function();

	/* Verify that the function is executed successfully. */
	post_flag = test_flag;
	zassert_true(post_flag == 1,
		"arm_ram_function() execution failed.");
}
/**
 * @}
 */
