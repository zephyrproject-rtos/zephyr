/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TL721X_PINCTRL_COMMON_H_
#define ZEPHYR_TL721X_PINCTRL_COMMON_H_

/* IDs for TL721X GPIO functions */

#define TL721X_FUNC_DEFAULT       0
#define TL721X_FUNC_PWM0          1
#define TL721X_FUNC_PWM1          2
#define TL721X_FUNC_PWM2          3
#define TL721X_FUNC_PWM3          4
#define TL721X_FUNC_PWM4          5
#define TL721X_FUNC_PWM5          6
#define TL721X_FUNC_PWM0_N        7
#define TL721X_FUNC_PWM1_N        8
#define TL721X_FUNC_PWM2_N        9
#define TL721X_FUNC_PWM3_N        10
#define TL721X_FUNC_PWM4_N        11
#define TL721X_FUNC_PWM5_N        12
#define TL721X_FUNC_GSPI_CN0_IO   13
#define TL721X_FUNC_GSPI_CK_IO    14
#define TL721X_FUNC_GSPI_IO3_IO   15
#define TL721X_FUNC_GSPI_IO2_IO   16
#define TL721X_FUNC_GSPI_MISO_IO  17
#define TL721X_FUNC_GSPI_MOSI_IO  18
#define TL721X_FUNC_I2C_SCL_IO    19
#define TL721X_FUNC_I2C_SDA_IO    20
#define TL721X_FUNC_UART0_CTS_I   21
#define TL721X_FUNC_UART0_RTS     22
#define TL721X_FUNC_UART0_TX      23
#define TL721X_FUNC_UART0_RTX_IO  24
#define TL721X_FUNC_UART1_CTS_I   25
#define TL721X_FUNC_UART1_RTS     26
#define TL721X_FUNC_UART1_TX      27
#define TL721X_FUNC_UART1_RTX_IO  28
#define TL721X_FUNC_CLK_7816      29
#define TL721X_FUNC_I2S0_BCK_IO   30
#define TL721X_FUNC_I2S0_LR0_IO   31
#define TL721X_FUNC_I2S0_DAT0_IO  32
#define TL721X_FUNC_I2S0_LR1_IO   33
#define TL721X_FUNC_I2S0_DAT1_IO  34
#define TL721X_FUNC_I2S0_CLK      35
#define TL721X_FUNC_I2S1_BCK_IO   36
#define TL721X_FUNC_I2S1_LR0_IO   37
#define TL721X_FUNC_I2S1_DAT0_IO  38
#define TL721X_FUNC_I2S1_LR1_IO   39
#define TL721X_FUNC_I2S1_DAT1_IO  40
#define TL721X_FUNC_I2S1_CLK      41
#define TL721X_FUNC_DMIC0_CLK     42
#define TL721X_FUNC_DMIC0_DAT_I   43
#define TL721X_FUNC_MSPI_CN2      44
#define TL721X_FUNC_MSPI_CN3      45
#define TL721X_FUNC_DBG_PROBE_CLK 46
#define TL721X_FUNC_TX_CYC2PA     47
#define TL721X_FUNC_WIFI_DENY_I   48
#define TL721X_FUNC_BT_ACTIVITY   49
#define TL721X_FUNC_BT_STATUS     50

#define TL721X_FUNC_ATSEL_0    52
#define TL721X_FUNC_ATSEL_1    53
#define TL721X_FUNC_ATSEL_2    54
#define TL721X_FUNC_ATSEL_3    55
#define TL721X_FUNC_RX_CYC2LNA 56

#define TL721X_FUNC_I2C1_SDA_IO  59
#define TL721X_FUNC_I2C1_SCL_IO  60
#define TL721X_FUNC_GSPI_CN1     61
#define TL721X_FUNC_GSPI_CN2     62
#define TL721X_FUNC_GSPI_CN3     63
#define TL721X_FUNC_ATSEL_4      64
#define TL721X_FUNC_ATSEL_5      65
#define TL721X_FUNC_IR_LEARN_I   66
#define TL721X_FUNC_UART2_CTS_I  67
#define TL721X_FUNC_UART2_RTS    68
#define TL721X_FUNC_UART2_TX     69
#define TL721X_FUNC_UART2_RTX_IO 70
#define TL721X_FUNC_SDM0_P       71
#define TL721X_FUNC_SDM0_N       72
#define TL721X_FUNC_SDM1_P       73
#define TL721X_FUNC_SDM1_N       74
#define TL721X_FUNC_I2S2_BCK_IO  75
#define TL721X_FUNC_I2S2_LR0_IO  76
#define TL721X_FUNC_I2S2_DAT0_IO 77
#define TL721X_FUNC_I2S2_LR1_IO  78
#define TL721X_FUNC_I2S2_DAT1_IO 79
#define TL721X_FUNC_I2S2_CLK     80
#define TL721X_FUNC_SSPI_CN_I    81
#define TL721X_FUNC_SSPI_CK_I    82
#define TL721X_FUNC_SSPI_SI_IO   83
#define TL721X_FUNC_SSPI_SO_IO   84
#define TL721X_FUNC_KEYS0_IO     85
#define TL721X_FUNC_PWM_SYNC_I   86
#define TL721X_FUNC_PWM6         87
#define TL721X_FUNC_PWM6_N       88
#define TL721X_FUNC_TMR0_CMP     89
#define TL721X_FUNC_TMR1_CMP     90

#define TL721X_FUNC_LSPI_CN_IO 92

#define TL721X_FUNC_MSPI_CN1 96
#define TL721X_FUNC_RZ_TX    97
#define TL721X_FUNC_SWM_IO   98

/* IDs for GPIO Ports  */

#define TLX_PORT_A 0x00
#define TLX_PORT_B 0x01
#define TLX_PORT_C 0x02
#define TLX_PORT_D 0x03
#define TLX_PORT_E 0x04
#define TLX_PORT_F 0x05
#define TLX_PORT_G 0x06

/* IDs for GPIO Pins */

#define TLX_PIN_0 0x01
#define TLX_PIN_1 0x02
#define TLX_PIN_2 0x04
#define TLX_PIN_3 0x08
#define TLX_PIN_4 0x10
#define TLX_PIN_5 0x20
#define TLX_PIN_6 0x40
#define TLX_PIN_7 0x80

/* TLX pinctrl pull-up/down */

#define TLX_PULL_NONE 0
#define TLX_PULL_DOWN 2
#define TLX_PULL_UP   3

/* Pin function positions */

#define TL721X_PIN_FUNC_POS 0xFF

/* Pin pull up positions */

#define TLX_PIN_0_PULL_UP_EN_POS 0x00
#define TLX_PIN_1_PULL_UP_EN_POS 0x02
#define TLX_PIN_2_PULL_UP_EN_POS 0x04
#define TLX_PIN_3_PULL_UP_EN_POS 0x06
#define TLX_PIN_4_PULL_UP_EN_POS 0x00
#define TLX_PIN_5_PULL_UP_EN_POS 0x02
#define TLX_PIN_6_PULL_UP_EN_POS 0x04
#define TLX_PIN_7_PULL_UP_EN_POS 0x06

/* TL721X pin configuration bit field positions and masks */

#define TLX_PULL_POS    24
#define TLX_PULL_MSK    0x3
#define TLX_FUNC_POS    16
#define TL721X_FUNC_MSK 0xFF
#define TLX_PORT_POS    8
#define TLX_PORT_MSK    0xFF

#define TLX_PIN_POS    0
#define TLX_PIN_MSK    0xFFFF
#define TLX_PIN_ID_MSK 0xFF

#define TL721X_PULL_NONE (TLX_PULL_NONE << (TLX_PULL_POS - TLX_FUNC_POS))
#define TL721X_PULL_DOWN (TLX_PULL_DOWN << (TLX_PULL_POS - TLX_FUNC_POS))
#define TL721X_PULL_UP   (TLX_PULL_UP << (TLX_PULL_POS - TLX_FUNC_POS))

/* Setters and getters */

#define TLX_PINMUX_SET(port, pin, func)                                                            \
	((func << TLX_FUNC_POS) | (port << TLX_PORT_POS) | (pin << TLX_PIN_POS))
#define TLX_PINMUX_GET_PULL(pinmux)   ((pinmux >> TLX_PULL_POS) & TLX_PULL_MSK)
#define TLX_PINMUX_GET_FUNC(pinmux)   ((pinmux >> TLX_FUNC_POS) & TL721X_FUNC_MSK)
#define TLX_PINMUX_GET_PIN(pinmux)    ((pinmux >> TLX_PIN_POS) & TLX_PIN_MSK)
#define TLX_PINMUX_GET_PIN_ID(pinmux) ((pinmux >> TLX_PIN_POS) & TLX_PIN_ID_MSK)

#endif /* ZEPHYR_TL721X_PINCTRL_COMMON_H_ */
