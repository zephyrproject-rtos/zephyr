#pragma once

/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall.h>

/* Newline between return type and function name */
/* clang-format off */
__syscall void
test_wrapped(int foo);
/* clang-format on */

#include <zephyr/syscalls/main.h>
