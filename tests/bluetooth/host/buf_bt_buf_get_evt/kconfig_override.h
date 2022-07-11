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
