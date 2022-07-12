/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Common Kconfig settings for bluetooth/host mocks
 */

#define CONFIG_BT_ID_MAX 2
#define CONFIG_BT_HCI_RESERVE 1
#define CONFIG_BT_BUF_EVT_RX_COUNT 10
#define CONFIG_BT_BUF_ACL_RX_COUNT 6
#define CONFIG_BT_BUF_ACL_RX_SIZE 27
#define CONFIG_BT_BUF_EVT_RX_SIZE 68

/*
 * Assertion configuration
 */

#define CONFIG_ASSERT 1
#define CONFIG_ASSERT_LEVEL 2
#define CONFIG_ASSERT_VERBOSE 1

