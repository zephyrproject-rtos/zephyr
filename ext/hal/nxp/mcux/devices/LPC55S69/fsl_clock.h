/*
 * Copyright (c) 2017 - 2018 , NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_CLOCK_H_
#define _FSL_CLOCK_H_

#include "fsl_device_registers.h"
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/*! @addtogroup clock */
/*! @{ */

/*! @file */

/*******************************************************************************
 * Definitions
 *****************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief CLOCK driver version 2.0.3. */
#define FSL_CLOCK_DRIVER_VERSION (MAKE_VERSION(2, 0, 3))
/*@}*/

/*! @brief Configure whether driver controls clock
 *
 * When set to 0, peripheral drivers will enable clock in initialize function
 * and disable clock in de-initialize function. When set to 1, peripheral
 * driver will not control the clock, application could control the clock out of
 * the driver.
 *
 * @note All drivers share this feature switcher. If it is set to 1, application
 * should handle clock enable and disable for all drivers.
 */
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL))
#define FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL 0
#endif

/*!
 * @brief User-defined the size of cache for CLOCK_PllGetConfig() function.
 *
 * Once define this MACRO to be non-zero value, CLOCK_PllGetConfig() function
 * would cache the recent calulation and accelerate the execution to get the
 * right settings.
 */
#ifndef CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT
#define CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT 2U
#endif

/*! @brief Clock ip name array for ROM. */
#define ROM_CLOCKS \
    {              \
        kCLOCK_Rom \
    }
/*! @brief Clock ip name array for SRAM. */
#define SRAM_CLOCKS                                            \
    {                                                          \
        kCLOCK_Sram1, kCLOCK_Sram2, kCLOCK_Sram3, kCLOCK_Sram4 \
    }
/*! @brief Clock ip name array for FLASH. */
#define FLASH_CLOCKS \
    {                \
        kCLOCK_Flash \
    }
/*! @brief Clock ip name array for FMC. */
#define FMC_CLOCKS \
    {              \
        kCLOCK_Fmc \
    }
/*! @brief Clock ip name array for INPUTMUX. */
#define INPUTMUX_CLOCKS                    \
    {                                      \
        kCLOCK_InputMux0, kCLOCK_InputMux1 \
    }
/*! @brief Clock ip name array for IOCON. */
#define IOCON_CLOCKS \
    {                \
        kCLOCK_Iocon \
    }
/*! @brief Clock ip name array for GPIO. */
#define GPIO_CLOCKS                                                                        \
    {                                                                                      \
        kCLOCK_Gpio0, kCLOCK_Gpio1, kCLOCK_Gpio2, kCLOCK_Gpio3, kCLOCK_Gpio4, kCLOCK_Gpio5 \
    }
/*! @brief Clock ip name array for PINT. */
#define PINT_CLOCKS \
    {               \
        kCLOCK_Pint \
    }
/*! @brief Clock ip name array for GINT. */
#define GINT_CLOCKS \
    {               \
        kCLOCK_Gint, kCLOCK_Gint \
    }
/*! @brief Clock ip name array for DMA. */
#define DMA_CLOCKS               \
    {                            \
        kCLOCK_Dma0, kCLOCK_Dma1 \
    }
/*! @brief Clock ip name array for CRC. */
#define CRC_CLOCKS \
    {              \
        kCLOCK_Crc \
    }
/*! @brief Clock ip name array for WWDT. */
#define WWDT_CLOCKS \
    {               \
        kCLOCK_Wwdt \
    }
/*! @brief Clock ip name array for RTC. */
#define RTC_CLOCKS \
    {              \
        kCLOCK_Rtc \
    }
/*! @brief Clock ip name array for Mailbox. */
#define MAILBOX_CLOCKS \
    {                  \
        kCLOCK_Mailbox \
    }
/*! @brief Clock ip name array for LPADC. */
#define LPADC_CLOCKS \
    {                \
        kCLOCK_Adc0  \
    }
/*! @brief Clock ip name array for MRT. */
#define MRT_CLOCKS \
    {              \
        kCLOCK_Mrt \
    }
/*! @brief Clock ip name array for OSTIMER. */
#define OSTIMER_CLOCKS  \
    {                   \
        kCLOCK_OsTimer0 \
    }
/*! @brief Clock ip name array for SCT0. */
#define SCT_CLOCKS  \
    {               \
        kCLOCK_Sct0 \
    }
/*! @brief Clock ip name array for SCTIPU. */
#define SCTIPU_CLOCKS \
    {                 \
        kCLOCK_Sctipu \
    }
/*! @brief Clock ip name array for UTICK. */
#define UTICK_CLOCKS  \
    {                 \
        kCLOCK_Utick0 \
    }
/*! @brief Clock ip name array for FLEXCOMM. */
#define FLEXCOMM_CLOCKS                                                                                             \
    {                                                                                                               \
        kCLOCK_FlexComm0, kCLOCK_FlexComm1, kCLOCK_FlexComm2, kCLOCK_FlexComm3, kCLOCK_FlexComm4, kCLOCK_FlexComm5, \
            kCLOCK_FlexComm6, kCLOCK_FlexComm7, kCLOCK_Hs_Lspi                                                      \
    }
/*! @brief Clock ip name array for LPUART. */
#define LPUART_CLOCKS                                                                                         \
    {                                                                                                         \
        kCLOCK_MinUart0, kCLOCK_MinUart1, kCLOCK_MinUart2, kCLOCK_MinUart3, kCLOCK_MinUart4, kCLOCK_MinUart5, \
            kCLOCK_MinUart6, kCLOCK_MinUart7                                                                  \
    }

/*! @brief Clock ip name array for BI2C. */
#define BI2C_CLOCKS                                                                                                    \
    {                                                                                                                  \
        kCLOCK_BI2c0, kCLOCK_BI2c1, kCLOCK_BI2c2, kCLOCK_BI2c3, kCLOCK_BI2c4, kCLOCK_BI2c5, kCLOCK_BI2c6, kCLOCK_BI2c7 \
    }
/*! @brief Clock ip name array for LSPI. */
#define LPSPI_CLOCKS                                                                                                   \
    {                                                                                                                  \
        kCLOCK_LSpi0, kCLOCK_LSpi1, kCLOCK_LSpi2, kCLOCK_LSpi3, kCLOCK_LSpi4, kCLOCK_LSpi5, kCLOCK_LSpi6, kCLOCK_LSpi7 \
    }
/*! @brief Clock ip name array for FLEXI2S. */
#define FLEXI2S_CLOCKS                                                                                        \
    {                                                                                                         \
        kCLOCK_FlexI2s0, kCLOCK_FlexI2s1, kCLOCK_FlexI2s2, kCLOCK_FlexI2s3, kCLOCK_FlexI2s4, kCLOCK_FlexI2s5, \
            kCLOCK_FlexI2s6, kCLOCK_FlexI2s7                                                                  \
    }
/*! @brief Clock ip name array for USBTYPC. */
#define USBTYPC_CLOCKS \
    {                  \
        kCLOCK_UsbTypc \
    }
/*! @brief Clock ip name array for CTIMER. */
#define CTIMER_CLOCKS                                                             \
    {                                                                             \
        kCLOCK_Timer0, kCLOCK_Timer1, kCLOCK_Timer2, kCLOCK_Timer3, kCLOCK_Timer4 \
    }
/*! @brief Clock ip name array for PVT */
#define PVT_CLOCKS \
    {              \
        kCLOCK_Pvt \
    }
/*! @brief Clock ip name array for EZHA */
#define EZHA_CLOCKS \
    {               \
        kCLOCK_Ezha \
    }
/*! @brief Clock ip name array for EZHB */
#define EZHB_CLOCKS \
    {               \
        kCLOCK_Ezhb \
    }
/*! @brief Clock ip name array for COMP */
#define COMP_CLOCKS \
    {               \
        kCLOCK_Comp \
    }
/*! @brief Clock ip name array for SDIO. */
#define SDIO_CLOCKS \
    {               \
        kCLOCK_Sdio \
    }
/*! @brief Clock ip name array for USB1CLK. */
#define USB1CLK_CLOCKS \
    {                  \
        kCLOCK_Usb1Clk \
    }
/*! @brief Clock ip name array for FREQME. */
#define FREQME_CLOCKS \
    {                 \
        kCLOCK_Freqme \
    }
/*! @brief Clock ip name array for USBRAM. */
#define USBRAM_CLOCKS  \
    {                  \
        kCLOCK_UsbRam1 \
    }
/*! @brief Clock ip name array for OTP. */
#define OTP_CLOCKS \
    {              \
        kCLOCK_Otp \
    }
/*! @brief Clock ip name array for RNG. */
#define RNG_CLOCKS \
    {              \
        kCLOCK_Rng \
    }
/*! @brief Clock ip name array for USBHMR0. */
#define USBHMR0_CLOCKS \
    {                  \
        kCLOCK_Usbhmr0 \
    }
/*! @brief Clock ip name array for USBHSL0. */
#define USBHSL0_CLOCKS \
    {                  \
        kCLOCK_Usbhsl0 \
    }
/*! @brief Clock ip name array for HashCrypt. */
#define HASHCRYPT_CLOCKS \
    {                    \
        kCLOCK_HashCrypt \
    }
/*! @brief Clock ip name array for PowerQuad. */
#define POWERQUAD_CLOCKS \
    {                    \
        kCLOCK_PowerQuad \
    }
/*! @brief Clock ip name array for PLULUT. */
#define PLULUT_CLOCKS \
    {                 \
        kCLOCK_PluLut \
    }
/*! @brief Clock ip name array for PUF. */
#define PUF_CLOCKS \
    {              \
        kCLOCK_Puf \
    }
/*! @brief Clock ip name array for CASPER. */
#define CASPER_CLOCKS \
    {                 \
        kCLOCK_Casper \
    }
/*! @brief Clock ip name array for ANALOGCTRL. */
#define ANALOGCTRL_CLOCKS \
    {                     \
        kCLOCK_AnalogCtrl \
    }
/*! @brief Clock ip name array for HS_LSPI. */
#define HS_LSPI_CLOCKS \
    {                  \
        kCLOCK_Hs_Lspi \
    }
/*! @brief Clock ip name array for GPIO_SEC. */
#define GPIO_SEC_CLOCKS \
    {                   \
        kCLOCK_Gpio_Sec \
    }
/*! @brief Clock ip name array for GPIO_SEC_INT. */
#define GPIO_SEC_INT_CLOCKS \
    {                       \
        kCLOCK_Gpio_Sec_Int \
    }
/*! @brief Clock ip name array for USBD. */
#define USBD_CLOCKS                              \
    {                                            \
        kCLOCK_Usbd0, kCLOCK_Usbh1, kCLOCK_Usbd1 \
    }
/*! @brief Clock ip name array for USBH. */
#define USBH_CLOCKS  \
    {                \
        kCLOCK_Usbh1 \
    }
#define PLU_CLOCKS    \
    {                 \
        kCLOCK_PluLut \
    }
#define SYSCTL_CLOCKS    \
    {                 \
        kCLOCK_Sysctl \
    }
/*! @brief Clock gate name used for CLOCK_EnableClock/CLOCK_DisableClock. */
/*------------------------------------------------------------------------------
 clock_ip_name_t definition:
------------------------------------------------------------------------------*/

#define CLK_GATE_REG_OFFSET_SHIFT 8U
#define CLK_GATE_REG_OFFSET_MASK 0xFFFFFF00U
#define CLK_GATE_BIT_SHIFT_SHIFT 0U
#define CLK_GATE_BIT_SHIFT_MASK 0x000000FFU

#define CLK_GATE_DEFINE(reg_offset, bit_shift)                                  \
    ((((reg_offset) << CLK_GATE_REG_OFFSET_SHIFT) & CLK_GATE_REG_OFFSET_MASK) | \
     (((bit_shift) << CLK_GATE_BIT_SHIFT_SHIFT) & CLK_GATE_BIT_SHIFT_MASK))

#define CLK_GATE_ABSTRACT_REG_OFFSET(x) (((uint32_t)(x)&CLK_GATE_REG_OFFSET_MASK) >> CLK_GATE_REG_OFFSET_SHIFT)
#define CLK_GATE_ABSTRACT_BITS_SHIFT(x) (((uint32_t)(x)&CLK_GATE_BIT_SHIFT_MASK) >> CLK_GATE_BIT_SHIFT_SHIFT)

#define AHB_CLK_CTRL0 0
#define AHB_CLK_CTRL1 1
#define AHB_CLK_CTRL2 2

/*! @brief Clock gate name used for CLOCK_EnableClock/CLOCK_DisableClock. */
typedef enum _clock_ip_name
{
    kCLOCK_IpInvalid = 0U,
    kCLOCK_Rom = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 1),
    kCLOCK_Sram1 = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 3),
    kCLOCK_Sram2 = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 4),
    kCLOCK_Sram3 = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 5),
    kCLOCK_Sram4 = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 6),
    kCLOCK_Flash = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 7),
    kCLOCK_Fmc = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 8),
    kCLOCK_InputMux = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 11),
    kCLOCK_Iocon = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 13),
    kCLOCK_Gpio0 = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 14),
    kCLOCK_Gpio1 = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 15),
    kCLOCK_Gpio2 = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 16),
    kCLOCK_Gpio3 = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 17),
    kCLOCK_Pint = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 18),
    kCLOCK_Gint = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 19),
    kCLOCK_Dma0 = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 20),
    kCLOCK_Crc = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 21),
    kCLOCK_Wwdt = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 22),
    kCLOCK_Rtc = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 23),
    kCLOCK_Mailbox = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 26),
    kCLOCK_Adc0 = CLK_GATE_DEFINE(AHB_CLK_CTRL0, 27),
    kCLOCK_Mrt = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 0),
    kCLOCK_OsTimer0 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 1),
    kCLOCK_Sct0 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 2),
    kCLOCK_Sctipu = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 6),
    kCLOCK_Utick0 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 10),
    kCLOCK_FlexComm0 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 11),
    kCLOCK_FlexComm1 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 12),
    kCLOCK_FlexComm2 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 13),
    kCLOCK_FlexComm3 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 14),
    kCLOCK_FlexComm4 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 15),
    kCLOCK_FlexComm5 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 16),
    kCLOCK_FlexComm6 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 17),
    kCLOCK_FlexComm7 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 18),
    kCLOCK_MinUart0 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 11),
    kCLOCK_MinUart1 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 12),
    kCLOCK_MinUart2 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 13),
    kCLOCK_MinUart3 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 14),
    kCLOCK_MinUart4 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 15),
    kCLOCK_MinUart5 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 16),
    kCLOCK_MinUart6 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 17),
    kCLOCK_MinUart7 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 18),
    kCLOCK_LSpi0 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 11),
    kCLOCK_LSpi1 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 12),
    kCLOCK_LSpi2 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 13),
    kCLOCK_LSpi3 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 14),
    kCLOCK_LSpi4 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 15),
    kCLOCK_LSpi5 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 16),
    kCLOCK_LSpi6 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 17),
    kCLOCK_LSpi7 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 18),
    kCLOCK_BI2c0 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 11),
    kCLOCK_BI2c1 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 12),
    kCLOCK_BI2c2 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 13),
    kCLOCK_BI2c3 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 14),
    kCLOCK_BI2c4 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 15),
    kCLOCK_BI2c5 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 16),
    kCLOCK_BI2c6 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 17),
    kCLOCK_BI2c7 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 18),
    kCLOCK_FlexI2s0 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 11),
    kCLOCK_FlexI2s1 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 12),
    kCLOCK_FlexI2s2 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 13),
    kCLOCK_FlexI2s3 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 14),
    kCLOCK_FlexI2s4 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 15),
    kCLOCK_FlexI2s5 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 16),
    kCLOCK_FlexI2s6 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 17),
    kCLOCK_FlexI2s7 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 18),
    kCLOCK_UsbTypc = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 20),
    kCLOCK_Timer2 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 22),
    kCLOCK_Usbd0 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 25),
    kCLOCK_Timer0 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 26),
    kCLOCK_Timer1 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 27),
    kCLOCK_Pvt = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 28),
    kCLOCK_Ezha = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 30),
    kCLOCK_Ezhb = CLK_GATE_DEFINE(AHB_CLK_CTRL1, 31),
    kCLOCK_Dma1 = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 1),
    kCLOCK_Comp = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 2),
    kCLOCK_Sdio = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 3),
    kCLOCK_Usbh1 = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 4),
    kCLOCK_Usbd1 = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 5),
    kCLOCK_UsbRam1 = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 6),
    kCLOCK_Usb1Clk = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 7),
    kCLOCK_Freqme = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 8),
    kCLOCK_Gpio4 = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 9),
    kCLOCK_Gpio5 = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 10),
    kCLOCK_Otp = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 12),
    kCLOCK_Rng = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 13),
    kCLOCK_InputMux1 = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 14),
    kCLOCK_Sysctl = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 15),    
    kCLOCK_Usbhmr0 = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 16),
    kCLOCK_Usbhsl0 = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 17),
    kCLOCK_HashCrypt = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 18),
    kCLOCK_PowerQuad = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 19),
    kCLOCK_PluLut = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 20),
    kCLOCK_Timer3 = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 21),
    kCLOCK_Timer4 = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 22),
    kCLOCK_Puf = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 23),
    kCLOCK_Casper = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 24),
    kCLOCK_AnalogCtrl = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 27),
    kCLOCK_Hs_Lspi = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 28),
    kCLOCK_Gpio_Sec = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 29),
    kCLOCK_Gpio_sec_Int = CLK_GATE_DEFINE(AHB_CLK_CTRL2, 30)
} clock_ip_name_t;

/*! @brief Peripherals clock source definition. */
#define BUS_CLK kCLOCK_BusClk

#define I2C0_CLK_SRC BUS_CLK

/*! @brief Clock name used to get clock frequency. */
typedef enum _clock_name
{
    kCLOCK_CoreSysClk, /*!< Core/system clock  (aka MAIN_CLK)                       */
    kCLOCK_BusClk,     /*!< Bus clock (AHB clock)                                   */
    kCLOCK_ClockOut,   /*!< CLOCKOUT                                                */
    kCLOCK_FroHf,      /*!< FRO48/96                                                */
    kCLOCK_Adc,        /*!< ADC                                                     */
    kCLOCK_Usb0,       /*!< USB0                                                    */
    kCLOCK_Usb1,       /*!< USB1                                                    */
    kCLOCK_Pll1Out,    /*!< PLL1 Output                                             */
    kCLOCK_Mclk,       /*!< MCLK                                                    */
    kCLOCK_Sct,        /*!< SCT                                                     */
    kCLOCK_SDio,       /*!< SDIO                                                    */
    kCLOCK_Fro12M,     /*!< FRO12M                                                  */
    kCLOCK_ExtClk,     /*!< External Clock                                          */
    kCLOCK_Pll0Out,    /*!< PLL0 Output                                             */
    kCLOCK_WdtClk,     /*!< Watchdog clock                                          */
    kCLOCK_FlexI2S,    /*!< FlexI2S clock                                           */
    kCLOCK_Flexcomm0,  /*!< Flexcomm0Clock                                          */
    kCLOCK_Flexcomm1,  /*!< Flexcomm1Clock                                          */
    kCLOCK_Flexcomm2,  /*!< Flexcomm2Clock                                          */
    kCLOCK_Flexcomm3,  /*!< Flexcomm3Clock                                          */
    kCLOCK_Flexcomm4,  /*!< Flexcomm4Clock                                          */
    kCLOCK_Flexcomm5,  /*!< Flexcomm5Clock                                          */
    kCLOCK_Flexcomm6,  /*!< Flexcomm6Clock                                          */
    kCLOCK_Flexcomm7,  /*!< Flexcomm7Clock                                          */
    kCLOCK_HsLspi,     /*!< HS LPSPI Clock                                          */
    kCLOCK_CTmier0,    /*!< CTmier0Clock                                            */
    kCLOCK_CTmier1,    /*!< CTmier1Clock                                            */
    kCLOCK_CTmier2,    /*!< CTmier2Clock                                            */
    kCLOCK_CTmier3,    /*!< CTmier3Clock                                            */
    kCLOCK_CTmier4,    /*!< CTmier4Clock                                            */
    kCLOCK_Systick0,   /*!< System Tick 0 Clock                                     */
    kCLOCK_Systick1,   /*!< System Tick 1 Clock                                     */

} clock_name_t;

/*! @brief Clock Mux Switches
*  The encoding is as follows each connection identified is 32bits wide while 24bits are valuable
*  starting from LSB upwards
*
*  [4 bits for choice, 0 means invalid choice] [8 bits mux ID]*
*
*/

#define CLK_ATTACH_ID(mux, sel, pos) (((mux << 0U) | ((sel + 1) & 0xFU) << 8U) << (pos * 12U))
#define MUX_A(mux, sel) CLK_ATTACH_ID(mux, sel, 0U)
#define MUX_B(mux, sel, selector) (CLK_ATTACH_ID(mux, sel, 1U) | (selector << 24U))

#define GET_ID_ITEM(connection) ((connection)&0xFFFU)
#define GET_ID_NEXT_ITEM(connection) ((connection) >> 12U)
#define GET_ID_ITEM_MUX(connection) ((connection)&0xFFU)
#define GET_ID_ITEM_SEL(connection) ((((connection)&0xF00U) >> 8U) - 1U)
#define GET_ID_SELECTOR(connection) ((connection)&0xF000000U)

#define CM_SYSTICKCLKSEL0 0
#define CM_SYSTICKCLKSEL1 1
#define CM_TRACECLKSEL 2
#define CM_CTIMERCLKSEL0 3
#define CM_CTIMERCLKSEL1 4
#define CM_CTIMERCLKSEL2 5
#define CM_CTIMERCLKSEL3 6
#define CM_CTIMERCLKSEL4 7
#define CM_MAINCLKSELA 8
#define CM_MAINCLKSELB 9
#define CM_CLKOUTCLKSEL 10
#define CM_PLL0CLKSEL 12
#define CM_PLL1CLKSEL 13
#define CM_ADCASYNCCLKSEL 17
#define CM_USB0CLKSEL 18
#define CM_FXCOMCLKSEL0 20
#define CM_FXCOMCLKSEL1 21
#define CM_FXCOMCLKSEL2 22
#define CM_FXCOMCLKSEL3 23
#define CM_FXCOMCLKSEL4 24
#define CM_FXCOMCLKSEL5 25
#define CM_FXCOMCLKSEL6 26
#define CM_FXCOMCLKSEL7 27
#define CM_HSLSPICLKSEL 28
#define CM_MCLKCLKSEL 32
#define CM_SCTCLKSEL 36
#define CM_SDIOCLKSEL 38

#define CM_RTCOSC32KCLKSEL 63

typedef enum _clock_attach_id
{

    kFRO12M_to_MAIN_CLK = MUX_A(CM_MAINCLKSELA, 0) | MUX_B(CM_MAINCLKSELB, 0, 0),
    kEXT_CLK_to_MAIN_CLK = MUX_A(CM_MAINCLKSELA, 1) | MUX_B(CM_MAINCLKSELB, 0, 0),
    kFRO1M_to_MAIN_CLK = MUX_A(CM_MAINCLKSELA, 2) | MUX_B(CM_MAINCLKSELB, 0, 0),
    kFRO_HF_to_MAIN_CLK = MUX_A(CM_MAINCLKSELA, 3) | MUX_B(CM_MAINCLKSELB, 0, 0),
    kPLL0_to_MAIN_CLK = MUX_A(CM_MAINCLKSELA, 0) | MUX_B(CM_MAINCLKSELB, 1, 0),
    kPLL1_to_MAIN_CLK = MUX_A(CM_MAINCLKSELA, 0) | MUX_B(CM_MAINCLKSELB, 2, 0),
    kOSC32K_to_MAIN_CLK = MUX_A(CM_MAINCLKSELA, 0) | MUX_B(CM_MAINCLKSELB, 3, 0),

    kMAIN_CLK_to_CLKOUT = MUX_A(CM_CLKOUTCLKSEL, 0),
    kPLL0_to_CLKOUT = MUX_A(CM_CLKOUTCLKSEL, 1),
    kEXT_CLK_to_CLKOUT = MUX_A(CM_CLKOUTCLKSEL, 2),
    kFRO_HF_to_CLKOUT = MUX_A(CM_CLKOUTCLKSEL, 3),
    kFRO1M_to_CLKOUT = MUX_A(CM_CLKOUTCLKSEL, 4),
    kPLL1_to_CLKOUT = MUX_A(CM_CLKOUTCLKSEL, 5),
    kOSC32K_to_CLKOUT = MUX_A(CM_CLKOUTCLKSEL, 6),
    kNONE_to_SYS_CLKOUT = MUX_A(CM_CLKOUTCLKSEL, 7),

    kFRO12M_to_PLL0 = MUX_A(CM_PLL0CLKSEL, 0),
    kEXT_CLK_to_PLL0 = MUX_A(CM_PLL0CLKSEL, 1),
    kFRO1M_to_PLL0 = MUX_A(CM_PLL0CLKSEL, 2),
    kOSC32K_to_PLL0 = MUX_A(CM_PLL0CLKSEL, 3),
    kNONE_to_PLL0 = MUX_A(CM_PLL0CLKSEL, 7),

    kMAIN_CLK_to_ADC_CLK = MUX_A(CM_ADCASYNCCLKSEL, 0),
    kPLL0_to_ADC_CLK = MUX_A(CM_ADCASYNCCLKSEL, 1),
    kFRO_HF_to_ADC_CLK = MUX_A(CM_ADCASYNCCLKSEL, 2),
    kFRO1M_to_ADC_CLK = MUX_A(CM_ADCASYNCCLKSEL, 3), /* Need confirm */
    kNONE_to_ADC_CLK = MUX_A(CM_ADCASYNCCLKSEL, 7),

    kMAIN_CLK_to_USB0_CLK = MUX_A(CM_USB0CLKSEL, 0),
    kPLL0_to_USB0_CLK = MUX_A(CM_USB0CLKSEL, 1),
    kFRO_HF_to_USB0_CLK = MUX_A(CM_USB0CLKSEL, 3),
    kPLL1_to_USB0_CLK = MUX_A(CM_USB0CLKSEL, 5),
    kNONE_to_USB0_CLK = MUX_A(CM_USB0CLKSEL, 7),

    kMAIN_CLK_to_FLEXCOMM0 = MUX_A(CM_FXCOMCLKSEL0, 0),
    kPLL0_DIV_to_FLEXCOMM0 = MUX_A(CM_FXCOMCLKSEL0, 1),
    kFRO12M_to_FLEXCOMM0 = MUX_A(CM_FXCOMCLKSEL0, 2),
    kFRO_HF_DIV_to_FLEXCOMM0 = MUX_A(CM_FXCOMCLKSEL0, 3),
    kFRO1M_to_FLEXCOMM0 = MUX_A(CM_FXCOMCLKSEL0, 4),
    kMCLK_to_FLEXCOMM0 = MUX_A(CM_FXCOMCLKSEL0, 5),
    kOSC32K_to_FLEXCOMM0 = MUX_A(CM_FXCOMCLKSEL0, 6),
    kNONE_to_FLEXCOMM0 = MUX_A(CM_FXCOMCLKSEL0, 7),

    kMAIN_CLK_to_FLEXCOMM1 = MUX_A(CM_FXCOMCLKSEL1, 0),
    kPLL0_DIV_to_FLEXCOMM1 = MUX_A(CM_FXCOMCLKSEL1, 1),
    kFRO12M_to_FLEXCOMM1 = MUX_A(CM_FXCOMCLKSEL1, 2),
    kFRO_HF_DIV_to_FLEXCOMM1 = MUX_A(CM_FXCOMCLKSEL1, 3),
    kFRO1M_to_FLEXCOMM1 = MUX_A(CM_FXCOMCLKSEL1, 4),
    kMCLK_to_FLEXCOMM1 = MUX_A(CM_FXCOMCLKSEL1, 5),
    kOSC32K_to_FLEXCOMM1 = MUX_A(CM_FXCOMCLKSEL1, 6),
    kNONE_to_FLEXCOMM1 = MUX_A(CM_FXCOMCLKSEL1, 7),

    kMAIN_CLK_to_FLEXCOMM2 = MUX_A(CM_FXCOMCLKSEL2, 0),
    kPLL0_DIV_to_FLEXCOMM2 = MUX_A(CM_FXCOMCLKSEL2, 1),
    kFRO12M_to_FLEXCOMM2 = MUX_A(CM_FXCOMCLKSEL2, 2),
    kFRO_HF_DIV_to_FLEXCOMM2 = MUX_A(CM_FXCOMCLKSEL2, 3),
    kFRO1M_to_FLEXCOMM2 = MUX_A(CM_FXCOMCLKSEL2, 4),
    kMCLK_to_FLEXCOMM2 = MUX_A(CM_FXCOMCLKSEL2, 5),
    kOSC32K_to_FLEXCOMM2 = MUX_A(CM_FXCOMCLKSEL2, 6),
    kNONE_to_FLEXCOMM2 = MUX_A(CM_FXCOMCLKSEL2, 7),

    kMAIN_CLK_to_FLEXCOMM3 = MUX_A(CM_FXCOMCLKSEL3, 0),
    kPLL0_DIV_to_FLEXCOMM3 = MUX_A(CM_FXCOMCLKSEL3, 1),
    kFRO12M_to_FLEXCOMM3 = MUX_A(CM_FXCOMCLKSEL3, 2),
    kFRO_HF_DIV_to_FLEXCOMM3 = MUX_A(CM_FXCOMCLKSEL3, 3),
    kFRO1M_to_FLEXCOMM3 = MUX_A(CM_FXCOMCLKSEL3, 4),
    kMCLK_to_FLEXCOMM3 = MUX_A(CM_FXCOMCLKSEL3, 5),
    kOSC32K_to_FLEXCOMM3 = MUX_A(CM_FXCOMCLKSEL3, 6),
    kNONE_to_FLEXCOMM3 = MUX_A(CM_FXCOMCLKSEL3, 7),

    kMAIN_CLK_to_FLEXCOMM4 = MUX_A(CM_FXCOMCLKSEL4, 0),
    kPLL0_DIV_to_FLEXCOMM4 = MUX_A(CM_FXCOMCLKSEL4, 1),
    kFRO12M_to_FLEXCOMM4 = MUX_A(CM_FXCOMCLKSEL4, 2),
    kFRO_HF_DIV_to_FLEXCOMM4 = MUX_A(CM_FXCOMCLKSEL4, 3),
    kFRO1M_to_FLEXCOMM4 = MUX_A(CM_FXCOMCLKSEL4, 4),
    kMCLK_to_FLEXCOMM4 = MUX_A(CM_FXCOMCLKSEL4, 5),
    kOSC32K_to_FLEXCOMM4 = MUX_A(CM_FXCOMCLKSEL4, 6),
    kNONE_to_FLEXCOMM4 = MUX_A(CM_FXCOMCLKSEL4, 7),

    kMAIN_CLK_to_FLEXCOMM5 = MUX_A(CM_FXCOMCLKSEL5, 0),
    kPLL0_DIV_to_FLEXCOMM5 = MUX_A(CM_FXCOMCLKSEL5, 1),
    kFRO12M_to_FLEXCOMM5 = MUX_A(CM_FXCOMCLKSEL5, 2),
    kFRO_HF_DIV_to_FLEXCOMM5 = MUX_A(CM_FXCOMCLKSEL5, 3),
    kFRO1M_to_FLEXCOMM5 = MUX_A(CM_FXCOMCLKSEL5, 4),
    kMCLK_to_FLEXCOMM5 = MUX_A(CM_FXCOMCLKSEL5, 5),
    kOSC32K_to_FLEXCOMM5 = MUX_A(CM_FXCOMCLKSEL5, 6),
    kNONE_to_FLEXCOMM5 = MUX_A(CM_FXCOMCLKSEL5, 7),

    kMAIN_CLK_to_FLEXCOMM6 = MUX_A(CM_FXCOMCLKSEL6, 0),
    kPLL0_DIV_to_FLEXCOMM6 = MUX_A(CM_FXCOMCLKSEL6, 1),
    kFRO12M_to_FLEXCOMM6 = MUX_A(CM_FXCOMCLKSEL6, 2),
    kFRO_HF_DIV_to_FLEXCOMM6 = MUX_A(CM_FXCOMCLKSEL6, 3),
    kFRO1M_to_FLEXCOMM6 = MUX_A(CM_FXCOMCLKSEL6, 4),
    kMCLK_to_FLEXCOMM6 = MUX_A(CM_FXCOMCLKSEL6, 5),
    kOSC32K_to_FLEXCOMM6 = MUX_A(CM_FXCOMCLKSEL6, 6),
    kNONE_to_FLEXCOMM6 = MUX_A(CM_FXCOMCLKSEL6, 7),

    kMAIN_CLK_to_FLEXCOMM7 = MUX_A(CM_FXCOMCLKSEL7, 0),
    kPLL0_DIV_to_FLEXCOMM7 = MUX_A(CM_FXCOMCLKSEL7, 1),
    kFRO12M_to_FLEXCOMM7 = MUX_A(CM_FXCOMCLKSEL7, 2),
    kFRO_HF_DIV_to_FLEXCOMM7 = MUX_A(CM_FXCOMCLKSEL7, 3),
    kFRO1M_to_FLEXCOMM7 = MUX_A(CM_FXCOMCLKSEL7, 4),
    kMCLK_to_FLEXCOMM7 = MUX_A(CM_FXCOMCLKSEL7, 5),
    kOSC32K_to_FLEXCOMM7 = MUX_A(CM_FXCOMCLKSEL7, 6),
    kNONE_to_FLEXCOMM7 = MUX_A(CM_FXCOMCLKSEL7, 7),

    kMAIN_CLK_to_HSLSPI = MUX_A(CM_HSLSPICLKSEL, 0),
    kPLL0_DIV_to_HSLSPI = MUX_A(CM_HSLSPICLKSEL, 1),
    kFRO12M_to_HSLSPI = MUX_A(CM_HSLSPICLKSEL, 2),
    kFRO_HF_DIV_to_HSLSPI = MUX_A(CM_HSLSPICLKSEL, 3),
    kFRO1M_to_HSLSPI = MUX_A(CM_HSLSPICLKSEL, 4),
    kOSC32K_to_HSLSPI = MUX_A(CM_HSLSPICLKSEL, 6),
    kNONE_to_HSLSPI = MUX_A(CM_HSLSPICLKSEL, 7),

    kFRO_HF_to_MCLK = MUX_A(CM_MCLKCLKSEL, 0),
    kPLL0_to_MCLK = MUX_A(CM_MCLKCLKSEL, 1),
    kFRO1M_to_MCLK = MUX_A(CM_MCLKCLKSEL, 2),    /* Need confirm */
    kMAIN_CLK_to_MCLK = MUX_A(CM_MCLKCLKSEL, 3), /* Need confirm */
    kNONE_to_MCLK = MUX_A(CM_MCLKCLKSEL, 7),

    kMAIN_CLK_to_SCT_CLK = MUX_A(CM_SCTCLKSEL, 0),
    kPLL0_to_SCT_CLK = MUX_A(CM_SCTCLKSEL, 1),
    kEXT_CLK_to_SCT_CLK = MUX_A(CM_SCTCLKSEL, 2),
    kFRO_HF_to_SCT_CLK = MUX_A(CM_SCTCLKSEL, 3),
    kMCLK_to_SCT_CLK = MUX_A(CM_SCTCLKSEL, 5),
    kNONE_to_SCT_CLK = MUX_A(CM_SCTCLKSEL, 7),

    kMAIN_CLK_to_SDIO_CLK = MUX_A(CM_SDIOCLKSEL, 0),
    kPLL0_to_SDIO_CLK = MUX_A(CM_SDIOCLKSEL, 1),
    kFRO_HF_to_SDIO_CLK = MUX_A(CM_SDIOCLKSEL, 3),
    kPLL1_to_SDIO_CLK = MUX_A(CM_SDIOCLKSEL, 5),
    kNONE_to_SDIO_CLK = MUX_A(CM_SDIOCLKSEL, 7),

    kFRO32K_to_OSC32K = MUX_A(CM_RTCOSC32KCLKSEL, 0),
    kXTAL32K_to_OSC32K = MUX_A(CM_RTCOSC32KCLKSEL, 1),

    kTRACE_DIV_to_TRACE = MUX_A(CM_TRACECLKSEL, 0),
    kFRO1M_to_TRACE = MUX_A(CM_TRACECLKSEL, 1),
    kOSC32K_to_TRACE = MUX_A(CM_TRACECLKSEL, 2),
    kNONE_to_TRACE = MUX_A(CM_TRACECLKSEL, 7),

    kSYSTICK_DIV0_to_SYSTICK0 = MUX_A(CM_SYSTICKCLKSEL0, 0),
    kFRO1M_to_SYSTICK0 = MUX_A(CM_SYSTICKCLKSEL0, 1),
    kOSC32K_to_SYSTICK0 = MUX_A(CM_SYSTICKCLKSEL0, 2),
    kNONE_to_SYSTICK0 = MUX_A(CM_SYSTICKCLKSEL0, 7),

    kSYSTICK_DIV1_to_SYSTICK1 = MUX_A(CM_SYSTICKCLKSEL1, 0),
    kFRO1M_to_SYSTICK1 = MUX_A(CM_SYSTICKCLKSEL1, 1),
    kOSC32K_to_SYSTICK1 = MUX_A(CM_SYSTICKCLKSEL1, 2),
    kNONE_to_SYSTICK1 = MUX_A(CM_SYSTICKCLKSEL1, 7),

    kFRO12M_to_PLL1 = MUX_A(CM_PLL1CLKSEL, 0),
    kEXT_CLK_to_PLL1 = MUX_A(CM_PLL1CLKSEL, 1),
    kFRO1M_to_PLL1 = MUX_A(CM_PLL1CLKSEL, 2),
    kOSC32K_to_PLL1 = MUX_A(CM_PLL1CLKSEL, 3),
    kNONE_to_PLL1 = MUX_A(CM_PLL1CLKSEL, 7),

    kMAIN_CLK_to_CTIMER0 = MUX_A(CM_CTIMERCLKSEL0, 0),
    kPLL0_to_CTIMER0 = MUX_A(CM_CTIMERCLKSEL0, 1),
    kFRO_HF_to_CTIMER0 = MUX_A(CM_CTIMERCLKSEL0, 3),
    kFRO1M_to_CTIMER0 = MUX_A(CM_CTIMERCLKSEL0, 4),
    kMCLK_to_CTIMER0 = MUX_A(CM_CTIMERCLKSEL0, 5),
    kOSC32K_to_CTIMER0 = MUX_A(CM_CTIMERCLKSEL0, 6),
    kNONE_to_CTIMER0 = MUX_A(CM_CTIMERCLKSEL0, 7),

    kMAIN_CLK_to_CTIMER1 = MUX_A(CM_CTIMERCLKSEL1, 0),
    kPLL0_to_CTIMER1 = MUX_A(CM_CTIMERCLKSEL1, 1),
    kFRO_HF_to_CTIMER1 = MUX_A(CM_CTIMERCLKSEL1, 3),
    kFRO1M_to_CTIMER1 = MUX_A(CM_CTIMERCLKSEL1, 4),
    kMCLK_to_CTIMER1 = MUX_A(CM_CTIMERCLKSEL1, 5),
    kOSC32K_to_CTIMER1 = MUX_A(CM_CTIMERCLKSEL1, 6),
    kNONE_to_CTIMER1 = MUX_A(CM_CTIMERCLKSEL1, 7),

    kMAIN_CLK_to_CTIMER2 = MUX_A(CM_CTIMERCLKSEL2, 0),
    kPLL0_to_CTIMER2 = MUX_A(CM_CTIMERCLKSEL2, 1),
    kFRO_HF_to_CTIMER2 = MUX_A(CM_CTIMERCLKSEL2, 3),
    kFRO1M_to_CTIMER2 = MUX_A(CM_CTIMERCLKSEL2, 4),
    kMCLK_to_CTIMER2 = MUX_A(CM_CTIMERCLKSEL2, 5),
    kOSC32K_to_CTIMER2 = MUX_A(CM_CTIMERCLKSEL2, 6),
    kNONE_to_CTIMER2 = MUX_A(CM_CTIMERCLKSEL2, 7),

    kMAIN_CLK_to_CTIMER3 = MUX_A(CM_CTIMERCLKSEL3, 0),
    kPLL0_to_CTIMER3 = MUX_A(CM_CTIMERCLKSEL3, 1),
    kFRO_HF_to_CTIMER3 = MUX_A(CM_CTIMERCLKSEL3, 3),
    kFRO1M_to_CTIMER3 = MUX_A(CM_CTIMERCLKSEL3, 4),
    kMCLK_to_CTIMER3 = MUX_A(CM_CTIMERCLKSEL3, 5),
    kOSC32K_to_CTIMER3 = MUX_A(CM_CTIMERCLKSEL3, 6),
    kNONE_to_CTIMER3 = MUX_A(CM_CTIMERCLKSEL3, 7),

    kMAIN_CLK_to_CTIMER4 = MUX_A(CM_CTIMERCLKSEL4, 0),
    kPLL0_to_CTIMER4 = MUX_A(CM_CTIMERCLKSEL4, 1),
    kFRO_HF_to_CTIMER4 = MUX_A(CM_CTIMERCLKSEL4, 3),
    kFRO1M_to_CTIMER4 = MUX_A(CM_CTIMERCLKSEL4, 4),
    kMCLK_to_CTIMER4 = MUX_A(CM_CTIMERCLKSEL4, 5),
    kOSC32K_to_CTIMER4 = MUX_A(CM_CTIMERCLKSEL4, 6),
    kNONE_to_CTIMER4 = MUX_A(CM_CTIMERCLKSEL4, 7),
    kNONE_to_NONE = (int)0x80000000U,
} clock_attach_id_t;

/*  Clock dividers */
typedef enum _clock_div_name
{
    kCLOCK_DivSystickClk0 = 0,
    kCLOCK_DivSystickClk1 = 1,
    kCLOCK_DivArmTrClkDiv = 2,
    kCLOCK_DivFlexFrg0 = 8,
    kCLOCK_DivFlexFrg1 = 9,
    kCLOCK_DivFlexFrg2 = 10,
    kCLOCK_DivFlexFrg3 = 11,
    kCLOCK_DivFlexFrg4 = 12,
    kCLOCK_DivFlexFrg5 = 13,
    kCLOCK_DivFlexFrg6 = 14,
    kCLOCK_DivFlexFrg7 = 15,
    kCLOCK_DivAhbClk = 32,
    kCLOCK_DivClkOut = 33,
    kCLOCK_DivFrohfClk = 34,
    kCLOCK_DivWdtClk = 35,
    kCLOCK_DivAdcAsyncClk = 37,
    kCLOCK_DivUsb0Clk = 38,
    kCLOCK_DivMClk = 43,
    kCLOCK_DivSctClk = 45,
    kCLOCK_DivSdioClk = 47,
    kCLOCK_DivPll0Clk = 49
} clock_div_name_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Enable the clock for specific IP.
 * @param name : Clock to be enabled.
 * @return  Nothing
 */
static inline void CLOCK_EnableClock(clock_ip_name_t clk)
{
    uint32_t index = CLK_GATE_ABSTRACT_REG_OFFSET(clk);
    SYSCON->AHBCLKCTRLSET[index] = (1U << CLK_GATE_ABSTRACT_BITS_SHIFT(clk));
}
/**
 * @brief Disable the clock for specific IP.
 * @param name : Clock to be Disabled.
 * @return  Nothing
 */
static inline void CLOCK_DisableClock(clock_ip_name_t clk)
{
    uint32_t index = CLK_GATE_ABSTRACT_REG_OFFSET(clk);
    SYSCON->AHBCLKCTRLCLR[index] = (1U << CLK_GATE_ABSTRACT_BITS_SHIFT(clk));
}
/**
 * @brief   Initialize the Core clock to given frequency (12, 48 or 96 MHz).
 * Turns on FRO and uses default CCO, if freq is 12000000, then high speed output is off, else high speed output is
 * enabled.
 * @param   iFreq   : Desired frequency (must be one of #CLK_FRO_12MHZ or #CLK_FRO_48MHZ or #CLK_FRO_96MHZ)
 * @return  returns success or fail status.
 */
status_t CLOCK_SetupFROClocking(uint32_t iFreq);
/**
 * @brief	Set the flash wait states for the input freuqency.
 * @param	iFreq	: Input frequency
 * @return	Nothing
 */
void CLOCK_SetFLASHAccessCyclesForFreq(uint32_t iFreq);
/**
 * @brief   Initialize the external osc clock to given frequency.
 * @param   iFreq   : Desired frequency (must be equal to exact rate in Hz)
 * @return  returns success or fail status.
 */
status_t CLOCK_SetupExtClocking(uint32_t iFreq);
/**
 * @brief   Initialize the I2S MCLK clock to given frequency.
 * @param   iFreq   : Desired frequency (must be equal to exact rate in Hz)
 * @return  returns success or fail status.
 */
status_t CLOCK_SetupI2SMClkClocking(uint32_t iFreq);
/**
 * @brief   Configure the clock selection muxes.
 * @param   connection  : Clock to be configured.
 * @return  Nothing
 */
void CLOCK_AttachClk(clock_attach_id_t connection);
/**
 * @brief   Get the actual clock attach id.
 * This fuction uses the offset in input attach id, then it reads the actual source value in
 * the register and combine the offset to obtain an actual attach id.
 * @param   attachId  : Clock attach id to get.
 * @return  Clock source value.
 */
clock_attach_id_t CLOCK_GetClockAttachId(clock_attach_id_t attachId);
/**
 * @brief   Setup peripheral clock dividers.
 * @param   div_name    : Clock divider name
 * @param divided_by_value: Value to be divided
 * @param reset :  Whether to reset the divider counter.
 * @return  Nothing
 */
void CLOCK_SetClkDiv(clock_div_name_t div_name, uint32_t divided_by_value, bool reset);
/**
 * @brief   Setup rtc 1khz clock divider.
 * @param divided_by_value: Value to be divided
 * @return  Nothing
 */
void CLOCK_SetRtc1khzClkDiv(uint32_t divided_by_value);
/**
 * @brief   Setup rtc 1hz clock divider.
 * @param divided_by_value: Value to be divided
 * @return  Nothing
 */
void CLOCK_SetRtc1hzClkDiv(uint32_t divided_by_value);

/**
 * @brief   Set the flexcomm output frequency.
 * @param   id      : flexcomm instance id
 *          freq    : output frequency
 * @return  0   : the frequency range is out of range.
 *          1   : switch successfully.
 */
uint32_t CLOCK_SetFlexCommClock(uint32_t id, uint32_t freq);

/*! @brief  Return Frequency of flexcomm input clock
 *  @param  id     : flexcomm instance id
 *  @return Frequency value
 */
uint32_t CLOCK_GetFlexCommInputClock(uint32_t id);

/*! @brief  Return Frequency of selected clock
 *  @return Frequency of selected clock
 */
uint32_t CLOCK_GetFreq(clock_name_t clockName);
/*! @brief  Return Frequency of FRO 12MHz
 *  @return Frequency of FRO 12MHz
 */
uint32_t CLOCK_GetFro12MFreq(void);
/*! @brief  Return Frequency of FRO 1MHz
 *  @return Frequency of FRO 1MHz
 */
uint32_t CLOCK_GetFro1MFreq(void);
/*! @brief  Return Frequency of ClockOut
 *  @return Frequency of ClockOut
 */
uint32_t CLOCK_GetClockOutClkFreq(void);
/*! @brief  Return Frequency of Adc Clock
 *  @return Frequency of Adc.
 */
uint32_t CLOCK_GetAdcClkFreq(void);
/*! @brief  Return Frequency of Usb0 Clock
 *  @return Frequency of Usb0 Clock.
 */
uint32_t CLOCK_GetUsb0ClkFreq(void);
/*! @brief  Return Frequency of Usb1 Clock
 *  @return Frequency of Usb1 Clock.
 */
uint32_t CLOCK_GetUsb1ClkFreq(void);
/*! @brief  Return Frequency of MClk Clock
 *  @return Frequency of MClk Clock.
 */
uint32_t CLOCK_GetMclkClkFreq(void);
/*! @brief  Return Frequency of SCTimer Clock
 *  @return Frequency of SCTimer Clock.
 */
uint32_t CLOCK_GetSctClkFreq(void);
/*! @brief  Return Frequency of SDIO Clock
 *  @return Frequency of SDIO Clock.
 */
uint32_t CLOCK_GetSdioClkFreq(void);
/*! @brief  Return Frequency of External Clock
 *  @return Frequency of External Clock. If no external clock is used returns 0.
 */
uint32_t CLOCK_GetExtClkFreq(void);
/*! @brief  Return Frequency of Watchdog
 *  @return Frequency of Watchdog
 */
uint32_t CLOCK_GetWdtClkFreq(void);
/*! @brief  Return Frequency of High-Freq output of FRO
 *  @return Frequency of High-Freq output of FRO
 */
uint32_t CLOCK_GetFroHfFreq(void);
/*! @brief  Return Frequency of PLL
 *  @return Frequency of PLL
 */
uint32_t CLOCK_GetPll0OutFreq(void);
/*! @brief  Return Frequency of USB PLL
 *  @return Frequency of PLL
 */
uint32_t CLOCK_GetPll1OutFreq(void);
/*! @brief  Return Frequency of 32kHz osc
 *  @return Frequency of 32kHz osc
 */
uint32_t CLOCK_GetOsc32KFreq(void);
/*! @brief  Return Frequency of Core System
 *  @return Frequency of Core System
 */
uint32_t CLOCK_GetCoreSysClkFreq(void);
/*! @brief  Return Frequency of I2S MCLK Clock
 *  @return Frequency of I2S MCLK Clock
 */
uint32_t CLOCK_GetI2SMClkFreq(void);
/*! @brief  Return Frequency of CTimer functional Clock
 *  @return Frequency of CTimer functional Clock
 */
uint32_t CLOCK_GetCTimerClkFreq(uint32_t id);
/*! @brief  Return Frequency of SystickClock
 *  @return Frequency of Systick Clock
 */
uint32_t CLOCK_GetSystickClkFreq(uint32_t id);

/*! @brief    Return  PLL0 input clock rate
*  @return    PLL0 input clock rate
*/
uint32_t CLOCK_GetPLL0InClockRate(void);

/*! @brief    Return  PLL1 input clock rate
*  @return    PLL1 input clock rate
*/
uint32_t CLOCK_GetPLL1InClockRate(void);

/*! @brief    Return  PLL0 output clock rate
*  @param    recompute   : Forces a PLL rate recomputation if true
*  @return    PLL0 output clock rate
*  @note The PLL rate is cached in the driver in a variable as
*  the rate computation function can take some time to perform. It
*  is recommended to use 'false' with the 'recompute' parameter.
*/
uint32_t CLOCK_GetPLL0OutClockRate(bool recompute);

/*! @brief    Return  PLL1 output clock rate
*  @param    recompute   : Forces a PLL rate recomputation if true
*  @return    PLL1 output clock rate
*  @note The PLL rate is cached in the driver in a variable as
*  the rate computation function can take some time to perform. It
*  is recommended to use 'false' with the 'recompute' parameter.
*/
uint32_t CLOCK_GetPLL1OutClockRate(bool recompute);

/*! @brief    Enables and disables PLL0 bypass mode
*  @brief    bypass  : true to bypass PLL0 (PLL0 output = PLL0 input, false to disable bypass
*  @return   PLL0 output clock rate
*/
__STATIC_INLINE void CLOCK_SetBypassPLL0(bool bypass)
{
    if (bypass)
    {
        SYSCON->PLL0CTRL |= (1UL << SYSCON_PLL0CTRL_BYPASSPLL_SHIFT);
    }
    else
    {
        SYSCON->PLL0CTRL &= ~(1UL << SYSCON_PLL0CTRL_BYPASSPLL_SHIFT);
    }
}

/*! @brief    Enables and disables PLL1 bypass mode
*  @brief    bypass  : true to bypass PLL1 (PLL1 output = PLL1 input, false to disable bypass
*  @return   PLL1 output clock rate
*/
__STATIC_INLINE void CLOCK_SetBypassPLL1(bool bypass)
{
    if (bypass)
    {
        SYSCON->PLL1CTRL |= (1UL << SYSCON_PLL1CTRL_BYPASSPLL_SHIFT);
    }
    else
    {
        SYSCON->PLL1CTRL &= ~(1UL << SYSCON_PLL1CTRL_BYPASSPLL_SHIFT);
    }
}

/*! @brief    Check if PLL is locked or not
*  @return   true if the PLL is locked, false if not locked
*/
__STATIC_INLINE bool CLOCK_IsPLL0Locked(void)
{
    return (bool)((SYSCON->PLL0STAT & SYSCON_PLL0STAT_LOCK_MASK) != 0);
}

/*! @brief	Check if PLL1 is locked or not
 *  @return	true if the PLL1 is locked, false if not locked
 */
__STATIC_INLINE bool CLOCK_IsPLL1Locked(void)
{
    return (bool)((SYSCON->PLL1STAT & SYSCON_PLL1STAT_LOCK_MASK) != 0);
}

/*! @brief Store the current PLL0 rate
*  @param    rate: Current rate of the PLL0
*  @return   Nothing
**/
void CLOCK_SetStoredPLL0ClockRate(uint32_t rate);

/*! @brief PLL configuration structure flags for 'flags' field
* These flags control how the PLL configuration function sets up the PLL setup structure.<br>
*
* When the PLL_CONFIGFLAG_USEINRATE flag is selected, the 'InputRate' field in the
* configuration structure must be assigned with the expected PLL frequency. If the
* PLL_CONFIGFLAG_USEINRATE is not used, 'InputRate' is ignored in the configuration
* function and the driver will determine the PLL rate from the currently selected
* PLL source. This flag might be used to configure the PLL input clock more accurately
* when using the WDT oscillator or a more dyanmic CLKIN source.<br>
*
* When the PLL_CONFIGFLAG_FORCENOFRACT flag is selected, the PLL hardware for the
* automatic bandwidth selection, Spread Spectrum (SS) support, and fractional M-divider
* are not used.<br>
*/
#define PLL_CONFIGFLAG_USEINRATE (1 << 0) /*!< Flag to use InputRate in PLL configuration structure for setup */
#define PLL_CONFIGFLAG_FORCENOFRACT (1 << 2)
/*!< Force non-fractional output mode, PLL output will not use the fractional, automatic bandwidth, or SS hardware */

/*! @brief PLL Spread Spectrum (SS) Programmable modulation frequency
 * See (MF) field in the PLL0SSCG1 register in the UM.
 */
typedef enum _ss_progmodfm
{
    kSS_MF_512 = (0 << 20), /*!< Nss = 512 (fm ? 3.9 - 7.8 kHz) */
    kSS_MF_384 = (1 << 20), /*!< Nss ?= 384 (fm ? 5.2 - 10.4 kHz) */
    kSS_MF_256 = (2 << 20), /*!< Nss = 256 (fm ? 7.8 - 15.6 kHz) */
    kSS_MF_128 = (3 << 20), /*!< Nss = 128 (fm ? 15.6 - 31.3 kHz) */
    kSS_MF_64 = (4 << 20),  /*!< Nss = 64 (fm ? 32.3 - 64.5 kHz) */
    kSS_MF_32 = (5 << 20),  /*!< Nss = 32 (fm ? 62.5- 125 kHz) */
    kSS_MF_24 = (6 << 20),  /*!< Nss ?= 24 (fm ? 83.3- 166.6 kHz) */
    kSS_MF_16 = (7 << 20)   /*!< Nss = 16 (fm ? 125- 250 kHz) */
} ss_progmodfm_t;

/*! @brief PLL Spread Spectrum (SS) Programmable frequency modulation depth
 * See (MR) field in the PLL0SSCG1 register in the UM.
 */
typedef enum _ss_progmoddp
{
    kSS_MR_K0 = (0 << 23),   /*!< k = 0 (no spread spectrum) */
    kSS_MR_K1 = (1 << 23),   /*!< k = 1 */
    kSS_MR_K1_5 = (2 << 23), /*!< k = 1.5 */
    kSS_MR_K2 = (3 << 23),   /*!< k = 2 */
    kSS_MR_K3 = (4 << 23),   /*!< k = 3 */
    kSS_MR_K4 = (5 << 23),   /*!< k = 4 */
    kSS_MR_K6 = (6 << 23),   /*!< k = 6 */
    kSS_MR_K8 = (7 << 23)    /*!< k = 8 */
} ss_progmoddp_t;

/*! @brief PLL Spread Spectrum (SS) Modulation waveform control
 * See (MC) field in the PLL0SSCG1 register in the UM.<br>
 * Compensation for low pass filtering of the PLL to get a triangular
 * modulation at the output of the PLL, giving a flat frequency spectrum.
 */
typedef enum _ss_modwvctrl
{
    kSS_MC_NOC = (0 << 26),  /*!< no compensation */
    kSS_MC_RECC = (2 << 26), /*!< recommended setting */
    kSS_MC_MAXC = (3 << 26), /*!< max. compensation */
} ss_modwvctrl_t;

/*! @brief PLL configuration structure
*
* This structure can be used to configure the settings for a PLL
* setup structure. Fill in the desired configuration for the PLL
* and call the PLL setup function to fill in a PLL setup structure.
*/
typedef struct _pll_config
{
    uint32_t desiredRate; /*!< Desired PLL rate in Hz */
    uint32_t inputRate;   /*!< PLL input clock in Hz, only used if PLL_CONFIGFLAG_USEINRATE flag is set */
    uint32_t flags;       /*!< PLL configuration flags, Or'ed value of PLL_CONFIGFLAG_* definitions */
    ss_progmodfm_t ss_mf; /*!< SS Programmable modulation frequency, only applicable when not using
                             PLL_CONFIGFLAG_FORCENOFRACT flag */
    ss_progmoddp_t ss_mr; /*!< SS Programmable frequency modulation depth, only applicable when not using
                             PLL_CONFIGFLAG_FORCENOFRACT flag */
    ss_modwvctrl_t
        ss_mc; /*!< SS Modulation waveform control, only applicable when not using PLL_CONFIGFLAG_FORCENOFRACT flag */
    bool mfDither; /*!< false for fixed modulation frequency or true for dithering, only applicable when not using
                      PLL_CONFIGFLAG_FORCENOFRACT flag */

} pll_config_t;

/*! @brief PLL setup structure flags for 'flags' field
* These flags control how the PLL setup function sets up the PLL
*/
#define PLL_SETUPFLAG_POWERUP (1 << 0)         /*!< Setup will power on the PLL after setup */
#define PLL_SETUPFLAG_WAITLOCK (1 << 1)        /*!< Setup will wait for PLL lock, implies the PLL will be pwoered on */
#define PLL_SETUPFLAG_ADGVOLT (1 << 2)         /*!< Optimize system voltage for the new PLL rate */
#define PLL_SETUPFLAG_USEFEEDBACKDIV2 (1 << 3) /*!< Use feedback divider by 2 in divider path */

/*! @brief PLL0 setup structure
* This structure can be used to pre-build a PLL setup configuration
* at run-time and quickly set the PLL to the configuration. It can be
* populated with the PLL setup function. If powering up or waiting
* for PLL lock, the PLL input clock source should be configured prior
* to PLL setup.
*/
typedef struct _pll_setup
{
    uint32_t pllctrl;    /*!< PLL control register PLL0CTRL */
    uint32_t pllndec;    /*!< PLL NDEC register PLL0NDEC */
    uint32_t pllpdec;    /*!< PLL PDEC register PLL0PDEC */
    uint32_t pllmdec;    /*!< PLL MDEC registers PLL0PDEC */
    uint32_t pllsscg[2]; /*!< PLL SSCTL registers PLL0SSCG*/
    uint32_t pllRate;    /*!< Acutal PLL rate */
    uint32_t flags;      /*!< PLL setup flags, Or'ed value of PLL_SETUPFLAG_* definitions */
} pll_setup_t;

/*! @brief PLL status definitions
*/
typedef enum _pll_error
{
    kStatus_PLL_Success = MAKE_STATUS(kStatusGroup_Generic, 0),         /*!< PLL operation was successful */
    kStatus_PLL_OutputTooLow = MAKE_STATUS(kStatusGroup_Generic, 1),    /*!< PLL output rate request was too low */
    kStatus_PLL_OutputTooHigh = MAKE_STATUS(kStatusGroup_Generic, 2),   /*!< PLL output rate request was too high */
    kStatus_PLL_InputTooLow = MAKE_STATUS(kStatusGroup_Generic, 3),     /*!< PLL input rate is too low */
    kStatus_PLL_InputTooHigh = MAKE_STATUS(kStatusGroup_Generic, 4),    /*!< PLL input rate is too high */
    kStatus_PLL_OutsideIntLimit = MAKE_STATUS(kStatusGroup_Generic, 5), /*!< Requested output rate isn't possible */
    kStatus_PLL_CCOTooLow = MAKE_STATUS(kStatusGroup_Generic, 6),       /*!< Requested CCO rate isn't possible */
    kStatus_PLL_CCOTooHigh = MAKE_STATUS(kStatusGroup_Generic, 7)       /*!< Requested CCO rate isn't possible */
} pll_error_t;

/*! @brief USB FS clock source definition. */
typedef enum _clock_usbfs_src
{
    kCLOCK_UsbfsSrcFro = (uint32_t)kCLOCK_FroHf,            /*!< Use FRO 96 MHz. */
    kCLOCK_UsbfsSrcPll0 = (uint32_t)kCLOCK_Pll0Out,         /*!< Use PLL0 output. */
    kCLOCK_UsbfsSrcMainClock = (uint32_t)kCLOCK_CoreSysClk, /*!< Use Main clock.    */
    kCLOCK_UsbfsSrcPll1 = (uint32_t)kCLOCK_Pll1Out,         /*!< Use PLL1 clock.    */

    kCLOCK_UsbfsSrcNone =
        SYSCON_USB0CLKSEL_SEL(7) /*!<this may be selected in order to reduce power when no output is needed. */
} clock_usbfs_src_t;

/*! @brief USBhs clock source definition. */
typedef enum _clock_usbhs_src
{
    kCLOCK_UsbSrcUnused = 0xFFFFFFFFU, /*!< Used when the function does not
                                            care the clock source. */
} clock_usbhs_src_t;

/*! @brief Source of the USB HS PHY. */
typedef enum _clock_usb_phy_src
{
    kCLOCK_UsbPhySrcExt = 0U, /*!< Use external crystal. */
} clock_usb_phy_src_t;

/*! @brief    Return PLL0 output clock rate from setup structure
*  @param    pSetup : Pointer to a PLL setup structure
*  @return   System PLL output clock rate the setup structure will generate
*/
uint32_t CLOCK_GetPLL0OutFromSetup(pll_setup_t *pSetup);

/*! @brief    Set PLL0 output based on the passed PLL setup data
*  @param    pControl    : Pointer to populated PLL control structure to generate setup with
*  @param    pSetup      : Pointer to PLL setup structure to be filled
*  @return   PLL_ERROR_SUCCESS on success, or PLL setup error code
*  @note Actual frequency for setup may vary from the desired frequency based on the
*  accuracy of input clocks, rounding, non-fractional PLL mode, etc.
*/
pll_error_t CLOCK_SetupPLL0Data(pll_config_t *pControl, pll_setup_t *pSetup);

/*! @brief    Set PLL output from PLL setup structure (precise frequency)
* @param pSetup  : Pointer to populated PLL setup structure
* @param flagcfg : Flag configuration for PLL config structure
* @return    PLL_ERROR_SUCCESS on success, or PLL setup error code
* @note  This function will power off the PLL, setup the PLL with the
* new setup data, and then optionally powerup the PLL, wait for PLL lock,
* and adjust system voltages to the new PLL rate. The function will not
* alter any source clocks (ie, main systen clock) that may use the PLL,
* so these should be setup prior to and after exiting the function.
*/
pll_error_t CLOCK_SetupPLL0Prec(pll_setup_t *pSetup, uint32_t flagcfg);

/**
* @brief Set PLL output from PLL setup structure (precise frequency)
* @param pSetup  : Pointer to populated PLL setup structure
* @return    kStatus_PLL_Success on success, or PLL setup error code
* @note  This function will power off the PLL, setup the PLL with the
* new setup data, and then optionally powerup the PLL, wait for PLL lock,
* and adjust system voltages to the new PLL rate. The function will not
* alter any source clocks (ie, main systen clock) that may use the PLL,
* so these should be setup prior to and after exiting the function.
*/
pll_error_t CLOCK_SetPLL0Freq(const pll_setup_t *pSetup);

/**
* @brief Set PLL output from PLL setup structure (precise frequency)
* @param pSetup  : Pointer to populated PLL setup structure
* @return    kStatus_PLL_Success on success, or PLL setup error code
* @note  This function will power off the PLL, setup the PLL with the
* new setup data, and then optionally powerup the PLL, wait for PLL lock,
* and adjust system voltages to the new PLL rate. The function will not
* alter any source clocks (ie, main systen clock) that may use the PLL,
* so these should be setup prior to and after exiting the function.
*/
pll_error_t CLOCK_SetPLL1Freq(const pll_setup_t *pSetup);

/*! @brief    Set PLL0 output based on the multiplier and input frequency
* @param multiply_by : multiplier
* @param input_freq  : Clock input frequency of the PLL
* @return    Nothing
* @note  Unlike the Chip_Clock_SetupSystemPLLPrec() function, this
* function does not disable or enable PLL power, wait for PLL lock,
* or adjust system voltages. These must be done in the application.
* The function will not alter any source clocks (ie, main systen clock)
* that may use the PLL, so these should be setup prior to and after
* exiting the function.
*/
void CLOCK_SetupPLL0Mult(uint32_t multiply_by, uint32_t input_freq);

/*! @brief Disable USB clock.
*
* Disable USB clock.
*/
static inline void CLOCK_DisableUsbDevicefs0Clock(clock_ip_name_t clk)
{
    CLOCK_DisableClock(clk);
}

/*! @brief Enable USB Device FS clock.
* @param src : clock source
* @param freq: clock frequency
* Enable USB Device Full Speed clock.
*/
bool CLOCK_EnableUsbfs0DeviceClock(clock_usbfs_src_t src, uint32_t freq);

/*! @brief Enable USB HOST FS clock.
* @param src : clock source
* @param freq: clock frequency
* Enable USB HOST Full Speed clock.
*/
bool CLOCK_EnableUsbfs0HostClock(clock_usbfs_src_t src, uint32_t freq);

/*! @brief Enable USB phy clock.
* Enable USB phy clock.
*/
bool CLOCK_EnableUsbhs0PhyPllClock(clock_usb_phy_src_t src, uint32_t freq);

/*! @brief Enable USB Device HS clock.
* Enable USB Device High Speed clock.
*/
bool CLOCK_EnableUsbhs0DeviceClock(clock_usbhs_src_t src, uint32_t freq);

/*! @brief Enable USB HOST HS clock.
* Enable USB HOST High Speed clock.
*/
bool CLOCK_EnableUsbhs0HostClock(clock_usbhs_src_t src, uint32_t freq);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @} */

#endif /* _FSL_CLOCK_H_ */
