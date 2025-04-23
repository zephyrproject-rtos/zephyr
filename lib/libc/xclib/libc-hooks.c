/*
 * Copyright (c) 2025 Cadence Design Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

char __os_flag __attribute__((retain)); /* Needed by xclib to indicate the presense of an OS */

int arch_printk_char_out(int c);
static int (*_stdout_hook)(int c) = arch_printk_char_out;

void __stdout_hook_install(int (*hook)(int))
{
	_stdout_hook = hook;
}
