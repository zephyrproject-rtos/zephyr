/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32H2_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32H2_INTMUX_H_

#define PMU_INTR_SOURCE                   0
#define EFUSE_INTR_SOURCE                 1
#define LP_RTC_TIMER_INTR_SOURCE          2
#define LP_BLE_TIMER_INTR_SOURCE          3
#define LP_WDT_INTR_SOURCE                4
#define LP_PERI_TIMEOUT_INTR_SOURCE       5
#define LP_APM_M0_INTR_SOURCE             6
#define FROM_CPU_INTR0_SOURCE             7
#define FROM_CPU_INTR1_SOURCE             8
#define FROM_CPU_INTR2_SOURCE             9
#define FROM_CPU_INTR3_SOURCE             10
#define ASSIST_DEBUG_INTR_SOURCE          11
#define TRACE_INTR_SOURCE                 12
#define CACHE_INTR_SOURCE                 13
#define CPU_PERI_TIMEOUT_INTR_SOURCE      14
#define BT_MAC_INTR_SOURCE                15
#define BT_BB_INTR_SOURCE                 16
#define BT_BB_NMI_INTR_SOURCE             17
#define COEX_INTR_SOURCE                  18
#define BLE_TIMER_INTR_SOURCE             19
#define BLE_SEC_INTR_SOURCE               20
#define ZB_MAC_INTR_SOURCE                21
#define GPIO_INTR_SOURCE                  22
#define GPIO_NMI_SOURCE                   23
#define PAU_INTR_SOURCE                   24
#define HP_PERI_TIMEOUT_INTR_SOURCE       25
#define HP_APM_M0_INTR_SOURCE             26
#define HP_APM_M1_INTR_SOURCE             27
#define HP_APM_M2_INTR_SOURCE             28
#define HP_APM_M3_INTR_SOURCE             29
#define MSPI_INTR_SOURCE                  30
#define I2S1_INTR_SOURCE                  31
#define UHCI0_INTR_SOURCE                 32
#define UART0_INTR_SOURCE                 33
#define UART1_INTR_SOURCE                 34
#define LEDC_INTR_SOURCE                  35
#define TWAI0_INTR_SOURCE                 36
#define USB_SERIAL_JTAG_INTR_SOURCE       37
#define RMT_INTR_SOURCE                   38
#define I2C_EXT0_INTR_SOURCE              39
#define I2C_EXT1_INTR_SOURCE              40
#define TG0_T0_LEVEL_INTR_SOURCE          41
#define TG0_WDT_LEVEL_INTR_SOURCE         42
#define TG1_T0_LEVEL_INTR_SOURCE          43
#define TG1_WDT_LEVEL_INTR_SOURCE         44
#define SYSTIMER_TARGET0_EDGE_INTR_SOURCE 45
#define SYSTIMER_TARGET1_EDGE_INTR_SOURCE 46
#define SYSTIMER_TARGET2_EDGE_INTR_SOURCE 47
#define APB_ADC_INTR_SOURCE               48
#define MCPWM0_INTR_SOURCE                49
#define PCNT_INTR_SOURCE                  50
#define PARL_IO_TX_INTR_SOURCE            51
#define PARL_IO_RX_INTR_SOURCE            52
#define DMA_IN_CH0_INTR_SOURCE            53
#define DMA_IN_CH1_INTR_SOURCE            54
#define DMA_IN_CH2_INTR_SOURCE            55
#define DMA_OUT_CH0_INTR_SOURCE           56
#define DMA_OUT_CH1_INTR_SOURCE           57
#define DMA_OUT_CH2_INTR_SOURCE           58
#define GSPI2_INTR_SOURCE                 59
#define AES_INTR_SOURCE                   60
#define SHA_INTR_SOURCE                   61
#define RSA_INTR_SOURCE                   62
#define ECC_INTR_SOURCE                   63
#define ECDSA_INTR_SOURCE                 64
#define MAX_INTR_SOURCE                   65

/* Zero will allocate low/medium levels of priority (ESP_INTR_FLAG_LOWMED) */
#define IRQ_DEFAULT_PRIORITY 0

#define ESP_INTR_FLAG_SHARED (1 << 8) /* Interrupt can be shared between ISRs */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32H2_INTMUX_H_ */
