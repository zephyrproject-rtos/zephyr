/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_W91_PINCTRL_COMMON_H_
#define ZEPHYR_W91_PINCTRL_COMMON_H_

/* IDs for GPIO Pins */
#define W91_PIN_0        0
#define W91_PIN_1        1
#define W91_PIN_2        2
#define W91_PIN_3        3
#define W91_PIN_4        4
#define W91_PIN_5        5
#define W91_PIN_6        6
#define W91_PIN_7        7
#define W91_PIN_8        8
#define W91_PIN_9        9
#define W91_PIN_10       10
#define W91_PIN_11       11
#define W91_PIN_12       12
#define W91_PIN_13       13
#define W91_PIN_14       14
#define W91_PIN_15       15
#define W91_PIN_16       16
#define W91_PIN_17       17
#define W91_PIN_18       18
#define W91_PIN_19       19
#define W91_PIN_20       20
#define W91_PIN_21       21
#define W91_PIN_22       22
#define W91_PIN_23       23
#define W91_PIN_24       24

/* IDs for W91 GPIO functions */
#define W91_FUNC_DEFAULT           0
#define W91_FUNC_UART0_CTS         1
#define W91_FUNC_UART0_RTS         2
#define W91_FUNC_UART0_TXD         3
#define W91_FUNC_UART0_RXD         4
#define W91_FUNC_UART1_CTS         5
#define W91_FUNC_UART1_RTS         6
#define W91_FUNC_UART1_TXD         7
#define W91_FUNC_UART1_RXD         8
#define W91_FUNC_UART2_CTS         9
#define W91_FUNC_UART2_RTS         10
#define W91_FUNC_UART2_TXD         11
#define W91_FUNC_UART2_RXD         12
#define W91_FUNC_PWM_0             13
#define W91_FUNC_PWM_1             14
#define W91_FUNC_PWM_2             15
#define W91_FUNC_PWM_3             16
#define W91_FUNC_PWM_4             17
#define W91_FUNC_PWM_5             18
#define W91_FUNC_QSPI_CLK          19
#define W91_FUNC_QSPI_CS           20
#define W91_FUNC_QSPI_MOSI         21
#define W91_FUNC_QSPI_MISO         22
#define W91_FUNC_QSPI_WP           23
#define W91_FUNC_QSPI_HOLD         24
#define W91_FUNC_SPI1_CLK          25
#define W91_FUNC_SPI1_CS           26
#define W91_FUNC_SPI1_MOSI         27
#define W91_FUNC_SPI1_MISO         28
#define W91_FUNC_SPI1_WP           29
#define W91_FUNC_SPI1_HOLD         30
#define W91_FUNC_SPI2_CLK          31
#define W91_FUNC_SPI2_CS           32
#define W91_FUNC_SPI2_MOSI         33
#define W91_FUNC_SPI2_MISO         34
#define W91_FUNC_SPI2_WP           35
#define W91_FUNC_SPI2_HOLD         36
#define W91_FUNC_I2C0_SCL          37
#define W91_FUNC_I2C0_SDA          38
#define W91_FUNC_I2C1_SCL          39
#define W91_FUNC_I2C1_SDA          40
#define W91_FUNC_I2S_BCK_IO        41
#define W91_FUNC_I2S_LR_OUT_IO     42
#define W91_FUNC_I2S_DAT_OUT       43
#define W91_FUNC_I2S_LR_IN_IO      44
#define W91_FUNC_I2S_DAT_IN_I      45
#define W91_FUNC_I2S_CLK           46
#define W91_FUNC_SDIO_DATA0        47
#define W91_FUNC_SDIO_DATA1        48
#define W91_FUNC_SDIO_DATA2        49
#define W91_FUNC_SDIO_DATA3        50
#define W91_FUNC_SDIO_CLK          51
#define W91_FUNC_SDIO_CMD          52

/* W91 pinctrl pull-up/down */
#define W91_PULL_NONE    0
#define W91_PULL_DOWN    2
#define W91_PULL_UP      3

/* W91 pin configuration bit field positions and masks */
#define W91_PULL_POS     21
#define W91_PULL_MSK     0x3
#define W91_FUNC_POS     16
#define W91_FUNC_MSK     0xFF
#define W91_PIN_POS      0
#define W91_PIN_MSK      0xFFFF
#define W91_PIN_ID_MSK   0xFF

/* Setters and getters */
#define W91_PINMUX_SET(pin, func)   ((func << W91_FUNC_POS) | \
					   (pin << W91_PIN_POS))
#define W91_PINMUX_GET_PULL(pinmux)       ((pinmux >> W91_PULL_POS) & W91_PULL_MSK)
#define W91_PINMUX_GET_FUNC(pinmux)       ((pinmux >> W91_FUNC_POS) & W91_FUNC_MSK)
#define W91_PINMUX_GET_PIN(pinmux)        ((pinmux >> W91_PIN_POS) & W91_PIN_MSK)
#define W91_PINMUX_GET_PIN_ID(pinmux)     ((pinmux >> W91_PIN_POS) & W91_PIN_ID_MSK)

#endif  /* ZEPHYR_W91_PINCTRL_COMMON_H_ */
