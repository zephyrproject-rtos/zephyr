/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBAD_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBAD_PINCTRL_H_

/* PINMUX Function definitions */
#define AMEBA_GPIO        0
#define AMEBA_UART        1
#define AMEBA_UART_RTSCTS 2
#define AMEBA_LOGUART     2
#define AMEBA_SPIM        3
#define AMEBA_SPIS        3
#define AMEBA_RTC         4
#define AMEBA_TIMINPUT    4
#define AMEBA_IR          5
#define AMEBA_SPIF        6
#define AMEBA_I2C         7
#define AMEBA_SDIOD       8
#define AMEBA_SDIOH       8
#define AMEBA_PWM         9
#define AMEBA_PWM_HS      9
#define AMEBA_PWM_LP      10
#define AMEBA_SWD         11
#define AMEBA_I2S         12
#define AMEBA_DMIC        12
#define AMEBA_LCD         13
#define AMEBA_USB         14
#define AMEBA_QDEC        15
#define AMEBA_SGPIO       16
#define AMEBA_RFE         18
#define AMEBA_BTCOEX      19
#define AMEBA_WIFIFW      20
#define AMEBA_EXT_PCM     20
#define AMEBA_EXT_BT      20
#define AMEBA_BB_PIN      21
#define AMEBA_SIC         22
#define AMEBA_TIMINPUT_HS 22
#define AMEBA_DBGPORT     23
#define AMEBA_BBDBG       25
#define AMEBA_EXT32K      28
#define AMEBA_RTCOUT      28
#define AMEBA_KEYSCAN_ROW 29
#define AMEBA_KEYSCAN_COL 30
#define AMEBA_WAKEUP      31

/* Define pins number: bit[14:13] port, bit[12:8] pin, bit[7:0] function ID */
#define AMEBA_PORT_PIN(port, line)       ((((port) - 'A') << 5) + (line))
#define AMEBA_PINMUX(port, line, funcid) (((AMEBA_PORT_PIN(port, line)) << 8) | (funcid))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBAD_PINCTRL_H_ */
