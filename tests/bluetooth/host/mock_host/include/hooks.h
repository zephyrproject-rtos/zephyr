/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Include declarations and defines related to hooks
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

/** Validate the timeout integer value passed to net_buf_alloc_fixed()
 *
 *  A hook function that will be called from net_buf_alloc_fixed() with
 *  the timeout integer value as an input parameter.
 *
 *  This hook is used to validate the timeout value.
 *  It will check the passed input parameter to check its value if it
 *  matches the expected value using ztest_check_expected_value().
 *
 *  A typical use should call ztest_expect_value() to set the expected
 *  value before calling any function that will call net_buf_alloc_fixed().
 *
 *  @param value Input integer value to be checked
 */
void hooks_net_buf_alloc_fixed_timeout_validation_hook(uint32_t value);
