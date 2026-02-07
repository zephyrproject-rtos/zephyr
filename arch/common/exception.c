/*
 * Copyright (c) 2026 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/exception.h>

exception_dump_hook_t exception_dump_hook;
exception_drain_hook_t exception_drain_hook;
