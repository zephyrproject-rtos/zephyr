/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef USER_DOMAIN_COMMON_H
#define USER_DOMAIN_COMMON_H

#include <stdint.h>

/* Set by a suite before provoking a fault that must be fatal. */
extern volatile bool expect_fault;

#if defined(CONFIG_ARM64)
extern volatile unsigned int last_fault_reason;
extern volatile uint64_t last_fault_spsr;
extern volatile uint64_t last_fault_far;
#endif

#endif
