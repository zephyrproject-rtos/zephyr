/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP32-C61 GPIO signal map definitions
 * @ingroup pinctrl_esp32c61
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP32C61_GPIO_SIGMAP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP32C61_GPIO_SIGMAP_H_

/** @cond INTERNAL_HIDDEN */

#define ESP_NOSIG ESP_SIG_INVAL /**< No signal (invalid) */

/* LEDC signals */
#define ESP_EXT_ADC_START    0 /**< External ADC start */
#define ESP_LEDC_LS_SIG_OUT0 0 /**< LEDC low-speed signal output 0 */
#define ESP_LEDC_LS_SIG_OUT1 1 /**< LEDC low-speed signal output 1 */
#define ESP_LEDC_LS_SIG_OUT2 2 /**< LEDC low-speed signal output 2 */
#define ESP_LEDC_LS_SIG_OUT3 3 /**< LEDC low-speed signal output 3 */
#define ESP_LEDC_LS_SIG_OUT4 4 /**< LEDC low-speed signal output 4 */
#define ESP_LEDC_LS_SIG_OUT5 5 /**< LEDC low-speed signal output 5 */

/* UART0 signals */
#define ESP_U0RXD_IN  6 /**< UART0 RXD input */
#define ESP_U0TXD_OUT 6 /**< UART0 TXD output */
#define ESP_U0CTS_IN  7 /**< UART0 CTS input */
#define ESP_U0RTS_OUT 7 /**< UART0 RTS output */
#define ESP_U0DSR_IN  8 /**< UART0 DSR input */
#define ESP_U0DTR_OUT 8 /**< UART0 DTR output */

/* UART1 signals */
#define ESP_U1RXD_IN  9  /**< UART1 RXD input */
#define ESP_U1TXD_OUT 9  /**< UART1 TXD output */
#define ESP_U1CTS_IN  10 /**< UART1 CTS input */
#define ESP_U1RTS_OUT 10 /**< UART1 RTS output */
#define ESP_U1DSR_IN  11 /**< UART1 DSR input */
#define ESP_U1DTR_OUT 11 /**< UART1 DTR output */

/* I2S signals */
#define ESP_I2S_MCLK_IN  12 /**< I2S master clock input */
#define ESP_I2S_MCLK_OUT 12 /**< I2S master clock output */
#define ESP_I2SO_BCK_IN  13 /**< I2S output bit clock input */
#define ESP_I2SO_BCK_OUT 13 /**< I2S output bit clock output */
#define ESP_I2SO_WS_IN   14 /**< I2S output word select input */
#define ESP_I2SO_WS_OUT  14 /**< I2S output word select output */
#define ESP_I2SI_SD_IN   15 /**< I2S input serial data input */
#define ESP_I2SO_SD_OUT  15 /**< I2S output serial data output */
#define ESP_I2SI_BCK_IN  16 /**< I2S input bit clock input */
#define ESP_I2SI_BCK_OUT 16 /**< I2S input bit clock output */
#define ESP_I2SI_WS_IN   17 /**< I2S input word select input */
#define ESP_I2SI_WS_OUT  17 /**< I2S input word select output */
#define ESP_I2SO_SD1_OUT 18 /**< I2S output serial data 1 output */

/* CPU test bus signals */
#define ESP_CPU_TESTBUS0 19 /**< CPU test bus 0 */
#define ESP_CPU_TESTBUS1 20 /**< CPU test bus 1 */
#define ESP_CPU_TESTBUS2 21 /**< CPU test bus 2 */
#define ESP_CPU_TESTBUS3 22 /**< CPU test bus 3 */
#define ESP_CPU_TESTBUS4 23 /**< CPU test bus 4 */
#define ESP_CPU_TESTBUS5 24 /**< CPU test bus 5 */
#define ESP_CPU_TESTBUS6 25 /**< CPU test bus 6 */
#define ESP_CPU_TESTBUS7 26 /**< CPU test bus 7 */

/* CPU GPIO signals */
#define ESP_CPU_GPIO_IN0  27 /**< CPU GPIO input 0 */
#define ESP_CPU_GPIO_OUT0 27 /**< CPU GPIO output 0 */
#define ESP_CPU_GPIO_IN1  28 /**< CPU GPIO input 1 */
#define ESP_CPU_GPIO_OUT1 28 /**< CPU GPIO output 1 */
#define ESP_CPU_GPIO_IN2  29 /**< CPU GPIO input 2 */
#define ESP_CPU_GPIO_OUT2 29 /**< CPU GPIO output 2 */
#define ESP_CPU_GPIO_IN3  30 /**< CPU GPIO input 3 */
#define ESP_CPU_GPIO_OUT3 30 /**< CPU GPIO output 3 */
#define ESP_CPU_GPIO_IN4  31 /**< CPU GPIO input 4 */
#define ESP_CPU_GPIO_OUT4 31 /**< CPU GPIO output 4 */
#define ESP_CPU_GPIO_IN5  32 /**< CPU GPIO input 5 */
#define ESP_CPU_GPIO_OUT5 32 /**< CPU GPIO output 5 */
#define ESP_CPU_GPIO_IN6  33 /**< CPU GPIO input 6 */
#define ESP_CPU_GPIO_OUT6 33 /**< CPU GPIO output 6 */
#define ESP_CPU_GPIO_IN7  34 /**< CPU GPIO input 7 */
#define ESP_CPU_GPIO_OUT7 34 /**< CPU GPIO output 7 */

/* USB JTAG signals */
#define ESP_USB_JTAG_TDO     35 /**< USB JTAG TDO */
#define ESP_USB_JTAG_TRST    35 /**< USB JTAG TRST */
#define ESP_USB_JTAG_SRST    36 /**< USB JTAG SRST */
#define ESP_USB_JTAG_TCK     37 /**< USB JTAG TCK */
#define ESP_USB_JTAG_TMS     38 /**< USB JTAG TMS */
#define ESP_USB_JTAG_TDI     39 /**< USB JTAG TDI */
#define ESP_CPU_USB_JTAG_TDO 40 /**< CPU USB JTAG TDO */

/* USB external PHY signals */
#define ESP_USB_EXTPHY_VP     41 /**< USB external PHY VP input */
#define ESP_USB_EXTPHY_OEN    41 /**< USB external PHY OEN output */
#define ESP_USB_EXTPHY_VM     42 /**< USB external PHY VM input */
#define ESP_USB_EXTPHY_SPEED  42 /**< USB external PHY speed output */
#define ESP_USB_EXTPHY_RCV    43 /**< USB external PHY RCV input */
#define ESP_USB_EXTPHY_VPO    43 /**< USB external PHY VPO output */
#define ESP_USB_EXTPHY_VMO    44 /**< USB external PHY VMO output */
#define ESP_USB_EXTPHY_SUSPND 45 /**< USB external PHY suspend output */

/* I2C signals */
#define ESP_I2CEXT0_SCL_IN  46 /**< I2C0 SCL input */
#define ESP_I2CEXT0_SCL_OUT 46 /**< I2C0 SCL output */
#define ESP_I2CEXT0_SDA_IN  47 /**< I2C0 SDA input */
#define ESP_I2CEXT0_SDA_OUT 47 /**< I2C0 SDA output */

/* Antenna select signals */
#define ESP_ANT_SEL0  48 /**< Antenna select 0 */
#define ESP_ANT_SEL1  49 /**< Antenna select 1 */
#define ESP_ANT_SEL2  50 /**< Antenna select 2 */
#define ESP_ANT_SEL3  51 /**< Antenna select 3 */
#define ESP_ANT_SEL4  52 /**< Antenna select 4 */
#define ESP_ANT_SEL5  53 /**< Antenna select 5 */
#define ESP_ANT_SEL6  54 /**< Antenna select 6 */
#define ESP_ANT_SEL7  55 /**< Antenna select 7 */
#define ESP_ANT_SEL8  56 /**< Antenna select 8 */
#define ESP_ANT_SEL9  57 /**< Antenna select 9 */
#define ESP_ANT_SEL10 58 /**< Antenna select 10 */
#define ESP_ANT_SEL11 59 /**< Antenna select 11 */
#define ESP_ANT_SEL12 60 /**< Antenna select 12 */
#define ESP_ANT_SEL13 61 /**< Antenna select 13 */
#define ESP_ANT_SEL14 62 /**< Antenna select 14 */
#define ESP_ANT_SEL15 63 /**< Antenna select 15 */

/* FSPI signals */
#define ESP_FSPICLK_IN  64 /**< FSPI clock input */
#define ESP_FSPICLK_OUT 64 /**< FSPI clock output */
#define ESP_FSPIQ_IN    65 /**< FSPI Q input */
#define ESP_FSPIQ_OUT   65 /**< FSPI Q output */
#define ESP_FSPID_IN    66 /**< FSPI D input */
#define ESP_FSPID_OUT   66 /**< FSPI D output */
#define ESP_FSPIHD_IN   67 /**< FSPI HD input */
#define ESP_FSPIHD_OUT  67 /**< FSPI HD output */
#define ESP_FSPIWP_IN   68 /**< FSPI WP input */
#define ESP_FSPIWP_OUT  68 /**< FSPI WP output */
#define ESP_FSPICS0_IN  69 /**< FSPI CS0 input */
#define ESP_FSPICS0_OUT 69 /**< FSPI CS0 output */

/* UART2 signals */
#define ESP_U2RXD_IN  72 /**< UART2 RXD input */
#define ESP_U2TXD_OUT 72 /**< UART2 TXD output */
#define ESP_U2CTS_IN  73 /**< UART2 CTS input */
#define ESP_U2RTS_OUT 73 /**< UART2 RTS output */
#define ESP_U2DSR_IN  74 /**< UART2 DSR input */
#define ESP_U2DTR_OUT 74 /**< UART2 DTR output */

/* External priority signals */
#define ESP_EXTERN_PRIORITY_I 82 /**< External priority input */
#define ESP_EXTERN_PRIORITY_O 82 /**< External priority output */
#define ESP_EXTERN_ACTIVE_I   83 /**< External active input */
#define ESP_EXTERN_ACTIVE_O   83 /**< External active output */

/* Generic input function signals */
#define ESP_SIG_IN_FUNC97  97  /**< Generic input function 97 */
#define ESP_SIG_IN_FUNC98  98  /**< Generic input function 98 */
#define ESP_SIG_IN_FUNC99  99  /**< Generic input function 99 */
#define ESP_SIG_IN_FUNC100 100 /**< Generic input function 100 */

/* FSPI CS signals */
#define ESP_FSPICS1_OUT 102 /**< FSPI CS1 output */
#define ESP_FSPICS2_OUT 103 /**< FSPI CS2 output */
#define ESP_FSPICS3_OUT 104 /**< FSPI CS3 output */
#define ESP_FSPICS4_OUT 105 /**< FSPI CS4 output */
#define ESP_FSPICS5_OUT 106 /**< FSPI CS5 output */

/* GPIO ETM signals */
#define ESP_GPIO_EVENT_MATRIX_IN0 118 /**< GPIO event matrix input 0 */
#define ESP_GPIO_TASK_MATRIX_OUT0 118 /**< GPIO task matrix output 0 */
#define ESP_GPIO_EVENT_MATRIX_IN1 119 /**< GPIO event matrix input 1 */
#define ESP_GPIO_TASK_MATRIX_OUT1 119 /**< GPIO task matrix output 1 */
#define ESP_GPIO_EVENT_MATRIX_IN2 120 /**< GPIO event matrix input 2 */
#define ESP_GPIO_TASK_MATRIX_OUT2 120 /**< GPIO task matrix output 2 */
#define ESP_GPIO_EVENT_MATRIX_IN3 121 /**< GPIO event matrix input 3 */
#define ESP_GPIO_TASK_MATRIX_OUT3 121 /**< GPIO task matrix output 3 */

/* SDIO signal */
#define ESP_SDIO_TOHOST_INT_OUT 124 /**< SDIO to-host interrupt output */

/* Clock output signals */
#define ESP_CLK_OUT_OUT1 126 /**< Clock output 1 */
#define ESP_CLK_OUT_OUT2 127 /**< Clock output 2 */
#define ESP_CLK_OUT_OUT3 128 /**< Clock output 3 */

/* Modem diagnostic signals */
#define ESP_MODEM_DIAG0  129 /**< Modem diagnostic 0 */
#define ESP_MODEM_DIAG1  130 /**< Modem diagnostic 1 */
#define ESP_MODEM_DIAG2  131 /**< Modem diagnostic 2 */
#define ESP_MODEM_DIAG3  132 /**< Modem diagnostic 3 */
#define ESP_MODEM_DIAG4  133 /**< Modem diagnostic 4 */
#define ESP_MODEM_DIAG5  134 /**< Modem diagnostic 5 */
#define ESP_MODEM_DIAG6  135 /**< Modem diagnostic 6 */
#define ESP_MODEM_DIAG7  136 /**< Modem diagnostic 7 */
#define ESP_MODEM_DIAG8  137 /**< Modem diagnostic 8 */
#define ESP_MODEM_DIAG9  138 /**< Modem diagnostic 9 */
#define ESP_MODEM_DIAG10 139 /**< Modem diagnostic 10 */
#define ESP_MODEM_DIAG11 140 /**< Modem diagnostic 11 */
#define ESP_MODEM_DIAG12 141 /**< Modem diagnostic 12 */
#define ESP_MODEM_DIAG13 142 /**< Modem diagnostic 13 */
#define ESP_MODEM_DIAG14 143 /**< Modem diagnostic 14 */
#define ESP_MODEM_DIAG15 144 /**< Modem diagnostic 15 */
#define ESP_MODEM_DIAG16 145 /**< Modem diagnostic 16 */
#define ESP_MODEM_DIAG17 146 /**< Modem diagnostic 17 */
#define ESP_MODEM_DIAG18 147 /**< Modem diagnostic 18 */
#define ESP_MODEM_DIAG19 148 /**< Modem diagnostic 19 */
#define ESP_MODEM_DIAG20 149 /**< Modem diagnostic 20 */
#define ESP_MODEM_DIAG21 150 /**< Modem diagnostic 21 */
#define ESP_MODEM_DIAG22 151 /**< Modem diagnostic 22 */
#define ESP_MODEM_DIAG23 152 /**< Modem diagnostic 23 */
#define ESP_MODEM_DIAG24 153 /**< Modem diagnostic 24 */
#define ESP_MODEM_DIAG25 154 /**< Modem diagnostic 25 */
#define ESP_MODEM_DIAG26 155 /**< Modem diagnostic 26 */
#define ESP_MODEM_DIAG27 156 /**< Modem diagnostic 27 */
#define ESP_MODEM_DIAG28 157 /**< Modem diagnostic 28 */
#define ESP_MODEM_DIAG29 158 /**< Modem diagnostic 29 */
#define ESP_MODEM_DIAG30 159 /**< Modem diagnostic 30 */
#define ESP_MODEM_DIAG31 160 /**< Modem diagnostic 31 */

#define ESP_SIG_GPIO_OUT 256 /**< GPIO output signal */

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP32C61_GPIO_SIGMAP_H_ */
