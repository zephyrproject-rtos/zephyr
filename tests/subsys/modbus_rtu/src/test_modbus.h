/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_MODBUS_H__
#define __TEST_MODBUS_H__

#include <drivers/uart.h>
#include <ztest.h>
#include <modbus/modbus_rtu.h>

#define MB_TEST_BAUDRATE_LOW	9600
#define MB_TEST_BAUDRATE_HIGH	115200
#define MB_TEST_PARITY		UART_CFG_PARITY_NONE
#define MB_TEST_RESPONSE_TO	50000
#define MB_TEST_NODE_ADDR	0x01
#define MB_TEST_FP_OFFSET	5000

#if defined(CONFIG_BOARD_FRDM_K64F)
/*
 * UART1(PTB16)-RX <-> UART2(PTD3)-TX pins and
 * UART1(PTB17)-TX <-> UART2(PTD2)-RX pins have to be connected.
 */
#define MB_TEST_IFACE_CLIENT		0
#define MB_TEST_IFACE_SERVER		1
#else
#define MB_TEST_IFACE_CLIENT		0
#define MB_TEST_IFACE_SERVER		0
#endif

void test_server_rtu_setup_low_none(void);
void test_server_rtu_setup_low_odd(void);
void test_server_rtu_setup_high_even(void);
void test_server_rtu_setup_ascii(void);
void test_server_rtu_disable(void);

void test_client_rtu_setup_low_none(void);
void test_client_rtu_setup_low_odd(void);
void test_client_rtu_setup_high_even(void);
void test_client_rtu_setup_ascii(void);
void test_rtu_coil_wr_rd(void);
void test_rtu_di_rd(void);
void test_rtu_input_reg(void);
void test_rtu_holding_reg(void);
void test_rtu_diagnostic(void);
void test_client_rtu_disable(void);

#endif /* __TEST_MODBUS_H__ */
