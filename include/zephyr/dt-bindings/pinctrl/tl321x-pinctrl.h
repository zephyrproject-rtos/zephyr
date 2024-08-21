/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TL321X_PINCTRL_COMMON_H_
#define ZEPHYR_TL321X_PINCTRL_COMMON_H_

/* IDs for TL321X GPIO functions */

#define TL321X_FUNC_DEFAULT            0
#define TL321X_FUNC_PWM0               1
#define TL321X_FUNC_PWM1               2
#define TL321X_FUNC_PWM2               3
#define TL321X_FUNC_PWM3               4
#define TL321X_FUNC_PWM4               5
#define TL321X_FUNC_PWM5               6
#define TL321X_FUNC_PWM0_N             7
#define TL321X_FUNC_PWM1_N             8
#define TL321X_FUNC_PWM2_N             9
#define TL321X_FUNC_PWM3_N             10
#define TL321X_FUNC_PWM4_N             11
#define TL321X_FUNC_PWM5_N             12
#define TL321X_FUNC_MSPI_CN1           13
#define TL321X_FUNC_MSPI_CN2           14
#define TL321X_FUNC_MSPI_CN3           15
#define TL321X_FUNC_GSPI_CN1           16
#define TL321X_FUNC_GSPI_CN2           17
#define TL321X_FUNC_GSPI_CN3           18
#define TL321X_FUNC_GSPI_CN0_IO        19
#define TL321X_FUNC_GSPI_CK_IO         20
#define TL321X_FUNC_GSPI_IO3_IO        21
#define TL321X_FUNC_GSPI_IO2_IO        22
#define TL321X_FUNC_GSPI_MISO_IO       23
#define TL321X_FUNC_GSPI_MOSI_IO       24
#define TL321X_FUNC_I2C_SCL_IO         25
#define TL321X_FUNC_I2C_SDA_IO         26
#define TL321X_FUNC_I2C1_SDA_IO        27
#define TL321X_FUNC_I2C1_SCL_IO        28
#define TL321X_FUNC_UART0_CTS_I        29
#define TL321X_FUNC_UART0_RTS          30
#define TL321X_FUNC_UART0_TX           31
#define TL321X_FUNC_UART0_RTX_IO       32
#define TL321X_FUNC_UART1_CTS_I        33
#define TL321X_FUNC_UART1_RTS          34
#define TL321X_FUNC_UART1_TX           35
#define TL321X_FUNC_UART1_RTX_IO       36
#define TL321X_FUNC_UART2_CTS_I        37
#define TL321X_FUNC_UART2_RTS          38
#define TL321X_FUNC_UART2_TX           39
#define TL321X_FUNC_UART2_RTX_IO       40
#define TL321X_FUNC_CLK_7816           41

#define TL321X_FUNC_I2S0_BCK_IO        43
#define TL321X_FUNC_I2S0_LR_OUT_IO     44
#define TL321X_FUNC_I2S0_DAT_OUT       45
#define TL321X_FUNC_I2S0_LR_IN_IO      46
#define TL321X_FUNC_I2S0_DAT_IN_I      47
#define TL321X_FUNC_I2S0_CLK           48
#define TL321X_FUNC_DMIC0_CLK          49
#define TL321X_FUNC_DMIC0_DAT_I        50
#define TL321X_FUNC_SDM0_P             51
#define TL321X_FUNC_SDM0_N             52,
#define TL321X_FUNC_SDM1_P             53,
#define TL321X_FUNC_SDM1_N             54,
#define TL321X_FUNC_IR_LEARN_I         55,
#define TL321X_FUNC_SSPI_CN_I          56,
#define TL321X_FUNC_SSPI_CK_I          57,
#define TL321X_FUNC_SSPI_SI_IO         58,
#define TL321X_FUNC_SSPI_SO_IO         59,
#define TL321X_FUNC_KEYS0_IO           60,
#define TL321X_FUNC_TMR0_CMP           61,
#define TL321X_FUNC_TMR1_CMP           62,
#define TL321X_FUNC_ RZ_TX             63,
#define TL321X_FUNC_SWM_IO             64,
#define TL321X_FUNC_TX_CYC2PA          65,
#define TL321X_FUNC_WIFI_DENY_I        66
#define TL321X_FUNC_BT_ACTIVITY        67
#define TL321X_FUNC_BT_STATUS          68
#define TL321X_FUNC_ATSEL_0            69
#define TL321X_FUNC_ATSEL_1            70
#define TL321X_FUNC_ATSEL_2            71
#define TL321X_FUNC_ATSEL_3            72
#define TL321X_FUNC_ATSEL_4            73
#define TL321X_FUNC_ATSEL_5            74
#define TL321X_FUNC_RX_CYC2LNA         75

#define TL321X_FUNC_DBG_BB0            77
#define TL321X_FUNC_DBG_ADC_I_DAT0     78

/* IDs for GPIO Ports  */

#define TLX_PORT_A       0x00
#define TLX_PORT_B       0x01
#define TLX_PORT_C       0x02
#define TLX_PORT_D       0x03
#define TLX_PORT_E       0x04
#define TLX_PORT_F       0x05

/* IDs for GPIO Pins */

#define TLX_PIN_0        0x01
#define TLX_PIN_1        0x02
#define TLX_PIN_2        0x04
#define TLX_PIN_3        0x08
#define TLX_PIN_4        0x10
#define TLX_PIN_5        0x20
#define TLX_PIN_6        0x40
#define TLX_PIN_7        0x80

/* B9x pinctrl pull-up/down */

#define TLX_PULL_NONE    0
#define TLX_PULL_DOWN    2
#define TLX_PULL_UP      3

/* Pin function positions */

#define TL321X_PIN_FUNC_POS    0x3F

/* Pin pull up positions */

#define TLX_PIN_0_PULL_UP_EN_POS    0x00
#define TLX_PIN_1_PULL_UP_EN_POS    0x02
#define TLX_PIN_2_PULL_UP_EN_POS    0x04
#define TLX_PIN_3_PULL_UP_EN_POS    0x06
#define TLX_PIN_4_PULL_UP_EN_POS    0x00
#define TLX_PIN_5_PULL_UP_EN_POS    0x02
#define TLX_PIN_6_PULL_UP_EN_POS    0x04
#define TLX_PIN_7_PULL_UP_EN_POS    0x06

/* TL321X pin configuration bit field positions and masks */

#define TLX_PULL_POS     21
#define TLX_PULL_MSK     0x3
#define TLX_FUNC_POS     16
#define TL321X_FUNC_MSK     0x1F
#define TLX_PORT_POS     8
#define TLX_PORT_MSK     0xFF

#define TLX_PIN_POS      0
#define TLX_PIN_MSK      0xFFFF
#define TLX_PIN_ID_MSK   0xFF

/* Setters and getters */

#define TLX_PINMUX_SET(port, pin, func)   ((func << TLX_FUNC_POS) | \
					   (port << TLX_PORT_POS) | \
					   (pin << TLX_PIN_POS))
#define TLX_PINMUX_GET_PULL(pinmux)       ((pinmux >> TLX_PULL_POS) & TLX_PULL_MSK)
#define TLX_PINMUX_GET_FUNC(pinmux)       ((pinmux >> TLX_FUNC_POS) & TL321X_FUNC_MSK)
#define TLX_PINMUX_GET_PIN(pinmux)        ((pinmux >> TLX_PIN_POS) & TLX_PIN_MSK)
#define TLX_PINMUX_GET_PIN_ID(pinmux)     ((pinmux >> TLX_PIN_POS) & TLX_PIN_ID_MSK)

#endif  /* ZEPHYR_TL321X_PINCTRL_COMMON_H_ */
