/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_PWRMGMT_H__
#define __TEST_PWRMGMT_H__


/** @brief Alternates between light and deep sleep cycles.
 *
 * For light sleep, the test sleeps in main thread for 500 ms longer than
 * SUSPEND_TO_IDLE.
 *
 * Similarly for deep sleep, the test sleeps in the main thread for 500 ms
 * longer than STANDBY.
 *
 * @param cycles to repeat the cycle described above.
 * @retval 0 if successful, errno otherwise.
 */
int test_pwr_mgmt_singlethread(uint8_t cycles);

/** @brief Alternates between light and deep sleep cycles.
 *
 * Performs same approach to achieve light and deep sleep, but additional
 * it suspend all threads within the app.
 *
 * @param cycles to repeat the cycle described above.
 * @retval 0 if successful, errno otherwise.
 */
int test_pwr_mgmt_multithread(uint8_t cycles);

/** @brief Initializes the board simply without assertions
 *
 * Performs a dummy initialization for the board to enter light/deep sleep
 * without assertions to check if power management is enabled correctly
 * on the board. Also serves to initialize the counters.
 *
 * @retval 0 if successful, errno otherwise.
 */
int test_dummy_init(void);

#endif /* __TEST_PWRMGMT_H__ */
