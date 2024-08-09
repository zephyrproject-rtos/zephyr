/*
 * Copyright (c) 2024 Linaro LTD
 * SPDX-License-Identifier: Apache-2.0
 */

/* This main is brought into the Rust application. */
#include <zephyr/kernel.h>

#ifdef CONFIG_RUST

extern void rust_main(void);

int main(void)
{
	rust_main();
	return 0;
}

#ifdef CONFIG_PRINTK
/*
 * Until we have syscall support in Rust, wrap this syscall.
 */
void wrapped_str_out(char *c, size_t n)
{
	k_str_out(c, n);
}
#endif

#endif
