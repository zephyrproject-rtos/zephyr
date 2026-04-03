/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Header file for the INTEL LW UART
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_INTEL_LW_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_INTEL_LW_H_

/* End of packet feature.
 * Driver will trigger interrupt upon receiving end of package character.
 * Please enable CONFIG_UART_INTEL_LW_EOP to use this feature.
 * Use the api: uart_drv_cmd with CMD_ENABLE_EOP to enable the feature.
 * This cmd will write the ip register and also set a flag to the driver.
 * The flag will modify uart_irq_callback_user_data_set
 * to set call back function for eop interrupt.
 * Flag is cleared after uart_irq_callback_user_data_set is called.
 */
#define CMD_ENABLE_EOP          0x01
#define CMD_DISABLE_EOP         0x02

/* Transmit break feature.
 * Use uart_drv_cmd with CMD_TRBK_EN to break ongoing transmit.
 * After this cmd, uart is unable to transmit any data.
 * Please use CMD_TRBK_DIS to resume normal operation.
 * Please also call uart_intel_lw_err_check, to clear the error caused
 * by transmit break.
 */
#define CMD_TRBK_EN             0x03
#define CMD_TRBK_DIS            0x04

/* This driver supports interrupt driven api.
 * Polling for data under normal operation, might cause unexpected behaviour.
 * If users wish to poll for data, please use the api:
 * uart_drv_cmd with CMD_POLL_ASSERT_RTS before polling out/in.
 * Then use CMD_POLL_DEASSERT_RTS to resume normal operation after polling.
 */
#define CMD_POLL_ASSERT_RTS     0x05
#define CMD_POLL_DEASSERT_RTS   0x06

#endif /* ZEPHYR_DRIVERS_SERIAL_UART_INTEL_LW_H_ */
