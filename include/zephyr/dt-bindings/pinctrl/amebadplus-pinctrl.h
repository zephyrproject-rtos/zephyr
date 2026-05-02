/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBADPLUS_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBADPLUS_PINCTRL_H_

/* PINMUX Function definitions */
#define AMEBA_GPIO        0
#define AMEBA_LOG_UART    1
#define AMEBA_UART        1
#define AMEBA_SPIC0_FLASH 2
#define AMEBA_SPIC1_FLASH 3
#define AMEBA_SPIC1_PSRAM 4
#define AMEBA_OSPI        5
#define AMEBA_QSPI        5
#define AMEBA_ADC         6
#define AMEBA_CAP_TOUCH   6
#define AMEBA_SIC         7
#define AMEBA_SPI         8
#define AMEBA_SWD         9
#define AMEBA_SDIO        10
#define AMEBA_ANT_DIV     11
#define AMEBA_EXT_BT      12
#define AMEBA_BT_IO       13
#define AMEBA_BT          14
#define AMEBA_EXT_ZIGBEE  15
#define AMEBA_TIMER       16
#define AMEBA_USB         17
#define AMEBA_DEBUG       18
#define AMEBA_UART0_TXD   19
#define AMEBA_UART0_RXD   20
#define AMEBA_UART0_CTS   21
#define AMEBA_UART0_RTS   22
#define AMEBA_UART1_TXD   23
#define AMEBA_UART1_RXD   24
#define AMEBA_UART2_TXD   25
#define AMEBA_UART2_RXD   26
#define AMEBA_UART2_CTS   27
#define AMEBA_UART2_RTS   28
#define AMEBA_SPI1_CLK    29
#define AMEBA_SPI1_MISO   30
#define AMEBA_SPI1_MOSI   31
#define AMEBA_SPI1_CS     32
#define AMEBA_LEDC        33
#define AMEBA_I2S0_MCLK   34
#define AMEBA_I2S0_BCLK   35
#define AMEBA_I2S0_WS     36
#define AMEBA_I2S0_DIO0   37
#define AMEBA_I2S0_DIO1   38
#define AMEBA_I2S0_DIO2   39
#define AMEBA_I2S0_DIO3   40
#define AMEBA_I2S1_MCLK   41
#define AMEBA_I2S1_BCLK   42
#define AMEBA_I2S1_WS     43
#define AMEBA_I2S1_DIO0   44
#define AMEBA_I2S1_DIO1   45
#define AMEBA_I2S1_DIO2   46
#define AMEBA_I2S1_DIO3   47
#define AMEBA_I2C0_SCL    48
#define AMEBA_I2C0_SDA    49
#define AMEBA_I2C1_SCL    50
#define AMEBA_I2C1_SDA    51
#define AMEBA_PWM0        52
#define AMEBA_PWM1        53
#define AMEBA_PWM2        54
#define AMEBA_PWM3        55
#define AMEBA_PWM4        56
#define AMEBA_PWM5        57
#define AMEBA_PWM6        58
#define AMEBA_PWM7        59
#define AMEBA_BT_UART_TXD 60
#define AMEBA_BT_UART_RTS 61
#define AMEBA_DMIC_CLK    62
#define AMEBA_DMIC_DATA   63
#define AMEBA_IR_TX       64
#define AMEBA_IR_RX       65
#define AMEBA_KEY_ROW0    66
#define AMEBA_KEY_ROW1    67
#define AMEBA_KEY_ROW2    68
#define AMEBA_KEY_ROW3    69
#define AMEBA_KEY_ROW4    70
#define AMEBA_KEY_ROW5    71
#define AMEBA_KEY_ROW6    72
#define AMEBA_KEY_ROW7    73
#define AMEBA_KEY_COL0    74
#define AMEBA_KEY_COL1    75
#define AMEBA_KEY_COL2    76
#define AMEBA_KEY_COL3    77
#define AMEBA_KEY_COL4    78
#define AMEBA_KEY_COL5    79
#define AMEBA_KEY_COL6    80
#define AMEBA_KEY_COL7    81

/* Define pins number: bit[14:13] port, bit[12:8] pin, bit[7:0] function ID */
#define AMEBA_PORT_PIN(port, line)       ((((port) - 'A') << 5) + (line))
#define AMEBA_PINMUX(port, line, funcid) (((AMEBA_PORT_PIN(port, line)) << 8) | (funcid))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBADPLUS_PINCTRL_H_ */
