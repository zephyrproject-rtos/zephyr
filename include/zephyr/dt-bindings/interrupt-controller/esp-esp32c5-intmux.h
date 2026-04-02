/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP32-C5 interrupt multiplexer definitions for device tree bindings
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C5_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C5_INTMUX_H_

#define WIFI_MAC_INTR_SOURCE           0  /**< Wi-Fi MAC interrupt, level */
#define WIFI_MAC_NMI_SOURCE            1  /**< Wi-Fi MAC interrupt, NMI */
#define WIFI_PWR_INTR_SOURCE           2  /**< Wi-Fi power interrupt */
#define WIFI_BB_INTR_SOURCE            3  /**< Wi-Fi BB interrupt, level */
#define BT_MAC_INTR_SOURCE             4  /**< BT MAC interrupt */
#define BT_BB_INTR_SOURCE              5  /**< BT BB interrupt, level */
#define BT_BB_NMI_SOURCE               6  /**< BT BB interrupt, NMI */
#define LP_TIMER_INTR_SOURCE           7  /**< LP timer interrupt */
#define COEX_INTR_SOURCE               8  /**< Coexistence interrupt */
#define BLE_TIMER_INTR_SOURCE          9  /**< BLE timer interrupt */
#define BLE_SEC_INTR_SOURCE            10 /**< BLE SEC interrupt */
#define I2C_MASTER_SOURCE              11 /**< I2C master interrupt, level */
#define ZB_MAC_SOURCE                  12 /**< ZigBee MAC interrupt */
#define PMU_INTR_SOURCE                13 /**< PMU interrupt */
#define EFUSE_INTR_SOURCE              14 /**< eFuse interrupt, level */
#define LP_RTC_TIMER_INTR_SOURCE       15 /**< LP RTC timer interrupt */
#define LP_UART_INTR_SOURCE            16 /**< LP UART interrupt */
#define LP_I2C_INTR_SOURCE             17 /**< LP I2C interrupt */
#define LP_WDT_INTR_SOURCE             18 /**< LP watchdog interrupt */
#define LP_PERI_TIMEOUT_INTR_SOURCE    19 /**< LP peripheral timeout interrupt */
#define LP_APM_M0_INTR_SOURCE          20 /**< LP APM M0 interrupt */
#define LP_APM_M1_INTR_SOURCE          21 /**< LP APM M1 interrupt */
#define HUK_INTR_SOURCE                22 /**< HUK interrupt */
#define FROM_CPU_INTR0_SOURCE          23 /**< CPU interrupt 0, level */
#define FROM_CPU_INTR1_SOURCE          24 /**< CPU interrupt 1, level */
#define FROM_CPU_INTR2_SOURCE          25 /**< CPU interrupt 2, level */
#define FROM_CPU_INTR3_SOURCE          26 /**< CPU interrupt 3, level */
#define ASSIST_DEBUG_INTR_SOURCE       27 /**< Assist debug module interrupt, level */
#define TRACE_INTR_SOURCE              28 /**< Trace interrupt */
#define CACHE_INTR_SOURCE              29 /**< Cache interrupt */
#define CPU_PERI_TIMEOUT_INTR_SOURCE   30 /**< CPU peripheral timeout interrupt */
#define GPIO_INTR_SOURCE               31 /**< GPIO interrupt, level */
#define GPIO_EXT_SOURCE                32 /**< GPIO external interrupt */
#define PAU_INTR_SOURCE                33 /**< PAU interrupt */
#define HP_PERI_TIMEOUT_INTR_SOURCE    34 /**< HP peripheral timeout interrupt */
#define MODEM_PERI_TIMEOUT_INTR_SOURCE 35 /**< Modem peripheral timeout interrupt */
#define HP_APM_M0_INTR_SOURCE          36 /**< HP APM M0 interrupt */
#define HP_APM_M1_INTR_SOURCE          37 /**< HP APM M1 interrupt */
#define HP_APM_M2_INTR_SOURCE          38 /**< HP APM M2 interrupt */
#define HP_APM_M3_INTR_SOURCE          39 /**< HP APM M3 interrupt */
#define HP_APM_M4_INTR_SOURCE          40 /**< HP APM M4 interrupt */
#define LP_APM0_INTR_SOURCE            41 /**< LP APM0 interrupt */
#define CPU_APM_M0_INTR_SOURCE         42 /**< CPU APM M0 interrupt */
#define CPU_APM_M1_INTR_SOURCE         43 /**< CPU APM M1 interrupt */
#define MSPI_INTR_SOURCE               44 /**< MSPI interrupt */
#define I2S0_INTR_SOURCE               45 /**< I2S0 interrupt, level */
#define UHCI0_INTR_SOURCE              46 /**< UHCI0 interrupt, level */
#define UART0_INTR_SOURCE              47 /**< UART0 interrupt, level */
#define UART1_INTR_SOURCE              48 /**< UART1 interrupt, level */
#define LEDC_INTR_SOURCE               49 /**< LED PWM interrupt, level */
#define TWAI0_INTR_SOURCE              50 /**< TWAI0 interrupt, level */
#define TWAI0_TIMER_INTR_SOURCE        51 /**< TWAI0 timer interrupt */
#define TWAI1_INTR_SOURCE              52 /**< TWAI1 interrupt, level */
#define TWAI1_TIMER_INTR_SOURCE        53 /**< TWAI1 timer interrupt */
#define USB_SERIAL_JTAG_INTR_SOURCE    54 /**< USB Serial JTAG interrupt, level */
#define RMT_INTR_SOURCE                55 /**< Remote controller interrupt, level */
#define I2C_EXT0_INTR_SOURCE           56 /**< I2C controller 0 interrupt, level */
#define TG0_T0_LEVEL_INTR_SOURCE       57 /**< Timer group 0, timer 0 interrupt, level */
#define TG0_WDT_LEVEL_INTR_SOURCE      58 /**< Timer group 0, watchdog interrupt, level */
#define TG1_T0_LEVEL_INTR_SOURCE       59 /**< Timer group 1, timer 0 interrupt, level */
#define TG1_WDT_LEVEL_INTR_SOURCE      60 /**< Timer group 1, watchdog interrupt, level */
#define SYSTIMER_TARGET0_INTR_SOURCE   61 /**< System timer target 0 interrupt */
#define SYSTIMER_TARGET1_INTR_SOURCE   62 /**< System timer target 1 interrupt */
#define SYSTIMER_TARGET2_INTR_SOURCE   63 /**< System timer target 2 interrupt */
#define APB_ADC_INTR_SOURCE            64 /**< APB ADC interrupt, level */
#define MCPWM0_INTR_SOURCE             65 /**< MCPWM0 interrupt, level */
#define PCNT_INTR_SOURCE               66 /**< Pulse counter interrupt */
#define PARL_IO_TX_INTR_SOURCE         67 /**< Parallel IO TX interrupt */
#define PARL_IO_RX_INTR_SOURCE         68 /**< Parallel IO RX interrupt */
#define SLC0_INTR_SOURCE               69 /**< SLC0 interrupt */
#define SLC1_INTR_SOURCE               70 /**< SLC1 interrupt */
#define DMA_IN_CH0_INTR_SOURCE         71 /**< DMA IN channel 0 interrupt, level */
#define DMA_IN_CH1_INTR_SOURCE         72 /**< DMA IN channel 1 interrupt, level */
#define DMA_IN_CH2_INTR_SOURCE         73 /**< DMA IN channel 2 interrupt, level */
#define DMA_OUT_CH0_INTR_SOURCE        74 /**< DMA OUT channel 0 interrupt, level */
#define DMA_OUT_CH1_INTR_SOURCE        75 /**< DMA OUT channel 1 interrupt, level */
#define DMA_OUT_CH2_INTR_SOURCE        76 /**< DMA OUT channel 2 interrupt, level */
#define GSPI2_INTR_SOURCE              77 /**< GSPI2 interrupt */
#define AES_INTR_SOURCE                78 /**< AES accelerator interrupt, level */
#define SHA_INTR_SOURCE                79 /**< SHA accelerator interrupt, level */
#define RSA_INTR_SOURCE                80 /**< RSA accelerator interrupt, level */
#define ECC_INTR_SOURCE                81 /**< ECC accelerator interrupt, level */
#define ECDSA_INTR_SOURCE              82 /**< ECDSA interrupt */
#define KM_INTR_SOURCE                 83 /**< Key manager interrupt */
#define MAX_INTR_SOURCE                84 /**< Total number of interrupt sources */

/**
 * @brief Default interrupt priority.
 *
 * Zero will allocate low/medium levels of priority (ESP_INTR_FLAG_LOWMED).
 */
#define IRQ_DEFAULT_PRIORITY 0

#define ESP_INTR_FLAG_SHARED (1 << 8) /**< Interrupt can be shared between ISRs */

/* LP Core intmux */
#define LP_CORE_IO_INTR_SOURCE    0 /**< LP core IO interrupt */
#define LP_CORE_I2C_INTR_SOURCE   1 /**< LP core I2C interrupt */
#define LP_CORE_UART_INTR_SOURCE  2 /**< LP core UART interrupt */
#define LP_CORE_TIMER_INTR_SOURCE 3 /**< LP core timer interrupt */
#define LP_CORE_PMU_INTR_SOURCE   5 /**< LP core PMU interrupt */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C5_INTMUX_H_ */
