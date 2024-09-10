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

/* On most arches, panic is entirely macros resulting in some kind of inline assembly.  Create this
 * wrapper so the Rust panic handler can call the same kind of panic.
 */
void rust_panic_wrap(void)
{
	k_panic();
}

#endif
