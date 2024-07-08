/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_MISC_HFC_MULTILINK_SRC_DATA_H_
#define ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_MISC_HFC_MULTILINK_SRC_DATA_H_

#define TESTER_NAME "tester"
#define SDU_NUM 3
#define L2CAP_TEST_PSM 0x0080
/* use the first dynamic channel ID */
#define L2CAP_TEST_CID 0x0040
#define PAYLOAD_LEN 50

#define EXPECTED_CONN_INTERVAL 50
#define CONN_INTERVAL_TOL 20

#endif /* ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_MISC_HFC_MULTILINK_SRC_DATA_H_ */
