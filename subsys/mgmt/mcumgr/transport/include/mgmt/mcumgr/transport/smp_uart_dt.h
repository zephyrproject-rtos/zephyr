/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree rules shared by the MCUmgr UART transports.
 */

#ifndef MGMT_MCUMGR_SMP_UART_DT_H_
#define MGMT_MCUMGR_SMP_UART_DT_H_

#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

/**
 * Whether the UART transports use the zephyr,uart-mcumgr chosen UART: only when no
 * zephyr,smp-uart or zephyr,smp-uart-raw node of an enabled transport exists. Boards often set
 * the chosen UART to their console, which an application that declares its own nodes may not
 * want to run MCUmgr on. Nodes of a disabled transport are ignored, as is any node whose driver
 * is disabled.
 */
#if (defined(CONFIG_MCUMGR_TRANSPORT_UART) && DT_HAS_COMPAT_STATUS_OKAY(zephyr_smp_uart)) ||	\
	(defined(CONFIG_MCUMGR_TRANSPORT_RAW_UART) &&						\
	 DT_HAS_COMPAT_STATUS_OKAY(zephyr_smp_uart_raw))
#define SMP_UART_CHOSEN_IN_USE 0
#else
#define SMP_UART_CHOSEN_IN_USE DT_HAS_CHOSEN(zephyr_uart_mcumgr)
#endif

/**
 * Checks an enabled zephyr,smp-uart or zephyr,smp-uart-raw node. Other users of the same
 * UART would replace the transport's UART callback.
 */
#define SMP_UART_NODE_CHECK(node_id)							\
	BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_PARENT(node_id)),			\
		     DT_NODE_PATH(node_id) " is on a disabled UART");			\
	BUILD_ASSERT(DT_CHILD_NUM_STATUS_OKAY(DT_PARENT(node_id)) == 1,			\
		     DT_NODE_PATH(node_id) " must be the only enabled node on its UART");

#endif /* MGMT_MCUMGR_SMP_UART_DT_H_ */
