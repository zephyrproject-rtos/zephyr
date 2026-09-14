/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Derived from components/soc/esp32s31/include/soc/gpio_sig_map.h
 */

/**
 * @file
 * @brief GPIO signal map for Espressif ESP32-S31
 *
 * Maps peripheral signals to GPIO matrix input/output indices.
 *
 * @ingroup pinctrl_esp32s31
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP32S31_GPIO_SIGMAP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP32S31_GPIO_SIGMAP_H_

/** @cond INTERNAL_HIDDEN */

#define ESP_NOSIG ESP_SIG_INVAL /**< No signal (invalid) */

#define ESP_U0RXD_IN          10 /**< UART0 RXD input */
#define ESP_U0TXD_OUT         10 /**< UART0 TXD output */
#define ESP_U0CTS_IN          11 /**< UART0 CTS input */
#define ESP_U0RTS_OUT         11 /**< UART0 RTS output */
#define ESP_U0DSR_IN          12 /**< UART0 DSR input */
#define ESP_U0DTR_OUT         12 /**< UART0 DTR output */
#define ESP_U1RXD_IN          13 /**< UART1 RXD input */
#define ESP_U1TXD_OUT         13 /**< UART1 TXD output */
#define ESP_U1CTS_IN          14 /**< UART1 CTS input */
#define ESP_U1RTS_OUT         14 /**< UART1 RTS output */
#define ESP_U1DSR_IN          15 /**< UART1 DSR input */
#define ESP_U1DTR_OUT         15 /**< UART1 DTR output */
#define ESP_U2RXD_IN          16 /**< UART2 RXD input */
#define ESP_U2TXD_OUT         16 /**< UART2 TXD output */
#define ESP_U2CTS_IN          17 /**< UART2 CTS input */
#define ESP_U2RTS_OUT         17 /**< UART2 RTS output */
#define ESP_U2DSR_IN          18 /**< UART2 DSR input */
#define ESP_U2DTR_OUT         18 /**< UART2 DTR output */
#define ESP_FSPICLK_IN        53 /**< FSPI CLK input */
#define ESP_FSPICLK_OUT       53 /**< FSPI CLK output */
#define ESP_FSPIQ_IN          54 /**< FSPI Q input */
#define ESP_FSPIQ_OUT         54 /**< FSPI Q output */
#define ESP_FSPID_IN          55 /**< FSPI D input */
#define ESP_FSPID_OUT         55 /**< FSPI D output */
#define ESP_FSPIHD_IN         56 /**< FSPI HD input */
#define ESP_FSPIHD_OUT        56 /**< FSPI HD output */
#define ESP_FSPIWP_IN         57 /**< FSPI WP input */
#define ESP_FSPIWP_OUT        57 /**< FSPI WP output */
#define ESP_FSPICS0_IN        62 /**< FSPI CS0 input */
#define ESP_FSPICS0_OUT       62 /**< FSPI CS0 output */
#define ESP_FSPICS1_OUT       63 /**< FSPI CS1 output */
#define ESP_FSPICS2_OUT       64 /**< FSPI CS2 output */
#define ESP_FSPICS3_OUT       65 /**< FSPI CS3 output */
#define ESP_FSPICS4_OUT       66 /**< FSPI CS4 output */
#define ESP_FSPICS5_OUT       67 /**< FSPI CS5 output */
#define ESP_SPI3_CK_IN        47 /**< SPI3 CK input */
#define ESP_SPI3_CK_OUT       47 /**< SPI3 CK output */
#define ESP_SPI3_Q_IN         48 /**< SPI3 Q input */
#define ESP_SPI3_QO_OUT       48 /**< SPI3 Q output */
#define ESP_SPI3_D_IN         49 /**< SPI3 D input */
#define ESP_SPI3_D_OUT        49 /**< SPI3 D output */
#define ESP_SPI3_HOLD_IN      50 /**< SPI3 HOLD input */
#define ESP_SPI3_HOLD_OUT     50 /**< SPI3 HOLD output */
#define ESP_SPI3_WP_IN        51 /**< SPI3 WP input */
#define ESP_SPI3_WP_OUT       51 /**< SPI3 WP output */
#define ESP_SPI3_CS_IN        52 /**< SPI3 CS input */
#define ESP_SPI3_CS_OUT       52 /**< SPI3 CS output */
#define ESP_SPI3_CS1_OUT      46 /**< SPI3 CS1 output */
#define ESP_SPI3_CS2_OUT      45 /**< SPI3 CS2 output */
#define ESP_I2CEXT0_SCL_IN    68 /**< I2C0 SCL input */
#define ESP_I2CEXT0_SCL_OUT   68 /**< I2C0 SCL output */
#define ESP_I2CEXT0_SDA_IN    69 /**< I2C0 SDA input */
#define ESP_I2CEXT0_SDA_OUT   69 /**< I2C0 SDA output */
#define ESP_I2CEXT1_SCL_IN    70 /**< I2C1 SCL input */
#define ESP_I2CEXT1_SCL_OUT   70 /**< I2C1 SCL output */
#define ESP_I2CEXT1_SDA_IN    71 /**< I2C1 SDA input */
#define ESP_I2CEXT1_SDA_OUT   71 /**< I2C1 SDA output */
#define ESP_PCNT0_RST_IN0     63 /**< PCNT0 reset input, unit 0 */
#define ESP_PCNT0_RST_IN1     64 /**< PCNT0 reset input, unit 1 */
#define ESP_PCNT0_RST_IN2     65 /**< PCNT0 reset input, unit 2 */
#define ESP_PCNT0_RST_IN3     66 /**< PCNT0 reset input, unit 3 */
#define ESP_PWM0_SYNC0_IN     89 /**< MCPWM0 sync 0 input */
#define ESP_PWM0_OUT0A        89 /**< MCPWM0 output 0A */
#define ESP_PWM0_SYNC1_IN     90 /**< MCPWM0 sync 1 input */
#define ESP_PWM0_OUT0B        90 /**< MCPWM0 output 0B */
#define ESP_PWM0_SYNC2_IN     91 /**< MCPWM0 sync 2 input */
#define ESP_PWM0_OUT1A        91 /**< MCPWM0 output 1A */
#define ESP_PWM0_F0_IN        92 /**< MCPWM0 fault 0 input */
#define ESP_PWM0_OUT1B        92 /**< MCPWM0 output 1B */
#define ESP_PWM0_F1_IN        93 /**< MCPWM0 fault 1 input */
#define ESP_PWM0_OUT2A        93 /**< MCPWM0 output 2A */
#define ESP_PWM0_F2_IN        94 /**< MCPWM0 fault 2 input */
#define ESP_PWM0_OUT2B        94 /**< MCPWM0 output 2B */
#define ESP_PWM0_CAP0_IN      95 /**< MCPWM0 capture 0 input */
#define ESP_PWM1_OUT0A        95 /**< MCPWM1 output 0A */
#define ESP_PWM0_CAP1_IN      96 /**< MCPWM0 capture 1 input */
#define ESP_PWM1_OUT0B        96 /**< MCPWM1 output 0B */
#define ESP_PWM0_CAP2_IN      97 /**< MCPWM0 capture 2 input */
#define ESP_PWM1_OUT1A        97 /**< MCPWM1 output 1A */
#define ESP_PWM1_SYNC0_IN     98 /**< MCPWM1 sync 0 input */
#define ESP_PWM1_OUT1B        98 /**< MCPWM1 output 1B */
#define ESP_PWM1_SYNC1_IN     99 /**< MCPWM1 sync 1 input */
#define ESP_PWM1_OUT2A        99 /**< MCPWM1 output 2A */
#define ESP_PWM1_SYNC2_IN     100 /**< MCPWM1 sync 2 input */
#define ESP_PWM1_OUT2B        100 /**< MCPWM1 output 2B */
#define ESP_PWM1_F0_IN        101 /**< MCPWM1 fault 0 input */
#define ESP_PWM1_F1_IN        102 /**< MCPWM1 fault 1 input */
#define ESP_PWM1_F2_IN        103 /**< MCPWM1 fault 2 input */
#define ESP_PWM1_CAP0_IN      104 /**< MCPWM1 capture 0 input */
#define ESP_PWM1_CAP1_IN      105 /**< MCPWM1 capture 1 input */
#define ESP_PWM1_CAP2_IN      106 /**< MCPWM1 capture 2 input */
#define ESP_TWAI0_RX          80 /**< TWAI0 RX input */
#define ESP_TWAI0_TX          80 /**< TWAI0 TX output */
#define ESP_TWAI1_RX          81 /**< TWAI1 RX input */
#define ESP_TWAI1_TX          83 /**< TWAI1 TX output */
#define ESP_LEDC_LS_SIG_OUT0  126 /**< LEDC channel 0 output */
#define ESP_LEDC_LS_SIG_OUT1  127 /**< LEDC channel 1 output */
#define ESP_LEDC_LS_SIG_OUT2  128 /**< LEDC channel 2 output */
#define ESP_LEDC_LS_SIG_OUT3  129 /**< LEDC channel 3 output */
#define ESP_LEDC_LS_SIG_OUT4  130 /**< LEDC channel 4 output */
#define ESP_LEDC_LS_SIG_OUT5  131 /**< LEDC channel 5 output */
#define ESP_LEDC_LS_SIG_OUT6  132 /**< LEDC channel 6 output */
#define ESP_LEDC_LS_SIG_OUT7  133 /**< LEDC channel 7 output */
#define ESP_PCNT0_SIG_CH0_IN0  141 /**< PCNT0 signal channel 0 input, unit 0 */
#define ESP_PCNT0_SIG_CH0_IN1  142 /**< PCNT0 signal channel 0 input, unit 1 */
#define ESP_PCNT0_SIG_CH0_IN2  143 /**< PCNT0 signal channel 0 input, unit 2 */
#define ESP_PCNT0_SIG_CH0_IN3  144 /**< PCNT0 signal channel 0 input, unit 3 */
#define ESP_PCNT0_SIG_CH1_IN0  145 /**< PCNT0 signal channel 1 input, unit 0 */
#define ESP_PCNT0_SIG_CH1_IN1  146 /**< PCNT0 signal channel 1 input, unit 1 */
#define ESP_PCNT0_SIG_CH1_IN2  147 /**< PCNT0 signal channel 1 input, unit 2 */
#define ESP_PCNT0_SIG_CH1_IN3  148 /**< PCNT0 signal channel 1 input, unit 3 */
#define ESP_PCNT0_CTRL_CH0_IN0 149 /**< PCNT0 control channel 0 input, unit 0 */
#define ESP_PCNT0_CTRL_CH0_IN1 150 /**< PCNT0 control channel 0 input, unit 1 */
#define ESP_PCNT0_CTRL_CH0_IN2 151 /**< PCNT0 control channel 0 input, unit 2 */
#define ESP_PCNT0_CTRL_CH0_IN3 152 /**< PCNT0 control channel 0 input, unit 3 */
#define ESP_PCNT0_CTRL_CH1_IN0 153 /**< PCNT0 control channel 1 input, unit 0 */
#define ESP_PCNT0_CTRL_CH1_IN1 154 /**< PCNT0 control channel 1 input, unit 1 */
#define ESP_PCNT0_CTRL_CH1_IN2 155 /**< PCNT0 control channel 1 input, unit 2 */
#define ESP_PCNT0_CTRL_CH1_IN3 156 /**< PCNT0 control channel 1 input, unit 3 */
#define ESP_RMT_SIG_IN0        246 /**< RMT signal input 0 */
#define ESP_RMT_SIG_OUT0       246 /**< RMT signal output 0 */
#define ESP_RMT_SIG_IN1        247 /**< RMT signal input 1 */
#define ESP_RMT_SIG_OUT1       247 /**< RMT signal output 1 */
#define ESP_RMT_SIG_IN2        248 /**< RMT signal input 2 */
#define ESP_RMT_SIG_OUT2       248 /**< RMT signal output 2 */
#define ESP_RMT_SIG_IN3        249 /**< RMT signal input 3 */
#define ESP_RMT_SIG_OUT3       249 /**< RMT signal output 3 */

/* UART3 */
#define ESP_U3CTS_IN                 138 /**< UART3 CTS input */
#define ESP_U3DSR_IN                 139 /**< UART3 DSR input */
#define ESP_U3DTR_OUT                116 /**< UART3 DTR output */
#define ESP_U3RTS_OUT                138 /**< UART3 RTS output */
#define ESP_U3RXD_IN                 137 /**< UART3 RXD input */
#define ESP_U3TXD_OUT                137 /**< UART3 TXD output */

/* I2S0 */
#define ESP_I2S0I_BCK_IN              29 /**< I2S0 RX BCK input */
#define ESP_I2S0I_BCK_OUT             29 /**< I2S0 RX BCK output */
#define ESP_I2S0I_SD1_IN              43 /**< I2S0 RX SD1 input */
#define ESP_I2S0I_SD2_IN              44 /**< I2S0 RX SD2 input */
#define ESP_I2S0I_SD3_IN              45 /**< I2S0 RX SD3 input */
#define ESP_I2S0I_SD_IN               28 /**< I2S0 RX SD input */
#define ESP_I2S0I_WS_IN               30 /**< I2S0 RX WS input */
#define ESP_I2S0I_WS_OUT              30 /**< I2S0 RX WS output */
#define ESP_I2S0O_BCK_IN              25 /**< I2S0 TX BCK input */
#define ESP_I2S0O_BCK_OUT             25 /**< I2S0 TX BCK output */
#define ESP_I2S0O_SD1_OUT             43 /**< I2S0 TX SD1 output */
#define ESP_I2S0O_SD_OUT              28 /**< I2S0 TX SD output */
#define ESP_I2S0O_WS_IN               27 /**< I2S0 TX WS input */
#define ESP_I2S0O_WS_OUT              27 /**< I2S0 TX WS output */
#define ESP_I2S0_MCLK_IN              26 /**< I2S0 MCLK input */
#define ESP_I2S0_MCLK_OUT             26 /**< I2S0 MCLK output */

/* I2S1 */
#define ESP_I2S1I_BCK_IN              35 /**< I2S1 RX BCK input */
#define ESP_I2S1I_BCK_OUT             35 /**< I2S1 RX BCK output */
#define ESP_I2S1I_SD_IN               34 /**< I2S1 RX SD input */
#define ESP_I2S1I_WS_IN               36 /**< I2S1 RX WS input */
#define ESP_I2S1I_WS_OUT              36 /**< I2S1 RX WS output */
#define ESP_I2S1O_BCK_IN              31 /**< I2S1 TX BCK input */
#define ESP_I2S1O_BCK_OUT             31 /**< I2S1 TX BCK output */
#define ESP_I2S1O_SD_OUT              34 /**< I2S1 TX SD output */
#define ESP_I2S1O_WS_IN               33 /**< I2S1 TX WS input */
#define ESP_I2S1O_WS_OUT              33 /**< I2S1 TX WS output */
#define ESP_I2S1_MCLK_IN              32 /**< I2S1 MCLK input */
#define ESP_I2S1_MCLK_OUT             32 /**< I2S1 MCLK output */

/* MCPWM2 */
#define ESP_PWM2_CAP0_IN             113 /**< MCPWM2 capture 0 input */
#define ESP_PWM2_CAP1_IN             114 /**< MCPWM2 capture 1 input */
#define ESP_PWM2_CAP2_IN             115 /**< MCPWM2 capture 2 input */
#define ESP_PWM2_F0_IN               110 /**< MCPWM2 fault 0 input */
#define ESP_PWM2_F1_IN               111 /**< MCPWM2 fault 1 input */
#define ESP_PWM2_F2_IN               112 /**< MCPWM2 fault 2 input */
#define ESP_PWM2_OUT0A                86 /**< MCPWM2 output 0A */
#define ESP_PWM2_OUT0B                87 /**< MCPWM2 output 0B */
#define ESP_PWM2_OUT1A                88 /**< MCPWM2 output 1A */
#define ESP_PWM2_OUT1B               110 /**< MCPWM2 output 1B */
#define ESP_PWM2_OUT2A               111 /**< MCPWM2 output 2A */
#define ESP_PWM2_OUT2B               112 /**< MCPWM2 output 2B */
#define ESP_PWM2_SYNC0_IN             86 /**< MCPWM2 sync 0 input */
#define ESP_PWM2_SYNC1_IN             87 /**< MCPWM2 sync 1 input */
#define ESP_PWM2_SYNC2_IN             88 /**< MCPWM2 sync 2 input */

/* MCPWM3 */
#define ESP_PWM3_CAP0_IN             125 /**< MCPWM3 capture 0 input */
#define ESP_PWM3_CAP1_IN             134 /**< MCPWM3 capture 1 input */
#define ESP_PWM3_CAP2_IN             135 /**< MCPWM3 capture 2 input */
#define ESP_PWM3_F0_IN               122 /**< MCPWM3 fault 0 input */
#define ESP_PWM3_F1_IN               123 /**< MCPWM3 fault 1 input */
#define ESP_PWM3_F2_IN               124 /**< MCPWM3 fault 2 input */
#define ESP_PWM3_OUT0A               113 /**< MCPWM3 output 0A */
#define ESP_PWM3_OUT0B               114 /**< MCPWM3 output 0B */
#define ESP_PWM3_OUT1A               115 /**< MCPWM3 output 1A */
#define ESP_PWM3_OUT1B               134 /**< MCPWM3 output 1B */
#define ESP_PWM3_OUT2A               135 /**< MCPWM3 output 2A */
#define ESP_PWM3_OUT2B               136 /**< MCPWM3 output 2B */
#define ESP_PWM3_SYNC0_IN            119 /**< MCPWM3 sync 0 input */
#define ESP_PWM3_SYNC1_IN            120 /**< MCPWM3 sync 1 input */
#define ESP_PWM3_SYNC2_IN            121 /**< MCPWM3 sync 2 input */

/* LEDC1 */
#define ESP_LEDC1_LS_SIG_OUT0        230 /**< LEDC1 channel 0 output */
#define ESP_LEDC1_LS_SIG_OUT1        231 /**< LEDC1 channel 1 output */
#define ESP_LEDC1_LS_SIG_OUT2        232 /**< LEDC1 channel 2 output */
#define ESP_LEDC1_LS_SIG_OUT3        233 /**< LEDC1 channel 3 output */
#define ESP_LEDC1_LS_SIG_OUT4        234 /**< LEDC1 channel 4 output */
#define ESP_LEDC1_LS_SIG_OUT5        235 /**< LEDC1 channel 5 output */
#define ESP_LEDC1_LS_SIG_OUT6        236 /**< LEDC1 channel 6 output */
#define ESP_LEDC1_LS_SIG_OUT7        237 /**< LEDC1 channel 7 output */

/* PARLIO */
#define ESP_PARLIO_RX_CLK_IN         186 /**< PARLIO RX clock input */
#define ESP_PARLIO_RX_CLK_OUT        186 /**< PARLIO RX clock output */
#define ESP_PARLIO_RX_DATA0_IN       188 /**< PARLIO RX data 0 input */
#define ESP_PARLIO_RX_DATA10_IN      198 /**< PARLIO RX data 10 input */
#define ESP_PARLIO_RX_DATA11_IN      199 /**< PARLIO RX data 11 input */
#define ESP_PARLIO_RX_DATA12_IN      200 /**< PARLIO RX data 12 input */
#define ESP_PARLIO_RX_DATA13_IN      201 /**< PARLIO RX data 13 input */
#define ESP_PARLIO_RX_DATA14_IN      202 /**< PARLIO RX data 14 input */
#define ESP_PARLIO_RX_DATA15_IN      203 /**< PARLIO RX data 15 input */
#define ESP_PARLIO_RX_DATA1_IN       189 /**< PARLIO RX data 1 input */
#define ESP_PARLIO_RX_DATA2_IN       190 /**< PARLIO RX data 2 input */
#define ESP_PARLIO_RX_DATA3_IN       191 /**< PARLIO RX data 3 input */
#define ESP_PARLIO_RX_DATA4_IN       192 /**< PARLIO RX data 4 input */
#define ESP_PARLIO_RX_DATA5_IN       193 /**< PARLIO RX data 5 input */
#define ESP_PARLIO_RX_DATA6_IN       194 /**< PARLIO RX data 6 input */
#define ESP_PARLIO_RX_DATA7_IN       195 /**< PARLIO RX data 7 input */
#define ESP_PARLIO_RX_DATA8_IN       196 /**< PARLIO RX data 8 input */
#define ESP_PARLIO_RX_DATA9_IN       197 /**< PARLIO RX data 9 input */
#define ESP_PARLIO_TX_CLK_IN         187 /**< PARLIO TX clock input */
#define ESP_PARLIO_TX_CLK_OUT        187 /**< PARLIO TX clock output */
#define ESP_PARLIO_TX_CS_OUT         107 /**< PARLIO TX chip select output */
#define ESP_PARLIO_TX_DATA0_OUT      188 /**< PARLIO TX data 0 output */
#define ESP_PARLIO_TX_DATA10_OUT     198 /**< PARLIO TX data 10 output */
#define ESP_PARLIO_TX_DATA11_OUT     199 /**< PARLIO TX data 11 output */
#define ESP_PARLIO_TX_DATA12_OUT     200 /**< PARLIO TX data 12 output */
#define ESP_PARLIO_TX_DATA13_OUT     201 /**< PARLIO TX data 13 output */
#define ESP_PARLIO_TX_DATA14_OUT     202 /**< PARLIO TX data 14 output */
#define ESP_PARLIO_TX_DATA15_OUT     203 /**< PARLIO TX data 15 output */
#define ESP_PARLIO_TX_DATA1_OUT      189 /**< PARLIO TX data 1 output */
#define ESP_PARLIO_TX_DATA2_OUT      190 /**< PARLIO TX data 2 output */
#define ESP_PARLIO_TX_DATA3_OUT      191 /**< PARLIO TX data 3 output */
#define ESP_PARLIO_TX_DATA4_OUT      192 /**< PARLIO TX data 4 output */
#define ESP_PARLIO_TX_DATA5_OUT      193 /**< PARLIO TX data 5 output */
#define ESP_PARLIO_TX_DATA6_OUT      194 /**< PARLIO TX data 6 output */
#define ESP_PARLIO_TX_DATA7_OUT      195 /**< PARLIO TX data 7 output */
#define ESP_PARLIO_TX_DATA8_OUT      196 /**< PARLIO TX data 8 output */
#define ESP_PARLIO_TX_DATA9_OUT      197 /**< PARLIO TX data 9 output */

#define ESP_SIG_GPIO_OUT 256 /**< GPIO output signal */

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP32S31_GPIO_SIGMAP_H_ */
