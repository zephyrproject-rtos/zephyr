/*
 * Copyright (c) 2024 Linaro LTD
 * SPDX-License-Identifier: Apache-2.0
 */

/* This main is brought into the Rust application. */

#ifdef CONFIG_RUST

extern void rust_main();

int main(void)
{
	rust_main();
	return 0;
}

#endif
