/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BMC_APP_WEB_TERMINAL_HOST_SERIAL
int console_bridge_ws_init(void);
#else
static inline int console_bridge_ws_init(void) { return 0; }
#endif
