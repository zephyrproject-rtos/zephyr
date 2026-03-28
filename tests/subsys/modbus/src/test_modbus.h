/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_MODBUS_H__
#define __TEST_MODBUS_H__

#include <zephyr/drivers/uart.h>
#include <zephyr/ztest.h>
#include <zephyr/modbus/modbus.h>

#define MB_TEST_BAUDRATE_LOW	9600
#define MB_TEST_BAUDRATE_HIGH	115200
#define MB_TEST_PARITY		UART_CFG_PARITY_NONE
#define MB_TEST_RESPONSE_TO	50000
#define MB_TEST_NODE_ADDR	0x01
#define MB_TEST_FP_OFFSET	5000

/*
 * Integration platform for this test is FRDM-K64F.
 * The board must be prepared accordingly:
 * UART3(PTC16)-RX <-> UART2(PTD3)-TX pins and
 * UART3(PTC17)-TX <-> UART2(PTD2)-RX pins have to be connected.
 */

uint8_t test_get_client_iface(void);
uint8_t test_get_server_iface(void);

void test_server_setup_low_none(void);
void test_server_setup_low_odd(void);
void test_server_setup_high_even(void);
void test_server_setup_ascii(void);
void test_server_setup_raw(void);
void test_server_disable(void);

void test_client_setup_low_none(void);
void test_client_setup_low_odd(void);
void test_client_setup_high_even(void);
void test_client_setup_ascii(void);
void test_client_setup_raw(void);
void test_coil_wr_rd(void);
void test_di_rd(void);
void test_input_reg(void);
void test_holding_reg(void);
void test_diagnostic(void);
void test_client_disable(void);

int client_raw_cb(const int iface, const struct modbus_adu *adu,
		void *user_data);
int server_raw_cb(const int iface, const struct modbus_adu *adu,
		void *user_data);

#endif /* __TEST_MODBUS_H__ */
