/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_BT_HCI_RAW) || !defined(CONFIG_HAS_BT_CTLR) || \
	!defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
/* Following build configurations use configurable CONFIG_BT_BUF_CMD_TX_COUNT:
 * 1. Host + Controller build with and without Controller to Host data flow control, or
 * 2. Host-only with and without Controller to Host data flow control, or
 * 3. Controller-only without Controller to Host data flow control support
 */
#if !defined(CONFIG_BT_HCI_RAW)
/* Host + Controller build, and Host-only build */

/* Auto initiated additional HCI command buffers enqueued in the Host */
#define BT_BUF_CMD_TX_REMOTE_VERSION       COND_CODE_1(CONFIG_BT_REMOTE_VERSION, (1), (0))
#define BT_BUF_CMD_TX_AUTO_PHY_UPDATE      COND_CODE_1(CONFIG_BT_AUTO_PHY_UPDATE, (1), (0))
#define BT_BUF_CMD_TX_AUTO_DATA_LEN_UPDATE COND_CODE_1(CONFIG_BT_AUTO_DATA_LEN_UPDATE, (1), (0))

#else /* CONFIG_BT_HCI_RAW */
#if defined(CONFIG_HAS_BT_CTLR)
/* Controller-only build need no additional HCI command buffers */
BUILD_ASSERT((CONFIG_BT_BUF_CMD_TX_COUNT == CONFIG_BT_CTLR_HCI_NUM_CMD_PKT_MAX),
	     "Mismatch in allocated HCI command buffers compared to Controller supported value.");
#endif /* CONFIG_HAS_BT_CTLR */

/* Controller-only build do not enqueue auto initiated HCI commands */
#define BT_BUF_CMD_TX_REMOTE_VERSION       0
#define BT_BUF_CMD_TX_AUTO_PHY_UPDATE      0
#define BT_BUF_CMD_TX_AUTO_DATA_LEN_UPDATE 0
#endif /* !CONFIG_BT_HCI_RAW */

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
/* When Controller to Host data flow control is supported in Host plus Controller build, or
 * Host-only build, then we need additional BT_BUF_ACL_RX_COUNT number of HCI command buffers for
 * enqueuing Host Number of Completed Packets command.
 *
 * Host keeps the first, and subsequent, Rx buffers (that comes from the driver) for each connection
 * to do re-assembly into, up to the L2CAP SDU length required number of Rx buffers.
 * BT_BUF_ACL_RX_COUNT_EXTRA holds the application configured number of buffers across active
 * connections for recombination of HCI data packets to L2CAP SDUs.
 *
 * BT_BUF_HCI_EVT_RX_COUNT defines the number of available buffers reserved for "synchronous"
 * processing of HCI events like Number of Completed Packets, disconnection complete etc.
 *
 * BT_BUF_HCI_ACL_RX_COUNT defines the number of available buffers for Controller to Host data
 * flow control; keeping the application configured BT_BUF_ACL_RX_COUNT_EXTRA number of buffers
 * available for L2CAP recombination, and a reserved number of buffers for processing HCI events.
 */

/* FIXME: Calculate the maximum number of HCI events of different types that a connection can
 *        enqueue while the Host is slow at processing HCI events.
 *
 *        1. Disconnection Complete event
 *        2. LE Connection Update Complete event
 *        3. LE Long Term Key Request event
 *        4. LE Remote Connection Parameter Request Event
 *        5. LE Data Length Change event
 *        6. LE PHY Update Complete event
 *        7. ...
 *
 * As there is no HCI event flow control defined, implementations will need a transport level flow
 * control to restrict buffers required on resource constraint devices, i.e. if these events are not
 * processed "synchronous".
 */
#define BT_BUF_HCI_EVT_RX_COUNT          1
#define BT_BUF_HCI_ACL_RX_COUNT          (BT_BUF_RX_COUNT - BT_BUF_HCI_EVT_RX_COUNT - \
					  BT_BUF_ACL_RX_COUNT_EXTRA)
#define BT_BUF_CMD_TX_HOST_NUM_CMPLT_PKT (BT_BUF_HCI_ACL_RX_COUNT)

#else /* !CONFIG_BT_HCI_ACL_FLOW_CONTROL */
#define BT_BUF_CMD_TX_HOST_NUM_CMPLT_PKT 0
#endif /* !CONFIG_BT_HCI_ACL_FLOW_CONTROL */

/* Based on Host + Controller, Host-only or Controller-only; Calculate the HCI Command Tx count. */
#define BT_BUF_CMD_TX_COUNT (CONFIG_BT_BUF_CMD_TX_COUNT + \
			     BT_BUF_CMD_TX_HOST_NUM_CMPLT_PKT + \
			     BT_BUF_CMD_TX_REMOTE_VERSION + \
			     BT_BUF_CMD_TX_AUTO_PHY_UPDATE + \
			     BT_BUF_CMD_TX_AUTO_DATA_LEN_UPDATE)

#elif defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
/* When Controller to Host data flow control is supported in the Controller-only build, ensure
 * that BT_BUF_CMD_TX_COUNT is greater than or equal to (BT_BUF_RX_COUNT + Ncmd),
 * where Ncmd is supported maximum Num_HCI_Command_Packets in the Controller implementation.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_BUF_CMD_TX_COUNT),
	     "Configurable HCI command buffer count disallowed.");
BUILD_ASSERT(IS_ENABLED(CONFIG_BT_CTLR_HCI_NUM_CMD_PKT_MAX),
	     "Undefined Controller implementation supported Num_HCI_Command_Packets value.");

/** Can use all of buffer count needed for HCI ACL, HCI ISO or Event RX buffers for ACL RX */
#define BT_BUF_HCI_ACL_RX_COUNT (BT_BUF_RX_COUNT)

/* Controller-only with Controller to Host data flow control */
#define BT_BUF_CMD_TX_COUNT     (BT_BUF_RX_COUNT + CONFIG_BT_CTLR_HCI_NUM_CMD_PKT_MAX)

#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */
