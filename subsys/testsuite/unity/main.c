/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 **/

#include <zephyr/sys/printk.h>
#include <unity.h>
#ifdef CONFIG_BOARD_NATIVE_POSIX
#include "posix_board_if.h"
#include "cmdline.h"
#endif

void main(void)
{
#ifdef CONFIG_BOARD_NATIVE_POSIX
	printk("Parsing command line arguments\n");
	int argc;
	char **argv;

	native_get_test_cmd_line_args(&argc, &argv);
	posix_exit(unity_main(argc, argv));
#else
	printk("Ignoring command line arguments\n");
	unity_main(0, NULL);
#endif
}
