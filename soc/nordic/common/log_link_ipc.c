/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log_link_ipc.h>

#if defined(CONFIG_LOG_LINK_IPC_PPR) && DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(cpuapp_cpuppr_ipc))
LOG_LINK_IPC_DEFINE(ppr, DT_NODELABEL(cpuapp_cpuppr_ipc),
		    COND_CODE_1(CONFIG_LOG_LINK_IPC_RX_HOLD,
		(0), (CONFIG_LOG_LINK_IPC_BUFFER_SIZE)),
							     true);
#endif

#if defined(CONFIG_LOG_LINK_IPC_FLPR) && DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(cpuapp_cpuflpr_ipc))
LOG_LINK_IPC_DEFINE(flpr, DT_NODELABEL(cpuapp_cpuflpr_ipc),
		    COND_CODE_1(CONFIG_LOG_LINK_IPC_RX_HOLD,
		(0), (CONFIG_LOG_LINK_IPC_BUFFER_SIZE)),
							     true);
#endif

#if defined(CONFIG_LOG_LINK_IPC_RADIO) && DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(cpuapp_cpurad_ipc))
LOG_LINK_IPC_DEFINE(rad, DT_NODELABEL(cpuapp_cpurad_ipc),
		    COND_CODE_1(CONFIG_LOG_LINK_IPC_RX_HOLD,
		(0), (CONFIG_LOG_LINK_IPC_BUFFER_SIZE)),
							     true);
#endif
