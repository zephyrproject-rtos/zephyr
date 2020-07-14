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
 * CONFIG_SYS_PM_MIN_RESIDENCY_SLEEP_1.
 *
 * Similarly for deep sleep, the test sleeps in the main thread for 500 ms
 * longer than CONFIG_SYS_PM_MIN_RESIDENCY_DEEP_SLEEP_1.
 *
 * @param use_logging test progress will be reported using logging,
 *        otherwise printk.
 * @param cycles to repeat the cycle described above.
 * @retval 0 if successful, errno otherwise.
 */
int test_pwr_mgmt_singlethread(bool use_logging, uint8_t cycles);

/** @brief Alternates between light and deep sleep cycles.
 *
 * Performs same approach to achieve light and deep sleep, but additional
 * it suspend all threads within the app.
 *
 * @param use_logging test progress will be reported using logging,
 *        otherwise printk.
 * @param cycles to repeat the cycle described above.
 * @retval 0 if successful, errno otherwise.
 */
int test_pwr_mgmt_multithread(bool use_logging, uint8_t cycles);

#endif /* __TEST_PWRMGMT_H__ */
