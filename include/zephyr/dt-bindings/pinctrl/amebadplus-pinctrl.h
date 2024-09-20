/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBADPLUS_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBADPLUS_PINCTRL_H_

/* Define pin modes */
#define AMEBA_GPIO             0x0
#define AMEBA_LOG_UART         0x1
#define AMEBA_UART             0x1
#define AMEBA_SPIC0_FLASH      0x2
#define AMEBA_SPIC1_FLASH      0x3
#define AMEBA_SPIC1_PSRAM      0x4
#define AMEBA_QSPI_OPSI        0x5
#define AMEBA_CAPTOUCH_AUX_ADC 0x6
#define AMEBA_SIC              0x7
#define AMEBA_SPI              0x8
#define AMEBA_SWD              0x9
#define AMEBA_SDIO             0xA
#define AMEBA_ANT_DIV          0xB
#define AMEBA_EXT_BT           0xC
#define AMEBA_BT_IO            0xD
#define AMEBA_BT               0xE
#define AMEBA_EXT_ZIGBEE       0xF
#define AMEBA_TIMER            0x10
#define AMEBA_USB              0x11
#define AMEBA_DEBUG            0x12
#define AMEBA_UART0_TXD        0x13
#define AMEBA_UART0_RXD        0x14
#define AMEBA_UART0_CTS        0x15
#define AMEBA_UART0_RTS        0x16
#define AMEBA_UART1_TXD        0x17
#define AMEBA_UART1_RXD        0x18
#define AMEBA_UART2_TXD        0x19
#define AMEBA_UART2_RXD        0x1A
#define AMEBA_UART2_CTS        0x1B
#define AMEBA_UART2_RTS        0x1C
#define AMEBA_SPI1_CLK         0x1D
#define AMEBA_SPI1_MISO        0x1E
#define AMEBA_SPI1_MOSI        0x1F
#define AMEBA_SPI1_CS          0x20
#define AMEBA_LEDC             0x21
#define AMEBA_I2S0_MCLK        0x22
#define AMEBA_I2S0_BCLK        0x23
#define AMEBA_I2S0_WS          0x24
#define AMEBA_I2S0_DIO0        0x25
#define AMEBA_I2S0_DIO1        0x26
#define AMEBA_I2S0_DIO2        0x27
#define AMEBA_I2S0_DIO3        0x28
#define AMEBA_I2S1_MCLK        0x29
#define AMEBA_I2S1_BCLK        0x2A
#define AMEBA_I2S1_WS          0x2B
#define AMEBA_I2S1_DIO0        0x2C
#define AMEBA_I2S1_DIO1        0x2D
#define AMEBA_I2S1_DIO2        0x2E
#define AMEBA_I2S1_DIO3        0x2F
#define AMEBA_I2C0_SCL         0x30
#define AMEBA_I2C0_SDA         0x31
#define AMEBA_I2C1_SCL         0x32
#define AMEBA_I2C1_SDA         0x33
#define AMEBA_PWM0             0x34
#define AMEBA_PWM1             0x35
#define AMEBA_PWM2             0x36
#define AMEBA_PWM3             0x37
#define AMEBA_PWM4             0x38
#define AMEBA_PWM5             0x39
#define AMEBA_PWM6             0x3A
#define AMEBA_PWM7             0x3B
#define AMEBA_BT_UART_TXD      0x3C
#define AMEBA_BT_UART_RTS      0x3D
#define AMEBA_DMIC_CLK         0x3E
#define AMEBA_DMIC_DATA        0x3F
#define AMEBA_IR_TX            0x40
#define AMEBA_IR_RX            0x41
#define AMEBA_KEY_ROW0         0x42
#define AMEBA_KEY_ROW1         0x43
#define AMEBA_KEY_ROW2         0x44
#define AMEBA_KEY_ROW3         0x45
#define AMEBA_KEY_ROW4         0x46
#define AMEBA_KEY_ROW5         0x47
#define AMEBA_KEY_ROW6         0x48
#define AMEBA_KEY_ROW7         0x49
#define AMEBA_KEY_COL0         0x4A
#define AMEBA_KEY_COL1         0x4B
#define AMEBA_KEY_COL2         0x4C
#define AMEBA_KEY_COL3         0x4D
#define AMEBA_KEY_COL4         0x4E
#define AMEBA_KEY_COL5         0x4F
#define AMEBA_KEY_COL6         0x50
#define AMEBA_KEY_COL7         0x51

/* Define pins number */
#define AMEBA_PORT_PIN(port, line)     ((((port) - 'A') << 5) + (line))
#define AMEBA_PINMUX(port, line, mode) (((AMEBA_PORT_PIN(port, line)) << 8) | (mode))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBADPLUS_PINCTRL_H_ */
