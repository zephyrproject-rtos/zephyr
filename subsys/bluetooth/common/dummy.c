/**
 * @file dummy.c
 * Static compilation checks.
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdalign.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>

/*
 * The unpacked structs below are used inside __packed structures that reflect
 * what goes on the wire. While alignment is not a problem for those, since all
 * their members are bytes or byte arrays, the size is. They must not be padded
 * by the compiler, otherwise the on-wire packet will not map the packed
 * structure correctly.
 *
 * The bt_addr structs are not marked __packed because it's considered ugly by
 * some. But here is a proof that the structs have all the properties of, and
 * can be safely used as packed structs.
 */
BUILD_ASSERT(sizeof(bt_addr_t) == BT_ADDR_SIZE);
BUILD_ASSERT(alignof(bt_addr_t) == 1);
BUILD_ASSERT(sizeof(bt_addr_le_t) == BT_ADDR_LE_SIZE);
BUILD_ASSERT(alignof(bt_addr_le_t) == 1);

#if defined(CONFIG_BT_HCI_HOST)
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
BUILD_ASSERT(!IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE), "Immediate logging "
	     "on selected backend(s) not "
	     "supported with the software Link Layer");
#endif

#if defined(CONFIG_BT_CONN) && defined(CONFIG_BT_HCI_HOST)
/* The host needs more ACL buffers than maximum ACL links. This is because of
 * the way we re-assemble ACL packets into L2CAP PDUs.
 *
 * We keep around the first buffer (that comes from the driver) to do
 * re-assembly into, and if all links are re-assembling, there will be no buffer
 * available for the HCI driver to allocate from.
 *
 * Fixing it properly involves a re-design of the HCI driver interface.
 */
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
BUILD_ASSERT(CONFIG_BT_BUF_ACL_RX_COUNT > CONFIG_BT_MAX_CONN);

#else /* !CONFIG_BT_HCI_ACL_FLOW_CONTROL */

/* BT_BUF_RX_COUNT is defined in include/zephyr/bluetooth/buf.h */
BUILD_ASSERT(BT_BUF_RX_COUNT > CONFIG_BT_MAX_CONN,
	     "BT_BUF_RX_COUNT needs to be greater than CONFIG_BT_MAX_CONN. "
	     "In order to do that, increase CONFIG_BT_BUF_ACL_RX_COUNT.");

#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */

#endif /* CONFIG_BT_CONN */
