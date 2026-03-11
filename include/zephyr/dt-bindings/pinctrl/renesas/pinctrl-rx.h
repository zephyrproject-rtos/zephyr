/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RX pin control (pinctrl) definitions for Zephyr.
 *
 * This header provides macro constants for encoding pin function selections
 * and pin indices for Renesas RX SoCs. These values are used by the DeviceTree
 * pinctrl subsystem to describe peripheral pin mappings.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_PINCTRL_RX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_PINCTRL_RX_H_

/**
 * @name Multi-Function Pin Controller (MPC) Definitions for Renesas RX SoCs.
 * @{
 */

/** @brief Bit position of the port number field. */
#define RX_PORT_NUM_POS  0 /**< Port number position. */
/** @brief Mask for the port number field. */
#define RX_PORT_NUM_MASK 0x1f /**< Port number mask. */

/** @brief Bit position of the pin number field. */
#define RX_PIN_NUM_POS  5 /**< Pin number position. */
/** @brief Mask for the pin number field. */
#define RX_PIN_NUM_MASK 0xf /**< Pin number mask. */

/** @brief Bit position of the Peripheral Select (PSEL) field. */
#define RX_PSEL_MASK 0x1f /**< PSEL mask. */
/** @brief Mask for the Peripheral Select (PSEL) field. */
#define RX_PSEL_POS  9 /**< PSEL position. */

/**
 * @name Renesas RX Peripheral Selection (PSEL) Pins
 * @{
 */

#define RX_PSEL_RSCI      0xA /**< RSCI function. */
#define RX_PSEL_RSCI_TXDB 0xC /**< RSCI TXD/B function. */
#define RX_PSEL_SCI_1     0xA /**< SCI1 function. */
#define RX_PSEL_SCI_5     0xA /**< SCI5 function. */
#define RX_PSEL_SCI_6     0xB /**< SCI6 function. */
#define RX_PSEL_SCI_12    0xC /**< SCI12 function. */
#define RX_PSEL_TMR       0x5 /**< TMR function. */
#define RX_PSEL_POE       0x7 /**< POE function. */
#define RX_PSEL_ADC       0x0 /**< ADC function. */
#define RX_PSEL_LVD       0x0 /**< LVD function. */

/** @} */

/**
 * @name Renesas RX MPC Port 0 Pin Function Select codes (PSEL values)
 * @{
 */
#define RX_PSEL_P0nPFS_HIZ    0x0
#define RX_PSEL_P0nPFS_ADTRG0 0x1

/** @} */

/**
 * @name  Renesas RX MPC Port 1 Pin Function Select codes (PSEL values)
 * @{
 */
#define RX_PSEL_P1nPFS_MTIOC0B 0x01 /**< Multi-function timer channel 0B. */
#define RX_PSEL_P1nPFS_MTIOC3A 0x01 /**< Multi-function timer channel 3A. */
#define RX_PSEL_P1nPFS_MTIOC3C 0x01 /**< Multi-function timer channel 3C. */

#define RX_PSEL_P1nPFS_MTCLKA  0x02 /**< Multi-Timer Clock A function. */
#define RX_PSEL_P1nPFS_MTCLKB  0x02 /**< Multi-Timer Clock B function. */
#define RX_PSEL_P1nPFS_MTIOC3B 0x02 /**< Multi-function timer channel 3B. */
#define RX_PSEL_P1nPFS_MTIOC3D 0x02 /**< Multi-function timer channel 3D. */

#define RX_PSEL_P1nPFS_TMCI1 0x5 /**< Timer input capture channel 1. */
#define RX_PSEL_P1nPFS_TMO1  0x5 /**< Timer output compare channel 1. */
#define RX_PSEL_P1nPFS_TMCI2 0x5 /**< Timer input capture channel 2. */
#define RX_PSEL_P1nPFS_TMO2  0x5 /**< Timer output compare channel 2. */
#define RX_PSEL_P1nPFS_TMRI2 0x5 /**< Timer read input channel 2. */
#define RX_PSEL_P1nPFS_TMO3  0x5 /**< Timer output compare channel 3. */

#define RX_PSEL_P1nPFS_RTCOUT 0x7 /**< Realtime clock. */
#define RX_PSEL_P1nPFS_POE8   0x7 /**< Port output enable 8. */

#define RX_PSEL_P1nPFS_ADTRG0 0x9 /**< ADC trigger input 0. */

#define RX_PSEL_P1nPFS_RXD1   0xA /**< Serial communication interface RXD1. */
#define RX_PSEL_P1nPFS_SMISO1 0xA /**< SPI MISO channel 1. */
#define RX_PSEL_P1nPFS_SSCL1  0xA /**< I2C SSCL channel 1. */
#define RX_PSEL_P1nPFS_TXD1   0xA /**< Serial communication interface TXD1. */
#define RX_PSEL_P1nPFS_SMOSI1 0xA /**< SPI MOSI channel 1. */
#define RX_PSEL_P1nPFS_SSDA1  0xA /**< I2C SSDA channel 1. */

#define RX_PSEL_P1nPFS_CTS1 0xB /**< Serial communication interface CTS1. */
#define RX_PSEL_P1nPFS_RTS1 0xB /**< Serial communication interface RTS1. */
#define RX_PSEL_P1nPFS_SS1  0xB /**< SPI slave select channel 1. */

#define RX_PSEL_P1nPFS_MOSIA 0xD /**< SPI MOSI. */
#define RX_PSEL_P1nPFS_MISOA 0xD /**< SPI MISO. */

#define RX_PSEL_P1nPFS_SCL 0xF /**< I2C SCL. */
#define RX_PSEL_P1nPFS_SDA 0xF /**< I2C SDA. */

#define RX_PSEL_P1nPFS_TS5 0x19 /**< Touch sensor input 5. */
#define RX_PSEL_P1nPFS_TS6 0x19 /**< Touch sensor input 6. */

/** @} */

/**
 * @name  Renesas RX MPC Port 1 Pin Function Select codes (PSEL values)
 * @{
 */

#define RX_PSEL_P2nPFS_MTIOC1A 0x01 /**< Multi-function timer channel 1A. */
#define RX_PSEL_P2nPFS_MTIOC1B 0x01 /**< Multi-function timer channel 1B. */
#define RX_PSEL_P2nPFS_MTIOC2A 0x01 /**< Multi-function timer channel 2A. */
#define RX_PSEL_P2nPFS_MTIOC2B 0x01 /**< Multi-function timer channel 2B. */
#define RX_PSEL_P2nPFS_MTIOC3B 0x01 /**< Multi-function timer channel 3B. */
#define RX_PSEL_P2nPFS_MTIOC3D 0x01 /**< Multi-function timer channel 3D. */
#define RX_PSEL_P2nPFS_MTIOC4A 0x01 /**< Multi-function timer channel 4A. */
#define RX_PSEL_P2nPFS_MTIOC4C 0x01 /**< Multi-function timer channel 4C. */

#define RX_PSEL_P2nPFS_MTCLKA 0x02 /**< Multi-Timer external clock A. */
#define RX_PSEL_P2nPFS_MTCLKB 0x02 /**< Multi-Timer external clock B. */
#define RX_PSEL_P2nPFS_MTCLKC 0x02 /**< Multi-Timer external clock C. */
#define RX_PSEL_P2nPFS_MTCLKD 0x02 /**< Multi-Timer external clock D. */

#define RX_PSEL_P2nPFS_TMCI0 0x5 /**< Timer input capture channel 0. */
#define RX_PSEL_P2nPFS_TMO0  0x5 /**< Timer output compare channel 0. */
#define RX_PSEL_P2nPFS_TMRI0 0x5 /**< Timer read input channel 0. */
#define RX_PSEL_P2nPFS_TMO1  0x5 /**< Timer output compare channel 1. */
#define RX_PSEL_P2nPFS_TMRI1 0x5 /**< Timer read input channel 1. */
#define RX_PSEL_P2nPFS_TMCI3 0x5 /**< Timer input capture channel 3. */

#define RX_PSEL_P2nPFS_ADTRG0 0x9 /**< ADC trigger input 0. */

#define RX_PSEL_P2nPFS_RXD0   0xA /**< Serial communication interface RXD0. */
#define RX_PSEL_P2nPFS_SMISO0 0xA /**< SPI MISO channel 0. */
#define RX_PSEL_P2nPFS_SSCL0  0xA /**< I2C SSCL channel 0. */
#define RX_PSEL_P2nPFS_TXD0   0xA /**< Serial communication interface TXD0. */
#define RX_PSEL_P2nPFS_SMOSI0 0xA /**< SPI MOSI channel 0. */
#define RX_PSEL_P2nPFS_SSDA0  0xA /**< I2C SSDA channel 0. */
#define RX_PSEL_P2nPFS_SCK0   0xA /**< SPI SCK channel 0. */
#define RX_PSEL_P2nPFS_TXD1   0xA /**< Serial communication interface TXD1. */
#define RX_PSEL_P2nPFS_SMOSI1 0xA /**< SPI MOSI channel 1. */
#define RX_PSEL_P2nPFS_SSDA1  0xA /**< I2C SSDA channel 1. */
#define RX_PSEL_P2nPFS_SCK1   0xA /**< SPI SCK channel 1. */

#define RX_PSEL_P2nPFS_CTS0 0xB /**< Serial communication interface CTS0. */
#define RX_PSEL_P2nPFS_RTS0 0xB /**< Serial communication interface RTS0. */
#define RX_PSEL_P2nPFS_SS0  0xB /**< SPI slave select channel 0. */

#define RX_PSEL_P2nPFS_TS3 0x19 /**< Touch sensor input 3. */
#define RX_PSEL_P2nPFS_TS4 0x19 /**< Touch sensor input 4. */

/** @} */

/**
 * @name  Renesas RX MPC Port 3 Pin Function Select codes (PSEL values)
 * @{
 */
#define RX_PSEL_P3nPFS_MTIOC0A 0x01 /**< Multi-function timer channel 0A. */
#define RX_PSEL_P3nPFS_MTIOC0C 0x01 /**< Multi-function timer channel 0C. */
#define RX_PSEL_P3nPFS_MTIOC0D 0x01 /**< Multi-function timer channel 0D. */
#define RX_PSEL_P3nPFS_MTIOC4B 0x01 /**< Multi-function timer channel 4B. */
#define RX_PSEL_P3nPFS_MTIOC4D 0x01 /**< Multi-function timer channel 4D. */

#define RX_PSEL_P3nPFS_TMCI2 0x5 /**< Timer input capture channel 2. */
#define RX_PSEL_P3nPFS_TMO3  0x5 /**< Timer output compare channel 3. */
#define RX_PSEL_P3nPFS_TMRI3 0x5 /**< Timer read input channel 3. */
#define RX_PSEL_P3nPFS_TMCI3 0x5 /**< Timer input capture channel 3. */

#define RX_PSEL_P3nPFS_RTCOUT 0x7 /**< Realtime clock output. */
#define RX_PSEL_P3nPFS_POE2   0x7 /**< Port output enable 2. */
#define RX_PSEL_P3nPFS_POE3   0x7 /**< Port output enable 3. */
#define RX_PSEL_P3nPFS_POE8   0x7 /**< Port output enable 8. */

#define RX_PSEL_P3nPFS_RXD1   0xA /**< Serial communication interface RXD1. */
#define RX_PSEL_P3nPFS_SMISO1 0xA /**< SPI MISO channel 1. */
#define RX_PSEL_P3nPFS_SSCL1  0xA /**< I2C SSCL channel 1. */

#define RX_PSEL_P3nPFS_CTS1   0xB /**< Serial communication interface CTS1. */
#define RX_PSEL_P3nPFS_RTS1   0xB /**< Serial communication interface RTS1. */
#define RX_PSEL_P3nPFS_SS1    0xB /**< SPI slave select channel 1. */
#define RX_PSEL_P3nPFS_RXD6   0xB /**< Serial communication interface RXD6. */
#define RX_PSEL_P3nPFS_SMISO6 0xB /**< SPI MISO channel 6. */
#define RX_PSEL_P3nPFS_SSCL6  0xB /**< I2C SSCL channel 6. */
#define RX_PSEL_P3nPFS_TXD6   0xB /**< Serial communication interface TXD6. */
#define RX_PSEL_P3nPFS_SMOSI6 0xB /**< SPI MOSI channel 6. */
#define RX_PSEL_P3nPFS_SSDA6  0xB /**< I2C SSDA channel 6. */
#define RX_PSEL_P3nPFS_SCK6   0xB /**< SPI SCK channel 6. */

#define RX_PSEL_P3nPFS_TS0 0x19 /**< Touch sensor input 0. */
#define RX_PSEL_P3nPFS_TS1 0x19 /**< Touch sensor input 1. */
#define RX_PSEL_P3nPFS_TS2 0x19 /**< Touch sensor input 2. */

/** @} */

/**
 * @name  Renesas RX MPC Port 5 Pin Function Select codes (PSEL values)
 * @{
 */
#define RX_PSEL_P5nPFS_MTIOC4B 0x01 /**< Multi-function timer channel 4B. */
#define RX_PSEL_P5nPFS_MTIOC4D 0x01 /**< Multi-function timer channel 4D. */

#define RX_PSEL_P5nPFS_TMCI1 0x5 /**< Timer input capture channel 1. */
#define RX_PSEL_P5nPFS_TMO3  0x5 /**< Timer output compare channel 3. */

#define RX_PSEL_P5nPFS_TS11 0x19 /** Touch sensor input 11. */
#define RX_PSEL_P5nPFS_TS12 0x19 /** Touch sensor input 12. */

#define RX_PSEL_P5nPFS_PMC0 0x19 /** PMC0 function. */
#define RX_PSEL_P5nPFS_PMC1 0x19 /** PMC1 function. */

/** @} */

/**
 * @name  Renesas RX MPC Port A Pin Function Select codes (PSEL values)
 * @{
 */
#define RX_PSEL_PAnPFS_MTIOC4A 0x01 /**< Multi-function timer channel 4A. */
#define RX_PSEL_PAnPFS_MTIOC0B 0x01 /**< Multi-function timer channel 0B. */
#define RX_PSEL_PAnPFS_MTIOC0D 0x01 /**< Multi-function timer channel 0D. */
#define RX_PSEL_PAnPFS_MTIOC5U 0x01 /**< Multi-function timer channel 5U. */
#define RX_PSEL_PAnPFS_MTIOC5V 0x01 /**< Multi-function timer channel 5V. */

#define RX_PSEL_PAnPFS_MTCLKA 0x02 /**< Multi-function timer clock channel A. */
#define RX_PSEL_PAnPFS_MTCLKB 0x02 /**< Multi-function timer clock channel B. */
#define RX_PSEL_PAnPFS_MTCLKC 0x02 /**< Multi-function timer clock channel C. */
#define RX_PSEL_PAnPFS_MTCLKD 0x02 /**< Multi-function timer clock channel D. */

#define RX_PSEL_PAnPFS_TMRI0 0x5 /**< Timer read input channel 0. */
#define RX_PSEL_PAnPFS_TMCI3 0x5 /**< Timer input capture channel 3.*/

#define RX_PSEL_PAnPFS_POE2   0x7 /**< Port output enable 2. */
#define RX_PSEL_PAnPFS_CACREF 0x7 /**< CAC reference voltage output. */

#define RX_PSEL_PAnPFS_RXD5   0xA /**< Serial communication interface RXD5. */
#define RX_PSEL_PAnPFS_SMISO5 0xA /**< SPI MISO channel 5. */
#define RX_PSEL_PAnPFS_SSCL5  0xA /**< I2C SSCL channel 5. */
#define RX_PSEL_PAnPFS_TXD5   0xA /**< Serial communication interface TXD5. */
#define RX_PSEL_PAnPFS_SMOSI5 0xA /**< SPI MOSI channel 5. */
#define RX_PSEL_PAnPFS_SSDA5  0xA /**< I2C SSDA channel 5. */
#define RX_PSEL_PAnPFS_SCK5   0xA /**< SPI SCK channel 5. */

#define RX_PSEL_PAnPFS_CTS5 0xB /**< Serial communication interface CTS5. */
#define RX_PSEL_PAnPFS_RTS5 0xB /**< Serial communication interface RTS5. */
#define RX_PSEL_PAnPFS_SS5  0xB /**< SPI slave select channel 5. */

#define RX_PSEL_PAnPFS_SSLA0  0xD /**< SPI slave select A0. */
#define RX_PSEL_PAnPFS_SSLA1  0xD /**< SPI slave select A1. */
#define RX_PSEL_PAnPFS_SSLA2  0xD /**< SPI slave select A2. */
#define RX_PSEL_PAnPFS_SSLA3  0xD /**< SPI slave select A3. */
#define RX_PSEL_PAnPFS_RSPCKA 0xD /**< SPI RSPCK A. */
#define RX_PSEL_PAnPFS_MOSIA  0xD /**< SPI MOSI A. */
#define RX_PSEL_PAnPFS_MISOA  0xD /**< SPI MISO A. */

#define RX_PSEL_PAnPFS_TS26 0x19 /**< Touch sensor input 26. */
#define RX_PSEL_PAnPFS_TS27 0x19 /**< Touch sensor input 27. */
#define RX_PSEL_PAnPFS_TS28 0x19 /**< Touch sensor input 28. */
#define RX_PSEL_PAnPFS_TS29 0x19 /**< Touch sensor input 29. */
#define RX_PSEL_PAnPFS_TS30 0x19 /**< Touch sensor input 30. */
#define RX_PSEL_PAnPFS_TS31 0x19 /**< Touch sensor input 31. */
#define RX_PSEL_PAnPFS_TS32 0x19 /**< Touch sensor input 32. */

/** @} */

/**
 * @name  Renesas RX MPC Port B Pin Function Select codes (PSEL values)
 * @{
 */
#define RX_PSEL_PBnPFS_MTIOC0A 0x01 /**< Multi-function timer channel 0A. */
#define RX_PSEL_PBnPFS_MTIOC0C 0x01 /**< Multi-function timer channel 0C. */
#define RX_PSEL_PBnPFS_MTIOC2A 0x01 /**< Multi-function timer channel 2A. */
#define RX_PSEL_PBnPFS_MTIOC3B 0x01 /**< Multi-function timer channel 3B. */
#define RX_PSEL_PBnPFS_MTIOC3D 0x01 /**< Multi-function timer channel 3D. */
#define RX_PSEL_PBnPFS_MTIOC5W 0x01 /**< Multi-function timer channel 5W. */

#define RX_PSEL_PBnPFS_MTIOC1B 0x02 /**< Multi-function timer channel 1B. */
#define RX_PSEL_PBnPFS_MTIOC4A 0x02 /**< Multi-function timer channel 4A. */
#define RX_PSEL_PBnPFS_MTIOC4C 0x02 /**< Multi-function timer channel 4C. */

#define RX_PSEL_PBnPFS_TMO0  0x5 /**< Timer output compare channel 0. */
#define RX_PSEL_PBnPFS_TMRI1 0x5 /**< Timer read input channel 1. */
#define RX_PSEL_PBnPFS_TMCI0 0x5 /**< Timer input capture channel 0. */

#define RX_PSEL_PBnPFS_POE1 0x7 /**< Port output enable 1. */
#define RX_PSEL_PBnPFS_POE3 0x7 /**< Port output enable 3. */

#define RX_PSEL_PBnPFS_RXD9   0xA /**< Serial communication interface RXD9. */
#define RX_PSEL_PBnPFS_SMISO9 0xA /**< SPI MISO channel 9. */
#define RX_PSEL_PBnPFS_SSCL9  0xA /**< I2C SSCL channel 9. */
#define RX_PSEL_PBnPFS_TXD9   0xA /**< Serial communication interface TXD9. */
#define RX_PSEL_PBnPFS_SMOSI9 0xA /**< SPI MOSI channel 9. */
#define RX_PSEL_PBnPFS_SSDA9  0xA /**< I2C SSDA channel 9. */
#define RX_PSEL_PBnPFS_SCK9   0xA /**< SPI SCK channel 9. */

#define RX_PSEL_PBnPFS_CTS6   0xB /**< Serial communication interface CTS6. */
#define RX_PSEL_PBnPFS_RTS6   0xB /**< Serial communication interface RTS6. */
#define RX_PSEL_PBnPFS_SS6    0xB /**< SPI slave select channel 6. */
#define RX_PSEL_PBnPFS_CTS9   0xB /**< Serial communication interface CTS9. */
#define RX_PSEL_PBnPFS_RTS9   0xB /**< Serial communication interface RTS9. */
#define RX_PSEL_PBnPFS_SS9    0xB /**< SPI slave select channel 9. */
#define RX_PSEL_PBnPFS_RXD6   0xB /**< Serial communication interface RXD6. */
#define RX_PSEL_PBnPFS_SMISO6 0xB /**< SPI MISO channel 6. */
#define RX_PSEL_PBnPFS_SSCL6  0xB /**< I2C SSCL channel 6. */
#define RX_PSEL_PBnPFS_TXD6   0xB /**< Serial communication interface TXD6. */
#define RX_PSEL_PBnPFS_SMOSI6 0xB /**< SPI MOSI channel 6. */
#define RX_PSEL_PBnPFS_SSDA6  0xB /**< I2C SSDA channel 6. */
#define RX_PSEL_PBnPFS_SCK6   0xB /**< SPI SCK channel 6. */

#define RX_PSEL_PBnPFS_RSPCKA 0xD /**< SPI RSPCK A. */

#define RX_PSEL_PBnPFS_CMPOB1 0x10 /**< Comparator output B1. */

#define RX_PSEL_PBnPFS_TS18 0x19 /**< Touch sensor input 18. */
#define RX_PSEL_PBnPFS_TS19 0x19 /**< Touch sensor input 19. */
#define RX_PSEL_PBnPFS_TS20 0x19 /**< Touch sensor input 20. */
#define RX_PSEL_PBnPFS_TS21 0x19 /**< Touch sensor input 21. */
#define RX_PSEL_PBnPFS_TS22 0x19 /**< Touch sensor input 22. */
#define RX_PSEL_PBnPFS_TS23 0x19 /**< Touch sensor input 23. */
#define RX_PSEL_PBnPFS_TS24 0x19 /**< Touch sensor input 24. */
#define RX_PSEL_PBnPFS_TS25 0x19 /**< Touch sensor input 25. */

/** @} */

/**
 * @name  Renesas RX MPC Port C Pin Function Select codes (PSEL values)
 * @{
 */
#define RX_PSEL_PCnPFS_MTIOC3A 0x01 /**< Multi-function timer channel 3A. */
#define RX_PSEL_PCnPFS_MTIOC3B 0x01 /**< Multi-function timer channel 3B. */
#define RX_PSEL_PCnPFS_MTIOC3C 0x01 /**< Multi-function timer channel 3C. */
#define RX_PSEL_PCnPFS_MTIOC3D 0x01 /**< Multi-function timer channel 3D. */
#define RX_PSEL_PCnPFS_MTIOC4B 0x01 /**< Multi-function timer channel 4B. */
#define RX_PSEL_PCnPFS_MTIOC4D 0x01 /**< Multi-function timer channel 4D. */

#define RX_PSEL_PCnPFS_MTCLKA 0x02 /**< Multi-Timer Clock A function. */
#define RX_PSEL_PCnPFS_MTCLKB 0x02 /**< Multi-Timer Clock B function. */
#define RX_PSEL_PCnPFS_MTCLKC 0x02 /**< Multi-Timer Clock C function. */
#define RX_PSEL_PCnPFS_MTCLKD 0x02 /**< Multi-Timer Clock D function. */

#define RX_PSEL_PCnPFS_TMCI1 0x5 /**< Timer input capture channel 1. */
#define RX_PSEL_PCnPFS_TMO2  0x5 /**< Timer output compare channel 2. */
#define RX_PSEL_PCnPFS_TMRI2 0x5 /**< Timer read input channel 2. */
#define RX_PSEL_PCnPFS_TMCI2 0x5 /**< Timer input capture channel 2. */

#define RX_PSEL_PCnPFS_POE0   0x7 /**< Port output enable 0. */
#define RX_PSEL_PCnPFS_CACREF 0x7 /**< CAC reference voltage output. */

#define RX_PSEL_PCnPFS_RXD5   0xA /**< Serial communication interface RXD5. */
#define RX_PSEL_PCnPFS_SMISO5 0xA /**< SPI MISO channel 5. */
#define RX_PSEL_PCnPFS_SSCL5  0xA /**< I2C SSCL channel 5. */
#define RX_PSEL_PCnPFS_TXD5   0xA /**< Serial communication interface TXD5. */
#define RX_PSEL_PCnPFS_SMOSI5 0xA /**< SPI MOSI channel 5. */
#define RX_PSEL_PCnPFS_SSDA5  0xA /**< I2C SSDA channel 5. */
#define RX_PSEL_PCnPFS_SCK5   0xA /**< SPI SCK channel 5. */
#define RX_PSEL_PCnPFS_RXD8   0xA /**< Serial communication interface RXD8. */
#define RX_PSEL_PCnPFS_SMISO8 0xA /**< SPI MISO channel 8. */
#define RX_PSEL_PCnPFS_SSCL8  0xA /**< I2C SSCL channel 8. */
#define RX_PSEL_PCnPFS_TXD8   0xA /**< Serial communication interface TXD8. */
#define RX_PSEL_PCnPFS_SMOSI8 0xA /**< SPI MOSI channel 8. */
#define RX_PSEL_PCnPFS_SSDA8  0xA /**< I2C SSDA channel 8. */
#define RX_PSEL_PCnPFS_SCK8   0xA /**< SPI SCK channel 8. */

#define RX_PSEL_PCnPFS_CTS5 0xB /**< Serial communication interface CTS5. */
#define RX_PSEL_PCnPFS_RTS5 0xB /**< Serial communication interface RTS5. */
#define RX_PSEL_PCnPFS_SS5  0xB /**< SPI slave select channel 5. */
#define RX_PSEL_PCnPFS_CTS8 0xB /**< Serial communication interface CTS8. */
#define RX_PSEL_PCnPFS_RTS8 0xB /**< Serial communication interface RTS8. */
#define RX_PSEL_PCnPFS_SS8  0xB /**< SPI slave select channel 8. */

#define RX_PSEL_PCnPFS_SSLA0  0xD /**< SPI slave select A0. */
#define RX_PSEL_PCnPFS_SSLA1  0xD /**< SPI slave select A1. */
#define RX_PSEL_PCnPFS_SSLA2  0xD /**< SPI slave select A2. */
#define RX_PSEL_PCnPFS_SSLA3  0xD /**< SPI slave select A3. */
#define RX_PSEL_PCnPFS_RSPCKA 0xD /**< SPI RSPCK A. */
#define RX_PSEL_PCnPFS_MOSIA  0xD /**< SPI MOSI A. */
#define RX_PSEL_PCnPFS_MISOA  0xD /**< SPI MISO A. */

#define RX_PSEL_PCnPFS_TS13  0x19 /**< Touch sensor input 13. */
#define RX_PSEL_PCnPFS_TS14  0x19 /**< Touch sensor input 14. */
#define RX_PSEL_PCnPFS_TS15  0x19 /**< Touch sensor input 15. */
#define RX_PSEL_PCnPFS_TS16  0x19 /**< Touch sensor input 16. */
#define RX_PSEL_PCnPFS_TS17  0x19 /**< Touch sensor input 17. */
#define RX_PSEL_PCnPFS_TSCAP 0x19 /**< Touch sensor capacitor connection. */

/** @} */

/**
 * @name  Renesas RX MPC Port D Pin Function Select codes (PSEL values)
 * @{
 */
#define RX_PSEL_PDnPFS_MTIOC4B 0x01 /**< Multi-function timer channel 4B. */
#define RX_PSEL_PDnPFS_MTIOC4D 0x01 /**< Multi-function timer channel 4D. */
#define RX_PSEL_PDnPFS_MTIOC5W 0x01 /**< Multi-function timer channel 5W. */
#define RX_PSEL_PDnPFS_MTIOC5V 0x01 /**< Multi-function timer channel 5V. */
#define RX_PSEL_PDnPFS_MTIOC5U 0x01 /**< Multi-function timer channel 5U. */

#define RX_PSEL_PDnPFS_POE0 0x7 /**< Port output enable 0. */
#define RX_PSEL_PDnPFS_POE1 0x7 /**< Port output enable 1. */
#define RX_PSEL_PDnPFS_POE2 0x7 /**< Port output enable 2. */
#define RX_PSEL_PDnPFS_POE3 0x7 /**< Port output enable 3. */
#define RX_PSEL_PDnPFS_POE8 0x7 /**< Port output enable 8. */

#define RX_PSEL_PDnPFS_RXD6   0xB /**< Serial communication interface RXD6. */
#define RX_PSEL_PDnPFS_SMISO6 0xB /**< SPI MISO channel 6. */
#define RX_PSEL_PDnPFS_SSCL6  0xB /**< I2C SSCL channel 6. */
#define RX_PSEL_PDnPFS_TXD6   0xB /**< Serial communication interface TXD6. */
#define RX_PSEL_PDnPFS_SMOSI6 0xB /**< SPI MOSI channel 6. */
#define RX_PSEL_PDnPFS_SSDA6  0xB /**< I2C SSDA channel 6. */
#define RX_PSEL_PDnPFS_SCK6   0xB /**< SPI SCK channel 6. */

/** @} */

/**
 * @name  Renesas RX MPC Port E Pin Function Select codes (PSEL values)
 * @{
 */
#define RX_PSEL_PEnPFS_MTIOC4A 0x01 /**< Multi-function timer channel 4A. */
#define RX_PSEL_PEnPFS_MTIOC4B 0x01 /**< Multi-function timer channel 4B. */
#define RX_PSEL_PEnPFS_MTIOC4C 0x01 /**< Multi-function timer channel 4C. */
#define RX_PSEL_PEnPFS_MTIOC4D 0x01 /**< Multi-function timer channel 4D. */

#define RX_PSEL_PEnPFS_MTIOC1A 0x02 /**< Multi-function timer channel 1A. */
#define RX_PSEL_PEnPFS_MTIOC2B 0x02 /**< Multi-function timer channel 2B. */

#define RX_PSEL_PEnPFS_POE8 0x7 /**< Port output enable 8. */

#define RX_PSEL_PEnPFS_CLKOUT 0x9 /**< Clock output function. */

#define RX_PSEL_PEnPFS_RXD12   0xC /**< Serial communication interface RXD12. */
#define RX_PSEL_PEnPFS_SMISO12 0xC /**< SPI MISO channel 12. */
#define RX_PSEL_PEnPFS_SSCL12  0xC /**< I2C SSCL channel 12. */
#define RX_PSEL_PEnPFS_TXD12   0xC /**< Serial communication interface TXD12. */
#define RX_PSEL_PEnPFS_SMOSI12 0xC /**< SPI MOSI channel 12. */
#define RX_PSEL_PEnPFS_SSDA12  0xC /**< I2C SSDA channel 12. */
#define RX_PSEL_PEnPFS_SCK12   0xC /**< SPI SCK channel 12. */
#define RX_PSEL_PEnPFS_TXDX12  0xC /**< RSCI TXD/X function. */
#define RX_PSEL_PEnPFS_RXDX12  0xC /**< RSCI RXD/X function. */
#define RX_PSEL_PEnPFS_SIOX12  0xC /**< RSCI SIO/X function. */
#define RX_PSEL_PEnPFS_CTS12   0xC /**< Serial communication interface CTS12. */
#define RX_PSEL_PEnPFS_RTS12   0xC /**< Serial communication interface RTS12. */
#define RX_PSEL_PEnPFS_SS12    0xC /**< SPI slave select channel 12. */

#define RX_PSEL_PEnPFS_CMPOB0 0X10 /**< Comparator output B0. */

#define RX_PSEL_PEnPFS_TS33 0X19 /**< Touch sensor input 33. */
#define RX_PSEL_PEnPFS_TS34 0x19 /**< Touch sensor input 34. */
#define RX_PSEL_PEnPFS_TS35 0x19 /**< Touch sensor input 35. */

/** @} */

/**
 * @name  Renesas RX MPC Port H Pin Function Select codes (PSEL values)
 * @{
 */
#define RX_PSEL_PHnPFS_TMO0  0x05 /**< Timer output compare channel 0. */
#define RX_PSEL_PHnPFS_TMRI0 0x05 /**< Timer read input channel 0. */
#define RX_PSEL_PHnPFS_TMCI0 0x05 /**< Timer input capture channel 0. */

#define RX_PSEL_PHnPFS_CACREF 0x7 /**< CAC reference voltage output. */

#define RX_PSEL_PHnPFS_TS7  0x19 /**< Touch sensor input 7. */
#define RX_PSEL_PHnPFS_TS8  0x19 /**< Touch sensor input 8. */
#define RX_PSEL_PHnPFS_TS9  0x19 /**< Touch sensor input 9. */
#define RX_PSEL_PHnPFS_TS10 0x19 /**< Touch sensor input 10. */

/** @} */

/**
 * @name  Renesas RX MPC Port H Pin Function Select codes (PSEL values)
 * @{
 */
#define RX_PSEL_PJnPFS_MTIOC3A 0x01 /**< Multi-function timer channel 3A. */
#define RX_PSEL_PJnPFS_MTIOC3C 0x01 /**< Multi-function timer channel 3C. */

#define RX_PSEL_PJnPFS_CTS6 0xB /**< Serial communication interface CTS6. */
#define RX_PSEL_PJnPFS_TTS6 0xB /**< Serial communication interface TTS6. */
#define RX_PSEL_PJnPFS_SS6  0xB /**< SPI slave select channel 6. */

/** @} */

/**
 * @brief Macro to encode a pin configuration for Renesas RX SoCs.
 *
 * This macro encodes the mode, peripheral selection (PSEL), port number,
 * and pin number into a single 32-bit value suitable for use in DeviceTree
 * pinctrl configurations.
 *
 * @param psel       Peripheral selection value (use RX_PSEL_* macros).
 * @param port_num   Port number.
 * @param pin_num    Pin number within the port.
 *
 * @return Encoded pin configuration value.
 */
#define RX_PSEL(psel, port_num, pin_num)                                                           \
	(psel << RX_PSEL_POS | pin_num << RX_PIN_NUM_POS | port_num << RX_PORT_NUM_POS)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_PINCTRL_RX_H_ */
