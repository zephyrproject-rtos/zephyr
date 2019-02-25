/**
  ******************************************************************************
  * @file    stm32wbxx_hal_gpio_ex.h
  * @author  MCD Application Team
  * @brief   Header file of GPIO HAL Extended module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32WBxx_HAL_GPIO_EX_H
#define STM32WBxx_HAL_GPIO_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal_def.h"

/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */

/** @defgroup GPIOEx GPIOEx
  * @brief GPIO Extended HAL module driver
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup GPIOEx_Exported_Constants GPIOEx Exported Constants
  * @{
  */

/** @defgroup GPIOEx_Alternate_function_selection GPIOEx Alternate function selection
  * @{
  */


 /* The table below gives an overview of the different alternate functions per port.
  * For more details refer yourself to the product data sheet.
  *
  */

 /*     |   AF0    |   AF1    |   AF2    |   AF3    |   AF4    |   AF5    |   AF6    |   AF7    |
  *_____________________________________________________________________________________________
  *     |SYS_AF    |TIM       |TIM       |SPI/SAI/TI|I2C       | I2C      | RF       |  USART   |
  *_____________________________________________________________________________________________
  * PA0 |          |TIM2_CH1  |          |          |          |          |RF_DTB2   |          |
  * PA1 |          |TIM2_CH2  |          |          |I2C1_SMBA |SPI1_SCK  |RF_DTB3   |          |
  * PA2 |          |TIM2_CH3  |          |          |          |          |RF_DTB4   |          |
  * PA3 |          |TIM2_CH4  |          |SAI1_CK1  |          |          |RF_DTB5   |          |
  * PA4 |          |          |          |          |          |SPI1_NSS  |RF_DTB6   |          |
  * PA5 |          |TIM2_CH1  |TIM2_ETR  |          |          |SPI1_SCK  |RF_DTB7   |          |
  * PA6 |          |TIM1_BKIN |          |          |          |SPI1_MISO |RF_DTB8   |          |
  * PA7 |          |TIM1_CH1N |          |          |I2C3_SCL  |SPI1_MOSI |RF_DTB9   |          |
  * PA8 |MCO       |TIM1_CH1  |          |SAI1_CK2  |          |          |RF_DTB12  |USART1_CK |
  * PA9 |          |TIM2_CH2  |          |SAI1_DI2  |I2C1_SCL  |SPI2_SCK  |RF_DTB13  |USART1_TX |
  * PA10|          |TIM2_CH3  |          |SAI1_DI1  |I2C1_SDA  |          |RF_DTB14  |USART1_RX |
  * PA11|          |TIM2_CH4  |TIM1_BKIN2|          |          |SPI1_MISO |RF_DTB15  |USART1_CTS|
  * PA12|          |TIM2_ETR  |          |          |          |SPI1_MOSI |RF_MISO   |USART1_RTS|
  * PA13|JTMS_SWDIO|          |          |          |          |          |          |          |
  * PA14|JTCK_SWCLK|LPTIM1_OUT|          |          |I2C1_SMBA |          |          |          |
  * PA15|JTDI      |TIM2_CH1  |TIM2_ETR  |          |          |SPI1_NSS  |          |          |
  *______________________________________________________________________________________________
  * PB0 |          |          |          |          |          |          |          |          |
  * PB1 |          |          |          |          |          |          |          |          |
  * PB2 |RTC_OUT   |LPTIM1_OUT|          |          |I2C3_SMBA |SPI1_NSS  |RF_DTB10  |          |
  * PB3 |JTDO      |TIM2_CH2  |          |          |          |SPI1_SCK  |          |USART1_RTS|
  * PB4 |NJTRST    |          |          |          |I2C3_SDA  |SPI1_MISO |          |USART1_CTS|
  * PB5 |          |LPTIM1_IN1|          |          |I2C1_SMBA |SPI1_MOSI |RF_MOSI   |USART1_CK |
  * PB6 |          |LPTIM1_ETR|          |          |I2C1_SCL  |          |RF_SCK    |USART1_TX |
  * PB7 |          |LPTIM1_IN2|          |TIM1_BKIN |I2C1_SDA  |          |RF_DTB11  |USART1_RX |
  * PB8 |          |TIM1_CH2N |          |SAI1_CK1  |I2C1_SCL  |          |RF_DTB16  |          |
  * PB9 |          |TIM1_CH3N |          |SAI1_DI2  |I2C1_SDA  |SPI2_NSS  |          |          |
  * PB10|          |TIM2_CH3  |          |          |I2C3_SCL  |SPI2_SCK  |RF_DTB18  |          |
  * PB11|          |TIM2_CH4  |          |          |I2C3_SDA  |          |RF_DTB17  |          |
  * PB12|          |TIM1_BKIN |          |TIM1_BKIN |I2C3_SMBA |SPI2_NSS  |          |          |
  * PB13|          |TIM1_CH1N |          |          |I2C3_SCL  |SPI2_SCK  |          |          |
  * PB14|          |TIM1_CH2N |          |          |I2C3_SDA  |SPI2_MISO |          |          |
  * PB15|RTC_REFIN |TIM1_CH3N |          |          |          |SPI2_MOSI |          |          |
  *______________________________________________________________________________________________
  * PC0 |          |LPTIM1_IN1|          |          |I2C3_SCL  |          |          |          |
  * PC1 |          |LPTIM1_OUT|          |SPI2_MOSI |I2C3_SDA  |          |          |          |
  * PC2 |          |LPTIM1_IN2|          |          |          |SPI2_MISO |          |          |
  * PC3 |          |LPTIM1_ETR|          |SAI1_DI1  |          |SPI2_MOSI |          |          |
  * PC4 |          |          |          |          |          |          |          |          |
  * PC5 |          |          |          |SAI1_DI3  |          |          |          |          |
  * PC6 |          |          |          |          |          |          |          |          |
  * PC7 |          |          |          |          |          |          |          |          |
  * PC8 |          |          |          |          |          |          |          |          |
  * PC9 |          |          |          |TIM1_BKIN |          |          |          |          |
  * PC10|TRACED1   |          |          |          |          |          |          |          |
  * PC11|          |          |          |          |          |          |          |          |
  * PC12|TRACED3   |          |          |          |          |          |          |          |
  * PC13|          |          |          |          |          |          |          |          |
  * PC14|          |          |          |          |          |          |RF_DTB0   |          |
  * PC15|          |          |          |          |          |          |RF_DTB1   |          |
  *______________________________________________________________________________________________
  * PD0 |          |          |          |          |          |SPI2_NSS  |          |          |
  * PD1 |          |          |          |          |          |SPI2_SCK  |          |          |
  * PD2 |TRACED2   |          |          |          |          |          |          |          |
  * PD3 |          |          |          |SPI2_SCK  |          |SPI2_MISO |          |          |
  * PD4 |          |          |          |          |          |SPI2_MOSI |          |          |
  * PD5 |          |          |          |          |          |          |          |          |
  * PD6 |          |          |          |SAI1_DI1  |          |          |          |          |
  * PD7 |          |          |          |          |          |          |          |          |
  * PD8 |          |          |TIM1_BKIN2|          |          |          |          |          |
  * PD9 |TRACED0   |          |          |          |          |          |          |          |
  * PD10|TRIG_IO   |          |          |          |          |          |          |          |
  * PD11|          |          |          |          |          |          |          |          |
  * PD12|          |          |          |          |          |          |          |          |
  * PD13|          |          |          |          |          |          |          |          |
  * PD14|          |TIM1_CH1  |          |          |          |          |          |          |
  * PD15|          |TIM1_CH2  |          |          |          |          |          |          |
  *______________________________________________________________________________________________
  * PE0 |          |TIM1_ETR  |          |          |          |          |          |          |
  * PE1 |          |          |          |          |          |          |          |          |
  * PE2 |TRACED2   |          |          |SAI1_CK1  |          |          |          |          |
  * PE3 |          |          |          |          |          |          |          |          |
  * PE4 |          |          |          |          |          |          |          |          |
  *______________________________________________________________________________________________
  * PH0 |          |          |          |          |          |          |          |          |
  * PH1 |          |          |          |          |          |          |          |          |
  * PE2 |          |          |          |          |          |          |          |          |
  * PH3 |          |          |          |          |          |          |RF_NSS    |          |
  *______________________________________________________________________________________________*/


 /*     |   AF8    |   AF9    |   AF10   |   AF11   |  AF12    |  AF13    | AF14     | AF15     |
  *_____________________________________________________________________________________________
  *     |LPUART1   |TSC       |USB/QUADSP|LCD       |COMP/TIM  |SAI       |TIM       |EVENTOUT  |
  *_____________________________________________________________________________________________
  * PA0 |          |          |          |          |COMP1_OUT |SAI1_E_CLK|TIM2_ETR  |EVENTOUT  |
  * PA1 |          |          |          |LCD_SEG0  |          |          |          |EVENTOUT  |
  * PA2 |LPUART1_TX|          |QSPI_NCS  |LCD_SEG1  |COMP2_OUT |          |          |EVENTOUT  |
  * PA3 |LPUART1_RX|          |QSPI_CLK  |LCD_SEG2  |          |SAI1_CLK_A|          |EVENTOUT  |
  * PA4 |          |          |          |          |          |SAI1_FS_B |LPTIM2_OUT|EVENTOUT  |
  * PA5 |          |          |          |          |          |          |LPTIM2_ETR|EVENTOUT  |
  * PA6 |LPUART1_CT|          |QSPI_IO3  |LCD_SEG3  |TIM1_BKIN |          |TIM16_CH1 |EVENTOUT  |
  * PA7 |          |          |QSPI_IO2  |LCD_SEG4  |COMP2_OUT |          |TIM17_CH1 |EVENTOUT  |
  * PA8 |          |          |          |LCD_COM0  |          |SAI1_SCK_A|LPTIM2_OUT|EVENTOUT  |
  * PA9 |          |          |          |LCD_COM1  |          |SAI1_FS_A |          |EVENTOUT  |
  * PA10|          |          |USB_CRS_SY|LCD_COM2  |          |SAI1_SD_A |TIM17_BKIN|EVENTOUT  |
  * PA11|          |          |USB_DM    |          |TIM1_BKIN2|          |          |EVENTOUT  |
  * PA12|          |          |USB_DP    |          |          |          |          |EVENTOUT  |
  * PA13|IR_OUT    |          |USB_NOE   |          |          |SAI1_SD_B |          |EVENTOUT  |
  * PA14|          |          |          |          |          |SAI1_FS_B |          |EVENTOUT  |
  * PA15|          |TSC_G3_IO1|          |LCD_SEG17 |          |          |          |EVENTOUT  |
  *______________________________________________________________________________________________
  * PB0 |          |          |          |LCD_SEG5  |COMP1_OUT |          |          |EVENTOUT  |
  * PB1 |LPUART1_RT|          |          |LCD_SEG6  |          |          |LPTIM2_IN1|EVENTOUT  |
  * PB2 |          |          |          |LCD_VLCD  |          |SAI1_E_CLK|          |EVENTOUT  |
  * PB3 |          |          |          |LCD_SEG7  |          |SAI1_SCK_B|          |EVENTOUT  |
  * PB4 |          |TSC_G2_IO1|          |LCD_SEG8  |          |SAI1_CLK_B|TIM17_BKIN|EVENTOUT  |
  * PB5 |          |TSC_G2_IO2|          |LCD_SEG9  |COMP2_OUT |SAI1_SD_B |TIM16_BKIN|EVENTOUT  |
  * PB6 |          |TSC_G2_IO3|          |          |          |SAI1_FS_B |TIM16_CH1N|EVENTOUT  |
  * PB7 |          |TSC_G2_IO4|          |LCD_SEG21 |          |          |TIM17_CH1N|EVENTOUT  |
  * PB8 |          |          |QSPI_IO1  |LCD_SEG16 |          |SAI1_CLK_A|TIM16_CH1 |EVENTOUT  |
  * PB9 |IR_OUT    |TSC_G7_IO4|QSPI_IO0  |LCD_COM3  |          |SAI1_FS_A |TIM17_CH1 |EVENTOUT  |
  * PB10|LPUART1_RX|TSC_SYNC  |QSPI_CLK  |LCD_SEG10 |COMP1_OUT |SAI1_SCK_A|          |EVENTOUT  |
  * PB11|LPUART1_TX|          |QSPI_NCS  |LCD_SEG11 |COMP2_OUT |          |          |EVENTOUT  |
  * PB12|LPUART1_RT|TSC_G1_IO1|          |LCD_SEG12 |          |SAI1_FS_A |          |EVENTOUT  |
  * PB13|LPUART1_CT|TSC_G1_IO2|          |LCD_SEG13 |          |SAI1_SCK_A|          |EVENTOUT  |
  * PB14|          |TSC_G1_IO3|          |LCD_SEG14 |          |SAI1_CLK_A|          |EVENTOUT  |
  * PB15|          |TSC_G1_IO4|          |LCD_SEG15 |          |SAI1_SD_A |          |EVENTOUT  |
  *______________________________________________________________________________________________
  * PC0 |LPUART1_RX|          |          |LCD_SEG18 |          |          |LPTIM2_IN1|EVENTOUT  |
  * PC1 |LPUART1_TX|          |          |LCD_SEG19 |          |          |          |EVENTOUT  |
  * PC2 |          |          |          |LCD_SEG20 |          |          |          |EVENTOUT  |
  * PC3 |          |          |          |LCD_VLCD  |          |SAI1_SD_A |LPTIM2_ETR|EVENTOUT  |
  * PC4 |          |          |          |LCD_SEG22 |          |          |          |EVENTOUT  |
  * PC5 |          |          |          |LCD_SEG23 |          |          |          |EVENTOUT  |
  * PC6 |          |TSC_G4_IO1|          |LCD_SEG24 |          |          |          |EVENTOUT  |
  * PC7 |          |TSC_G4_IO2|          |LCD_SEG25 |          |          |          |EVENTOUT  |
  * PC8 |          |TSC_G4_IO3|          |LCD_SEG26 |          |          |          |EVENTOUT  |
  * PC9 |          |TSC_G4_IO4|USB_NOE   |LCD_SEG27 |          |SAI1_SCK_B|          |EVENTOUT  |
  * PC10|          |TSC_G3_IO2|          |LCD_Cx_SEx|          |          |          |EVENTOUT  |
  * PC11|          |TSC_G3_IO3|          |LCD_Cx_SEx|          |          |          |EVENTOUT  |
  * PC12|          |TSC_G3_IO4|          |LCD_Cx_SEx|          |          |          |EVENTOUT  |
  * PC13|          |          |          |          |          |          |          |EVENTOUT  |
  * PC14|          |          |          |          |          |          |          |EVENTOUT  |
  * PC15|          |          |          |          |          |          |          |EVENTOUT  |
  *______________________________________________________________________________________________
  * PD0 |          |          |          |          |          |          |          |EVENTOUT  |
  * PD1 |          |          |          |          |          |          |          |EVENTOUT  |
  * PD2 |          |TSC_SYNC  |          |LCD_Cx_SEx|          |          |          |EVENTOUT  |
  * PD3 |          |          |QSPI_NCS  |          |          |          |          |EVENTOUT  |
  * PD4 |          |TSC_G5_IO1|QSPI_IO0  |          |          |          |          |EVENTOUT  |
  * PD5 |          |TSC_G5_IO2|QSPI_IO1  |          |          |SAI1_CLK_B|          |EVENTOUT  |
  * PD6 |          |TSC_G5_IO3|QSPI_IO2  |          |          |SAI1_SD_A |          |EVENTOUT  |
  * PD7 |          |TSC_G5_IO4|QSPI_IO3  |LCD_SEG39 |          |          |          |EVENTOUT  |
  * PD8 |          |          |          |LCD_SEG28 |          |          |          |EVENTOUT  |
  * PD9 |          |          |          |LCD_SEG29 |          |          |          |EVENTOUT  |
  * PD10|          |TSC_G6_IO1|          |LCD_SEG30 |          |          |          |EVENTOUT  |
  * PD11|          |TSC_G6_IO2|          |LCD_SEG31 |          |          |LPTIM2_ETR|EVENTOUT  |
  * PD12|          |TSC_G6_IO3|          |LCD_SEG32 |          |          |LPTIM2_IN1|EVENTOUT  |
  * PD13|          |TSC_G6_IO4|          |LCD_SEG33 |          |          |LPTIM2_OUT|EVENTOUT  |
  * PD14|          |          |          |LCD_SEG34 |          |          |          |EVENTOUT  |
  * PD15|          |          |          |LCD_SEG35 |          |          |          |EVENTOUT  |
  *______________________________________________________________________________________________
  * PE0 |          |TSC_G7_IO3|          |LCD_SEG36 |          |          |TIM16_CH1 |EVENTOUT  |
  * PE1 |          |TSC_G7_IO2|          |LCD_SEG37 |          |          |TIM17_CH1 |EVENTOUT  |
  * PE2 |          |TSC_G7_IO1|          |LCD_SEG38 |          |SAI1_CLK_A|          |EVENTOUT  |
  * PE3 |          |          |          |          |          |          |          |EVENTOUT  |
  * PE4 |          |          |          |          |          |          |          |EVENTOUT  |
  *______________________________________________________________________________________________
  * PH0 |          |          |          |          |          |          |          |EVENTOUT  |
  * PH1 |          |          |          |          |          |          |          |EVENTOUT  |
  * PE2 |          |          |          |          |          |          |          |EVENTOUT  |
  * PH3 |          |          |          |          |          |          |          |EVENTOUT  |
  *______________________________________________________________________________________________*/

/**
  * @brief   AF 0 selection
  */

#define GPIO_AF0_MCO           ((uint8_t)0x00)  /*!< MCO Alternate Function mapping                 */
#define GPIO_AF0_LSCO          ((uint8_t)0x00)  /*!< LSCO Alternate Function mapping                */
#define GPIO_AF0_JTMS_SWDIO    ((uint8_t)0x00)  /*!< JTMS-SWDIO Alternate Function mapping          */
#define GPIO_AF0_JTCK_SWCLK    ((uint8_t)0x00)  /*!< JTCK-SWCLK Alternate Function mapping          */
#define GPIO_AF0_JTDI          ((uint8_t)0x00)  /*!< JTDI Alternate Function mapping                */
#define GPIO_AF0_RTC_OUT       ((uint8_t)0x00)  /*!< RCT_OUT Alternate Function mapping             */
#define GPIO_AF0_JTD_TRACE     ((uint8_t)0x00)  /*!< JTDO-TRACESWO Alternate Function mapping       */
#define GPIO_AF0_NJTRST        ((uint8_t)0x00)  /*!< NJTRST Alternate Function mapping              */
#define GPIO_AF0_RTC_REFIN     ((uint8_t)0x00)  /*!< RTC_REFIN Alternate Function mapping           */
#define GPIO_AF0_TRACED0       ((uint8_t)0x00)  /*!< TRACED0 Alternate Function mapping             */
#define GPIO_AF0_TRACED1       ((uint8_t)0x00)  /*!< TRACED1 Alternate Function mapping             */
#define GPIO_AF0_TRACED2       ((uint8_t)0x00)  /*!< TRACED2 Alternate Function mapping             */
#define GPIO_AF0_TRACED3       ((uint8_t)0x00)  /*!< TRACED3 Alternate Function mapping             */
#define GPIO_AF0_TRIG_INOUT    ((uint8_t)0x00)  /*!< TRIG_INOUT Alternate Function mapping          */
#define GPIO_AF0_TRACECK       ((uint8_t)0x00)  /*!< TRACECK Alternate Function mapping             */
#define GPIO_AF0_SYS           ((uint8_t)0x00)  /*!< System Function mapping                        */

 /**
  * @brief   AF 1 selection
  */
#define GPIO_AF1_TIM1           ((uint8_t)0x01)  /*!< TIM1 Alternate Function mapping           */
#define GPIO_AF1_TIM2           ((uint8_t)0x01)  /*!< TIM2 Alternate Function mapping            */
#define GPIO_AF1_LPTIM1         ((uint8_t)0x01)  /*!< LPTIM1 Alternate Function mapping          */

/**
  * @brief   AF 2 selection
  */

#define GPIO_AF2_TIM2           ((uint8_t)0x02)  /*!< TIM2 Alternate Function mapping           */
#define GPIO_AF2_TIM1           ((uint8_t)0x02)  /*!< TIM1 Alternate Function mapping         */
/**
  * @brief   AF 3 selection
  */
#define GPIO_AF3_SAI1           ((uint8_t)0x03)  /*!< SAI1_CK1 Alternate Function mapping           */
#define GPIO_AF3_SPI2           ((uint8_t)0x03)  /*!< SPI2 Alternate Function mapping          */
#define GPIO_AF3_TIM1           ((uint8_t)0x03)  /*!< TIM1 Alternate Function mapping          */

/**
  * @brief   AF 4 selection
  */
#define GPIO_AF4_I2C1           ((uint8_t)0x04)  /*!< I2C1 Alternate Function mapping         */
#define GPIO_AF4_I2C3           ((uint8_t)0x04)  /*!< I2C3 Alternate Function mapping          */

/**
  * @brief   AF 5 selection
  */
#define GPIO_AF5_SPI1           ((uint8_t)0x05)  /*!< SPI1 Alternate Function mapping        */
#define GPIO_AF5_SPI2           ((uint8_t)0x05)  /*!< SPI2 Alternate Function mapping        */
/**
  * @brief   AF 6 selection
  */
#define GPIO_AF6_MCO            ((uint8_t)0x06)  /*!< MCO Alternate Function mapping       */
#define GPIO_AF6_LSCO           ((uint8_t)0x06)  /*!< LSCO Alternate Function mapping      */
#define GPIO_AF6_RF_DTB0        ((uint8_t)0x06)  /*!< RF_DTB0 Alternate Function mapping   */
#define GPIO_AF6_RF_DTB1        ((uint8_t)0x06)  /*!< RF_DTB1 Alternate Function mapping   */
#define GPIO_AF6_RF_DTB2        ((uint8_t)0x06)  /*!< RF_DTB2 Alternate Function mapping   */
#define GPIO_AF6_RF_DTB3        ((uint8_t)0x06)  /*!< RF_DTB3 Alternate Function mapping   */
#define GPIO_AF6_RF_DTB4        ((uint8_t)0x06)  /*!< RF_DTB4 Alternate Function mapping   */
#define GPIO_AF6_RF_DTB5        ((uint8_t)0x06)  /*!< RF_DTB5 Alternate Function mapping   */
#define GPIO_AF6_RF_DTB6        ((uint8_t)0x06)  /*!< RF_DTB6 Alternate Function mapping   */
#define GPIO_AF6_RF_DTB7        ((uint8_t)0x06)  /*!< RF_DTB7 Alternate Function mapping   */
#define GPIO_AF6_RF_DTB8        ((uint8_t)0x06)  /*!< RF_DTB8 Alternate Function mapping   */
#define GPIO_AF6_RF_DTB9        ((uint8_t)0x06)  /*!< RF_DTB9 Alternate Function mapping   */
#define GPIO_AF6_RF_DTB10       ((uint8_t)0x06)  /*!< RF_DTB10 Alternate Function mapping  */
#define GPIO_AF6_RF_DTB11       ((uint8_t)0x06)  /*!< RF_DTB11 Alternate Function mapping  */
#define GPIO_AF6_RF_DTB12       ((uint8_t)0x06)  /*!< RF_DTB12 Alternate Function mapping  */
#define GPIO_AF6_RF_DTB13       ((uint8_t)0x06)  /*!< RF_DTB13 Alternate Function mapping  */
#define GPIO_AF6_RF_DTB14       ((uint8_t)0x06)  /*!< RF_DTB14 Alternate Function mapping  */
#define GPIO_AF6_RF_DTB15       ((uint8_t)0x06)  /*!< RF_DTB15 Alternate Function mapping  */
#define GPIO_AF6_RF_DTB16       ((uint8_t)0x06)  /*!< RF_DTB16 Alternate Function mapping  */
#define GPIO_AF6_RF_DTB17       ((uint8_t)0x06)  /*!< RF_DTB17 Alternate Function mapping  */
#define GPIO_AF6_RF_DTB18       ((uint8_t)0x06)  /*!< RF_DTB18 Alternate Function mapping  */
#define GPIO_AF6_RF_MISO        ((uint8_t)0x06)  /*!< RF_MISO Alternate Function mapping   */
#define GPIO_AF6_RF_MOSI        ((uint8_t)0x06)  /*!< RF_MOSI Alternate Function mapping   */
#define GPIO_AF6_RF_SCK         ((uint8_t)0x06)  /*!< RF_SCK Alternate Function mapping    */
#define GPIO_AF6_RF_NSS         ((uint8_t)0x06)  /*!< RF_NSS Alternate Function mapping    */
/**
  * @brief   AF 7 selection
  */
#define GPIO_AF7_USART1         ((uint8_t)0x07)  /*!< USART1 Alternate Function mapping     */

/**
  * @brief   AF 8 selection
  */
#define GPIO_AF8_LPUART1        ((uint8_t)0x08)  /*!< LPUART1 Alternate Function mapping      */
#define GPIO_AF8_IR             ((uint8_t)0x08)  /*!< IR Alternate Function mapping          */

/**
  * @brief   AF 9 selection
  */
 #define GPIO_AF9_TSC           ((uint8_t)0x09)  /*!< TSC Alternate Function mapping      */

/**
  * @brief   AF 10 selection
  */
#define GPIO_AF10_QUADSPI       ((uint8_t)0x0a)  /*!< QUADSPI Alternate Function mapping  */
#define GPIO_AF10_USB           ((uint8_t)0x0a)  /*!< USB Alternate Function mapping      */

/**
   * @brief   AF 11 selection
   */
#define GPIO_AF11_LCD           ((uint8_t)0x0b)  /*!< LCD Alternate Function mapping      */

/**
 * @brief   AF 12 selection
 */
#define GPIO_AF12_COMP1         ((uint8_t)0x0c)  /*!< COMP1 Alternate Function mapping      */
#define GPIO_AF12_COMP2         ((uint8_t)0x0c)  /*!< COMP2 Alternate Function mapping      */
#define GPIO_AF12_TIM1          ((uint8_t)0x0c)  /*!< TIM1 Alternate Function mapping      */

/**
 * @brief   AF 13 selection
 */
#define GPIO_AF13_SAI1          ((uint8_t)0x0d)  /*!< SAI1 Alternate Function mapping      */

/**
 * @brief   AF 14 selection
 */
#define GPIO_AF14_TIM2          ((uint8_t)0x0e)  /*!< TIM2 Alternate Function mapping       */
#define GPIO_AF14_TIM16         ((uint8_t)0x0e)  /*!< TIM16 Alternate Function mapping      */
#define GPIO_AF14_TIM17         ((uint8_t)0x0e)  /*!< TIM17 Alternate Function mapping      */
#define GPIO_AF14_LPTIM2        ((uint8_t)0x0e)  /*!< LPTIM2 Alternate Function mapping     */


/**
* @brief   AF 15 selection
*/

#define GPIO_AF15_EVENTOUT          ((uint8_t)0x0f)  /*!< EVENTOUT Alternate Function mapping      */

#define IS_GPIO_AF(AF)              ((AF) <= (uint8_t)0x0f)


/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup GPIOEx_Exported_Macros GPIOEx Exported Macros
  * @{
  */

/** @defgroup GPIOEx_Get_Port_Index GPIOEx Get Port Index
* @{
  */

#define GPIO_GET_INDEX(__GPIOx__)    (((__GPIOx__) == (GPIOA))? 0uL :\
                                      ((__GPIOx__) == (GPIOB))? 1uL :\
                                      ((__GPIOx__) == (GPIOC))? 2uL :\
                                      ((__GPIOx__) == (GPIOD))? 3uL :\
                                      ((__GPIOx__) == (GPIOE))? 4uL : 7uL)

 /**
  * @}
  */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32WBxx_HAL_GPIO_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
