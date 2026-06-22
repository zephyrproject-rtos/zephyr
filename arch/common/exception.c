/*
 * Copyright (c) 2026 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/exception.h>

arch_exception_dump_hook_t arch_exception_dump_hook;
arch_exception_drain_hook_t arch_exception_drain_hook;
