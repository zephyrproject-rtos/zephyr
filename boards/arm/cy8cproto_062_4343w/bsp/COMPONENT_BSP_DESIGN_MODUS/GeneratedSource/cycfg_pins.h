/*******************************************************************************
 * File Name: cycfg_pins.h
 *
 * Description:
 * Pin configuration
 * This file was automatically generated and should not be modified.
 * Tools Package 2.4.0.5721
 * mtb-pdl-cat1 3.0.0.10651
 * personalities 5.0.0.0
 * udd 3.0.0.1377
 *
 ********************************************************************************
 * Copyright 2021 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ********************************************************************************/

#if !defined(CYCFG_PINS_H)
#define CYCFG_PINS_H

#include "cycfg_notices.h"
#include "cy_gpio.h"
#if defined (CY_USING_HAL)
    #include "cyhal_hwmgr.h"
#endif // defined (CY_USING_HAL)
#include "cycfg_routing.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define CYBSP_WCO_IN_ENABLED 1U
#define CYBSP_WCO_IN_PORT GPIO_PRT0
#define CYBSP_WCO_IN_PORT_NUM 0U
#define CYBSP_WCO_IN_PIN 0U
#define CYBSP_WCO_IN_NUM 0U
#define CYBSP_WCO_IN_DRIVEMODE CY_GPIO_DM_ANALOG
#define CYBSP_WCO_IN_INIT_DRIVESTATE 1
#ifndef ioss_0_port_0_pin_0_HSIOM
    #define ioss_0_port_0_pin_0_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_WCO_IN_HSIOM ioss_0_port_0_pin_0_HSIOM
#define CYBSP_WCO_IN_IRQ ioss_interrupts_gpio_0_IRQn
#if defined (CY_USING_HAL)
    #define CYBSP_WCO_IN_HAL_PORT_PIN P0_0
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WCO_IN P0_0
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WCO_IN_HAL_IRQ CYHAL_GPIO_IRQ_NONE
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WCO_IN_HAL_DIR CYHAL_GPIO_DIR_INPUT
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WCO_IN_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_ANALOG
#endif // defined (CY_USING_HAL)
#define CYBSP_WCO_OUT_ENABLED 1U
#define CYBSP_WCO_OUT_PORT GPIO_PRT0
#define CYBSP_WCO_OUT_PORT_NUM 0U
#define CYBSP_WCO_OUT_PIN 1U
#define CYBSP_WCO_OUT_NUM 1U
#define CYBSP_WCO_OUT_DRIVEMODE CY_GPIO_DM_ANALOG
#define CYBSP_WCO_OUT_INIT_DRIVESTATE 1
#ifndef ioss_0_port_0_pin_1_HSIOM
    #define ioss_0_port_0_pin_1_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_WCO_OUT_HSIOM ioss_0_port_0_pin_1_HSIOM
#define CYBSP_WCO_OUT_IRQ ioss_interrupts_gpio_0_IRQn
#if defined (CY_USING_HAL)
    #define CYBSP_WCO_OUT_HAL_PORT_PIN P0_1
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WCO_OUT P0_1
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WCO_OUT_HAL_IRQ CYHAL_GPIO_IRQ_NONE
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WCO_OUT_HAL_DIR CYHAL_GPIO_DIR_INPUT
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WCO_OUT_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_ANALOG
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SW2 (P0_4)
    #define CYBSP_USER_BTN1 CYBSP_SW2
    #define CYBSP_USER_BTN CYBSP_SW2
    #define CYBSP_WIFI_HOST_WAKE CYBSP_SW2
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_PDM_CLK (P10_4)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_PDM_DATA (P10_5)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_QSPI_SS (P11_2)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_QSPI_D3 (P11_3)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_QSPI_D2 (P11_4)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_QSPI_D1 (P11_5)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_QSPI_D0 (P11_6)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_QSPI_SCK (P11_7)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SDHC_CMD (P12_4)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SDHC_CLK (P12_5)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SDHC_IO0 (P13_0)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SDHC_IO1 (P13_1)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SDHC_IO2 (P13_2)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SDHC_IO3 (P13_3)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SDHC_DETECT (P13_5)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_LED4 (P13_7)
    #define CYBSP_USER_LED1 CYBSP_LED4
    #define CYBSP_USER_LED CYBSP_LED4
#endif // defined (CY_USING_HAL)
#define CYBSP_CSD_TX_ENABLED 1U
#define CYBSP_CS_TX_ENABLED CYBSP_CSD_TX_ENABLED
#define CYBSP_CSD_TX_PORT GPIO_PRT1
#define CYBSP_CS_TX_PORT CYBSP_CSD_TX_PORT
#define CYBSP_CSD_TX_PORT_NUM 1U
#define CYBSP_CS_TX_PORT_NUM CYBSP_CSD_TX_PORT_NUM
#define CYBSP_CSD_TX_PIN 0U
#define CYBSP_CS_TX_PIN CYBSP_CSD_TX_PIN
#define CYBSP_CSD_TX_NUM 0U
#define CYBSP_CS_TX_NUM CYBSP_CSD_TX_NUM
#define CYBSP_CSD_TX_DRIVEMODE CY_GPIO_DM_ANALOG
#define CYBSP_CS_TX_DRIVEMODE CYBSP_CSD_TX_DRIVEMODE
#define CYBSP_CSD_TX_INIT_DRIVESTATE 1
#define CYBSP_CS_TX_INIT_DRIVESTATE CYBSP_CSD_TX_INIT_DRIVESTATE
#ifndef ioss_0_port_1_pin_0_HSIOM
    #define ioss_0_port_1_pin_0_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_CSD_TX_HSIOM ioss_0_port_1_pin_0_HSIOM
#define CYBSP_CS_TX_HSIOM CYBSP_CSD_TX_HSIOM
#define CYBSP_CSD_TX_IRQ ioss_interrupts_gpio_1_IRQn
#define CYBSP_CS_TX_IRQ CYBSP_CSD_TX_IRQ
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_TX_HAL_PORT_PIN P1_0
    #define CYBSP_CS_TX_HAL_PORT_PIN CYBSP_CSD_TX_HAL_PORT_PIN
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_TX P1_0
    #define CYBSP_CS_TX CYBSP_CSD_TX
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_TX_HAL_IRQ CYHAL_GPIO_IRQ_NONE
    #define CYBSP_CS_TX_HAL_IRQ CYBSP_CSD_TX_HAL_IRQ
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_TX_HAL_DIR CYHAL_GPIO_DIR_INPUT
    #define CYBSP_CS_TX_HAL_DIR CYBSP_CSD_TX_HAL_DIR
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_TX_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_ANALOG
    #define CYBSP_CS_TX_HAL_DRIVEMODE CYBSP_CSD_TX_HAL_DRIVEMODE
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WIFI_SDIO_D0 (P2_0)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WIFI_SDIO_D1 (P2_1)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WIFI_SDIO_D2 (P2_2)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WIFI_SDIO_D3 (P2_3)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WIFI_SDIO_CMD (P2_4)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WIFI_SDIO_CLK (P2_5)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_WIFI_WL_REG_ON (P2_6)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_BT_UART_RX (P3_0)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_BT_UART_TX (P3_1)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_BT_UART_RTS (P3_2)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_BT_UART_CTS (P3_3)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_BT_POWER (P3_4)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_BT_DEVICE_WAKE (P3_5)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_BT_HOST_WAKE (P4_0)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_DEBUG_UART_RX (P5_0)
    #define CYBSP_I2S_MCLK CYBSP_DEBUG_UART_RX
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_DEBUG_UART_TX (P5_1)
    #define CYBSP_I2S_TX_SCK CYBSP_DEBUG_UART_TX
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_I2S_TX_WS (P5_2)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_I2S_TX_DATA (P5_3)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_I2S_RX_SCK (P5_4)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_I2S_RX_WS (P5_5)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_I2S_RX_DATA (P5_6)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_I2C_SCL (P6_0)
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_I2C_SDA (P6_1)
#endif // defined (CY_USING_HAL)
#define CYBSP_SWO_ENABLED 1U
#define CYBSP_SWO_PORT GPIO_PRT6
#define CYBSP_SWO_PORT_NUM 6U
#define CYBSP_SWO_PIN 4U
#define CYBSP_SWO_NUM 4U
#define CYBSP_SWO_DRIVEMODE CY_GPIO_DM_STRONG_IN_OFF
#define CYBSP_SWO_INIT_DRIVESTATE 1
#ifndef ioss_0_port_6_pin_4_HSIOM
    #define ioss_0_port_6_pin_4_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_SWO_HSIOM ioss_0_port_6_pin_4_HSIOM
#define CYBSP_SWO_IRQ ioss_interrupts_gpio_6_IRQn
#if defined (CY_USING_HAL)
    #define CYBSP_SWO_HAL_PORT_PIN P6_4
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SWO P6_4
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SWO_HAL_IRQ CYHAL_GPIO_IRQ_NONE
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SWO_HAL_DIR CYHAL_GPIO_DIR_OUTPUT
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SWO_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_STRONG
#endif // defined (CY_USING_HAL)
#define CYBSP_SWDIO_ENABLED 1U
#define CYBSP_SWDIO_PORT GPIO_PRT6
#define CYBSP_SWDIO_PORT_NUM 6U
#define CYBSP_SWDIO_PIN 6U
#define CYBSP_SWDIO_NUM 6U
#define CYBSP_SWDIO_DRIVEMODE CY_GPIO_DM_PULLUP
#define CYBSP_SWDIO_INIT_DRIVESTATE 1
#ifndef ioss_0_port_6_pin_6_HSIOM
    #define ioss_0_port_6_pin_6_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_SWDIO_HSIOM ioss_0_port_6_pin_6_HSIOM
#define CYBSP_SWDIO_IRQ ioss_interrupts_gpio_6_IRQn
#if defined (CY_USING_HAL)
    #define CYBSP_SWDIO_HAL_PORT_PIN P6_6
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SWDIO P6_6
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SWDIO_HAL_IRQ CYHAL_GPIO_IRQ_NONE
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SWDIO_HAL_DIR CYHAL_GPIO_DIR_BIDIRECTIONAL
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SWDIO_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_PULLUP
#endif // defined (CY_USING_HAL)
#define CYBSP_SWDCK_ENABLED 1U
#define CYBSP_SWDCK_PORT GPIO_PRT6
#define CYBSP_SWDCK_PORT_NUM 6U
#define CYBSP_SWDCK_PIN 7U
#define CYBSP_SWDCK_NUM 7U
#define CYBSP_SWDCK_DRIVEMODE CY_GPIO_DM_PULLDOWN
#define CYBSP_SWDCK_INIT_DRIVESTATE 1
#ifndef ioss_0_port_6_pin_7_HSIOM
    #define ioss_0_port_6_pin_7_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_SWDCK_HSIOM ioss_0_port_6_pin_7_HSIOM
#define CYBSP_SWDCK_IRQ ioss_interrupts_gpio_6_IRQn
#if defined (CY_USING_HAL)
    #define CYBSP_SWDCK_HAL_PORT_PIN P6_7
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SWDCK P6_7
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SWDCK_HAL_IRQ CYHAL_GPIO_IRQ_NONE
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SWDCK_HAL_DIR CYHAL_GPIO_DIR_BIDIRECTIONAL
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_SWDCK_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_PULLDOWN
#endif // defined (CY_USING_HAL)
#define CYBSP_CINA_ENABLED 1U
#define CYBSP_CINA_PORT GPIO_PRT7
#define CYBSP_CINA_PORT_NUM 7U
#define CYBSP_CINA_PIN 1U
#define CYBSP_CINA_NUM 1U
#define CYBSP_CINA_DRIVEMODE CY_GPIO_DM_ANALOG
#define CYBSP_CINA_INIT_DRIVESTATE 1
#ifndef ioss_0_port_7_pin_1_HSIOM
    #define ioss_0_port_7_pin_1_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_CINA_HSIOM ioss_0_port_7_pin_1_HSIOM
#define CYBSP_CINA_IRQ ioss_interrupts_gpio_7_IRQn
#if defined (CY_USING_HAL)
    #define CYBSP_CINA_HAL_PORT_PIN P7_1
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CINA P7_1
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CINA_HAL_IRQ CYHAL_GPIO_IRQ_NONE
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CINA_HAL_DIR CYHAL_GPIO_DIR_INPUT
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CINA_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_ANALOG
#endif // defined (CY_USING_HAL)
#define CYBSP_CINB_ENABLED 1U
#define CYBSP_CINB_PORT GPIO_PRT7
#define CYBSP_CINB_PORT_NUM 7U
#define CYBSP_CINB_PIN 2U
#define CYBSP_CINB_NUM 2U
#define CYBSP_CINB_DRIVEMODE CY_GPIO_DM_ANALOG
#define CYBSP_CINB_INIT_DRIVESTATE 1
#ifndef ioss_0_port_7_pin_2_HSIOM
    #define ioss_0_port_7_pin_2_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_CINB_HSIOM ioss_0_port_7_pin_2_HSIOM
#define CYBSP_CINB_IRQ ioss_interrupts_gpio_7_IRQn
#if defined (CY_USING_HAL)
    #define CYBSP_CINB_HAL_PORT_PIN P7_2
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CINB P7_2
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CINB_HAL_IRQ CYHAL_GPIO_IRQ_NONE
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CINB_HAL_DIR CYHAL_GPIO_DIR_INPUT
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CINB_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_ANALOG
#endif // defined (CY_USING_HAL)
#define CYBSP_CMOD_ENABLED 1U
#define CYBSP_CMOD_PORT GPIO_PRT7
#define CYBSP_CMOD_PORT_NUM 7U
#define CYBSP_CMOD_PIN 7U
#define CYBSP_CMOD_NUM 7U
#define CYBSP_CMOD_DRIVEMODE CY_GPIO_DM_ANALOG
#define CYBSP_CMOD_INIT_DRIVESTATE 1
#ifndef ioss_0_port_7_pin_7_HSIOM
    #define ioss_0_port_7_pin_7_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_CMOD_HSIOM ioss_0_port_7_pin_7_HSIOM
#define CYBSP_CMOD_IRQ ioss_interrupts_gpio_7_IRQn
#if defined (CY_USING_HAL)
    #define CYBSP_CMOD_HAL_PORT_PIN P7_7
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CMOD P7_7
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CMOD_HAL_IRQ CYHAL_GPIO_IRQ_NONE
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CMOD_HAL_DIR CYHAL_GPIO_DIR_INPUT
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CMOD_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_ANALOG
#endif // defined (CY_USING_HAL)
#define CYBSP_CSD_BTN0_ENABLED 1U
#define CYBSP_CS_BTN0_ENABLED CYBSP_CSD_BTN0_ENABLED
#define CYBSP_CSD_BTN0_PORT GPIO_PRT8
#define CYBSP_CS_BTN0_PORT CYBSP_CSD_BTN0_PORT
#define CYBSP_CSD_BTN0_PORT_NUM 8U
#define CYBSP_CS_BTN0_PORT_NUM CYBSP_CSD_BTN0_PORT_NUM
#define CYBSP_CSD_BTN0_PIN 1U
#define CYBSP_CS_BTN0_PIN CYBSP_CSD_BTN0_PIN
#define CYBSP_CSD_BTN0_NUM 1U
#define CYBSP_CS_BTN0_NUM CYBSP_CSD_BTN0_NUM
#define CYBSP_CSD_BTN0_DRIVEMODE CY_GPIO_DM_ANALOG
#define CYBSP_CS_BTN0_DRIVEMODE CYBSP_CSD_BTN0_DRIVEMODE
#define CYBSP_CSD_BTN0_INIT_DRIVESTATE 1
#define CYBSP_CS_BTN0_INIT_DRIVESTATE CYBSP_CSD_BTN0_INIT_DRIVESTATE
#ifndef ioss_0_port_8_pin_1_HSIOM
    #define ioss_0_port_8_pin_1_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_CSD_BTN0_HSIOM ioss_0_port_8_pin_1_HSIOM
#define CYBSP_CS_BTN0_HSIOM CYBSP_CSD_BTN0_HSIOM
#define CYBSP_CSD_BTN0_IRQ ioss_interrupts_gpio_8_IRQn
#define CYBSP_CS_BTN0_IRQ CYBSP_CSD_BTN0_IRQ
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_BTN0_HAL_PORT_PIN P8_1
    #define CYBSP_CS_BTN0_HAL_PORT_PIN CYBSP_CSD_BTN0_HAL_PORT_PIN
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_BTN0 P8_1
    #define CYBSP_CS_BTN0 CYBSP_CSD_BTN0
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_BTN0_HAL_IRQ CYHAL_GPIO_IRQ_NONE
    #define CYBSP_CS_BTN0_HAL_IRQ CYBSP_CSD_BTN0_HAL_IRQ
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_BTN0_HAL_DIR CYHAL_GPIO_DIR_INPUT
    #define CYBSP_CS_BTN0_HAL_DIR CYBSP_CSD_BTN0_HAL_DIR
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_BTN0_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_ANALOG
    #define CYBSP_CS_BTN0_HAL_DRIVEMODE CYBSP_CSD_BTN0_HAL_DRIVEMODE
#endif // defined (CY_USING_HAL)
#define CYBSP_CSD_BTN1_ENABLED 1U
#define CYBSP_CS_BTN1_ENABLED CYBSP_CSD_BTN1_ENABLED
#define CYBSP_CSD_BTN1_PORT GPIO_PRT8
#define CYBSP_CS_BTN1_PORT CYBSP_CSD_BTN1_PORT
#define CYBSP_CSD_BTN1_PORT_NUM 8U
#define CYBSP_CS_BTN1_PORT_NUM CYBSP_CSD_BTN1_PORT_NUM
#define CYBSP_CSD_BTN1_PIN 2U
#define CYBSP_CS_BTN1_PIN CYBSP_CSD_BTN1_PIN
#define CYBSP_CSD_BTN1_NUM 2U
#define CYBSP_CS_BTN1_NUM CYBSP_CSD_BTN1_NUM
#define CYBSP_CSD_BTN1_DRIVEMODE CY_GPIO_DM_ANALOG
#define CYBSP_CS_BTN1_DRIVEMODE CYBSP_CSD_BTN1_DRIVEMODE
#define CYBSP_CSD_BTN1_INIT_DRIVESTATE 1
#define CYBSP_CS_BTN1_INIT_DRIVESTATE CYBSP_CSD_BTN1_INIT_DRIVESTATE
#ifndef ioss_0_port_8_pin_2_HSIOM
    #define ioss_0_port_8_pin_2_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_CSD_BTN1_HSIOM ioss_0_port_8_pin_2_HSIOM
#define CYBSP_CS_BTN1_HSIOM CYBSP_CSD_BTN1_HSIOM
#define CYBSP_CSD_BTN1_IRQ ioss_interrupts_gpio_8_IRQn
#define CYBSP_CS_BTN1_IRQ CYBSP_CSD_BTN1_IRQ
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_BTN1_HAL_PORT_PIN P8_2
    #define CYBSP_CS_BTN1_HAL_PORT_PIN CYBSP_CSD_BTN1_HAL_PORT_PIN
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_BTN1 P8_2
    #define CYBSP_CS_BTN1 CYBSP_CSD_BTN1
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_BTN1_HAL_IRQ CYHAL_GPIO_IRQ_NONE
    #define CYBSP_CS_BTN1_HAL_IRQ CYBSP_CSD_BTN1_HAL_IRQ
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_BTN1_HAL_DIR CYHAL_GPIO_DIR_INPUT
    #define CYBSP_CS_BTN1_HAL_DIR CYBSP_CSD_BTN1_HAL_DIR
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_BTN1_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_ANALOG
    #define CYBSP_CS_BTN1_HAL_DRIVEMODE CYBSP_CSD_BTN1_HAL_DRIVEMODE
#endif // defined (CY_USING_HAL)
#define CYBSP_CSD_SLD0_ENABLED 1U
#define CYBSP_CS_SLD0_ENABLED CYBSP_CSD_SLD0_ENABLED
#define CYBSP_CSD_SLD0_PORT GPIO_PRT8
#define CYBSP_CS_SLD0_PORT CYBSP_CSD_SLD0_PORT
#define CYBSP_CSD_SLD0_PORT_NUM 8U
#define CYBSP_CS_SLD0_PORT_NUM CYBSP_CSD_SLD0_PORT_NUM
#define CYBSP_CSD_SLD0_PIN 3U
#define CYBSP_CS_SLD0_PIN CYBSP_CSD_SLD0_PIN
#define CYBSP_CSD_SLD0_NUM 3U
#define CYBSP_CS_SLD0_NUM CYBSP_CSD_SLD0_NUM
#define CYBSP_CSD_SLD0_DRIVEMODE CY_GPIO_DM_ANALOG
#define CYBSP_CS_SLD0_DRIVEMODE CYBSP_CSD_SLD0_DRIVEMODE
#define CYBSP_CSD_SLD0_INIT_DRIVESTATE 1
#define CYBSP_CS_SLD0_INIT_DRIVESTATE CYBSP_CSD_SLD0_INIT_DRIVESTATE
#ifndef ioss_0_port_8_pin_3_HSIOM
    #define ioss_0_port_8_pin_3_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_CSD_SLD0_HSIOM ioss_0_port_8_pin_3_HSIOM
#define CYBSP_CS_SLD0_HSIOM CYBSP_CSD_SLD0_HSIOM
#define CYBSP_CSD_SLD0_IRQ ioss_interrupts_gpio_8_IRQn
#define CYBSP_CS_SLD0_IRQ CYBSP_CSD_SLD0_IRQ
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD0_HAL_PORT_PIN P8_3
    #define CYBSP_CS_SLD0_HAL_PORT_PIN CYBSP_CSD_SLD0_HAL_PORT_PIN
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD0 P8_3
    #define CYBSP_CS_SLD0 CYBSP_CSD_SLD0
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD0_HAL_IRQ CYHAL_GPIO_IRQ_NONE
    #define CYBSP_CS_SLD0_HAL_IRQ CYBSP_CSD_SLD0_HAL_IRQ
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD0_HAL_DIR CYHAL_GPIO_DIR_INPUT
    #define CYBSP_CS_SLD0_HAL_DIR CYBSP_CSD_SLD0_HAL_DIR
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD0_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_ANALOG
    #define CYBSP_CS_SLD0_HAL_DRIVEMODE CYBSP_CSD_SLD0_HAL_DRIVEMODE
#endif // defined (CY_USING_HAL)
#define CYBSP_CSD_SLD1_ENABLED 1U
#define CYBSP_CS_SLD1_ENABLED CYBSP_CSD_SLD1_ENABLED
#define CYBSP_CSD_SLD1_PORT GPIO_PRT8
#define CYBSP_CS_SLD1_PORT CYBSP_CSD_SLD1_PORT
#define CYBSP_CSD_SLD1_PORT_NUM 8U
#define CYBSP_CS_SLD1_PORT_NUM CYBSP_CSD_SLD1_PORT_NUM
#define CYBSP_CSD_SLD1_PIN 4U
#define CYBSP_CS_SLD1_PIN CYBSP_CSD_SLD1_PIN
#define CYBSP_CSD_SLD1_NUM 4U
#define CYBSP_CS_SLD1_NUM CYBSP_CSD_SLD1_NUM
#define CYBSP_CSD_SLD1_DRIVEMODE CY_GPIO_DM_ANALOG
#define CYBSP_CS_SLD1_DRIVEMODE CYBSP_CSD_SLD1_DRIVEMODE
#define CYBSP_CSD_SLD1_INIT_DRIVESTATE 1
#define CYBSP_CS_SLD1_INIT_DRIVESTATE CYBSP_CSD_SLD1_INIT_DRIVESTATE
#ifndef ioss_0_port_8_pin_4_HSIOM
    #define ioss_0_port_8_pin_4_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_CSD_SLD1_HSIOM ioss_0_port_8_pin_4_HSIOM
#define CYBSP_CS_SLD1_HSIOM CYBSP_CSD_SLD1_HSIOM
#define CYBSP_CSD_SLD1_IRQ ioss_interrupts_gpio_8_IRQn
#define CYBSP_CS_SLD1_IRQ CYBSP_CSD_SLD1_IRQ
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD1_HAL_PORT_PIN P8_4
    #define CYBSP_CS_SLD1_HAL_PORT_PIN CYBSP_CSD_SLD1_HAL_PORT_PIN
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD1 P8_4
    #define CYBSP_CS_SLD1 CYBSP_CSD_SLD1
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD1_HAL_IRQ CYHAL_GPIO_IRQ_NONE
    #define CYBSP_CS_SLD1_HAL_IRQ CYBSP_CSD_SLD1_HAL_IRQ
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD1_HAL_DIR CYHAL_GPIO_DIR_INPUT
    #define CYBSP_CS_SLD1_HAL_DIR CYBSP_CSD_SLD1_HAL_DIR
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD1_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_ANALOG
    #define CYBSP_CS_SLD1_HAL_DRIVEMODE CYBSP_CSD_SLD1_HAL_DRIVEMODE
#endif // defined (CY_USING_HAL)
#define CYBSP_CSD_SLD2_ENABLED 1U
#define CYBSP_CS_SLD2_ENABLED CYBSP_CSD_SLD2_ENABLED
#define CYBSP_CSD_SLD2_PORT GPIO_PRT8
#define CYBSP_CS_SLD2_PORT CYBSP_CSD_SLD2_PORT
#define CYBSP_CSD_SLD2_PORT_NUM 8U
#define CYBSP_CS_SLD2_PORT_NUM CYBSP_CSD_SLD2_PORT_NUM
#define CYBSP_CSD_SLD2_PIN 5U
#define CYBSP_CS_SLD2_PIN CYBSP_CSD_SLD2_PIN
#define CYBSP_CSD_SLD2_NUM 5U
#define CYBSP_CS_SLD2_NUM CYBSP_CSD_SLD2_NUM
#define CYBSP_CSD_SLD2_DRIVEMODE CY_GPIO_DM_ANALOG
#define CYBSP_CS_SLD2_DRIVEMODE CYBSP_CSD_SLD2_DRIVEMODE
#define CYBSP_CSD_SLD2_INIT_DRIVESTATE 1
#define CYBSP_CS_SLD2_INIT_DRIVESTATE CYBSP_CSD_SLD2_INIT_DRIVESTATE
#ifndef ioss_0_port_8_pin_5_HSIOM
    #define ioss_0_port_8_pin_5_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_CSD_SLD2_HSIOM ioss_0_port_8_pin_5_HSIOM
#define CYBSP_CS_SLD2_HSIOM CYBSP_CSD_SLD2_HSIOM
#define CYBSP_CSD_SLD2_IRQ ioss_interrupts_gpio_8_IRQn
#define CYBSP_CS_SLD2_IRQ CYBSP_CSD_SLD2_IRQ
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD2_HAL_PORT_PIN P8_5
    #define CYBSP_CS_SLD2_HAL_PORT_PIN CYBSP_CSD_SLD2_HAL_PORT_PIN
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD2 P8_5
    #define CYBSP_CS_SLD2 CYBSP_CSD_SLD2
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD2_HAL_IRQ CYHAL_GPIO_IRQ_NONE
    #define CYBSP_CS_SLD2_HAL_IRQ CYBSP_CSD_SLD2_HAL_IRQ
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD2_HAL_DIR CYHAL_GPIO_DIR_INPUT
    #define CYBSP_CS_SLD2_HAL_DIR CYBSP_CSD_SLD2_HAL_DIR
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD2_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_ANALOG
    #define CYBSP_CS_SLD2_HAL_DRIVEMODE CYBSP_CSD_SLD2_HAL_DRIVEMODE
#endif // defined (CY_USING_HAL)
#define CYBSP_CSD_SLD3_ENABLED 1U
#define CYBSP_CS_SLD3_ENABLED CYBSP_CSD_SLD3_ENABLED
#define CYBSP_CSD_SLD3_PORT GPIO_PRT8
#define CYBSP_CS_SLD3_PORT CYBSP_CSD_SLD3_PORT
#define CYBSP_CSD_SLD3_PORT_NUM 8U
#define CYBSP_CS_SLD3_PORT_NUM CYBSP_CSD_SLD3_PORT_NUM
#define CYBSP_CSD_SLD3_PIN 6U
#define CYBSP_CS_SLD3_PIN CYBSP_CSD_SLD3_PIN
#define CYBSP_CSD_SLD3_NUM 6U
#define CYBSP_CS_SLD3_NUM CYBSP_CSD_SLD3_NUM
#define CYBSP_CSD_SLD3_DRIVEMODE CY_GPIO_DM_ANALOG
#define CYBSP_CS_SLD3_DRIVEMODE CYBSP_CSD_SLD3_DRIVEMODE
#define CYBSP_CSD_SLD3_INIT_DRIVESTATE 1
#define CYBSP_CS_SLD3_INIT_DRIVESTATE CYBSP_CSD_SLD3_INIT_DRIVESTATE
#ifndef ioss_0_port_8_pin_6_HSIOM
    #define ioss_0_port_8_pin_6_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_CSD_SLD3_HSIOM ioss_0_port_8_pin_6_HSIOM
#define CYBSP_CS_SLD3_HSIOM CYBSP_CSD_SLD3_HSIOM
#define CYBSP_CSD_SLD3_IRQ ioss_interrupts_gpio_8_IRQn
#define CYBSP_CS_SLD3_IRQ CYBSP_CSD_SLD3_IRQ
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD3_HAL_PORT_PIN P8_6
    #define CYBSP_CS_SLD3_HAL_PORT_PIN CYBSP_CSD_SLD3_HAL_PORT_PIN
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD3 P8_6
    #define CYBSP_CS_SLD3 CYBSP_CSD_SLD3
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD3_HAL_IRQ CYHAL_GPIO_IRQ_NONE
    #define CYBSP_CS_SLD3_HAL_IRQ CYBSP_CSD_SLD3_HAL_IRQ
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD3_HAL_DIR CYHAL_GPIO_DIR_INPUT
    #define CYBSP_CS_SLD3_HAL_DIR CYBSP_CSD_SLD3_HAL_DIR
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD3_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_ANALOG
    #define CYBSP_CS_SLD3_HAL_DRIVEMODE CYBSP_CSD_SLD3_HAL_DRIVEMODE
#endif // defined (CY_USING_HAL)
#define CYBSP_CSD_SLD4_ENABLED 1U
#define CYBSP_CS_SLD4_ENABLED CYBSP_CSD_SLD4_ENABLED
#define CYBSP_CSD_SLD4_PORT GPIO_PRT8
#define CYBSP_CS_SLD4_PORT CYBSP_CSD_SLD4_PORT
#define CYBSP_CSD_SLD4_PORT_NUM 8U
#define CYBSP_CS_SLD4_PORT_NUM CYBSP_CSD_SLD4_PORT_NUM
#define CYBSP_CSD_SLD4_PIN 7U
#define CYBSP_CS_SLD4_PIN CYBSP_CSD_SLD4_PIN
#define CYBSP_CSD_SLD4_NUM 7U
#define CYBSP_CS_SLD4_NUM CYBSP_CSD_SLD4_NUM
#define CYBSP_CSD_SLD4_DRIVEMODE CY_GPIO_DM_ANALOG
#define CYBSP_CS_SLD4_DRIVEMODE CYBSP_CSD_SLD4_DRIVEMODE
#define CYBSP_CSD_SLD4_INIT_DRIVESTATE 1
#define CYBSP_CS_SLD4_INIT_DRIVESTATE CYBSP_CSD_SLD4_INIT_DRIVESTATE
#ifndef ioss_0_port_8_pin_7_HSIOM
    #define ioss_0_port_8_pin_7_HSIOM HSIOM_SEL_GPIO
#endif
#define CYBSP_CSD_SLD4_HSIOM ioss_0_port_8_pin_7_HSIOM
#define CYBSP_CS_SLD4_HSIOM CYBSP_CSD_SLD4_HSIOM
#define CYBSP_CSD_SLD4_IRQ ioss_interrupts_gpio_8_IRQn
#define CYBSP_CS_SLD4_IRQ CYBSP_CSD_SLD4_IRQ
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD4_HAL_PORT_PIN P8_7
    #define CYBSP_CS_SLD4_HAL_PORT_PIN CYBSP_CSD_SLD4_HAL_PORT_PIN
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD4 P8_7
    #define CYBSP_CS_SLD4 CYBSP_CSD_SLD4
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD4_HAL_IRQ CYHAL_GPIO_IRQ_NONE
    #define CYBSP_CS_SLD4_HAL_IRQ CYBSP_CSD_SLD4_HAL_IRQ
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD4_HAL_DIR CYHAL_GPIO_DIR_INPUT
    #define CYBSP_CS_SLD4_HAL_DIR CYBSP_CSD_SLD4_HAL_DIR
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
    #define CYBSP_CSD_SLD4_HAL_DRIVEMODE CYHAL_GPIO_DRIVE_ANALOG
    #define CYBSP_CS_SLD4_HAL_DRIVEMODE CYBSP_CSD_SLD4_HAL_DRIVEMODE
#endif // defined (CY_USING_HAL)

extern const cy_stc_gpio_pin_config_t CYBSP_WCO_IN_config;
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_WCO_IN_obj;
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_WCO_OUT_config;
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_WCO_OUT_obj;
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_CSD_TX_config;
#define CYBSP_CS_TX_config CYBSP_CSD_TX_config
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_CSD_TX_obj;
    #define CYBSP_CS_TX_obj CYBSP_CSD_TX_obj
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_SWO_config;
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_SWO_obj;
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_SWDIO_config;
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_SWDIO_obj;
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_SWDCK_config;
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_SWDCK_obj;
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_CINA_config;
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_CINA_obj;
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_CINB_config;
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_CINB_obj;
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_CMOD_config;
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_CMOD_obj;
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_CSD_BTN0_config;
#define CYBSP_CS_BTN0_config CYBSP_CSD_BTN0_config
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_CSD_BTN0_obj;
    #define CYBSP_CS_BTN0_obj CYBSP_CSD_BTN0_obj
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_CSD_BTN1_config;
#define CYBSP_CS_BTN1_config CYBSP_CSD_BTN1_config
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_CSD_BTN1_obj;
    #define CYBSP_CS_BTN1_obj CYBSP_CSD_BTN1_obj
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_CSD_SLD0_config;
#define CYBSP_CS_SLD0_config CYBSP_CSD_SLD0_config
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_CSD_SLD0_obj;
    #define CYBSP_CS_SLD0_obj CYBSP_CSD_SLD0_obj
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_CSD_SLD1_config;
#define CYBSP_CS_SLD1_config CYBSP_CSD_SLD1_config
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_CSD_SLD1_obj;
    #define CYBSP_CS_SLD1_obj CYBSP_CSD_SLD1_obj
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_CSD_SLD2_config;
#define CYBSP_CS_SLD2_config CYBSP_CSD_SLD2_config
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_CSD_SLD2_obj;
    #define CYBSP_CS_SLD2_obj CYBSP_CSD_SLD2_obj
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_CSD_SLD3_config;
#define CYBSP_CS_SLD3_config CYBSP_CSD_SLD3_config
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_CSD_SLD3_obj;
    #define CYBSP_CS_SLD3_obj CYBSP_CSD_SLD3_obj
#endif // defined (CY_USING_HAL)
extern const cy_stc_gpio_pin_config_t CYBSP_CSD_SLD4_config;
#define CYBSP_CS_SLD4_config CYBSP_CSD_SLD4_config
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_CSD_SLD4_obj;
    #define CYBSP_CS_SLD4_obj CYBSP_CSD_SLD4_obj
#endif // defined (CY_USING_HAL)

void init_cycfg_pins(void);

#if defined(__cplusplus)
}
#endif


#endif /* CYCFG_PINS_H */
