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

#endif
