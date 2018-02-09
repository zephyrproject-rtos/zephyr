/**************************************************************************//**
 * @file efm32hg350f64.h
 * @brief CMSIS Cortex-M Peripheral Access Layer Header File
 *        for EFM32HG350F64
 * @version 5.1.2
 ******************************************************************************
 * @section License
 * <b>Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com</b>
 ******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Laboratories, Inc.
 * has no obligation to support this Software. Silicon Laboratories, Inc. is
 * providing the Software "AS IS", with no express or implied warranties of any
 * kind, including, but not limited to, any implied warranties of
 * merchantability or fitness for any particular purpose or warranties against
 * infringement of any proprietary rights of a third party.
 *
 * Silicon Laboratories, Inc. will not be liable for any consequential,
 * incidental, or special damages, or any other relief, or for any claim by
 * any third party, arising from your use of this Software.
 *
 *****************************************************************************/

#ifndef EFM32HG350F64_H
#define EFM32HG350F64_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************//**
 * @addtogroup Parts
 * @{
 *****************************************************************************/

/**************************************************************************//**
 * @defgroup EFM32HG350F64 EFM32HG350F64
 * @{
 *****************************************************************************/

/** Interrupt Number Definition */
typedef enum IRQn
{
/******  Cortex-M0+ Processor Exceptions Numbers *****************************************/
  NonMaskableInt_IRQn = -14,                /*!< -14 Cortex-M0+ Non Maskable Interrupt   */
  HardFault_IRQn      = -13,                /*!< -13 Cortex-M0+ Hard Fault Interrupt     */
  SVCall_IRQn         = -5,                 /*!< -5  Cortex-M0+ SV Call Interrupt        */
  PendSV_IRQn         = -2,                 /*!< -2  Cortex-M0+ Pend SV Interrupt        */
  SysTick_IRQn        = -1,                 /*!< -1  Cortex-M0+ System Tick Interrupt    */

/******  EFM32HG Peripheral Interrupt Numbers ********************************************/
  DMA_IRQn            = 0,  /*!< 0 EFM32 DMA Interrupt */
  GPIO_EVEN_IRQn      = 1,  /*!< 1 EFM32 GPIO_EVEN Interrupt */
  TIMER0_IRQn         = 2,  /*!< 2 EFM32 TIMER0 Interrupt */
  ACMP0_IRQn          = 3,  /*!< 3 EFM32 ACMP0 Interrupt */
  ADC0_IRQn           = 4,  /*!< 4 EFM32 ADC0 Interrupt */
  I2C0_IRQn           = 5,  /*!< 5 EFM32 I2C0 Interrupt */
  GPIO_ODD_IRQn       = 6,  /*!< 6 EFM32 GPIO_ODD Interrupt */
  TIMER1_IRQn         = 7,  /*!< 7 EFM32 TIMER1 Interrupt */
  USART1_RX_IRQn      = 8,  /*!< 8 EFM32 USART1_RX Interrupt */
  USART1_TX_IRQn      = 9,  /*!< 9 EFM32 USART1_TX Interrupt */
  LEUART0_IRQn        = 10, /*!< 10 EFM32 LEUART0 Interrupt */
  PCNT0_IRQn          = 11, /*!< 11 EFM32 PCNT0 Interrupt */
  RTC_IRQn            = 12, /*!< 12 EFM32 RTC Interrupt */
  CMU_IRQn            = 13, /*!< 13 EFM32 CMU Interrupt */
  VCMP_IRQn           = 14, /*!< 14 EFM32 VCMP Interrupt */
  MSC_IRQn            = 15, /*!< 15 EFM32 MSC Interrupt */
  AES_IRQn            = 16, /*!< 16 EFM32 AES Interrupt */
  USART0_RX_IRQn      = 17, /*!< 17 EFM32 USART0_RX Interrupt */
  USART0_TX_IRQn      = 18, /*!< 18 EFM32 USART0_TX Interrupt */
  USB_IRQn            = 19, /*!< 19 EFM32 USB Interrupt */
  TIMER2_IRQn         = 20, /*!< 20 EFM32 TIMER2 Interrupt */
} IRQn_Type;

/**************************************************************************//**
 * @defgroup EFM32HG350F64_Core EFM32HG350F64 Core
 * @{
 * @brief Processor and Core Peripheral Section
 *****************************************************************************/
#define __MPU_PRESENT             0 /**< MPU not present */
#define __VTOR_PRESENT            1 /**< Presence of VTOR register in SCB */
#define __NVIC_PRIO_BITS          2 /**< NVIC interrupt priority bits */
#define __Vendor_SysTickConfig    0 /**< Is 1 if different SysTick counter is used */

/** @} End of group EFM32HG350F64_Core */

/**************************************************************************//**
* @defgroup EFM32HG350F64_Part EFM32HG350F64 Part
* @{
******************************************************************************/

/** Part family */
#define _EFM32_HAPPY_FAMILY                     1  /**< Happy Gecko EFM32HG MCU Family */
#define _EFM_DEVICE                                /**< Silicon Labs EFM-type microcontroller */
#define _SILICON_LABS_32B_SERIES_0                 /**< Silicon Labs series number */
#define _SILICON_LABS_32B_SERIES                0  /**< Silicon Labs series number */
#define _SILICON_LABS_GECKO_INTERNAL_SDID       77 /** Silicon Labs internal use only, may change any time */
#define _SILICON_LABS_GECKO_INTERNAL_SDID_77       /** Silicon Labs internal use only, may change any time */
#define _SILICON_LABS_32B_PLATFORM_1               /**< @deprecated Silicon Labs platform name */
#define _SILICON_LABS_32B_PLATFORM              1  /**< @deprecated Silicon Labs platform name */

/* If part number is not defined as compiler option, define it */
#if !defined(EFM32HG350F64)
#define EFM32HG350F64    1 /**< Happy Gecko Part  */
#endif

/** Configure part number */
#define PART_NUMBER          "EFM32HG350F64" /**< Part Number */

/** Memory Base addresses and limits */
#define FLASH_MEM_BASE       ((uint32_t) 0x0UL)        /**< FLASH base address  */
#define FLASH_MEM_SIZE       ((uint32_t) 0x10000000UL) /**< FLASH available address space  */
#define FLASH_MEM_END        ((uint32_t) 0xFFFFFFFUL)  /**< FLASH end address  */
#define FLASH_MEM_BITS       ((uint32_t) 0x28UL)       /**< FLASH used bits  */
#define AES_MEM_BASE         ((uint32_t) 0x400E0000UL) /**< AES base address  */
#define AES_MEM_SIZE         ((uint32_t) 0x400UL)      /**< AES available address space  */
#define AES_MEM_END          ((uint32_t) 0x400E03FFUL) /**< AES end address  */
#define AES_MEM_BITS         ((uint32_t) 0x10UL)       /**< AES used bits  */
#define USBC_MEM_BASE        ((uint32_t) 0x40100000UL) /**< USBC base address  */
#define USBC_MEM_SIZE        ((uint32_t) 0x40000UL)    /**< USBC available address space  */
#define USBC_MEM_END         ((uint32_t) 0x4013FFFFUL) /**< USBC end address  */
#define USBC_MEM_BITS        ((uint32_t) 0x18UL)       /**< USBC used bits  */
#define PER_MEM_BASE         ((uint32_t) 0x40000000UL) /**< PER base address  */
#define PER_MEM_SIZE         ((uint32_t) 0xE0000UL)    /**< PER available address space  */
#define PER_MEM_END          ((uint32_t) 0x400DFFFFUL) /**< PER end address  */
#define PER_MEM_BITS         ((uint32_t) 0x20UL)       /**< PER used bits  */
#define RAM_MEM_BASE         ((uint32_t) 0x20000000UL) /**< RAM base address  */
#define RAM_MEM_SIZE         ((uint32_t) 0x40000UL)    /**< RAM available address space  */
#define RAM_MEM_END          ((uint32_t) 0x2003FFFFUL) /**< RAM end address  */
#define RAM_MEM_BITS         ((uint32_t) 0x18UL)       /**< RAM used bits  */
#define DEVICE_MEM_BASE      ((uint32_t) 0xF0040000UL) /**< DEVICE base address  */
#define DEVICE_MEM_SIZE      ((uint32_t) 0x1000UL)     /**< DEVICE available address space  */
#define DEVICE_MEM_END       ((uint32_t) 0xF0040FFFUL) /**< DEVICE end address  */
#define DEVICE_MEM_BITS      ((uint32_t) 0x12UL)       /**< DEVICE used bits  */
#define RAM_CODE_MEM_BASE    ((uint32_t) 0x10000000UL) /**< RAM_CODE base address  */
#define RAM_CODE_MEM_SIZE    ((uint32_t) 0x20000UL)    /**< RAM_CODE available address space  */
#define RAM_CODE_MEM_END     ((uint32_t) 0x1001FFFFUL) /**< RAM_CODE end address  */
#define RAM_CODE_MEM_BITS    ((uint32_t) 0x17UL)       /**< RAM_CODE used bits  */

/** Flash and SRAM limits for EFM32HG350F64 */
#define FLASH_BASE           (0x00000000UL) /**< Flash Base Address */
#define FLASH_SIZE           (0x00010000UL) /**< Available Flash Memory */
#define FLASH_PAGE_SIZE      1024           /**< Flash Memory page size */
#define SRAM_BASE            (0x20000000UL) /**< SRAM Base Address */
#define SRAM_SIZE            (0x00002000UL) /**< Available SRAM Memory */
#define __CM0PLUS_REV        0x001          /**< Cortex-M0+ Core revision r0p1 */
#define PRS_CHAN_COUNT       6              /**< Number of PRS channels */
#define DMA_CHAN_COUNT       6              /**< Number of DMA channels */
#define EXT_IRQ_COUNT        21             /**< Number of External (NVIC) interrupts */

/** AF channels connect the different on-chip peripherals with the af-mux */
#define AFCHAN_MAX           42
#define AFCHANLOC_MAX        7
/** Analog AF channels */
#define AFACHAN_MAX          27

/* Part number capabilities */

#define TIMER_PRESENT         /**< TIMER is available in this part */
#define TIMER_COUNT         3 /**< 3 TIMERs available  */
#define ACMP_PRESENT          /**< ACMP is available in this part */
#define ACMP_COUNT          1 /**< 1 ACMPs available  */
#define USART_PRESENT         /**< USART is available in this part */
#define USART_COUNT         2 /**< 2 USARTs available  */
#define IDAC_PRESENT          /**< IDAC is available in this part */
#define IDAC_COUNT          1 /**< 1 IDACs available  */
#define ADC_PRESENT           /**< ADC is available in this part */
#define ADC_COUNT           1 /**< 1 ADCs available  */
#define LEUART_PRESENT        /**< LEUART is available in this part */
#define LEUART_COUNT        1 /**< 1 LEUARTs available  */
#define PCNT_PRESENT          /**< PCNT is available in this part */
#define PCNT_COUNT          1 /**< 1 PCNTs available  */
#define I2C_PRESENT           /**< I2C is available in this part */
#define I2C_COUNT           1 /**< 1 I2Cs available  */
#define AES_PRESENT
#define AES_COUNT           1
#define DMA_PRESENT
#define DMA_COUNT           1
#define LE_PRESENT
#define LE_COUNT            1
#define USBC_PRESENT
#define USBC_COUNT          1
#define USBLE_PRESENT
#define USBLE_COUNT         1
#define USB_PRESENT
#define USB_COUNT           1
#define MSC_PRESENT
#define MSC_COUNT           1
#define EMU_PRESENT
#define EMU_COUNT           1
#define RMU_PRESENT
#define RMU_COUNT           1
#define CMU_PRESENT
#define CMU_COUNT           1
#define PRS_PRESENT
#define PRS_COUNT           1
#define GPIO_PRESENT
#define GPIO_COUNT          1
#define VCMP_PRESENT
#define VCMP_COUNT          1
#define RTC_PRESENT
#define RTC_COUNT           1
#define HFXTAL_PRESENT
#define HFXTAL_COUNT        1
#define LFXTAL_PRESENT
#define LFXTAL_COUNT        1
#define USHFRCO_PRESENT
#define USHFRCO_COUNT       1
#define WDOG_PRESENT
#define WDOG_COUNT          1
#define DBG_PRESENT
#define DBG_COUNT           1
#define MTB_PRESENT
#define MTB_COUNT           1
#define BOOTLOADER_PRESENT
#define BOOTLOADER_COUNT    1
#define ANALOG_PRESENT
#define ANALOG_COUNT        1

/** @} End of group EFM32HG350F64_Part */

#define ARM_MATH_CM0PLUS
#include "arm_math.h"       /* To get __CLZ definitions etc. */
#include "core_cm0plus.h"   /* Cortex-M0+ processor and core peripherals */
#include "system_efm32hg.h" /* System Header */

/**************************************************************************//**
 * @defgroup EFM32HG350F64_Peripheral_TypeDefs EFM32HG350F64 Peripheral TypeDefs
 * @{
 * @brief Device Specific Peripheral Register Structures
 *****************************************************************************/

#include "efm32hg_aes.h"
#include "efm32hg_dma_ch.h"
#include "efm32hg_dma.h"
#include "efm32hg_usb_diep.h"
#include "efm32hg_usb_doep.h"
#include "efm32hg_usb.h"
#include "efm32hg_msc.h"
#include "efm32hg_emu.h"
#include "efm32hg_rmu.h"
#include "efm32hg_cmu.h"
#include "efm32hg_timer_cc.h"
#include "efm32hg_timer.h"
#include "efm32hg_acmp.h"
#include "efm32hg_usart.h"
#include "efm32hg_prs_ch.h"
#include "efm32hg_prs.h"
#include "efm32hg_idac.h"
#include "efm32hg_gpio_p.h"
#include "efm32hg_gpio.h"
#include "efm32hg_vcmp.h"
#include "efm32hg_adc.h"
#include "efm32hg_leuart.h"
#include "efm32hg_pcnt.h"
#include "efm32hg_i2c.h"
#include "efm32hg_rtc.h"
#include "efm32hg_wdog.h"
#include "efm32hg_mtb.h"
#include "efm32hg_dma_descriptor.h"
#include "efm32hg_devinfo.h"
#include "efm32hg_romtable.h"
#include "efm32hg_calibrate.h"

/** @} End of group EFM32HG350F64_Peripheral_TypeDefs */

/**************************************************************************//**
 * @defgroup EFM32HG350F64_Peripheral_Base EFM32HG350F64 Peripheral Memory Map
 * @{
 *****************************************************************************/

#define AES_BASE          (0x400E0000UL) /**< AES base address  */
#define DMA_BASE          (0x400C2000UL) /**< DMA base address  */
#define USB_BASE          (0x400C4000UL) /**< USB base address  */
#define MSC_BASE          (0x400C0000UL) /**< MSC base address  */
#define EMU_BASE          (0x400C6000UL) /**< EMU base address  */
#define RMU_BASE          (0x400CA000UL) /**< RMU base address  */
#define CMU_BASE          (0x400C8000UL) /**< CMU base address  */
#define TIMER0_BASE       (0x40010000UL) /**< TIMER0 base address  */
#define TIMER1_BASE       (0x40010400UL) /**< TIMER1 base address  */
#define TIMER2_BASE       (0x40010800UL) /**< TIMER2 base address  */
#define ACMP0_BASE        (0x40001000UL) /**< ACMP0 base address  */
#define USART0_BASE       (0x4000C000UL) /**< USART0 base address  */
#define USART1_BASE       (0x4000C400UL) /**< USART1 base address  */
#define PRS_BASE          (0x400CC000UL) /**< PRS base address  */
#define IDAC0_BASE        (0x40004000UL) /**< IDAC0 base address  */
#define GPIO_BASE         (0x40006000UL) /**< GPIO base address  */
#define VCMP_BASE         (0x40000000UL) /**< VCMP base address  */
#define ADC0_BASE         (0x40002000UL) /**< ADC0 base address  */
#define LEUART0_BASE      (0x40084000UL) /**< LEUART0 base address  */
#define PCNT0_BASE        (0x40086000UL) /**< PCNT0 base address  */
#define I2C0_BASE         (0x4000A000UL) /**< I2C0 base address  */
#define RTC_BASE          (0x40080000UL) /**< RTC base address  */
#define WDOG_BASE         (0x40088000UL) /**< WDOG base address  */
#define MTB_BASE          (0xF0040000UL) /**< MTB base address  */
#define CALIBRATE_BASE    (0x0FE08000UL) /**< CALIBRATE base address */
#define DEVINFO_BASE      (0x0FE081B0UL) /**< DEVINFO base address */
#define ROMTABLE_BASE     (0xF00FFFD0UL) /**< ROMTABLE base address */
#define LOCKBITS_BASE     (0x0FE04000UL) /**< Lock-bits page base address */
#define USERDATA_BASE     (0x0FE00000UL) /**< User data page base address */

/** @} End of group EFM32HG350F64_Peripheral_Base */

/**************************************************************************//**
 * @defgroup EFM32HG350F64_Peripheral_Declaration  EFM32HG350F64 Peripheral Declarations
 * @{
 *****************************************************************************/

#define AES          ((AES_TypeDef *) AES_BASE)             /**< AES base pointer */
#define DMA          ((DMA_TypeDef *) DMA_BASE)             /**< DMA base pointer */
#define USB          ((USB_TypeDef *) USB_BASE)             /**< USB base pointer */
#define MSC          ((MSC_TypeDef *) MSC_BASE)             /**< MSC base pointer */
#define EMU          ((EMU_TypeDef *) EMU_BASE)             /**< EMU base pointer */
#define RMU          ((RMU_TypeDef *) RMU_BASE)             /**< RMU base pointer */
#define CMU          ((CMU_TypeDef *) CMU_BASE)             /**< CMU base pointer */
#define TIMER0       ((TIMER_TypeDef *) TIMER0_BASE)        /**< TIMER0 base pointer */
#define TIMER1       ((TIMER_TypeDef *) TIMER1_BASE)        /**< TIMER1 base pointer */
#define TIMER2       ((TIMER_TypeDef *) TIMER2_BASE)        /**< TIMER2 base pointer */
#define ACMP0        ((ACMP_TypeDef *) ACMP0_BASE)          /**< ACMP0 base pointer */
#define USART0       ((USART_TypeDef *) USART0_BASE)        /**< USART0 base pointer */
#define USART1       ((USART_TypeDef *) USART1_BASE)        /**< USART1 base pointer */
#define PRS          ((PRS_TypeDef *) PRS_BASE)             /**< PRS base pointer */
#define IDAC0        ((IDAC_TypeDef *) IDAC0_BASE)          /**< IDAC0 base pointer */
#define GPIO         ((GPIO_TypeDef *) GPIO_BASE)           /**< GPIO base pointer */
#define VCMP         ((VCMP_TypeDef *) VCMP_BASE)           /**< VCMP base pointer */
#define ADC0         ((ADC_TypeDef *) ADC0_BASE)            /**< ADC0 base pointer */
#define LEUART0      ((LEUART_TypeDef *) LEUART0_BASE)      /**< LEUART0 base pointer */
#define PCNT0        ((PCNT_TypeDef *) PCNT0_BASE)          /**< PCNT0 base pointer */
#define I2C0         ((I2C_TypeDef *) I2C0_BASE)            /**< I2C0 base pointer */
#define RTC          ((RTC_TypeDef *) RTC_BASE)             /**< RTC base pointer */
#define WDOG         ((WDOG_TypeDef *) WDOG_BASE)           /**< WDOG base pointer */
#define MTB          ((MTB_TypeDef *) MTB_BASE)             /**< MTB base pointer */
#define CALIBRATE    ((CALIBRATE_TypeDef *) CALIBRATE_BASE) /**< CALIBRATE base pointer */
#define DEVINFO      ((DEVINFO_TypeDef *) DEVINFO_BASE)     /**< DEVINFO base pointer */
#define ROMTABLE     ((ROMTABLE_TypeDef *) ROMTABLE_BASE)   /**< ROMTABLE base pointer */

/** @} End of group EFM32HG350F64_Peripheral_Declaration */

/**************************************************************************//**
 * @defgroup EFM32HG350F64_BitFields EFM32HG350F64 Bit Fields
 * @{
 *****************************************************************************/

#include "efm32hg_prs_signals.h"
#include "efm32hg_dmareq.h"
#include "efm32hg_dmactrl.h"

/**************************************************************************//**
 * @defgroup EFM32HG350F64_UNLOCK EFM32HG350F64 Unlock Codes
 * @{
 *****************************************************************************/
#define MSC_UNLOCK_CODE      0x1B71 /**< MSC unlock code */
#define EMU_UNLOCK_CODE      0xADE8 /**< EMU unlock code */
#define CMU_UNLOCK_CODE      0x580E /**< CMU unlock code */
#define TIMER_UNLOCK_CODE    0xCE80 /**< TIMER unlock code */
#define GPIO_UNLOCK_CODE     0xA534 /**< GPIO unlock code */

/** @} End of group EFM32HG350F64_UNLOCK */

/** @} End of group EFM32HG350F64_BitFields */

/**************************************************************************//**
 * @defgroup EFM32HG350F64_Alternate_Function EFM32HG350F64 Alternate Function
 * @{
 *****************************************************************************/

#include "efm32hg_af_ports.h"
#include "efm32hg_af_pins.h"

/** @} End of group EFM32HG350F64_Alternate_Function */

/**************************************************************************//**
 *  @brief Set the value of a bit field within a register.
 *
 *  @param REG
 *       The register to update
 *  @param MASK
 *       The mask for the bit field to update
 *  @param VALUE
 *       The value to write to the bit field
 *  @param OFFSET
 *       The number of bits that the field is offset within the register.
 *       0 (zero) means LSB.
 *****************************************************************************/
#define SET_BIT_FIELD(REG, MASK, VALUE, OFFSET) \
  REG = ((REG) &~(MASK)) | (((VALUE) << (OFFSET)) & (MASK));

/** @} End of group EFM32HG350F64 */

/** @} End of group Parts */

#ifdef __cplusplus
}
#endif
#endif /* EFM32HG350F64_H */
