/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell_remote.h>

/* File is used when multiple remote shell clients are enabled. */

#if defined(CONFIG_SHELL_REMOTE_PPR) && DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(cpuapp_cpuppr_ipc))
SHELL_REMOTE_CONN(ppr, DT_NODELABEL(cpuapp_cpuppr_ipc));
#endif

#if defined(CONFIG_SHELL_REMOTE_FLPR) && DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(cpuapp_cpuflpr_ipc))
SHELL_REMOTE_CONN(flpr, DT_NODELABEL(cpuapp_cpuflpr_ipc));
#endif

#if defined(CONFIG_SHELL_REMOTE_RADIO) && DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(cpuapp_cpurad_ipc))
SHELL_REMOTE_CONN(radio, DT_NODELABEL(cpuapp_cpurad_ipc));
#endif
