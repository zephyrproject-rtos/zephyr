/*
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_WIFI)
void wifi_connect(void);
#else
#define wifi_connect()
#endif
