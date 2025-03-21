/*
 * Copyright (c) 2018-2019 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is based on DAP.c from CMSIS-DAP Source (Revision:    V2.0.0)
 * https://github.com/ARM-software/CMSIS_5/tree/develop/CMSIS/DAP/Firmware
 * Copyright (c) 2013-2017 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DAP controller private header
 */

#ifndef ZEPHYR_INCLUDE_CMSIS_DAP_H_
#define ZEPHYR_INCLUDE_CMSIS_DAP_H_

#include <zephyr/kernel.h>

/* Firmware Version */
#define DAP_FW_VER				"2.1.0"

/* DAP Command IDs */
#define ID_DAP_INFO				0x00U
#define ID_DAP_HOST_STATUS			0x01U
#define ID_DAP_CONNECT				0x02U
#define ID_DAP_DISCONNECT			0x03U
#define ID_DAP_TRANSFER_CONFIGURE		0x04U
#define ID_DAP_TRANSFER				0x05U
#define ID_DAP_TRANSFER_BLOCK			0x06U
#define ID_DAP_TRANSFER_ABORT			0x07U
#define ID_DAP_WRITE_ABORT			0x08U
#define ID_DAP_DELAY				0x09U
#define ID_DAP_RESET_TARGET			0x0AU

#define ID_DAP_SWJ_PINS				0x10U
#define ID_DAP_SWJ_CLOCK			0x11U
#define ID_DAP_SWJ_SEQUENCE			0x12U

#define ID_DAP_SWDP_CONFIGURE			0x13U
#define ID_DAP_SWDP_SEQUENCE			0x1DU

#define ID_DAP_JTAG_SEQUENCE			0x14U
#define ID_DAP_JTAG_CONFIGURE			0x15U
#define ID_DAP_JTAG_IDCODE			0x16U

#define ID_DAP_SWO_TRANSPORT			0x17U
#define ID_DAP_SWO_MODE				0x18U
#define ID_DAP_SWO_BAUDRATE			0x19U
#define ID_DAP_SWO_CONTROL			0x1AU
#define ID_DAP_SWO_STATUS			0x1BU
#define ID_DAP_SWO_DATA				0x1CU

#define ID_DAP_UART_TRANSPORT			0x1FU
#define ID_DAP_UART_CONFIGURE			0x20U
#define ID_DAP_UART_CONTROL			0x22U
#define ID_DAP_UART_STATUS			0x23U
#define ID_DAP_UART_TRANSFER			0x21U

#define ID_DAP_QUEUE_COMMANDS			0x7EU
#define ID_DAP_EXECUTE_COMMANDS			0x7FU

/* DAP Vendor Command IDs */
#define ID_DAP_VENDOR0				0x80U
#define ID_DAP_VENDOR31				0x9FU
#define ID_DAP_INVALID				0xFFU

/* DAP Status Code */
#define DAP_OK					0U
#define DAP_ERROR				0xFFU

/* DAP ID */
#define DAP_ID_VENDOR				0x01U
#define DAP_ID_PRODUCT				0x02U
#define DAP_ID_SER_NUM				0x03U
#define DAP_ID_FW_VER				0x04U
#define DAP_ID_DEVICE_VENDOR			0x05U
#define DAP_ID_DEVICE_NAME			0x06U
#define DAP_ID_BOARD_VENDOR			0x07U
#define DAP_ID_BOARD_NAME			0x08U
#define DAP_ID_PRODUCT_FW_VER			0x09U
#define DAP_ID_CAPABILITIES			0xF0U
#define DAP_ID_TIMESTAMP_CLOCK			0xF1U
#define DAP_ID_UART_RX_BUFFER_SIZE		0xFBU
#define DAP_ID_UART_TX_BUFFER_SIZE		0xFCU
#define DAP_ID_SWO_BUFFER_SIZE			0xFDU
#define DAP_ID_PACKET_COUNT			0xFEU
#define DAP_ID_PACKET_SIZE			0xFFU

/* DAP Host Status */
#define DAP_DEBUGGER_CONNECTED			0U
#define DAP_TARGET_RUNNING			1U

/* DAP Port */
#define DAP_PORT_AUTODETECT			0U
#define DAP_PORT_DISABLED			0U
#define DAP_PORT_SWD				1U
#define DAP_PORT_JTAG				2U

/* DAP transfer request bits */
#define DAP_TRANSFER_MATCH_VALUE		BIT(4)
#define DAP_TRANSFER_MATCH_MASK			BIT(5)

/* DAP transfer response bits */
#define DAP_TRANSFER_MISMATCH			BIT(4)

/* DAP controller capabilities */
#define DAP_DP_SUPPORTS_SWD			BIT(0)
#define DAP_DP_SUPPORTS_JTAG			BIT(1)
#define DAP_SWO_SUPPORTS_UART			BIT(2)
#define DAP_SWO_SUPPORTS_MANCHESTER		BIT(3)
#define DAP_SUPPORTS_ATOMIC_COMMANDS		BIT(4)
#define DAP_SUPPORTS_TIMESTAMP_CLOCK		BIT(5)
#define DAP_SWO_SUPPORTS_STREAM			BIT(6)

/* DP Register (DPv1) */
#define DP_IDCODE				0x00U
#define DP_ABORT				0x00U
#define DP_CTRL_STAT				0x04U
#define DP_SELECT				0x08U
#define DP_RESEND				0x08U
#define DP_RDBUFF				0x0CU

#define DAP_MBMSG_REGISTER_IFACE		0x0U
#define DAP_MBMSG_FROM_IFACE			0x1U
#define DAP_MBMSG_FROM_CONTROLLER		0x2U

/* Keep it internal until an other interface has been implemented. */
int dap_setup(const struct device *const dev);
uint32_t dap_execute_cmd(const uint8_t *request, uint8_t *response);
void dap_update_pkt_size(const uint16_t pkt_size);

#endif	/* ZEPHYR_INCLUDE_CMSIS_DAP_H_ */
