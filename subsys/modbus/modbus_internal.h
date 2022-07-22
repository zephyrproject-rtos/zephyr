/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Parts of this file are based on mb.h from uC/Modbus Stack.
 *
 *                                uC/Modbus
 *                         The Embedded Modbus Stack
 *
 *      Copyright 2003-2020 Silicon Laboratories Inc. www.silabs.com
 *
 *                   SPDX-License-Identifier: APACHE-2.0
 *
 * This software is subject to an open source license and is distributed by
 *  Silicon Laboratories Inc. pursuant to the terms of the Apache License,
 *      Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef ZEPHYR_INCLUDE_MODBUS_INTERNAL_H_
#define ZEPHYR_INCLUDE_MODBUS_INTERNAL_H_

#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/modbus/modbus.h>

#ifdef CONFIG_MODBUS_FP_EXTENSIONS
#define MODBUS_FP_EXTENSIONS_ADDR		5000
#else
#define MODBUS_FP_EXTENSIONS_ADDR		UINT16_MAX
#endif

#define MODBUS_RTU_MTU				256

/* Modbus function codes */
#define	MODBUS_FC01_COIL_RD			1
#define	MODBUS_FC02_DI_RD			2
#define	MODBUS_FC03_HOLDING_REG_RD		3
#define	MODBUS_FC04_IN_REG_RD			4
#define	MODBUS_FC05_COIL_WR			5
#define	MODBUS_FC06_HOLDING_REG_WR		6
#define	MODBUS_FC08_DIAGNOSTICS			8
#define	MODBUS_FC15_COILS_WR			15
#define	MODBUS_FC16_HOLDING_REGS_WR		16

/* Diagnostic sub-function codes */
#define MODBUS_FC08_SUBF_QUERY			0
#define MODBUS_FC08_SUBF_CLR_CTR		10
#define MODBUS_FC08_SUBF_BUS_MSG_CTR		11
#define MODBUS_FC08_SUBF_BUS_CRC_CTR		12
#define MODBUS_FC08_SUBF_BUS_EXCEPT_CTR		13
#define MODBUS_FC08_SUBF_SERVER_MSG_CTR		14
#define MODBUS_FC08_SUBF_SERVER_NO_RESP_CTR	15

/* Modbus exception codes */
#define MODBUS_EXC_NONE				0
#define MODBUS_EXC_ILLEGAL_FC			1
#define MODBUS_EXC_ILLEGAL_DATA_ADDR		2
#define MODBUS_EXC_ILLEGAL_DATA_VAL		3
#define MODBUS_EXC_SERVER_DEVICE_FAILURE	4
#define MODBUS_EXC_ACK				5
#define MODBUS_EXC_SERVER_DEVICE_BUSY		6
#define MODBUS_EXC_MEM_PARITY_ERROR		8
#define MODBUS_EXC_GW_PATH_UNAVAILABLE		10
#define MODBUS_EXC_GW_TARGET_FAILED_TO_RESP	11

/* Modbus RTU (ASCII) constants */
#define MODBUS_COIL_OFF_CODE			0x0000
#define MODBUS_COIL_ON_CODE			0xFF00
#define MODBUS_RTU_MIN_MSG_SIZE			4
#define MODBUS_CRC16_POLY			0xA001
#define MODBUS_ASCII_MIN_MSG_SIZE		11
#define MODBUS_ASCII_START_FRAME_CHAR		':'
#define MODBUS_ASCII_END_FRAME_CHAR1		'\r'
#define MODBUS_ASCII_END_FRAME_CHAR2		'\n'

/* Modbus ADU constants */
#define MODBUS_ADU_PROTO_ID			0x0000

struct modbus_serial_config {
	/* UART device */
	const struct device *dev;
	/* RTU timeout (maximum inter-frame delay) */
	uint32_t rtu_timeout;
	/* Pointer to current position in buffer */
	uint8_t *uart_buf_ptr;
	/* Pointer to driver enable (DE) pin config */
	struct gpio_dt_spec *de;
	/* Pointer to receiver enable (nRE) pin config */
	struct gpio_dt_spec *re;
	/* RTU timer to detect frame end point */
	struct k_timer rtu_timer;
	/* Number of bytes received or to send */
	uint16_t uart_buf_ctr;
	/* Storage of received characters or characters to send */
	uint8_t uart_buf[CONFIG_MODBUS_BUFFER_SIZE];
};

#define MODBUS_STATE_CONFIGURED		0

struct modbus_context {
	/* Interface name */
	const char *iface_name;
	union {
		/* Serial line configuration */
		struct modbus_serial_config *cfg;
		/* RAW TX callback */
		modbus_raw_cb_t raw_tx_cb;
	};
	/* MODBUS mode */
	enum modbus_mode mode;
	/* True if interface is configured as client */
	bool client;
	/* Amount of time client is willing to wait for response from server */
	uint32_t rxwait_to;
	/* Pointer to user server callbacks */
	struct modbus_user_callbacks *mbs_user_cb;
	/* Interface state */
	atomic_t state;

	/* Client's mutually exclusive access */
	struct k_mutex iface_lock;
	/* Wait for response semaphore */
	struct k_sem client_wait_sem;
	/* Server work item */
	struct k_work server_work;
	/* Received frame */
	struct modbus_adu rx_adu;
	/* Frame to transmit */
	struct modbus_adu tx_adu;

	/* Records error from frame reception, e.g. CRC error */
	int rx_adu_err;

#ifdef CONFIG_MODBUS_FC08_DIAGNOSTIC
	uint16_t mbs_msg_ctr;
	uint16_t mbs_crc_err_ctr;
	uint16_t mbs_except_ctr;
	uint16_t mbs_server_msg_ctr;
	uint16_t mbs_noresp_ctr;
#endif
	/* Unit ID */
	uint8_t unit_id;

};

/**
 * @brief Get Modbus interface context.
 *
 * @param ctx        Modbus interface context
 *
 * @retval           Pointer to interface context or NULL
 *                   if interface not available or not configured;
 */
struct modbus_context *modbus_get_context(const uint8_t iface);

/**
 * @brief Get Modbus interface index.
 *
 * @param ctx        Pointer to Modbus interface context
 *
 * @retval           Interface index or negative error value.
 */
int modbus_iface_get_by_ctx(const struct modbus_context *ctx);

/**
 * @brief Send ADU.
 *
 * @param ctx        Modbus interface context
 */
void modbus_tx_adu(struct modbus_context *ctx);

/**
 * @brief Send ADU and wait certain time for response.
 *
 * @param ctx        Modbus interface context
 *
 * @retval           0 If the function was successful,
 *                   -ENOTSUP if Modbus mode is not supported,
 *                   -ETIMEDOUT on timeout,
 *                   -EMSGSIZE on length error,
 *                   -EIO on CRC error.
 */
int modbus_tx_wait_rx_adu(struct modbus_context *ctx);

/**
 * @brief Let server handle the received ADU.
 *
 * @param ctx        Modbus interface context
 *
 * @retval           True if the server has prepared a response ADU
 *                   that should be sent.
 */
bool modbus_server_handler(struct modbus_context *ctx);

/**
 * @brief Reset server stats.
 *
 * @param ctx        Modbus interface context
 */
void modbus_reset_stats(struct modbus_context *ctx);

/**
 * @brief Disable serial line reception.
 *
 * @param ctx        Modbus interface context
 */
void modbus_serial_rx_disable(struct modbus_context *ctx);

/**
 * @brief Enable serial line reception.
 *
 * @param ctx        Modbus interface context
 */
void modbus_serial_rx_enable(struct modbus_context *ctx);

/**
 * @brief Assemble ADU from serial line RX buffer
 *
 * @param ctx        Modbus interface context
 *
 * @retval           0 If the function was successful,
 *                   -ENOTSUP if serial line mode is not supported,
 *                   -EMSGSIZE on length error,
 *                   -EIO on CRC error.
 */
int modbus_serial_rx_adu(struct modbus_context *ctx);

/**
 * @brief Assemble ADU from serial line RX buffer
 *
 * @param ctx        Modbus interface context
 *
 * @retval           0 If the function was successful,
 *                   -ENOTSUP if serial line mode is not supported.
 */
int modbus_serial_tx_adu(struct modbus_context *ctx);

/**
 * @brief Initialize serial line support.
 *
 * @param ctx        Modbus interface context
 * @param param      Configuration parameter of the interface
 *
 * @retval           0 If the function was successful.
 */
int modbus_serial_init(struct modbus_context *ctx,
		       struct modbus_iface_param param);

/**
 * @brief Disable serial line support.
 *
 * @param ctx        Modbus interface context
 */
void modbus_serial_disable(struct modbus_context *ctx);

int modbus_raw_rx_adu(struct modbus_context *ctx);
int modbus_raw_tx_adu(struct modbus_context *ctx);
int modbus_raw_init(struct modbus_context *ctx,
		    struct modbus_iface_param param);

#endif /* ZEPHYR_INCLUDE_MODBUS_INTERNAL_H_ */
