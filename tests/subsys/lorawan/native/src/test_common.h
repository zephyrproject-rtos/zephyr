/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_SUBSYS_LORAWAN_NATIVE_TEST_COMMON_H_
#define TESTS_SUBSYS_LORAWAN_NATIVE_TEST_COMMON_H_

#include <stdint.h>

#define APP_PORT 1U

/* RX2 opens RECEIVE_DELAY2 after TX completion with the fixture's RxDelay of 1 s */
#define TEST_RX2_DELAY_MS 2000

/* RETRANSMIT_TIMEOUT is a random delay of 1 to 3 s */
#define TEST_RETRANSMIT_TIMEOUT_MIN_MS 1000
#define TEST_RETRANSMIT_TIMEOUT_MAX_MS 3000

extern uint8_t app_payload[4];

void lorawan_native_before(void *fixture);
void lorawan_native_after(void *fixture);
void run_successful_join(uint32_t rx_window);
void prepare_send_test(void);
void assert_retransmit_timeout(int64_t tx_time, int64_t next_time);

#endif /* TESTS_SUBSYS_LORAWAN_NATIVE_TEST_COMMON_H_ */
