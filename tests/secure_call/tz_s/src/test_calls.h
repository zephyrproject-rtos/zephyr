/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Scanned by parse_secure_calls.py (via APPLICATION_DEFINED_SYSCALL) so that
 * the S-side z_secure_mrsh_* entry functions are generated and compiled in.
 */

#ifndef TESTS_SECURE_CALL_TZ_S_TEST_CALLS_H_
#define TESTS_SECURE_CALL_TZ_S_TEST_CALLS_H_

#include <stdint.h>
#include <zephyr/toolchain.h>

__secure_call int sc_add(int a, int b);
__secure_call void sc_fill(uint8_t *buf, size_t len);
__secure_call int sc_nop(void);

#endif /* TESTS_SECURE_CALL_TZ_S_TEST_CALLS_H_ */
