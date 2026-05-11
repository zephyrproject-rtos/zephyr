/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_CONSOLE_BRIDGE
int console_bridge_init(void);
#else
static inline int console_bridge_init(void) { return 0; }
#endif
