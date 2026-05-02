/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP32-C5 GPIO signal map definitions for device tree bindings
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP32C5_GPIO_SIGMAP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP32C5_GPIO_SIGMAP_H_

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

/* Parallel IO signals */
#define ESP_PARL_RX_DATA0 48 /**< Parallel IO RX data 0 */
#define ESP_PARL_TX_DATA0 48 /**< Parallel IO TX data 0 */
#define ESP_PARL_RX_DATA1 49 /**< Parallel IO RX data 1 */
#define ESP_PARL_TX_DATA1 49 /**< Parallel IO TX data 1 */
#define ESP_PARL_RX_DATA2 50 /**< Parallel IO RX data 2 */
#define ESP_PARL_TX_DATA2 50 /**< Parallel IO TX data 2 */
#define ESP_PARL_RX_DATA3 51 /**< Parallel IO RX data 3 */
#define ESP_PARL_TX_DATA3 51 /**< Parallel IO TX data 3 */
#define ESP_PARL_RX_DATA4 52 /**< Parallel IO RX data 4 */
#define ESP_PARL_TX_DATA4 52 /**< Parallel IO TX data 4 */
#define ESP_PARL_RX_DATA5 53 /**< Parallel IO RX data 5 */
#define ESP_PARL_TX_DATA5 53 /**< Parallel IO TX data 5 */
#define ESP_PARL_RX_DATA6 54 /**< Parallel IO RX data 6 */
#define ESP_PARL_TX_DATA6 54 /**< Parallel IO TX data 6 */
#define ESP_PARL_RX_DATA7 55 /**< Parallel IO RX data 7 */
#define ESP_PARL_TX_DATA7 55 /**< Parallel IO TX data 7 */

/* FSPI signals */
#define ESP_FSPICLK_IN  56 /**< FSPI clock input */
#define ESP_FSPICLK_OUT 56 /**< FSPI clock output */
#define ESP_FSPIQ_IN    57 /**< FSPI Q input */
#define ESP_FSPIQ_OUT   57 /**< FSPI Q output */
#define ESP_FSPID_IN    58 /**< FSPI D input */
#define ESP_FSPID_OUT   58 /**< FSPI D output */
#define ESP_FSPIHD_IN   59 /**< FSPI HD input */
#define ESP_FSPIHD_OUT  59 /**< FSPI HD output */
#define ESP_FSPIWP_IN   60 /**< FSPI WP input */
#define ESP_FSPIWP_OUT  60 /**< FSPI WP output */
#define ESP_FSPICS0_IN  61 /**< FSPI CS0 input */
#define ESP_FSPICS0_OUT 61 /**< FSPI CS0 output */

/* Parallel IO clock signals */
#define ESP_PARL_RX_CLK_IN  62 /**< Parallel IO RX clock input */
#define ESP_PARL_RX_CLK_OUT 62 /**< Parallel IO RX clock output */
#define ESP_PARL_TX_CLK_IN  63 /**< Parallel IO TX clock input */
#define ESP_PARL_TX_CLK_OUT 63 /**< Parallel IO TX clock output */

/* RMT signals */
#define ESP_RMT_SIG_IN0  64 /**< RMT signal input 0 */
#define ESP_RMT_SIG_OUT0 64 /**< RMT signal output 0 */
#define ESP_RMT_SIG_IN1  65 /**< RMT signal input 1 */
#define ESP_RMT_SIG_OUT1 65 /**< RMT signal output 1 */

/* TWAI0 signals */
#define ESP_TWAI0_RX         66 /**< TWAI0 RX input */
#define ESP_TWAI0_TX         66 /**< TWAI0 TX output */
#define ESP_TWAI0_BUS_OFF_ON 67 /**< TWAI0 bus off/on output */
#define ESP_TWAI0_CLKOUT     68 /**< TWAI0 clock output */
#define ESP_TWAI0_STANDBY    69 /**< TWAI0 standby output */

/* TWAI1 signals */
#define ESP_TWAI1_RX         70 /**< TWAI1 RX input */
#define ESP_TWAI1_TX         70 /**< TWAI1 TX output */
#define ESP_TWAI1_BUS_OFF_ON 71 /**< TWAI1 bus off/on output */
#define ESP_TWAI1_CLKOUT     72 /**< TWAI1 clock output */
#define ESP_TWAI1_STANDBY    73 /**< TWAI1 standby output */

/* External priority signals */
#define ESP_EXTERN_PRIORITY_I 74 /**< External priority input */
#define ESP_EXTERN_PRIORITY_O 74 /**< External priority output */
#define ESP_EXTERN_ACTIVE_I   75 /**< External active input */
#define ESP_EXTERN_ACTIVE_O   75 /**< External active output */

/* PCNT / GPIO sigma-delta signals */
#define ESP_PCNT_RST_IN0 76 /**< PCNT reset input 0 */
#define ESP_GPIO_SD0_OUT 76 /**< GPIO sigma-delta output 0 */
#define ESP_PCNT_RST_IN1 77 /**< PCNT reset input 1 */
#define ESP_GPIO_SD1_OUT 77 /**< GPIO sigma-delta output 1 */
#define ESP_PCNT_RST_IN2 78 /**< PCNT reset input 2 */
#define ESP_GPIO_SD2_OUT 78 /**< GPIO sigma-delta output 2 */
#define ESP_PCNT_RST_IN3 79 /**< PCNT reset input 3 */
#define ESP_GPIO_SD3_OUT 79 /**< GPIO sigma-delta output 3 */

/* MCPWM signals */
#define ESP_PWM0_SYNC0_IN 80 /**< MCPWM0 sync 0 input */
#define ESP_PWM0_OUT0A    80 /**< MCPWM0 output 0A */
#define ESP_PWM0_SYNC1_IN 81 /**< MCPWM0 sync 1 input */
#define ESP_PWM0_OUT0B    81 /**< MCPWM0 output 0B */
#define ESP_PWM0_SYNC2_IN 82 /**< MCPWM0 sync 2 input */
#define ESP_PWM0_OUT1A    82 /**< MCPWM0 output 1A */
#define ESP_PWM0_F0_IN    83 /**< MCPWM0 fault 0 input */
#define ESP_PWM0_OUT1B    83 /**< MCPWM0 output 1B */
#define ESP_PWM0_F1_IN    84 /**< MCPWM0 fault 1 input */
#define ESP_PWM0_OUT2A    84 /**< MCPWM0 output 2A */
#define ESP_PWM0_F2_IN    85 /**< MCPWM0 fault 2 input */
#define ESP_PWM0_OUT2B    85 /**< MCPWM0 output 2B */
#define ESP_PWM0_CAP0_IN  86 /**< MCPWM0 capture 0 input */
#define ESP_PARL_TX_CS_O  86 /**< Parallel IO TX chip select output */
#define ESP_PWM0_CAP1_IN  87 /**< MCPWM0 capture 1 input */
#define ESP_PWM0_CAP2_IN  88 /**< MCPWM0 capture 2 input */

/* GPIO ETM signals */
#define ESP_GPIO_EVENT_MATRIX_IN0 89 /**< GPIO event matrix input 0 */
#define ESP_GPIO_TASK_MATRIX_OUT0 89 /**< GPIO task matrix output 0 */
#define ESP_GPIO_EVENT_MATRIX_IN1 90 /**< GPIO event matrix input 1 */
#define ESP_GPIO_TASK_MATRIX_OUT1 90 /**< GPIO task matrix output 1 */
#define ESP_GPIO_EVENT_MATRIX_IN2 91 /**< GPIO event matrix input 2 */
#define ESP_GPIO_TASK_MATRIX_OUT2 91 /**< GPIO task matrix output 2 */
#define ESP_GPIO_EVENT_MATRIX_IN3 92 /**< GPIO event matrix input 3 */
#define ESP_GPIO_TASK_MATRIX_OUT3 92 /**< GPIO task matrix output 3 */

/* Clock output signals */
#define ESP_CLK_OUT_OUT1 93 /**< Clock output 1 */
#define ESP_CLK_OUT_OUT2 94 /**< Clock output 2 */
#define ESP_CLK_OUT_OUT3 95 /**< Clock output 3 */

/* SDIO signal */
#define ESP_SDIO_TOHOST_INT_OUT 96 /**< SDIO to-host interrupt output */

/* Generic input function signals */
#define ESP_SIG_IN_FUNC_97  97  /**< Generic input function 97 */
#define ESP_SIG_IN_FUNC97   97  /**< Generic input function 97 (alias) */
#define ESP_SIG_IN_FUNC_98  98  /**< Generic input function 98 */
#define ESP_SIG_IN_FUNC98   98  /**< Generic input function 98 (alias) */
#define ESP_SIG_IN_FUNC_99  99  /**< Generic input function 99 */
#define ESP_SIG_IN_FUNC99   99  /**< Generic input function 99 (alias) */
#define ESP_SIG_IN_FUNC_100 100 /**< Generic input function 100 */
#define ESP_SIG_IN_FUNC100  100 /**< Generic input function 100 (alias) */

/* PCNT / FSPI CS signals */
#define ESP_PCNT_SIG_CH0_IN0  101 /**< PCNT signal channel 0 input 0 */
#define ESP_FSPICS1_OUT       101 /**< FSPI CS1 output */
#define ESP_PCNT_SIG_CH1_IN0  102 /**< PCNT signal channel 1 input 0 */
#define ESP_FSPICS2_OUT       102 /**< FSPI CS2 output */
#define ESP_PCNT_CTRL_CH0_IN0 103 /**< PCNT control channel 0 input 0 */
#define ESP_FSPICS3_OUT       103 /**< FSPI CS3 output */
#define ESP_PCNT_CTRL_CH1_IN0 104 /**< PCNT control channel 1 input 0 */
#define ESP_FSPICS4_OUT       104 /**< FSPI CS4 output */
#define ESP_PCNT_SIG_CH0_IN1  105 /**< PCNT signal channel 0 input 1 */
#define ESP_FSPICS5_OUT       105 /**< FSPI CS5 output */

/* PCNT / Modem diagnostic signals */
#define ESP_PCNT_SIG_CH1_IN1  106 /**< PCNT signal channel 1 input 1 */
#define ESP_MODEM_DIAG0       106 /**< Modem diagnostic 0 */
#define ESP_PCNT_CTRL_CH0_IN1 107 /**< PCNT control channel 0 input 1 */
#define ESP_MODEM_DIAG1       107 /**< Modem diagnostic 1 */
#define ESP_PCNT_CTRL_CH1_IN1 108 /**< PCNT control channel 1 input 1 */
#define ESP_MODEM_DIAG2       108 /**< Modem diagnostic 2 */
#define ESP_PCNT_SIG_CH0_IN2  109 /**< PCNT signal channel 0 input 2 */
#define ESP_MODEM_DIAG3       109 /**< Modem diagnostic 3 */
#define ESP_PCNT_SIG_CH1_IN2  110 /**< PCNT signal channel 1 input 2 */
#define ESP_MODEM_DIAG4       110 /**< Modem diagnostic 4 */
#define ESP_PCNT_CTRL_CH0_IN2 111 /**< PCNT control channel 0 input 2 */
#define ESP_MODEM_DIAG5       111 /**< Modem diagnostic 5 */
#define ESP_PCNT_CTRL_CH1_IN2 112 /**< PCNT control channel 1 input 2 */
#define ESP_MODEM_DIAG6       112 /**< Modem diagnostic 6 */
#define ESP_PCNT_SIG_CH0_IN3  113 /**< PCNT signal channel 0 input 3 */
#define ESP_MODEM_DIAG7       113 /**< Modem diagnostic 7 */
#define ESP_PCNT_SIG_CH1_IN3  114 /**< PCNT signal channel 1 input 3 */
#define ESP_MODEM_DIAG8       114 /**< Modem diagnostic 8 */
#define ESP_PCNT_CTRL_CH0_IN3 115 /**< PCNT control channel 0 input 3 */
#define ESP_MODEM_DIAG9       115 /**< Modem diagnostic 9 */
#define ESP_PCNT_CTRL_CH1_IN3 116 /**< PCNT control channel 1 input 3 */
#define ESP_MODEM_DIAG10      116 /**< Modem diagnostic 10 */

/* Modem diagnostic signals */
#define ESP_MODEM_DIAG11 117 /**< Modem diagnostic 11 */
#define ESP_MODEM_DIAG12 118 /**< Modem diagnostic 12 */
#define ESP_MODEM_DIAG13 119 /**< Modem diagnostic 13 */
#define ESP_MODEM_DIAG14 120 /**< Modem diagnostic 14 */
#define ESP_MODEM_DIAG15 121 /**< Modem diagnostic 15 */
#define ESP_MODEM_DIAG16 122 /**< Modem diagnostic 16 */
#define ESP_MODEM_DIAG17 123 /**< Modem diagnostic 17 */
#define ESP_MODEM_DIAG18 124 /**< Modem diagnostic 18 */
#define ESP_MODEM_DIAG19 125 /**< Modem diagnostic 19 */
#define ESP_MODEM_DIAG20 126 /**< Modem diagnostic 20 */
#define ESP_MODEM_DIAG21 127 /**< Modem diagnostic 21 */
#define ESP_MODEM_DIAG22 128 /**< Modem diagnostic 22 */
#define ESP_MODEM_DIAG23 129 /**< Modem diagnostic 23 */
#define ESP_MODEM_DIAG24 130 /**< Modem diagnostic 24 */
#define ESP_MODEM_DIAG25 131 /**< Modem diagnostic 25 */
#define ESP_MODEM_DIAG26 132 /**< Modem diagnostic 26 */
#define ESP_MODEM_DIAG27 133 /**< Modem diagnostic 27 */
#define ESP_MODEM_DIAG28 134 /**< Modem diagnostic 28 */
#define ESP_MODEM_DIAG29 135 /**< Modem diagnostic 29 */
#define ESP_MODEM_DIAG30 136 /**< Modem diagnostic 30 */
#define ESP_MODEM_DIAG31 137 /**< Modem diagnostic 31 */

/* Antenna select signals */
#define ESP_ANT_SEL0  138 /**< Antenna select 0 */
#define ESP_ANT_SEL1  139 /**< Antenna select 1 */
#define ESP_ANT_SEL2  140 /**< Antenna select 2 */
#define ESP_ANT_SEL3  141 /**< Antenna select 3 */
#define ESP_ANT_SEL4  142 /**< Antenna select 4 */
#define ESP_ANT_SEL5  143 /**< Antenna select 5 */
#define ESP_ANT_SEL6  144 /**< Antenna select 6 */
#define ESP_ANT_SEL7  145 /**< Antenna select 7 */
#define ESP_ANT_SEL8  146 /**< Antenna select 8 */
#define ESP_ANT_SEL9  147 /**< Antenna select 9 */
#define ESP_ANT_SEL10 148 /**< Antenna select 10 */
#define ESP_ANT_SEL11 149 /**< Antenna select 11 */
#define ESP_ANT_SEL12 150 /**< Antenna select 12 */
#define ESP_ANT_SEL13 151 /**< Antenna select 13 */
#define ESP_ANT_SEL14 152 /**< Antenna select 14 */
#define ESP_ANT_SEL15 153 /**< Antenna select 15 */

#define ESP_SIG_GPIO_OUT  256       /**< GPIO output signal */
#define ESP_GPIO_MAP_DATE 0x2311280 /**< GPIO map version date */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP32C5_GPIO_SIGMAP_H_ */
