// Copyright (c) 2024 Zühlke Engineering AG
// SPDX-License-Identifier: Apache-2.0

#include <zephyr/kernel.h>

// Provide a symbol to call from Rust
void rust_panic()
{
    k_panic();
}
