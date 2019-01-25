/*
** ###################################################################
**     Version:             rev. 1.0, 2016-05-09
**     Build:               b180806
**
**     Abstract:
**         Chip specific module features.
**
**     Copyright 2016 Freescale Semiconductor, Inc.
**     Copyright 2016-2018 NXP
**
**     SPDX-License-Identifier: BSD-3-Clause
**
**     http:                 www.nxp.com
**     mail:                 support@nxp.com
**
**     Revisions:
**     - rev. 1.0 (2016-05-09)
**         Initial version.
**
** ###################################################################
*/

#ifndef _LPC54114_cm0plus_FEATURES_H_
#define _LPC54114_cm0plus_FEATURES_H_

/* SOC module features */

/* @brief ADC availability on the SoC. */
#define FSL_FEATURE_SOC_ADC_COUNT (1)
/* @brief ASYNC_SYSCON availability on the SoC. */
#define FSL_FEATURE_SOC_ASYNC_SYSCON_COUNT (1)
/* @brief CRC availability on the SoC. */
#define FSL_FEATURE_SOC_CRC_COUNT (1)
/* @brief CTIMER availability on the SoC. */
#define FSL_FEATURE_SOC_CTIMER_COUNT (5)
/* @brief DMA availability on the SoC. */
#define FSL_FEATURE_SOC_DMA_COUNT (1)
/* @brief DMIC availability on the SoC. */
#define FSL_FEATURE_SOC_DMIC_COUNT (1)
/* @brief FLEXCOMM availability on the SoC. */
#define FSL_FEATURE_SOC_FLEXCOMM_COUNT (8)
/* @brief GINT availability on the SoC. */
#define FSL_FEATURE_SOC_GINT_COUNT (2)
/* @brief GPIO availability on the SoC. */
#define FSL_FEATURE_SOC_GPIO_COUNT (1)
/* @brief I2C availability on the SoC. */
#define FSL_FEATURE_SOC_I2C_COUNT (8)
/* @brief I2S availability on the SoC. */
#define FSL_FEATURE_SOC_I2S_COUNT (2)
/* @brief INPUTMUX availability on the SoC. */
#define FSL_FEATURE_SOC_INPUTMUX_COUNT (1)
/* @brief IOCON availability on the SoC. */
#define FSL_FEATURE_SOC_IOCON_COUNT (1)
/* @brief MAILBOX availability on the SoC. */
#define FSL_FEATURE_SOC_MAILBOX_COUNT (1)
/* @brief MRT availability on the SoC. */
#define FSL_FEATURE_SOC_MRT_COUNT (1)
/* @brief PINT availability on the SoC. */
#define FSL_FEATURE_SOC_PINT_COUNT (1)
/* @brief RTC availability on the SoC. */
#define FSL_FEATURE_SOC_RTC_COUNT (1)
/* @brief SCT availability on the SoC. */
#define FSL_FEATURE_SOC_SCT_COUNT (1)
/* @brief SPI availability on the SoC. */
#define FSL_FEATURE_SOC_SPI_COUNT (8)
/* @brief SPIFI availability on the SoC. */
#define FSL_FEATURE_SOC_SPIFI_COUNT (1)
/* @brief SYSCON availability on the SoC. */
#define FSL_FEATURE_SOC_SYSCON_COUNT (1)
/* @brief USART availability on the SoC. */
#define FSL_FEATURE_SOC_USART_COUNT (8)
/* @brief USB availability on the SoC. */
#define FSL_FEATURE_SOC_USB_COUNT (1)
/* @brief UTICK availability on the SoC. */
#define FSL_FEATURE_SOC_UTICK_COUNT (1)
/* @brief WWDT availability on the SoC. */
#define FSL_FEATURE_SOC_WWDT_COUNT (1)

/* ADC module features */

/* @brief Do not has input select (register INSEL). */
#define FSL_FEATURE_ADC_HAS_NO_INSEL  (0)
/* @brief Has ASYNMODE bitfile in CTRL reigster. */
#define FSL_FEATURE_ADC_HAS_CTRL_ASYNMODE (1)
/* @brief Has ASYNMODE bitfile in CTRL reigster. */
#define FSL_FEATURE_ADC_HAS_CTRL_RESOL (1)
/* @brief Has ASYNMODE bitfile in CTRL reigster. */
#define FSL_FEATURE_ADC_HAS_CTRL_BYPASSCAL (1)
/* @brief Has ASYNMODE bitfile in CTRL reigster. */
#define FSL_FEATURE_ADC_HAS_CTRL_TSAMP (1)
/* @brief Has ASYNMODE bitfile in CTRL reigster. */
#define FSL_FEATURE_ADC_HAS_CTRL_LPWRMODE (0)
/* @brief Has ASYNMODE bitfile in CTRL reigster. */
#define FSL_FEATURE_ADC_HAS_CTRL_CALMODE (0)
/* @brief Has startup register. */
#define FSL_FEATURE_ADC_HAS_STARTUP_REG (1)
/* @brief Has ADTrim register */
#define FSL_FEATURE_ADC_HAS_TRIM_REG (0)
/* @brief Has Calibration register. */
#define FSL_FEATURE_ADC_HAS_CALIB_REG (1)

/* DMA module features */

/* @brief Number of channels */
#define FSL_FEATURE_DMA_NUMBER_OF_CHANNELS (20)

/* FLEXCOMM module features */

/* @brief FLEXCOMM0 USART INDEX 0 */
#define FSL_FEATURE_FLEXCOMM0_USART_INDEX  (0)
/* @brief FLEXCOMM0 SPI INDEX 0 */
#define FSL_FEATURE_FLEXCOMM0_SPI_INDEX  (0)
/* @brief FLEXCOMM0 I2C INDEX 0 */
#define FSL_FEATURE_FLEXCOMM0_I2C_INDEX  (0)
/* @brief FLEXCOMM1 USART INDEX 1 */
#define FSL_FEATURE_FLEXCOMM1_USART_INDEX  (1)
/* @brief FLEXCOMM1 SPI INDEX 1 */
#define FSL_FEATURE_FLEXCOMM1_SPI_INDEX  (1)
/* @brief FLEXCOMM1 I2C INDEX 1 */
#define FSL_FEATURE_FLEXCOMM1_I2C_INDEX  (1)
/* @brief FLEXCOMM2 USART INDEX 2 */
#define FSL_FEATURE_FLEXCOMM2_USART_INDEX  (2)
/* @brief FLEXCOMM2 SPI INDEX 2 */
#define FSL_FEATURE_FLEXCOMM2_SPI_INDEX  (2)
/* @brief FLEXCOMM2 I2C INDEX 2 */
#define FSL_FEATURE_FLEXCOMM2_I2C_INDEX  (2)
/* @brief FLEXCOMM3 USART INDEX 3 */
#define FSL_FEATURE_FLEXCOMM3_USART_INDEX  (3)
/* @brief FLEXCOMM3 SPI INDEX 3 */
#define FSL_FEATURE_FLEXCOMM3_SPI_INDEX  (3)
/* @brief FLEXCOMM3 I2C INDEX 3 */
#define FSL_FEATURE_FLEXCOMM3_I2C_INDEX  (3)
/* @brief FLEXCOMM4 USART INDEX 4 */
#define FSL_FEATURE_FLEXCOMM4_USART_INDEX  (4)
/* @brief FLEXCOMM4 SPI INDEX 4 */
#define FSL_FEATURE_FLEXCOMM4_SPI_INDEX  (4)
/* @brief FLEXCOMM4 I2C INDEX 4 */
#define FSL_FEATURE_FLEXCOMM4_I2C_INDEX  (4)
/* @brief FLEXCOMM5 USART INDEX 5 */
#define FSL_FEATURE_FLEXCOMM5_USART_INDEX  (5)
/* @brief FLEXCOMM5 SPI INDEX 5 */
#define FSL_FEATURE_FLEXCOMM5_SPI_INDEX  (5)
/* @brief FLEXCOMM5 I2C INDEX 5 */
#define FSL_FEATURE_FLEXCOMM5_I2C_INDEX  (5)
/* @brief FLEXCOMM6 USART INDEX 6 */
#define FSL_FEATURE_FLEXCOMM6_USART_INDEX  (6)
/* @brief FLEXCOMM6 SPI INDEX 6 */
#define FSL_FEATURE_FLEXCOMM6_SPI_INDEX  (6)
/* @brief FLEXCOMM6 I2C INDEX 6 */
#define FSL_FEATURE_FLEXCOMM6_I2C_INDEX  (6)
/* @brief FLEXCOMM7 I2S INDEX 0 */
#define FSL_FEATURE_FLEXCOMM6_I2S_INDEX  (0)
/* @brief FLEXCOMM7 USART INDEX 7 */
#define FSL_FEATURE_FLEXCOMM7_USART_INDEX  (7)
/* @brief FLEXCOMM7 SPI INDEX 7 */
#define FSL_FEATURE_FLEXCOMM7_SPI_INDEX  (7)
/* @brief FLEXCOMM7 I2C INDEX 7 */
#define FSL_FEATURE_FLEXCOMM7_I2C_INDEX  (7)
/* @brief FLEXCOMM7 I2S INDEX 1 */
#define FSL_FEATURE_FLEXCOMM7_I2S_INDEX  (1)

/* MAILBOX module features */

/* @brief Mailbox side for current core */
#define FSL_FEATURE_MAILBOX_SIDE_B (1)
/* @brief Mailbox has no reset control */
#define FSL_FEATURE_MAILBOX_HAS_NO_RESET (1)

/* MRT module features */

/* @brief number of channels. */
#define FSL_FEATURE_MRT_NUMBER_OF_CHANNELS  (4)

/* interrupt module features */

/* @brief Lowest interrupt request number. */
#define FSL_FEATURE_INTERRUPT_IRQ_MIN (-14)
/* @brief Highest interrupt request number. */
#define FSL_FEATURE_INTERRUPT_IRQ_MAX (105)

/* PINT module features */

/* @brief Number of connected outputs */
#define FSL_FEATURE_PINT_NUMBER_OF_CONNECTED_OUTPUTS (4)

/* RTC module features */

/* @brief RTC has no reset control */
#define FSL_FEATURE_RTC_HAS_NO_RESET (1)

/* SCT module features */

/* @brief Number of events */
#define FSL_FEATURE_SCT_NUMBER_OF_EVENTS (10)
/* @brief Number of states */
#define FSL_FEATURE_SCT_NUMBER_OF_STATES (10)
/* @brief Number of match capture */
#define FSL_FEATURE_SCT_NUMBER_OF_MATCH_CAPTURE (10)
/* @brief Number of outputs */
#define FSL_FEATURE_SCT_NUMBER_OF_OUTPUTS (8)

/* SYSCON module features */

/* @brief Pointer to ROM IAP entry functions */
#define FSL_FEATURE_SYSCON_IAP_ENTRY_LOCATION (0x03000205)
/* @brief Flash page size in bytes */
#define FSL_FEATURE_SYSCON_FLASH_PAGE_SIZE_BYTES (256)
/* @brief Flash sector size in bytes */
#define FSL_FEATURE_SYSCON_FLASH_SECTOR_SIZE_BYTES (32768)
/* @brief Flash size in bytes */
#define FSL_FEATURE_SYSCON_FLASH_SIZE_BYTES (262144)
/* @brief IAP has Flash read & write function */
#define FSL_FEATURE_IAP_HAS_FLASH_FUNCTION (1)
/* @brief IAP has read Flash signature function  */
#define FSL_FEATURE_IAP_HAS_FLASH_SIGNATURE_READ (1)
/* @brief IAP has read extended Flash signature function */
#define FSL_FEATURE_IAP_HAS_FLASH_EXTENDED_SIGNATURE_READ (0)

/* SysTick module features */

/* @brief Systick has external reference clock. */
#define FSL_FEATURE_SYSTICK_HAS_EXT_REF (0)
/* @brief Systick external reference clock is core clock divided by this value. */
#define FSL_FEATURE_SYSTICK_EXT_REF_CORE_DIV (0)

/* USB module features */

/* @brief Number of the endpoint in USB FS */
#define FSL_FEATURE_USB_EP_NUM (5)

#endif /* _LPC54114_cm0plus_FEATURES_H_ */

