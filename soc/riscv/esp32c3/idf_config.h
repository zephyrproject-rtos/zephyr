/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * ESP32C3 IDF components integration helper
 */

#ifndef ZEPHYR_SOC_RISCV_ESP32C3_IDF_CONFIG_H_
#define ZEPHYR_SOC_RISCV_ESP32C3_IDF_CONFIG_H_

#include <zephyr/devicetree.h>

/* From sdkconfig.h */
#define CONFIG_IDF_TARGET_ESP32C3 1
#define CONFIG_IDF_FIRMWARE_CHIP_ID 0x0005
#define CONFIG_LOG_TIMESTAMP_SOURCE_RTOS 1
#define CONFIG_BOOTLOADER_LOG_LEVEL 3

#define CONFIG_BT_CTRL_ESP32C3 1
#define CONFIG_BT_SOC_SUPPORT_5_0 1
#define BTDM_CTRL_HCI_MODE_VHCI 1

#define CONFIG_ESPTOOLPY_FLASHMODE_DIO 1

#undef EMPTY

/**
 * Following was in stubs.h and spread in clk.h and other files
 * here we removed stubs.h and put everything related to the IDF
 * configuration and anything we can derived from the devicetree.
 */
#define CONFIG_ESP32C3_RTC_CLK_CAL_CYCLES	1024
#define CONFIG_ESP32C3_DEFAULT_CPU_FREQ_MHZ (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000000)

#define ESP_SOC_DEFAULT_RTC_CLK_CAL_CYCLES 1024

/* Extract configuration from the devicetree */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay) &&		\
	DT_PROP_BY_IDX(DT_NODELABEL(uart0), reg, 0) ==		\
	DT_PROP_BY_IDX(DT_CHOSEN(zephyr_console), reg, 0)
#define CONFIG_ESP_CONSOLE_UART_NUM 0
#define CONFIG_ESP_CONSOLE_UART_BAUDRATE DT_PROP(DT_NODELABEL(uart0), current_speed)

#elif DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) &&		\
	DT_PROP_BY_IDX(DT_NODELABEL(uart1), reg, 0) ==		\
	DT_PROP_BY_IDX(DT_CHOSEN(zephyr_console), reg, 0)
#define CONFIG_ESP_CONSOLE_UART_NUM 1
#define CONFIG_ESP_CONSOLE_UART_BAUDRATE DT_PROP(DT_NODELABEL(uart1), current_speed)

#elif DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) &&		\
	DT_PROP_BY_IDX(DT_NODELABEL(uart2), reg, 0) ==		\
	DT_PROP_BY_IDX(DT_CHOSEN(zephyr_console), reg, 0)
#define CONFIG_ESP_CONSOLE_UART_NUM 2
#define CONFIG_ESP_CONSOLE_UART_BAUDRATE DT_PROP(DT_NODELABEL(uart2), current_speed)

#else
#error "ESP console uart missing or not enabled in the dts"
#endif

#endif /* ZEPHYR_SOC_RISCV_ESP32C3_IDF_CONFIG_H_ */
