/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBASMART_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBASMART_PINCTRL_H_

/**
 * @file
 * @brief Realtek AmebaSmart (RTL8730E) Pinctrl Devicetree bindings
 */

/**
 * @name Pinmux Function definitions
 * @{
 */
#define AMEBA_GPIO          0   /**< GPIO function */
#define AMEBA_UART          1   /**< UART function */
#define AMEBA_LOGUART       2   /**< Log UART function, same ID as AMEBA_UART_RTSCTS */
#define AMEBA_UART_RTSCTS   2   /**< UART RTS/CTS function, same ID as AMEBA_LOGUART */
#define AMEBA_SPI           3   /**< SPI function */
#define AMEBA_RTC           4   /**< RTC function */
#define AMEBA_IR            5   /**< IR function */
#define AMEBA_SPIF          6   /**< SPI Flash function */
#define AMEBA_I2C           7   /**< I2C function */
#define AMEBA_SDIOH         8   /**< SDIO Host function */
#define AMEBA_LEDC          9   /**< LED Controller function */
#define AMEBA_PWM           10  /**< PWM function */
#define AMEBA_SWD           11  /**< SWD function */
#define AMEBA_AUDIO         12  /**< Audio function */
#define AMEBA_I2S0          13  /**< I2S0 function, same ID as AMEBA_I2S1 */
#define AMEBA_I2S1          13  /**< I2S1 function, same ID as AMEBA_I2S0 */
#define AMEBA_I2S2          14  /**< I2S2 function */
#define AMEBA_I2S3          15  /**< I2S3 function */
#define AMEBA_SPK           16  /**< Speaker output function, same ID as AMEBA_AUXIN */
#define AMEBA_AUXIN         16  /**< Analog aux input function, same ID as AMEBA_SPK */
#define AMEBA_DMIC          17  /**< Digital MIC function */
#define AMEBA_CAPTOUCH      18  /**< Capacitive touch function */
#define AMEBA_SIC           19  /**< SIC function */
#define AMEBA_MIPI          20  /**< MIPI function */
#define AMEBA_USB           21  /**< USB function */
#define AMEBA_FEM_C         22  /**< FEM control function, same ID as AMEBA_ANT_SEL */
#define AMEBA_ANT_SEL       22  /**< Antenna select function, same ID as AMEBA_FEM_C */
#define AMEBA_EXT_ZIGBEE    23  /**< External Zigbee function */
#define AMEBA_BT_UART       24  /**< BT UART function */
#define AMEBA_BT_GPIO       25  /**< BT GPIO function */
#define AMEBA_BT_RF         26  /**< BT RF function */
#define AMEBA_DBG_BTCOEX    27  /**< Debug BT coexistence function */
#define AMEBA_TIMINPUT_HS   28  /**< High-speed timer input function */
#define AMEBA_DBGPORT       29  /**< Debug port function */
#define AMEBA_WAKEUP        30  /**< Wakeup function */
#define AMEBA_SWD_EXTRA     32  /**< SWD extra function */
#define AMEBA_SDIO_EXTRA    33  /**< SDIO extra function */
#define AMEBA_I2S2_EXTRA1   34  /**< I2S2 extra DIN1 function */
#define AMEBA_I2S2_EXTRA2   35  /**< I2S2 extra DIN2 function */
#define AMEBA_I2S2_DOUT_EX  36  /**< I2S2 extra DOUT function */
#define AMEBA_I2S3_EXTRA    37  /**< I2S3 extra DIN0 function */
#define AMEBA_I2S3_DOUT_EX  38  /**< I2S3 extra DOUT function */
#define AMEBA_I2S3_DIN_EX   39  /**< I2S3 extra DIN1 function */
/** @} */

/**
 * @brief Encode port and pin number (port 'A'-'D', pin 0-31).
 */
#define AMEBA_PORT_PIN(port, line) ((((port) - 'A') << 5) + (line))

/**
 * @brief Encode pinmux configuration into a single 32-bit value.
 *
 * Layout:
 *   - bit[14:13]: port index (port - 'A', valid range: A-D)
 *   - bit[12:8]:  pin index (0-31)
 *   - bit[7:0]:   function ID
 *
 * Encoding matches the decoder in pinctrl_ameba.c.
 */
#define AMEBA_PINMUX(port, line, funcid) (((AMEBA_PORT_PIN(port, line)) << 8) | (funcid))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBASMART_PINCTRL_H_ */
