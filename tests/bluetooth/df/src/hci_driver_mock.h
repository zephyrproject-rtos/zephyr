/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_DF_HCI_DRIVER_MOCK_H_
#define ZEPHYR_TESTS_BLUETOOTH_DF_HCI_DRIVER_MOCK_H_

/* @brief Function initislizes virtual test HCI driver.
 *
 * @return Function returns zero in case of success,
 * negative error value in case of failure.
 */
int hci_init_test_driver(void);

#endif /* ZEPHYR_TESTS_BLUETOOTH_DF_HCI_DRIVER_MOCK_H_ */
