/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file test_calls.h
 * @brief Sample __secure_call declarations used by the secure_call test suite.
 *
 * This header is scanned by parse_secure_calls.py (via APPLICATION_DEFINED_SYSCALL)
 * to generate the NS-side wrappers in <zephyr/secure_calls/test_calls.h>.
 *
 * The three declarations exercise the three shapes the generator must handle:
 *   sc_add  — non-void return, all-scalar args
 *   sc_fill — void return, pointer + scalar args
 *   sc_nop  — zero args, non-void return
 */

#ifndef TESTS_SECURE_CALL_TEST_CALLS_H_
#define TESTS_SECURE_CALL_TEST_CALLS_H_

#include <stdint.h>
#include <zephyr/toolchain.h>

/** Add two integers in the secure world. */
__secure_call int sc_add(int a, int b);

/** Fill a buffer in the secure world. */
__secure_call void sc_fill(uint8_t *buf, size_t len);

/** Zero-argument secure call returning an int. */
__secure_call int sc_nop(void);

/* Include the generated NS-side inline wrappers (mirrors the __syscall pattern). */
#include <zephyr/secure_calls/test_calls.h>

#endif /* TESTS_SECURE_CALL_TEST_CALLS_H_ */
