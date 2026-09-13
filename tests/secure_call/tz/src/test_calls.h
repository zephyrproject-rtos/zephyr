/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_SECURE_CALL_TZ_TEST_CALLS_H_
#define TESTS_SECURE_CALL_TZ_TEST_CALLS_H_

#include <stdint.h>
#include <zephyr/toolchain.h>

__secure_call int sc_add(int a, int b);
__secure_call void sc_fill(uint8_t *buf, size_t len);
__secure_call int sc_nop(void);

/* Generated NS-side inline wrappers */
#include <zephyr/secure_calls/test_calls.h>

#endif /* TESTS_SECURE_CALL_TZ_TEST_CALLS_H_ */
