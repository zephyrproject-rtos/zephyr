/*
** ###################################################################
**     Version:             rev. 1.0, 2018-08-22
**     Build:               b190122
**
**     Abstract:
**         Chip specific module features.
**
**     Copyright 2016 Freescale Semiconductor, Inc.
**     Copyright 2016-2019 NXP
**     All rights reserved.
**
**     SPDX-License-Identifier: BSD-3-Clause
**
**     http:                 www.nxp.com
**     mail:                 support@nxp.com
**
**     Revisions:
**     - rev. 1.0 (2018-08-22)
**         Initial version based on v0.2UM
**
** ###################################################################
*/

#ifndef _LPC55S69_cm33_core0_FEATURES_H_
#define _LPC55S69_cm33_core0_FEATURES_H_

/* SOC module features */

/* @brief CASPER availability on the SoC. */
#define FSL_FEATURE_SOC_CASPER_COUNT (1)
/* @brief CRC availability on the SoC. */
#define FSL_FEATURE_SOC_CRC_COUNT (1)
/* @brief CTIMER availability on the SoC. */
#define FSL_FEATURE_SOC_CTIMER_COUNT (5)
/* @brief DMA availability on the SoC. */
#define FSL_FEATURE_SOC_DMA_COUNT (2)
/* @brief FLASH availability on the SoC. */
#define FSL_FEATURE_SOC_FLASH_COUNT (1)
/* @brief FLEXCOMM availability on the SoC. */
#define FSL_FEATURE_SOC_FLEXCOMM_COUNT (9)
/* @brief GINT availability on the SoC. */
#define FSL_FEATURE_SOC_GINT_COUNT (2)
/* @brief GPIO availability on the SoC. */
#define FSL_FEATURE_SOC_GPIO_COUNT (1)
/* @brief SECGPIO availability on the SoC. */
#define FSL_FEATURE_SOC_SECGPIO_COUNT (1)
/* @brief HASHCRYPT availability on the SoC. */
#define FSL_FEATURE_SOC_HASHCRYPT_COUNT (1)
/* @brief I2C availability on the SoC. */
#define FSL_FEATURE_SOC_I2C_COUNT (8)
/* @brief I2S availability on the SoC. */
#define FSL_FEATURE_SOC_I2S_COUNT (8)
/* @brief INPUTMUX availability on the SoC. */
#define FSL_FEATURE_SOC_INPUTMUX_COUNT (1)
/* @brief IOCON availability on the SoC. */
#define FSL_FEATURE_SOC_IOCON_COUNT (1)
/* @brief LPADC availability on the SoC. */
#define FSL_FEATURE_SOC_LPADC_COUNT (1)
/* @brief MAILBOX availability on the SoC. */
#define FSL_FEATURE_SOC_MAILBOX_COUNT (1)
/* @brief MRT availability on the SoC. */
#define FSL_FEATURE_SOC_MRT_COUNT (1)
/* @brief OSTIMER availability on the SoC. */
#define FSL_FEATURE_SOC_OSTIMER_COUNT (1)
/* @brief PINT availability on the SoC. */
#define FSL_FEATURE_SOC_PINT_COUNT (1)
/* @brief SECPINT availability on the SoC. */
#define FSL_FEATURE_SOC_SECPINT_COUNT (1)
/* @brief PMC availability on the SoC. */
#define FSL_FEATURE_SOC_PMC_COUNT (1)
/* @brief POWERQUAD availability on the SoC. */
#define FSL_FEATURE_SOC_POWERQUAD_COUNT (1)
/* @brief PUF availability on the SoC. */
#define FSL_FEATURE_SOC_PUF_COUNT (1)
/* @brief RNG1 availability on the SoC. */
#define FSL_FEATURE_SOC_LPC_RNG1_COUNT (1)
/* @brief RTC availability on the SoC. */
#define FSL_FEATURE_SOC_RTC_COUNT (1)
/* @brief SCT availability on the SoC. */
#define FSL_FEATURE_SOC_SCT_COUNT (1)
/* @brief SDIF availability on the SoC. */
#define FSL_FEATURE_SOC_SDIF_COUNT (1)
/* @brief SPI availability on the SoC. */
#define FSL_FEATURE_SOC_SPI_COUNT (9)
/* @brief SYSCON availability on the SoC. */
#define FSL_FEATURE_SOC_SYSCON_COUNT (1)
/* @brief SYSCTL1 availability on the SoC. */
#define FSL_FEATURE_SOC_SYSCTL1_COUNT (1)
/* @brief USART availability on the SoC. */
#define FSL_FEATURE_SOC_USART_COUNT (8)
/* @brief USB availability on the SoC. */
#define FSL_FEATURE_SOC_USB_COUNT (1)
/* @brief USBFSH availability on the SoC. */
#define FSL_FEATURE_SOC_USBFSH_COUNT (1)
/* @brief USBHSD availability on the SoC. */
#define FSL_FEATURE_SOC_USBHSD_COUNT (1)
/* @brief USBHSH availability on the SoC. */
#define FSL_FEATURE_SOC_USBHSH_COUNT (1)
/* @brief USBPHY availability on the SoC. */
#define FSL_FEATURE_SOC_USBPHY_COUNT (1)
/* @brief UTICK availability on the SoC. */
#define FSL_FEATURE_SOC_UTICK_COUNT (1)
/* @brief WWDT availability on the SoC. */
#define FSL_FEATURE_SOC_WWDT_COUNT (1)

/* LPADC module features */

/* @brief FIFO availability on the SoC. */
#define FSL_FEATURE_LPADC_FIFO_COUNT (2)
/* @brief Has differential mode (bitfield CMDLn[DIFF]). */
#define FSL_FEATURE_LPADC_HAS_CMDL_DIFF (0)
/* @brief Has channel scale (bitfield CMDLn[CSCALE]). */
#define FSL_FEATURE_LPADC_HAS_CMDL_CSCALE (0)
/* @brief Has conversion type select (bitfield CMDLn[CTYPE]). */
#define FSL_FEATURE_LPADC_HAS_CMDL_CTYPE (1)
/* @brief Has conversion resolution select  (bitfield CMDLn[MODE]). */
#define FSL_FEATURE_LPADC_HAS_CMDL_MODE (1)
/* @brief Has compare function enable (bitfield CMDHn[CMPEN]). */
#define FSL_FEATURE_LPADC_HAS_CMDH_CMPEN (1)
/* @brief Has Wait for trigger assertion before execution (bitfield CMDHn[WAIT_TRIG]). */
#define FSL_FEATURE_LPADC_HAS_CMDH_WAIT_TRIG (1)
/* @brief Has offset calibration (bitfield CTRL[CALOFS]). */
#define FSL_FEATURE_LPADC_HAS_CTRL_CALOFS (1)
/* @brief Has gain calibration (bitfield CTRL[CAL_REQ]). */
#define FSL_FEATURE_LPADC_HAS_CTRL_CAL_REQ (1)
/* @brief Has calibration average (bitfield CTRL[CAL_AVGS]). */
#define FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS (1)
/* @brief Has internal clock (bitfield CFG[ADCKEN]). */
#define FSL_FEATURE_LPADC_HAS_CFG_ADCKEN (0)
/* @brief Enable support for low voltage reference on option 1 reference (bitfield CFG[VREF1RNG]). */
#define FSL_FEATURE_LPADC_HAS_CFG_VREF1RNG (0)
/* @brief Has calibration (bitfield CFG[CALOFS]). */
#define FSL_FEATURE_LPADC_HAS_CFG_CALOFS (0)
/* @brief Has offset trim (register OFSTRIM). */
#define FSL_FEATURE_LPADC_HAS_OFSTRIM (1)

/* CASPER module features */

/* @brief Base address of the CASPER dedicated RAM */
#define FSL_FEATURE_CASPER_RAM_BASE_ADDRESS (0x04000000)
/* @brief Interleaving of the CASPER dedicated RAM */
#define FSL_FEATURE_CASPER_RAM_IS_INTERLEAVED (1)

/* DMA module features */

/* @brief Number of channels */
#define FSL_FEATURE_DMA_NUMBER_OF_CHANNELS (30)

/* HASHCRYPT module features */

/* @brief the address of alias offset */
#define FSL_FEATURE_HASHCRYPT_ALIAS_OFFSET (0x00000000)

/* I2S module features */

/* @brief I2S support dual channel transfer. */
#define FSL_FEATURE_I2S_SUPPORT_SECONDARY_CHANNEL (1)

/* IOCON module features */

/* @brief Func bit field width */
#define FSL_FEATURE_IOCON_FUNC_FIELD_WIDTH (4)

/* MAILBOX module features */

/* @brief Mailbox side for current core */
#define FSL_FEATURE_MAILBOX_SIDE_A (1)

/* MRT module features */

/* @brief number of channels. */
#define FSL_FEATURE_MRT_NUMBER_OF_CHANNELS  (4)

/* PINT module features */

/* @brief Number of connected outputs */
#define FSL_FEATURE_PINT_NUMBER_OF_CONNECTED_OUTPUTS (10)

/* POWERLIB module features */

/* @brief Niobe4's Powerlib API is different with other LPC series devices. */
#define FSL_FEATURE_POWERLIB_NIOBE4_EXTEND (1)

/* POWERQUAD module features */

/* @brief Sine and Cossine fix errata */
#define FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA  (1)

/* PUF module features */

/* @brief Number of PUF key slots available on device. */
#define FSL_FEATURE_PUF_HAS_KEYSLOTS (4)
/* @brief the shift status value */
#define FSL_FEATURE_PUF_HAS_SHIFT_STATUS (1)

/* SCT module features */

/* @brief Number of events */
#define FSL_FEATURE_SCT_NUMBER_OF_EVENTS (16)
/* @brief Number of states */
#define FSL_FEATURE_SCT_NUMBER_OF_STATES (32)
/* @brief Number of match capture */
#define FSL_FEATURE_SCT_NUMBER_OF_MATCH_CAPTURE (16)
/* @brief Number of outputs */
#define FSL_FEATURE_SCT_NUMBER_OF_OUTPUTS (10)

/* SDIF module features */

/* @brief FIFO depth, every location is a WORD */
#define FSL_FEATURE_SDIF_FIFO_DEPTH_64_32BITS  (64)
/* @brief Max DMA buffer size */
#define FSL_FEATURE_SDIF_INTERNAL_DMA_MAX_BUFFER_SIZE  (4096)
/* @brief Max source clock in HZ */
#define FSL_FEATURE_SDIF_MAX_SOURCE_CLOCK  (52000000)
/* @brief support 2 cards */
#define FSL_FEATURE_SDIF_ONE_INSTANCE_SUPPORT_TWO_CARD  (1)

/* SECPINT module features */

/* @brief Number of connected outputs */
#define FSL_FEATURE_SECPINT_NUMBER_OF_CONNECTED_OUTPUTS (2)

/* SYSCON module features */

/* @brief Pointer to ROM IAP entry functions */
#define FSL_FEATURE_SYSCON_IAP_ENTRY_LOCATION (0x03000205)
/* @brief Flash page size in bytes */
#define FSL_FEATURE_SYSCON_FLASH_PAGE_SIZE_BYTES (512)
/* @brief Flash sector size in bytes */
#define FSL_FEATURE_SYSCON_FLASH_SECTOR_SIZE_BYTES (32768)
/* @brief Flash size in bytes */
#define FSL_FEATURE_SYSCON_FLASH_SIZE_BYTES (622592)
/* @brief Has Power Down mode */
#define FSL_FEATURE_SYSCON_HAS_POWERDOWN_MODE (1)
/* @brief CCM_ANALOG availability on the SoC.  */
#define FSL_FEATURE_SOC_CCM_ANALOG_COUNT (1)

/* USB module features */

/* @brief Size of the USB dedicated RAM */
#define FSL_FEATURE_USB_USB_RAM (0x00004000)
/* @brief Base address of the USB dedicated RAM */
#define FSL_FEATURE_USB_USB_RAM_BASE_ADDRESS (0x40100000)
/* @brief USB version */
#define FSL_FEATURE_USB_VERSION (200)
/* @brief Number of the endpoint in USB FS */
#define FSL_FEATURE_USB_EP_NUM (5)

/* USBFSH module features */

/* @brief Size of the USB dedicated RAM */
#define FSL_FEATURE_USBFSH_USB_RAM (0x00004000)
/* @brief Base address of the USB dedicated RAM */
#define FSL_FEATURE_USBFSH_USB_RAM_BASE_ADDRESS (0x40100000)
/* @brief USBFSH version */
#define FSL_FEATURE_USBFSH_VERSION (200)

/* USBHSD module features */

/* @brief Size of the USB dedicated RAM */
#define FSL_FEATURE_USBHSD_USB_RAM (0x00004000)
/* @brief Base address of the USB dedicated RAM */
#define FSL_FEATURE_USBHSD_USB_RAM_BASE_ADDRESS (0x40100000)
/* @brief USBHSD version */
#define FSL_FEATURE_USBHSD_VERSION (300)
/* @brief Number of the endpoint in USB HS */
#define FSL_FEATURE_USBHSD_EP_NUM (6)

/* USBHSH module features */

/* @brief Size of the USB dedicated RAM */
#define FSL_FEATURE_USBHSH_USB_RAM (0x00004000)
/* @brief Base address of the USB dedicated RAM */
#define FSL_FEATURE_USBHSH_USB_RAM_BASE_ADDRESS (0x40100000)
/* @brief USBHSH version */
#define FSL_FEATURE_USBHSH_VERSION (300)

/* UTICK module features */

/* @brief UTICK does not support PD configure. */
#define FSL_FEATURE_UTICK_HAS_NO_PDCFG (1)

/* WWDT module features */

/* @brief WWDT does not support oscillator lock. */
#define FSL_FEATURE_WWDT_HAS_NO_OSCILLATOR_LOCK (1)
/* @brief WWDT does not support power down configure */
#define FSL_FEATURE_WWDT_HAS_NO_PDCFG (1)

#endif /* _LPC55S69_cm33_core0_FEATURES_H_ */

