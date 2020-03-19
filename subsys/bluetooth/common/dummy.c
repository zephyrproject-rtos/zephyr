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
#endif

#if defined(CONFIG_BT_CTLR)
/* The Bluetooth Controller's priority receive thread priority shall be higher
 * than the Bluetooth Host's Tx and the Controller's receive thread priority.
 * This is required in order to dispatch Number of Completed Packets event
 * before any new data arrives on a connection to the Host threads.
 */
BUILD_ASSERT(CONFIG_BT_CTLR_RX_PRIO < CONFIG_BT_HCI_TX_PRIO);
#endif /* CONFIG_BT_CTLR */

/* Immediate logging is not supported with the software-based Link Layer
 * since it introduces ISR latency due to outputting log messages with
 * interrupts disabled.
 */
#if !defined(CONFIG_TEST) && !defined(CONFIG_ARCH_POSIX) && \
	(defined(CONFIG_BT_LL_SW_SPLIT) || defined(CONFIG_BT_LL_SW_LEGACY))
BUILD_ASSERT_MSG(!IS_ENABLED(CONFIG_LOG_IMMEDIATE), "Immediate logging not "
		 "supported with the software Link Layer");
#endif
