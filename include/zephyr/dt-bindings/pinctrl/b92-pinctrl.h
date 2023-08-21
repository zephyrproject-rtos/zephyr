/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_B92_PINCTRL_COMMON_H_
#define ZEPHYR_B92_PINCTRL_COMMON_H_

/* IDs for B92 GPIO functions */

#define B92_FUNC_DEFAULT            0
#define B92_FUNC_PWM0               1
#define B92_FUNC_PWM1               2
#define B92_FUNC_PWM2               3
#define B92_FUNC_PWM3               4
#define B92_FUNC_PWM4               5
#define B92_FUNC_PWM5               6
#define B92_FUNC_PWM0_N             7
#define B92_FUNC_PWM1_N             8
#define B92_FUNC_PWM2_N             9
#define B92_FUNC_PWM3_N             10
#define B92_FUNC_PWM4_N             11
#define B92_FUNC_PWM5_N             12
#define B92_FUNC_GSPI_CN0_IO        13
#define B92_FUNC_GSPI_CK_IO         14
#define B92_FUNC_GSPI_IO3_IO        15
#define B92_FUNC_GSPI_IO2_IO        16
#define B92_FUNC_GSPI_MISO_IO       17
#define B92_FUNC_GSPI_MOSI_IO       18
#define B92_FUNC_I2C_SCL_IO         19
#define B92_FUNC_I2C_SDA_IO         20
#define B92_FUNC_UART0_CTS_I        21
#define B92_FUNC_UART0_RTS          22
#define B92_FUNC_UART0_TX           23
#define B92_FUNC_UART0_RTX_IO       24
#define B92_FUNC_UART1_CTS_I        25
#define B92_FUNC_UART1_RTS          26
#define B92_FUNC_UART1_TX           27
#define B92_FUNC_UART1_RTX_IO       28
#define B92_FUNC_CLK_7816           29
#define B92_FUNC_I2S0_BCK_IO        30
#define B92_FUNC_I2S0_LR_OUT_IO     31
#define B92_FUNC_I2S0_DAT_OUT       32
#define B92_FUNC_I2S0_LR_IN_IO      33
#define B92_FUNC_I2S0_DAT_IN_I      34
#define B92_FUNC_I2S0_CLK           35
#define B92_FUNC_I2S1_BCK_IO        36
#define B92_FUNC_I2S1_LR_OUT_IO     37
#define B92_FUNC_I2S1_DAT_OUT       38
#define B92_FUNC_I2S1_LR_IN_IO      39
#define B92_FUNC_I2S1_DAT_IN_I      40
#define B92_FUNC_I2S1_CLK           41
#define B92_FUNC_DMIC0_CLK          42
#define B92_FUNC_DMIC0_DAT_I        43
#define B92_FUNC_DMIC1_CLK          44
#define B92_FUNC_DMIC1_DAT_I        45
#define B92_FUNC_DBG_PROBE_CLK      46
#define B92_FUNC_TX_CYC2PA          47
#define B92_FUNC_WIFI_DENY_I        48
#define B92_FUNC_BT_ACTIVITY        49
#define B92_FUNC_BT_STATUS          50
#define B92_FUNC_BT_INBAND          51
#define B92_FUNC_ATSEL_0            52
#define B92_FUNC_ATSEL_1            53
#define B92_FUNC_ATSEL_2            54
#define B92_FUNC_ATSEL_3            55
#define B92_FUNC_RX_CYC2LNA         56
#define B92_FUNC_DBG_BB0            57
#define B92_FUNC_DBG_ADC_I_DAT0     58
#define B92_FUNC_I2C1_SDA_IO        59
#define B92_FUNC_I2C1_SCL_IO        60
#define B92_FUNC_GSPI_CN1           61
#define B92_FUNC_GSPI_CN2           62
#define B92_FUNC_GSPI_CN3           63

/* IDs for GPIO Ports  */

#define B9x_PORT_A       0x00
#define B9x_PORT_B       0x01
#define B9x_PORT_C       0x02
#define B9x_PORT_D       0x03
#define B9x_PORT_E       0x04
#define B9x_PORT_F       0x05

/* IDs for GPIO Pins */

#define B9x_PIN_0        0x01
#define B9x_PIN_1        0x02
#define B9x_PIN_2        0x04
#define B9x_PIN_3        0x08
#define B9x_PIN_4        0x10
#define B9x_PIN_5        0x20
#define B9x_PIN_6        0x40
#define B9x_PIN_7        0x80

/* B9x pinctrl pull-up/down */

#define B9x_PULL_NONE    0
#define B9x_PULL_DOWN    2
#define B9x_PULL_UP      3

/* Pin function positions */

#define B92_PIN_FUNC_POS    0x3F

/* Pin pull up positions */

#define B9x_PIN_0_PULL_UP_EN_POS    0x00
#define B9x_PIN_1_PULL_UP_EN_POS    0x02
#define B9x_PIN_2_PULL_UP_EN_POS    0x04
#define B9x_PIN_3_PULL_UP_EN_POS    0x06
#define B9x_PIN_4_PULL_UP_EN_POS    0x00
#define B9x_PIN_5_PULL_UP_EN_POS    0x02
#define B9x_PIN_6_PULL_UP_EN_POS    0x04
#define B9x_PIN_7_PULL_UP_EN_POS    0x06

/* B92 pin configuration bit field positions and masks */

#define B9x_PULL_POS     21
#define B9x_PULL_MSK     0x3
#define B9x_FUNC_POS     16
#define B92_FUNC_MSK     0x1F
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
