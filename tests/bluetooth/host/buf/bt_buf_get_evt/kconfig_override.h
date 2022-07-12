/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Override common Kconfig settings
 */


#ifdef EVT_DISCARDABLE_COUNT_CONFIG_SET
#define CONFIG_BT_BUF_EVT_DISCARDABLE_COUNT 3
#define CONFIG_BT_BUF_EVT_DISCARDABLE_SIZE 43
#endif

#ifdef BT_CONN_CONFIG_SET
#define CONFIG_BT_CONN
#define CONFIG_BT_MAX_CONN 4
#endif

#ifdef BT_ISO_CONFIG_SET
#define CONFIG_BT_ISO 1
#define CONFIG_BT_ID_MAX 2
#define CONFIG_BT_ISO_MAX_CHAN 1
#define CONFIG_BT_ISO_RX_BUF_COUNT 1
#define CONFIG_BT_ISO_RX_MTU 251
#endif
