/*
 * Copyright (c) 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bindesc.h>

BINDESC_STR_DEFINE(my_string, 1, "Hello world!");
BINDESC_UINT_DEFINE(my_int, 2, 5);
BINDESC_BYTES_DEFINE(my_bytes, 3, ({1, 2, 3, 4}));

int main(void)
{
	size_t i;

	/* Builtin descriptors */
	printk("Zephyr version: %s\n", BINDESC_GET_STR(kernel_version_string));
	printk("App version: %s\n", BINDESC_GET_STR(app_version_string));
	printk("Build time: %s\n", BINDESC_GET_STR(build_date_time_string));
	printk("Compiler: %s %s\n", BINDESC_GET_STR(c_compiler_name),
		BINDESC_GET_STR(c_compiler_version));

	/* Custom descriptors */
	printk("my_string: %s\n", BINDESC_GET_STR(my_string));
	printk("my_int: %d\n", BINDESC_GET_UINT(my_int));
	printk("my_bytes: ");
	for (i = 0; i < BINDESC_GET_SIZE(my_bytes); i++) {
		printk("%02x ", BINDESC_GET_BYTES(my_bytes)[i]);
	}
	printk("\n");

	return 0;
}
