/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_B95_PINCTRL_COMMON_H_
#define ZEPHYR_B95_PINCTRL_COMMON_H_

/* IDs for B95 GPIO functions */

#define B95_FUNC_DEFAULT            0
#define B95_FUNC_PWM0               1
#define B95_FUNC_PWM1               2
#define B95_FUNC_PWM2               3
#define B95_FUNC_PWM3               4
#define B95_FUNC_PWM4               5
#define B95_FUNC_PWM5               6
#define B95_FUNC_PWM0_N             7
#define B95_FUNC_PWM1_N             8
#define B95_FUNC_PWM2_N             9
#define B95_FUNC_PWM3_N             10
#define B95_FUNC_PWM4_N             11
#define B95_FUNC_PWM5_N             12
#define B95_FUNC_GSPI_CN0_IO        13
#define B95_FUNC_GSPI_CK_IO         14
#define B95_FUNC_GSPI_IO3_IO        15
#define B95_FUNC_GSPI_IO2_IO        16
#define B95_FUNC_GSPI_MISO_IO       17
#define B95_FUNC_GSPI_MOSI_IO       18
#define B95_FUNC_I2C_SCL_IO         19
#define B95_FUNC_I2C_SDA_IO         20
#define B95_FUNC_UART0_CTS_I        21
#define B95_FUNC_UART0_RTS          22
#define B95_FUNC_UART0_TX           23
#define B95_FUNC_UART0_RTX_IO       24
#define B95_FUNC_UART1_CTS_I        25
#define B95_FUNC_UART1_RTS          26
#define B95_FUNC_UART1_TX           27
#define B95_FUNC_UART1_RTX_IO       28
#define B95_FUNC_CLK_7816           29
#define B95_FUNC_I2S0_BCK_IO        30
#define B95_FUNC_I2S0_LR0_IO        31
#define B95_FUNC_I2S0_DAT0_IO       32
#define B95_FUNC_I2S0_LR1_IO        33
#define B95_FUNC_I2S0_DAT1_IO       34
#define B95_FUNC_I2S0_CLK           35
#define B95_FUNC_I2S1_BCK_IO        36
#define B95_FUNC_I2S1_LR0_IO        37
#define B95_FUNC_I2S1_DAT0_IO       38
#define B95_FUNC_I2S1_LR1_IO        39
#define B95_FUNC_I2S1_DAT1_IO       40
#define B95_FUNC_I2S1_CLK           41
#define B95_FUNC_DMIC0_CLK          42
#define B95_FUNC_DMIC0_DAT_I        43
#define B95_FUNC_MSPI_CN2           44
#define B95_FUNC_MSPI_CN3           45
#define B95_FUNC_DBG_PROBE_CLK      46
#define B95_FUNC_TX_CYC2PA          47
#define B95_FUNC_WIFI_DENY_I        48
#define B95_FUNC_BT_ACTIVITY        49
#define B95_FUNC_BT_STATUS          50

#define B95_FUNC_ATSEL_0            52
#define B95_FUNC_ATSEL_1            53
#define B95_FUNC_ATSEL_2            54
#define B95_FUNC_ATSEL_3            55
#define B95_FUNC_RX_CYC2LNA         56

#define B95_FUNC_I2C1_SDA_IO        59
#define B95_FUNC_I2C1_SCL_IO        60
#define B95_FUNC_GSPI_CN1           61
#define B95_FUNC_GSPI_CN2           62
#define B95_FUNC_GSPI_CN3           63
#define B95_FUNC_ATSEL_4            64
#define B95_FUNC_ATSEL_5            65
#define B95_FUNC_IR_LEARN_I         66
#define B95_FUNC_UART2_CTS_I        67
#define B95_FUNC_UART2_RTS          68
#define B95_FUNC_UART2_TX           69
#define B95_FUNC_UART2_RTX_IO       70
#define B95_FUNC_SDM0_P             71
#define B95_FUNC_SDM0_N             72
#define B95_FUNC_SDM1_P             73
#define B95_FUNC_SDM1_N             74
#define B95_FUNC_I2S2_BCK_IO        75
#define B95_FUNC_I2S2_LR0_IO        76
#define B95_FUNC_I2S2_DAT0_IO       77
#define B95_FUNC_I2S2_LR1_IO        78
#define B95_FUNC_I2S2_DAT1_IO       79
#define B95_FUNC_I2S2_CLK           80
#define B95_FUNC_SSPI_CN_I          81
#define B95_FUNC_SSPI_CK_I          82
#define B95_FUNC_SSPI_SI_IO         83
#define B95_FUNC_SSPI_SO_IO         84
#define B95_FUNC_KEYS0_IO           85
#define B95_FUNC_PWM_SYNC_I         86
#define B95_FUNC_PWM6               87
#define B95_FUNC_PWM6_N             88
#define B95_FUNC_TMR0_CMP           89
#define B95_FUNC_TMR1_CMP           90


#define B95_FUNC_LSPI_CN_IO         92

#define B95_FUNC_MSPI_CN1           96
#define B95_FUNC_RZ_TX              97
#define B95_FUNC_SWM_IO             98

/* IDs for GPIO Ports  */

#define B9x_PORT_A       0x00
#define B9x_PORT_B       0x01
#define B9x_PORT_C       0x02
#define B9x_PORT_D       0x03
#define B9x_PORT_E       0x04
#define B9x_PORT_F       0x05
#define B9x_PORT_G       0x06

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

#define B95_PIN_FUNC_POS    0xFF

/* Pin pull up positions */

#define B9x_PIN_0_PULL_UP_EN_POS    0x00
#define B9x_PIN_1_PULL_UP_EN_POS    0x02
#define B9x_PIN_2_PULL_UP_EN_POS    0x04
#define B9x_PIN_3_PULL_UP_EN_POS    0x06
#define B9x_PIN_4_PULL_UP_EN_POS    0x00
#define B9x_PIN_5_PULL_UP_EN_POS    0x02
#define B9x_PIN_6_PULL_UP_EN_POS    0x04
#define B9x_PIN_7_PULL_UP_EN_POS    0x06

/* B95 pin configuration bit field positions and masks */

#define B9x_PULL_POS     24
#define B9x_PULL_MSK     0x3
#define B9x_FUNC_POS     16
#define B95_FUNC_MSK     0xFF
#define B9x_PORT_POS     8
#define B9x_PORT_MSK     0xFF

#define B9x_PIN_POS      0
#define B9x_PIN_MSK      0xFFFF
#define B9x_PIN_ID_MSK   0xFF

#define B95_PULL_NONE    (B9x_PULL_NONE << (B9x_PULL_POS - B9x_FUNC_POS))
#define B95_PULL_DOWN    (B9x_PULL_DOWN << (B9x_PULL_POS - B9x_FUNC_POS))
#define B95_PULL_UP      (B9x_PULL_UP << (B9x_PULL_POS - B9x_FUNC_POS))

/* Setters and getters */

#define B9x_PINMUX_SET(port, pin, func)   ((func << B9x_FUNC_POS) | \
					   (port << B9x_PORT_POS) | \
					   (pin << B9x_PIN_POS))
#define B9x_PINMUX_GET_PULL(pinmux)       ((pinmux >> B9x_PULL_POS) & B9x_PULL_MSK)
#define B9x_PINMUX_GET_FUNC(pinmux)       ((pinmux >> B9x_FUNC_POS) & B95_FUNC_MSK)
#define B9x_PINMUX_GET_PIN(pinmux)        ((pinmux >> B9x_PIN_POS) & B9x_PIN_MSK)
#define B9x_PINMUX_GET_PIN_ID(pinmux)     ((pinmux >> B9x_PIN_POS) & B9x_PIN_ID_MSK)

#endif  /* ZEPHYR_B95_PINCTRL_COMMON_H_ */
