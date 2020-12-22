/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
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

#ifndef ZEPHYR_INCLUDE_MODBUS_RTU_INTERNAL_H_
#define ZEPHYR_INCLUDE_MODBUS_RTU_INTERNAL_H_

#include <zephyr.h>
#include <drivers/gpio.h>
#include <modbus/modbus_rtu.h>

#ifdef CONFIG_MODBUS_RTU_FP_EXTENSIONS
#define MODBUS_RTU_FP_ADDR			5000
#else
#define MODBUS_RTU_FP_ADDR			UINT16_MAX
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

/* Modbus RTU (ASCII) constants */
#define MODBUS_COIL_OFF_CODE			0x0000
#define MODBUS_COIL_ON_CODE			0xFF00
#define MODBUS_RTU_MIN_MSG_SIZE			4
#define MODBUS_CRC16_POLY			0xA001
#define MODBUS_ASCII_MIN_MSG_SIZE		11
#define MODBUS_ASCII_START_FRAME_CHAR		':'
#define MODBUS_ASCII_END_FRAME_CHAR1		'\r'
#define MODBUS_ASCII_END_FRAME_CHAR2		'\n'

struct mb_rtu_frame {
	uint16_t length;
	uint8_t addr;
	uint8_t fc;
	uint8_t data[CONFIG_MODBUS_RTU_BUFFER_SIZE - 4];
	uint16_t crc;
};

struct mb_rtu_gpio_config {
	const char *name;
	const struct device *dev;
	gpio_pin_t pin;
	gpio_dt_flags_t flags;
};

#define MB_RTU_STATE_CONFIGURED		0

struct mb_rtu_context {
	/* UART device name */
	const char *dev_name;
	/* UART device */
	const struct device *dev;
	/* True if ASCII mode is enabled */
	bool ascii_mode;
	/* True if interface is configured as client */
	bool client;
	/* Amount of time client is willing to wait for response from server */
	uint32_t rxwait_to;
	/* RTU timeout (maximum inter-frame delay) */
	uint32_t rtu_timeout;
	/* Pointer to user server callbacks */
	struct mbs_rtu_user_callbacks *mbs_user_cb;
	/* Interface state */
	atomic_t state;
	/* Pointer to current position in buffer */
	uint8_t *uart_buf_ptr;
	/* Pointer to driver enable (DE) pin config */
	struct mb_rtu_gpio_config *de;
	/* Pointer to receiver enable (nRE) pin config */
	struct mb_rtu_gpio_config *re;

	/* Client's mutually exclusive access */
	struct k_mutex iface_lock;
	/* Wait for response semaphore */
	struct k_sem client_wait_sem;
	/* Server work item */
	struct k_work server_work;
	/* RTU timer to detect frame end point */
	struct k_timer rtu_timer;
	/* Received frame */
	struct mb_rtu_frame rx_frame;
	/* Frame to transmit */
	struct mb_rtu_frame tx_frame;

	/* Number of bytes received or to send */
	uint16_t uart_buf_ctr;
	/* Records error from frame reception, e.g. CRC error */
	uint16_t rx_frame_err;

#ifdef CONFIG_MODBUS_RTU_FC08_DIAGNOSTIC
	uint16_t mbs_msg_ctr;
	uint16_t mbs_crc_err_ctr;
	uint16_t mbs_except_ctr;
	uint16_t mbs_server_msg_ctr;
	uint16_t mbs_noresp_ctr;
#endif
	/* Node address */
	uint8_t node_addr;
	/* Storage of received characters or characters to send */
	uint8_t uart_buf[CONFIG_MODBUS_RTU_BUFFER_SIZE];

};

struct mb_rtu_context *mb_get_context(const uint8_t iface);
int mb_rx_frame(struct mb_rtu_context *ctx);
void mb_tx_frame(struct mb_rtu_context *ctx);

bool mbs_rx_handler(struct mb_rtu_context *ctx);
void mbs_reset_statistics(struct mb_rtu_context *pch);

#endif /* ZEPHYR_INCLUDE_MODBUS_RTU_INTERNAL_H_ */
