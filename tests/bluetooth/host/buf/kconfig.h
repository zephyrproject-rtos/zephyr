/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Common Kconfig settings for bluetooth/host mocks */

#define CONFIG_BT_ID_MAX 2
#define CONFIG_BT_HCI_RESERVE 1
#define CONFIG_BT_BUF_EVT_RX_COUNT 10
#define CONFIG_BT_BUF_ACL_RX_COUNT 6
#define CONFIG_BT_BUF_ACL_RX_SIZE 27
#define CONFIG_BT_BUF_EVT_RX_SIZE 68

#ifdef BT_BUF_EVT_DISCARDABLE_COUNT_CONFIG_SET
/*  Keep the value 1 in order to make IS_ENABLED(CONFIG_BT_BUF_EVT_DISCARDABLE_COUNT) works */
#define CONFIG_BT_BUF_EVT_DISCARDABLE_COUNT 1
#define CONFIG_BT_BUF_EVT_DISCARDABLE_SIZE 43
#endif

#ifdef BT_ISO_CONFIG_SET
#define CONFIG_BT_ISO 1
#define CONFIG_BT_ID_MAX 2
#define CONFIG_BT_ISO_MAX_CHAN 1
#define CONFIG_BT_ISO_RX_BUF_COUNT 1
#define CONFIG_BT_ISO_RX_MTU 251
#endif

#ifdef BT_CONN_CONFIG_SET
/*  Keep the value 1 in order to make IS_ENABLED(CONFIG_BT_CONN) works */
#define CONFIG_BT_CONN 1
#define CONFIG_BT_MAX_CONN 4
#endif

#ifdef BT_ISO_UNICAST_CONFIG_SET
#define CONFIG_BT_ISO_UNICAST 1
#define CONFIG_BT_ISO_RX_BUF_COUNT 1
#define CONFIG_BT_ISO_RX_MTU 251
#endif

#ifdef BT_ISO_SYNC_RECEIVER_CONFIG_SET
#define CONFIG_BT_ISO_SYNC_RECEIVER 1
#define CONFIG_BT_ISO_RX_BUF_COUNT 1
#define CONFIG_BT_ISO_RX_MTU 251
#endif

/* Assertion configuration */

#define CONFIG_ASSERT 1
#define CONFIG_ASSERT_LEVEL 2
#define CONFIG_ASSERT_VERBOSE 1
