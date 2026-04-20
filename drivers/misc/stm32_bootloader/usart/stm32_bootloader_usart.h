/*
 * Copyright (c) 2026 Alex Fabre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_STM32_BOOTLOADER_UART_H_
#define ZEPHYR_DRIVERS_MISC_STM32_BOOTLOADER_UART_H_

/* AN3155 synchronisation and ACK bytes */
#define STM32_BL_UART_SYNC 0x7F
#define STM32_BL_UART_ACK  0x79
#define STM32_BL_UART_NACK 0x1F

/* AN3155 command bytes */
#define STM32_BL_UART_CMD_GET 0x00
#define STM32_BL_UART_CMD_GV  0x01
#define STM32_BL_UART_CMD_GID 0x02
#define STM32_BL_UART_CMD_RM  0x11
#define STM32_BL_UART_CMD_GO  0x21
#define STM32_BL_UART_CMD_WM  0x31
#define STM32_BL_UART_CMD_ER  0x43
#define STM32_BL_UART_CMD_EE  0x44

/* Mass-erase payload for Standard Erase (0x43) */
#define STM32_BL_UART_STD_MASS_ERASE 0xFF

#endif /* ZEPHYR_DRIVERS_MISC_STM32_BOOTLOADER_UART_H_ */
