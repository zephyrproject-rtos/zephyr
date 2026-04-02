/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Header file for the ALTERA UART
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_ALTERA_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_ALTERA_H_

/* End of packet feature.
 * Driver will trigger interrupt upon receiving end of package character.
 * Please enable CONFIG_UART_DRV_CMD and CONFIG_UART_ALTERA_EOP to use this feature.
 * Use the api: uart_drv_cmd with CMD_ENABLE_EOP to enable the feature.
 * This cmd will write the ip register and also set a flag to the driver.
 * The flag will modify uart_irq_callback_user_data_set
 * to set call back function for eop interrupt.
 * Flag is cleared after uart_irq_callback_user_data_set is called.
 */
#define CMD_ENABLE_EOP	0x01
#define CMD_DISABLE_EOP	0x02

#endif /* ZEPHYR_DRIVERS_SERIAL_UART_ALTERA_H_ */
