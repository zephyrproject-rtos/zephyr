/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_B92_PINCTRL_COMMON_H_
#define ZEPHYR_B92_PINCTRL_COMMON_H_

/* IDs for B92 GPIO functions */

#define B92_FUNC_GSPI_CN3 0x01
#define B92_FUNC_GSPI_CN2 0x02
#define B92_FUNC_GSPI_CN1 0x03
#define B92_FUNC_I2C1_SCL_IO 0x04
#define B92_FUNC_I2C1_SDA_IO 0x05
#define B92_FUNC_RX_CYC2LNA 0x06
#define B92_FUNC_ATSEL_3 0x07
#define B92_FUNC_ATSEL_2 0x08
#define B92_FUNC_ATSEL_1 0x09
#define B92_FUNC_ATSEL_0 0x0A
#define B92_FUNC_BT_INBAND 0x0B
#define B92_FUNC_BT_STATUS 0x0C
#define B92_FUNC_BT_ACTIVITY 0x0D
#define B92_FUNC_WIFI_DENY_I 0x0E
#define B92_FUNC_TX_CYC2PA 0x0F
#define B92_FUNC_DMIC1_DAT_I 0x10
#define B92_FUNC_DMIC1_CLK 0x11
#define B92_FUNC_DMIC0_DAT_I 0x12
#define B92_FUNC_DMIC0_CLK 0x13
#define B92_FUNC_I2S1_CLK 0x14
#define B92_FUNC_I2S1_DAT_IN_I 0x15
#define B92_FUNC_I2S1_LR_IN_IO 0x16
#define B92_FUNC_I2S1_DAT_OUT 0x17
#define B92_FUNC_I2S1_LR_OUT_IO 0x18
#define B92_FUNC_I2S1_BCK_IO 0x19
#define B92_FUNC_I2S0_CLK 0x1A
#define B92_FUNC_I2S0_DAT_IN_I 0x1B
#define B92_FUNC_I2S0_LR_IN_IO 0x1C
#define B92_FUNC_I2S0_DAT_OUT 0x1D
#define B92_FUNC_I2S0_LR_OUT_IO 0x1E
#define B92_FUNC_I2S0_BCK_IO 0x1F
#define B92_FUNC_CLK_7816 0x20
#define B92_FUNC_UART1_RTX_IO 0x21
#define B92_FUNC_UART1_TX 0x22
#define B92_FUNC_UART1_RTS 0x23
#define B92_FUNC_UART1_CTS_I 0x24
#define B92_FUNC_UART0_RTX_IO 0x25
#define B92_FUNC_UART0_TX 0x26
#define B92_FUNC_UART0_RTS 0x27
#define B92_FUNC_UART0_CTS_I 0x28
#define B92_FUNC_I2C_SDA_IO 0x29
#define B92_FUNC_I2C_SCL_IO 0x2A
#define B92_FUNC_GSPI_MOSI_IO 0x2B
#define B92_FUNC_GSPI_MISO_IO 0x2C
#define B92_FUNC_GSPI_IO2_IO 0x2D
#define B92_FUNC_GSPI_IO3_IO 0x2E
#define B92_FUNC_GSPI_CK_IO 0x2F
#define B92_FUNC_GSPI_CN0_IO 0x30
#define B92_FUNC_PWM5_N 0x31
#define B92_FUNC_PWM4_N 0x32
#define B92_FUNC_PWM3_N 0x33
#define B92_FUNC_PWM2_N 0x34
#define B92_FUNC_PWM1_N 0x35
#define B92_FUNC_PWM0_N 0x36
#define B92_FUNC_PWM5 0x37
#define B92_FUNC_PWM4 0x38
#define B92_FUNC_PWM3 0x39
#define B92_FUNC_PWM2 0x3A
#define B92_FUNC_PWM1 0x3B
#define B92_FUNC_PWM0 0x3C

/* IDs for GPIO Ports  */

#define B9x_PORT_A       0x00
#define B9x_PORT_B       0x01
#define B9x_PORT_C       0x02
#define B9x_PORT_D       0x03
#define B9x_PORT_E       0x04

/* IDs for GPIO Pins */

#define B9x_PIN_0        0x01
#define B9x_PIN_1        0x02
#define B9x_PIN_2        0x04
#define B9x_PIN_3        0x08
#define B9x_PIN_4        0x10
#define B9x_PIN_5        0x20
#define B9x_PIN_6        0x40
#define B9x_PIN_7        0x80

/* B92 pinctrl pull-up/down */

#define B9x_PULL_NONE    0
#define B9x_PULL_DOWN    2
#define B9x_PULL_UP      3

/* Pin function positions */

#define B9x_PIN_0_FUNC_POS    0x00
#define B9x_PIN_1_FUNC_POS    0x02
#define B9x_PIN_2_FUNC_POS    0x04
#define B9x_PIN_3_FUNC_POS    0x06
#define B9x_PIN_4_FUNC_POS    0x00
#define B9x_PIN_5_FUNC_POS    0x02
#define B9x_PIN_6_FUNC_POS    0x04
#define B9x_PIN_7_FUNC_POS    0x06

/* B92 pin configuration bit field positions and masks */

#define B9x_PULL_POS     19
#define B9x_PULL_MSK     0x3
#define B9x_FUNC_POS     16
#define B92_FUNC_MSK     0x3F
#define B9x_PORT_POS     8
#define B9x_PORT_MSK     0xFF

#define B9x_PIN_POS      0
#define B9x_PIN_MSK      0xFFFF
#define B9x_PIN_ID_MSK   0xFF

/* Setters and getters */

#define B9x_PINMUX_SET(port, pin, func)   ((func << B9x_FUNC_POS) | \
					   (port << B9x_PORT_POS) | \
					   (pin << B9x_PIN_POS))
#define B9x_PINMUX_GET_PULL(pinmux)       ((pinmux >> B9x_PULL_POS) & B9x_PULL_MSK)
#define B9x_PINMUX_GET_FUNC(pinmux)       ((pinmux >> B9x_FUNC_POS) & B92_FUNC_MSK)
#define B9x_PINMUX_GET_PIN(pinmux)        ((pinmux >> B9x_PIN_POS) & B9x_PIN_MSK)
#define B9x_PINMUX_GET_PIN_ID(pinmux)     ((pinmux >> B9x_PIN_POS) & B9x_PIN_ID_MSK)

#endif  /* ZEPHYR_B92_PINCTRL_COMMON_H_ */
