/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Derived from components/soc/esp32p4/include/soc/gpio_sig_map.h
 */

/**
 * @file
 * @brief GPIO signal map for Espressif ESP32-P4
 *
 * Maps peripheral signals to GPIO matrix input/output indices.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP32P4_GPIO_SIGMAP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP32P4_GPIO_SIGMAP_H_

#define ESP_NOSIG ESP_SIG_INVAL /**< No signal (invalid) */

/* UART0 */
#define ESP_U0RXD_IN  10 /**< UART0 RXD input */
#define ESP_U0TXD_OUT 10 /**< UART0 TXD output */
#define ESP_U0CTS_IN  11 /**< UART0 CTS input */
#define ESP_U0RTS_OUT 11 /**< UART0 RTS output */
#define ESP_U0DSR_IN  12 /**< UART0 DSR input */
#define ESP_U0DTR_OUT 12 /**< UART0 DTR output */

/* UART1 */
#define ESP_U1RXD_IN  13 /**< UART1 RXD input */
#define ESP_U1TXD_OUT 13 /**< UART1 TXD output */
#define ESP_U1CTS_IN  14 /**< UART1 CTS input */
#define ESP_U1RTS_OUT 14 /**< UART1 RTS output */
#define ESP_U1DSR_IN  15 /**< UART1 DSR input */
#define ESP_U1DTR_OUT 15 /**< UART1 DTR output */

/* UART2 */
#define ESP_U2RXD_IN  16 /**< UART2 RXD input */
#define ESP_U2TXD_OUT 16 /**< UART2 TXD output */
#define ESP_U2CTS_IN  17 /**< UART2 CTS input */
#define ESP_U2RTS_OUT 17 /**< UART2 RTS output */
#define ESP_U2DSR_IN  18 /**< UART2 DSR input */
#define ESP_U2DTR_OUT 18 /**< UART2 DTR output */

/* UART3 */
#define ESP_U3RXD_IN  19 /**< UART3 RXD input */
#define ESP_U3TXD_OUT 19 /**< UART3 TXD output */
#define ESP_U3CTS_IN  20 /**< UART3 CTS input */
#define ESP_U3RTS_OUT 20 /**< UART3 RTS output */
#define ESP_U3DSR_IN  21 /**< UART3 DSR input */
#define ESP_U3DTR_OUT 21 /**< UART3 DTR output */

/* UART4 */
#define ESP_U4RXD_IN  22 /**< UART4 RXD input */
#define ESP_U4TXD_OUT 22 /**< UART4 TXD output */
#define ESP_U4CTS_IN  23 /**< UART4 CTS input */
#define ESP_U4RTS_OUT 23 /**< UART4 RTS output */
#define ESP_U4DSR_IN  24 /**< UART4 DSR input */
#define ESP_U4DTR_OUT 24 /**< UART4 DTR output */

/* I2S1 */
#define ESP_I2S1_O_BCK_IN  31 /**< I2S1 output BCK input */
#define ESP_I2S1_O_BCK_OUT 31 /**< I2S1 output BCK output */
#define ESP_I2S1_MCLK_IN   32 /**< I2S1 MCLK input */
#define ESP_I2S1_MCLK_OUT  32 /**< I2S1 MCLK output */
#define ESP_I2S1_O_WS_IN   33 /**< I2S1 output WS input */
#define ESP_I2S1_O_WS_OUT  33 /**< I2S1 output WS output */
#define ESP_I2S1_I_SD_IN   34 /**< I2S1 input SD input */
#define ESP_I2S1_O_SD_OUT  34 /**< I2S1 output SD output */
#define ESP_I2S1_I_BCK_IN  35 /**< I2S1 input BCK input */
#define ESP_I2S1_I_BCK_OUT 35 /**< I2S1 input BCK output */
#define ESP_I2S1_I_WS_IN   36 /**< I2S1 input WS input */
#define ESP_I2S1_I_WS_OUT  36 /**< I2S1 input WS output */

/* I2S2 */
#define ESP_I2S2_O_BCK_IN  37 /**< I2S2 output BCK input */
#define ESP_I2S2_O_BCK_OUT 37 /**< I2S2 output BCK output */
#define ESP_I2S2_MCLK_IN   38 /**< I2S2 MCLK input */
#define ESP_I2S2_MCLK_OUT  38 /**< I2S2 MCLK output */
#define ESP_I2S2_O_WS_IN   39 /**< I2S2 output WS input */
#define ESP_I2S2_O_WS_OUT  39 /**< I2S2 output WS output */
#define ESP_I2S2_I_SD_IN   40 /**< I2S2 input SD input */
#define ESP_I2S2_O_SD_OUT  40 /**< I2S2 output SD output */
#define ESP_I2S2_I_BCK_IN  41 /**< I2S2 input BCK input */
#define ESP_I2S2_I_BCK_OUT 41 /**< I2S2 input BCK output */
#define ESP_I2S2_I_WS_IN   42 /**< I2S2 input WS input */
#define ESP_I2S2_I_WS_OUT  42 /**< I2S2 input WS output */

/* SPI3 */
#define ESP_SPI3_CS2_OUT  45 /**< SPI3 CS2 output */
#define ESP_SPI3_CS1_OUT  46 /**< SPI3 CS1 output */
#define ESP_SPI3_CK_IN    47 /**< SPI3 clock input */
#define ESP_SPI3_CK_OUT   47 /**< SPI3 clock output */
#define ESP_SPI3_Q_IN     48 /**< SPI3 Q input */
#define ESP_SPI3_QO_OUT   48 /**< SPI3 Q output */
#define ESP_SPI3_D_IN     49 /**< SPI3 D input */
#define ESP_SPI3_D_OUT    49 /**< SPI3 D output */
#define ESP_SPI3_HOLD_IN  50 /**< SPI3 HOLD input */
#define ESP_SPI3_HOLD_OUT 50 /**< SPI3 HOLD output */
#define ESP_SPI3_WP_IN    51 /**< SPI3 WP input */
#define ESP_SPI3_WP_OUT   51 /**< SPI3 WP output */
#define ESP_SPI3_CS_IN    52 /**< SPI3 CS input */
#define ESP_SPI3_CS_OUT   52 /**< SPI3 CS output */

/* SPI2 (FSPI) */
#define ESP_FSPICLK_IN  53 /**< FSPI clock input */
#define ESP_FSPICLK_OUT 53 /**< FSPI clock output */
#define ESP_FSPIQ_IN    54 /**< FSPI Q input */
#define ESP_FSPIQ_OUT   54 /**< FSPI Q output */
#define ESP_FSPID_IN    55 /**< FSPI D input */
#define ESP_FSPID_OUT   55 /**< FSPI D output */
#define ESP_FSPIHD_IN   56 /**< FSPI HD input */
#define ESP_FSPIHD_OUT  56 /**< FSPI HD output */
#define ESP_FSPIWP_IN   57 /**< FSPI WP input */
#define ESP_FSPIWP_OUT  57 /**< FSPI WP output */
#define ESP_FSPICS0_IN  62 /**< FSPI CS0 input */
#define ESP_FSPICS0_OUT 62 /**< FSPI CS0 output */

/* I2C0 */
#define ESP_I2CEXT0_SCL_IN  68 /**< I2C0 SCL input */
#define ESP_I2CEXT0_SCL_OUT 68 /**< I2C0 SCL output */
#define ESP_I2CEXT0_SDA_IN  69 /**< I2C0 SDA input */
#define ESP_I2CEXT0_SDA_OUT 69 /**< I2C0 SDA output */

/* I2C1 */
#define ESP_I2CEXT1_SCL_IN  70 /**< I2C1 SCL input */
#define ESP_I2CEXT1_SCL_OUT 70 /**< I2C1 SCL output */
#define ESP_I2CEXT1_SDA_IN  71 /**< I2C1 SDA input */
#define ESP_I2CEXT1_SDA_OUT 71 /**< I2C1 SDA output */

/* I2S0 */
#define ESP_I2S0_O_BCK_IN  72 /**< I2S0 output BCK input */
#define ESP_I2S0_O_BCK_OUT 72 /**< I2S0 output BCK output */
#define ESP_I2S0_MCLK_IN   73 /**< I2S0 MCLK input */
#define ESP_I2S0_MCLK_OUT  73 /**< I2S0 MCLK output */
#define ESP_I2S0_O_WS_IN   74 /**< I2S0 output WS input */
#define ESP_I2S0_O_WS_OUT  74 /**< I2S0 output WS output */
#define ESP_I2S0_I_SD_IN   75 /**< I2S0 input SD input */
#define ESP_I2S0_O_SD_OUT  76 /**< I2S0 output SD output */
#define ESP_I2S0_O_SD1_OUT 77 /**< I2S0 output SD1 output */
#define ESP_I2S0_I_BCK_IN  78 /**< I2S0 input BCK input */
#define ESP_I2S0_I_BCK_OUT 78 /**< I2S0 input BCK output */
#define ESP_I2S0_I_WS_IN   79 /**< I2S0 input WS input */
#define ESP_I2S0_I_WS_OUT  79 /**< I2S0 input WS output */

/* TWAI1 */
#define ESP_TWAI1_RX 83 /**< TWAI1 RX input */
#define ESP_TWAI1_TX 83 /**< TWAI1 TX output */

/* TWAI2 */
#define ESP_TWAI2_RX 86 /**< TWAI2 RX input */
#define ESP_TWAI2_TX 86 /**< TWAI2 TX output */

/* MCPWM0 */
#define ESP_PWM0_SYNC0_IN 89 /**< MCPWM0 sync 0 input */
#define ESP_PWM0_OUT0A    89 /**< MCPWM0 output 0A */
#define ESP_PWM0_SYNC1_IN 90 /**< MCPWM0 sync 1 input */
#define ESP_PWM0_OUT0B    90 /**< MCPWM0 output 0B */
#define ESP_PWM0_SYNC2_IN 91 /**< MCPWM0 sync 2 input */
#define ESP_PWM0_OUT1A    91 /**< MCPWM0 output 1A */
#define ESP_PWM0_F0_IN    92 /**< MCPWM0 fault 0 input */
#define ESP_PWM0_OUT1B    92 /**< MCPWM0 output 1B */
#define ESP_PWM0_F1_IN    93 /**< MCPWM0 fault 1 input */
#define ESP_PWM0_OUT2A    93 /**< MCPWM0 output 2A */
#define ESP_PWM0_F2_IN    94 /**< MCPWM0 fault 2 input */
#define ESP_PWM0_OUT2B    94 /**< MCPWM0 output 2B */
#define ESP_PWM0_CAP0_IN  95 /**< MCPWM0 capture 0 input */
#define ESP_PWM0_CAP1_IN  96 /**< MCPWM0 capture 1 input */
#define ESP_PWM0_CAP2_IN  97 /**< MCPWM0 capture 2 input */

/* TWAI0 */
#define ESP_TWAI0_RX 97 /**< TWAI0 RX input */
#define ESP_TWAI0_TX 96 /**< TWAI0 TX output */

/* LEDC */
#define ESP_LEDC_LS_SIG_OUT0 126 /**< LEDC low-speed signal output 0 */
#define ESP_LEDC_LS_SIG_OUT1 127 /**< LEDC low-speed signal output 1 */
#define ESP_LEDC_LS_SIG_OUT2 128 /**< LEDC low-speed signal output 2 */
#define ESP_LEDC_LS_SIG_OUT3 129 /**< LEDC low-speed signal output 3 */
#define ESP_LEDC_LS_SIG_OUT4 130 /**< LEDC low-speed signal output 4 */
#define ESP_LEDC_LS_SIG_OUT5 131 /**< LEDC low-speed signal output 5 */
#define ESP_LEDC_LS_SIG_OUT6 132 /**< LEDC low-speed signal output 6 */
#define ESP_LEDC_LS_SIG_OUT7 133 /**< LEDC low-speed signal output 7 */

/* EMAC */
#define ESP_EMAC_MDI_I 107 /**< EMAC MDIO data input */
#define ESP_EMAC_MDC_O 108 /**< EMAC MDIO clock output */
#define ESP_EMAC_MDO_O 109 /**< EMAC MDIO data output */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP32P4_GPIO_SIGMAP_H_ */
