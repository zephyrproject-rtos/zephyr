/**
 * @file dummy.c
 * Static compilation checks.
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#if defined(CONFIG_BT_HCI_HOST)
/* The Bluetooth subsystem requires the Tx thread to execute at higher priority
 * than the Rx thread as the Tx thread needs to process the acknowledgements
 * before new Rx data is processed. This is a necessity to correctly detect
 * transaction violations in ATT and SMP protocols.
 */
BUILD_ASSERT(CONFIG_BT_HCI_TX_PRIO < CONFIG_BT_RX_PRIO);

/* The Bluetooth subsystem requires that higher priority events shall be given
 * in a priority higher than the Bluetooth Host's Tx and the Controller's
 * receive thread priority.
 * This is required in order to dispatch Number of Completed Packets event
 * before any new data arrives on a connection to the Host threads.
 */
BUILD_ASSERT(CONFIG_BT_DRIVER_RX_HIGH_PRIO < CONFIG_BT_HCI_TX_PRIO);
#endif /* defined(CONFIG_BT_HCI_HOST) */

#if (defined(CONFIG_LOG_BACKEND_RTT) &&                     \
     (defined(CONFIG_SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL) || \
      defined(CONFIG_LOG_BACKEND_RTT_MODE_BLOCK))) ||       \
    defined(CONFIG_LOG_BACKEND_NET) ||                      \
    defined(CONFIG_LOG_IMMEDIATE_CLEAN_OUTPUT)
#define INCOMPATIBLE_IMMEDIATE_LOG_BACKEND 1
#endif

/* Immediate logging on most backend is not supported
 * with the software-based Link Layer since it introduces ISR latency
 * due to outputting log messages with interrupts disabled.
 */
#if !defined(CONFIG_TEST) && !defined(CONFIG_ARCH_POSIX) && \
    defined(CONFIG_BT_LL_SW_SPLIT) &&                       \
    defined(INCOMPATIBLE_IMMEDIATE_LOG_BACKEND)
BUILD_ASSERT(!IS_ENABLED(CONFIG_LOG_IMMEDIATE), "Immediate logging "
	     "on selected backend(s) not "
	     "supported with the software Link Layer");
#endif
