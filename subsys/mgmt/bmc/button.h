/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BUTTON_H__
#define __BUTTON_H__
#ifdef CONFIG_BMC_PERSISTENT_STORAGE
int button_init(void);
#else
static inline int button_init(void) { return 0; }
#endif
#endif
