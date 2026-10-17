/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_SUBSYS_LORAWAN_NATIVE_TEST_COMMON_H_
#define TESTS_SUBSYS_LORAWAN_NATIVE_TEST_COMMON_H_

#include <stdint.h>

#define APP_PORT 1U

extern uint8_t app_payload[4];

void lorawan_native_before(void *fixture);
void lorawan_native_after(void *fixture);
void run_successful_join(uint32_t rx_window);
void prepare_send_test(void);

#endif /* TESTS_SUBSYS_LORAWAN_NATIVE_TEST_COMMON_H_ */
