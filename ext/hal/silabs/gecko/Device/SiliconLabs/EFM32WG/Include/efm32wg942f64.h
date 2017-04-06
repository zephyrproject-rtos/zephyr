/**************************************************************************//**
 * @file efm32wg942f64.h
 * @brief CMSIS Cortex-M Peripheral Access Layer Header File
 *        for EFM32WG942F64
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

#ifndef EFM32WG942F64_H
#define EFM32WG942F64_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************//**
 * @addtogroup Parts
 * @{
 *****************************************************************************/

/**************************************************************************//**
 * @defgroup EFM32WG942F64 EFM32WG942F64
 * @{
 *****************************************************************************/

/** Interrupt Number Definition */
typedef enum IRQn
{
/******  Cortex-M4 Processor Exceptions Numbers ********************************************/
  NonMaskableInt_IRQn   = -14,              /*!< -14 Cortex-M4 Non Maskable Interrupt      */
  HardFault_IRQn        = -13,              /*!< -13 Cortex-M4 Hard Fault Interrupt        */
  MemoryManagement_IRQn = -12,              /*!< -12 Cortex-M4 Memory Management Interrupt */
  BusFault_IRQn         = -11,              /*!< -11 Cortex-M4 Bus Fault Interrupt         */
  UsageFault_IRQn       = -10,              /*!< -10 Cortex-M4 Usage Fault Interrupt       */
  SVCall_IRQn           = -5,               /*!< -5  Cortex-M4 SV Call Interrupt           */
  DebugMonitor_IRQn     = -4,               /*!< -4  Cortex-M4 Debug Monitor Interrupt     */
  PendSV_IRQn           = -2,               /*!< -2  Cortex-M4 Pend SV Interrupt           */
  SysTick_IRQn          = -1,               /*!< -1  Cortex-M4 System Tick Interrupt       */

/******  EFM32WG Peripheral Interrupt Numbers **********************************************/
  DMA_IRQn              = 0,  /*!< 0 EFM32 DMA Interrupt */
  GPIO_EVEN_IRQn        = 1,  /*!< 1 EFM32 GPIO_EVEN Interrupt */
  TIMER0_IRQn           = 2,  /*!< 2 EFM32 TIMER0 Interrupt */
  USART0_RX_IRQn        = 3,  /*!< 3 EFM32 USART0_RX Interrupt */
  USART0_TX_IRQn        = 4,  /*!< 4 EFM32 USART0_TX Interrupt */
  USB_IRQn              = 5,  /*!< 5 EFM32 USB Interrupt */
  ACMP0_IRQn            = 6,  /*!< 6 EFM32 ACMP0 Interrupt */
  ADC0_IRQn             = 7,  /*!< 7 EFM32 ADC0 Interrupt */
  DAC0_IRQn             = 8,  /*!< 8 EFM32 DAC0 Interrupt */
  I2C0_IRQn             = 9,  /*!< 9 EFM32 I2C0 Interrupt */
  I2C1_IRQn             = 10, /*!< 10 EFM32 I2C1 Interrupt */
  GPIO_ODD_IRQn         = 11, /*!< 11 EFM32 GPIO_ODD Interrupt */
  TIMER1_IRQn           = 12, /*!< 12 EFM32 TIMER1 Interrupt */
  TIMER2_IRQn           = 13, /*!< 13 EFM32 TIMER2 Interrupt */
  TIMER3_IRQn           = 14, /*!< 14 EFM32 TIMER3 Interrupt */
  USART1_RX_IRQn        = 15, /*!< 15 EFM32 USART1_RX Interrupt */
  USART1_TX_IRQn        = 16, /*!< 16 EFM32 USART1_TX Interrupt */
  LESENSE_IRQn          = 17, /*!< 17 EFM32 LESENSE Interrupt */
  USART2_RX_IRQn        = 18, /*!< 18 EFM32 USART2_RX Interrupt */
  USART2_TX_IRQn        = 19, /*!< 19 EFM32 USART2_TX Interrupt */
  LEUART0_IRQn          = 24, /*!< 24 EFM32 LEUART0 Interrupt */
  LEUART1_IRQn          = 25, /*!< 25 EFM32 LEUART1 Interrupt */
  LETIMER0_IRQn         = 26, /*!< 26 EFM32 LETIMER0 Interrupt */
  PCNT0_IRQn            = 27, /*!< 27 EFM32 PCNT0 Interrupt */
  PCNT1_IRQn            = 28, /*!< 28 EFM32 PCNT1 Interrupt */
  PCNT2_IRQn            = 29, /*!< 29 EFM32 PCNT2 Interrupt */
  RTC_IRQn              = 30, /*!< 30 EFM32 RTC Interrupt */
  BURTC_IRQn            = 31, /*!< 31 EFM32 BURTC Interrupt */
  CMU_IRQn              = 32, /*!< 32 EFM32 CMU Interrupt */
  VCMP_IRQn             = 33, /*!< 33 EFM32 VCMP Interrupt */
  LCD_IRQn              = 34, /*!< 34 EFM32 LCD Interrupt */
  MSC_IRQn              = 35, /*!< 35 EFM32 MSC Interrupt */
  AES_IRQn              = 36, /*!< 36 EFM32 AES Interrupt */
  EMU_IRQn              = 38, /*!< 38 EFM32 EMU Interrupt */
  FPUEH_IRQn            = 39, /*!< 39 EFM32 FPUEH Interrupt */
} IRQn_Type;

/**************************************************************************//**
 * @defgroup EFM32WG942F64_Core EFM32WG942F64 Core
 * @{
 * @brief Processor and Core Peripheral Section
 *****************************************************************************/
#define __MPU_PRESENT             1 /**< Presence of MPU  */
#define __FPU_PRESENT             1 /**< Presence of FPU  */
#define __VTOR_PRESENT            1 /**< Presence of VTOR register in SCB */
#define __NVIC_PRIO_BITS          3 /**< NVIC interrupt priority bits */
#define __Vendor_SysTickConfig    0 /**< Is 1 if different SysTick counter is used */

/** @} End of group EFM32WG942F64_Core */

/**************************************************************************//**
* @defgroup EFM32WG942F64_Part EFM32WG942F64 Part
* @{
******************************************************************************/

/** Part family */
#define _EFM32_WONDER_FAMILY                    1  /**< Wonder Gecko EFM32WG MCU Family */
#define _EFM_DEVICE                                /**< Silicon Labs EFM-type microcontroller */
#define _SILICON_LABS_32B_SERIES_0                 /**< Silicon Labs series number */
#define _SILICON_LABS_32B_SERIES                0  /**< Silicon Labs series number */
#define _SILICON_LABS_GECKO_INTERNAL_SDID       74 /** Silicon Labs internal use only, may change any time */
#define _SILICON_LABS_GECKO_INTERNAL_SDID_74       /** Silicon Labs internal use only, may change any time */
#define _SILICON_LABS_32B_PLATFORM_1               /**< @deprecated Silicon Labs platform name */
#define _SILICON_LABS_32B_PLATFORM              1  /**< @deprecated Silicon Labs platform name */

/* If part number is not defined as compiler option, define it */
#if !defined(EFM32WG942F64)
#define EFM32WG942F64    1 /**< Wonder Gecko Part  */
#endif

/** Configure part number */
#define PART_NUMBER          "EFM32WG942F64" /**< Part Number */

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
#define EBI_CODE_MEM_BASE    ((uint32_t) 0x12000000UL) /**< EBI_CODE base address  */
#define EBI_CODE_MEM_SIZE    ((uint32_t) 0xE000000UL)  /**< EBI_CODE available address space  */
#define EBI_CODE_MEM_END     ((uint32_t) 0x1FFFFFFFUL) /**< EBI_CODE end address  */
#define EBI_CODE_MEM_BITS    ((uint32_t) 0x28UL)       /**< EBI_CODE used bits  */
#define PER_MEM_BASE         ((uint32_t) 0x40000000UL) /**< PER base address  */
#define PER_MEM_SIZE         ((uint32_t) 0xE0000UL)    /**< PER available address space  */
#define PER_MEM_END          ((uint32_t) 0x400DFFFFUL) /**< PER end address  */
#define PER_MEM_BITS         ((uint32_t) 0x20UL)       /**< PER used bits  */
#define RAM_MEM_BASE         ((uint32_t) 0x20000000UL) /**< RAM base address  */
#define RAM_MEM_SIZE         ((uint32_t) 0x40000UL)    /**< RAM available address space  */
#define RAM_MEM_END          ((uint32_t) 0x2003FFFFUL) /**< RAM end address  */
#define RAM_MEM_BITS         ((uint32_t) 0x18UL)       /**< RAM used bits  */
#define RAM_CODE_MEM_BASE    ((uint32_t) 0x10000000UL) /**< RAM_CODE base address  */
#define RAM_CODE_MEM_SIZE    ((uint32_t) 0x20000UL)    /**< RAM_CODE available address space  */
#define RAM_CODE_MEM_END     ((uint32_t) 0x1001FFFFUL) /**< RAM_CODE end address  */
#define RAM_CODE_MEM_BITS    ((uint32_t) 0x17UL)       /**< RAM_CODE used bits  */
#define EBI_MEM_BASE         ((uint32_t) 0x80000000UL) /**< EBI base address  */
#define EBI_MEM_SIZE         ((uint32_t) 0x40000000UL) /**< EBI available address space  */
#define EBI_MEM_END          ((uint32_t) 0xBFFFFFFFUL) /**< EBI end address  */
#define EBI_MEM_BITS         ((uint32_t) 0x30UL)       /**< EBI used bits  */

/** Bit banding area */
#define BITBAND_PER_BASE     ((uint32_t) 0x42000000UL) /**< Peripheral Address Space bit-band area */
#define BITBAND_RAM_BASE     ((uint32_t) 0x22000000UL) /**< SRAM Address Space bit-band area */

/** Flash and SRAM limits for EFM32WG942F64 */
#define FLASH_BASE           (0x00000000UL) /**< Flash Base Address */
#define FLASH_SIZE           (0x00010000UL) /**< Available Flash Memory */
#define FLASH_PAGE_SIZE      2048           /**< Flash Memory page size */
#define SRAM_BASE            (0x20000000UL) /**< SRAM Base Address */
#define SRAM_SIZE            (0x00008000UL) /**< Available SRAM Memory */
#define __CM4_REV            0x001          /**< Cortex-M4 Core revision r0p1 */
#define PRS_CHAN_COUNT       12             /**< Number of PRS channels */
#define DMA_CHAN_COUNT       12             /**< Number of DMA channels */
#define EXT_IRQ_COUNT        40             /**< Number of External (NVIC) interrupts */

/** AF channels connect the different on-chip peripherals with the af-mux */
#define AFCHAN_MAX           163
#define AFCHANLOC_MAX        7
/** Analog AF channels */
#define AFACHAN_MAX          53

/* Part number capabilities */

#define USART_PRESENT         /**< USART is available in this part */
#define USART_COUNT         3 /**< 3 USARTs available  */
#define TIMER_PRESENT         /**< TIMER is available in this part */
#define TIMER_COUNT         4 /**< 4 TIMERs available  */
#define ACMP_PRESENT          /**< ACMP is available in this part */
#define ACMP_COUNT          2 /**< 2 ACMPs available  */
#define LEUART_PRESENT        /**< LEUART is available in this part */
#define LEUART_COUNT        2 /**< 2 LEUARTs available  */
#define LETIMER_PRESENT       /**< LETIMER is available in this part */
#define LETIMER_COUNT       1 /**< 1 LETIMERs available  */
#define PCNT_PRESENT          /**< PCNT is available in this part */
#define PCNT_COUNT          3 /**< 3 PCNTs available  */
#define I2C_PRESENT           /**< I2C is available in this part */
#define I2C_COUNT           2 /**< 2 I2Cs available  */
#define ADC_PRESENT           /**< ADC is available in this part */
#define ADC_COUNT           1 /**< 1 ADCs available  */
#define DAC_PRESENT           /**< DAC is available in this part */
#define DAC_COUNT           1 /**< 1 DACs available  */
#define DMA_PRESENT
#define DMA_COUNT           1
#define AES_PRESENT
#define AES_COUNT           1
#define USBC_PRESENT
#define USBC_COUNT          1
#define USB_PRESENT
#define USB_COUNT           1
#define LE_PRESENT
#define LE_COUNT            1
#define MSC_PRESENT
#define MSC_COUNT           1
#define EMU_PRESENT
#define EMU_COUNT           1
#define RMU_PRESENT
#define RMU_COUNT           1
#define CMU_PRESENT
#define CMU_COUNT           1
#define LESENSE_PRESENT
#define LESENSE_COUNT       1
#define FPUEH_PRESENT
#define FPUEH_COUNT         1
#define RTC_PRESENT
#define RTC_COUNT           1
#define GPIO_PRESENT
#define GPIO_COUNT          1
#define VCMP_PRESENT
#define VCMP_COUNT          1
#define PRS_PRESENT
#define PRS_COUNT           1
#define OPAMP_PRESENT
#define OPAMP_COUNT         1
#define BU_PRESENT
#define BU_COUNT            1
#define LCD_PRESENT
#define LCD_COUNT           1
#define BURTC_PRESENT
#define BURTC_COUNT         1
#define HFXTAL_PRESENT
#define HFXTAL_COUNT        1
#define LFXTAL_PRESENT
#define LFXTAL_COUNT        1
#define WDOG_PRESENT
#define WDOG_COUNT          1
#define DBG_PRESENT
#define DBG_COUNT           1
#define ETM_PRESENT
#define ETM_COUNT           1
#define BOOTLOADER_PRESENT
#define BOOTLOADER_COUNT    1
#define ANALOG_PRESENT
#define ANALOG_COUNT        1

#include "core_cm4.h"       /* Cortex-M4 processor and core peripherals */
#include "system_efm32wg.h" /* System Header */

/** @} End of group EFM32WG942F64_Part */

/**************************************************************************//**
 * @defgroup EFM32WG942F64_Peripheral_TypeDefs EFM32WG942F64 Peripheral TypeDefs
 * @{
 * @brief Device Specific Peripheral Register Structures
 *****************************************************************************/

#include "efm32wg_dma_ch.h"

/**************************************************************************//**
 * @defgroup EFM32WG942F64_DMA EFM32WG942F64 DMA
 * @{
 * @brief EFM32WG942F64_DMA Register Declaration
 *****************************************************************************/
typedef struct
{
  __IM uint32_t  STATUS;         /**< DMA Status Registers  */
  __OM uint32_t  CONFIG;         /**< DMA Configuration Register  */
  __IOM uint32_t CTRLBASE;       /**< Channel Control Data Base Pointer Register  */
  __IM uint32_t  ALTCTRLBASE;    /**< Channel Alternate Control Data Base Pointer Register  */
  __IM uint32_t  CHWAITSTATUS;   /**< Channel Wait on Request Status Register  */
  __OM uint32_t  CHSWREQ;        /**< Channel Software Request Register  */
  __IOM uint32_t CHUSEBURSTS;    /**< Channel Useburst Set Register  */
  __OM uint32_t  CHUSEBURSTC;    /**< Channel Useburst Clear Register  */
  __IOM uint32_t CHREQMASKS;     /**< Channel Request Mask Set Register  */
  __OM uint32_t  CHREQMASKC;     /**< Channel Request Mask Clear Register  */
  __IOM uint32_t CHENS;          /**< Channel Enable Set Register  */
  __OM uint32_t  CHENC;          /**< Channel Enable Clear Register  */
  __IOM uint32_t CHALTS;         /**< Channel Alternate Set Register  */
  __OM uint32_t  CHALTC;         /**< Channel Alternate Clear Register  */
  __IOM uint32_t CHPRIS;         /**< Channel Priority Set Register  */
  __OM uint32_t  CHPRIC;         /**< Channel Priority Clear Register  */
  uint32_t       RESERVED0[3];   /**< Reserved for future use **/
  __IOM uint32_t ERRORC;         /**< Bus Error Clear Register  */

  uint32_t       RESERVED1[880]; /**< Reserved for future use **/
  __IM uint32_t  CHREQSTATUS;    /**< Channel Request Status  */
  uint32_t       RESERVED2[1];   /**< Reserved for future use **/
  __IM uint32_t  CHSREQSTATUS;   /**< Channel Single Request Status  */

  uint32_t       RESERVED3[121]; /**< Reserved for future use **/
  __IM uint32_t  IF;             /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;            /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;            /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;            /**< Interrupt Enable register  */
  __IOM uint32_t CTRL;           /**< DMA Control Register  */
  __IOM uint32_t RDS;            /**< DMA Retain Descriptor State  */

  uint32_t       RESERVED4[2];   /**< Reserved for future use **/
  __IOM uint32_t LOOP0;          /**< Channel 0 Loop Register  */
  __IOM uint32_t LOOP1;          /**< Channel 1 Loop Register  */
  uint32_t       RESERVED5[14];  /**< Reserved for future use **/
  __IOM uint32_t RECT0;          /**< Channel 0 Rectangle Register  */

  uint32_t       RESERVED6[39];  /**< Reserved registers */
  DMA_CH_TypeDef CH[12];         /**< Channel registers */
} DMA_TypeDef;                   /** @} */

#include "efm32wg_aes.h"
#include "efm32wg_usb_hc.h"
#include "efm32wg_usb_diep.h"
#include "efm32wg_usb_doep.h"
#include "efm32wg_usb.h"
#include "efm32wg_msc.h"
#include "efm32wg_emu.h"
#include "efm32wg_rmu.h"

/**************************************************************************//**
 * @defgroup EFM32WG942F64_CMU EFM32WG942F64 CMU
 * @{
 * @brief EFM32WG942F64_CMU Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;         /**< CMU Control Register  */
  __IOM uint32_t HFCORECLKDIV; /**< High Frequency Core Clock Division Register  */
  __IOM uint32_t HFPERCLKDIV;  /**< High Frequency Peripheral Clock Division Register  */
  __IOM uint32_t HFRCOCTRL;    /**< HFRCO Control Register  */
  __IOM uint32_t LFRCOCTRL;    /**< LFRCO Control Register  */
  __IOM uint32_t AUXHFRCOCTRL; /**< AUXHFRCO Control Register  */
  __IOM uint32_t CALCTRL;      /**< Calibration Control Register  */
  __IOM uint32_t CALCNT;       /**< Calibration Counter Register  */
  __IOM uint32_t OSCENCMD;     /**< Oscillator Enable/Disable Command Register  */
  __IOM uint32_t CMD;          /**< Command Register  */
  __IOM uint32_t LFCLKSEL;     /**< Low Frequency Clock Select Register  */
  __IM uint32_t  STATUS;       /**< Status Register  */
  __IM uint32_t  IF;           /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;          /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;          /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;          /**< Interrupt Enable Register  */
  __IOM uint32_t HFCORECLKEN0; /**< High Frequency Core Clock Enable Register 0  */
  __IOM uint32_t HFPERCLKEN0;  /**< High Frequency Peripheral Clock Enable Register 0  */
  uint32_t       RESERVED0[2]; /**< Reserved for future use **/
  __IM uint32_t  SYNCBUSY;     /**< Synchronization Busy Register  */
  __IOM uint32_t FREEZE;       /**< Freeze Register  */
  __IOM uint32_t LFACLKEN0;    /**< Low Frequency A Clock Enable Register 0  (Async Reg)  */
  uint32_t       RESERVED1[1]; /**< Reserved for future use **/
  __IOM uint32_t LFBCLKEN0;    /**< Low Frequency B Clock Enable Register 0 (Async Reg)  */

  uint32_t       RESERVED2[1]; /**< Reserved for future use **/
  __IOM uint32_t LFAPRESC0;    /**< Low Frequency A Prescaler Register 0 (Async Reg)  */
  uint32_t       RESERVED3[1]; /**< Reserved for future use **/
  __IOM uint32_t LFBPRESC0;    /**< Low Frequency B Prescaler Register 0  (Async Reg)  */
  uint32_t       RESERVED4[1]; /**< Reserved for future use **/
  __IOM uint32_t PCNTCTRL;     /**< PCNT Control Register  */
  __IOM uint32_t LCDCTRL;      /**< LCD Control Register  */
  __IOM uint32_t ROUTE;        /**< I/O Routing Register  */
  __IOM uint32_t LOCK;         /**< Configuration Lock Register  */
} CMU_TypeDef;                 /** @} */

#include "efm32wg_lesense_st.h"
#include "efm32wg_lesense_buf.h"
#include "efm32wg_lesense_ch.h"
#include "efm32wg_lesense.h"
#include "efm32wg_fpueh.h"
#include "efm32wg_usart.h"
#include "efm32wg_timer_cc.h"
#include "efm32wg_timer.h"
#include "efm32wg_acmp.h"
#include "efm32wg_leuart.h"
#include "efm32wg_rtc.h"
#include "efm32wg_letimer.h"
#include "efm32wg_pcnt.h"
#include "efm32wg_i2c.h"
#include "efm32wg_gpio_p.h"
#include "efm32wg_gpio.h"
#include "efm32wg_vcmp.h"
#include "efm32wg_prs_ch.h"

/**************************************************************************//**
 * @defgroup EFM32WG942F64_PRS EFM32WG942F64 PRS
 * @{
 * @brief EFM32WG942F64_PRS Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t SWPULSE;      /**< Software Pulse Register  */
  __IOM uint32_t SWLEVEL;      /**< Software Level Register  */
  __IOM uint32_t ROUTE;        /**< I/O Routing Register  */

  uint32_t       RESERVED0[1]; /**< Reserved registers */
  PRS_CH_TypeDef CH[12];       /**< Channel registers */
} PRS_TypeDef;                 /** @} */

#include "efm32wg_adc.h"
#include "efm32wg_dac.h"
#include "efm32wg_lcd.h"
#include "efm32wg_burtc_ret.h"
#include "efm32wg_burtc.h"
#include "efm32wg_wdog.h"
#include "efm32wg_etm.h"
#include "efm32wg_dma_descriptor.h"
#include "efm32wg_devinfo.h"
#include "efm32wg_romtable.h"
#include "efm32wg_calibrate.h"

/** @} End of group EFM32WG942F64_Peripheral_TypeDefs */

/**************************************************************************//**
 * @defgroup EFM32WG942F64_Peripheral_Base EFM32WG942F64 Peripheral Memory Map
 * @{
 *****************************************************************************/

#define DMA_BASE          (0x400C2000UL) /**< DMA base address  */
#define AES_BASE          (0x400E0000UL) /**< AES base address  */
#define USB_BASE          (0x400C4000UL) /**< USB base address  */
#define MSC_BASE          (0x400C0000UL) /**< MSC base address  */
#define EMU_BASE          (0x400C6000UL) /**< EMU base address  */
#define RMU_BASE          (0x400CA000UL) /**< RMU base address  */
#define CMU_BASE          (0x400C8000UL) /**< CMU base address  */
#define LESENSE_BASE      (0x4008C000UL) /**< LESENSE base address  */
#define FPUEH_BASE        (0x400C1C00UL) /**< FPUEH base address  */
#define USART0_BASE       (0x4000C000UL) /**< USART0 base address  */
#define USART1_BASE       (0x4000C400UL) /**< USART1 base address  */
#define USART2_BASE       (0x4000C800UL) /**< USART2 base address  */
#define TIMER0_BASE       (0x40010000UL) /**< TIMER0 base address  */
#define TIMER1_BASE       (0x40010400UL) /**< TIMER1 base address  */
#define TIMER2_BASE       (0x40010800UL) /**< TIMER2 base address  */
#define TIMER3_BASE       (0x40010C00UL) /**< TIMER3 base address  */
#define ACMP0_BASE        (0x40001000UL) /**< ACMP0 base address  */
#define ACMP1_BASE        (0x40001400UL) /**< ACMP1 base address  */
#define LEUART0_BASE      (0x40084000UL) /**< LEUART0 base address  */
#define LEUART1_BASE      (0x40084400UL) /**< LEUART1 base address  */
#define RTC_BASE          (0x40080000UL) /**< RTC base address  */
#define LETIMER0_BASE     (0x40082000UL) /**< LETIMER0 base address  */
#define PCNT0_BASE        (0x40086000UL) /**< PCNT0 base address  */
#define PCNT1_BASE        (0x40086400UL) /**< PCNT1 base address  */
#define PCNT2_BASE        (0x40086800UL) /**< PCNT2 base address  */
#define I2C0_BASE         (0x4000A000UL) /**< I2C0 base address  */
#define I2C1_BASE         (0x4000A400UL) /**< I2C1 base address  */
#define GPIO_BASE         (0x40006000UL) /**< GPIO base address  */
#define VCMP_BASE         (0x40000000UL) /**< VCMP base address  */
#define PRS_BASE          (0x400CC000UL) /**< PRS base address  */
#define ADC0_BASE         (0x40002000UL) /**< ADC0 base address  */
#define DAC0_BASE         (0x40004000UL) /**< DAC0 base address  */
#define LCD_BASE          (0x4008A000UL) /**< LCD base address  */
#define BURTC_BASE        (0x40081000UL) /**< BURTC base address  */
#define WDOG_BASE         (0x40088000UL) /**< WDOG base address  */
#define ETM_BASE          (0xE0041000UL) /**< ETM base address  */
#define CALIBRATE_BASE    (0x0FE08000UL) /**< CALIBRATE base address */
#define DEVINFO_BASE      (0x0FE081B0UL) /**< DEVINFO base address */
#define ROMTABLE_BASE     (0xE00FFFD0UL) /**< ROMTABLE base address */
#define LOCKBITS_BASE     (0x0FE04000UL) /**< Lock-bits page base address */
#define USERDATA_BASE     (0x0FE00000UL) /**< User data page base address */

/** @} End of group EFM32WG942F64_Peripheral_Base */

/**************************************************************************//**
 * @defgroup EFM32WG942F64_Peripheral_Declaration  EFM32WG942F64 Peripheral Declarations
 * @{
 *****************************************************************************/

#define DMA          ((DMA_TypeDef *) DMA_BASE)             /**< DMA base pointer */
#define AES          ((AES_TypeDef *) AES_BASE)             /**< AES base pointer */
#define USB          ((USB_TypeDef *) USB_BASE)             /**< USB base pointer */
#define MSC          ((MSC_TypeDef *) MSC_BASE)             /**< MSC base pointer */
#define EMU          ((EMU_TypeDef *) EMU_BASE)             /**< EMU base pointer */
#define RMU          ((RMU_TypeDef *) RMU_BASE)             /**< RMU base pointer */
#define CMU          ((CMU_TypeDef *) CMU_BASE)             /**< CMU base pointer */
#define LESENSE      ((LESENSE_TypeDef *) LESENSE_BASE)     /**< LESENSE base pointer */
#define FPUEH        ((FPUEH_TypeDef *) FPUEH_BASE)         /**< FPUEH base pointer */
#define USART0       ((USART_TypeDef *) USART0_BASE)        /**< USART0 base pointer */
#define USART1       ((USART_TypeDef *) USART1_BASE)        /**< USART1 base pointer */
#define USART2       ((USART_TypeDef *) USART2_BASE)        /**< USART2 base pointer */
#define TIMER0       ((TIMER_TypeDef *) TIMER0_BASE)        /**< TIMER0 base pointer */
#define TIMER1       ((TIMER_TypeDef *) TIMER1_BASE)        /**< TIMER1 base pointer */
#define TIMER2       ((TIMER_TypeDef *) TIMER2_BASE)        /**< TIMER2 base pointer */
#define TIMER3       ((TIMER_TypeDef *) TIMER3_BASE)        /**< TIMER3 base pointer */
#define ACMP0        ((ACMP_TypeDef *) ACMP0_BASE)          /**< ACMP0 base pointer */
#define ACMP1        ((ACMP_TypeDef *) ACMP1_BASE)          /**< ACMP1 base pointer */
#define LEUART0      ((LEUART_TypeDef *) LEUART0_BASE)      /**< LEUART0 base pointer */
#define LEUART1      ((LEUART_TypeDef *) LEUART1_BASE)      /**< LEUART1 base pointer */
#define RTC          ((RTC_TypeDef *) RTC_BASE)             /**< RTC base pointer */
#define LETIMER0     ((LETIMER_TypeDef *) LETIMER0_BASE)    /**< LETIMER0 base pointer */
#define PCNT0        ((PCNT_TypeDef *) PCNT0_BASE)          /**< PCNT0 base pointer */
#define PCNT1        ((PCNT_TypeDef *) PCNT1_BASE)          /**< PCNT1 base pointer */
#define PCNT2        ((PCNT_TypeDef *) PCNT2_BASE)          /**< PCNT2 base pointer */
#define I2C0         ((I2C_TypeDef *) I2C0_BASE)            /**< I2C0 base pointer */
#define I2C1         ((I2C_TypeDef *) I2C1_BASE)            /**< I2C1 base pointer */
#define GPIO         ((GPIO_TypeDef *) GPIO_BASE)           /**< GPIO base pointer */
#define VCMP         ((VCMP_TypeDef *) VCMP_BASE)           /**< VCMP base pointer */
#define PRS          ((PRS_TypeDef *) PRS_BASE)             /**< PRS base pointer */
#define ADC0         ((ADC_TypeDef *) ADC0_BASE)            /**< ADC0 base pointer */
#define DAC0         ((DAC_TypeDef *) DAC0_BASE)            /**< DAC0 base pointer */
#define LCD          ((LCD_TypeDef *) LCD_BASE)             /**< LCD base pointer */
#define BURTC        ((BURTC_TypeDef *) BURTC_BASE)         /**< BURTC base pointer */
#define WDOG         ((WDOG_TypeDef *) WDOG_BASE)           /**< WDOG base pointer */
#define ETM          ((ETM_TypeDef *) ETM_BASE)             /**< ETM base pointer */
#define CALIBRATE    ((CALIBRATE_TypeDef *) CALIBRATE_BASE) /**< CALIBRATE base pointer */
#define DEVINFO      ((DEVINFO_TypeDef *) DEVINFO_BASE)     /**< DEVINFO base pointer */
#define ROMTABLE     ((ROMTABLE_TypeDef *) ROMTABLE_BASE)   /**< ROMTABLE base pointer */

/** @} End of group EFM32WG942F64_Peripheral_Declaration */

/**************************************************************************//**
 * @defgroup EFM32WG942F64_BitFields EFM32WG942F64 Bit Fields
 * @{
 *****************************************************************************/

/**************************************************************************//**
 * @addtogroup EFM32WG942F64_PRS_Signals
 * @{
 * @brief PRS Signal names
 *****************************************************************************/
#define PRS_VCMP_OUT             ((1 << 16) + 0)  /**< PRS Voltage comparator output */
#define PRS_ACMP0_OUT            ((2 << 16) + 0)  /**< PRS Analog comparator output */
#define PRS_ACMP1_OUT            ((3 << 16) + 0)  /**< PRS Analog comparator output */
#define PRS_DAC0_CH0             ((6 << 16) + 0)  /**< PRS DAC ch0 conversion done */
#define PRS_DAC0_CH1             ((6 << 16) + 1)  /**< PRS DAC ch1 conversion done */
#define PRS_ADC0_SINGLE          ((8 << 16) + 0)  /**< PRS ADC single conversion done */
#define PRS_ADC0_SCAN            ((8 << 16) + 1)  /**< PRS ADC scan conversion done */
#define PRS_USART0_IRTX          ((16 << 16) + 0) /**< PRS USART 0 IRDA out */
#define PRS_USART0_TXC           ((16 << 16) + 1) /**< PRS USART 0 TX complete */
#define PRS_USART0_RXDATAV       ((16 << 16) + 2) /**< PRS USART 0 RX Data Valid */
#define PRS_USART1_TXC           ((17 << 16) + 1) /**< PRS USART 1 TX complete */
#define PRS_USART1_RXDATAV       ((17 << 16) + 2) /**< PRS USART 1 RX Data Valid */
#define PRS_USART2_TXC           ((18 << 16) + 1) /**< PRS USART 2 TX complete */
#define PRS_USART2_RXDATAV       ((18 << 16) + 2) /**< PRS USART 2 RX Data Valid */
#define PRS_TIMER0_UF            ((28 << 16) + 0) /**< PRS Timer 0 Underflow */
#define PRS_TIMER0_OF            ((28 << 16) + 1) /**< PRS Timer 0 Overflow */
#define PRS_TIMER0_CC0           ((28 << 16) + 2) /**< PRS Timer 0 Compare/Capture 0 */
#define PRS_TIMER0_CC1           ((28 << 16) + 3) /**< PRS Timer 0 Compare/Capture 1 */
#define PRS_TIMER0_CC2           ((28 << 16) + 4) /**< PRS Timer 0 Compare/Capture 2 */
#define PRS_TIMER1_UF            ((29 << 16) + 0) /**< PRS Timer 1 Underflow */
#define PRS_TIMER1_OF            ((29 << 16) + 1) /**< PRS Timer 1 Overflow */
#define PRS_TIMER1_CC0           ((29 << 16) + 2) /**< PRS Timer 1 Compare/Capture 0 */
#define PRS_TIMER1_CC1           ((29 << 16) + 3) /**< PRS Timer 1 Compare/Capture 1 */
#define PRS_TIMER1_CC2           ((29 << 16) + 4) /**< PRS Timer 1 Compare/Capture 2 */
#define PRS_TIMER2_UF            ((30 << 16) + 0) /**< PRS Timer 2 Underflow */
#define PRS_TIMER2_OF            ((30 << 16) + 1) /**< PRS Timer 2 Overflow */
#define PRS_TIMER2_CC0           ((30 << 16) + 2) /**< PRS Timer 2 Compare/Capture 0 */
#define PRS_TIMER2_CC1           ((30 << 16) + 3) /**< PRS Timer 2 Compare/Capture 1 */
#define PRS_TIMER2_CC2           ((30 << 16) + 4) /**< PRS Timer 2 Compare/Capture 2 */
#define PRS_TIMER3_UF            ((31 << 16) + 0) /**< PRS Timer 3 Underflow */
#define PRS_TIMER3_OF            ((31 << 16) + 1) /**< PRS Timer 3 Overflow */
#define PRS_TIMER3_CC0           ((31 << 16) + 2) /**< PRS Timer 3 Compare/Capture 0 */
#define PRS_TIMER3_CC1           ((31 << 16) + 3) /**< PRS Timer 3 Compare/Capture 1 */
#define PRS_TIMER3_CC2           ((31 << 16) + 4) /**< PRS Timer 3 Compare/Capture 2 */
#define PRS_USB_SOF              ((36 << 16) + 0) /**< PRS USB Start of Frame */
#define PRS_USB_SOFSR            ((36 << 16) + 1) /**< PRS USB Start of Frame Sent/Received */
#define PRS_RTC_OF               ((40 << 16) + 0) /**< PRS RTC Overflow */
#define PRS_RTC_COMP0            ((40 << 16) + 1) /**< PRS RTC Compare 0 */
#define PRS_RTC_COMP1            ((40 << 16) + 2) /**< PRS RTC Compare 1 */
#define PRS_GPIO_PIN0            ((48 << 16) + 0) /**< PRS GPIO pin 0 */
#define PRS_GPIO_PIN1            ((48 << 16) + 1) /**< PRS GPIO pin 1 */
#define PRS_GPIO_PIN2            ((48 << 16) + 2) /**< PRS GPIO pin 2 */
#define PRS_GPIO_PIN3            ((48 << 16) + 3) /**< PRS GPIO pin 3 */
#define PRS_GPIO_PIN4            ((48 << 16) + 4) /**< PRS GPIO pin 4 */
#define PRS_GPIO_PIN5            ((48 << 16) + 5) /**< PRS GPIO pin 5 */
#define PRS_GPIO_PIN6            ((48 << 16) + 6) /**< PRS GPIO pin 6 */
#define PRS_GPIO_PIN7            ((48 << 16) + 7) /**< PRS GPIO pin 7 */
#define PRS_GPIO_PIN8            ((49 << 16) + 0) /**< PRS GPIO pin 8 */
#define PRS_GPIO_PIN9            ((49 << 16) + 1) /**< PRS GPIO pin 9 */
#define PRS_GPIO_PIN10           ((49 << 16) + 2) /**< PRS GPIO pin 10 */
#define PRS_GPIO_PIN11           ((49 << 16) + 3) /**< PRS GPIO pin 11 */
#define PRS_GPIO_PIN12           ((49 << 16) + 4) /**< PRS GPIO pin 12 */
#define PRS_GPIO_PIN13           ((49 << 16) + 5) /**< PRS GPIO pin 13 */
#define PRS_GPIO_PIN14           ((49 << 16) + 6) /**< PRS GPIO pin 14 */
#define PRS_GPIO_PIN15           ((49 << 16) + 7) /**< PRS GPIO pin 15 */
#define PRS_LETIMER0_CH0         ((52 << 16) + 0) /**< PRS LETIMER CH0 Out */
#define PRS_LETIMER0_CH1         ((52 << 16) + 1) /**< PRS LETIMER CH1 Out */
#define PRS_BURTC_OF             ((55 << 16) + 0) /**< PRS BURTC Overflow */
#define PRS_BURTC_COMP0          ((55 << 16) + 1) /**< PRS BURTC Compare 0 */
#define PRS_LESENSE_SCANRES0     ((57 << 16) + 0) /**< PRS LESENSE SCANRES register, bit 0 */
#define PRS_LESENSE_SCANRES1     ((57 << 16) + 1) /**< PRS LESENSE SCANRES register, bit 1 */
#define PRS_LESENSE_SCANRES2     ((57 << 16) + 2) /**< PRS LESENSE SCANRES register, bit 2 */
#define PRS_LESENSE_SCANRES3     ((57 << 16) + 3) /**< PRS LESENSE SCANRES register, bit 3 */
#define PRS_LESENSE_SCANRES4     ((57 << 16) + 4) /**< PRS LESENSE SCANRES register, bit 4 */
#define PRS_LESENSE_SCANRES5     ((57 << 16) + 5) /**< PRS LESENSE SCANRES register, bit 5 */
#define PRS_LESENSE_SCANRES6     ((57 << 16) + 6) /**< PRS LESENSE SCANRES register, bit 6 */
#define PRS_LESENSE_SCANRES7     ((57 << 16) + 7) /**< PRS LESENSE SCANRES register, bit 7 */
#define PRS_LESENSE_SCANRES8     ((58 << 16) + 0) /**< PRS LESENSE SCANRES register, bit 8 */
#define PRS_LESENSE_SCANRES9     ((58 << 16) + 1) /**< PRS LESENSE SCANRES register, bit 9 */
#define PRS_LESENSE_SCANRES10    ((58 << 16) + 2) /**< PRS LESENSE SCANRES register, bit 10 */
#define PRS_LESENSE_SCANRES11    ((58 << 16) + 3) /**< PRS LESENSE SCANRES register, bit 11 */
#define PRS_LESENSE_SCANRES12    ((58 << 16) + 4) /**< PRS LESENSE SCANRES register, bit 12 */
#define PRS_LESENSE_SCANRES13    ((58 << 16) + 5) /**< PRS LESENSE SCANRES register, bit 13 */
#define PRS_LESENSE_SCANRES14    ((58 << 16) + 6) /**< PRS LESENSE SCANRES register, bit 14 */
#define PRS_LESENSE_SCANRES15    ((58 << 16) + 7) /**< PRS LESENSE SCANRES register, bit 15 */
#define PRS_LESENSE_DEC0         ((59 << 16) + 0) /**< PRS LESENSE Decoder PRS out 0 */
#define PRS_LESENSE_DEC1         ((59 << 16) + 1) /**< PRS LESENSE Decoder PRS out 1 */
#define PRS_LESENSE_DEC2         ((59 << 16) + 2) /**< PRS LESENSE Decoder PRS out 2 */

/** @} End of group EFM32WG942F64_PRS */

#include "efm32wg_dmareq.h"
#include "efm32wg_dmactrl.h"

/**************************************************************************//**
 * @defgroup EFM32WG942F64_DMA_BitFields  EFM32WG942F64_DMA Bit Fields
 * @{
 *****************************************************************************/

/* Bit fields for DMA STATUS */
#define _DMA_STATUS_RESETVALUE                          0x100B0000UL                          /**< Default value for DMA_STATUS */
#define _DMA_STATUS_MASK                                0x001F00F1UL                          /**< Mask for DMA_STATUS */
#define DMA_STATUS_EN                                   (0x1UL << 0)                          /**< DMA Enable Status */
#define _DMA_STATUS_EN_SHIFT                            0                                     /**< Shift value for DMA_EN */
#define _DMA_STATUS_EN_MASK                             0x1UL                                 /**< Bit mask for DMA_EN */
#define _DMA_STATUS_EN_DEFAULT                          0x00000000UL                          /**< Mode DEFAULT for DMA_STATUS */
#define DMA_STATUS_EN_DEFAULT                           (_DMA_STATUS_EN_DEFAULT << 0)         /**< Shifted mode DEFAULT for DMA_STATUS */
#define _DMA_STATUS_STATE_SHIFT                         4                                     /**< Shift value for DMA_STATE */
#define _DMA_STATUS_STATE_MASK                          0xF0UL                                /**< Bit mask for DMA_STATE */
#define _DMA_STATUS_STATE_DEFAULT                       0x00000000UL                          /**< Mode DEFAULT for DMA_STATUS */
#define _DMA_STATUS_STATE_IDLE                          0x00000000UL                          /**< Mode IDLE for DMA_STATUS */
#define _DMA_STATUS_STATE_RDCHCTRLDATA                  0x00000001UL                          /**< Mode RDCHCTRLDATA for DMA_STATUS */
#define _DMA_STATUS_STATE_RDSRCENDPTR                   0x00000002UL                          /**< Mode RDSRCENDPTR for DMA_STATUS */
#define _DMA_STATUS_STATE_RDDSTENDPTR                   0x00000003UL                          /**< Mode RDDSTENDPTR for DMA_STATUS */
#define _DMA_STATUS_STATE_RDSRCDATA                     0x00000004UL                          /**< Mode RDSRCDATA for DMA_STATUS */
#define _DMA_STATUS_STATE_WRDSTDATA                     0x00000005UL                          /**< Mode WRDSTDATA for DMA_STATUS */
#define _DMA_STATUS_STATE_WAITREQCLR                    0x00000006UL                          /**< Mode WAITREQCLR for DMA_STATUS */
#define _DMA_STATUS_STATE_WRCHCTRLDATA                  0x00000007UL                          /**< Mode WRCHCTRLDATA for DMA_STATUS */
#define _DMA_STATUS_STATE_STALLED                       0x00000008UL                          /**< Mode STALLED for DMA_STATUS */
#define _DMA_STATUS_STATE_DONE                          0x00000009UL                          /**< Mode DONE for DMA_STATUS */
#define _DMA_STATUS_STATE_PERSCATTRANS                  0x0000000AUL                          /**< Mode PERSCATTRANS for DMA_STATUS */
#define DMA_STATUS_STATE_DEFAULT                        (_DMA_STATUS_STATE_DEFAULT << 4)      /**< Shifted mode DEFAULT for DMA_STATUS */
#define DMA_STATUS_STATE_IDLE                           (_DMA_STATUS_STATE_IDLE << 4)         /**< Shifted mode IDLE for DMA_STATUS */
#define DMA_STATUS_STATE_RDCHCTRLDATA                   (_DMA_STATUS_STATE_RDCHCTRLDATA << 4) /**< Shifted mode RDCHCTRLDATA for DMA_STATUS */
#define DMA_STATUS_STATE_RDSRCENDPTR                    (_DMA_STATUS_STATE_RDSRCENDPTR << 4)  /**< Shifted mode RDSRCENDPTR for DMA_STATUS */
#define DMA_STATUS_STATE_RDDSTENDPTR                    (_DMA_STATUS_STATE_RDDSTENDPTR << 4)  /**< Shifted mode RDDSTENDPTR for DMA_STATUS */
#define DMA_STATUS_STATE_RDSRCDATA                      (_DMA_STATUS_STATE_RDSRCDATA << 4)    /**< Shifted mode RDSRCDATA for DMA_STATUS */
#define DMA_STATUS_STATE_WRDSTDATA                      (_DMA_STATUS_STATE_WRDSTDATA << 4)    /**< Shifted mode WRDSTDATA for DMA_STATUS */
#define DMA_STATUS_STATE_WAITREQCLR                     (_DMA_STATUS_STATE_WAITREQCLR << 4)   /**< Shifted mode WAITREQCLR for DMA_STATUS */
#define DMA_STATUS_STATE_WRCHCTRLDATA                   (_DMA_STATUS_STATE_WRCHCTRLDATA << 4) /**< Shifted mode WRCHCTRLDATA for DMA_STATUS */
#define DMA_STATUS_STATE_STALLED                        (_DMA_STATUS_STATE_STALLED << 4)      /**< Shifted mode STALLED for DMA_STATUS */
#define DMA_STATUS_STATE_DONE                           (_DMA_STATUS_STATE_DONE << 4)         /**< Shifted mode DONE for DMA_STATUS */
#define DMA_STATUS_STATE_PERSCATTRANS                   (_DMA_STATUS_STATE_PERSCATTRANS << 4) /**< Shifted mode PERSCATTRANS for DMA_STATUS */
#define _DMA_STATUS_CHNUM_SHIFT                         16                                    /**< Shift value for DMA_CHNUM */
#define _DMA_STATUS_CHNUM_MASK                          0x1F0000UL                            /**< Bit mask for DMA_CHNUM */
#define _DMA_STATUS_CHNUM_DEFAULT                       0x0000000BUL                          /**< Mode DEFAULT for DMA_STATUS */
#define DMA_STATUS_CHNUM_DEFAULT                        (_DMA_STATUS_CHNUM_DEFAULT << 16)     /**< Shifted mode DEFAULT for DMA_STATUS */

/* Bit fields for DMA CONFIG */
#define _DMA_CONFIG_RESETVALUE                          0x00000000UL                      /**< Default value for DMA_CONFIG */
#define _DMA_CONFIG_MASK                                0x00000021UL                      /**< Mask for DMA_CONFIG */
#define DMA_CONFIG_EN                                   (0x1UL << 0)                      /**< Enable DMA */
#define _DMA_CONFIG_EN_SHIFT                            0                                 /**< Shift value for DMA_EN */
#define _DMA_CONFIG_EN_MASK                             0x1UL                             /**< Bit mask for DMA_EN */
#define _DMA_CONFIG_EN_DEFAULT                          0x00000000UL                      /**< Mode DEFAULT for DMA_CONFIG */
#define DMA_CONFIG_EN_DEFAULT                           (_DMA_CONFIG_EN_DEFAULT << 0)     /**< Shifted mode DEFAULT for DMA_CONFIG */
#define DMA_CONFIG_CHPROT                               (0x1UL << 5)                      /**< Channel Protection Control */
#define _DMA_CONFIG_CHPROT_SHIFT                        5                                 /**< Shift value for DMA_CHPROT */
#define _DMA_CONFIG_CHPROT_MASK                         0x20UL                            /**< Bit mask for DMA_CHPROT */
#define _DMA_CONFIG_CHPROT_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for DMA_CONFIG */
#define DMA_CONFIG_CHPROT_DEFAULT                       (_DMA_CONFIG_CHPROT_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CONFIG */

/* Bit fields for DMA CTRLBASE */
#define _DMA_CTRLBASE_RESETVALUE                        0x00000000UL                          /**< Default value for DMA_CTRLBASE */
#define _DMA_CTRLBASE_MASK                              0xFFFFFFFFUL                          /**< Mask for DMA_CTRLBASE */
#define _DMA_CTRLBASE_CTRLBASE_SHIFT                    0                                     /**< Shift value for DMA_CTRLBASE */
#define _DMA_CTRLBASE_CTRLBASE_MASK                     0xFFFFFFFFUL                          /**< Bit mask for DMA_CTRLBASE */
#define _DMA_CTRLBASE_CTRLBASE_DEFAULT                  0x00000000UL                          /**< Mode DEFAULT for DMA_CTRLBASE */
#define DMA_CTRLBASE_CTRLBASE_DEFAULT                   (_DMA_CTRLBASE_CTRLBASE_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CTRLBASE */

/* Bit fields for DMA ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_RESETVALUE                     0x00000100UL                                /**< Default value for DMA_ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_MASK                           0xFFFFFFFFUL                                /**< Mask for DMA_ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_ALTCTRLBASE_SHIFT              0                                           /**< Shift value for DMA_ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_ALTCTRLBASE_MASK               0xFFFFFFFFUL                                /**< Bit mask for DMA_ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_ALTCTRLBASE_DEFAULT            0x00000100UL                                /**< Mode DEFAULT for DMA_ALTCTRLBASE */
#define DMA_ALTCTRLBASE_ALTCTRLBASE_DEFAULT             (_DMA_ALTCTRLBASE_ALTCTRLBASE_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_ALTCTRLBASE */

/* Bit fields for DMA CHWAITSTATUS */
#define _DMA_CHWAITSTATUS_RESETVALUE                    0x00000FFFUL                                     /**< Default value for DMA_CHWAITSTATUS */
#define _DMA_CHWAITSTATUS_MASK                          0x00000FFFUL                                     /**< Mask for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH0WAITSTATUS                  (0x1UL << 0)                                     /**< Channel 0 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH0WAITSTATUS_SHIFT           0                                                /**< Shift value for DMA_CH0WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH0WAITSTATUS_MASK            0x1UL                                            /**< Bit mask for DMA_CH0WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH0WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH0WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH0WAITSTATUS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH1WAITSTATUS                  (0x1UL << 1)                                     /**< Channel 1 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH1WAITSTATUS_SHIFT           1                                                /**< Shift value for DMA_CH1WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH1WAITSTATUS_MASK            0x2UL                                            /**< Bit mask for DMA_CH1WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH1WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH1WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH1WAITSTATUS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH2WAITSTATUS                  (0x1UL << 2)                                     /**< Channel 2 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH2WAITSTATUS_SHIFT           2                                                /**< Shift value for DMA_CH2WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH2WAITSTATUS_MASK            0x4UL                                            /**< Bit mask for DMA_CH2WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH2WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH2WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH2WAITSTATUS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH3WAITSTATUS                  (0x1UL << 3)                                     /**< Channel 3 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH3WAITSTATUS_SHIFT           3                                                /**< Shift value for DMA_CH3WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH3WAITSTATUS_MASK            0x8UL                                            /**< Bit mask for DMA_CH3WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH3WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH3WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH3WAITSTATUS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH4WAITSTATUS                  (0x1UL << 4)                                     /**< Channel 4 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH4WAITSTATUS_SHIFT           4                                                /**< Shift value for DMA_CH4WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH4WAITSTATUS_MASK            0x10UL                                           /**< Bit mask for DMA_CH4WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH4WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH4WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH4WAITSTATUS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH5WAITSTATUS                  (0x1UL << 5)                                     /**< Channel 5 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH5WAITSTATUS_SHIFT           5                                                /**< Shift value for DMA_CH5WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH5WAITSTATUS_MASK            0x20UL                                           /**< Bit mask for DMA_CH5WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH5WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH5WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH5WAITSTATUS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH6WAITSTATUS                  (0x1UL << 6)                                     /**< Channel 6 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH6WAITSTATUS_SHIFT           6                                                /**< Shift value for DMA_CH6WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH6WAITSTATUS_MASK            0x40UL                                           /**< Bit mask for DMA_CH6WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH6WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH6WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH6WAITSTATUS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH7WAITSTATUS                  (0x1UL << 7)                                     /**< Channel 7 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH7WAITSTATUS_SHIFT           7                                                /**< Shift value for DMA_CH7WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH7WAITSTATUS_MASK            0x80UL                                           /**< Bit mask for DMA_CH7WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH7WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH7WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH7WAITSTATUS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH8WAITSTATUS                  (0x1UL << 8)                                     /**< Channel 8 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH8WAITSTATUS_SHIFT           8                                                /**< Shift value for DMA_CH8WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH8WAITSTATUS_MASK            0x100UL                                          /**< Bit mask for DMA_CH8WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH8WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH8WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH8WAITSTATUS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH9WAITSTATUS                  (0x1UL << 9)                                     /**< Channel 9 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH9WAITSTATUS_SHIFT           9                                                /**< Shift value for DMA_CH9WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH9WAITSTATUS_MASK            0x200UL                                          /**< Bit mask for DMA_CH9WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH9WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH9WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH9WAITSTATUS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH10WAITSTATUS                 (0x1UL << 10)                                    /**< Channel 10 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH10WAITSTATUS_SHIFT          10                                               /**< Shift value for DMA_CH10WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH10WAITSTATUS_MASK           0x400UL                                          /**< Bit mask for DMA_CH10WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH10WAITSTATUS_DEFAULT        0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH10WAITSTATUS_DEFAULT         (_DMA_CHWAITSTATUS_CH10WAITSTATUS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH11WAITSTATUS                 (0x1UL << 11)                                    /**< Channel 11 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH11WAITSTATUS_SHIFT          11                                               /**< Shift value for DMA_CH11WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH11WAITSTATUS_MASK           0x800UL                                          /**< Bit mask for DMA_CH11WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH11WAITSTATUS_DEFAULT        0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH11WAITSTATUS_DEFAULT         (_DMA_CHWAITSTATUS_CH11WAITSTATUS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */

/* Bit fields for DMA CHSWREQ */
#define _DMA_CHSWREQ_RESETVALUE                         0x00000000UL                           /**< Default value for DMA_CHSWREQ */
#define _DMA_CHSWREQ_MASK                               0x00000FFFUL                           /**< Mask for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH0SWREQ                            (0x1UL << 0)                           /**< Channel 0 Software Request */
#define _DMA_CHSWREQ_CH0SWREQ_SHIFT                     0                                      /**< Shift value for DMA_CH0SWREQ */
#define _DMA_CHSWREQ_CH0SWREQ_MASK                      0x1UL                                  /**< Bit mask for DMA_CH0SWREQ */
#define _DMA_CHSWREQ_CH0SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH0SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH0SWREQ_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH1SWREQ                            (0x1UL << 1)                           /**< Channel 1 Software Request */
#define _DMA_CHSWREQ_CH1SWREQ_SHIFT                     1                                      /**< Shift value for DMA_CH1SWREQ */
#define _DMA_CHSWREQ_CH1SWREQ_MASK                      0x2UL                                  /**< Bit mask for DMA_CH1SWREQ */
#define _DMA_CHSWREQ_CH1SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH1SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH1SWREQ_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH2SWREQ                            (0x1UL << 2)                           /**< Channel 2 Software Request */
#define _DMA_CHSWREQ_CH2SWREQ_SHIFT                     2                                      /**< Shift value for DMA_CH2SWREQ */
#define _DMA_CHSWREQ_CH2SWREQ_MASK                      0x4UL                                  /**< Bit mask for DMA_CH2SWREQ */
#define _DMA_CHSWREQ_CH2SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH2SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH2SWREQ_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH3SWREQ                            (0x1UL << 3)                           /**< Channel 3 Software Request */
#define _DMA_CHSWREQ_CH3SWREQ_SHIFT                     3                                      /**< Shift value for DMA_CH3SWREQ */
#define _DMA_CHSWREQ_CH3SWREQ_MASK                      0x8UL                                  /**< Bit mask for DMA_CH3SWREQ */
#define _DMA_CHSWREQ_CH3SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH3SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH3SWREQ_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH4SWREQ                            (0x1UL << 4)                           /**< Channel 4 Software Request */
#define _DMA_CHSWREQ_CH4SWREQ_SHIFT                     4                                      /**< Shift value for DMA_CH4SWREQ */
#define _DMA_CHSWREQ_CH4SWREQ_MASK                      0x10UL                                 /**< Bit mask for DMA_CH4SWREQ */
#define _DMA_CHSWREQ_CH4SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH4SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH4SWREQ_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH5SWREQ                            (0x1UL << 5)                           /**< Channel 5 Software Request */
#define _DMA_CHSWREQ_CH5SWREQ_SHIFT                     5                                      /**< Shift value for DMA_CH5SWREQ */
#define _DMA_CHSWREQ_CH5SWREQ_MASK                      0x20UL                                 /**< Bit mask for DMA_CH5SWREQ */
#define _DMA_CHSWREQ_CH5SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH5SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH5SWREQ_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH6SWREQ                            (0x1UL << 6)                           /**< Channel 6 Software Request */
#define _DMA_CHSWREQ_CH6SWREQ_SHIFT                     6                                      /**< Shift value for DMA_CH6SWREQ */
#define _DMA_CHSWREQ_CH6SWREQ_MASK                      0x40UL                                 /**< Bit mask for DMA_CH6SWREQ */
#define _DMA_CHSWREQ_CH6SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH6SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH6SWREQ_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH7SWREQ                            (0x1UL << 7)                           /**< Channel 7 Software Request */
#define _DMA_CHSWREQ_CH7SWREQ_SHIFT                     7                                      /**< Shift value for DMA_CH7SWREQ */
#define _DMA_CHSWREQ_CH7SWREQ_MASK                      0x80UL                                 /**< Bit mask for DMA_CH7SWREQ */
#define _DMA_CHSWREQ_CH7SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH7SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH7SWREQ_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH8SWREQ                            (0x1UL << 8)                           /**< Channel 8 Software Request */
#define _DMA_CHSWREQ_CH8SWREQ_SHIFT                     8                                      /**< Shift value for DMA_CH8SWREQ */
#define _DMA_CHSWREQ_CH8SWREQ_MASK                      0x100UL                                /**< Bit mask for DMA_CH8SWREQ */
#define _DMA_CHSWREQ_CH8SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH8SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH8SWREQ_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH9SWREQ                            (0x1UL << 9)                           /**< Channel 9 Software Request */
#define _DMA_CHSWREQ_CH9SWREQ_SHIFT                     9                                      /**< Shift value for DMA_CH9SWREQ */
#define _DMA_CHSWREQ_CH9SWREQ_MASK                      0x200UL                                /**< Bit mask for DMA_CH9SWREQ */
#define _DMA_CHSWREQ_CH9SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH9SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH9SWREQ_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH10SWREQ                           (0x1UL << 10)                          /**< Channel 10 Software Request */
#define _DMA_CHSWREQ_CH10SWREQ_SHIFT                    10                                     /**< Shift value for DMA_CH10SWREQ */
#define _DMA_CHSWREQ_CH10SWREQ_MASK                     0x400UL                                /**< Bit mask for DMA_CH10SWREQ */
#define _DMA_CHSWREQ_CH10SWREQ_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH10SWREQ_DEFAULT                   (_DMA_CHSWREQ_CH10SWREQ_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH11SWREQ                           (0x1UL << 11)                          /**< Channel 11 Software Request */
#define _DMA_CHSWREQ_CH11SWREQ_SHIFT                    11                                     /**< Shift value for DMA_CH11SWREQ */
#define _DMA_CHSWREQ_CH11SWREQ_MASK                     0x800UL                                /**< Bit mask for DMA_CH11SWREQ */
#define _DMA_CHSWREQ_CH11SWREQ_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH11SWREQ_DEFAULT                   (_DMA_CHSWREQ_CH11SWREQ_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHSWREQ */

/* Bit fields for DMA CHUSEBURSTS */
#define _DMA_CHUSEBURSTS_RESETVALUE                     0x00000000UL                                        /**< Default value for DMA_CHUSEBURSTS */
#define _DMA_CHUSEBURSTS_MASK                           0x00000FFFUL                                        /**< Mask for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH0USEBURSTS                    (0x1UL << 0)                                        /**< Channel 0 Useburst Set */
#define _DMA_CHUSEBURSTS_CH0USEBURSTS_SHIFT             0                                                   /**< Shift value for DMA_CH0USEBURSTS */
#define _DMA_CHUSEBURSTS_CH0USEBURSTS_MASK              0x1UL                                               /**< Bit mask for DMA_CH0USEBURSTS */
#define _DMA_CHUSEBURSTS_CH0USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define _DMA_CHUSEBURSTS_CH0USEBURSTS_SINGLEANDBURST    0x00000000UL                                        /**< Mode SINGLEANDBURST for DMA_CHUSEBURSTS */
#define _DMA_CHUSEBURSTS_CH0USEBURSTS_BURSTONLY         0x00000001UL                                        /**< Mode BURSTONLY for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH0USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH0USEBURSTS_DEFAULT << 0)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH0USEBURSTS_SINGLEANDBURST     (_DMA_CHUSEBURSTS_CH0USEBURSTS_SINGLEANDBURST << 0) /**< Shifted mode SINGLEANDBURST for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH0USEBURSTS_BURSTONLY          (_DMA_CHUSEBURSTS_CH0USEBURSTS_BURSTONLY << 0)      /**< Shifted mode BURSTONLY for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH1USEBURSTS                    (0x1UL << 1)                                        /**< Channel 1 Useburst Set */
#define _DMA_CHUSEBURSTS_CH1USEBURSTS_SHIFT             1                                                   /**< Shift value for DMA_CH1USEBURSTS */
#define _DMA_CHUSEBURSTS_CH1USEBURSTS_MASK              0x2UL                                               /**< Bit mask for DMA_CH1USEBURSTS */
#define _DMA_CHUSEBURSTS_CH1USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH1USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH1USEBURSTS_DEFAULT << 1)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH2USEBURSTS                    (0x1UL << 2)                                        /**< Channel 2 Useburst Set */
#define _DMA_CHUSEBURSTS_CH2USEBURSTS_SHIFT             2                                                   /**< Shift value for DMA_CH2USEBURSTS */
#define _DMA_CHUSEBURSTS_CH2USEBURSTS_MASK              0x4UL                                               /**< Bit mask for DMA_CH2USEBURSTS */
#define _DMA_CHUSEBURSTS_CH2USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH2USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH2USEBURSTS_DEFAULT << 2)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH3USEBURSTS                    (0x1UL << 3)                                        /**< Channel 3 Useburst Set */
#define _DMA_CHUSEBURSTS_CH3USEBURSTS_SHIFT             3                                                   /**< Shift value for DMA_CH3USEBURSTS */
#define _DMA_CHUSEBURSTS_CH3USEBURSTS_MASK              0x8UL                                               /**< Bit mask for DMA_CH3USEBURSTS */
#define _DMA_CHUSEBURSTS_CH3USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH3USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH3USEBURSTS_DEFAULT << 3)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH4USEBURSTS                    (0x1UL << 4)                                        /**< Channel 4 Useburst Set */
#define _DMA_CHUSEBURSTS_CH4USEBURSTS_SHIFT             4                                                   /**< Shift value for DMA_CH4USEBURSTS */
#define _DMA_CHUSEBURSTS_CH4USEBURSTS_MASK              0x10UL                                              /**< Bit mask for DMA_CH4USEBURSTS */
#define _DMA_CHUSEBURSTS_CH4USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH4USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH4USEBURSTS_DEFAULT << 4)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH5USEBURSTS                    (0x1UL << 5)                                        /**< Channel 5 Useburst Set */
#define _DMA_CHUSEBURSTS_CH5USEBURSTS_SHIFT             5                                                   /**< Shift value for DMA_CH5USEBURSTS */
#define _DMA_CHUSEBURSTS_CH5USEBURSTS_MASK              0x20UL                                              /**< Bit mask for DMA_CH5USEBURSTS */
#define _DMA_CHUSEBURSTS_CH5USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH5USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH5USEBURSTS_DEFAULT << 5)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH6USEBURSTS                    (0x1UL << 6)                                        /**< Channel 6 Useburst Set */
#define _DMA_CHUSEBURSTS_CH6USEBURSTS_SHIFT             6                                                   /**< Shift value for DMA_CH6USEBURSTS */
#define _DMA_CHUSEBURSTS_CH6USEBURSTS_MASK              0x40UL                                              /**< Bit mask for DMA_CH6USEBURSTS */
#define _DMA_CHUSEBURSTS_CH6USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH6USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH6USEBURSTS_DEFAULT << 6)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH7USEBURSTS                    (0x1UL << 7)                                        /**< Channel 7 Useburst Set */
#define _DMA_CHUSEBURSTS_CH7USEBURSTS_SHIFT             7                                                   /**< Shift value for DMA_CH7USEBURSTS */
#define _DMA_CHUSEBURSTS_CH7USEBURSTS_MASK              0x80UL                                              /**< Bit mask for DMA_CH7USEBURSTS */
#define _DMA_CHUSEBURSTS_CH7USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH7USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH7USEBURSTS_DEFAULT << 7)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH8USEBURSTS                    (0x1UL << 8)                                        /**< Channel 8 Useburst Set */
#define _DMA_CHUSEBURSTS_CH8USEBURSTS_SHIFT             8                                                   /**< Shift value for DMA_CH8USEBURSTS */
#define _DMA_CHUSEBURSTS_CH8USEBURSTS_MASK              0x100UL                                             /**< Bit mask for DMA_CH8USEBURSTS */
#define _DMA_CHUSEBURSTS_CH8USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH8USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH8USEBURSTS_DEFAULT << 8)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH9USEBURSTS                    (0x1UL << 9)                                        /**< Channel 9 Useburst Set */
#define _DMA_CHUSEBURSTS_CH9USEBURSTS_SHIFT             9                                                   /**< Shift value for DMA_CH9USEBURSTS */
#define _DMA_CHUSEBURSTS_CH9USEBURSTS_MASK              0x200UL                                             /**< Bit mask for DMA_CH9USEBURSTS */
#define _DMA_CHUSEBURSTS_CH9USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH9USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH9USEBURSTS_DEFAULT << 9)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH10USEBURSTS                   (0x1UL << 10)                                       /**< Channel 10 Useburst Set */
#define _DMA_CHUSEBURSTS_CH10USEBURSTS_SHIFT            10                                                  /**< Shift value for DMA_CH10USEBURSTS */
#define _DMA_CHUSEBURSTS_CH10USEBURSTS_MASK             0x400UL                                             /**< Bit mask for DMA_CH10USEBURSTS */
#define _DMA_CHUSEBURSTS_CH10USEBURSTS_DEFAULT          0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH10USEBURSTS_DEFAULT           (_DMA_CHUSEBURSTS_CH10USEBURSTS_DEFAULT << 10)      /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH11USEBURSTS                   (0x1UL << 11)                                       /**< Channel 11 Useburst Set */
#define _DMA_CHUSEBURSTS_CH11USEBURSTS_SHIFT            11                                                  /**< Shift value for DMA_CH11USEBURSTS */
#define _DMA_CHUSEBURSTS_CH11USEBURSTS_MASK             0x800UL                                             /**< Bit mask for DMA_CH11USEBURSTS */
#define _DMA_CHUSEBURSTS_CH11USEBURSTS_DEFAULT          0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH11USEBURSTS_DEFAULT           (_DMA_CHUSEBURSTS_CH11USEBURSTS_DEFAULT << 11)      /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */

/* Bit fields for DMA CHUSEBURSTC */
#define _DMA_CHUSEBURSTC_RESETVALUE                     0x00000000UL                                   /**< Default value for DMA_CHUSEBURSTC */
#define _DMA_CHUSEBURSTC_MASK                           0x00000FFFUL                                   /**< Mask for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH0USEBURSTC                    (0x1UL << 0)                                   /**< Channel 0 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH0USEBURSTC_SHIFT             0                                              /**< Shift value for DMA_CH0USEBURSTC */
#define _DMA_CHUSEBURSTC_CH0USEBURSTC_MASK              0x1UL                                          /**< Bit mask for DMA_CH0USEBURSTC */
#define _DMA_CHUSEBURSTC_CH0USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH0USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH0USEBURSTC_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH1USEBURSTC                    (0x1UL << 1)                                   /**< Channel 1 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH1USEBURSTC_SHIFT             1                                              /**< Shift value for DMA_CH1USEBURSTC */
#define _DMA_CHUSEBURSTC_CH1USEBURSTC_MASK              0x2UL                                          /**< Bit mask for DMA_CH1USEBURSTC */
#define _DMA_CHUSEBURSTC_CH1USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH1USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH1USEBURSTC_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH2USEBURSTC                    (0x1UL << 2)                                   /**< Channel 2 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH2USEBURSTC_SHIFT             2                                              /**< Shift value for DMA_CH2USEBURSTC */
#define _DMA_CHUSEBURSTC_CH2USEBURSTC_MASK              0x4UL                                          /**< Bit mask for DMA_CH2USEBURSTC */
#define _DMA_CHUSEBURSTC_CH2USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH2USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH2USEBURSTC_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH3USEBURSTC                    (0x1UL << 3)                                   /**< Channel 3 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH3USEBURSTC_SHIFT             3                                              /**< Shift value for DMA_CH3USEBURSTC */
#define _DMA_CHUSEBURSTC_CH3USEBURSTC_MASK              0x8UL                                          /**< Bit mask for DMA_CH3USEBURSTC */
#define _DMA_CHUSEBURSTC_CH3USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH3USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH3USEBURSTC_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH4USEBURSTC                    (0x1UL << 4)                                   /**< Channel 4 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH4USEBURSTC_SHIFT             4                                              /**< Shift value for DMA_CH4USEBURSTC */
#define _DMA_CHUSEBURSTC_CH4USEBURSTC_MASK              0x10UL                                         /**< Bit mask for DMA_CH4USEBURSTC */
#define _DMA_CHUSEBURSTC_CH4USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH4USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH4USEBURSTC_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH5USEBURSTC                    (0x1UL << 5)                                   /**< Channel 5 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH5USEBURSTC_SHIFT             5                                              /**< Shift value for DMA_CH5USEBURSTC */
#define _DMA_CHUSEBURSTC_CH5USEBURSTC_MASK              0x20UL                                         /**< Bit mask for DMA_CH5USEBURSTC */
#define _DMA_CHUSEBURSTC_CH5USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH5USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH5USEBURSTC_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH6USEBURSTC                    (0x1UL << 6)                                   /**< Channel 6 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH6USEBURSTC_SHIFT             6                                              /**< Shift value for DMA_CH6USEBURSTC */
#define _DMA_CHUSEBURSTC_CH6USEBURSTC_MASK              0x40UL                                         /**< Bit mask for DMA_CH6USEBURSTC */
#define _DMA_CHUSEBURSTC_CH6USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH6USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH6USEBURSTC_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH7USEBURSTC                    (0x1UL << 7)                                   /**< Channel 7 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH7USEBURSTC_SHIFT             7                                              /**< Shift value for DMA_CH7USEBURSTC */
#define _DMA_CHUSEBURSTC_CH7USEBURSTC_MASK              0x80UL                                         /**< Bit mask for DMA_CH7USEBURSTC */
#define _DMA_CHUSEBURSTC_CH7USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH7USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH7USEBURSTC_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH08USEBURSTC                   (0x1UL << 8)                                   /**< Channel 8 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH08USEBURSTC_SHIFT            8                                              /**< Shift value for DMA_CH08USEBURSTC */
#define _DMA_CHUSEBURSTC_CH08USEBURSTC_MASK             0x100UL                                        /**< Bit mask for DMA_CH08USEBURSTC */
#define _DMA_CHUSEBURSTC_CH08USEBURSTC_DEFAULT          0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH08USEBURSTC_DEFAULT           (_DMA_CHUSEBURSTC_CH08USEBURSTC_DEFAULT << 8)  /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH9USEBURSTC                    (0x1UL << 9)                                   /**< Channel 9 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH9USEBURSTC_SHIFT             9                                              /**< Shift value for DMA_CH9USEBURSTC */
#define _DMA_CHUSEBURSTC_CH9USEBURSTC_MASK              0x200UL                                        /**< Bit mask for DMA_CH9USEBURSTC */
#define _DMA_CHUSEBURSTC_CH9USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH9USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH9USEBURSTC_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH10USEBURSTC                   (0x1UL << 10)                                  /**< Channel 10 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH10USEBURSTC_SHIFT            10                                             /**< Shift value for DMA_CH10USEBURSTC */
#define _DMA_CHUSEBURSTC_CH10USEBURSTC_MASK             0x400UL                                        /**< Bit mask for DMA_CH10USEBURSTC */
#define _DMA_CHUSEBURSTC_CH10USEBURSTC_DEFAULT          0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH10USEBURSTC_DEFAULT           (_DMA_CHUSEBURSTC_CH10USEBURSTC_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH11USEBURSTC                   (0x1UL << 11)                                  /**< Channel 11 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH11USEBURSTC_SHIFT            11                                             /**< Shift value for DMA_CH11USEBURSTC */
#define _DMA_CHUSEBURSTC_CH11USEBURSTC_MASK             0x800UL                                        /**< Bit mask for DMA_CH11USEBURSTC */
#define _DMA_CHUSEBURSTC_CH11USEBURSTC_DEFAULT          0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH11USEBURSTC_DEFAULT           (_DMA_CHUSEBURSTC_CH11USEBURSTC_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */

/* Bit fields for DMA CHREQMASKS */
#define _DMA_CHREQMASKS_RESETVALUE                      0x00000000UL                                 /**< Default value for DMA_CHREQMASKS */
#define _DMA_CHREQMASKS_MASK                            0x00000FFFUL                                 /**< Mask for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH0REQMASKS                      (0x1UL << 0)                                 /**< Channel 0 Request Mask Set */
#define _DMA_CHREQMASKS_CH0REQMASKS_SHIFT               0                                            /**< Shift value for DMA_CH0REQMASKS */
#define _DMA_CHREQMASKS_CH0REQMASKS_MASK                0x1UL                                        /**< Bit mask for DMA_CH0REQMASKS */
#define _DMA_CHREQMASKS_CH0REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH0REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH0REQMASKS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH1REQMASKS                      (0x1UL << 1)                                 /**< Channel 1 Request Mask Set */
#define _DMA_CHREQMASKS_CH1REQMASKS_SHIFT               1                                            /**< Shift value for DMA_CH1REQMASKS */
#define _DMA_CHREQMASKS_CH1REQMASKS_MASK                0x2UL                                        /**< Bit mask for DMA_CH1REQMASKS */
#define _DMA_CHREQMASKS_CH1REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH1REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH1REQMASKS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH2REQMASKS                      (0x1UL << 2)                                 /**< Channel 2 Request Mask Set */
#define _DMA_CHREQMASKS_CH2REQMASKS_SHIFT               2                                            /**< Shift value for DMA_CH2REQMASKS */
#define _DMA_CHREQMASKS_CH2REQMASKS_MASK                0x4UL                                        /**< Bit mask for DMA_CH2REQMASKS */
#define _DMA_CHREQMASKS_CH2REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH2REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH2REQMASKS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH3REQMASKS                      (0x1UL << 3)                                 /**< Channel 3 Request Mask Set */
#define _DMA_CHREQMASKS_CH3REQMASKS_SHIFT               3                                            /**< Shift value for DMA_CH3REQMASKS */
#define _DMA_CHREQMASKS_CH3REQMASKS_MASK                0x8UL                                        /**< Bit mask for DMA_CH3REQMASKS */
#define _DMA_CHREQMASKS_CH3REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH3REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH3REQMASKS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH4REQMASKS                      (0x1UL << 4)                                 /**< Channel 4 Request Mask Set */
#define _DMA_CHREQMASKS_CH4REQMASKS_SHIFT               4                                            /**< Shift value for DMA_CH4REQMASKS */
#define _DMA_CHREQMASKS_CH4REQMASKS_MASK                0x10UL                                       /**< Bit mask for DMA_CH4REQMASKS */
#define _DMA_CHREQMASKS_CH4REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH4REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH4REQMASKS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH5REQMASKS                      (0x1UL << 5)                                 /**< Channel 5 Request Mask Set */
#define _DMA_CHREQMASKS_CH5REQMASKS_SHIFT               5                                            /**< Shift value for DMA_CH5REQMASKS */
#define _DMA_CHREQMASKS_CH5REQMASKS_MASK                0x20UL                                       /**< Bit mask for DMA_CH5REQMASKS */
#define _DMA_CHREQMASKS_CH5REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH5REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH5REQMASKS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH6REQMASKS                      (0x1UL << 6)                                 /**< Channel 6 Request Mask Set */
#define _DMA_CHREQMASKS_CH6REQMASKS_SHIFT               6                                            /**< Shift value for DMA_CH6REQMASKS */
#define _DMA_CHREQMASKS_CH6REQMASKS_MASK                0x40UL                                       /**< Bit mask for DMA_CH6REQMASKS */
#define _DMA_CHREQMASKS_CH6REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH6REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH6REQMASKS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH7REQMASKS                      (0x1UL << 7)                                 /**< Channel 7 Request Mask Set */
#define _DMA_CHREQMASKS_CH7REQMASKS_SHIFT               7                                            /**< Shift value for DMA_CH7REQMASKS */
#define _DMA_CHREQMASKS_CH7REQMASKS_MASK                0x80UL                                       /**< Bit mask for DMA_CH7REQMASKS */
#define _DMA_CHREQMASKS_CH7REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH7REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH7REQMASKS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH8REQMASKS                      (0x1UL << 8)                                 /**< Channel 8 Request Mask Set */
#define _DMA_CHREQMASKS_CH8REQMASKS_SHIFT               8                                            /**< Shift value for DMA_CH8REQMASKS */
#define _DMA_CHREQMASKS_CH8REQMASKS_MASK                0x100UL                                      /**< Bit mask for DMA_CH8REQMASKS */
#define _DMA_CHREQMASKS_CH8REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH8REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH8REQMASKS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH9REQMASKS                      (0x1UL << 9)                                 /**< Channel 9 Request Mask Set */
#define _DMA_CHREQMASKS_CH9REQMASKS_SHIFT               9                                            /**< Shift value for DMA_CH9REQMASKS */
#define _DMA_CHREQMASKS_CH9REQMASKS_MASK                0x200UL                                      /**< Bit mask for DMA_CH9REQMASKS */
#define _DMA_CHREQMASKS_CH9REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH9REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH9REQMASKS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH10REQMASKS                     (0x1UL << 10)                                /**< Channel 10 Request Mask Set */
#define _DMA_CHREQMASKS_CH10REQMASKS_SHIFT              10                                           /**< Shift value for DMA_CH10REQMASKS */
#define _DMA_CHREQMASKS_CH10REQMASKS_MASK               0x400UL                                      /**< Bit mask for DMA_CH10REQMASKS */
#define _DMA_CHREQMASKS_CH10REQMASKS_DEFAULT            0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH10REQMASKS_DEFAULT             (_DMA_CHREQMASKS_CH10REQMASKS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH11REQMASKS                     (0x1UL << 11)                                /**< Channel 11 Request Mask Set */
#define _DMA_CHREQMASKS_CH11REQMASKS_SHIFT              11                                           /**< Shift value for DMA_CH11REQMASKS */
#define _DMA_CHREQMASKS_CH11REQMASKS_MASK               0x800UL                                      /**< Bit mask for DMA_CH11REQMASKS */
#define _DMA_CHREQMASKS_CH11REQMASKS_DEFAULT            0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH11REQMASKS_DEFAULT             (_DMA_CHREQMASKS_CH11REQMASKS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHREQMASKS */

/* Bit fields for DMA CHREQMASKC */
#define _DMA_CHREQMASKC_RESETVALUE                      0x00000000UL                                 /**< Default value for DMA_CHREQMASKC */
#define _DMA_CHREQMASKC_MASK                            0x00000FFFUL                                 /**< Mask for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH0REQMASKC                      (0x1UL << 0)                                 /**< Channel 0 Request Mask Clear */
#define _DMA_CHREQMASKC_CH0REQMASKC_SHIFT               0                                            /**< Shift value for DMA_CH0REQMASKC */
#define _DMA_CHREQMASKC_CH0REQMASKC_MASK                0x1UL                                        /**< Bit mask for DMA_CH0REQMASKC */
#define _DMA_CHREQMASKC_CH0REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH0REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH0REQMASKC_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH1REQMASKC                      (0x1UL << 1)                                 /**< Channel 1 Request Mask Clear */
#define _DMA_CHREQMASKC_CH1REQMASKC_SHIFT               1                                            /**< Shift value for DMA_CH1REQMASKC */
#define _DMA_CHREQMASKC_CH1REQMASKC_MASK                0x2UL                                        /**< Bit mask for DMA_CH1REQMASKC */
#define _DMA_CHREQMASKC_CH1REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH1REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH1REQMASKC_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH2REQMASKC                      (0x1UL << 2)                                 /**< Channel 2 Request Mask Clear */
#define _DMA_CHREQMASKC_CH2REQMASKC_SHIFT               2                                            /**< Shift value for DMA_CH2REQMASKC */
#define _DMA_CHREQMASKC_CH2REQMASKC_MASK                0x4UL                                        /**< Bit mask for DMA_CH2REQMASKC */
#define _DMA_CHREQMASKC_CH2REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH2REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH2REQMASKC_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH3REQMASKC                      (0x1UL << 3)                                 /**< Channel 3 Request Mask Clear */
#define _DMA_CHREQMASKC_CH3REQMASKC_SHIFT               3                                            /**< Shift value for DMA_CH3REQMASKC */
#define _DMA_CHREQMASKC_CH3REQMASKC_MASK                0x8UL                                        /**< Bit mask for DMA_CH3REQMASKC */
#define _DMA_CHREQMASKC_CH3REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH3REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH3REQMASKC_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH4REQMASKC                      (0x1UL << 4)                                 /**< Channel 4 Request Mask Clear */
#define _DMA_CHREQMASKC_CH4REQMASKC_SHIFT               4                                            /**< Shift value for DMA_CH4REQMASKC */
#define _DMA_CHREQMASKC_CH4REQMASKC_MASK                0x10UL                                       /**< Bit mask for DMA_CH4REQMASKC */
#define _DMA_CHREQMASKC_CH4REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH4REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH4REQMASKC_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH5REQMASKC                      (0x1UL << 5)                                 /**< Channel 5 Request Mask Clear */
#define _DMA_CHREQMASKC_CH5REQMASKC_SHIFT               5                                            /**< Shift value for DMA_CH5REQMASKC */
#define _DMA_CHREQMASKC_CH5REQMASKC_MASK                0x20UL                                       /**< Bit mask for DMA_CH5REQMASKC */
#define _DMA_CHREQMASKC_CH5REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH5REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH5REQMASKC_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH6REQMASKC                      (0x1UL << 6)                                 /**< Channel 6 Request Mask Clear */
#define _DMA_CHREQMASKC_CH6REQMASKC_SHIFT               6                                            /**< Shift value for DMA_CH6REQMASKC */
#define _DMA_CHREQMASKC_CH6REQMASKC_MASK                0x40UL                                       /**< Bit mask for DMA_CH6REQMASKC */
#define _DMA_CHREQMASKC_CH6REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH6REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH6REQMASKC_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH7REQMASKC                      (0x1UL << 7)                                 /**< Channel 7 Request Mask Clear */
#define _DMA_CHREQMASKC_CH7REQMASKC_SHIFT               7                                            /**< Shift value for DMA_CH7REQMASKC */
#define _DMA_CHREQMASKC_CH7REQMASKC_MASK                0x80UL                                       /**< Bit mask for DMA_CH7REQMASKC */
#define _DMA_CHREQMASKC_CH7REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH7REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH7REQMASKC_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH8REQMASKC                      (0x1UL << 8)                                 /**< Channel 8 Request Mask Clear */
#define _DMA_CHREQMASKC_CH8REQMASKC_SHIFT               8                                            /**< Shift value for DMA_CH8REQMASKC */
#define _DMA_CHREQMASKC_CH8REQMASKC_MASK                0x100UL                                      /**< Bit mask for DMA_CH8REQMASKC */
#define _DMA_CHREQMASKC_CH8REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH8REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH8REQMASKC_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH9REQMASKC                      (0x1UL << 9)                                 /**< Channel 9 Request Mask Clear */
#define _DMA_CHREQMASKC_CH9REQMASKC_SHIFT               9                                            /**< Shift value for DMA_CH9REQMASKC */
#define _DMA_CHREQMASKC_CH9REQMASKC_MASK                0x200UL                                      /**< Bit mask for DMA_CH9REQMASKC */
#define _DMA_CHREQMASKC_CH9REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH9REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH9REQMASKC_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH10REQMASKC                     (0x1UL << 10)                                /**< Channel 10 Request Mask Clear */
#define _DMA_CHREQMASKC_CH10REQMASKC_SHIFT              10                                           /**< Shift value for DMA_CH10REQMASKC */
#define _DMA_CHREQMASKC_CH10REQMASKC_MASK               0x400UL                                      /**< Bit mask for DMA_CH10REQMASKC */
#define _DMA_CHREQMASKC_CH10REQMASKC_DEFAULT            0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH10REQMASKC_DEFAULT             (_DMA_CHREQMASKC_CH10REQMASKC_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH11REQMASKC                     (0x1UL << 11)                                /**< Channel 11 Request Mask Clear */
#define _DMA_CHREQMASKC_CH11REQMASKC_SHIFT              11                                           /**< Shift value for DMA_CH11REQMASKC */
#define _DMA_CHREQMASKC_CH11REQMASKC_MASK               0x800UL                                      /**< Bit mask for DMA_CH11REQMASKC */
#define _DMA_CHREQMASKC_CH11REQMASKC_DEFAULT            0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH11REQMASKC_DEFAULT             (_DMA_CHREQMASKC_CH11REQMASKC_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHREQMASKC */

/* Bit fields for DMA CHENS */
#define _DMA_CHENS_RESETVALUE                           0x00000000UL                       /**< Default value for DMA_CHENS */
#define _DMA_CHENS_MASK                                 0x00000FFFUL                       /**< Mask for DMA_CHENS */
#define DMA_CHENS_CH0ENS                                (0x1UL << 0)                       /**< Channel 0 Enable Set */
#define _DMA_CHENS_CH0ENS_SHIFT                         0                                  /**< Shift value for DMA_CH0ENS */
#define _DMA_CHENS_CH0ENS_MASK                          0x1UL                              /**< Bit mask for DMA_CH0ENS */
#define _DMA_CHENS_CH0ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH0ENS_DEFAULT                        (_DMA_CHENS_CH0ENS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH1ENS                                (0x1UL << 1)                       /**< Channel 1 Enable Set */
#define _DMA_CHENS_CH1ENS_SHIFT                         1                                  /**< Shift value for DMA_CH1ENS */
#define _DMA_CHENS_CH1ENS_MASK                          0x2UL                              /**< Bit mask for DMA_CH1ENS */
#define _DMA_CHENS_CH1ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH1ENS_DEFAULT                        (_DMA_CHENS_CH1ENS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH2ENS                                (0x1UL << 2)                       /**< Channel 2 Enable Set */
#define _DMA_CHENS_CH2ENS_SHIFT                         2                                  /**< Shift value for DMA_CH2ENS */
#define _DMA_CHENS_CH2ENS_MASK                          0x4UL                              /**< Bit mask for DMA_CH2ENS */
#define _DMA_CHENS_CH2ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH2ENS_DEFAULT                        (_DMA_CHENS_CH2ENS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH3ENS                                (0x1UL << 3)                       /**< Channel 3 Enable Set */
#define _DMA_CHENS_CH3ENS_SHIFT                         3                                  /**< Shift value for DMA_CH3ENS */
#define _DMA_CHENS_CH3ENS_MASK                          0x8UL                              /**< Bit mask for DMA_CH3ENS */
#define _DMA_CHENS_CH3ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH3ENS_DEFAULT                        (_DMA_CHENS_CH3ENS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH4ENS                                (0x1UL << 4)                       /**< Channel 4 Enable Set */
#define _DMA_CHENS_CH4ENS_SHIFT                         4                                  /**< Shift value for DMA_CH4ENS */
#define _DMA_CHENS_CH4ENS_MASK                          0x10UL                             /**< Bit mask for DMA_CH4ENS */
#define _DMA_CHENS_CH4ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH4ENS_DEFAULT                        (_DMA_CHENS_CH4ENS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH5ENS                                (0x1UL << 5)                       /**< Channel 5 Enable Set */
#define _DMA_CHENS_CH5ENS_SHIFT                         5                                  /**< Shift value for DMA_CH5ENS */
#define _DMA_CHENS_CH5ENS_MASK                          0x20UL                             /**< Bit mask for DMA_CH5ENS */
#define _DMA_CHENS_CH5ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH5ENS_DEFAULT                        (_DMA_CHENS_CH5ENS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH6ENS                                (0x1UL << 6)                       /**< Channel 6 Enable Set */
#define _DMA_CHENS_CH6ENS_SHIFT                         6                                  /**< Shift value for DMA_CH6ENS */
#define _DMA_CHENS_CH6ENS_MASK                          0x40UL                             /**< Bit mask for DMA_CH6ENS */
#define _DMA_CHENS_CH6ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH6ENS_DEFAULT                        (_DMA_CHENS_CH6ENS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH7ENS                                (0x1UL << 7)                       /**< Channel 7 Enable Set */
#define _DMA_CHENS_CH7ENS_SHIFT                         7                                  /**< Shift value for DMA_CH7ENS */
#define _DMA_CHENS_CH7ENS_MASK                          0x80UL                             /**< Bit mask for DMA_CH7ENS */
#define _DMA_CHENS_CH7ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH7ENS_DEFAULT                        (_DMA_CHENS_CH7ENS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH8ENS                                (0x1UL << 8)                       /**< Channel 8 Enable Set */
#define _DMA_CHENS_CH8ENS_SHIFT                         8                                  /**< Shift value for DMA_CH8ENS */
#define _DMA_CHENS_CH8ENS_MASK                          0x100UL                            /**< Bit mask for DMA_CH8ENS */
#define _DMA_CHENS_CH8ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH8ENS_DEFAULT                        (_DMA_CHENS_CH8ENS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH9ENS                                (0x1UL << 9)                       /**< Channel 9 Enable Set */
#define _DMA_CHENS_CH9ENS_SHIFT                         9                                  /**< Shift value for DMA_CH9ENS */
#define _DMA_CHENS_CH9ENS_MASK                          0x200UL                            /**< Bit mask for DMA_CH9ENS */
#define _DMA_CHENS_CH9ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH9ENS_DEFAULT                        (_DMA_CHENS_CH9ENS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH10ENS                               (0x1UL << 10)                      /**< Channel 10 Enable Set */
#define _DMA_CHENS_CH10ENS_SHIFT                        10                                 /**< Shift value for DMA_CH10ENS */
#define _DMA_CHENS_CH10ENS_MASK                         0x400UL                            /**< Bit mask for DMA_CH10ENS */
#define _DMA_CHENS_CH10ENS_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH10ENS_DEFAULT                       (_DMA_CHENS_CH10ENS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH11ENS                               (0x1UL << 11)                      /**< Channel 11 Enable Set */
#define _DMA_CHENS_CH11ENS_SHIFT                        11                                 /**< Shift value for DMA_CH11ENS */
#define _DMA_CHENS_CH11ENS_MASK                         0x800UL                            /**< Bit mask for DMA_CH11ENS */
#define _DMA_CHENS_CH11ENS_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH11ENS_DEFAULT                       (_DMA_CHENS_CH11ENS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHENS */

/* Bit fields for DMA CHENC */
#define _DMA_CHENC_RESETVALUE                           0x00000000UL                       /**< Default value for DMA_CHENC */
#define _DMA_CHENC_MASK                                 0x00000FFFUL                       /**< Mask for DMA_CHENC */
#define DMA_CHENC_CH0ENC                                (0x1UL << 0)                       /**< Channel 0 Enable Clear */
#define _DMA_CHENC_CH0ENC_SHIFT                         0                                  /**< Shift value for DMA_CH0ENC */
#define _DMA_CHENC_CH0ENC_MASK                          0x1UL                              /**< Bit mask for DMA_CH0ENC */
#define _DMA_CHENC_CH0ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH0ENC_DEFAULT                        (_DMA_CHENC_CH0ENC_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH1ENC                                (0x1UL << 1)                       /**< Channel 1 Enable Clear */
#define _DMA_CHENC_CH1ENC_SHIFT                         1                                  /**< Shift value for DMA_CH1ENC */
#define _DMA_CHENC_CH1ENC_MASK                          0x2UL                              /**< Bit mask for DMA_CH1ENC */
#define _DMA_CHENC_CH1ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH1ENC_DEFAULT                        (_DMA_CHENC_CH1ENC_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH2ENC                                (0x1UL << 2)                       /**< Channel 2 Enable Clear */
#define _DMA_CHENC_CH2ENC_SHIFT                         2                                  /**< Shift value for DMA_CH2ENC */
#define _DMA_CHENC_CH2ENC_MASK                          0x4UL                              /**< Bit mask for DMA_CH2ENC */
#define _DMA_CHENC_CH2ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH2ENC_DEFAULT                        (_DMA_CHENC_CH2ENC_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH3ENC                                (0x1UL << 3)                       /**< Channel 3 Enable Clear */
#define _DMA_CHENC_CH3ENC_SHIFT                         3                                  /**< Shift value for DMA_CH3ENC */
#define _DMA_CHENC_CH3ENC_MASK                          0x8UL                              /**< Bit mask for DMA_CH3ENC */
#define _DMA_CHENC_CH3ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH3ENC_DEFAULT                        (_DMA_CHENC_CH3ENC_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH4ENC                                (0x1UL << 4)                       /**< Channel 4 Enable Clear */
#define _DMA_CHENC_CH4ENC_SHIFT                         4                                  /**< Shift value for DMA_CH4ENC */
#define _DMA_CHENC_CH4ENC_MASK                          0x10UL                             /**< Bit mask for DMA_CH4ENC */
#define _DMA_CHENC_CH4ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH4ENC_DEFAULT                        (_DMA_CHENC_CH4ENC_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH5ENC                                (0x1UL << 5)                       /**< Channel 5 Enable Clear */
#define _DMA_CHENC_CH5ENC_SHIFT                         5                                  /**< Shift value for DMA_CH5ENC */
#define _DMA_CHENC_CH5ENC_MASK                          0x20UL                             /**< Bit mask for DMA_CH5ENC */
#define _DMA_CHENC_CH5ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH5ENC_DEFAULT                        (_DMA_CHENC_CH5ENC_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH6ENC                                (0x1UL << 6)                       /**< Channel 6 Enable Clear */
#define _DMA_CHENC_CH6ENC_SHIFT                         6                                  /**< Shift value for DMA_CH6ENC */
#define _DMA_CHENC_CH6ENC_MASK                          0x40UL                             /**< Bit mask for DMA_CH6ENC */
#define _DMA_CHENC_CH6ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH6ENC_DEFAULT                        (_DMA_CHENC_CH6ENC_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH7ENC                                (0x1UL << 7)                       /**< Channel 7 Enable Clear */
#define _DMA_CHENC_CH7ENC_SHIFT                         7                                  /**< Shift value for DMA_CH7ENC */
#define _DMA_CHENC_CH7ENC_MASK                          0x80UL                             /**< Bit mask for DMA_CH7ENC */
#define _DMA_CHENC_CH7ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH7ENC_DEFAULT                        (_DMA_CHENC_CH7ENC_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH8ENC                                (0x1UL << 8)                       /**< Channel 8 Enable Clear */
#define _DMA_CHENC_CH8ENC_SHIFT                         8                                  /**< Shift value for DMA_CH8ENC */
#define _DMA_CHENC_CH8ENC_MASK                          0x100UL                            /**< Bit mask for DMA_CH8ENC */
#define _DMA_CHENC_CH8ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH8ENC_DEFAULT                        (_DMA_CHENC_CH8ENC_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH9ENC                                (0x1UL << 9)                       /**< Channel 9 Enable Clear */
#define _DMA_CHENC_CH9ENC_SHIFT                         9                                  /**< Shift value for DMA_CH9ENC */
#define _DMA_CHENC_CH9ENC_MASK                          0x200UL                            /**< Bit mask for DMA_CH9ENC */
#define _DMA_CHENC_CH9ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH9ENC_DEFAULT                        (_DMA_CHENC_CH9ENC_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH10ENC                               (0x1UL << 10)                      /**< Channel 10 Enable Clear */
#define _DMA_CHENC_CH10ENC_SHIFT                        10                                 /**< Shift value for DMA_CH10ENC */
#define _DMA_CHENC_CH10ENC_MASK                         0x400UL                            /**< Bit mask for DMA_CH10ENC */
#define _DMA_CHENC_CH10ENC_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH10ENC_DEFAULT                       (_DMA_CHENC_CH10ENC_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH11ENC                               (0x1UL << 11)                      /**< Channel 11 Enable Clear */
#define _DMA_CHENC_CH11ENC_SHIFT                        11                                 /**< Shift value for DMA_CH11ENC */
#define _DMA_CHENC_CH11ENC_MASK                         0x800UL                            /**< Bit mask for DMA_CH11ENC */
#define _DMA_CHENC_CH11ENC_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH11ENC_DEFAULT                       (_DMA_CHENC_CH11ENC_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHENC */

/* Bit fields for DMA CHALTS */
#define _DMA_CHALTS_RESETVALUE                          0x00000000UL                         /**< Default value for DMA_CHALTS */
#define _DMA_CHALTS_MASK                                0x00000FFFUL                         /**< Mask for DMA_CHALTS */
#define DMA_CHALTS_CH0ALTS                              (0x1UL << 0)                         /**< Channel 0 Alternate Structure Set */
#define _DMA_CHALTS_CH0ALTS_SHIFT                       0                                    /**< Shift value for DMA_CH0ALTS */
#define _DMA_CHALTS_CH0ALTS_MASK                        0x1UL                                /**< Bit mask for DMA_CH0ALTS */
#define _DMA_CHALTS_CH0ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH0ALTS_DEFAULT                      (_DMA_CHALTS_CH0ALTS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH1ALTS                              (0x1UL << 1)                         /**< Channel 1 Alternate Structure Set */
#define _DMA_CHALTS_CH1ALTS_SHIFT                       1                                    /**< Shift value for DMA_CH1ALTS */
#define _DMA_CHALTS_CH1ALTS_MASK                        0x2UL                                /**< Bit mask for DMA_CH1ALTS */
#define _DMA_CHALTS_CH1ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH1ALTS_DEFAULT                      (_DMA_CHALTS_CH1ALTS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH2ALTS                              (0x1UL << 2)                         /**< Channel 2 Alternate Structure Set */
#define _DMA_CHALTS_CH2ALTS_SHIFT                       2                                    /**< Shift value for DMA_CH2ALTS */
#define _DMA_CHALTS_CH2ALTS_MASK                        0x4UL                                /**< Bit mask for DMA_CH2ALTS */
#define _DMA_CHALTS_CH2ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH2ALTS_DEFAULT                      (_DMA_CHALTS_CH2ALTS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH3ALTS                              (0x1UL << 3)                         /**< Channel 3 Alternate Structure Set */
#define _DMA_CHALTS_CH3ALTS_SHIFT                       3                                    /**< Shift value for DMA_CH3ALTS */
#define _DMA_CHALTS_CH3ALTS_MASK                        0x8UL                                /**< Bit mask for DMA_CH3ALTS */
#define _DMA_CHALTS_CH3ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH3ALTS_DEFAULT                      (_DMA_CHALTS_CH3ALTS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH4ALTS                              (0x1UL << 4)                         /**< Channel 4 Alternate Structure Set */
#define _DMA_CHALTS_CH4ALTS_SHIFT                       4                                    /**< Shift value for DMA_CH4ALTS */
#define _DMA_CHALTS_CH4ALTS_MASK                        0x10UL                               /**< Bit mask for DMA_CH4ALTS */
#define _DMA_CHALTS_CH4ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH4ALTS_DEFAULT                      (_DMA_CHALTS_CH4ALTS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH5ALTS                              (0x1UL << 5)                         /**< Channel 5 Alternate Structure Set */
#define _DMA_CHALTS_CH5ALTS_SHIFT                       5                                    /**< Shift value for DMA_CH5ALTS */
#define _DMA_CHALTS_CH5ALTS_MASK                        0x20UL                               /**< Bit mask for DMA_CH5ALTS */
#define _DMA_CHALTS_CH5ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH5ALTS_DEFAULT                      (_DMA_CHALTS_CH5ALTS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH6ALTS                              (0x1UL << 6)                         /**< Channel 6 Alternate Structure Set */
#define _DMA_CHALTS_CH6ALTS_SHIFT                       6                                    /**< Shift value for DMA_CH6ALTS */
#define _DMA_CHALTS_CH6ALTS_MASK                        0x40UL                               /**< Bit mask for DMA_CH6ALTS */
#define _DMA_CHALTS_CH6ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH6ALTS_DEFAULT                      (_DMA_CHALTS_CH6ALTS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH7ALTS                              (0x1UL << 7)                         /**< Channel 7 Alternate Structure Set */
#define _DMA_CHALTS_CH7ALTS_SHIFT                       7                                    /**< Shift value for DMA_CH7ALTS */
#define _DMA_CHALTS_CH7ALTS_MASK                        0x80UL                               /**< Bit mask for DMA_CH7ALTS */
#define _DMA_CHALTS_CH7ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH7ALTS_DEFAULT                      (_DMA_CHALTS_CH7ALTS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH8ALTS                              (0x1UL << 8)                         /**< Channel 8 Alternate Structure Set */
#define _DMA_CHALTS_CH8ALTS_SHIFT                       8                                    /**< Shift value for DMA_CH8ALTS */
#define _DMA_CHALTS_CH8ALTS_MASK                        0x100UL                              /**< Bit mask for DMA_CH8ALTS */
#define _DMA_CHALTS_CH8ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH8ALTS_DEFAULT                      (_DMA_CHALTS_CH8ALTS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH9ALTS                              (0x1UL << 9)                         /**< Channel 9 Alternate Structure Set */
#define _DMA_CHALTS_CH9ALTS_SHIFT                       9                                    /**< Shift value for DMA_CH9ALTS */
#define _DMA_CHALTS_CH9ALTS_MASK                        0x200UL                              /**< Bit mask for DMA_CH9ALTS */
#define _DMA_CHALTS_CH9ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH9ALTS_DEFAULT                      (_DMA_CHALTS_CH9ALTS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH10ALTS                             (0x1UL << 10)                        /**< Channel 10 Alternate Structure Set */
#define _DMA_CHALTS_CH10ALTS_SHIFT                      10                                   /**< Shift value for DMA_CH10ALTS */
#define _DMA_CHALTS_CH10ALTS_MASK                       0x400UL                              /**< Bit mask for DMA_CH10ALTS */
#define _DMA_CHALTS_CH10ALTS_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH10ALTS_DEFAULT                     (_DMA_CHALTS_CH10ALTS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH11ALTS                             (0x1UL << 11)                        /**< Channel 11 Alternate Structure Set */
#define _DMA_CHALTS_CH11ALTS_SHIFT                      11                                   /**< Shift value for DMA_CH11ALTS */
#define _DMA_CHALTS_CH11ALTS_MASK                       0x800UL                              /**< Bit mask for DMA_CH11ALTS */
#define _DMA_CHALTS_CH11ALTS_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH11ALTS_DEFAULT                     (_DMA_CHALTS_CH11ALTS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHALTS */

/* Bit fields for DMA CHALTC */
#define _DMA_CHALTC_RESETVALUE                          0x00000000UL                         /**< Default value for DMA_CHALTC */
#define _DMA_CHALTC_MASK                                0x00000FFFUL                         /**< Mask for DMA_CHALTC */
#define DMA_CHALTC_CH0ALTC                              (0x1UL << 0)                         /**< Channel 0 Alternate Clear */
#define _DMA_CHALTC_CH0ALTC_SHIFT                       0                                    /**< Shift value for DMA_CH0ALTC */
#define _DMA_CHALTC_CH0ALTC_MASK                        0x1UL                                /**< Bit mask for DMA_CH0ALTC */
#define _DMA_CHALTC_CH0ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH0ALTC_DEFAULT                      (_DMA_CHALTC_CH0ALTC_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH1ALTC                              (0x1UL << 1)                         /**< Channel 1 Alternate Clear */
#define _DMA_CHALTC_CH1ALTC_SHIFT                       1                                    /**< Shift value for DMA_CH1ALTC */
#define _DMA_CHALTC_CH1ALTC_MASK                        0x2UL                                /**< Bit mask for DMA_CH1ALTC */
#define _DMA_CHALTC_CH1ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH1ALTC_DEFAULT                      (_DMA_CHALTC_CH1ALTC_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH2ALTC                              (0x1UL << 2)                         /**< Channel 2 Alternate Clear */
#define _DMA_CHALTC_CH2ALTC_SHIFT                       2                                    /**< Shift value for DMA_CH2ALTC */
#define _DMA_CHALTC_CH2ALTC_MASK                        0x4UL                                /**< Bit mask for DMA_CH2ALTC */
#define _DMA_CHALTC_CH2ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH2ALTC_DEFAULT                      (_DMA_CHALTC_CH2ALTC_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH3ALTC                              (0x1UL << 3)                         /**< Channel 3 Alternate Clear */
#define _DMA_CHALTC_CH3ALTC_SHIFT                       3                                    /**< Shift value for DMA_CH3ALTC */
#define _DMA_CHALTC_CH3ALTC_MASK                        0x8UL                                /**< Bit mask for DMA_CH3ALTC */
#define _DMA_CHALTC_CH3ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH3ALTC_DEFAULT                      (_DMA_CHALTC_CH3ALTC_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH4ALTC                              (0x1UL << 4)                         /**< Channel 4 Alternate Clear */
#define _DMA_CHALTC_CH4ALTC_SHIFT                       4                                    /**< Shift value for DMA_CH4ALTC */
#define _DMA_CHALTC_CH4ALTC_MASK                        0x10UL                               /**< Bit mask for DMA_CH4ALTC */
#define _DMA_CHALTC_CH4ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH4ALTC_DEFAULT                      (_DMA_CHALTC_CH4ALTC_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH5ALTC                              (0x1UL << 5)                         /**< Channel 5 Alternate Clear */
#define _DMA_CHALTC_CH5ALTC_SHIFT                       5                                    /**< Shift value for DMA_CH5ALTC */
#define _DMA_CHALTC_CH5ALTC_MASK                        0x20UL                               /**< Bit mask for DMA_CH5ALTC */
#define _DMA_CHALTC_CH5ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH5ALTC_DEFAULT                      (_DMA_CHALTC_CH5ALTC_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH6ALTC                              (0x1UL << 6)                         /**< Channel 6 Alternate Clear */
#define _DMA_CHALTC_CH6ALTC_SHIFT                       6                                    /**< Shift value for DMA_CH6ALTC */
#define _DMA_CHALTC_CH6ALTC_MASK                        0x40UL                               /**< Bit mask for DMA_CH6ALTC */
#define _DMA_CHALTC_CH6ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH6ALTC_DEFAULT                      (_DMA_CHALTC_CH6ALTC_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH7ALTC                              (0x1UL << 7)                         /**< Channel 7 Alternate Clear */
#define _DMA_CHALTC_CH7ALTC_SHIFT                       7                                    /**< Shift value for DMA_CH7ALTC */
#define _DMA_CHALTC_CH7ALTC_MASK                        0x80UL                               /**< Bit mask for DMA_CH7ALTC */
#define _DMA_CHALTC_CH7ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH7ALTC_DEFAULT                      (_DMA_CHALTC_CH7ALTC_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH8ALTC                              (0x1UL << 8)                         /**< Channel 8 Alternate Clear */
#define _DMA_CHALTC_CH8ALTC_SHIFT                       8                                    /**< Shift value for DMA_CH8ALTC */
#define _DMA_CHALTC_CH8ALTC_MASK                        0x100UL                              /**< Bit mask for DMA_CH8ALTC */
#define _DMA_CHALTC_CH8ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH8ALTC_DEFAULT                      (_DMA_CHALTC_CH8ALTC_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH9ALTC                              (0x1UL << 9)                         /**< Channel 9 Alternate Clear */
#define _DMA_CHALTC_CH9ALTC_SHIFT                       9                                    /**< Shift value for DMA_CH9ALTC */
#define _DMA_CHALTC_CH9ALTC_MASK                        0x200UL                              /**< Bit mask for DMA_CH9ALTC */
#define _DMA_CHALTC_CH9ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH9ALTC_DEFAULT                      (_DMA_CHALTC_CH9ALTC_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH10ALTC                             (0x1UL << 10)                        /**< Channel 10 Alternate Clear */
#define _DMA_CHALTC_CH10ALTC_SHIFT                      10                                   /**< Shift value for DMA_CH10ALTC */
#define _DMA_CHALTC_CH10ALTC_MASK                       0x400UL                              /**< Bit mask for DMA_CH10ALTC */
#define _DMA_CHALTC_CH10ALTC_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH10ALTC_DEFAULT                     (_DMA_CHALTC_CH10ALTC_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH11ALTC                             (0x1UL << 11)                        /**< Channel 11 Alternate Clear */
#define _DMA_CHALTC_CH11ALTC_SHIFT                      11                                   /**< Shift value for DMA_CH11ALTC */
#define _DMA_CHALTC_CH11ALTC_MASK                       0x800UL                              /**< Bit mask for DMA_CH11ALTC */
#define _DMA_CHALTC_CH11ALTC_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH11ALTC_DEFAULT                     (_DMA_CHALTC_CH11ALTC_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHALTC */

/* Bit fields for DMA CHPRIS */
#define _DMA_CHPRIS_RESETVALUE                          0x00000000UL                         /**< Default value for DMA_CHPRIS */
#define _DMA_CHPRIS_MASK                                0x00000FFFUL                         /**< Mask for DMA_CHPRIS */
#define DMA_CHPRIS_CH0PRIS                              (0x1UL << 0)                         /**< Channel 0 High Priority Set */
#define _DMA_CHPRIS_CH0PRIS_SHIFT                       0                                    /**< Shift value for DMA_CH0PRIS */
#define _DMA_CHPRIS_CH0PRIS_MASK                        0x1UL                                /**< Bit mask for DMA_CH0PRIS */
#define _DMA_CHPRIS_CH0PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH0PRIS_DEFAULT                      (_DMA_CHPRIS_CH0PRIS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH1PRIS                              (0x1UL << 1)                         /**< Channel 1 High Priority Set */
#define _DMA_CHPRIS_CH1PRIS_SHIFT                       1                                    /**< Shift value for DMA_CH1PRIS */
#define _DMA_CHPRIS_CH1PRIS_MASK                        0x2UL                                /**< Bit mask for DMA_CH1PRIS */
#define _DMA_CHPRIS_CH1PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH1PRIS_DEFAULT                      (_DMA_CHPRIS_CH1PRIS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH2PRIS                              (0x1UL << 2)                         /**< Channel 2 High Priority Set */
#define _DMA_CHPRIS_CH2PRIS_SHIFT                       2                                    /**< Shift value for DMA_CH2PRIS */
#define _DMA_CHPRIS_CH2PRIS_MASK                        0x4UL                                /**< Bit mask for DMA_CH2PRIS */
#define _DMA_CHPRIS_CH2PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH2PRIS_DEFAULT                      (_DMA_CHPRIS_CH2PRIS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH3PRIS                              (0x1UL << 3)                         /**< Channel 3 High Priority Set */
#define _DMA_CHPRIS_CH3PRIS_SHIFT                       3                                    /**< Shift value for DMA_CH3PRIS */
#define _DMA_CHPRIS_CH3PRIS_MASK                        0x8UL                                /**< Bit mask for DMA_CH3PRIS */
#define _DMA_CHPRIS_CH3PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH3PRIS_DEFAULT                      (_DMA_CHPRIS_CH3PRIS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH4PRIS                              (0x1UL << 4)                         /**< Channel 4 High Priority Set */
#define _DMA_CHPRIS_CH4PRIS_SHIFT                       4                                    /**< Shift value for DMA_CH4PRIS */
#define _DMA_CHPRIS_CH4PRIS_MASK                        0x10UL                               /**< Bit mask for DMA_CH4PRIS */
#define _DMA_CHPRIS_CH4PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH4PRIS_DEFAULT                      (_DMA_CHPRIS_CH4PRIS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH5PRIS                              (0x1UL << 5)                         /**< Channel 5 High Priority Set */
#define _DMA_CHPRIS_CH5PRIS_SHIFT                       5                                    /**< Shift value for DMA_CH5PRIS */
#define _DMA_CHPRIS_CH5PRIS_MASK                        0x20UL                               /**< Bit mask for DMA_CH5PRIS */
#define _DMA_CHPRIS_CH5PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH5PRIS_DEFAULT                      (_DMA_CHPRIS_CH5PRIS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH6PRIS                              (0x1UL << 6)                         /**< Channel 6 High Priority Set */
#define _DMA_CHPRIS_CH6PRIS_SHIFT                       6                                    /**< Shift value for DMA_CH6PRIS */
#define _DMA_CHPRIS_CH6PRIS_MASK                        0x40UL                               /**< Bit mask for DMA_CH6PRIS */
#define _DMA_CHPRIS_CH6PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH6PRIS_DEFAULT                      (_DMA_CHPRIS_CH6PRIS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH7PRIS                              (0x1UL << 7)                         /**< Channel 7 High Priority Set */
#define _DMA_CHPRIS_CH7PRIS_SHIFT                       7                                    /**< Shift value for DMA_CH7PRIS */
#define _DMA_CHPRIS_CH7PRIS_MASK                        0x80UL                               /**< Bit mask for DMA_CH7PRIS */
#define _DMA_CHPRIS_CH7PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH7PRIS_DEFAULT                      (_DMA_CHPRIS_CH7PRIS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH8PRIS                              (0x1UL << 8)                         /**< Channel 8 High Priority Set */
#define _DMA_CHPRIS_CH8PRIS_SHIFT                       8                                    /**< Shift value for DMA_CH8PRIS */
#define _DMA_CHPRIS_CH8PRIS_MASK                        0x100UL                              /**< Bit mask for DMA_CH8PRIS */
#define _DMA_CHPRIS_CH8PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH8PRIS_DEFAULT                      (_DMA_CHPRIS_CH8PRIS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH9PRIS                              (0x1UL << 9)                         /**< Channel 9 High Priority Set */
#define _DMA_CHPRIS_CH9PRIS_SHIFT                       9                                    /**< Shift value for DMA_CH9PRIS */
#define _DMA_CHPRIS_CH9PRIS_MASK                        0x200UL                              /**< Bit mask for DMA_CH9PRIS */
#define _DMA_CHPRIS_CH9PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH9PRIS_DEFAULT                      (_DMA_CHPRIS_CH9PRIS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH10PRIS                             (0x1UL << 10)                        /**< Channel 10 High Priority Set */
#define _DMA_CHPRIS_CH10PRIS_SHIFT                      10                                   /**< Shift value for DMA_CH10PRIS */
#define _DMA_CHPRIS_CH10PRIS_MASK                       0x400UL                              /**< Bit mask for DMA_CH10PRIS */
#define _DMA_CHPRIS_CH10PRIS_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH10PRIS_DEFAULT                     (_DMA_CHPRIS_CH10PRIS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH11PRIS                             (0x1UL << 11)                        /**< Channel 11 High Priority Set */
#define _DMA_CHPRIS_CH11PRIS_SHIFT                      11                                   /**< Shift value for DMA_CH11PRIS */
#define _DMA_CHPRIS_CH11PRIS_MASK                       0x800UL                              /**< Bit mask for DMA_CH11PRIS */
#define _DMA_CHPRIS_CH11PRIS_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH11PRIS_DEFAULT                     (_DMA_CHPRIS_CH11PRIS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHPRIS */

/* Bit fields for DMA CHPRIC */
#define _DMA_CHPRIC_RESETVALUE                          0x00000000UL                         /**< Default value for DMA_CHPRIC */
#define _DMA_CHPRIC_MASK                                0x00000FFFUL                         /**< Mask for DMA_CHPRIC */
#define DMA_CHPRIC_CH0PRIC                              (0x1UL << 0)                         /**< Channel 0 High Priority Clear */
#define _DMA_CHPRIC_CH0PRIC_SHIFT                       0                                    /**< Shift value for DMA_CH0PRIC */
#define _DMA_CHPRIC_CH0PRIC_MASK                        0x1UL                                /**< Bit mask for DMA_CH0PRIC */
#define _DMA_CHPRIC_CH0PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH0PRIC_DEFAULT                      (_DMA_CHPRIC_CH0PRIC_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH1PRIC                              (0x1UL << 1)                         /**< Channel 1 High Priority Clear */
#define _DMA_CHPRIC_CH1PRIC_SHIFT                       1                                    /**< Shift value for DMA_CH1PRIC */
#define _DMA_CHPRIC_CH1PRIC_MASK                        0x2UL                                /**< Bit mask for DMA_CH1PRIC */
#define _DMA_CHPRIC_CH1PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH1PRIC_DEFAULT                      (_DMA_CHPRIC_CH1PRIC_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH2PRIC                              (0x1UL << 2)                         /**< Channel 2 High Priority Clear */
#define _DMA_CHPRIC_CH2PRIC_SHIFT                       2                                    /**< Shift value for DMA_CH2PRIC */
#define _DMA_CHPRIC_CH2PRIC_MASK                        0x4UL                                /**< Bit mask for DMA_CH2PRIC */
#define _DMA_CHPRIC_CH2PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH2PRIC_DEFAULT                      (_DMA_CHPRIC_CH2PRIC_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH3PRIC                              (0x1UL << 3)                         /**< Channel 3 High Priority Clear */
#define _DMA_CHPRIC_CH3PRIC_SHIFT                       3                                    /**< Shift value for DMA_CH3PRIC */
#define _DMA_CHPRIC_CH3PRIC_MASK                        0x8UL                                /**< Bit mask for DMA_CH3PRIC */
#define _DMA_CHPRIC_CH3PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH3PRIC_DEFAULT                      (_DMA_CHPRIC_CH3PRIC_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH4PRIC                              (0x1UL << 4)                         /**< Channel 4 High Priority Clear */
#define _DMA_CHPRIC_CH4PRIC_SHIFT                       4                                    /**< Shift value for DMA_CH4PRIC */
#define _DMA_CHPRIC_CH4PRIC_MASK                        0x10UL                               /**< Bit mask for DMA_CH4PRIC */
#define _DMA_CHPRIC_CH4PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH4PRIC_DEFAULT                      (_DMA_CHPRIC_CH4PRIC_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH5PRIC                              (0x1UL << 5)                         /**< Channel 5 High Priority Clear */
#define _DMA_CHPRIC_CH5PRIC_SHIFT                       5                                    /**< Shift value for DMA_CH5PRIC */
#define _DMA_CHPRIC_CH5PRIC_MASK                        0x20UL                               /**< Bit mask for DMA_CH5PRIC */
#define _DMA_CHPRIC_CH5PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH5PRIC_DEFAULT                      (_DMA_CHPRIC_CH5PRIC_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH6PRIC                              (0x1UL << 6)                         /**< Channel 6 High Priority Clear */
#define _DMA_CHPRIC_CH6PRIC_SHIFT                       6                                    /**< Shift value for DMA_CH6PRIC */
#define _DMA_CHPRIC_CH6PRIC_MASK                        0x40UL                               /**< Bit mask for DMA_CH6PRIC */
#define _DMA_CHPRIC_CH6PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH6PRIC_DEFAULT                      (_DMA_CHPRIC_CH6PRIC_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH7PRIC                              (0x1UL << 7)                         /**< Channel 7 High Priority Clear */
#define _DMA_CHPRIC_CH7PRIC_SHIFT                       7                                    /**< Shift value for DMA_CH7PRIC */
#define _DMA_CHPRIC_CH7PRIC_MASK                        0x80UL                               /**< Bit mask for DMA_CH7PRIC */
#define _DMA_CHPRIC_CH7PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH7PRIC_DEFAULT                      (_DMA_CHPRIC_CH7PRIC_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH8PRIC                              (0x1UL << 8)                         /**< Channel 8 High Priority Clear */
#define _DMA_CHPRIC_CH8PRIC_SHIFT                       8                                    /**< Shift value for DMA_CH8PRIC */
#define _DMA_CHPRIC_CH8PRIC_MASK                        0x100UL                              /**< Bit mask for DMA_CH8PRIC */
#define _DMA_CHPRIC_CH8PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH8PRIC_DEFAULT                      (_DMA_CHPRIC_CH8PRIC_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH9PRIC                              (0x1UL << 9)                         /**< Channel 9 High Priority Clear */
#define _DMA_CHPRIC_CH9PRIC_SHIFT                       9                                    /**< Shift value for DMA_CH9PRIC */
#define _DMA_CHPRIC_CH9PRIC_MASK                        0x200UL                              /**< Bit mask for DMA_CH9PRIC */
#define _DMA_CHPRIC_CH9PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH9PRIC_DEFAULT                      (_DMA_CHPRIC_CH9PRIC_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH10PRIC                             (0x1UL << 10)                        /**< Channel 10 High Priority Clear */
#define _DMA_CHPRIC_CH10PRIC_SHIFT                      10                                   /**< Shift value for DMA_CH10PRIC */
#define _DMA_CHPRIC_CH10PRIC_MASK                       0x400UL                              /**< Bit mask for DMA_CH10PRIC */
#define _DMA_CHPRIC_CH10PRIC_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH10PRIC_DEFAULT                     (_DMA_CHPRIC_CH10PRIC_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH11PRIC                             (0x1UL << 11)                        /**< Channel 11 High Priority Clear */
#define _DMA_CHPRIC_CH11PRIC_SHIFT                      11                                   /**< Shift value for DMA_CH11PRIC */
#define _DMA_CHPRIC_CH11PRIC_MASK                       0x800UL                              /**< Bit mask for DMA_CH11PRIC */
#define _DMA_CHPRIC_CH11PRIC_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH11PRIC_DEFAULT                     (_DMA_CHPRIC_CH11PRIC_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHPRIC */

/* Bit fields for DMA ERRORC */
#define _DMA_ERRORC_RESETVALUE                          0x00000000UL                      /**< Default value for DMA_ERRORC */
#define _DMA_ERRORC_MASK                                0x00000001UL                      /**< Mask for DMA_ERRORC */
#define DMA_ERRORC_ERRORC                               (0x1UL << 0)                      /**< Bus Error Clear */
#define _DMA_ERRORC_ERRORC_SHIFT                        0                                 /**< Shift value for DMA_ERRORC */
#define _DMA_ERRORC_ERRORC_MASK                         0x1UL                             /**< Bit mask for DMA_ERRORC */
#define _DMA_ERRORC_ERRORC_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for DMA_ERRORC */
#define DMA_ERRORC_ERRORC_DEFAULT                       (_DMA_ERRORC_ERRORC_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_ERRORC */

/* Bit fields for DMA CHREQSTATUS */
#define _DMA_CHREQSTATUS_RESETVALUE                     0x00000000UL                                   /**< Default value for DMA_CHREQSTATUS */
#define _DMA_CHREQSTATUS_MASK                           0x00000FFFUL                                   /**< Mask for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH0REQSTATUS                    (0x1UL << 0)                                   /**< Channel 0 Request Status */
#define _DMA_CHREQSTATUS_CH0REQSTATUS_SHIFT             0                                              /**< Shift value for DMA_CH0REQSTATUS */
#define _DMA_CHREQSTATUS_CH0REQSTATUS_MASK              0x1UL                                          /**< Bit mask for DMA_CH0REQSTATUS */
#define _DMA_CHREQSTATUS_CH0REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH0REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH0REQSTATUS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH1REQSTATUS                    (0x1UL << 1)                                   /**< Channel 1 Request Status */
#define _DMA_CHREQSTATUS_CH1REQSTATUS_SHIFT             1                                              /**< Shift value for DMA_CH1REQSTATUS */
#define _DMA_CHREQSTATUS_CH1REQSTATUS_MASK              0x2UL                                          /**< Bit mask for DMA_CH1REQSTATUS */
#define _DMA_CHREQSTATUS_CH1REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH1REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH1REQSTATUS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH2REQSTATUS                    (0x1UL << 2)                                   /**< Channel 2 Request Status */
#define _DMA_CHREQSTATUS_CH2REQSTATUS_SHIFT             2                                              /**< Shift value for DMA_CH2REQSTATUS */
#define _DMA_CHREQSTATUS_CH2REQSTATUS_MASK              0x4UL                                          /**< Bit mask for DMA_CH2REQSTATUS */
#define _DMA_CHREQSTATUS_CH2REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH2REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH2REQSTATUS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH3REQSTATUS                    (0x1UL << 3)                                   /**< Channel 3 Request Status */
#define _DMA_CHREQSTATUS_CH3REQSTATUS_SHIFT             3                                              /**< Shift value for DMA_CH3REQSTATUS */
#define _DMA_CHREQSTATUS_CH3REQSTATUS_MASK              0x8UL                                          /**< Bit mask for DMA_CH3REQSTATUS */
#define _DMA_CHREQSTATUS_CH3REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH3REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH3REQSTATUS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH4REQSTATUS                    (0x1UL << 4)                                   /**< Channel 4 Request Status */
#define _DMA_CHREQSTATUS_CH4REQSTATUS_SHIFT             4                                              /**< Shift value for DMA_CH4REQSTATUS */
#define _DMA_CHREQSTATUS_CH4REQSTATUS_MASK              0x10UL                                         /**< Bit mask for DMA_CH4REQSTATUS */
#define _DMA_CHREQSTATUS_CH4REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH4REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH4REQSTATUS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH5REQSTATUS                    (0x1UL << 5)                                   /**< Channel 5 Request Status */
#define _DMA_CHREQSTATUS_CH5REQSTATUS_SHIFT             5                                              /**< Shift value for DMA_CH5REQSTATUS */
#define _DMA_CHREQSTATUS_CH5REQSTATUS_MASK              0x20UL                                         /**< Bit mask for DMA_CH5REQSTATUS */
#define _DMA_CHREQSTATUS_CH5REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH5REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH5REQSTATUS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH6REQSTATUS                    (0x1UL << 6)                                   /**< Channel 6 Request Status */
#define _DMA_CHREQSTATUS_CH6REQSTATUS_SHIFT             6                                              /**< Shift value for DMA_CH6REQSTATUS */
#define _DMA_CHREQSTATUS_CH6REQSTATUS_MASK              0x40UL                                         /**< Bit mask for DMA_CH6REQSTATUS */
#define _DMA_CHREQSTATUS_CH6REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH6REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH6REQSTATUS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH7REQSTATUS                    (0x1UL << 7)                                   /**< Channel 7 Request Status */
#define _DMA_CHREQSTATUS_CH7REQSTATUS_SHIFT             7                                              /**< Shift value for DMA_CH7REQSTATUS */
#define _DMA_CHREQSTATUS_CH7REQSTATUS_MASK              0x80UL                                         /**< Bit mask for DMA_CH7REQSTATUS */
#define _DMA_CHREQSTATUS_CH7REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH7REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH7REQSTATUS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH8REQSTATUS                    (0x1UL << 8)                                   /**< Channel 8 Request Status */
#define _DMA_CHREQSTATUS_CH8REQSTATUS_SHIFT             8                                              /**< Shift value for DMA_CH8REQSTATUS */
#define _DMA_CHREQSTATUS_CH8REQSTATUS_MASK              0x100UL                                        /**< Bit mask for DMA_CH8REQSTATUS */
#define _DMA_CHREQSTATUS_CH8REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH8REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH8REQSTATUS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH9REQSTATUS                    (0x1UL << 9)                                   /**< Channel 9 Request Status */
#define _DMA_CHREQSTATUS_CH9REQSTATUS_SHIFT             9                                              /**< Shift value for DMA_CH9REQSTATUS */
#define _DMA_CHREQSTATUS_CH9REQSTATUS_MASK              0x200UL                                        /**< Bit mask for DMA_CH9REQSTATUS */
#define _DMA_CHREQSTATUS_CH9REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH9REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH9REQSTATUS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH10REQSTATUS                   (0x1UL << 10)                                  /**< Channel 10 Request Status */
#define _DMA_CHREQSTATUS_CH10REQSTATUS_SHIFT            10                                             /**< Shift value for DMA_CH10REQSTATUS */
#define _DMA_CHREQSTATUS_CH10REQSTATUS_MASK             0x400UL                                        /**< Bit mask for DMA_CH10REQSTATUS */
#define _DMA_CHREQSTATUS_CH10REQSTATUS_DEFAULT          0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH10REQSTATUS_DEFAULT           (_DMA_CHREQSTATUS_CH10REQSTATUS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH11REQSTATUS                   (0x1UL << 11)                                  /**< Channel 11 Request Status */
#define _DMA_CHREQSTATUS_CH11REQSTATUS_SHIFT            11                                             /**< Shift value for DMA_CH11REQSTATUS */
#define _DMA_CHREQSTATUS_CH11REQSTATUS_MASK             0x800UL                                        /**< Bit mask for DMA_CH11REQSTATUS */
#define _DMA_CHREQSTATUS_CH11REQSTATUS_DEFAULT          0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH11REQSTATUS_DEFAULT           (_DMA_CHREQSTATUS_CH11REQSTATUS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */

/* Bit fields for DMA CHSREQSTATUS */
#define _DMA_CHSREQSTATUS_RESETVALUE                    0x00000000UL                                     /**< Default value for DMA_CHSREQSTATUS */
#define _DMA_CHSREQSTATUS_MASK                          0x00000FFFUL                                     /**< Mask for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH0SREQSTATUS                  (0x1UL << 0)                                     /**< Channel 0 Single Request Status */
#define _DMA_CHSREQSTATUS_CH0SREQSTATUS_SHIFT           0                                                /**< Shift value for DMA_CH0SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH0SREQSTATUS_MASK            0x1UL                                            /**< Bit mask for DMA_CH0SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH0SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH0SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH0SREQSTATUS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH1SREQSTATUS                  (0x1UL << 1)                                     /**< Channel 1 Single Request Status */
#define _DMA_CHSREQSTATUS_CH1SREQSTATUS_SHIFT           1                                                /**< Shift value for DMA_CH1SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH1SREQSTATUS_MASK            0x2UL                                            /**< Bit mask for DMA_CH1SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH1SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH1SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH1SREQSTATUS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH2SREQSTATUS                  (0x1UL << 2)                                     /**< Channel 2 Single Request Status */
#define _DMA_CHSREQSTATUS_CH2SREQSTATUS_SHIFT           2                                                /**< Shift value for DMA_CH2SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH2SREQSTATUS_MASK            0x4UL                                            /**< Bit mask for DMA_CH2SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH2SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH2SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH2SREQSTATUS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH3SREQSTATUS                  (0x1UL << 3)                                     /**< Channel 3 Single Request Status */
#define _DMA_CHSREQSTATUS_CH3SREQSTATUS_SHIFT           3                                                /**< Shift value for DMA_CH3SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH3SREQSTATUS_MASK            0x8UL                                            /**< Bit mask for DMA_CH3SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH3SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH3SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH3SREQSTATUS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH4SREQSTATUS                  (0x1UL << 4)                                     /**< Channel 4 Single Request Status */
#define _DMA_CHSREQSTATUS_CH4SREQSTATUS_SHIFT           4                                                /**< Shift value for DMA_CH4SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH4SREQSTATUS_MASK            0x10UL                                           /**< Bit mask for DMA_CH4SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH4SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH4SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH4SREQSTATUS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH5SREQSTATUS                  (0x1UL << 5)                                     /**< Channel 5 Single Request Status */
#define _DMA_CHSREQSTATUS_CH5SREQSTATUS_SHIFT           5                                                /**< Shift value for DMA_CH5SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH5SREQSTATUS_MASK            0x20UL                                           /**< Bit mask for DMA_CH5SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH5SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH5SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH5SREQSTATUS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH6SREQSTATUS                  (0x1UL << 6)                                     /**< Channel 6 Single Request Status */
#define _DMA_CHSREQSTATUS_CH6SREQSTATUS_SHIFT           6                                                /**< Shift value for DMA_CH6SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH6SREQSTATUS_MASK            0x40UL                                           /**< Bit mask for DMA_CH6SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH6SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH6SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH6SREQSTATUS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH7SREQSTATUS                  (0x1UL << 7)                                     /**< Channel 7 Single Request Status */
#define _DMA_CHSREQSTATUS_CH7SREQSTATUS_SHIFT           7                                                /**< Shift value for DMA_CH7SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH7SREQSTATUS_MASK            0x80UL                                           /**< Bit mask for DMA_CH7SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH7SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH7SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH7SREQSTATUS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH8SREQSTATUS                  (0x1UL << 8)                                     /**< Channel 8 Single Request Status */
#define _DMA_CHSREQSTATUS_CH8SREQSTATUS_SHIFT           8                                                /**< Shift value for DMA_CH8SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH8SREQSTATUS_MASK            0x100UL                                          /**< Bit mask for DMA_CH8SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH8SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH8SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH8SREQSTATUS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH9SREQSTATUS                  (0x1UL << 9)                                     /**< Channel 9 Single Request Status */
#define _DMA_CHSREQSTATUS_CH9SREQSTATUS_SHIFT           9                                                /**< Shift value for DMA_CH9SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH9SREQSTATUS_MASK            0x200UL                                          /**< Bit mask for DMA_CH9SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH9SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH9SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH9SREQSTATUS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH10SREQSTATUS                 (0x1UL << 10)                                    /**< Channel 10 Single Request Status */
#define _DMA_CHSREQSTATUS_CH10SREQSTATUS_SHIFT          10                                               /**< Shift value for DMA_CH10SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH10SREQSTATUS_MASK           0x400UL                                          /**< Bit mask for DMA_CH10SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH10SREQSTATUS_DEFAULT        0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH10SREQSTATUS_DEFAULT         (_DMA_CHSREQSTATUS_CH10SREQSTATUS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH11SREQSTATUS                 (0x1UL << 11)                                    /**< Channel 11 Single Request Status */
#define _DMA_CHSREQSTATUS_CH11SREQSTATUS_SHIFT          11                                               /**< Shift value for DMA_CH11SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH11SREQSTATUS_MASK           0x800UL                                          /**< Bit mask for DMA_CH11SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH11SREQSTATUS_DEFAULT        0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH11SREQSTATUS_DEFAULT         (_DMA_CHSREQSTATUS_CH11SREQSTATUS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */

/* Bit fields for DMA IF */
#define _DMA_IF_RESETVALUE                              0x00000000UL                     /**< Default value for DMA_IF */
#define _DMA_IF_MASK                                    0x80000FFFUL                     /**< Mask for DMA_IF */
#define DMA_IF_CH0DONE                                  (0x1UL << 0)                     /**< DMA Channel 0 Complete Interrupt Flag */
#define _DMA_IF_CH0DONE_SHIFT                           0                                /**< Shift value for DMA_CH0DONE */
#define _DMA_IF_CH0DONE_MASK                            0x1UL                            /**< Bit mask for DMA_CH0DONE */
#define _DMA_IF_CH0DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH0DONE_DEFAULT                          (_DMA_IF_CH0DONE_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH1DONE                                  (0x1UL << 1)                     /**< DMA Channel 1 Complete Interrupt Flag */
#define _DMA_IF_CH1DONE_SHIFT                           1                                /**< Shift value for DMA_CH1DONE */
#define _DMA_IF_CH1DONE_MASK                            0x2UL                            /**< Bit mask for DMA_CH1DONE */
#define _DMA_IF_CH1DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH1DONE_DEFAULT                          (_DMA_IF_CH1DONE_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH2DONE                                  (0x1UL << 2)                     /**< DMA Channel 2 Complete Interrupt Flag */
#define _DMA_IF_CH2DONE_SHIFT                           2                                /**< Shift value for DMA_CH2DONE */
#define _DMA_IF_CH2DONE_MASK                            0x4UL                            /**< Bit mask for DMA_CH2DONE */
#define _DMA_IF_CH2DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH2DONE_DEFAULT                          (_DMA_IF_CH2DONE_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH3DONE                                  (0x1UL << 3)                     /**< DMA Channel 3 Complete Interrupt Flag */
#define _DMA_IF_CH3DONE_SHIFT                           3                                /**< Shift value for DMA_CH3DONE */
#define _DMA_IF_CH3DONE_MASK                            0x8UL                            /**< Bit mask for DMA_CH3DONE */
#define _DMA_IF_CH3DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH3DONE_DEFAULT                          (_DMA_IF_CH3DONE_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH4DONE                                  (0x1UL << 4)                     /**< DMA Channel 4 Complete Interrupt Flag */
#define _DMA_IF_CH4DONE_SHIFT                           4                                /**< Shift value for DMA_CH4DONE */
#define _DMA_IF_CH4DONE_MASK                            0x10UL                           /**< Bit mask for DMA_CH4DONE */
#define _DMA_IF_CH4DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH4DONE_DEFAULT                          (_DMA_IF_CH4DONE_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH5DONE                                  (0x1UL << 5)                     /**< DMA Channel 5 Complete Interrupt Flag */
#define _DMA_IF_CH5DONE_SHIFT                           5                                /**< Shift value for DMA_CH5DONE */
#define _DMA_IF_CH5DONE_MASK                            0x20UL                           /**< Bit mask for DMA_CH5DONE */
#define _DMA_IF_CH5DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH5DONE_DEFAULT                          (_DMA_IF_CH5DONE_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH6DONE                                  (0x1UL << 6)                     /**< DMA Channel 6 Complete Interrupt Flag */
#define _DMA_IF_CH6DONE_SHIFT                           6                                /**< Shift value for DMA_CH6DONE */
#define _DMA_IF_CH6DONE_MASK                            0x40UL                           /**< Bit mask for DMA_CH6DONE */
#define _DMA_IF_CH6DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH6DONE_DEFAULT                          (_DMA_IF_CH6DONE_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH7DONE                                  (0x1UL << 7)                     /**< DMA Channel 7 Complete Interrupt Flag */
#define _DMA_IF_CH7DONE_SHIFT                           7                                /**< Shift value for DMA_CH7DONE */
#define _DMA_IF_CH7DONE_MASK                            0x80UL                           /**< Bit mask for DMA_CH7DONE */
#define _DMA_IF_CH7DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH7DONE_DEFAULT                          (_DMA_IF_CH7DONE_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH8DONE                                  (0x1UL << 8)                     /**< DMA Channel 8 Complete Interrupt Flag */
#define _DMA_IF_CH8DONE_SHIFT                           8                                /**< Shift value for DMA_CH8DONE */
#define _DMA_IF_CH8DONE_MASK                            0x100UL                          /**< Bit mask for DMA_CH8DONE */
#define _DMA_IF_CH8DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH8DONE_DEFAULT                          (_DMA_IF_CH8DONE_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH9DONE                                  (0x1UL << 9)                     /**< DMA Channel 9 Complete Interrupt Flag */
#define _DMA_IF_CH9DONE_SHIFT                           9                                /**< Shift value for DMA_CH9DONE */
#define _DMA_IF_CH9DONE_MASK                            0x200UL                          /**< Bit mask for DMA_CH9DONE */
#define _DMA_IF_CH9DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH9DONE_DEFAULT                          (_DMA_IF_CH9DONE_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH10DONE                                 (0x1UL << 10)                    /**< DMA Channel 10 Complete Interrupt Flag */
#define _DMA_IF_CH10DONE_SHIFT                          10                               /**< Shift value for DMA_CH10DONE */
#define _DMA_IF_CH10DONE_MASK                           0x400UL                          /**< Bit mask for DMA_CH10DONE */
#define _DMA_IF_CH10DONE_DEFAULT                        0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH10DONE_DEFAULT                         (_DMA_IF_CH10DONE_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH11DONE                                 (0x1UL << 11)                    /**< DMA Channel 11 Complete Interrupt Flag */
#define _DMA_IF_CH11DONE_SHIFT                          11                               /**< Shift value for DMA_CH11DONE */
#define _DMA_IF_CH11DONE_MASK                           0x800UL                          /**< Bit mask for DMA_CH11DONE */
#define _DMA_IF_CH11DONE_DEFAULT                        0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH11DONE_DEFAULT                         (_DMA_IF_CH11DONE_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_ERR                                      (0x1UL << 31)                    /**< DMA Error Interrupt Flag */
#define _DMA_IF_ERR_SHIFT                               31                               /**< Shift value for DMA_ERR */
#define _DMA_IF_ERR_MASK                                0x80000000UL                     /**< Bit mask for DMA_ERR */
#define _DMA_IF_ERR_DEFAULT                             0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_ERR_DEFAULT                              (_DMA_IF_ERR_DEFAULT << 31)      /**< Shifted mode DEFAULT for DMA_IF */

/* Bit fields for DMA IFS */
#define _DMA_IFS_RESETVALUE                             0x00000000UL                      /**< Default value for DMA_IFS */
#define _DMA_IFS_MASK                                   0x80000FFFUL                      /**< Mask for DMA_IFS */
#define DMA_IFS_CH0DONE                                 (0x1UL << 0)                      /**< DMA Channel 0 Complete Interrupt Flag Set */
#define _DMA_IFS_CH0DONE_SHIFT                          0                                 /**< Shift value for DMA_CH0DONE */
#define _DMA_IFS_CH0DONE_MASK                           0x1UL                             /**< Bit mask for DMA_CH0DONE */
#define _DMA_IFS_CH0DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH0DONE_DEFAULT                         (_DMA_IFS_CH0DONE_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH1DONE                                 (0x1UL << 1)                      /**< DMA Channel 1 Complete Interrupt Flag Set */
#define _DMA_IFS_CH1DONE_SHIFT                          1                                 /**< Shift value for DMA_CH1DONE */
#define _DMA_IFS_CH1DONE_MASK                           0x2UL                             /**< Bit mask for DMA_CH1DONE */
#define _DMA_IFS_CH1DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH1DONE_DEFAULT                         (_DMA_IFS_CH1DONE_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH2DONE                                 (0x1UL << 2)                      /**< DMA Channel 2 Complete Interrupt Flag Set */
#define _DMA_IFS_CH2DONE_SHIFT                          2                                 /**< Shift value for DMA_CH2DONE */
#define _DMA_IFS_CH2DONE_MASK                           0x4UL                             /**< Bit mask for DMA_CH2DONE */
#define _DMA_IFS_CH2DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH2DONE_DEFAULT                         (_DMA_IFS_CH2DONE_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH3DONE                                 (0x1UL << 3)                      /**< DMA Channel 3 Complete Interrupt Flag Set */
#define _DMA_IFS_CH3DONE_SHIFT                          3                                 /**< Shift value for DMA_CH3DONE */
#define _DMA_IFS_CH3DONE_MASK                           0x8UL                             /**< Bit mask for DMA_CH3DONE */
#define _DMA_IFS_CH3DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH3DONE_DEFAULT                         (_DMA_IFS_CH3DONE_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH4DONE                                 (0x1UL << 4)                      /**< DMA Channel 4 Complete Interrupt Flag Set */
#define _DMA_IFS_CH4DONE_SHIFT                          4                                 /**< Shift value for DMA_CH4DONE */
#define _DMA_IFS_CH4DONE_MASK                           0x10UL                            /**< Bit mask for DMA_CH4DONE */
#define _DMA_IFS_CH4DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH4DONE_DEFAULT                         (_DMA_IFS_CH4DONE_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH5DONE                                 (0x1UL << 5)                      /**< DMA Channel 5 Complete Interrupt Flag Set */
#define _DMA_IFS_CH5DONE_SHIFT                          5                                 /**< Shift value for DMA_CH5DONE */
#define _DMA_IFS_CH5DONE_MASK                           0x20UL                            /**< Bit mask for DMA_CH5DONE */
#define _DMA_IFS_CH5DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH5DONE_DEFAULT                         (_DMA_IFS_CH5DONE_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH6DONE                                 (0x1UL << 6)                      /**< DMA Channel 6 Complete Interrupt Flag Set */
#define _DMA_IFS_CH6DONE_SHIFT                          6                                 /**< Shift value for DMA_CH6DONE */
#define _DMA_IFS_CH6DONE_MASK                           0x40UL                            /**< Bit mask for DMA_CH6DONE */
#define _DMA_IFS_CH6DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH6DONE_DEFAULT                         (_DMA_IFS_CH6DONE_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH7DONE                                 (0x1UL << 7)                      /**< DMA Channel 7 Complete Interrupt Flag Set */
#define _DMA_IFS_CH7DONE_SHIFT                          7                                 /**< Shift value for DMA_CH7DONE */
#define _DMA_IFS_CH7DONE_MASK                           0x80UL                            /**< Bit mask for DMA_CH7DONE */
#define _DMA_IFS_CH7DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH7DONE_DEFAULT                         (_DMA_IFS_CH7DONE_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH8DONE                                 (0x1UL << 8)                      /**< DMA Channel 8 Complete Interrupt Flag Set */
#define _DMA_IFS_CH8DONE_SHIFT                          8                                 /**< Shift value for DMA_CH8DONE */
#define _DMA_IFS_CH8DONE_MASK                           0x100UL                           /**< Bit mask for DMA_CH8DONE */
#define _DMA_IFS_CH8DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH8DONE_DEFAULT                         (_DMA_IFS_CH8DONE_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH9DONE                                 (0x1UL << 9)                      /**< DMA Channel 9 Complete Interrupt Flag Set */
#define _DMA_IFS_CH9DONE_SHIFT                          9                                 /**< Shift value for DMA_CH9DONE */
#define _DMA_IFS_CH9DONE_MASK                           0x200UL                           /**< Bit mask for DMA_CH9DONE */
#define _DMA_IFS_CH9DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH9DONE_DEFAULT                         (_DMA_IFS_CH9DONE_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH10DONE                                (0x1UL << 10)                     /**< DMA Channel 10 Complete Interrupt Flag Set */
#define _DMA_IFS_CH10DONE_SHIFT                         10                                /**< Shift value for DMA_CH10DONE */
#define _DMA_IFS_CH10DONE_MASK                          0x400UL                           /**< Bit mask for DMA_CH10DONE */
#define _DMA_IFS_CH10DONE_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH10DONE_DEFAULT                        (_DMA_IFS_CH10DONE_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH11DONE                                (0x1UL << 11)                     /**< DMA Channel 11 Complete Interrupt Flag Set */
#define _DMA_IFS_CH11DONE_SHIFT                         11                                /**< Shift value for DMA_CH11DONE */
#define _DMA_IFS_CH11DONE_MASK                          0x800UL                           /**< Bit mask for DMA_CH11DONE */
#define _DMA_IFS_CH11DONE_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH11DONE_DEFAULT                        (_DMA_IFS_CH11DONE_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_ERR                                     (0x1UL << 31)                     /**< DMA Error Interrupt Flag Set */
#define _DMA_IFS_ERR_SHIFT                              31                                /**< Shift value for DMA_ERR */
#define _DMA_IFS_ERR_MASK                               0x80000000UL                      /**< Bit mask for DMA_ERR */
#define _DMA_IFS_ERR_DEFAULT                            0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_ERR_DEFAULT                             (_DMA_IFS_ERR_DEFAULT << 31)      /**< Shifted mode DEFAULT for DMA_IFS */

/* Bit fields for DMA IFC */
#define _DMA_IFC_RESETVALUE                             0x00000000UL                      /**< Default value for DMA_IFC */
#define _DMA_IFC_MASK                                   0x80000FFFUL                      /**< Mask for DMA_IFC */
#define DMA_IFC_CH0DONE                                 (0x1UL << 0)                      /**< DMA Channel 0 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH0DONE_SHIFT                          0                                 /**< Shift value for DMA_CH0DONE */
#define _DMA_IFC_CH0DONE_MASK                           0x1UL                             /**< Bit mask for DMA_CH0DONE */
#define _DMA_IFC_CH0DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH0DONE_DEFAULT                         (_DMA_IFC_CH0DONE_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH1DONE                                 (0x1UL << 1)                      /**< DMA Channel 1 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH1DONE_SHIFT                          1                                 /**< Shift value for DMA_CH1DONE */
#define _DMA_IFC_CH1DONE_MASK                           0x2UL                             /**< Bit mask for DMA_CH1DONE */
#define _DMA_IFC_CH1DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH1DONE_DEFAULT                         (_DMA_IFC_CH1DONE_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH2DONE                                 (0x1UL << 2)                      /**< DMA Channel 2 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH2DONE_SHIFT                          2                                 /**< Shift value for DMA_CH2DONE */
#define _DMA_IFC_CH2DONE_MASK                           0x4UL                             /**< Bit mask for DMA_CH2DONE */
#define _DMA_IFC_CH2DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH2DONE_DEFAULT                         (_DMA_IFC_CH2DONE_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH3DONE                                 (0x1UL << 3)                      /**< DMA Channel 3 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH3DONE_SHIFT                          3                                 /**< Shift value for DMA_CH3DONE */
#define _DMA_IFC_CH3DONE_MASK                           0x8UL                             /**< Bit mask for DMA_CH3DONE */
#define _DMA_IFC_CH3DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH3DONE_DEFAULT                         (_DMA_IFC_CH3DONE_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH4DONE                                 (0x1UL << 4)                      /**< DMA Channel 4 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH4DONE_SHIFT                          4                                 /**< Shift value for DMA_CH4DONE */
#define _DMA_IFC_CH4DONE_MASK                           0x10UL                            /**< Bit mask for DMA_CH4DONE */
#define _DMA_IFC_CH4DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH4DONE_DEFAULT                         (_DMA_IFC_CH4DONE_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH5DONE                                 (0x1UL << 5)                      /**< DMA Channel 5 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH5DONE_SHIFT                          5                                 /**< Shift value for DMA_CH5DONE */
#define _DMA_IFC_CH5DONE_MASK                           0x20UL                            /**< Bit mask for DMA_CH5DONE */
#define _DMA_IFC_CH5DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH5DONE_DEFAULT                         (_DMA_IFC_CH5DONE_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH6DONE                                 (0x1UL << 6)                      /**< DMA Channel 6 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH6DONE_SHIFT                          6                                 /**< Shift value for DMA_CH6DONE */
#define _DMA_IFC_CH6DONE_MASK                           0x40UL                            /**< Bit mask for DMA_CH6DONE */
#define _DMA_IFC_CH6DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH6DONE_DEFAULT                         (_DMA_IFC_CH6DONE_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH7DONE                                 (0x1UL << 7)                      /**< DMA Channel 7 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH7DONE_SHIFT                          7                                 /**< Shift value for DMA_CH7DONE */
#define _DMA_IFC_CH7DONE_MASK                           0x80UL                            /**< Bit mask for DMA_CH7DONE */
#define _DMA_IFC_CH7DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH7DONE_DEFAULT                         (_DMA_IFC_CH7DONE_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH8DONE                                 (0x1UL << 8)                      /**< DMA Channel 8 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH8DONE_SHIFT                          8                                 /**< Shift value for DMA_CH8DONE */
#define _DMA_IFC_CH8DONE_MASK                           0x100UL                           /**< Bit mask for DMA_CH8DONE */
#define _DMA_IFC_CH8DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH8DONE_DEFAULT                         (_DMA_IFC_CH8DONE_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH9DONE                                 (0x1UL << 9)                      /**< DMA Channel 9 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH9DONE_SHIFT                          9                                 /**< Shift value for DMA_CH9DONE */
#define _DMA_IFC_CH9DONE_MASK                           0x200UL                           /**< Bit mask for DMA_CH9DONE */
#define _DMA_IFC_CH9DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH9DONE_DEFAULT                         (_DMA_IFC_CH9DONE_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH10DONE                                (0x1UL << 10)                     /**< DMA Channel 10 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH10DONE_SHIFT                         10                                /**< Shift value for DMA_CH10DONE */
#define _DMA_IFC_CH10DONE_MASK                          0x400UL                           /**< Bit mask for DMA_CH10DONE */
#define _DMA_IFC_CH10DONE_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH10DONE_DEFAULT                        (_DMA_IFC_CH10DONE_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH11DONE                                (0x1UL << 11)                     /**< DMA Channel 11 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH11DONE_SHIFT                         11                                /**< Shift value for DMA_CH11DONE */
#define _DMA_IFC_CH11DONE_MASK                          0x800UL                           /**< Bit mask for DMA_CH11DONE */
#define _DMA_IFC_CH11DONE_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH11DONE_DEFAULT                        (_DMA_IFC_CH11DONE_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_ERR                                     (0x1UL << 31)                     /**< DMA Error Interrupt Flag Clear */
#define _DMA_IFC_ERR_SHIFT                              31                                /**< Shift value for DMA_ERR */
#define _DMA_IFC_ERR_MASK                               0x80000000UL                      /**< Bit mask for DMA_ERR */
#define _DMA_IFC_ERR_DEFAULT                            0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_ERR_DEFAULT                             (_DMA_IFC_ERR_DEFAULT << 31)      /**< Shifted mode DEFAULT for DMA_IFC */

/* Bit fields for DMA IEN */
#define _DMA_IEN_RESETVALUE                             0x00000000UL                      /**< Default value for DMA_IEN */
#define _DMA_IEN_MASK                                   0x80000FFFUL                      /**< Mask for DMA_IEN */
#define DMA_IEN_CH0DONE                                 (0x1UL << 0)                      /**< DMA Channel 0 Complete Interrupt Enable */
#define _DMA_IEN_CH0DONE_SHIFT                          0                                 /**< Shift value for DMA_CH0DONE */
#define _DMA_IEN_CH0DONE_MASK                           0x1UL                             /**< Bit mask for DMA_CH0DONE */
#define _DMA_IEN_CH0DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH0DONE_DEFAULT                         (_DMA_IEN_CH0DONE_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH1DONE                                 (0x1UL << 1)                      /**< DMA Channel 1 Complete Interrupt Enable */
#define _DMA_IEN_CH1DONE_SHIFT                          1                                 /**< Shift value for DMA_CH1DONE */
#define _DMA_IEN_CH1DONE_MASK                           0x2UL                             /**< Bit mask for DMA_CH1DONE */
#define _DMA_IEN_CH1DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH1DONE_DEFAULT                         (_DMA_IEN_CH1DONE_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH2DONE                                 (0x1UL << 2)                      /**< DMA Channel 2 Complete Interrupt Enable */
#define _DMA_IEN_CH2DONE_SHIFT                          2                                 /**< Shift value for DMA_CH2DONE */
#define _DMA_IEN_CH2DONE_MASK                           0x4UL                             /**< Bit mask for DMA_CH2DONE */
#define _DMA_IEN_CH2DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH2DONE_DEFAULT                         (_DMA_IEN_CH2DONE_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH3DONE                                 (0x1UL << 3)                      /**< DMA Channel 3 Complete Interrupt Enable */
#define _DMA_IEN_CH3DONE_SHIFT                          3                                 /**< Shift value for DMA_CH3DONE */
#define _DMA_IEN_CH3DONE_MASK                           0x8UL                             /**< Bit mask for DMA_CH3DONE */
#define _DMA_IEN_CH3DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH3DONE_DEFAULT                         (_DMA_IEN_CH3DONE_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH4DONE                                 (0x1UL << 4)                      /**< DMA Channel 4 Complete Interrupt Enable */
#define _DMA_IEN_CH4DONE_SHIFT                          4                                 /**< Shift value for DMA_CH4DONE */
#define _DMA_IEN_CH4DONE_MASK                           0x10UL                            /**< Bit mask for DMA_CH4DONE */
#define _DMA_IEN_CH4DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH4DONE_DEFAULT                         (_DMA_IEN_CH4DONE_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH5DONE                                 (0x1UL << 5)                      /**< DMA Channel 5 Complete Interrupt Enable */
#define _DMA_IEN_CH5DONE_SHIFT                          5                                 /**< Shift value for DMA_CH5DONE */
#define _DMA_IEN_CH5DONE_MASK                           0x20UL                            /**< Bit mask for DMA_CH5DONE */
#define _DMA_IEN_CH5DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH5DONE_DEFAULT                         (_DMA_IEN_CH5DONE_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH6DONE                                 (0x1UL << 6)                      /**< DMA Channel 6 Complete Interrupt Enable */
#define _DMA_IEN_CH6DONE_SHIFT                          6                                 /**< Shift value for DMA_CH6DONE */
#define _DMA_IEN_CH6DONE_MASK                           0x40UL                            /**< Bit mask for DMA_CH6DONE */
#define _DMA_IEN_CH6DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH6DONE_DEFAULT                         (_DMA_IEN_CH6DONE_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH7DONE                                 (0x1UL << 7)                      /**< DMA Channel 7 Complete Interrupt Enable */
#define _DMA_IEN_CH7DONE_SHIFT                          7                                 /**< Shift value for DMA_CH7DONE */
#define _DMA_IEN_CH7DONE_MASK                           0x80UL                            /**< Bit mask for DMA_CH7DONE */
#define _DMA_IEN_CH7DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH7DONE_DEFAULT                         (_DMA_IEN_CH7DONE_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH8DONE                                 (0x1UL << 8)                      /**< DMA Channel 8 Complete Interrupt Enable */
#define _DMA_IEN_CH8DONE_SHIFT                          8                                 /**< Shift value for DMA_CH8DONE */
#define _DMA_IEN_CH8DONE_MASK                           0x100UL                           /**< Bit mask for DMA_CH8DONE */
#define _DMA_IEN_CH8DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH8DONE_DEFAULT                         (_DMA_IEN_CH8DONE_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH9DONE                                 (0x1UL << 9)                      /**< DMA Channel 9 Complete Interrupt Enable */
#define _DMA_IEN_CH9DONE_SHIFT                          9                                 /**< Shift value for DMA_CH9DONE */
#define _DMA_IEN_CH9DONE_MASK                           0x200UL                           /**< Bit mask for DMA_CH9DONE */
#define _DMA_IEN_CH9DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH9DONE_DEFAULT                         (_DMA_IEN_CH9DONE_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH10DONE                                (0x1UL << 10)                     /**< DMA Channel 10 Complete Interrupt Enable */
#define _DMA_IEN_CH10DONE_SHIFT                         10                                /**< Shift value for DMA_CH10DONE */
#define _DMA_IEN_CH10DONE_MASK                          0x400UL                           /**< Bit mask for DMA_CH10DONE */
#define _DMA_IEN_CH10DONE_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH10DONE_DEFAULT                        (_DMA_IEN_CH10DONE_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH11DONE                                (0x1UL << 11)                     /**< DMA Channel 11 Complete Interrupt Enable */
#define _DMA_IEN_CH11DONE_SHIFT                         11                                /**< Shift value for DMA_CH11DONE */
#define _DMA_IEN_CH11DONE_MASK                          0x800UL                           /**< Bit mask for DMA_CH11DONE */
#define _DMA_IEN_CH11DONE_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH11DONE_DEFAULT                        (_DMA_IEN_CH11DONE_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_ERR                                     (0x1UL << 31)                     /**< DMA Error Interrupt Flag Enable */
#define _DMA_IEN_ERR_SHIFT                              31                                /**< Shift value for DMA_ERR */
#define _DMA_IEN_ERR_MASK                               0x80000000UL                      /**< Bit mask for DMA_ERR */
#define _DMA_IEN_ERR_DEFAULT                            0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_ERR_DEFAULT                             (_DMA_IEN_ERR_DEFAULT << 31)      /**< Shifted mode DEFAULT for DMA_IEN */

/* Bit fields for DMA CTRL */
#define _DMA_CTRL_RESETVALUE                            0x00000000UL                      /**< Default value for DMA_CTRL */
#define _DMA_CTRL_MASK                                  0x00000003UL                      /**< Mask for DMA_CTRL */
#define DMA_CTRL_DESCRECT                               (0x1UL << 0)                      /**< Descriptor Specifies Rectangle */
#define _DMA_CTRL_DESCRECT_SHIFT                        0                                 /**< Shift value for DMA_DESCRECT */
#define _DMA_CTRL_DESCRECT_MASK                         0x1UL                             /**< Bit mask for DMA_DESCRECT */
#define _DMA_CTRL_DESCRECT_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for DMA_CTRL */
#define DMA_CTRL_DESCRECT_DEFAULT                       (_DMA_CTRL_DESCRECT_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CTRL */
#define DMA_CTRL_PRDU                                   (0x1UL << 1)                      /**< Prevent Rect Descriptor Update */
#define _DMA_CTRL_PRDU_SHIFT                            1                                 /**< Shift value for DMA_PRDU */
#define _DMA_CTRL_PRDU_MASK                             0x2UL                             /**< Bit mask for DMA_PRDU */
#define _DMA_CTRL_PRDU_DEFAULT                          0x00000000UL                      /**< Mode DEFAULT for DMA_CTRL */
#define DMA_CTRL_PRDU_DEFAULT                           (_DMA_CTRL_PRDU_DEFAULT << 1)     /**< Shifted mode DEFAULT for DMA_CTRL */

/* Bit fields for DMA RDS */
#define _DMA_RDS_RESETVALUE                             0x00000000UL                     /**< Default value for DMA_RDS */
#define _DMA_RDS_MASK                                   0x00000FFFUL                     /**< Mask for DMA_RDS */
#define DMA_RDS_RDSCH0                                  (0x1UL << 0)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH0_SHIFT                           0                                /**< Shift value for DMA_RDSCH0 */
#define _DMA_RDS_RDSCH0_MASK                            0x1UL                            /**< Bit mask for DMA_RDSCH0 */
#define _DMA_RDS_RDSCH0_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH0_DEFAULT                          (_DMA_RDS_RDSCH0_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH1                                  (0x1UL << 1)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH1_SHIFT                           1                                /**< Shift value for DMA_RDSCH1 */
#define _DMA_RDS_RDSCH1_MASK                            0x2UL                            /**< Bit mask for DMA_RDSCH1 */
#define _DMA_RDS_RDSCH1_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH1_DEFAULT                          (_DMA_RDS_RDSCH1_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH2                                  (0x1UL << 2)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH2_SHIFT                           2                                /**< Shift value for DMA_RDSCH2 */
#define _DMA_RDS_RDSCH2_MASK                            0x4UL                            /**< Bit mask for DMA_RDSCH2 */
#define _DMA_RDS_RDSCH2_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH2_DEFAULT                          (_DMA_RDS_RDSCH2_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH3                                  (0x1UL << 3)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH3_SHIFT                           3                                /**< Shift value for DMA_RDSCH3 */
#define _DMA_RDS_RDSCH3_MASK                            0x8UL                            /**< Bit mask for DMA_RDSCH3 */
#define _DMA_RDS_RDSCH3_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH3_DEFAULT                          (_DMA_RDS_RDSCH3_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH4                                  (0x1UL << 4)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH4_SHIFT                           4                                /**< Shift value for DMA_RDSCH4 */
#define _DMA_RDS_RDSCH4_MASK                            0x10UL                           /**< Bit mask for DMA_RDSCH4 */
#define _DMA_RDS_RDSCH4_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH4_DEFAULT                          (_DMA_RDS_RDSCH4_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH5                                  (0x1UL << 5)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH5_SHIFT                           5                                /**< Shift value for DMA_RDSCH5 */
#define _DMA_RDS_RDSCH5_MASK                            0x20UL                           /**< Bit mask for DMA_RDSCH5 */
#define _DMA_RDS_RDSCH5_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH5_DEFAULT                          (_DMA_RDS_RDSCH5_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH6                                  (0x1UL << 6)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH6_SHIFT                           6                                /**< Shift value for DMA_RDSCH6 */
#define _DMA_RDS_RDSCH6_MASK                            0x40UL                           /**< Bit mask for DMA_RDSCH6 */
#define _DMA_RDS_RDSCH6_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH6_DEFAULT                          (_DMA_RDS_RDSCH6_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH7                                  (0x1UL << 7)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH7_SHIFT                           7                                /**< Shift value for DMA_RDSCH7 */
#define _DMA_RDS_RDSCH7_MASK                            0x80UL                           /**< Bit mask for DMA_RDSCH7 */
#define _DMA_RDS_RDSCH7_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH7_DEFAULT                          (_DMA_RDS_RDSCH7_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH8                                  (0x1UL << 8)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH8_SHIFT                           8                                /**< Shift value for DMA_RDSCH8 */
#define _DMA_RDS_RDSCH8_MASK                            0x100UL                          /**< Bit mask for DMA_RDSCH8 */
#define _DMA_RDS_RDSCH8_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH8_DEFAULT                          (_DMA_RDS_RDSCH8_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH9                                  (0x1UL << 9)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH9_SHIFT                           9                                /**< Shift value for DMA_RDSCH9 */
#define _DMA_RDS_RDSCH9_MASK                            0x200UL                          /**< Bit mask for DMA_RDSCH9 */
#define _DMA_RDS_RDSCH9_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH9_DEFAULT                          (_DMA_RDS_RDSCH9_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH10                                 (0x1UL << 10)                    /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH10_SHIFT                          10                               /**< Shift value for DMA_RDSCH10 */
#define _DMA_RDS_RDSCH10_MASK                           0x400UL                          /**< Bit mask for DMA_RDSCH10 */
#define _DMA_RDS_RDSCH10_DEFAULT                        0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH10_DEFAULT                         (_DMA_RDS_RDSCH10_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH11                                 (0x1UL << 11)                    /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH11_SHIFT                          11                               /**< Shift value for DMA_RDSCH11 */
#define _DMA_RDS_RDSCH11_MASK                           0x800UL                          /**< Bit mask for DMA_RDSCH11 */
#define _DMA_RDS_RDSCH11_DEFAULT                        0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH11_DEFAULT                         (_DMA_RDS_RDSCH11_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_RDS */

/* Bit fields for DMA LOOP0 */
#define _DMA_LOOP0_RESETVALUE                           0x00000000UL                    /**< Default value for DMA_LOOP0 */
#define _DMA_LOOP0_MASK                                 0x000103FFUL                    /**< Mask for DMA_LOOP0 */
#define _DMA_LOOP0_WIDTH_SHIFT                          0                               /**< Shift value for DMA_WIDTH */
#define _DMA_LOOP0_WIDTH_MASK                           0x3FFUL                         /**< Bit mask for DMA_WIDTH */
#define _DMA_LOOP0_WIDTH_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_LOOP0 */
#define DMA_LOOP0_WIDTH_DEFAULT                         (_DMA_LOOP0_WIDTH_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_LOOP0 */
#define DMA_LOOP0_EN                                    (0x1UL << 16)                   /**< DMA Channel 0 Loop Enable */
#define _DMA_LOOP0_EN_SHIFT                             16                              /**< Shift value for DMA_EN */
#define _DMA_LOOP0_EN_MASK                              0x10000UL                       /**< Bit mask for DMA_EN */
#define _DMA_LOOP0_EN_DEFAULT                           0x00000000UL                    /**< Mode DEFAULT for DMA_LOOP0 */
#define DMA_LOOP0_EN_DEFAULT                            (_DMA_LOOP0_EN_DEFAULT << 16)   /**< Shifted mode DEFAULT for DMA_LOOP0 */

/* Bit fields for DMA LOOP1 */
#define _DMA_LOOP1_RESETVALUE                           0x00000000UL                    /**< Default value for DMA_LOOP1 */
#define _DMA_LOOP1_MASK                                 0x000103FFUL                    /**< Mask for DMA_LOOP1 */
#define _DMA_LOOP1_WIDTH_SHIFT                          0                               /**< Shift value for DMA_WIDTH */
#define _DMA_LOOP1_WIDTH_MASK                           0x3FFUL                         /**< Bit mask for DMA_WIDTH */
#define _DMA_LOOP1_WIDTH_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_LOOP1 */
#define DMA_LOOP1_WIDTH_DEFAULT                         (_DMA_LOOP1_WIDTH_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_LOOP1 */
#define DMA_LOOP1_EN                                    (0x1UL << 16)                   /**< DMA Channel 1 Loop Enable */
#define _DMA_LOOP1_EN_SHIFT                             16                              /**< Shift value for DMA_EN */
#define _DMA_LOOP1_EN_MASK                              0x10000UL                       /**< Bit mask for DMA_EN */
#define _DMA_LOOP1_EN_DEFAULT                           0x00000000UL                    /**< Mode DEFAULT for DMA_LOOP1 */
#define DMA_LOOP1_EN_DEFAULT                            (_DMA_LOOP1_EN_DEFAULT << 16)   /**< Shifted mode DEFAULT for DMA_LOOP1 */

/* Bit fields for DMA RECT0 */
#define _DMA_RECT0_RESETVALUE                           0x00000000UL                         /**< Default value for DMA_RECT0 */
#define _DMA_RECT0_MASK                                 0xFFFFFFFFUL                         /**< Mask for DMA_RECT0 */
#define _DMA_RECT0_HEIGHT_SHIFT                         0                                    /**< Shift value for DMA_HEIGHT */
#define _DMA_RECT0_HEIGHT_MASK                          0x3FFUL                              /**< Bit mask for DMA_HEIGHT */
#define _DMA_RECT0_HEIGHT_DEFAULT                       0x00000000UL                         /**< Mode DEFAULT for DMA_RECT0 */
#define DMA_RECT0_HEIGHT_DEFAULT                        (_DMA_RECT0_HEIGHT_DEFAULT << 0)     /**< Shifted mode DEFAULT for DMA_RECT0 */
#define _DMA_RECT0_SRCSTRIDE_SHIFT                      10                                   /**< Shift value for DMA_SRCSTRIDE */
#define _DMA_RECT0_SRCSTRIDE_MASK                       0x1FFC00UL                           /**< Bit mask for DMA_SRCSTRIDE */
#define _DMA_RECT0_SRCSTRIDE_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_RECT0 */
#define DMA_RECT0_SRCSTRIDE_DEFAULT                     (_DMA_RECT0_SRCSTRIDE_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_RECT0 */
#define _DMA_RECT0_DSTSTRIDE_SHIFT                      21                                   /**< Shift value for DMA_DSTSTRIDE */
#define _DMA_RECT0_DSTSTRIDE_MASK                       0xFFE00000UL                         /**< Bit mask for DMA_DSTSTRIDE */
#define _DMA_RECT0_DSTSTRIDE_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_RECT0 */
#define DMA_RECT0_DSTSTRIDE_DEFAULT                     (_DMA_RECT0_DSTSTRIDE_DEFAULT << 21) /**< Shifted mode DEFAULT for DMA_RECT0 */

/* Bit fields for DMA CH_CTRL */
#define _DMA_CH_CTRL_RESETVALUE                         0x00000000UL                                  /**< Default value for DMA_CH_CTRL */
#define _DMA_CH_CTRL_MASK                               0x003F000FUL                                  /**< Mask for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_SHIFT                       0                                             /**< Shift value for DMA_SIGSEL */
#define _DMA_CH_CTRL_SIGSEL_MASK                        0xFUL                                         /**< Bit mask for DMA_SIGSEL */
#define _DMA_CH_CTRL_SIGSEL_ADC0SINGLE                  0x00000000UL                                  /**< Mode ADC0SINGLE for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_DAC0CH0                     0x00000000UL                                  /**< Mode DAC0CH0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART0RXDATAV               0x00000000UL                                  /**< Mode USART0RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1RXDATAV               0x00000000UL                                  /**< Mode USART1RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART2RXDATAV               0x00000000UL                                  /**< Mode USART2RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART0RXDATAV              0x00000000UL                                  /**< Mode LEUART0RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART1RXDATAV              0x00000000UL                                  /**< Mode LEUART1RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_I2C0RXDATAV                 0x00000000UL                                  /**< Mode I2C0RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_I2C1RXDATAV                 0x00000000UL                                  /**< Mode I2C1RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER0UFOF                  0x00000000UL                                  /**< Mode TIMER0UFOF for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER1UFOF                  0x00000000UL                                  /**< Mode TIMER1UFOF for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER2UFOF                  0x00000000UL                                  /**< Mode TIMER2UFOF for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER3UFOF                  0x00000000UL                                  /**< Mode TIMER3UFOF for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_MSCWDATA                    0x00000000UL                                  /**< Mode MSCWDATA for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_AESDATAWR                   0x00000000UL                                  /**< Mode AESDATAWR for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LESENSEBUFDATAV             0x00000000UL                                  /**< Mode LESENSEBUFDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_ADC0SCAN                    0x00000001UL                                  /**< Mode ADC0SCAN for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_DAC0CH1                     0x00000001UL                                  /**< Mode DAC0CH1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART0TXBL                  0x00000001UL                                  /**< Mode USART0TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1TXBL                  0x00000001UL                                  /**< Mode USART1TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART2TXBL                  0x00000001UL                                  /**< Mode USART2TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART0TXBL                 0x00000001UL                                  /**< Mode LEUART0TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART1TXBL                 0x00000001UL                                  /**< Mode LEUART1TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_I2C0TXBL                    0x00000001UL                                  /**< Mode I2C0TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_I2C1TXBL                    0x00000001UL                                  /**< Mode I2C1TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER0CC0                   0x00000001UL                                  /**< Mode TIMER0CC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER1CC0                   0x00000001UL                                  /**< Mode TIMER1CC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER2CC0                   0x00000001UL                                  /**< Mode TIMER2CC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER3CC0                   0x00000001UL                                  /**< Mode TIMER3CC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_AESXORDATAWR                0x00000001UL                                  /**< Mode AESXORDATAWR for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART0TXEMPTY               0x00000002UL                                  /**< Mode USART0TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1TXEMPTY               0x00000002UL                                  /**< Mode USART1TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART2TXEMPTY               0x00000002UL                                  /**< Mode USART2TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART0TXEMPTY              0x00000002UL                                  /**< Mode LEUART0TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART1TXEMPTY              0x00000002UL                                  /**< Mode LEUART1TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER0CC1                   0x00000002UL                                  /**< Mode TIMER0CC1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER1CC1                   0x00000002UL                                  /**< Mode TIMER1CC1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER2CC1                   0x00000002UL                                  /**< Mode TIMER2CC1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER3CC1                   0x00000002UL                                  /**< Mode TIMER3CC1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_AESDATARD                   0x00000002UL                                  /**< Mode AESDATARD for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1RXDATAVRIGHT          0x00000003UL                                  /**< Mode USART1RXDATAVRIGHT for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART2RXDATAVRIGHT          0x00000003UL                                  /**< Mode USART2RXDATAVRIGHT for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER0CC2                   0x00000003UL                                  /**< Mode TIMER0CC2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER1CC2                   0x00000003UL                                  /**< Mode TIMER1CC2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER2CC2                   0x00000003UL                                  /**< Mode TIMER2CC2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER3CC2                   0x00000003UL                                  /**< Mode TIMER3CC2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_AESKEYWR                    0x00000003UL                                  /**< Mode AESKEYWR for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1TXBLRIGHT             0x00000004UL                                  /**< Mode USART1TXBLRIGHT for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART2TXBLRIGHT             0x00000004UL                                  /**< Mode USART2TXBLRIGHT for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_ADC0SINGLE                   (_DMA_CH_CTRL_SIGSEL_ADC0SINGLE << 0)         /**< Shifted mode ADC0SINGLE for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_DAC0CH0                      (_DMA_CH_CTRL_SIGSEL_DAC0CH0 << 0)            /**< Shifted mode DAC0CH0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART0RXDATAV                (_DMA_CH_CTRL_SIGSEL_USART0RXDATAV << 0)      /**< Shifted mode USART0RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1RXDATAV                (_DMA_CH_CTRL_SIGSEL_USART1RXDATAV << 0)      /**< Shifted mode USART1RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART2RXDATAV                (_DMA_CH_CTRL_SIGSEL_USART2RXDATAV << 0)      /**< Shifted mode USART2RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART0RXDATAV               (_DMA_CH_CTRL_SIGSEL_LEUART0RXDATAV << 0)     /**< Shifted mode LEUART0RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART1RXDATAV               (_DMA_CH_CTRL_SIGSEL_LEUART1RXDATAV << 0)     /**< Shifted mode LEUART1RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_I2C0RXDATAV                  (_DMA_CH_CTRL_SIGSEL_I2C0RXDATAV << 0)        /**< Shifted mode I2C0RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_I2C1RXDATAV                  (_DMA_CH_CTRL_SIGSEL_I2C1RXDATAV << 0)        /**< Shifted mode I2C1RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER0UFOF                   (_DMA_CH_CTRL_SIGSEL_TIMER0UFOF << 0)         /**< Shifted mode TIMER0UFOF for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER1UFOF                   (_DMA_CH_CTRL_SIGSEL_TIMER1UFOF << 0)         /**< Shifted mode TIMER1UFOF for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER2UFOF                   (_DMA_CH_CTRL_SIGSEL_TIMER2UFOF << 0)         /**< Shifted mode TIMER2UFOF for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER3UFOF                   (_DMA_CH_CTRL_SIGSEL_TIMER3UFOF << 0)         /**< Shifted mode TIMER3UFOF for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_MSCWDATA                     (_DMA_CH_CTRL_SIGSEL_MSCWDATA << 0)           /**< Shifted mode MSCWDATA for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_AESDATAWR                    (_DMA_CH_CTRL_SIGSEL_AESDATAWR << 0)          /**< Shifted mode AESDATAWR for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LESENSEBUFDATAV              (_DMA_CH_CTRL_SIGSEL_LESENSEBUFDATAV << 0)    /**< Shifted mode LESENSEBUFDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_ADC0SCAN                     (_DMA_CH_CTRL_SIGSEL_ADC0SCAN << 0)           /**< Shifted mode ADC0SCAN for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_DAC0CH1                      (_DMA_CH_CTRL_SIGSEL_DAC0CH1 << 0)            /**< Shifted mode DAC0CH1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART0TXBL                   (_DMA_CH_CTRL_SIGSEL_USART0TXBL << 0)         /**< Shifted mode USART0TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1TXBL                   (_DMA_CH_CTRL_SIGSEL_USART1TXBL << 0)         /**< Shifted mode USART1TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART2TXBL                   (_DMA_CH_CTRL_SIGSEL_USART2TXBL << 0)         /**< Shifted mode USART2TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART0TXBL                  (_DMA_CH_CTRL_SIGSEL_LEUART0TXBL << 0)        /**< Shifted mode LEUART0TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART1TXBL                  (_DMA_CH_CTRL_SIGSEL_LEUART1TXBL << 0)        /**< Shifted mode LEUART1TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_I2C0TXBL                     (_DMA_CH_CTRL_SIGSEL_I2C0TXBL << 0)           /**< Shifted mode I2C0TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_I2C1TXBL                     (_DMA_CH_CTRL_SIGSEL_I2C1TXBL << 0)           /**< Shifted mode I2C1TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER0CC0                    (_DMA_CH_CTRL_SIGSEL_TIMER0CC0 << 0)          /**< Shifted mode TIMER0CC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER1CC0                    (_DMA_CH_CTRL_SIGSEL_TIMER1CC0 << 0)          /**< Shifted mode TIMER1CC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER2CC0                    (_DMA_CH_CTRL_SIGSEL_TIMER2CC0 << 0)          /**< Shifted mode TIMER2CC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER3CC0                    (_DMA_CH_CTRL_SIGSEL_TIMER3CC0 << 0)          /**< Shifted mode TIMER3CC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_AESXORDATAWR                 (_DMA_CH_CTRL_SIGSEL_AESXORDATAWR << 0)       /**< Shifted mode AESXORDATAWR for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART0TXEMPTY                (_DMA_CH_CTRL_SIGSEL_USART0TXEMPTY << 0)      /**< Shifted mode USART0TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1TXEMPTY                (_DMA_CH_CTRL_SIGSEL_USART1TXEMPTY << 0)      /**< Shifted mode USART1TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART2TXEMPTY                (_DMA_CH_CTRL_SIGSEL_USART2TXEMPTY << 0)      /**< Shifted mode USART2TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART0TXEMPTY               (_DMA_CH_CTRL_SIGSEL_LEUART0TXEMPTY << 0)     /**< Shifted mode LEUART0TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART1TXEMPTY               (_DMA_CH_CTRL_SIGSEL_LEUART1TXEMPTY << 0)     /**< Shifted mode LEUART1TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER0CC1                    (_DMA_CH_CTRL_SIGSEL_TIMER0CC1 << 0)          /**< Shifted mode TIMER0CC1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER1CC1                    (_DMA_CH_CTRL_SIGSEL_TIMER1CC1 << 0)          /**< Shifted mode TIMER1CC1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER2CC1                    (_DMA_CH_CTRL_SIGSEL_TIMER2CC1 << 0)          /**< Shifted mode TIMER2CC1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER3CC1                    (_DMA_CH_CTRL_SIGSEL_TIMER3CC1 << 0)          /**< Shifted mode TIMER3CC1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_AESDATARD                    (_DMA_CH_CTRL_SIGSEL_AESDATARD << 0)          /**< Shifted mode AESDATARD for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1RXDATAVRIGHT           (_DMA_CH_CTRL_SIGSEL_USART1RXDATAVRIGHT << 0) /**< Shifted mode USART1RXDATAVRIGHT for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART2RXDATAVRIGHT           (_DMA_CH_CTRL_SIGSEL_USART2RXDATAVRIGHT << 0) /**< Shifted mode USART2RXDATAVRIGHT for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER0CC2                    (_DMA_CH_CTRL_SIGSEL_TIMER0CC2 << 0)          /**< Shifted mode TIMER0CC2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER1CC2                    (_DMA_CH_CTRL_SIGSEL_TIMER1CC2 << 0)          /**< Shifted mode TIMER1CC2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER2CC2                    (_DMA_CH_CTRL_SIGSEL_TIMER2CC2 << 0)          /**< Shifted mode TIMER2CC2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER3CC2                    (_DMA_CH_CTRL_SIGSEL_TIMER3CC2 << 0)          /**< Shifted mode TIMER3CC2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_AESKEYWR                     (_DMA_CH_CTRL_SIGSEL_AESKEYWR << 0)           /**< Shifted mode AESKEYWR for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1TXBLRIGHT              (_DMA_CH_CTRL_SIGSEL_USART1TXBLRIGHT << 0)    /**< Shifted mode USART1TXBLRIGHT for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART2TXBLRIGHT              (_DMA_CH_CTRL_SIGSEL_USART2TXBLRIGHT << 0)    /**< Shifted mode USART2TXBLRIGHT for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_SHIFT                    16                                            /**< Shift value for DMA_SOURCESEL */
#define _DMA_CH_CTRL_SOURCESEL_MASK                     0x3F0000UL                                    /**< Bit mask for DMA_SOURCESEL */
#define _DMA_CH_CTRL_SOURCESEL_NONE                     0x00000000UL                                  /**< Mode NONE for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_ADC0                     0x00000008UL                                  /**< Mode ADC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_DAC0                     0x0000000AUL                                  /**< Mode DAC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_USART0                   0x0000000CUL                                  /**< Mode USART0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_USART1                   0x0000000DUL                                  /**< Mode USART1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_USART2                   0x0000000EUL                                  /**< Mode USART2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_LEUART0                  0x00000010UL                                  /**< Mode LEUART0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_LEUART1                  0x00000011UL                                  /**< Mode LEUART1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_I2C0                     0x00000014UL                                  /**< Mode I2C0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_I2C1                     0x00000015UL                                  /**< Mode I2C1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_TIMER0                   0x00000018UL                                  /**< Mode TIMER0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_TIMER1                   0x00000019UL                                  /**< Mode TIMER1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_TIMER2                   0x0000001AUL                                  /**< Mode TIMER2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_TIMER3                   0x0000001BUL                                  /**< Mode TIMER3 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_MSC                      0x00000030UL                                  /**< Mode MSC for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_AES                      0x00000031UL                                  /**< Mode AES for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_LESENSE                  0x00000032UL                                  /**< Mode LESENSE for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_NONE                      (_DMA_CH_CTRL_SOURCESEL_NONE << 16)           /**< Shifted mode NONE for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_ADC0                      (_DMA_CH_CTRL_SOURCESEL_ADC0 << 16)           /**< Shifted mode ADC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_DAC0                      (_DMA_CH_CTRL_SOURCESEL_DAC0 << 16)           /**< Shifted mode DAC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_USART0                    (_DMA_CH_CTRL_SOURCESEL_USART0 << 16)         /**< Shifted mode USART0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_USART1                    (_DMA_CH_CTRL_SOURCESEL_USART1 << 16)         /**< Shifted mode USART1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_USART2                    (_DMA_CH_CTRL_SOURCESEL_USART2 << 16)         /**< Shifted mode USART2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_LEUART0                   (_DMA_CH_CTRL_SOURCESEL_LEUART0 << 16)        /**< Shifted mode LEUART0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_LEUART1                   (_DMA_CH_CTRL_SOURCESEL_LEUART1 << 16)        /**< Shifted mode LEUART1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_I2C0                      (_DMA_CH_CTRL_SOURCESEL_I2C0 << 16)           /**< Shifted mode I2C0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_I2C1                      (_DMA_CH_CTRL_SOURCESEL_I2C1 << 16)           /**< Shifted mode I2C1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_TIMER0                    (_DMA_CH_CTRL_SOURCESEL_TIMER0 << 16)         /**< Shifted mode TIMER0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_TIMER1                    (_DMA_CH_CTRL_SOURCESEL_TIMER1 << 16)         /**< Shifted mode TIMER1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_TIMER2                    (_DMA_CH_CTRL_SOURCESEL_TIMER2 << 16)         /**< Shifted mode TIMER2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_TIMER3                    (_DMA_CH_CTRL_SOURCESEL_TIMER3 << 16)         /**< Shifted mode TIMER3 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_MSC                       (_DMA_CH_CTRL_SOURCESEL_MSC << 16)            /**< Shifted mode MSC for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_AES                       (_DMA_CH_CTRL_SOURCESEL_AES << 16)            /**< Shifted mode AES for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_LESENSE                   (_DMA_CH_CTRL_SOURCESEL_LESENSE << 16)        /**< Shifted mode LESENSE for DMA_CH_CTRL */

/** @} End of group EFM32WG942F64_DMA */



/**************************************************************************//**
 * @defgroup EFM32WG942F64_CMU_BitFields  EFM32WG942F64_CMU Bit Fields
 * @{
 *****************************************************************************/

/* Bit fields for CMU CTRL */
#define _CMU_CTRL_RESETVALUE                        0x000C262CUL                                /**< Default value for CMU_CTRL */
#define _CMU_CTRL_MASK                              0x57FFFEEFUL                                /**< Mask for CMU_CTRL */
#define _CMU_CTRL_HFXOMODE_SHIFT                    0                                           /**< Shift value for CMU_HFXOMODE */
#define _CMU_CTRL_HFXOMODE_MASK                     0x3UL                                       /**< Bit mask for CMU_HFXOMODE */
#define _CMU_CTRL_HFXOMODE_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_HFXOMODE_XTAL                     0x00000000UL                                /**< Mode XTAL for CMU_CTRL */
#define _CMU_CTRL_HFXOMODE_BUFEXTCLK                0x00000001UL                                /**< Mode BUFEXTCLK for CMU_CTRL */
#define _CMU_CTRL_HFXOMODE_DIGEXTCLK                0x00000002UL                                /**< Mode DIGEXTCLK for CMU_CTRL */
#define CMU_CTRL_HFXOMODE_DEFAULT                   (_CMU_CTRL_HFXOMODE_DEFAULT << 0)           /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFXOMODE_XTAL                      (_CMU_CTRL_HFXOMODE_XTAL << 0)              /**< Shifted mode XTAL for CMU_CTRL */
#define CMU_CTRL_HFXOMODE_BUFEXTCLK                 (_CMU_CTRL_HFXOMODE_BUFEXTCLK << 0)         /**< Shifted mode BUFEXTCLK for CMU_CTRL */
#define CMU_CTRL_HFXOMODE_DIGEXTCLK                 (_CMU_CTRL_HFXOMODE_DIGEXTCLK << 0)         /**< Shifted mode DIGEXTCLK for CMU_CTRL */
#define _CMU_CTRL_HFXOBOOST_SHIFT                   2                                           /**< Shift value for CMU_HFXOBOOST */
#define _CMU_CTRL_HFXOBOOST_MASK                    0xCUL                                       /**< Bit mask for CMU_HFXOBOOST */
#define _CMU_CTRL_HFXOBOOST_50PCENT                 0x00000000UL                                /**< Mode 50PCENT for CMU_CTRL */
#define _CMU_CTRL_HFXOBOOST_70PCENT                 0x00000001UL                                /**< Mode 70PCENT for CMU_CTRL */
#define _CMU_CTRL_HFXOBOOST_80PCENT                 0x00000002UL                                /**< Mode 80PCENT for CMU_CTRL */
#define _CMU_CTRL_HFXOBOOST_DEFAULT                 0x00000003UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_HFXOBOOST_100PCENT                0x00000003UL                                /**< Mode 100PCENT for CMU_CTRL */
#define CMU_CTRL_HFXOBOOST_50PCENT                  (_CMU_CTRL_HFXOBOOST_50PCENT << 2)          /**< Shifted mode 50PCENT for CMU_CTRL */
#define CMU_CTRL_HFXOBOOST_70PCENT                  (_CMU_CTRL_HFXOBOOST_70PCENT << 2)          /**< Shifted mode 70PCENT for CMU_CTRL */
#define CMU_CTRL_HFXOBOOST_80PCENT                  (_CMU_CTRL_HFXOBOOST_80PCENT << 2)          /**< Shifted mode 80PCENT for CMU_CTRL */
#define CMU_CTRL_HFXOBOOST_DEFAULT                  (_CMU_CTRL_HFXOBOOST_DEFAULT << 2)          /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFXOBOOST_100PCENT                 (_CMU_CTRL_HFXOBOOST_100PCENT << 2)         /**< Shifted mode 100PCENT for CMU_CTRL */
#define _CMU_CTRL_HFXOBUFCUR_SHIFT                  5                                           /**< Shift value for CMU_HFXOBUFCUR */
#define _CMU_CTRL_HFXOBUFCUR_MASK                   0x60UL                                      /**< Bit mask for CMU_HFXOBUFCUR */
#define _CMU_CTRL_HFXOBUFCUR_DEFAULT                0x00000001UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_HFXOBUFCUR_BOOSTUPTO32MHZ         0x00000001UL                                /**< Mode BOOSTUPTO32MHZ for CMU_CTRL */
#define _CMU_CTRL_HFXOBUFCUR_BOOSTABOVE32MHZ        0x00000003UL                                /**< Mode BOOSTABOVE32MHZ for CMU_CTRL */
#define CMU_CTRL_HFXOBUFCUR_DEFAULT                 (_CMU_CTRL_HFXOBUFCUR_DEFAULT << 5)         /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFXOBUFCUR_BOOSTUPTO32MHZ          (_CMU_CTRL_HFXOBUFCUR_BOOSTUPTO32MHZ << 5)  /**< Shifted mode BOOSTUPTO32MHZ for CMU_CTRL */
#define CMU_CTRL_HFXOBUFCUR_BOOSTABOVE32MHZ         (_CMU_CTRL_HFXOBUFCUR_BOOSTABOVE32MHZ << 5) /**< Shifted mode BOOSTABOVE32MHZ for CMU_CTRL */
#define CMU_CTRL_HFXOGLITCHDETEN                    (0x1UL << 7)                                /**< HFXO Glitch Detector Enable */
#define _CMU_CTRL_HFXOGLITCHDETEN_SHIFT             7                                           /**< Shift value for CMU_HFXOGLITCHDETEN */
#define _CMU_CTRL_HFXOGLITCHDETEN_MASK              0x80UL                                      /**< Bit mask for CMU_HFXOGLITCHDETEN */
#define _CMU_CTRL_HFXOGLITCHDETEN_DEFAULT           0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFXOGLITCHDETEN_DEFAULT            (_CMU_CTRL_HFXOGLITCHDETEN_DEFAULT << 7)    /**< Shifted mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_HFXOTIMEOUT_SHIFT                 9                                           /**< Shift value for CMU_HFXOTIMEOUT */
#define _CMU_CTRL_HFXOTIMEOUT_MASK                  0x600UL                                     /**< Bit mask for CMU_HFXOTIMEOUT */
#define _CMU_CTRL_HFXOTIMEOUT_8CYCLES               0x00000000UL                                /**< Mode 8CYCLES for CMU_CTRL */
#define _CMU_CTRL_HFXOTIMEOUT_256CYCLES             0x00000001UL                                /**< Mode 256CYCLES for CMU_CTRL */
#define _CMU_CTRL_HFXOTIMEOUT_1KCYCLES              0x00000002UL                                /**< Mode 1KCYCLES for CMU_CTRL */
#define _CMU_CTRL_HFXOTIMEOUT_DEFAULT               0x00000003UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_HFXOTIMEOUT_16KCYCLES             0x00000003UL                                /**< Mode 16KCYCLES for CMU_CTRL */
#define CMU_CTRL_HFXOTIMEOUT_8CYCLES                (_CMU_CTRL_HFXOTIMEOUT_8CYCLES << 9)        /**< Shifted mode 8CYCLES for CMU_CTRL */
#define CMU_CTRL_HFXOTIMEOUT_256CYCLES              (_CMU_CTRL_HFXOTIMEOUT_256CYCLES << 9)      /**< Shifted mode 256CYCLES for CMU_CTRL */
#define CMU_CTRL_HFXOTIMEOUT_1KCYCLES               (_CMU_CTRL_HFXOTIMEOUT_1KCYCLES << 9)       /**< Shifted mode 1KCYCLES for CMU_CTRL */
#define CMU_CTRL_HFXOTIMEOUT_DEFAULT                (_CMU_CTRL_HFXOTIMEOUT_DEFAULT << 9)        /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFXOTIMEOUT_16KCYCLES              (_CMU_CTRL_HFXOTIMEOUT_16KCYCLES << 9)      /**< Shifted mode 16KCYCLES for CMU_CTRL */
#define _CMU_CTRL_LFXOMODE_SHIFT                    11                                          /**< Shift value for CMU_LFXOMODE */
#define _CMU_CTRL_LFXOMODE_MASK                     0x1800UL                                    /**< Bit mask for CMU_LFXOMODE */
#define _CMU_CTRL_LFXOMODE_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_LFXOMODE_XTAL                     0x00000000UL                                /**< Mode XTAL for CMU_CTRL */
#define _CMU_CTRL_LFXOMODE_BUFEXTCLK                0x00000001UL                                /**< Mode BUFEXTCLK for CMU_CTRL */
#define _CMU_CTRL_LFXOMODE_DIGEXTCLK                0x00000002UL                                /**< Mode DIGEXTCLK for CMU_CTRL */
#define CMU_CTRL_LFXOMODE_DEFAULT                   (_CMU_CTRL_LFXOMODE_DEFAULT << 11)          /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_LFXOMODE_XTAL                      (_CMU_CTRL_LFXOMODE_XTAL << 11)             /**< Shifted mode XTAL for CMU_CTRL */
#define CMU_CTRL_LFXOMODE_BUFEXTCLK                 (_CMU_CTRL_LFXOMODE_BUFEXTCLK << 11)        /**< Shifted mode BUFEXTCLK for CMU_CTRL */
#define CMU_CTRL_LFXOMODE_DIGEXTCLK                 (_CMU_CTRL_LFXOMODE_DIGEXTCLK << 11)        /**< Shifted mode DIGEXTCLK for CMU_CTRL */
#define CMU_CTRL_LFXOBOOST                          (0x1UL << 13)                               /**< LFXO Start-up Boost Current */
#define _CMU_CTRL_LFXOBOOST_SHIFT                   13                                          /**< Shift value for CMU_LFXOBOOST */
#define _CMU_CTRL_LFXOBOOST_MASK                    0x2000UL                                    /**< Bit mask for CMU_LFXOBOOST */
#define _CMU_CTRL_LFXOBOOST_70PCENT                 0x00000000UL                                /**< Mode 70PCENT for CMU_CTRL */
#define _CMU_CTRL_LFXOBOOST_DEFAULT                 0x00000001UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_LFXOBOOST_100PCENT                0x00000001UL                                /**< Mode 100PCENT for CMU_CTRL */
#define CMU_CTRL_LFXOBOOST_70PCENT                  (_CMU_CTRL_LFXOBOOST_70PCENT << 13)         /**< Shifted mode 70PCENT for CMU_CTRL */
#define CMU_CTRL_LFXOBOOST_DEFAULT                  (_CMU_CTRL_LFXOBOOST_DEFAULT << 13)         /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_LFXOBOOST_100PCENT                 (_CMU_CTRL_LFXOBOOST_100PCENT << 13)        /**< Shifted mode 100PCENT for CMU_CTRL */
#define _CMU_CTRL_HFCLKDIV_SHIFT                    14                                          /**< Shift value for CMU_HFCLKDIV */
#define _CMU_CTRL_HFCLKDIV_MASK                     0x1C000UL                                   /**< Bit mask for CMU_HFCLKDIV */
#define _CMU_CTRL_HFCLKDIV_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFCLKDIV_DEFAULT                   (_CMU_CTRL_HFCLKDIV_DEFAULT << 14)          /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_LFXOBUFCUR                         (0x1UL << 17)                               /**< LFXO Boost Buffer Current */
#define _CMU_CTRL_LFXOBUFCUR_SHIFT                  17                                          /**< Shift value for CMU_LFXOBUFCUR */
#define _CMU_CTRL_LFXOBUFCUR_MASK                   0x20000UL                                   /**< Bit mask for CMU_LFXOBUFCUR */
#define _CMU_CTRL_LFXOBUFCUR_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_LFXOBUFCUR_DEFAULT                 (_CMU_CTRL_LFXOBUFCUR_DEFAULT << 17)        /**< Shifted mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_LFXOTIMEOUT_SHIFT                 18                                          /**< Shift value for CMU_LFXOTIMEOUT */
#define _CMU_CTRL_LFXOTIMEOUT_MASK                  0xC0000UL                                   /**< Bit mask for CMU_LFXOTIMEOUT */
#define _CMU_CTRL_LFXOTIMEOUT_8CYCLES               0x00000000UL                                /**< Mode 8CYCLES for CMU_CTRL */
#define _CMU_CTRL_LFXOTIMEOUT_1KCYCLES              0x00000001UL                                /**< Mode 1KCYCLES for CMU_CTRL */
#define _CMU_CTRL_LFXOTIMEOUT_16KCYCLES             0x00000002UL                                /**< Mode 16KCYCLES for CMU_CTRL */
#define _CMU_CTRL_LFXOTIMEOUT_DEFAULT               0x00000003UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_LFXOTIMEOUT_32KCYCLES             0x00000003UL                                /**< Mode 32KCYCLES for CMU_CTRL */
#define CMU_CTRL_LFXOTIMEOUT_8CYCLES                (_CMU_CTRL_LFXOTIMEOUT_8CYCLES << 18)       /**< Shifted mode 8CYCLES for CMU_CTRL */
#define CMU_CTRL_LFXOTIMEOUT_1KCYCLES               (_CMU_CTRL_LFXOTIMEOUT_1KCYCLES << 18)      /**< Shifted mode 1KCYCLES for CMU_CTRL */
#define CMU_CTRL_LFXOTIMEOUT_16KCYCLES              (_CMU_CTRL_LFXOTIMEOUT_16KCYCLES << 18)     /**< Shifted mode 16KCYCLES for CMU_CTRL */
#define CMU_CTRL_LFXOTIMEOUT_DEFAULT                (_CMU_CTRL_LFXOTIMEOUT_DEFAULT << 18)       /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_LFXOTIMEOUT_32KCYCLES              (_CMU_CTRL_LFXOTIMEOUT_32KCYCLES << 18)     /**< Shifted mode 32KCYCLES for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_SHIFT                  20                                          /**< Shift value for CMU_CLKOUTSEL0 */
#define _CMU_CTRL_CLKOUTSEL0_MASK                   0x700000UL                                  /**< Bit mask for CMU_CLKOUTSEL0 */
#define _CMU_CTRL_CLKOUTSEL0_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFRCO                  0x00000000UL                                /**< Mode HFRCO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFXO                   0x00000001UL                                /**< Mode HFXO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFCLK2                 0x00000002UL                                /**< Mode HFCLK2 for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFCLK4                 0x00000003UL                                /**< Mode HFCLK4 for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFCLK8                 0x00000004UL                                /**< Mode HFCLK8 for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFCLK16                0x00000005UL                                /**< Mode HFCLK16 for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_ULFRCO                 0x00000006UL                                /**< Mode ULFRCO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_AUXHFRCO               0x00000007UL                                /**< Mode AUXHFRCO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_DEFAULT                 (_CMU_CTRL_CLKOUTSEL0_DEFAULT << 20)        /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFRCO                   (_CMU_CTRL_CLKOUTSEL0_HFRCO << 20)          /**< Shifted mode HFRCO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFXO                    (_CMU_CTRL_CLKOUTSEL0_HFXO << 20)           /**< Shifted mode HFXO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFCLK2                  (_CMU_CTRL_CLKOUTSEL0_HFCLK2 << 20)         /**< Shifted mode HFCLK2 for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFCLK4                  (_CMU_CTRL_CLKOUTSEL0_HFCLK4 << 20)         /**< Shifted mode HFCLK4 for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFCLK8                  (_CMU_CTRL_CLKOUTSEL0_HFCLK8 << 20)         /**< Shifted mode HFCLK8 for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFCLK16                 (_CMU_CTRL_CLKOUTSEL0_HFCLK16 << 20)        /**< Shifted mode HFCLK16 for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_ULFRCO                  (_CMU_CTRL_CLKOUTSEL0_ULFRCO << 20)         /**< Shifted mode ULFRCO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_AUXHFRCO                (_CMU_CTRL_CLKOUTSEL0_AUXHFRCO << 20)       /**< Shifted mode AUXHFRCO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_SHIFT                  23                                          /**< Shift value for CMU_CLKOUTSEL1 */
#define _CMU_CTRL_CLKOUTSEL1_MASK                   0x7800000UL                                 /**< Bit mask for CMU_CLKOUTSEL1 */
#define _CMU_CTRL_CLKOUTSEL1_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_LFRCO                  0x00000000UL                                /**< Mode LFRCO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_LFXO                   0x00000001UL                                /**< Mode LFXO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_HFCLK                  0x00000002UL                                /**< Mode HFCLK for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_LFXOQ                  0x00000003UL                                /**< Mode LFXOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_HFXOQ                  0x00000004UL                                /**< Mode HFXOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_LFRCOQ                 0x00000005UL                                /**< Mode LFRCOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_HFRCOQ                 0x00000006UL                                /**< Mode HFRCOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_AUXHFRCOQ              0x00000007UL                                /**< Mode AUXHFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_DEFAULT                 (_CMU_CTRL_CLKOUTSEL1_DEFAULT << 23)        /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_LFRCO                   (_CMU_CTRL_CLKOUTSEL1_LFRCO << 23)          /**< Shifted mode LFRCO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_LFXO                    (_CMU_CTRL_CLKOUTSEL1_LFXO << 23)           /**< Shifted mode LFXO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_HFCLK                   (_CMU_CTRL_CLKOUTSEL1_HFCLK << 23)          /**< Shifted mode HFCLK for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_LFXOQ                   (_CMU_CTRL_CLKOUTSEL1_LFXOQ << 23)          /**< Shifted mode LFXOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_HFXOQ                   (_CMU_CTRL_CLKOUTSEL1_HFXOQ << 23)          /**< Shifted mode HFXOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_LFRCOQ                  (_CMU_CTRL_CLKOUTSEL1_LFRCOQ << 23)         /**< Shifted mode LFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_HFRCOQ                  (_CMU_CTRL_CLKOUTSEL1_HFRCOQ << 23)         /**< Shifted mode HFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_AUXHFRCOQ               (_CMU_CTRL_CLKOUTSEL1_AUXHFRCOQ << 23)      /**< Shifted mode AUXHFRCOQ for CMU_CTRL */
#define CMU_CTRL_DBGCLK                             (0x1UL << 28)                               /**< Debug Clock */
#define _CMU_CTRL_DBGCLK_SHIFT                      28                                          /**< Shift value for CMU_DBGCLK */
#define _CMU_CTRL_DBGCLK_MASK                       0x10000000UL                                /**< Bit mask for CMU_DBGCLK */
#define _CMU_CTRL_DBGCLK_DEFAULT                    0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_DBGCLK_AUXHFRCO                   0x00000000UL                                /**< Mode AUXHFRCO for CMU_CTRL */
#define _CMU_CTRL_DBGCLK_HFCLK                      0x00000001UL                                /**< Mode HFCLK for CMU_CTRL */
#define CMU_CTRL_DBGCLK_DEFAULT                     (_CMU_CTRL_DBGCLK_DEFAULT << 28)            /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_DBGCLK_AUXHFRCO                    (_CMU_CTRL_DBGCLK_AUXHFRCO << 28)           /**< Shifted mode AUXHFRCO for CMU_CTRL */
#define CMU_CTRL_DBGCLK_HFCLK                       (_CMU_CTRL_DBGCLK_HFCLK << 28)              /**< Shifted mode HFCLK for CMU_CTRL */
#define CMU_CTRL_HFLE                               (0x1UL << 30)                               /**< High-Frequency LE Interface */
#define _CMU_CTRL_HFLE_SHIFT                        30                                          /**< Shift value for CMU_HFLE */
#define _CMU_CTRL_HFLE_MASK                         0x40000000UL                                /**< Bit mask for CMU_HFLE */
#define _CMU_CTRL_HFLE_DEFAULT                      0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFLE_DEFAULT                       (_CMU_CTRL_HFLE_DEFAULT << 30)              /**< Shifted mode DEFAULT for CMU_CTRL */

/* Bit fields for CMU HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_RESETVALUE                0x00000000UL                                    /**< Default value for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_MASK                      0x0000010FUL                                    /**< Mask for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_SHIFT        0                                               /**< Shift value for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_MASK         0xFUL                                           /**< Bit mask for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_DEFAULT      0x00000000UL                                    /**< Mode DEFAULT for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK        0x00000000UL                                    /**< Mode HFCLK for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK2       0x00000001UL                                    /**< Mode HFCLK2 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK4       0x00000002UL                                    /**< Mode HFCLK4 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK8       0x00000003UL                                    /**< Mode HFCLK8 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK16      0x00000004UL                                    /**< Mode HFCLK16 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK32      0x00000005UL                                    /**< Mode HFCLK32 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK64      0x00000006UL                                    /**< Mode HFCLK64 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK128     0x00000007UL                                    /**< Mode HFCLK128 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK256     0x00000008UL                                    /**< Mode HFCLK256 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK512     0x00000009UL                                    /**< Mode HFCLK512 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_DEFAULT       (_CMU_HFCORECLKDIV_HFCORECLKDIV_DEFAULT << 0)   /**< Shifted mode DEFAULT for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK         (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK << 0)     /**< Shifted mode HFCLK for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK2        (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK2 << 0)    /**< Shifted mode HFCLK2 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK4        (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK4 << 0)    /**< Shifted mode HFCLK4 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK8        (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK8 << 0)    /**< Shifted mode HFCLK8 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK16       (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK16 << 0)   /**< Shifted mode HFCLK16 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK32       (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK32 << 0)   /**< Shifted mode HFCLK32 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK64       (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK64 << 0)   /**< Shifted mode HFCLK64 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK128      (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK128 << 0)  /**< Shifted mode HFCLK128 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK256      (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK256 << 0)  /**< Shifted mode HFCLK256 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK512      (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK512 << 0)  /**< Shifted mode HFCLK512 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKLEDIV             (0x1UL << 8)                                    /**< Additional Division Factor For HFCORECLKLE */
#define _CMU_HFCORECLKDIV_HFCORECLKLEDIV_SHIFT      8                                               /**< Shift value for CMU_HFCORECLKLEDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKLEDIV_MASK       0x100UL                                         /**< Bit mask for CMU_HFCORECLKLEDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKLEDIV_DEFAULT    0x00000000UL                                    /**< Mode DEFAULT for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKLEDIV_DIV2       0x00000000UL                                    /**< Mode DIV2 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKLEDIV_DIV4       0x00000001UL                                    /**< Mode DIV4 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKLEDIV_DEFAULT     (_CMU_HFCORECLKDIV_HFCORECLKLEDIV_DEFAULT << 8) /**< Shifted mode DEFAULT for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKLEDIV_DIV2        (_CMU_HFCORECLKDIV_HFCORECLKLEDIV_DIV2 << 8)    /**< Shifted mode DIV2 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKLEDIV_DIV4        (_CMU_HFCORECLKDIV_HFCORECLKLEDIV_DIV4 << 8)    /**< Shifted mode DIV4 for CMU_HFCORECLKDIV */

/* Bit fields for CMU HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_RESETVALUE                 0x00000100UL                                 /**< Default value for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_MASK                       0x0000010FUL                                 /**< Mask for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_SHIFT          0                                            /**< Shift value for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_MASK           0xFUL                                        /**< Bit mask for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_DEFAULT        0x00000000UL                                 /**< Mode DEFAULT for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK          0x00000000UL                                 /**< Mode HFCLK for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK2         0x00000001UL                                 /**< Mode HFCLK2 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK4         0x00000002UL                                 /**< Mode HFCLK4 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK8         0x00000003UL                                 /**< Mode HFCLK8 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK16        0x00000004UL                                 /**< Mode HFCLK16 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK32        0x00000005UL                                 /**< Mode HFCLK32 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK64        0x00000006UL                                 /**< Mode HFCLK64 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK128       0x00000007UL                                 /**< Mode HFCLK128 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK256       0x00000008UL                                 /**< Mode HFCLK256 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK512       0x00000009UL                                 /**< Mode HFCLK512 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_DEFAULT         (_CMU_HFPERCLKDIV_HFPERCLKDIV_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK           (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK << 0)    /**< Shifted mode HFCLK for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK2          (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK2 << 0)   /**< Shifted mode HFCLK2 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK4          (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK4 << 0)   /**< Shifted mode HFCLK4 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK8          (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK8 << 0)   /**< Shifted mode HFCLK8 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK16         (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK16 << 0)  /**< Shifted mode HFCLK16 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK32         (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK32 << 0)  /**< Shifted mode HFCLK32 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK64         (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK64 << 0)  /**< Shifted mode HFCLK64 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK128        (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK128 << 0) /**< Shifted mode HFCLK128 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK256        (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK256 << 0) /**< Shifted mode HFCLK256 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK512        (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK512 << 0) /**< Shifted mode HFCLK512 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKEN                  (0x1UL << 8)                                 /**< HFPERCLK Enable */
#define _CMU_HFPERCLKDIV_HFPERCLKEN_SHIFT           8                                            /**< Shift value for CMU_HFPERCLKEN */
#define _CMU_HFPERCLKDIV_HFPERCLKEN_MASK            0x100UL                                      /**< Bit mask for CMU_HFPERCLKEN */
#define _CMU_HFPERCLKDIV_HFPERCLKEN_DEFAULT         0x00000001UL                                 /**< Mode DEFAULT for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKEN_DEFAULT          (_CMU_HFPERCLKDIV_HFPERCLKEN_DEFAULT << 8)   /**< Shifted mode DEFAULT for CMU_HFPERCLKDIV */

/* Bit fields for CMU HFRCOCTRL */
#define _CMU_HFRCOCTRL_RESETVALUE                   0x00000380UL                           /**< Default value for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_MASK                         0x0001F7FFUL                           /**< Mask for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_TUNING_SHIFT                 0                                      /**< Shift value for CMU_TUNING */
#define _CMU_HFRCOCTRL_TUNING_MASK                  0xFFUL                                 /**< Bit mask for CMU_TUNING */
#define _CMU_HFRCOCTRL_TUNING_DEFAULT               0x00000080UL                           /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_TUNING_DEFAULT                (_CMU_HFRCOCTRL_TUNING_DEFAULT << 0)   /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_SHIFT                   8                                      /**< Shift value for CMU_BAND */
#define _CMU_HFRCOCTRL_BAND_MASK                    0x700UL                                /**< Bit mask for CMU_BAND */
#define _CMU_HFRCOCTRL_BAND_1MHZ                    0x00000000UL                           /**< Mode 1MHZ for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_7MHZ                    0x00000001UL                           /**< Mode 7MHZ for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_11MHZ                   0x00000002UL                           /**< Mode 11MHZ for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_DEFAULT                 0x00000003UL                           /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_14MHZ                   0x00000003UL                           /**< Mode 14MHZ for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_21MHZ                   0x00000004UL                           /**< Mode 21MHZ for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_28MHZ                   0x00000005UL                           /**< Mode 28MHZ for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_1MHZ                     (_CMU_HFRCOCTRL_BAND_1MHZ << 8)        /**< Shifted mode 1MHZ for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_7MHZ                     (_CMU_HFRCOCTRL_BAND_7MHZ << 8)        /**< Shifted mode 7MHZ for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_11MHZ                    (_CMU_HFRCOCTRL_BAND_11MHZ << 8)       /**< Shifted mode 11MHZ for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_DEFAULT                  (_CMU_HFRCOCTRL_BAND_DEFAULT << 8)     /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_14MHZ                    (_CMU_HFRCOCTRL_BAND_14MHZ << 8)       /**< Shifted mode 14MHZ for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_21MHZ                    (_CMU_HFRCOCTRL_BAND_21MHZ << 8)       /**< Shifted mode 21MHZ for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_28MHZ                    (_CMU_HFRCOCTRL_BAND_28MHZ << 8)       /**< Shifted mode 28MHZ for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_SUDELAY_SHIFT                12                                     /**< Shift value for CMU_SUDELAY */
#define _CMU_HFRCOCTRL_SUDELAY_MASK                 0x1F000UL                              /**< Bit mask for CMU_SUDELAY */
#define _CMU_HFRCOCTRL_SUDELAY_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_SUDELAY_DEFAULT               (_CMU_HFRCOCTRL_SUDELAY_DEFAULT << 12) /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */

/* Bit fields for CMU LFRCOCTRL */
#define _CMU_LFRCOCTRL_RESETVALUE                   0x00000040UL                         /**< Default value for CMU_LFRCOCTRL */
#define _CMU_LFRCOCTRL_MASK                         0x0000007FUL                         /**< Mask for CMU_LFRCOCTRL */
#define _CMU_LFRCOCTRL_TUNING_SHIFT                 0                                    /**< Shift value for CMU_TUNING */
#define _CMU_LFRCOCTRL_TUNING_MASK                  0x7FUL                               /**< Bit mask for CMU_TUNING */
#define _CMU_LFRCOCTRL_TUNING_DEFAULT               0x00000040UL                         /**< Mode DEFAULT for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_TUNING_DEFAULT                (_CMU_LFRCOCTRL_TUNING_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_LFRCOCTRL */

/* Bit fields for CMU AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_RESETVALUE                0x00000080UL                            /**< Default value for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_MASK                      0x000007FFUL                            /**< Mask for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_TUNING_SHIFT              0                                       /**< Shift value for CMU_TUNING */
#define _CMU_AUXHFRCOCTRL_TUNING_MASK               0xFFUL                                  /**< Bit mask for CMU_TUNING */
#define _CMU_AUXHFRCOCTRL_TUNING_DEFAULT            0x00000080UL                            /**< Mode DEFAULT for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_TUNING_DEFAULT             (_CMU_AUXHFRCOCTRL_TUNING_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_SHIFT                8                                       /**< Shift value for CMU_BAND */
#define _CMU_AUXHFRCOCTRL_BAND_MASK                 0x700UL                                 /**< Bit mask for CMU_BAND */
#define _CMU_AUXHFRCOCTRL_BAND_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_14MHZ                0x00000000UL                            /**< Mode 14MHZ for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_11MHZ                0x00000001UL                            /**< Mode 11MHZ for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_7MHZ                 0x00000002UL                            /**< Mode 7MHZ for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_1MHZ                 0x00000003UL                            /**< Mode 1MHZ for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_28MHZ                0x00000006UL                            /**< Mode 28MHZ for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_21MHZ                0x00000007UL                            /**< Mode 21MHZ for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_DEFAULT               (_CMU_AUXHFRCOCTRL_BAND_DEFAULT << 8)   /**< Shifted mode DEFAULT for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_14MHZ                 (_CMU_AUXHFRCOCTRL_BAND_14MHZ << 8)     /**< Shifted mode 14MHZ for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_11MHZ                 (_CMU_AUXHFRCOCTRL_BAND_11MHZ << 8)     /**< Shifted mode 11MHZ for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_7MHZ                  (_CMU_AUXHFRCOCTRL_BAND_7MHZ << 8)      /**< Shifted mode 7MHZ for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_1MHZ                  (_CMU_AUXHFRCOCTRL_BAND_1MHZ << 8)      /**< Shifted mode 1MHZ for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_28MHZ                 (_CMU_AUXHFRCOCTRL_BAND_28MHZ << 8)     /**< Shifted mode 28MHZ for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_21MHZ                 (_CMU_AUXHFRCOCTRL_BAND_21MHZ << 8)     /**< Shifted mode 21MHZ for CMU_AUXHFRCOCTRL */

/* Bit fields for CMU CALCTRL */
#define _CMU_CALCTRL_RESETVALUE                     0x00000000UL                         /**< Default value for CMU_CALCTRL */
#define _CMU_CALCTRL_MASK                           0x0000007FUL                         /**< Mask for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_SHIFT                    0                                    /**< Shift value for CMU_UPSEL */
#define _CMU_CALCTRL_UPSEL_MASK                     0x7UL                                /**< Bit mask for CMU_UPSEL */
#define _CMU_CALCTRL_UPSEL_DEFAULT                  0x00000000UL                         /**< Mode DEFAULT for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_HFXO                     0x00000000UL                         /**< Mode HFXO for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_LFXO                     0x00000001UL                         /**< Mode LFXO for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_HFRCO                    0x00000002UL                         /**< Mode HFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_LFRCO                    0x00000003UL                         /**< Mode LFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_AUXHFRCO                 0x00000004UL                         /**< Mode AUXHFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_DEFAULT                   (_CMU_CALCTRL_UPSEL_DEFAULT << 0)    /**< Shifted mode DEFAULT for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_HFXO                      (_CMU_CALCTRL_UPSEL_HFXO << 0)       /**< Shifted mode HFXO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_LFXO                      (_CMU_CALCTRL_UPSEL_LFXO << 0)       /**< Shifted mode LFXO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_HFRCO                     (_CMU_CALCTRL_UPSEL_HFRCO << 0)      /**< Shifted mode HFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_LFRCO                     (_CMU_CALCTRL_UPSEL_LFRCO << 0)      /**< Shifted mode LFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_AUXHFRCO                  (_CMU_CALCTRL_UPSEL_AUXHFRCO << 0)   /**< Shifted mode AUXHFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_SHIFT                  3                                    /**< Shift value for CMU_DOWNSEL */
#define _CMU_CALCTRL_DOWNSEL_MASK                   0x38UL                               /**< Bit mask for CMU_DOWNSEL */
#define _CMU_CALCTRL_DOWNSEL_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_HFCLK                  0x00000000UL                         /**< Mode HFCLK for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_HFXO                   0x00000001UL                         /**< Mode HFXO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_LFXO                   0x00000002UL                         /**< Mode LFXO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_HFRCO                  0x00000003UL                         /**< Mode HFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_LFRCO                  0x00000004UL                         /**< Mode LFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_AUXHFRCO               0x00000005UL                         /**< Mode AUXHFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_DEFAULT                 (_CMU_CALCTRL_DOWNSEL_DEFAULT << 3)  /**< Shifted mode DEFAULT for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_HFCLK                   (_CMU_CALCTRL_DOWNSEL_HFCLK << 3)    /**< Shifted mode HFCLK for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_HFXO                    (_CMU_CALCTRL_DOWNSEL_HFXO << 3)     /**< Shifted mode HFXO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_LFXO                    (_CMU_CALCTRL_DOWNSEL_LFXO << 3)     /**< Shifted mode LFXO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_HFRCO                   (_CMU_CALCTRL_DOWNSEL_HFRCO << 3)    /**< Shifted mode HFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_LFRCO                   (_CMU_CALCTRL_DOWNSEL_LFRCO << 3)    /**< Shifted mode LFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_AUXHFRCO                (_CMU_CALCTRL_DOWNSEL_AUXHFRCO << 3) /**< Shifted mode AUXHFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_CONT                            (0x1UL << 6)                         /**< Continuous Calibration */
#define _CMU_CALCTRL_CONT_SHIFT                     6                                    /**< Shift value for CMU_CONT */
#define _CMU_CALCTRL_CONT_MASK                      0x40UL                               /**< Bit mask for CMU_CONT */
#define _CMU_CALCTRL_CONT_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_CALCTRL */
#define CMU_CALCTRL_CONT_DEFAULT                    (_CMU_CALCTRL_CONT_DEFAULT << 6)     /**< Shifted mode DEFAULT for CMU_CALCTRL */

/* Bit fields for CMU CALCNT */
#define _CMU_CALCNT_RESETVALUE                      0x00000000UL                      /**< Default value for CMU_CALCNT */
#define _CMU_CALCNT_MASK                            0x000FFFFFUL                      /**< Mask for CMU_CALCNT */
#define _CMU_CALCNT_CALCNT_SHIFT                    0                                 /**< Shift value for CMU_CALCNT */
#define _CMU_CALCNT_CALCNT_MASK                     0xFFFFFUL                         /**< Bit mask for CMU_CALCNT */
#define _CMU_CALCNT_CALCNT_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for CMU_CALCNT */
#define CMU_CALCNT_CALCNT_DEFAULT                   (_CMU_CALCNT_CALCNT_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_CALCNT */

/* Bit fields for CMU OSCENCMD */
#define _CMU_OSCENCMD_RESETVALUE                    0x00000000UL                             /**< Default value for CMU_OSCENCMD */
#define _CMU_OSCENCMD_MASK                          0x000003FFUL                             /**< Mask for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFRCOEN                        (0x1UL << 0)                             /**< HFRCO Enable */
#define _CMU_OSCENCMD_HFRCOEN_SHIFT                 0                                        /**< Shift value for CMU_HFRCOEN */
#define _CMU_OSCENCMD_HFRCOEN_MASK                  0x1UL                                    /**< Bit mask for CMU_HFRCOEN */
#define _CMU_OSCENCMD_HFRCOEN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFRCOEN_DEFAULT                (_CMU_OSCENCMD_HFRCOEN_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFRCODIS                       (0x1UL << 1)                             /**< HFRCO Disable */
#define _CMU_OSCENCMD_HFRCODIS_SHIFT                1                                        /**< Shift value for CMU_HFRCODIS */
#define _CMU_OSCENCMD_HFRCODIS_MASK                 0x2UL                                    /**< Bit mask for CMU_HFRCODIS */
#define _CMU_OSCENCMD_HFRCODIS_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFRCODIS_DEFAULT               (_CMU_OSCENCMD_HFRCODIS_DEFAULT << 1)    /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFXOEN                         (0x1UL << 2)                             /**< HFXO Enable */
#define _CMU_OSCENCMD_HFXOEN_SHIFT                  2                                        /**< Shift value for CMU_HFXOEN */
#define _CMU_OSCENCMD_HFXOEN_MASK                   0x4UL                                    /**< Bit mask for CMU_HFXOEN */
#define _CMU_OSCENCMD_HFXOEN_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFXOEN_DEFAULT                 (_CMU_OSCENCMD_HFXOEN_DEFAULT << 2)      /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFXODIS                        (0x1UL << 3)                             /**< HFXO Disable */
#define _CMU_OSCENCMD_HFXODIS_SHIFT                 3                                        /**< Shift value for CMU_HFXODIS */
#define _CMU_OSCENCMD_HFXODIS_MASK                  0x8UL                                    /**< Bit mask for CMU_HFXODIS */
#define _CMU_OSCENCMD_HFXODIS_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFXODIS_DEFAULT                (_CMU_OSCENCMD_HFXODIS_DEFAULT << 3)     /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_AUXHFRCOEN                     (0x1UL << 4)                             /**< AUXHFRCO Enable */
#define _CMU_OSCENCMD_AUXHFRCOEN_SHIFT              4                                        /**< Shift value for CMU_AUXHFRCOEN */
#define _CMU_OSCENCMD_AUXHFRCOEN_MASK               0x10UL                                   /**< Bit mask for CMU_AUXHFRCOEN */
#define _CMU_OSCENCMD_AUXHFRCOEN_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_AUXHFRCOEN_DEFAULT             (_CMU_OSCENCMD_AUXHFRCOEN_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_AUXHFRCODIS                    (0x1UL << 5)                             /**< AUXHFRCO Disable */
#define _CMU_OSCENCMD_AUXHFRCODIS_SHIFT             5                                        /**< Shift value for CMU_AUXHFRCODIS */
#define _CMU_OSCENCMD_AUXHFRCODIS_MASK              0x20UL                                   /**< Bit mask for CMU_AUXHFRCODIS */
#define _CMU_OSCENCMD_AUXHFRCODIS_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_AUXHFRCODIS_DEFAULT            (_CMU_OSCENCMD_AUXHFRCODIS_DEFAULT << 5) /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFRCOEN                        (0x1UL << 6)                             /**< LFRCO Enable */
#define _CMU_OSCENCMD_LFRCOEN_SHIFT                 6                                        /**< Shift value for CMU_LFRCOEN */
#define _CMU_OSCENCMD_LFRCOEN_MASK                  0x40UL                                   /**< Bit mask for CMU_LFRCOEN */
#define _CMU_OSCENCMD_LFRCOEN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFRCOEN_DEFAULT                (_CMU_OSCENCMD_LFRCOEN_DEFAULT << 6)     /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFRCODIS                       (0x1UL << 7)                             /**< LFRCO Disable */
#define _CMU_OSCENCMD_LFRCODIS_SHIFT                7                                        /**< Shift value for CMU_LFRCODIS */
#define _CMU_OSCENCMD_LFRCODIS_MASK                 0x80UL                                   /**< Bit mask for CMU_LFRCODIS */
#define _CMU_OSCENCMD_LFRCODIS_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFRCODIS_DEFAULT               (_CMU_OSCENCMD_LFRCODIS_DEFAULT << 7)    /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFXOEN                         (0x1UL << 8)                             /**< LFXO Enable */
#define _CMU_OSCENCMD_LFXOEN_SHIFT                  8                                        /**< Shift value for CMU_LFXOEN */
#define _CMU_OSCENCMD_LFXOEN_MASK                   0x100UL                                  /**< Bit mask for CMU_LFXOEN */
#define _CMU_OSCENCMD_LFXOEN_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFXOEN_DEFAULT                 (_CMU_OSCENCMD_LFXOEN_DEFAULT << 8)      /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFXODIS                        (0x1UL << 9)                             /**< LFXO Disable */
#define _CMU_OSCENCMD_LFXODIS_SHIFT                 9                                        /**< Shift value for CMU_LFXODIS */
#define _CMU_OSCENCMD_LFXODIS_MASK                  0x200UL                                  /**< Bit mask for CMU_LFXODIS */
#define _CMU_OSCENCMD_LFXODIS_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFXODIS_DEFAULT                (_CMU_OSCENCMD_LFXODIS_DEFAULT << 9)     /**< Shifted mode DEFAULT for CMU_OSCENCMD */

/* Bit fields for CMU CMD */
#define _CMU_CMD_RESETVALUE                         0x00000000UL                          /**< Default value for CMU_CMD */
#define _CMU_CMD_MASK                               0x000000FFUL                          /**< Mask for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_SHIFT                     0                                     /**< Shift value for CMU_HFCLKSEL */
#define _CMU_CMD_HFCLKSEL_MASK                      0x7UL                                 /**< Bit mask for CMU_HFCLKSEL */
#define _CMU_CMD_HFCLKSEL_DEFAULT                   0x00000000UL                          /**< Mode DEFAULT for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_HFRCO                     0x00000001UL                          /**< Mode HFRCO for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_HFXO                      0x00000002UL                          /**< Mode HFXO for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_LFRCO                     0x00000003UL                          /**< Mode LFRCO for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_LFXO                      0x00000004UL                          /**< Mode LFXO for CMU_CMD */
#define CMU_CMD_HFCLKSEL_DEFAULT                    (_CMU_CMD_HFCLKSEL_DEFAULT << 0)      /**< Shifted mode DEFAULT for CMU_CMD */
#define CMU_CMD_HFCLKSEL_HFRCO                      (_CMU_CMD_HFCLKSEL_HFRCO << 0)        /**< Shifted mode HFRCO for CMU_CMD */
#define CMU_CMD_HFCLKSEL_HFXO                       (_CMU_CMD_HFCLKSEL_HFXO << 0)         /**< Shifted mode HFXO for CMU_CMD */
#define CMU_CMD_HFCLKSEL_LFRCO                      (_CMU_CMD_HFCLKSEL_LFRCO << 0)        /**< Shifted mode LFRCO for CMU_CMD */
#define CMU_CMD_HFCLKSEL_LFXO                       (_CMU_CMD_HFCLKSEL_LFXO << 0)         /**< Shifted mode LFXO for CMU_CMD */
#define CMU_CMD_CALSTART                            (0x1UL << 3)                          /**< Calibration Start */
#define _CMU_CMD_CALSTART_SHIFT                     3                                     /**< Shift value for CMU_CALSTART */
#define _CMU_CMD_CALSTART_MASK                      0x8UL                                 /**< Bit mask for CMU_CALSTART */
#define _CMU_CMD_CALSTART_DEFAULT                   0x00000000UL                          /**< Mode DEFAULT for CMU_CMD */
#define CMU_CMD_CALSTART_DEFAULT                    (_CMU_CMD_CALSTART_DEFAULT << 3)      /**< Shifted mode DEFAULT for CMU_CMD */
#define CMU_CMD_CALSTOP                             (0x1UL << 4)                          /**< Calibration Stop */
#define _CMU_CMD_CALSTOP_SHIFT                      4                                     /**< Shift value for CMU_CALSTOP */
#define _CMU_CMD_CALSTOP_MASK                       0x10UL                                /**< Bit mask for CMU_CALSTOP */
#define _CMU_CMD_CALSTOP_DEFAULT                    0x00000000UL                          /**< Mode DEFAULT for CMU_CMD */
#define CMU_CMD_CALSTOP_DEFAULT                     (_CMU_CMD_CALSTOP_DEFAULT << 4)       /**< Shifted mode DEFAULT for CMU_CMD */
#define _CMU_CMD_USBCCLKSEL_SHIFT                   5                                     /**< Shift value for CMU_USBCCLKSEL */
#define _CMU_CMD_USBCCLKSEL_MASK                    0xE0UL                                /**< Bit mask for CMU_USBCCLKSEL */
#define _CMU_CMD_USBCCLKSEL_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for CMU_CMD */
#define _CMU_CMD_USBCCLKSEL_HFCLKNODIV              0x00000001UL                          /**< Mode HFCLKNODIV for CMU_CMD */
#define _CMU_CMD_USBCCLKSEL_LFXO                    0x00000002UL                          /**< Mode LFXO for CMU_CMD */
#define _CMU_CMD_USBCCLKSEL_LFRCO                   0x00000003UL                          /**< Mode LFRCO for CMU_CMD */
#define CMU_CMD_USBCCLKSEL_DEFAULT                  (_CMU_CMD_USBCCLKSEL_DEFAULT << 5)    /**< Shifted mode DEFAULT for CMU_CMD */
#define CMU_CMD_USBCCLKSEL_HFCLKNODIV               (_CMU_CMD_USBCCLKSEL_HFCLKNODIV << 5) /**< Shifted mode HFCLKNODIV for CMU_CMD */
#define CMU_CMD_USBCCLKSEL_LFXO                     (_CMU_CMD_USBCCLKSEL_LFXO << 5)       /**< Shifted mode LFXO for CMU_CMD */
#define CMU_CMD_USBCCLKSEL_LFRCO                    (_CMU_CMD_USBCCLKSEL_LFRCO << 5)      /**< Shifted mode LFRCO for CMU_CMD */

/* Bit fields for CMU LFCLKSEL */
#define _CMU_LFCLKSEL_RESETVALUE                    0x00000005UL                             /**< Default value for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_MASK                          0x0011000FUL                             /**< Mask for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFA_SHIFT                     0                                        /**< Shift value for CMU_LFA */
#define _CMU_LFCLKSEL_LFA_MASK                      0x3UL                                    /**< Bit mask for CMU_LFA */
#define _CMU_LFCLKSEL_LFA_DISABLED                  0x00000000UL                             /**< Mode DISABLED for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFA_DEFAULT                   0x00000001UL                             /**< Mode DEFAULT for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFA_LFRCO                     0x00000001UL                             /**< Mode LFRCO for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFA_LFXO                      0x00000002UL                             /**< Mode LFXO for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFA_HFCORECLKLEDIV2           0x00000003UL                             /**< Mode HFCORECLKLEDIV2 for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFA_DISABLED                   (_CMU_LFCLKSEL_LFA_DISABLED << 0)        /**< Shifted mode DISABLED for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFA_DEFAULT                    (_CMU_LFCLKSEL_LFA_DEFAULT << 0)         /**< Shifted mode DEFAULT for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFA_LFRCO                      (_CMU_LFCLKSEL_LFA_LFRCO << 0)           /**< Shifted mode LFRCO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFA_LFXO                       (_CMU_LFCLKSEL_LFA_LFXO << 0)            /**< Shifted mode LFXO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFA_HFCORECLKLEDIV2            (_CMU_LFCLKSEL_LFA_HFCORECLKLEDIV2 << 0) /**< Shifted mode HFCORECLKLEDIV2 for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFB_SHIFT                     2                                        /**< Shift value for CMU_LFB */
#define _CMU_LFCLKSEL_LFB_MASK                      0xCUL                                    /**< Bit mask for CMU_LFB */
#define _CMU_LFCLKSEL_LFB_DISABLED                  0x00000000UL                             /**< Mode DISABLED for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFB_DEFAULT                   0x00000001UL                             /**< Mode DEFAULT for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFB_LFRCO                     0x00000001UL                             /**< Mode LFRCO for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFB_LFXO                      0x00000002UL                             /**< Mode LFXO for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFB_HFCORECLKLEDIV2           0x00000003UL                             /**< Mode HFCORECLKLEDIV2 for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFB_DISABLED                   (_CMU_LFCLKSEL_LFB_DISABLED << 2)        /**< Shifted mode DISABLED for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFB_DEFAULT                    (_CMU_LFCLKSEL_LFB_DEFAULT << 2)         /**< Shifted mode DEFAULT for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFB_LFRCO                      (_CMU_LFCLKSEL_LFB_LFRCO << 2)           /**< Shifted mode LFRCO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFB_LFXO                       (_CMU_LFCLKSEL_LFB_LFXO << 2)            /**< Shifted mode LFXO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFB_HFCORECLKLEDIV2            (_CMU_LFCLKSEL_LFB_HFCORECLKLEDIV2 << 2) /**< Shifted mode HFCORECLKLEDIV2 for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFAE                           (0x1UL << 16)                            /**< Clock Select for LFA Extended */
#define _CMU_LFCLKSEL_LFAE_SHIFT                    16                                       /**< Shift value for CMU_LFAE */
#define _CMU_LFCLKSEL_LFAE_MASK                     0x10000UL                                /**< Bit mask for CMU_LFAE */
#define _CMU_LFCLKSEL_LFAE_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFAE_DISABLED                 0x00000000UL                             /**< Mode DISABLED for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFAE_ULFRCO                   0x00000001UL                             /**< Mode ULFRCO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFAE_DEFAULT                   (_CMU_LFCLKSEL_LFAE_DEFAULT << 16)       /**< Shifted mode DEFAULT for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFAE_DISABLED                  (_CMU_LFCLKSEL_LFAE_DISABLED << 16)      /**< Shifted mode DISABLED for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFAE_ULFRCO                    (_CMU_LFCLKSEL_LFAE_ULFRCO << 16)        /**< Shifted mode ULFRCO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFBE                           (0x1UL << 20)                            /**< Clock Select for LFB Extended */
#define _CMU_LFCLKSEL_LFBE_SHIFT                    20                                       /**< Shift value for CMU_LFBE */
#define _CMU_LFCLKSEL_LFBE_MASK                     0x100000UL                               /**< Bit mask for CMU_LFBE */
#define _CMU_LFCLKSEL_LFBE_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFBE_DISABLED                 0x00000000UL                             /**< Mode DISABLED for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFBE_ULFRCO                   0x00000001UL                             /**< Mode ULFRCO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFBE_DEFAULT                   (_CMU_LFCLKSEL_LFBE_DEFAULT << 20)       /**< Shifted mode DEFAULT for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFBE_DISABLED                  (_CMU_LFCLKSEL_LFBE_DISABLED << 20)      /**< Shifted mode DISABLED for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFBE_ULFRCO                    (_CMU_LFCLKSEL_LFBE_ULFRCO << 20)        /**< Shifted mode ULFRCO for CMU_LFCLKSEL */

/* Bit fields for CMU STATUS */
#define _CMU_STATUS_RESETVALUE                      0x00000403UL                             /**< Default value for CMU_STATUS */
#define _CMU_STATUS_MASK                            0x0003FFFFUL                             /**< Mask for CMU_STATUS */
#define CMU_STATUS_HFRCOENS                         (0x1UL << 0)                             /**< HFRCO Enable Status */
#define _CMU_STATUS_HFRCOENS_SHIFT                  0                                        /**< Shift value for CMU_HFRCOENS */
#define _CMU_STATUS_HFRCOENS_MASK                   0x1UL                                    /**< Bit mask for CMU_HFRCOENS */
#define _CMU_STATUS_HFRCOENS_DEFAULT                0x00000001UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCOENS_DEFAULT                 (_CMU_STATUS_HFRCOENS_DEFAULT << 0)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCORDY                         (0x1UL << 1)                             /**< HFRCO Ready */
#define _CMU_STATUS_HFRCORDY_SHIFT                  1                                        /**< Shift value for CMU_HFRCORDY */
#define _CMU_STATUS_HFRCORDY_MASK                   0x2UL                                    /**< Bit mask for CMU_HFRCORDY */
#define _CMU_STATUS_HFRCORDY_DEFAULT                0x00000001UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCORDY_DEFAULT                 (_CMU_STATUS_HFRCORDY_DEFAULT << 1)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOENS                          (0x1UL << 2)                             /**< HFXO Enable Status */
#define _CMU_STATUS_HFXOENS_SHIFT                   2                                        /**< Shift value for CMU_HFXOENS */
#define _CMU_STATUS_HFXOENS_MASK                    0x4UL                                    /**< Bit mask for CMU_HFXOENS */
#define _CMU_STATUS_HFXOENS_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOENS_DEFAULT                  (_CMU_STATUS_HFXOENS_DEFAULT << 2)       /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXORDY                          (0x1UL << 3)                             /**< HFXO Ready */
#define _CMU_STATUS_HFXORDY_SHIFT                   3                                        /**< Shift value for CMU_HFXORDY */
#define _CMU_STATUS_HFXORDY_MASK                    0x8UL                                    /**< Bit mask for CMU_HFXORDY */
#define _CMU_STATUS_HFXORDY_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXORDY_DEFAULT                  (_CMU_STATUS_HFXORDY_DEFAULT << 3)       /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCOENS                      (0x1UL << 4)                             /**< AUXHFRCO Enable Status */
#define _CMU_STATUS_AUXHFRCOENS_SHIFT               4                                        /**< Shift value for CMU_AUXHFRCOENS */
#define _CMU_STATUS_AUXHFRCOENS_MASK                0x10UL                                   /**< Bit mask for CMU_AUXHFRCOENS */
#define _CMU_STATUS_AUXHFRCOENS_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCOENS_DEFAULT              (_CMU_STATUS_AUXHFRCOENS_DEFAULT << 4)   /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCORDY                      (0x1UL << 5)                             /**< AUXHFRCO Ready */
#define _CMU_STATUS_AUXHFRCORDY_SHIFT               5                                        /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_STATUS_AUXHFRCORDY_MASK                0x20UL                                   /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_STATUS_AUXHFRCORDY_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCORDY_DEFAULT              (_CMU_STATUS_AUXHFRCORDY_DEFAULT << 5)   /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOENS                         (0x1UL << 6)                             /**< LFRCO Enable Status */
#define _CMU_STATUS_LFRCOENS_SHIFT                  6                                        /**< Shift value for CMU_LFRCOENS */
#define _CMU_STATUS_LFRCOENS_MASK                   0x40UL                                   /**< Bit mask for CMU_LFRCOENS */
#define _CMU_STATUS_LFRCOENS_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOENS_DEFAULT                 (_CMU_STATUS_LFRCOENS_DEFAULT << 6)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCORDY                         (0x1UL << 7)                             /**< LFRCO Ready */
#define _CMU_STATUS_LFRCORDY_SHIFT                  7                                        /**< Shift value for CMU_LFRCORDY */
#define _CMU_STATUS_LFRCORDY_MASK                   0x80UL                                   /**< Bit mask for CMU_LFRCORDY */
#define _CMU_STATUS_LFRCORDY_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCORDY_DEFAULT                 (_CMU_STATUS_LFRCORDY_DEFAULT << 7)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOENS                          (0x1UL << 8)                             /**< LFXO Enable Status */
#define _CMU_STATUS_LFXOENS_SHIFT                   8                                        /**< Shift value for CMU_LFXOENS */
#define _CMU_STATUS_LFXOENS_MASK                    0x100UL                                  /**< Bit mask for CMU_LFXOENS */
#define _CMU_STATUS_LFXOENS_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOENS_DEFAULT                  (_CMU_STATUS_LFXOENS_DEFAULT << 8)       /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXORDY                          (0x1UL << 9)                             /**< LFXO Ready */
#define _CMU_STATUS_LFXORDY_SHIFT                   9                                        /**< Shift value for CMU_LFXORDY */
#define _CMU_STATUS_LFXORDY_MASK                    0x200UL                                  /**< Bit mask for CMU_LFXORDY */
#define _CMU_STATUS_LFXORDY_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXORDY_DEFAULT                  (_CMU_STATUS_LFXORDY_DEFAULT << 9)       /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCOSEL                         (0x1UL << 10)                            /**< HFRCO Selected */
#define _CMU_STATUS_HFRCOSEL_SHIFT                  10                                       /**< Shift value for CMU_HFRCOSEL */
#define _CMU_STATUS_HFRCOSEL_MASK                   0x400UL                                  /**< Bit mask for CMU_HFRCOSEL */
#define _CMU_STATUS_HFRCOSEL_DEFAULT                0x00000001UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCOSEL_DEFAULT                 (_CMU_STATUS_HFRCOSEL_DEFAULT << 10)     /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOSEL                          (0x1UL << 11)                            /**< HFXO Selected */
#define _CMU_STATUS_HFXOSEL_SHIFT                   11                                       /**< Shift value for CMU_HFXOSEL */
#define _CMU_STATUS_HFXOSEL_MASK                    0x800UL                                  /**< Bit mask for CMU_HFXOSEL */
#define _CMU_STATUS_HFXOSEL_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOSEL_DEFAULT                  (_CMU_STATUS_HFXOSEL_DEFAULT << 11)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOSEL                         (0x1UL << 12)                            /**< LFRCO Selected */
#define _CMU_STATUS_LFRCOSEL_SHIFT                  12                                       /**< Shift value for CMU_LFRCOSEL */
#define _CMU_STATUS_LFRCOSEL_MASK                   0x1000UL                                 /**< Bit mask for CMU_LFRCOSEL */
#define _CMU_STATUS_LFRCOSEL_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOSEL_DEFAULT                 (_CMU_STATUS_LFRCOSEL_DEFAULT << 12)     /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOSEL                          (0x1UL << 13)                            /**< LFXO Selected */
#define _CMU_STATUS_LFXOSEL_SHIFT                   13                                       /**< Shift value for CMU_LFXOSEL */
#define _CMU_STATUS_LFXOSEL_MASK                    0x2000UL                                 /**< Bit mask for CMU_LFXOSEL */
#define _CMU_STATUS_LFXOSEL_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOSEL_DEFAULT                  (_CMU_STATUS_LFXOSEL_DEFAULT << 13)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_CALBSY                           (0x1UL << 14)                            /**< Calibration Busy */
#define _CMU_STATUS_CALBSY_SHIFT                    14                                       /**< Shift value for CMU_CALBSY */
#define _CMU_STATUS_CALBSY_MASK                     0x4000UL                                 /**< Bit mask for CMU_CALBSY */
#define _CMU_STATUS_CALBSY_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_CALBSY_DEFAULT                   (_CMU_STATUS_CALBSY_DEFAULT << 14)       /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_USBCHFCLKSEL                     (0x1UL << 15)                            /**< USBC HFCLK Selected */
#define _CMU_STATUS_USBCHFCLKSEL_SHIFT              15                                       /**< Shift value for CMU_USBCHFCLKSEL */
#define _CMU_STATUS_USBCHFCLKSEL_MASK               0x8000UL                                 /**< Bit mask for CMU_USBCHFCLKSEL */
#define _CMU_STATUS_USBCHFCLKSEL_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_USBCHFCLKSEL_DEFAULT             (_CMU_STATUS_USBCHFCLKSEL_DEFAULT << 15) /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_USBCLFXOSEL                      (0x1UL << 16)                            /**< USBC LFXO Selected */
#define _CMU_STATUS_USBCLFXOSEL_SHIFT               16                                       /**< Shift value for CMU_USBCLFXOSEL */
#define _CMU_STATUS_USBCLFXOSEL_MASK                0x10000UL                                /**< Bit mask for CMU_USBCLFXOSEL */
#define _CMU_STATUS_USBCLFXOSEL_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_USBCLFXOSEL_DEFAULT              (_CMU_STATUS_USBCLFXOSEL_DEFAULT << 16)  /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_USBCLFRCOSEL                     (0x1UL << 17)                            /**< USBC LFRCO Selected */
#define _CMU_STATUS_USBCLFRCOSEL_SHIFT              17                                       /**< Shift value for CMU_USBCLFRCOSEL */
#define _CMU_STATUS_USBCLFRCOSEL_MASK               0x20000UL                                /**< Bit mask for CMU_USBCLFRCOSEL */
#define _CMU_STATUS_USBCLFRCOSEL_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_USBCLFRCOSEL_DEFAULT             (_CMU_STATUS_USBCLFRCOSEL_DEFAULT << 17) /**< Shifted mode DEFAULT for CMU_STATUS */

/* Bit fields for CMU IF */
#define _CMU_IF_RESETVALUE                          0x00000001UL                        /**< Default value for CMU_IF */
#define _CMU_IF_MASK                                0x000000FFUL                        /**< Mask for CMU_IF */
#define CMU_IF_HFRCORDY                             (0x1UL << 0)                        /**< HFRCO Ready Interrupt Flag */
#define _CMU_IF_HFRCORDY_SHIFT                      0                                   /**< Shift value for CMU_HFRCORDY */
#define _CMU_IF_HFRCORDY_MASK                       0x1UL                               /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IF_HFRCORDY_DEFAULT                    0x00000001UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFRCORDY_DEFAULT                     (_CMU_IF_HFRCORDY_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_HFXORDY                              (0x1UL << 1)                        /**< HFXO Ready Interrupt Flag */
#define _CMU_IF_HFXORDY_SHIFT                       1                                   /**< Shift value for CMU_HFXORDY */
#define _CMU_IF_HFXORDY_MASK                        0x2UL                               /**< Bit mask for CMU_HFXORDY */
#define _CMU_IF_HFXORDY_DEFAULT                     0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFXORDY_DEFAULT                      (_CMU_IF_HFXORDY_DEFAULT << 1)      /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_LFRCORDY                             (0x1UL << 2)                        /**< LFRCO Ready Interrupt Flag */
#define _CMU_IF_LFRCORDY_SHIFT                      2                                   /**< Shift value for CMU_LFRCORDY */
#define _CMU_IF_LFRCORDY_MASK                       0x4UL                               /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IF_LFRCORDY_DEFAULT                    0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_LFRCORDY_DEFAULT                     (_CMU_IF_LFRCORDY_DEFAULT << 2)     /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_LFXORDY                              (0x1UL << 3)                        /**< LFXO Ready Interrupt Flag */
#define _CMU_IF_LFXORDY_SHIFT                       3                                   /**< Shift value for CMU_LFXORDY */
#define _CMU_IF_LFXORDY_MASK                        0x8UL                               /**< Bit mask for CMU_LFXORDY */
#define _CMU_IF_LFXORDY_DEFAULT                     0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_LFXORDY_DEFAULT                      (_CMU_IF_LFXORDY_DEFAULT << 3)      /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_AUXHFRCORDY                          (0x1UL << 4)                        /**< AUXHFRCO Ready Interrupt Flag */
#define _CMU_IF_AUXHFRCORDY_SHIFT                   4                                   /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IF_AUXHFRCORDY_MASK                    0x10UL                              /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IF_AUXHFRCORDY_DEFAULT                 0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_AUXHFRCORDY_DEFAULT                  (_CMU_IF_AUXHFRCORDY_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_CALRDY                               (0x1UL << 5)                        /**< Calibration Ready Interrupt Flag */
#define _CMU_IF_CALRDY_SHIFT                        5                                   /**< Shift value for CMU_CALRDY */
#define _CMU_IF_CALRDY_MASK                         0x20UL                              /**< Bit mask for CMU_CALRDY */
#define _CMU_IF_CALRDY_DEFAULT                      0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_CALRDY_DEFAULT                       (_CMU_IF_CALRDY_DEFAULT << 5)       /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_CALOF                                (0x1UL << 6)                        /**< Calibration Overflow Interrupt Flag */
#define _CMU_IF_CALOF_SHIFT                         6                                   /**< Shift value for CMU_CALOF */
#define _CMU_IF_CALOF_MASK                          0x40UL                              /**< Bit mask for CMU_CALOF */
#define _CMU_IF_CALOF_DEFAULT                       0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_CALOF_DEFAULT                        (_CMU_IF_CALOF_DEFAULT << 6)        /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_USBCHFCLKSEL                         (0x1UL << 7)                        /**< USBC HFCLK Selected Interrupt Flag */
#define _CMU_IF_USBCHFCLKSEL_SHIFT                  7                                   /**< Shift value for CMU_USBCHFCLKSEL */
#define _CMU_IF_USBCHFCLKSEL_MASK                   0x80UL                              /**< Bit mask for CMU_USBCHFCLKSEL */
#define _CMU_IF_USBCHFCLKSEL_DEFAULT                0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_USBCHFCLKSEL_DEFAULT                 (_CMU_IF_USBCHFCLKSEL_DEFAULT << 7) /**< Shifted mode DEFAULT for CMU_IF */

/* Bit fields for CMU IFS */
#define _CMU_IFS_RESETVALUE                         0x00000000UL                         /**< Default value for CMU_IFS */
#define _CMU_IFS_MASK                               0x000000FFUL                         /**< Mask for CMU_IFS */
#define CMU_IFS_HFRCORDY                            (0x1UL << 0)                         /**< HFRCO Ready Interrupt Flag Set */
#define _CMU_IFS_HFRCORDY_SHIFT                     0                                    /**< Shift value for CMU_HFRCORDY */
#define _CMU_IFS_HFRCORDY_MASK                      0x1UL                                /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IFS_HFRCORDY_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFRCORDY_DEFAULT                    (_CMU_IFS_HFRCORDY_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXORDY                             (0x1UL << 1)                         /**< HFXO Ready Interrupt Flag Set */
#define _CMU_IFS_HFXORDY_SHIFT                      1                                    /**< Shift value for CMU_HFXORDY */
#define _CMU_IFS_HFXORDY_MASK                       0x2UL                                /**< Bit mask for CMU_HFXORDY */
#define _CMU_IFS_HFXORDY_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXORDY_DEFAULT                     (_CMU_IFS_HFXORDY_DEFAULT << 1)      /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFRCORDY                            (0x1UL << 2)                         /**< LFRCO Ready Interrupt Flag Set */
#define _CMU_IFS_LFRCORDY_SHIFT                     2                                    /**< Shift value for CMU_LFRCORDY */
#define _CMU_IFS_LFRCORDY_MASK                      0x4UL                                /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IFS_LFRCORDY_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFRCORDY_DEFAULT                    (_CMU_IFS_LFRCORDY_DEFAULT << 2)     /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFXORDY                             (0x1UL << 3)                         /**< LFXO Ready Interrupt Flag Set */
#define _CMU_IFS_LFXORDY_SHIFT                      3                                    /**< Shift value for CMU_LFXORDY */
#define _CMU_IFS_LFXORDY_MASK                       0x8UL                                /**< Bit mask for CMU_LFXORDY */
#define _CMU_IFS_LFXORDY_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFXORDY_DEFAULT                     (_CMU_IFS_LFXORDY_DEFAULT << 3)      /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_AUXHFRCORDY                         (0x1UL << 4)                         /**< AUXHFRCO Ready Interrupt Flag Set */
#define _CMU_IFS_AUXHFRCORDY_SHIFT                  4                                    /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IFS_AUXHFRCORDY_MASK                   0x10UL                               /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IFS_AUXHFRCORDY_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_AUXHFRCORDY_DEFAULT                 (_CMU_IFS_AUXHFRCORDY_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALRDY                              (0x1UL << 5)                         /**< Calibration Ready Interrupt Flag Set */
#define _CMU_IFS_CALRDY_SHIFT                       5                                    /**< Shift value for CMU_CALRDY */
#define _CMU_IFS_CALRDY_MASK                        0x20UL                               /**< Bit mask for CMU_CALRDY */
#define _CMU_IFS_CALRDY_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALRDY_DEFAULT                      (_CMU_IFS_CALRDY_DEFAULT << 5)       /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALOF                               (0x1UL << 6)                         /**< Calibration Overflow Interrupt Flag Set */
#define _CMU_IFS_CALOF_SHIFT                        6                                    /**< Shift value for CMU_CALOF */
#define _CMU_IFS_CALOF_MASK                         0x40UL                               /**< Bit mask for CMU_CALOF */
#define _CMU_IFS_CALOF_DEFAULT                      0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALOF_DEFAULT                       (_CMU_IFS_CALOF_DEFAULT << 6)        /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_USBCHFCLKSEL                        (0x1UL << 7)                         /**< USBC HFCLK Selected Interrupt Flag Set */
#define _CMU_IFS_USBCHFCLKSEL_SHIFT                 7                                    /**< Shift value for CMU_USBCHFCLKSEL */
#define _CMU_IFS_USBCHFCLKSEL_MASK                  0x80UL                               /**< Bit mask for CMU_USBCHFCLKSEL */
#define _CMU_IFS_USBCHFCLKSEL_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_USBCHFCLKSEL_DEFAULT                (_CMU_IFS_USBCHFCLKSEL_DEFAULT << 7) /**< Shifted mode DEFAULT for CMU_IFS */

/* Bit fields for CMU IFC */
#define _CMU_IFC_RESETVALUE                         0x00000000UL                         /**< Default value for CMU_IFC */
#define _CMU_IFC_MASK                               0x000000FFUL                         /**< Mask for CMU_IFC */
#define CMU_IFC_HFRCORDY                            (0x1UL << 0)                         /**< HFRCO Ready Interrupt Flag Clear */
#define _CMU_IFC_HFRCORDY_SHIFT                     0                                    /**< Shift value for CMU_HFRCORDY */
#define _CMU_IFC_HFRCORDY_MASK                      0x1UL                                /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IFC_HFRCORDY_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFRCORDY_DEFAULT                    (_CMU_IFC_HFRCORDY_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXORDY                             (0x1UL << 1)                         /**< HFXO Ready Interrupt Flag Clear */
#define _CMU_IFC_HFXORDY_SHIFT                      1                                    /**< Shift value for CMU_HFXORDY */
#define _CMU_IFC_HFXORDY_MASK                       0x2UL                                /**< Bit mask for CMU_HFXORDY */
#define _CMU_IFC_HFXORDY_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXORDY_DEFAULT                     (_CMU_IFC_HFXORDY_DEFAULT << 1)      /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFRCORDY                            (0x1UL << 2)                         /**< LFRCO Ready Interrupt Flag Clear */
#define _CMU_IFC_LFRCORDY_SHIFT                     2                                    /**< Shift value for CMU_LFRCORDY */
#define _CMU_IFC_LFRCORDY_MASK                      0x4UL                                /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IFC_LFRCORDY_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFRCORDY_DEFAULT                    (_CMU_IFC_LFRCORDY_DEFAULT << 2)     /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFXORDY                             (0x1UL << 3)                         /**< LFXO Ready Interrupt Flag Clear */
#define _CMU_IFC_LFXORDY_SHIFT                      3                                    /**< Shift value for CMU_LFXORDY */
#define _CMU_IFC_LFXORDY_MASK                       0x8UL                                /**< Bit mask for CMU_LFXORDY */
#define _CMU_IFC_LFXORDY_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFXORDY_DEFAULT                     (_CMU_IFC_LFXORDY_DEFAULT << 3)      /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_AUXHFRCORDY                         (0x1UL << 4)                         /**< AUXHFRCO Ready Interrupt Flag Clear */
#define _CMU_IFC_AUXHFRCORDY_SHIFT                  4                                    /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IFC_AUXHFRCORDY_MASK                   0x10UL                               /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IFC_AUXHFRCORDY_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_AUXHFRCORDY_DEFAULT                 (_CMU_IFC_AUXHFRCORDY_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALRDY                              (0x1UL << 5)                         /**< Calibration Ready Interrupt Flag Clear */
#define _CMU_IFC_CALRDY_SHIFT                       5                                    /**< Shift value for CMU_CALRDY */
#define _CMU_IFC_CALRDY_MASK                        0x20UL                               /**< Bit mask for CMU_CALRDY */
#define _CMU_IFC_CALRDY_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALRDY_DEFAULT                      (_CMU_IFC_CALRDY_DEFAULT << 5)       /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALOF                               (0x1UL << 6)                         /**< Calibration Overflow Interrupt Flag Clear */
#define _CMU_IFC_CALOF_SHIFT                        6                                    /**< Shift value for CMU_CALOF */
#define _CMU_IFC_CALOF_MASK                         0x40UL                               /**< Bit mask for CMU_CALOF */
#define _CMU_IFC_CALOF_DEFAULT                      0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALOF_DEFAULT                       (_CMU_IFC_CALOF_DEFAULT << 6)        /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_USBCHFCLKSEL                        (0x1UL << 7)                         /**< USBC HFCLK Selected Interrupt Flag Clear */
#define _CMU_IFC_USBCHFCLKSEL_SHIFT                 7                                    /**< Shift value for CMU_USBCHFCLKSEL */
#define _CMU_IFC_USBCHFCLKSEL_MASK                  0x80UL                               /**< Bit mask for CMU_USBCHFCLKSEL */
#define _CMU_IFC_USBCHFCLKSEL_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_USBCHFCLKSEL_DEFAULT                (_CMU_IFC_USBCHFCLKSEL_DEFAULT << 7) /**< Shifted mode DEFAULT for CMU_IFC */

/* Bit fields for CMU IEN */
#define _CMU_IEN_RESETVALUE                         0x00000000UL                         /**< Default value for CMU_IEN */
#define _CMU_IEN_MASK                               0x000000FFUL                         /**< Mask for CMU_IEN */
#define CMU_IEN_HFRCORDY                            (0x1UL << 0)                         /**< HFRCO Ready Interrupt Enable */
#define _CMU_IEN_HFRCORDY_SHIFT                     0                                    /**< Shift value for CMU_HFRCORDY */
#define _CMU_IEN_HFRCORDY_MASK                      0x1UL                                /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IEN_HFRCORDY_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFRCORDY_DEFAULT                    (_CMU_IEN_HFRCORDY_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXORDY                             (0x1UL << 1)                         /**< HFXO Ready Interrupt Enable */
#define _CMU_IEN_HFXORDY_SHIFT                      1                                    /**< Shift value for CMU_HFXORDY */
#define _CMU_IEN_HFXORDY_MASK                       0x2UL                                /**< Bit mask for CMU_HFXORDY */
#define _CMU_IEN_HFXORDY_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXORDY_DEFAULT                     (_CMU_IEN_HFXORDY_DEFAULT << 1)      /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFRCORDY                            (0x1UL << 2)                         /**< LFRCO Ready Interrupt Enable */
#define _CMU_IEN_LFRCORDY_SHIFT                     2                                    /**< Shift value for CMU_LFRCORDY */
#define _CMU_IEN_LFRCORDY_MASK                      0x4UL                                /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IEN_LFRCORDY_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFRCORDY_DEFAULT                    (_CMU_IEN_LFRCORDY_DEFAULT << 2)     /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFXORDY                             (0x1UL << 3)                         /**< LFXO Ready Interrupt Enable */
#define _CMU_IEN_LFXORDY_SHIFT                      3                                    /**< Shift value for CMU_LFXORDY */
#define _CMU_IEN_LFXORDY_MASK                       0x8UL                                /**< Bit mask for CMU_LFXORDY */
#define _CMU_IEN_LFXORDY_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFXORDY_DEFAULT                     (_CMU_IEN_LFXORDY_DEFAULT << 3)      /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_AUXHFRCORDY                         (0x1UL << 4)                         /**< AUXHFRCO Ready Interrupt Enable */
#define _CMU_IEN_AUXHFRCORDY_SHIFT                  4                                    /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IEN_AUXHFRCORDY_MASK                   0x10UL                               /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IEN_AUXHFRCORDY_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_AUXHFRCORDY_DEFAULT                 (_CMU_IEN_AUXHFRCORDY_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALRDY                              (0x1UL << 5)                         /**< Calibration Ready Interrupt Enable */
#define _CMU_IEN_CALRDY_SHIFT                       5                                    /**< Shift value for CMU_CALRDY */
#define _CMU_IEN_CALRDY_MASK                        0x20UL                               /**< Bit mask for CMU_CALRDY */
#define _CMU_IEN_CALRDY_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALRDY_DEFAULT                      (_CMU_IEN_CALRDY_DEFAULT << 5)       /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALOF                               (0x1UL << 6)                         /**< Calibration Overflow Interrupt Enable */
#define _CMU_IEN_CALOF_SHIFT                        6                                    /**< Shift value for CMU_CALOF */
#define _CMU_IEN_CALOF_MASK                         0x40UL                               /**< Bit mask for CMU_CALOF */
#define _CMU_IEN_CALOF_DEFAULT                      0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALOF_DEFAULT                       (_CMU_IEN_CALOF_DEFAULT << 6)        /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_USBCHFCLKSEL                        (0x1UL << 7)                         /**< USBC HFCLK Selected Interrupt Enable */
#define _CMU_IEN_USBCHFCLKSEL_SHIFT                 7                                    /**< Shift value for CMU_USBCHFCLKSEL */
#define _CMU_IEN_USBCHFCLKSEL_MASK                  0x80UL                               /**< Bit mask for CMU_USBCHFCLKSEL */
#define _CMU_IEN_USBCHFCLKSEL_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_USBCHFCLKSEL_DEFAULT                (_CMU_IEN_USBCHFCLKSEL_DEFAULT << 7) /**< Shifted mode DEFAULT for CMU_IEN */

/* Bit fields for CMU HFCORECLKEN0 */
#define _CMU_HFCORECLKEN0_RESETVALUE                0x00000000UL                          /**< Default value for CMU_HFCORECLKEN0 */
#define _CMU_HFCORECLKEN0_MASK                      0x0000001FUL                          /**< Mask for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_DMA                        (0x1UL << 0)                          /**< Direct Memory Access Controller Clock Enable */
#define _CMU_HFCORECLKEN0_DMA_SHIFT                 0                                     /**< Shift value for CMU_DMA */
#define _CMU_HFCORECLKEN0_DMA_MASK                  0x1UL                                 /**< Bit mask for CMU_DMA */
#define _CMU_HFCORECLKEN0_DMA_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_DMA_DEFAULT                (_CMU_HFCORECLKEN0_DMA_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_AES                        (0x1UL << 1)                          /**< Advanced Encryption Standard Accelerator Clock Enable */
#define _CMU_HFCORECLKEN0_AES_SHIFT                 1                                     /**< Shift value for CMU_AES */
#define _CMU_HFCORECLKEN0_AES_MASK                  0x2UL                                 /**< Bit mask for CMU_AES */
#define _CMU_HFCORECLKEN0_AES_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_AES_DEFAULT                (_CMU_HFCORECLKEN0_AES_DEFAULT << 1)  /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_USBC                       (0x1UL << 2)                          /**< Universal Serial Bus Interface Core Clock Enable */
#define _CMU_HFCORECLKEN0_USBC_SHIFT                2                                     /**< Shift value for CMU_USBC */
#define _CMU_HFCORECLKEN0_USBC_MASK                 0x4UL                                 /**< Bit mask for CMU_USBC */
#define _CMU_HFCORECLKEN0_USBC_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_USBC_DEFAULT               (_CMU_HFCORECLKEN0_USBC_DEFAULT << 2) /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_USB                        (0x1UL << 3)                          /**< Universal Serial Bus Interface Clock Enable */
#define _CMU_HFCORECLKEN0_USB_SHIFT                 3                                     /**< Shift value for CMU_USB */
#define _CMU_HFCORECLKEN0_USB_MASK                  0x8UL                                 /**< Bit mask for CMU_USB */
#define _CMU_HFCORECLKEN0_USB_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_USB_DEFAULT                (_CMU_HFCORECLKEN0_USB_DEFAULT << 3)  /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_LE                         (0x1UL << 4)                          /**< Low Energy Peripheral Interface Clock Enable */
#define _CMU_HFCORECLKEN0_LE_SHIFT                  4                                     /**< Shift value for CMU_LE */
#define _CMU_HFCORECLKEN0_LE_MASK                   0x10UL                                /**< Bit mask for CMU_LE */
#define _CMU_HFCORECLKEN0_LE_DEFAULT                0x00000000UL                          /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_LE_DEFAULT                 (_CMU_HFCORECLKEN0_LE_DEFAULT << 4)   /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */

/* Bit fields for CMU HFPERCLKEN0 */
#define _CMU_HFPERCLKEN0_RESETVALUE                 0x00000000UL                           /**< Default value for CMU_HFPERCLKEN0 */
#define _CMU_HFPERCLKEN0_MASK                       0x0003FFE7UL                           /**< Mask for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART0                      (0x1UL << 0)                           /**< Universal Synchronous/Asynchronous Receiver/Transmitter 0 Clock Enable */
#define _CMU_HFPERCLKEN0_USART0_SHIFT               0                                      /**< Shift value for CMU_USART0 */
#define _CMU_HFPERCLKEN0_USART0_MASK                0x1UL                                  /**< Bit mask for CMU_USART0 */
#define _CMU_HFPERCLKEN0_USART0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART0_DEFAULT              (_CMU_HFPERCLKEN0_USART0_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART1                      (0x1UL << 1)                           /**< Universal Synchronous/Asynchronous Receiver/Transmitter 1 Clock Enable */
#define _CMU_HFPERCLKEN0_USART1_SHIFT               1                                      /**< Shift value for CMU_USART1 */
#define _CMU_HFPERCLKEN0_USART1_MASK                0x2UL                                  /**< Bit mask for CMU_USART1 */
#define _CMU_HFPERCLKEN0_USART1_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART1_DEFAULT              (_CMU_HFPERCLKEN0_USART1_DEFAULT << 1) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART2                      (0x1UL << 2)                           /**< Universal Synchronous/Asynchronous Receiver/Transmitter 2 Clock Enable */
#define _CMU_HFPERCLKEN0_USART2_SHIFT               2                                      /**< Shift value for CMU_USART2 */
#define _CMU_HFPERCLKEN0_USART2_MASK                0x4UL                                  /**< Bit mask for CMU_USART2 */
#define _CMU_HFPERCLKEN0_USART2_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART2_DEFAULT              (_CMU_HFPERCLKEN0_USART2_DEFAULT << 2) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER0                      (0x1UL << 5)                           /**< Timer 0 Clock Enable */
#define _CMU_HFPERCLKEN0_TIMER0_SHIFT               5                                      /**< Shift value for CMU_TIMER0 */
#define _CMU_HFPERCLKEN0_TIMER0_MASK                0x20UL                                 /**< Bit mask for CMU_TIMER0 */
#define _CMU_HFPERCLKEN0_TIMER0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER0_DEFAULT              (_CMU_HFPERCLKEN0_TIMER0_DEFAULT << 5) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER1                      (0x1UL << 6)                           /**< Timer 1 Clock Enable */
#define _CMU_HFPERCLKEN0_TIMER1_SHIFT               6                                      /**< Shift value for CMU_TIMER1 */
#define _CMU_HFPERCLKEN0_TIMER1_MASK                0x40UL                                 /**< Bit mask for CMU_TIMER1 */
#define _CMU_HFPERCLKEN0_TIMER1_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER1_DEFAULT              (_CMU_HFPERCLKEN0_TIMER1_DEFAULT << 6) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER2                      (0x1UL << 7)                           /**< Timer 2 Clock Enable */
#define _CMU_HFPERCLKEN0_TIMER2_SHIFT               7                                      /**< Shift value for CMU_TIMER2 */
#define _CMU_HFPERCLKEN0_TIMER2_MASK                0x80UL                                 /**< Bit mask for CMU_TIMER2 */
#define _CMU_HFPERCLKEN0_TIMER2_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER2_DEFAULT              (_CMU_HFPERCLKEN0_TIMER2_DEFAULT << 7) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER3                      (0x1UL << 8)                           /**< Timer 3 Clock Enable */
#define _CMU_HFPERCLKEN0_TIMER3_SHIFT               8                                      /**< Shift value for CMU_TIMER3 */
#define _CMU_HFPERCLKEN0_TIMER3_MASK                0x100UL                                /**< Bit mask for CMU_TIMER3 */
#define _CMU_HFPERCLKEN0_TIMER3_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER3_DEFAULT              (_CMU_HFPERCLKEN0_TIMER3_DEFAULT << 8) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ACMP0                       (0x1UL << 9)                           /**< Analog Comparator 0 Clock Enable */
#define _CMU_HFPERCLKEN0_ACMP0_SHIFT                9                                      /**< Shift value for CMU_ACMP0 */
#define _CMU_HFPERCLKEN0_ACMP0_MASK                 0x200UL                                /**< Bit mask for CMU_ACMP0 */
#define _CMU_HFPERCLKEN0_ACMP0_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ACMP0_DEFAULT               (_CMU_HFPERCLKEN0_ACMP0_DEFAULT << 9)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ACMP1                       (0x1UL << 10)                          /**< Analog Comparator 1 Clock Enable */
#define _CMU_HFPERCLKEN0_ACMP1_SHIFT                10                                     /**< Shift value for CMU_ACMP1 */
#define _CMU_HFPERCLKEN0_ACMP1_MASK                 0x400UL                                /**< Bit mask for CMU_ACMP1 */
#define _CMU_HFPERCLKEN0_ACMP1_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ACMP1_DEFAULT               (_CMU_HFPERCLKEN0_ACMP1_DEFAULT << 10) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_I2C0                        (0x1UL << 11)                          /**< I2C 0 Clock Enable */
#define _CMU_HFPERCLKEN0_I2C0_SHIFT                 11                                     /**< Shift value for CMU_I2C0 */
#define _CMU_HFPERCLKEN0_I2C0_MASK                  0x800UL                                /**< Bit mask for CMU_I2C0 */
#define _CMU_HFPERCLKEN0_I2C0_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_I2C0_DEFAULT                (_CMU_HFPERCLKEN0_I2C0_DEFAULT << 11)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_I2C1                        (0x1UL << 12)                          /**< I2C 1 Clock Enable */
#define _CMU_HFPERCLKEN0_I2C1_SHIFT                 12                                     /**< Shift value for CMU_I2C1 */
#define _CMU_HFPERCLKEN0_I2C1_MASK                  0x1000UL                               /**< Bit mask for CMU_I2C1 */
#define _CMU_HFPERCLKEN0_I2C1_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_I2C1_DEFAULT                (_CMU_HFPERCLKEN0_I2C1_DEFAULT << 12)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_GPIO                        (0x1UL << 13)                          /**< General purpose Input/Output Clock Enable */
#define _CMU_HFPERCLKEN0_GPIO_SHIFT                 13                                     /**< Shift value for CMU_GPIO */
#define _CMU_HFPERCLKEN0_GPIO_MASK                  0x2000UL                               /**< Bit mask for CMU_GPIO */
#define _CMU_HFPERCLKEN0_GPIO_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_GPIO_DEFAULT                (_CMU_HFPERCLKEN0_GPIO_DEFAULT << 13)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_VCMP                        (0x1UL << 14)                          /**< Voltage Comparator Clock Enable */
#define _CMU_HFPERCLKEN0_VCMP_SHIFT                 14                                     /**< Shift value for CMU_VCMP */
#define _CMU_HFPERCLKEN0_VCMP_MASK                  0x4000UL                               /**< Bit mask for CMU_VCMP */
#define _CMU_HFPERCLKEN0_VCMP_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_VCMP_DEFAULT                (_CMU_HFPERCLKEN0_VCMP_DEFAULT << 14)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_PRS                         (0x1UL << 15)                          /**< Peripheral Reflex System Clock Enable */
#define _CMU_HFPERCLKEN0_PRS_SHIFT                  15                                     /**< Shift value for CMU_PRS */
#define _CMU_HFPERCLKEN0_PRS_MASK                   0x8000UL                               /**< Bit mask for CMU_PRS */
#define _CMU_HFPERCLKEN0_PRS_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_PRS_DEFAULT                 (_CMU_HFPERCLKEN0_PRS_DEFAULT << 15)   /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ADC0                        (0x1UL << 16)                          /**< Analog to Digital Converter 0 Clock Enable */
#define _CMU_HFPERCLKEN0_ADC0_SHIFT                 16                                     /**< Shift value for CMU_ADC0 */
#define _CMU_HFPERCLKEN0_ADC0_MASK                  0x10000UL                              /**< Bit mask for CMU_ADC0 */
#define _CMU_HFPERCLKEN0_ADC0_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ADC0_DEFAULT                (_CMU_HFPERCLKEN0_ADC0_DEFAULT << 16)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_DAC0                        (0x1UL << 17)                          /**< Digital to Analog Converter 0 Clock Enable */
#define _CMU_HFPERCLKEN0_DAC0_SHIFT                 17                                     /**< Shift value for CMU_DAC0 */
#define _CMU_HFPERCLKEN0_DAC0_MASK                  0x20000UL                              /**< Bit mask for CMU_DAC0 */
#define _CMU_HFPERCLKEN0_DAC0_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_DAC0_DEFAULT                (_CMU_HFPERCLKEN0_DAC0_DEFAULT << 17)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */

/* Bit fields for CMU SYNCBUSY */
#define _CMU_SYNCBUSY_RESETVALUE                    0x00000000UL                           /**< Default value for CMU_SYNCBUSY */
#define _CMU_SYNCBUSY_MASK                          0x00000055UL                           /**< Mask for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFACLKEN0                      (0x1UL << 0)                           /**< Low Frequency A Clock Enable 0 Busy */
#define _CMU_SYNCBUSY_LFACLKEN0_SHIFT               0                                      /**< Shift value for CMU_LFACLKEN0 */
#define _CMU_SYNCBUSY_LFACLKEN0_MASK                0x1UL                                  /**< Bit mask for CMU_LFACLKEN0 */
#define _CMU_SYNCBUSY_LFACLKEN0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFACLKEN0_DEFAULT              (_CMU_SYNCBUSY_LFACLKEN0_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFAPRESC0                      (0x1UL << 2)                           /**< Low Frequency A Prescaler 0 Busy */
#define _CMU_SYNCBUSY_LFAPRESC0_SHIFT               2                                      /**< Shift value for CMU_LFAPRESC0 */
#define _CMU_SYNCBUSY_LFAPRESC0_MASK                0x4UL                                  /**< Bit mask for CMU_LFAPRESC0 */
#define _CMU_SYNCBUSY_LFAPRESC0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFAPRESC0_DEFAULT              (_CMU_SYNCBUSY_LFAPRESC0_DEFAULT << 2) /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFBCLKEN0                      (0x1UL << 4)                           /**< Low Frequency B Clock Enable 0 Busy */
#define _CMU_SYNCBUSY_LFBCLKEN0_SHIFT               4                                      /**< Shift value for CMU_LFBCLKEN0 */
#define _CMU_SYNCBUSY_LFBCLKEN0_MASK                0x10UL                                 /**< Bit mask for CMU_LFBCLKEN0 */
#define _CMU_SYNCBUSY_LFBCLKEN0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFBCLKEN0_DEFAULT              (_CMU_SYNCBUSY_LFBCLKEN0_DEFAULT << 4) /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFBPRESC0                      (0x1UL << 6)                           /**< Low Frequency B Prescaler 0 Busy */
#define _CMU_SYNCBUSY_LFBPRESC0_SHIFT               6                                      /**< Shift value for CMU_LFBPRESC0 */
#define _CMU_SYNCBUSY_LFBPRESC0_MASK                0x40UL                                 /**< Bit mask for CMU_LFBPRESC0 */
#define _CMU_SYNCBUSY_LFBPRESC0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFBPRESC0_DEFAULT              (_CMU_SYNCBUSY_LFBPRESC0_DEFAULT << 6) /**< Shifted mode DEFAULT for CMU_SYNCBUSY */

/* Bit fields for CMU FREEZE */
#define _CMU_FREEZE_RESETVALUE                      0x00000000UL                         /**< Default value for CMU_FREEZE */
#define _CMU_FREEZE_MASK                            0x00000001UL                         /**< Mask for CMU_FREEZE */
#define CMU_FREEZE_REGFREEZE                        (0x1UL << 0)                         /**< Register Update Freeze */
#define _CMU_FREEZE_REGFREEZE_SHIFT                 0                                    /**< Shift value for CMU_REGFREEZE */
#define _CMU_FREEZE_REGFREEZE_MASK                  0x1UL                                /**< Bit mask for CMU_REGFREEZE */
#define _CMU_FREEZE_REGFREEZE_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_FREEZE */
#define _CMU_FREEZE_REGFREEZE_UPDATE                0x00000000UL                         /**< Mode UPDATE for CMU_FREEZE */
#define _CMU_FREEZE_REGFREEZE_FREEZE                0x00000001UL                         /**< Mode FREEZE for CMU_FREEZE */
#define CMU_FREEZE_REGFREEZE_DEFAULT                (_CMU_FREEZE_REGFREEZE_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_FREEZE */
#define CMU_FREEZE_REGFREEZE_UPDATE                 (_CMU_FREEZE_REGFREEZE_UPDATE << 0)  /**< Shifted mode UPDATE for CMU_FREEZE */
#define CMU_FREEZE_REGFREEZE_FREEZE                 (_CMU_FREEZE_REGFREEZE_FREEZE << 0)  /**< Shifted mode FREEZE for CMU_FREEZE */

/* Bit fields for CMU LFACLKEN0 */
#define _CMU_LFACLKEN0_RESETVALUE                   0x00000000UL                           /**< Default value for CMU_LFACLKEN0 */
#define _CMU_LFACLKEN0_MASK                         0x0000000FUL                           /**< Mask for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LESENSE                       (0x1UL << 0)                           /**< Low Energy Sensor Interface Clock Enable */
#define _CMU_LFACLKEN0_LESENSE_SHIFT                0                                      /**< Shift value for CMU_LESENSE */
#define _CMU_LFACLKEN0_LESENSE_MASK                 0x1UL                                  /**< Bit mask for CMU_LESENSE */
#define _CMU_LFACLKEN0_LESENSE_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LESENSE_DEFAULT               (_CMU_LFACLKEN0_LESENSE_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_RTC                           (0x1UL << 1)                           /**< Real-Time Counter Clock Enable */
#define _CMU_LFACLKEN0_RTC_SHIFT                    1                                      /**< Shift value for CMU_RTC */
#define _CMU_LFACLKEN0_RTC_MASK                     0x2UL                                  /**< Bit mask for CMU_RTC */
#define _CMU_LFACLKEN0_RTC_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_RTC_DEFAULT                   (_CMU_LFACLKEN0_RTC_DEFAULT << 1)      /**< Shifted mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LETIMER0                      (0x1UL << 2)                           /**< Low Energy Timer 0 Clock Enable */
#define _CMU_LFACLKEN0_LETIMER0_SHIFT               2                                      /**< Shift value for CMU_LETIMER0 */
#define _CMU_LFACLKEN0_LETIMER0_MASK                0x4UL                                  /**< Bit mask for CMU_LETIMER0 */
#define _CMU_LFACLKEN0_LETIMER0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LETIMER0_DEFAULT              (_CMU_LFACLKEN0_LETIMER0_DEFAULT << 2) /**< Shifted mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LCD                           (0x1UL << 3)                           /**< Liquid Crystal Display Controller Clock Enable */
#define _CMU_LFACLKEN0_LCD_SHIFT                    3                                      /**< Shift value for CMU_LCD */
#define _CMU_LFACLKEN0_LCD_MASK                     0x8UL                                  /**< Bit mask for CMU_LCD */
#define _CMU_LFACLKEN0_LCD_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LCD_DEFAULT                   (_CMU_LFACLKEN0_LCD_DEFAULT << 3)      /**< Shifted mode DEFAULT for CMU_LFACLKEN0 */

/* Bit fields for CMU LFBCLKEN0 */
#define _CMU_LFBCLKEN0_RESETVALUE                   0x00000000UL                          /**< Default value for CMU_LFBCLKEN0 */
#define _CMU_LFBCLKEN0_MASK                         0x00000003UL                          /**< Mask for CMU_LFBCLKEN0 */
#define CMU_LFBCLKEN0_LEUART0                       (0x1UL << 0)                          /**< Low Energy UART 0 Clock Enable */
#define _CMU_LFBCLKEN0_LEUART0_SHIFT                0                                     /**< Shift value for CMU_LEUART0 */
#define _CMU_LFBCLKEN0_LEUART0_MASK                 0x1UL                                 /**< Bit mask for CMU_LEUART0 */
#define _CMU_LFBCLKEN0_LEUART0_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for CMU_LFBCLKEN0 */
#define CMU_LFBCLKEN0_LEUART0_DEFAULT               (_CMU_LFBCLKEN0_LEUART0_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_LFBCLKEN0 */
#define CMU_LFBCLKEN0_LEUART1                       (0x1UL << 1)                          /**< Low Energy UART 1 Clock Enable */
#define _CMU_LFBCLKEN0_LEUART1_SHIFT                1                                     /**< Shift value for CMU_LEUART1 */
#define _CMU_LFBCLKEN0_LEUART1_MASK                 0x2UL                                 /**< Bit mask for CMU_LEUART1 */
#define _CMU_LFBCLKEN0_LEUART1_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for CMU_LFBCLKEN0 */
#define CMU_LFBCLKEN0_LEUART1_DEFAULT               (_CMU_LFBCLKEN0_LEUART1_DEFAULT << 1) /**< Shifted mode DEFAULT for CMU_LFBCLKEN0 */

/* Bit fields for CMU LFAPRESC0 */
#define _CMU_LFAPRESC0_RESETVALUE                   0x00000000UL                            /**< Default value for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_MASK                         0x00003FF3UL                            /**< Mask for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LESENSE_SHIFT                0                                       /**< Shift value for CMU_LESENSE */
#define _CMU_LFAPRESC0_LESENSE_MASK                 0x3UL                                   /**< Bit mask for CMU_LESENSE */
#define _CMU_LFAPRESC0_LESENSE_DIV1                 0x00000000UL                            /**< Mode DIV1 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LESENSE_DIV2                 0x00000001UL                            /**< Mode DIV2 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LESENSE_DIV4                 0x00000002UL                            /**< Mode DIV4 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LESENSE_DIV8                 0x00000003UL                            /**< Mode DIV8 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LESENSE_DIV1                  (_CMU_LFAPRESC0_LESENSE_DIV1 << 0)      /**< Shifted mode DIV1 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LESENSE_DIV2                  (_CMU_LFAPRESC0_LESENSE_DIV2 << 0)      /**< Shifted mode DIV2 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LESENSE_DIV4                  (_CMU_LFAPRESC0_LESENSE_DIV4 << 0)      /**< Shifted mode DIV4 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LESENSE_DIV8                  (_CMU_LFAPRESC0_LESENSE_DIV8 << 0)      /**< Shifted mode DIV8 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_SHIFT                    4                                       /**< Shift value for CMU_RTC */
#define _CMU_LFAPRESC0_RTC_MASK                     0xF0UL                                  /**< Bit mask for CMU_RTC */
#define _CMU_LFAPRESC0_RTC_DIV1                     0x00000000UL                            /**< Mode DIV1 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV2                     0x00000001UL                            /**< Mode DIV2 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV4                     0x00000002UL                            /**< Mode DIV4 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV8                     0x00000003UL                            /**< Mode DIV8 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV16                    0x00000004UL                            /**< Mode DIV16 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV32                    0x00000005UL                            /**< Mode DIV32 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV64                    0x00000006UL                            /**< Mode DIV64 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV128                   0x00000007UL                            /**< Mode DIV128 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV256                   0x00000008UL                            /**< Mode DIV256 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV512                   0x00000009UL                            /**< Mode DIV512 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV1024                  0x0000000AUL                            /**< Mode DIV1024 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV2048                  0x0000000BUL                            /**< Mode DIV2048 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV4096                  0x0000000CUL                            /**< Mode DIV4096 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV8192                  0x0000000DUL                            /**< Mode DIV8192 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV16384                 0x0000000EUL                            /**< Mode DIV16384 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV32768                 0x0000000FUL                            /**< Mode DIV32768 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV1                      (_CMU_LFAPRESC0_RTC_DIV1 << 4)          /**< Shifted mode DIV1 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV2                      (_CMU_LFAPRESC0_RTC_DIV2 << 4)          /**< Shifted mode DIV2 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV4                      (_CMU_LFAPRESC0_RTC_DIV4 << 4)          /**< Shifted mode DIV4 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV8                      (_CMU_LFAPRESC0_RTC_DIV8 << 4)          /**< Shifted mode DIV8 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV16                     (_CMU_LFAPRESC0_RTC_DIV16 << 4)         /**< Shifted mode DIV16 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV32                     (_CMU_LFAPRESC0_RTC_DIV32 << 4)         /**< Shifted mode DIV32 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV64                     (_CMU_LFAPRESC0_RTC_DIV64 << 4)         /**< Shifted mode DIV64 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV128                    (_CMU_LFAPRESC0_RTC_DIV128 << 4)        /**< Shifted mode DIV128 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV256                    (_CMU_LFAPRESC0_RTC_DIV256 << 4)        /**< Shifted mode DIV256 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV512                    (_CMU_LFAPRESC0_RTC_DIV512 << 4)        /**< Shifted mode DIV512 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV1024                   (_CMU_LFAPRESC0_RTC_DIV1024 << 4)       /**< Shifted mode DIV1024 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV2048                   (_CMU_LFAPRESC0_RTC_DIV2048 << 4)       /**< Shifted mode DIV2048 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV4096                   (_CMU_LFAPRESC0_RTC_DIV4096 << 4)       /**< Shifted mode DIV4096 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV8192                   (_CMU_LFAPRESC0_RTC_DIV8192 << 4)       /**< Shifted mode DIV8192 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV16384                  (_CMU_LFAPRESC0_RTC_DIV16384 << 4)      /**< Shifted mode DIV16384 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV32768                  (_CMU_LFAPRESC0_RTC_DIV32768 << 4)      /**< Shifted mode DIV32768 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_SHIFT               8                                       /**< Shift value for CMU_LETIMER0 */
#define _CMU_LFAPRESC0_LETIMER0_MASK                0xF00UL                                 /**< Bit mask for CMU_LETIMER0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV1                0x00000000UL                            /**< Mode DIV1 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV2                0x00000001UL                            /**< Mode DIV2 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV4                0x00000002UL                            /**< Mode DIV4 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV8                0x00000003UL                            /**< Mode DIV8 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV16               0x00000004UL                            /**< Mode DIV16 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV32               0x00000005UL                            /**< Mode DIV32 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV64               0x00000006UL                            /**< Mode DIV64 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV128              0x00000007UL                            /**< Mode DIV128 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV256              0x00000008UL                            /**< Mode DIV256 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV512              0x00000009UL                            /**< Mode DIV512 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV1024             0x0000000AUL                            /**< Mode DIV1024 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV2048             0x0000000BUL                            /**< Mode DIV2048 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV4096             0x0000000CUL                            /**< Mode DIV4096 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV8192             0x0000000DUL                            /**< Mode DIV8192 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV16384            0x0000000EUL                            /**< Mode DIV16384 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV32768            0x0000000FUL                            /**< Mode DIV32768 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV1                 (_CMU_LFAPRESC0_LETIMER0_DIV1 << 8)     /**< Shifted mode DIV1 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV2                 (_CMU_LFAPRESC0_LETIMER0_DIV2 << 8)     /**< Shifted mode DIV2 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV4                 (_CMU_LFAPRESC0_LETIMER0_DIV4 << 8)     /**< Shifted mode DIV4 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV8                 (_CMU_LFAPRESC0_LETIMER0_DIV8 << 8)     /**< Shifted mode DIV8 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV16                (_CMU_LFAPRESC0_LETIMER0_DIV16 << 8)    /**< Shifted mode DIV16 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV32                (_CMU_LFAPRESC0_LETIMER0_DIV32 << 8)    /**< Shifted mode DIV32 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV64                (_CMU_LFAPRESC0_LETIMER0_DIV64 << 8)    /**< Shifted mode DIV64 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV128               (_CMU_LFAPRESC0_LETIMER0_DIV128 << 8)   /**< Shifted mode DIV128 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV256               (_CMU_LFAPRESC0_LETIMER0_DIV256 << 8)   /**< Shifted mode DIV256 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV512               (_CMU_LFAPRESC0_LETIMER0_DIV512 << 8)   /**< Shifted mode DIV512 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV1024              (_CMU_LFAPRESC0_LETIMER0_DIV1024 << 8)  /**< Shifted mode DIV1024 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV2048              (_CMU_LFAPRESC0_LETIMER0_DIV2048 << 8)  /**< Shifted mode DIV2048 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV4096              (_CMU_LFAPRESC0_LETIMER0_DIV4096 << 8)  /**< Shifted mode DIV4096 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV8192              (_CMU_LFAPRESC0_LETIMER0_DIV8192 << 8)  /**< Shifted mode DIV8192 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV16384             (_CMU_LFAPRESC0_LETIMER0_DIV16384 << 8) /**< Shifted mode DIV16384 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV32768             (_CMU_LFAPRESC0_LETIMER0_DIV32768 << 8) /**< Shifted mode DIV32768 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LCD_SHIFT                    12                                      /**< Shift value for CMU_LCD */
#define _CMU_LFAPRESC0_LCD_MASK                     0x3000UL                                /**< Bit mask for CMU_LCD */
#define _CMU_LFAPRESC0_LCD_DIV16                    0x00000000UL                            /**< Mode DIV16 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LCD_DIV32                    0x00000001UL                            /**< Mode DIV32 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LCD_DIV64                    0x00000002UL                            /**< Mode DIV64 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LCD_DIV128                   0x00000003UL                            /**< Mode DIV128 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LCD_DIV16                     (_CMU_LFAPRESC0_LCD_DIV16 << 12)        /**< Shifted mode DIV16 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LCD_DIV32                     (_CMU_LFAPRESC0_LCD_DIV32 << 12)        /**< Shifted mode DIV32 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LCD_DIV64                     (_CMU_LFAPRESC0_LCD_DIV64 << 12)        /**< Shifted mode DIV64 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LCD_DIV128                    (_CMU_LFAPRESC0_LCD_DIV128 << 12)       /**< Shifted mode DIV128 for CMU_LFAPRESC0 */

/* Bit fields for CMU LFBPRESC0 */
#define _CMU_LFBPRESC0_RESETVALUE                   0x00000000UL                       /**< Default value for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_MASK                         0x00000033UL                       /**< Mask for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART0_SHIFT                0                                  /**< Shift value for CMU_LEUART0 */
#define _CMU_LFBPRESC0_LEUART0_MASK                 0x3UL                              /**< Bit mask for CMU_LEUART0 */
#define _CMU_LFBPRESC0_LEUART0_DIV1                 0x00000000UL                       /**< Mode DIV1 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART0_DIV2                 0x00000001UL                       /**< Mode DIV2 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART0_DIV4                 0x00000002UL                       /**< Mode DIV4 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART0_DIV8                 0x00000003UL                       /**< Mode DIV8 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART0_DIV1                  (_CMU_LFBPRESC0_LEUART0_DIV1 << 0) /**< Shifted mode DIV1 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART0_DIV2                  (_CMU_LFBPRESC0_LEUART0_DIV2 << 0) /**< Shifted mode DIV2 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART0_DIV4                  (_CMU_LFBPRESC0_LEUART0_DIV4 << 0) /**< Shifted mode DIV4 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART0_DIV8                  (_CMU_LFBPRESC0_LEUART0_DIV8 << 0) /**< Shifted mode DIV8 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART1_SHIFT                4                                  /**< Shift value for CMU_LEUART1 */
#define _CMU_LFBPRESC0_LEUART1_MASK                 0x30UL                             /**< Bit mask for CMU_LEUART1 */
#define _CMU_LFBPRESC0_LEUART1_DIV1                 0x00000000UL                       /**< Mode DIV1 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART1_DIV2                 0x00000001UL                       /**< Mode DIV2 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART1_DIV4                 0x00000002UL                       /**< Mode DIV4 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART1_DIV8                 0x00000003UL                       /**< Mode DIV8 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART1_DIV1                  (_CMU_LFBPRESC0_LEUART1_DIV1 << 4) /**< Shifted mode DIV1 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART1_DIV2                  (_CMU_LFBPRESC0_LEUART1_DIV2 << 4) /**< Shifted mode DIV2 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART1_DIV4                  (_CMU_LFBPRESC0_LEUART1_DIV4 << 4) /**< Shifted mode DIV4 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART1_DIV8                  (_CMU_LFBPRESC0_LEUART1_DIV8 << 4) /**< Shifted mode DIV8 for CMU_LFBPRESC0 */

/* Bit fields for CMU PCNTCTRL */
#define _CMU_PCNTCTRL_RESETVALUE                    0x00000000UL                             /**< Default value for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_MASK                          0x0000003FUL                             /**< Mask for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKEN                     (0x1UL << 0)                             /**< PCNT0 Clock Enable */
#define _CMU_PCNTCTRL_PCNT0CLKEN_SHIFT              0                                        /**< Shift value for CMU_PCNT0CLKEN */
#define _CMU_PCNTCTRL_PCNT0CLKEN_MASK               0x1UL                                    /**< Bit mask for CMU_PCNT0CLKEN */
#define _CMU_PCNTCTRL_PCNT0CLKEN_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKEN_DEFAULT             (_CMU_PCNTCTRL_PCNT0CLKEN_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKSEL                    (0x1UL << 1)                             /**< PCNT0 Clock Select */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_SHIFT             1                                        /**< Shift value for CMU_PCNT0CLKSEL */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_MASK              0x2UL                                    /**< Bit mask for CMU_PCNT0CLKSEL */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_LFACLK            0x00000000UL                             /**< Mode LFACLK for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_PCNT0S0           0x00000001UL                             /**< Mode PCNT0S0 for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKSEL_DEFAULT            (_CMU_PCNTCTRL_PCNT0CLKSEL_DEFAULT << 1) /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKSEL_LFACLK             (_CMU_PCNTCTRL_PCNT0CLKSEL_LFACLK << 1)  /**< Shifted mode LFACLK for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKSEL_PCNT0S0            (_CMU_PCNTCTRL_PCNT0CLKSEL_PCNT0S0 << 1) /**< Shifted mode PCNT0S0 for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT1CLKEN                     (0x1UL << 2)                             /**< PCNT1 Clock Enable */
#define _CMU_PCNTCTRL_PCNT1CLKEN_SHIFT              2                                        /**< Shift value for CMU_PCNT1CLKEN */
#define _CMU_PCNTCTRL_PCNT1CLKEN_MASK               0x4UL                                    /**< Bit mask for CMU_PCNT1CLKEN */
#define _CMU_PCNTCTRL_PCNT1CLKEN_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT1CLKEN_DEFAULT             (_CMU_PCNTCTRL_PCNT1CLKEN_DEFAULT << 2)  /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT1CLKSEL                    (0x1UL << 3)                             /**< PCNT1 Clock Select */
#define _CMU_PCNTCTRL_PCNT1CLKSEL_SHIFT             3                                        /**< Shift value for CMU_PCNT1CLKSEL */
#define _CMU_PCNTCTRL_PCNT1CLKSEL_MASK              0x8UL                                    /**< Bit mask for CMU_PCNT1CLKSEL */
#define _CMU_PCNTCTRL_PCNT1CLKSEL_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT1CLKSEL_LFACLK            0x00000000UL                             /**< Mode LFACLK for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT1CLKSEL_PCNT1S0           0x00000001UL                             /**< Mode PCNT1S0 for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT1CLKSEL_DEFAULT            (_CMU_PCNTCTRL_PCNT1CLKSEL_DEFAULT << 3) /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT1CLKSEL_LFACLK             (_CMU_PCNTCTRL_PCNT1CLKSEL_LFACLK << 3)  /**< Shifted mode LFACLK for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT1CLKSEL_PCNT1S0            (_CMU_PCNTCTRL_PCNT1CLKSEL_PCNT1S0 << 3) /**< Shifted mode PCNT1S0 for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT2CLKEN                     (0x1UL << 4)                             /**< PCNT2 Clock Enable */
#define _CMU_PCNTCTRL_PCNT2CLKEN_SHIFT              4                                        /**< Shift value for CMU_PCNT2CLKEN */
#define _CMU_PCNTCTRL_PCNT2CLKEN_MASK               0x10UL                                   /**< Bit mask for CMU_PCNT2CLKEN */
#define _CMU_PCNTCTRL_PCNT2CLKEN_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT2CLKEN_DEFAULT             (_CMU_PCNTCTRL_PCNT2CLKEN_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT2CLKSEL                    (0x1UL << 5)                             /**< PCNT2 Clock Select */
#define _CMU_PCNTCTRL_PCNT2CLKSEL_SHIFT             5                                        /**< Shift value for CMU_PCNT2CLKSEL */
#define _CMU_PCNTCTRL_PCNT2CLKSEL_MASK              0x20UL                                   /**< Bit mask for CMU_PCNT2CLKSEL */
#define _CMU_PCNTCTRL_PCNT2CLKSEL_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT2CLKSEL_LFACLK            0x00000000UL                             /**< Mode LFACLK for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT2CLKSEL_PCNT2S0           0x00000001UL                             /**< Mode PCNT2S0 for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT2CLKSEL_DEFAULT            (_CMU_PCNTCTRL_PCNT2CLKSEL_DEFAULT << 5) /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT2CLKSEL_LFACLK             (_CMU_PCNTCTRL_PCNT2CLKSEL_LFACLK << 5)  /**< Shifted mode LFACLK for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT2CLKSEL_PCNT2S0            (_CMU_PCNTCTRL_PCNT2CLKSEL_PCNT2S0 << 5) /**< Shifted mode PCNT2S0 for CMU_PCNTCTRL */

/* Bit fields for CMU LCDCTRL */
#define _CMU_LCDCTRL_RESETVALUE                     0x00000020UL                         /**< Default value for CMU_LCDCTRL */
#define _CMU_LCDCTRL_MASK                           0x0000007FUL                         /**< Mask for CMU_LCDCTRL */
#define _CMU_LCDCTRL_FDIV_SHIFT                     0                                    /**< Shift value for CMU_FDIV */
#define _CMU_LCDCTRL_FDIV_MASK                      0x7UL                                /**< Bit mask for CMU_FDIV */
#define _CMU_LCDCTRL_FDIV_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_LCDCTRL */
#define CMU_LCDCTRL_FDIV_DEFAULT                    (_CMU_LCDCTRL_FDIV_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBOOSTEN                        (0x1UL << 3)                         /**< Voltage Boost Enable */
#define _CMU_LCDCTRL_VBOOSTEN_SHIFT                 3                                    /**< Shift value for CMU_VBOOSTEN */
#define _CMU_LCDCTRL_VBOOSTEN_MASK                  0x8UL                                /**< Bit mask for CMU_VBOOSTEN */
#define _CMU_LCDCTRL_VBOOSTEN_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBOOSTEN_DEFAULT                (_CMU_LCDCTRL_VBOOSTEN_DEFAULT << 3) /**< Shifted mode DEFAULT for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_SHIFT                   4                                    /**< Shift value for CMU_VBFDIV */
#define _CMU_LCDCTRL_VBFDIV_MASK                    0x70UL                               /**< Bit mask for CMU_VBFDIV */
#define _CMU_LCDCTRL_VBFDIV_DIV1                    0x00000000UL                         /**< Mode DIV1 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV2                    0x00000001UL                         /**< Mode DIV2 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DEFAULT                 0x00000002UL                         /**< Mode DEFAULT for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV4                    0x00000002UL                         /**< Mode DIV4 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV8                    0x00000003UL                         /**< Mode DIV8 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV16                   0x00000004UL                         /**< Mode DIV16 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV32                   0x00000005UL                         /**< Mode DIV32 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV64                   0x00000006UL                         /**< Mode DIV64 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV128                  0x00000007UL                         /**< Mode DIV128 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV1                     (_CMU_LCDCTRL_VBFDIV_DIV1 << 4)      /**< Shifted mode DIV1 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV2                     (_CMU_LCDCTRL_VBFDIV_DIV2 << 4)      /**< Shifted mode DIV2 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DEFAULT                  (_CMU_LCDCTRL_VBFDIV_DEFAULT << 4)   /**< Shifted mode DEFAULT for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV4                     (_CMU_LCDCTRL_VBFDIV_DIV4 << 4)      /**< Shifted mode DIV4 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV8                     (_CMU_LCDCTRL_VBFDIV_DIV8 << 4)      /**< Shifted mode DIV8 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV16                    (_CMU_LCDCTRL_VBFDIV_DIV16 << 4)     /**< Shifted mode DIV16 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV32                    (_CMU_LCDCTRL_VBFDIV_DIV32 << 4)     /**< Shifted mode DIV32 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV64                    (_CMU_LCDCTRL_VBFDIV_DIV64 << 4)     /**< Shifted mode DIV64 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV128                   (_CMU_LCDCTRL_VBFDIV_DIV128 << 4)    /**< Shifted mode DIV128 for CMU_LCDCTRL */

/* Bit fields for CMU ROUTE */
#define _CMU_ROUTE_RESETVALUE                       0x00000000UL                         /**< Default value for CMU_ROUTE */
#define _CMU_ROUTE_MASK                             0x0000001FUL                         /**< Mask for CMU_ROUTE */
#define CMU_ROUTE_CLKOUT0PEN                        (0x1UL << 0)                         /**< CLKOUT0 Pin Enable */
#define _CMU_ROUTE_CLKOUT0PEN_SHIFT                 0                                    /**< Shift value for CMU_CLKOUT0PEN */
#define _CMU_ROUTE_CLKOUT0PEN_MASK                  0x1UL                                /**< Bit mask for CMU_CLKOUT0PEN */
#define _CMU_ROUTE_CLKOUT0PEN_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_ROUTE */
#define CMU_ROUTE_CLKOUT0PEN_DEFAULT                (_CMU_ROUTE_CLKOUT0PEN_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_ROUTE */
#define CMU_ROUTE_CLKOUT1PEN                        (0x1UL << 1)                         /**< CLKOUT1 Pin Enable */
#define _CMU_ROUTE_CLKOUT1PEN_SHIFT                 1                                    /**< Shift value for CMU_CLKOUT1PEN */
#define _CMU_ROUTE_CLKOUT1PEN_MASK                  0x2UL                                /**< Bit mask for CMU_CLKOUT1PEN */
#define _CMU_ROUTE_CLKOUT1PEN_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_ROUTE */
#define CMU_ROUTE_CLKOUT1PEN_DEFAULT                (_CMU_ROUTE_CLKOUT1PEN_DEFAULT << 1) /**< Shifted mode DEFAULT for CMU_ROUTE */
#define _CMU_ROUTE_LOCATION_SHIFT                   2                                    /**< Shift value for CMU_LOCATION */
#define _CMU_ROUTE_LOCATION_MASK                    0x1CUL                               /**< Bit mask for CMU_LOCATION */
#define _CMU_ROUTE_LOCATION_LOC0                    0x00000000UL                         /**< Mode LOC0 for CMU_ROUTE */
#define _CMU_ROUTE_LOCATION_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for CMU_ROUTE */
#define _CMU_ROUTE_LOCATION_LOC1                    0x00000001UL                         /**< Mode LOC1 for CMU_ROUTE */
#define _CMU_ROUTE_LOCATION_LOC2                    0x00000002UL                         /**< Mode LOC2 for CMU_ROUTE */
#define CMU_ROUTE_LOCATION_LOC0                     (_CMU_ROUTE_LOCATION_LOC0 << 2)      /**< Shifted mode LOC0 for CMU_ROUTE */
#define CMU_ROUTE_LOCATION_DEFAULT                  (_CMU_ROUTE_LOCATION_DEFAULT << 2)   /**< Shifted mode DEFAULT for CMU_ROUTE */
#define CMU_ROUTE_LOCATION_LOC1                     (_CMU_ROUTE_LOCATION_LOC1 << 2)      /**< Shifted mode LOC1 for CMU_ROUTE */
#define CMU_ROUTE_LOCATION_LOC2                     (_CMU_ROUTE_LOCATION_LOC2 << 2)      /**< Shifted mode LOC2 for CMU_ROUTE */

/* Bit fields for CMU LOCK */
#define _CMU_LOCK_RESETVALUE                        0x00000000UL                      /**< Default value for CMU_LOCK */
#define _CMU_LOCK_MASK                              0x0000FFFFUL                      /**< Mask for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_SHIFT                     0                                 /**< Shift value for CMU_LOCKKEY */
#define _CMU_LOCK_LOCKKEY_MASK                      0xFFFFUL                          /**< Bit mask for CMU_LOCKKEY */
#define _CMU_LOCK_LOCKKEY_DEFAULT                   0x00000000UL                      /**< Mode DEFAULT for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_LOCK                      0x00000000UL                      /**< Mode LOCK for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_UNLOCKED                  0x00000000UL                      /**< Mode UNLOCKED for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_LOCKED                    0x00000001UL                      /**< Mode LOCKED for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_UNLOCK                    0x0000580EUL                      /**< Mode UNLOCK for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_DEFAULT                    (_CMU_LOCK_LOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_LOCK                       (_CMU_LOCK_LOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_UNLOCKED                   (_CMU_LOCK_LOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_LOCKED                     (_CMU_LOCK_LOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_UNLOCK                     (_CMU_LOCK_LOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for CMU_LOCK */

/** @} End of group EFM32WG942F64_CMU */



/**************************************************************************//**
 * @defgroup EFM32WG942F64_PRS_BitFields  EFM32WG942F64_PRS Bit Fields
 * @{
 *****************************************************************************/

/* Bit fields for PRS SWPULSE */
#define _PRS_SWPULSE_RESETVALUE                 0x00000000UL                           /**< Default value for PRS_SWPULSE */
#define _PRS_SWPULSE_MASK                       0x00000FFFUL                           /**< Mask for PRS_SWPULSE */
#define PRS_SWPULSE_CH0PULSE                    (0x1UL << 0)                           /**< Channel 0 Pulse Generation */
#define _PRS_SWPULSE_CH0PULSE_SHIFT             0                                      /**< Shift value for PRS_CH0PULSE */
#define _PRS_SWPULSE_CH0PULSE_MASK              0x1UL                                  /**< Bit mask for PRS_CH0PULSE */
#define _PRS_SWPULSE_CH0PULSE_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH0PULSE_DEFAULT            (_PRS_SWPULSE_CH0PULSE_DEFAULT << 0)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH1PULSE                    (0x1UL << 1)                           /**< Channel 1 Pulse Generation */
#define _PRS_SWPULSE_CH1PULSE_SHIFT             1                                      /**< Shift value for PRS_CH1PULSE */
#define _PRS_SWPULSE_CH1PULSE_MASK              0x2UL                                  /**< Bit mask for PRS_CH1PULSE */
#define _PRS_SWPULSE_CH1PULSE_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH1PULSE_DEFAULT            (_PRS_SWPULSE_CH1PULSE_DEFAULT << 1)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH2PULSE                    (0x1UL << 2)                           /**< Channel 2 Pulse Generation */
#define _PRS_SWPULSE_CH2PULSE_SHIFT             2                                      /**< Shift value for PRS_CH2PULSE */
#define _PRS_SWPULSE_CH2PULSE_MASK              0x4UL                                  /**< Bit mask for PRS_CH2PULSE */
#define _PRS_SWPULSE_CH2PULSE_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH2PULSE_DEFAULT            (_PRS_SWPULSE_CH2PULSE_DEFAULT << 2)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH3PULSE                    (0x1UL << 3)                           /**< Channel 3 Pulse Generation */
#define _PRS_SWPULSE_CH3PULSE_SHIFT             3                                      /**< Shift value for PRS_CH3PULSE */
#define _PRS_SWPULSE_CH3PULSE_MASK              0x8UL                                  /**< Bit mask for PRS_CH3PULSE */
#define _PRS_SWPULSE_CH3PULSE_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH3PULSE_DEFAULT            (_PRS_SWPULSE_CH3PULSE_DEFAULT << 3)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH4PULSE                    (0x1UL << 4)                           /**< Channel 4 Pulse Generation */
#define _PRS_SWPULSE_CH4PULSE_SHIFT             4                                      /**< Shift value for PRS_CH4PULSE */
#define _PRS_SWPULSE_CH4PULSE_MASK              0x10UL                                 /**< Bit mask for PRS_CH4PULSE */
#define _PRS_SWPULSE_CH4PULSE_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH4PULSE_DEFAULT            (_PRS_SWPULSE_CH4PULSE_DEFAULT << 4)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH5PULSE                    (0x1UL << 5)                           /**< Channel 5 Pulse Generation */
#define _PRS_SWPULSE_CH5PULSE_SHIFT             5                                      /**< Shift value for PRS_CH5PULSE */
#define _PRS_SWPULSE_CH5PULSE_MASK              0x20UL                                 /**< Bit mask for PRS_CH5PULSE */
#define _PRS_SWPULSE_CH5PULSE_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH5PULSE_DEFAULT            (_PRS_SWPULSE_CH5PULSE_DEFAULT << 5)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH6PULSE                    (0x1UL << 6)                           /**< Channel 6 Pulse Generation */
#define _PRS_SWPULSE_CH6PULSE_SHIFT             6                                      /**< Shift value for PRS_CH6PULSE */
#define _PRS_SWPULSE_CH6PULSE_MASK              0x40UL                                 /**< Bit mask for PRS_CH6PULSE */
#define _PRS_SWPULSE_CH6PULSE_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH6PULSE_DEFAULT            (_PRS_SWPULSE_CH6PULSE_DEFAULT << 6)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH7PULSE                    (0x1UL << 7)                           /**< Channel 7 Pulse Generation */
#define _PRS_SWPULSE_CH7PULSE_SHIFT             7                                      /**< Shift value for PRS_CH7PULSE */
#define _PRS_SWPULSE_CH7PULSE_MASK              0x80UL                                 /**< Bit mask for PRS_CH7PULSE */
#define _PRS_SWPULSE_CH7PULSE_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH7PULSE_DEFAULT            (_PRS_SWPULSE_CH7PULSE_DEFAULT << 7)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH8PULSE                    (0x1UL << 8)                           /**< Channel 8 Pulse Generation */
#define _PRS_SWPULSE_CH8PULSE_SHIFT             8                                      /**< Shift value for PRS_CH8PULSE */
#define _PRS_SWPULSE_CH8PULSE_MASK              0x100UL                                /**< Bit mask for PRS_CH8PULSE */
#define _PRS_SWPULSE_CH8PULSE_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH8PULSE_DEFAULT            (_PRS_SWPULSE_CH8PULSE_DEFAULT << 8)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH9PULSE                    (0x1UL << 9)                           /**< Channel 9 Pulse Generation */
#define _PRS_SWPULSE_CH9PULSE_SHIFT             9                                      /**< Shift value for PRS_CH9PULSE */
#define _PRS_SWPULSE_CH9PULSE_MASK              0x200UL                                /**< Bit mask for PRS_CH9PULSE */
#define _PRS_SWPULSE_CH9PULSE_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH9PULSE_DEFAULT            (_PRS_SWPULSE_CH9PULSE_DEFAULT << 9)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH10PULSE                   (0x1UL << 10)                          /**< Channel 10 Pulse Generation */
#define _PRS_SWPULSE_CH10PULSE_SHIFT            10                                     /**< Shift value for PRS_CH10PULSE */
#define _PRS_SWPULSE_CH10PULSE_MASK             0x400UL                                /**< Bit mask for PRS_CH10PULSE */
#define _PRS_SWPULSE_CH10PULSE_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH10PULSE_DEFAULT           (_PRS_SWPULSE_CH10PULSE_DEFAULT << 10) /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH11PULSE                   (0x1UL << 11)                          /**< Channel 11 Pulse Generation */
#define _PRS_SWPULSE_CH11PULSE_SHIFT            11                                     /**< Shift value for PRS_CH11PULSE */
#define _PRS_SWPULSE_CH11PULSE_MASK             0x800UL                                /**< Bit mask for PRS_CH11PULSE */
#define _PRS_SWPULSE_CH11PULSE_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH11PULSE_DEFAULT           (_PRS_SWPULSE_CH11PULSE_DEFAULT << 11) /**< Shifted mode DEFAULT for PRS_SWPULSE */

/* Bit fields for PRS SWLEVEL */
#define _PRS_SWLEVEL_RESETVALUE                 0x00000000UL                           /**< Default value for PRS_SWLEVEL */
#define _PRS_SWLEVEL_MASK                       0x00000FFFUL                           /**< Mask for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH0LEVEL                    (0x1UL << 0)                           /**< Channel 0 Software Level */
#define _PRS_SWLEVEL_CH0LEVEL_SHIFT             0                                      /**< Shift value for PRS_CH0LEVEL */
#define _PRS_SWLEVEL_CH0LEVEL_MASK              0x1UL                                  /**< Bit mask for PRS_CH0LEVEL */
#define _PRS_SWLEVEL_CH0LEVEL_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH0LEVEL_DEFAULT            (_PRS_SWLEVEL_CH0LEVEL_DEFAULT << 0)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH1LEVEL                    (0x1UL << 1)                           /**< Channel 1 Software Level */
#define _PRS_SWLEVEL_CH1LEVEL_SHIFT             1                                      /**< Shift value for PRS_CH1LEVEL */
#define _PRS_SWLEVEL_CH1LEVEL_MASK              0x2UL                                  /**< Bit mask for PRS_CH1LEVEL */
#define _PRS_SWLEVEL_CH1LEVEL_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH1LEVEL_DEFAULT            (_PRS_SWLEVEL_CH1LEVEL_DEFAULT << 1)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH2LEVEL                    (0x1UL << 2)                           /**< Channel 2 Software Level */
#define _PRS_SWLEVEL_CH2LEVEL_SHIFT             2                                      /**< Shift value for PRS_CH2LEVEL */
#define _PRS_SWLEVEL_CH2LEVEL_MASK              0x4UL                                  /**< Bit mask for PRS_CH2LEVEL */
#define _PRS_SWLEVEL_CH2LEVEL_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH2LEVEL_DEFAULT            (_PRS_SWLEVEL_CH2LEVEL_DEFAULT << 2)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH3LEVEL                    (0x1UL << 3)                           /**< Channel 3 Software Level */
#define _PRS_SWLEVEL_CH3LEVEL_SHIFT             3                                      /**< Shift value for PRS_CH3LEVEL */
#define _PRS_SWLEVEL_CH3LEVEL_MASK              0x8UL                                  /**< Bit mask for PRS_CH3LEVEL */
#define _PRS_SWLEVEL_CH3LEVEL_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH3LEVEL_DEFAULT            (_PRS_SWLEVEL_CH3LEVEL_DEFAULT << 3)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH4LEVEL                    (0x1UL << 4)                           /**< Channel 4 Software Level */
#define _PRS_SWLEVEL_CH4LEVEL_SHIFT             4                                      /**< Shift value for PRS_CH4LEVEL */
#define _PRS_SWLEVEL_CH4LEVEL_MASK              0x10UL                                 /**< Bit mask for PRS_CH4LEVEL */
#define _PRS_SWLEVEL_CH4LEVEL_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH4LEVEL_DEFAULT            (_PRS_SWLEVEL_CH4LEVEL_DEFAULT << 4)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH5LEVEL                    (0x1UL << 5)                           /**< Channel 5 Software Level */
#define _PRS_SWLEVEL_CH5LEVEL_SHIFT             5                                      /**< Shift value for PRS_CH5LEVEL */
#define _PRS_SWLEVEL_CH5LEVEL_MASK              0x20UL                                 /**< Bit mask for PRS_CH5LEVEL */
#define _PRS_SWLEVEL_CH5LEVEL_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH5LEVEL_DEFAULT            (_PRS_SWLEVEL_CH5LEVEL_DEFAULT << 5)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH6LEVEL                    (0x1UL << 6)                           /**< Channel 6 Software Level */
#define _PRS_SWLEVEL_CH6LEVEL_SHIFT             6                                      /**< Shift value for PRS_CH6LEVEL */
#define _PRS_SWLEVEL_CH6LEVEL_MASK              0x40UL                                 /**< Bit mask for PRS_CH6LEVEL */
#define _PRS_SWLEVEL_CH6LEVEL_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH6LEVEL_DEFAULT            (_PRS_SWLEVEL_CH6LEVEL_DEFAULT << 6)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH7LEVEL                    (0x1UL << 7)                           /**< Channel 7 Software Level */
#define _PRS_SWLEVEL_CH7LEVEL_SHIFT             7                                      /**< Shift value for PRS_CH7LEVEL */
#define _PRS_SWLEVEL_CH7LEVEL_MASK              0x80UL                                 /**< Bit mask for PRS_CH7LEVEL */
#define _PRS_SWLEVEL_CH7LEVEL_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH7LEVEL_DEFAULT            (_PRS_SWLEVEL_CH7LEVEL_DEFAULT << 7)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH8LEVEL                    (0x1UL << 8)                           /**< Channel 8 Software Level */
#define _PRS_SWLEVEL_CH8LEVEL_SHIFT             8                                      /**< Shift value for PRS_CH8LEVEL */
#define _PRS_SWLEVEL_CH8LEVEL_MASK              0x100UL                                /**< Bit mask for PRS_CH8LEVEL */
#define _PRS_SWLEVEL_CH8LEVEL_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH8LEVEL_DEFAULT            (_PRS_SWLEVEL_CH8LEVEL_DEFAULT << 8)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH9LEVEL                    (0x1UL << 9)                           /**< Channel 9 Software Level */
#define _PRS_SWLEVEL_CH9LEVEL_SHIFT             9                                      /**< Shift value for PRS_CH9LEVEL */
#define _PRS_SWLEVEL_CH9LEVEL_MASK              0x200UL                                /**< Bit mask for PRS_CH9LEVEL */
#define _PRS_SWLEVEL_CH9LEVEL_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH9LEVEL_DEFAULT            (_PRS_SWLEVEL_CH9LEVEL_DEFAULT << 9)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH10LEVEL                   (0x1UL << 10)                          /**< Channel 10 Software Level */
#define _PRS_SWLEVEL_CH10LEVEL_SHIFT            10                                     /**< Shift value for PRS_CH10LEVEL */
#define _PRS_SWLEVEL_CH10LEVEL_MASK             0x400UL                                /**< Bit mask for PRS_CH10LEVEL */
#define _PRS_SWLEVEL_CH10LEVEL_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH10LEVEL_DEFAULT           (_PRS_SWLEVEL_CH10LEVEL_DEFAULT << 10) /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH11LEVEL                   (0x1UL << 11)                          /**< Channel 11 Software Level */
#define _PRS_SWLEVEL_CH11LEVEL_SHIFT            11                                     /**< Shift value for PRS_CH11LEVEL */
#define _PRS_SWLEVEL_CH11LEVEL_MASK             0x800UL                                /**< Bit mask for PRS_CH11LEVEL */
#define _PRS_SWLEVEL_CH11LEVEL_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH11LEVEL_DEFAULT           (_PRS_SWLEVEL_CH11LEVEL_DEFAULT << 11) /**< Shifted mode DEFAULT for PRS_SWLEVEL */

/* Bit fields for PRS ROUTE */
#define _PRS_ROUTE_RESETVALUE                   0x00000000UL                       /**< Default value for PRS_ROUTE */
#define _PRS_ROUTE_MASK                         0x0000070FUL                       /**< Mask for PRS_ROUTE */
#define PRS_ROUTE_CH0PEN                        (0x1UL << 0)                       /**< CH0 Pin Enable */
#define _PRS_ROUTE_CH0PEN_SHIFT                 0                                  /**< Shift value for PRS_CH0PEN */
#define _PRS_ROUTE_CH0PEN_MASK                  0x1UL                              /**< Bit mask for PRS_CH0PEN */
#define _PRS_ROUTE_CH0PEN_DEFAULT               0x00000000UL                       /**< Mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH0PEN_DEFAULT                (_PRS_ROUTE_CH0PEN_DEFAULT << 0)   /**< Shifted mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH1PEN                        (0x1UL << 1)                       /**< CH1 Pin Enable */
#define _PRS_ROUTE_CH1PEN_SHIFT                 1                                  /**< Shift value for PRS_CH1PEN */
#define _PRS_ROUTE_CH1PEN_MASK                  0x2UL                              /**< Bit mask for PRS_CH1PEN */
#define _PRS_ROUTE_CH1PEN_DEFAULT               0x00000000UL                       /**< Mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH1PEN_DEFAULT                (_PRS_ROUTE_CH1PEN_DEFAULT << 1)   /**< Shifted mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH2PEN                        (0x1UL << 2)                       /**< CH2 Pin Enable */
#define _PRS_ROUTE_CH2PEN_SHIFT                 2                                  /**< Shift value for PRS_CH2PEN */
#define _PRS_ROUTE_CH2PEN_MASK                  0x4UL                              /**< Bit mask for PRS_CH2PEN */
#define _PRS_ROUTE_CH2PEN_DEFAULT               0x00000000UL                       /**< Mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH2PEN_DEFAULT                (_PRS_ROUTE_CH2PEN_DEFAULT << 2)   /**< Shifted mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH3PEN                        (0x1UL << 3)                       /**< CH3 Pin Enable */
#define _PRS_ROUTE_CH3PEN_SHIFT                 3                                  /**< Shift value for PRS_CH3PEN */
#define _PRS_ROUTE_CH3PEN_MASK                  0x8UL                              /**< Bit mask for PRS_CH3PEN */
#define _PRS_ROUTE_CH3PEN_DEFAULT               0x00000000UL                       /**< Mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH3PEN_DEFAULT                (_PRS_ROUTE_CH3PEN_DEFAULT << 3)   /**< Shifted mode DEFAULT for PRS_ROUTE */
#define _PRS_ROUTE_LOCATION_SHIFT               8                                  /**< Shift value for PRS_LOCATION */
#define _PRS_ROUTE_LOCATION_MASK                0x700UL                            /**< Bit mask for PRS_LOCATION */
#define _PRS_ROUTE_LOCATION_LOC0                0x00000000UL                       /**< Mode LOC0 for PRS_ROUTE */
#define _PRS_ROUTE_LOCATION_DEFAULT             0x00000000UL                       /**< Mode DEFAULT for PRS_ROUTE */
#define _PRS_ROUTE_LOCATION_LOC1                0x00000001UL                       /**< Mode LOC1 for PRS_ROUTE */
#define PRS_ROUTE_LOCATION_LOC0                 (_PRS_ROUTE_LOCATION_LOC0 << 8)    /**< Shifted mode LOC0 for PRS_ROUTE */
#define PRS_ROUTE_LOCATION_DEFAULT              (_PRS_ROUTE_LOCATION_DEFAULT << 8) /**< Shifted mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_LOCATION_LOC1                 (_PRS_ROUTE_LOCATION_LOC1 << 8)    /**< Shifted mode LOC1 for PRS_ROUTE */

/* Bit fields for PRS CH_CTRL */
#define _PRS_CH_CTRL_RESETVALUE                 0x00000000UL                                /**< Default value for PRS_CH_CTRL */
#define _PRS_CH_CTRL_MASK                       0x133F0007UL                                /**< Mask for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_SHIFT               0                                           /**< Shift value for PRS_SIGSEL */
#define _PRS_CH_CTRL_SIGSEL_MASK                0x7UL                                       /**< Bit mask for PRS_SIGSEL */
#define _PRS_CH_CTRL_SIGSEL_VCMPOUT             0x00000000UL                                /**< Mode VCMPOUT for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_ACMP0OUT            0x00000000UL                                /**< Mode ACMP0OUT for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_ACMP1OUT            0x00000000UL                                /**< Mode ACMP1OUT for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_DAC0CH0             0x00000000UL                                /**< Mode DAC0CH0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_ADC0SINGLE          0x00000000UL                                /**< Mode ADC0SINGLE for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART0IRTX          0x00000000UL                                /**< Mode USART0IRTX for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0UF            0x00000000UL                                /**< Mode TIMER0UF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1UF            0x00000000UL                                /**< Mode TIMER1UF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER2UF            0x00000000UL                                /**< Mode TIMER2UF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER3UF            0x00000000UL                                /**< Mode TIMER3UF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USBSOF              0x00000000UL                                /**< Mode USBSOF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_RTCOF               0x00000000UL                                /**< Mode RTCOF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN0            0x00000000UL                                /**< Mode GPIOPIN0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN8            0x00000000UL                                /**< Mode GPIOPIN8 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LETIMER0CH0         0x00000000UL                                /**< Mode LETIMER0CH0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_BURTCOF             0x00000000UL                                /**< Mode BURTCOF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES0     0x00000000UL                                /**< Mode LESENSESCANRES0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES8     0x00000000UL                                /**< Mode LESENSESCANRES8 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSEDEC0         0x00000000UL                                /**< Mode LESENSEDEC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_DAC0CH1             0x00000001UL                                /**< Mode DAC0CH1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_ADC0SCAN            0x00000001UL                                /**< Mode ADC0SCAN for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART0TXC           0x00000001UL                                /**< Mode USART0TXC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART1TXC           0x00000001UL                                /**< Mode USART1TXC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART2TXC           0x00000001UL                                /**< Mode USART2TXC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0OF            0x00000001UL                                /**< Mode TIMER0OF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1OF            0x00000001UL                                /**< Mode TIMER1OF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER2OF            0x00000001UL                                /**< Mode TIMER2OF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER3OF            0x00000001UL                                /**< Mode TIMER3OF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USBSOFSR            0x00000001UL                                /**< Mode USBSOFSR for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_RTCCOMP0            0x00000001UL                                /**< Mode RTCCOMP0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN1            0x00000001UL                                /**< Mode GPIOPIN1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN9            0x00000001UL                                /**< Mode GPIOPIN9 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LETIMER0CH1         0x00000001UL                                /**< Mode LETIMER0CH1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_BURTCCOMP0          0x00000001UL                                /**< Mode BURTCCOMP0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES1     0x00000001UL                                /**< Mode LESENSESCANRES1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES9     0x00000001UL                                /**< Mode LESENSESCANRES9 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSEDEC1         0x00000001UL                                /**< Mode LESENSEDEC1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART0RXDATAV       0x00000002UL                                /**< Mode USART0RXDATAV for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART1RXDATAV       0x00000002UL                                /**< Mode USART1RXDATAV for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART2RXDATAV       0x00000002UL                                /**< Mode USART2RXDATAV for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0CC0           0x00000002UL                                /**< Mode TIMER0CC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1CC0           0x00000002UL                                /**< Mode TIMER1CC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER2CC0           0x00000002UL                                /**< Mode TIMER2CC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER3CC0           0x00000002UL                                /**< Mode TIMER3CC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_RTCCOMP1            0x00000002UL                                /**< Mode RTCCOMP1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN2            0x00000002UL                                /**< Mode GPIOPIN2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN10           0x00000002UL                                /**< Mode GPIOPIN10 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES2     0x00000002UL                                /**< Mode LESENSESCANRES2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES10    0x00000002UL                                /**< Mode LESENSESCANRES10 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSEDEC2         0x00000002UL                                /**< Mode LESENSEDEC2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0CC1           0x00000003UL                                /**< Mode TIMER0CC1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1CC1           0x00000003UL                                /**< Mode TIMER1CC1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER2CC1           0x00000003UL                                /**< Mode TIMER2CC1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER3CC1           0x00000003UL                                /**< Mode TIMER3CC1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN3            0x00000003UL                                /**< Mode GPIOPIN3 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN11           0x00000003UL                                /**< Mode GPIOPIN11 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES3     0x00000003UL                                /**< Mode LESENSESCANRES3 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES11    0x00000003UL                                /**< Mode LESENSESCANRES11 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0CC2           0x00000004UL                                /**< Mode TIMER0CC2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1CC2           0x00000004UL                                /**< Mode TIMER1CC2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER2CC2           0x00000004UL                                /**< Mode TIMER2CC2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER3CC2           0x00000004UL                                /**< Mode TIMER3CC2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN4            0x00000004UL                                /**< Mode GPIOPIN4 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN12           0x00000004UL                                /**< Mode GPIOPIN12 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES4     0x00000004UL                                /**< Mode LESENSESCANRES4 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES12    0x00000004UL                                /**< Mode LESENSESCANRES12 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN5            0x00000005UL                                /**< Mode GPIOPIN5 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN13           0x00000005UL                                /**< Mode GPIOPIN13 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES5     0x00000005UL                                /**< Mode LESENSESCANRES5 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES13    0x00000005UL                                /**< Mode LESENSESCANRES13 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN6            0x00000006UL                                /**< Mode GPIOPIN6 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN14           0x00000006UL                                /**< Mode GPIOPIN14 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES6     0x00000006UL                                /**< Mode LESENSESCANRES6 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES14    0x00000006UL                                /**< Mode LESENSESCANRES14 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN7            0x00000007UL                                /**< Mode GPIOPIN7 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN15           0x00000007UL                                /**< Mode GPIOPIN15 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES7     0x00000007UL                                /**< Mode LESENSESCANRES7 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LESENSESCANRES15    0x00000007UL                                /**< Mode LESENSESCANRES15 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_VCMPOUT              (_PRS_CH_CTRL_SIGSEL_VCMPOUT << 0)          /**< Shifted mode VCMPOUT for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_ACMP0OUT             (_PRS_CH_CTRL_SIGSEL_ACMP0OUT << 0)         /**< Shifted mode ACMP0OUT for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_ACMP1OUT             (_PRS_CH_CTRL_SIGSEL_ACMP1OUT << 0)         /**< Shifted mode ACMP1OUT for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_DAC0CH0              (_PRS_CH_CTRL_SIGSEL_DAC0CH0 << 0)          /**< Shifted mode DAC0CH0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_ADC0SINGLE           (_PRS_CH_CTRL_SIGSEL_ADC0SINGLE << 0)       /**< Shifted mode ADC0SINGLE for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART0IRTX           (_PRS_CH_CTRL_SIGSEL_USART0IRTX << 0)       /**< Shifted mode USART0IRTX for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0UF             (_PRS_CH_CTRL_SIGSEL_TIMER0UF << 0)         /**< Shifted mode TIMER0UF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1UF             (_PRS_CH_CTRL_SIGSEL_TIMER1UF << 0)         /**< Shifted mode TIMER1UF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER2UF             (_PRS_CH_CTRL_SIGSEL_TIMER2UF << 0)         /**< Shifted mode TIMER2UF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER3UF             (_PRS_CH_CTRL_SIGSEL_TIMER3UF << 0)         /**< Shifted mode TIMER3UF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USBSOF               (_PRS_CH_CTRL_SIGSEL_USBSOF << 0)           /**< Shifted mode USBSOF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_RTCOF                (_PRS_CH_CTRL_SIGSEL_RTCOF << 0)            /**< Shifted mode RTCOF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN0             (_PRS_CH_CTRL_SIGSEL_GPIOPIN0 << 0)         /**< Shifted mode GPIOPIN0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN8             (_PRS_CH_CTRL_SIGSEL_GPIOPIN8 << 0)         /**< Shifted mode GPIOPIN8 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LETIMER0CH0          (_PRS_CH_CTRL_SIGSEL_LETIMER0CH0 << 0)      /**< Shifted mode LETIMER0CH0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_BURTCOF              (_PRS_CH_CTRL_SIGSEL_BURTCOF << 0)          /**< Shifted mode BURTCOF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES0      (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES0 << 0)  /**< Shifted mode LESENSESCANRES0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES8      (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES8 << 0)  /**< Shifted mode LESENSESCANRES8 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSEDEC0          (_PRS_CH_CTRL_SIGSEL_LESENSEDEC0 << 0)      /**< Shifted mode LESENSEDEC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_DAC0CH1              (_PRS_CH_CTRL_SIGSEL_DAC0CH1 << 0)          /**< Shifted mode DAC0CH1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_ADC0SCAN             (_PRS_CH_CTRL_SIGSEL_ADC0SCAN << 0)         /**< Shifted mode ADC0SCAN for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART0TXC            (_PRS_CH_CTRL_SIGSEL_USART0TXC << 0)        /**< Shifted mode USART0TXC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART1TXC            (_PRS_CH_CTRL_SIGSEL_USART1TXC << 0)        /**< Shifted mode USART1TXC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART2TXC            (_PRS_CH_CTRL_SIGSEL_USART2TXC << 0)        /**< Shifted mode USART2TXC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0OF             (_PRS_CH_CTRL_SIGSEL_TIMER0OF << 0)         /**< Shifted mode TIMER0OF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1OF             (_PRS_CH_CTRL_SIGSEL_TIMER1OF << 0)         /**< Shifted mode TIMER1OF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER2OF             (_PRS_CH_CTRL_SIGSEL_TIMER2OF << 0)         /**< Shifted mode TIMER2OF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER3OF             (_PRS_CH_CTRL_SIGSEL_TIMER3OF << 0)         /**< Shifted mode TIMER3OF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USBSOFSR             (_PRS_CH_CTRL_SIGSEL_USBSOFSR << 0)         /**< Shifted mode USBSOFSR for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_RTCCOMP0             (_PRS_CH_CTRL_SIGSEL_RTCCOMP0 << 0)         /**< Shifted mode RTCCOMP0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN1             (_PRS_CH_CTRL_SIGSEL_GPIOPIN1 << 0)         /**< Shifted mode GPIOPIN1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN9             (_PRS_CH_CTRL_SIGSEL_GPIOPIN9 << 0)         /**< Shifted mode GPIOPIN9 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LETIMER0CH1          (_PRS_CH_CTRL_SIGSEL_LETIMER0CH1 << 0)      /**< Shifted mode LETIMER0CH1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_BURTCCOMP0           (_PRS_CH_CTRL_SIGSEL_BURTCCOMP0 << 0)       /**< Shifted mode BURTCCOMP0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES1      (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES1 << 0)  /**< Shifted mode LESENSESCANRES1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES9      (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES9 << 0)  /**< Shifted mode LESENSESCANRES9 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSEDEC1          (_PRS_CH_CTRL_SIGSEL_LESENSEDEC1 << 0)      /**< Shifted mode LESENSEDEC1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART0RXDATAV        (_PRS_CH_CTRL_SIGSEL_USART0RXDATAV << 0)    /**< Shifted mode USART0RXDATAV for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART1RXDATAV        (_PRS_CH_CTRL_SIGSEL_USART1RXDATAV << 0)    /**< Shifted mode USART1RXDATAV for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART2RXDATAV        (_PRS_CH_CTRL_SIGSEL_USART2RXDATAV << 0)    /**< Shifted mode USART2RXDATAV for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0CC0            (_PRS_CH_CTRL_SIGSEL_TIMER0CC0 << 0)        /**< Shifted mode TIMER0CC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1CC0            (_PRS_CH_CTRL_SIGSEL_TIMER1CC0 << 0)        /**< Shifted mode TIMER1CC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER2CC0            (_PRS_CH_CTRL_SIGSEL_TIMER2CC0 << 0)        /**< Shifted mode TIMER2CC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER3CC0            (_PRS_CH_CTRL_SIGSEL_TIMER3CC0 << 0)        /**< Shifted mode TIMER3CC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_RTCCOMP1             (_PRS_CH_CTRL_SIGSEL_RTCCOMP1 << 0)         /**< Shifted mode RTCCOMP1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN2             (_PRS_CH_CTRL_SIGSEL_GPIOPIN2 << 0)         /**< Shifted mode GPIOPIN2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN10            (_PRS_CH_CTRL_SIGSEL_GPIOPIN10 << 0)        /**< Shifted mode GPIOPIN10 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES2      (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES2 << 0)  /**< Shifted mode LESENSESCANRES2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES10     (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES10 << 0) /**< Shifted mode LESENSESCANRES10 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSEDEC2          (_PRS_CH_CTRL_SIGSEL_LESENSEDEC2 << 0)      /**< Shifted mode LESENSEDEC2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0CC1            (_PRS_CH_CTRL_SIGSEL_TIMER0CC1 << 0)        /**< Shifted mode TIMER0CC1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1CC1            (_PRS_CH_CTRL_SIGSEL_TIMER1CC1 << 0)        /**< Shifted mode TIMER1CC1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER2CC1            (_PRS_CH_CTRL_SIGSEL_TIMER2CC1 << 0)        /**< Shifted mode TIMER2CC1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER3CC1            (_PRS_CH_CTRL_SIGSEL_TIMER3CC1 << 0)        /**< Shifted mode TIMER3CC1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN3             (_PRS_CH_CTRL_SIGSEL_GPIOPIN3 << 0)         /**< Shifted mode GPIOPIN3 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN11            (_PRS_CH_CTRL_SIGSEL_GPIOPIN11 << 0)        /**< Shifted mode GPIOPIN11 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES3      (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES3 << 0)  /**< Shifted mode LESENSESCANRES3 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES11     (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES11 << 0) /**< Shifted mode LESENSESCANRES11 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0CC2            (_PRS_CH_CTRL_SIGSEL_TIMER0CC2 << 0)        /**< Shifted mode TIMER0CC2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1CC2            (_PRS_CH_CTRL_SIGSEL_TIMER1CC2 << 0)        /**< Shifted mode TIMER1CC2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER2CC2            (_PRS_CH_CTRL_SIGSEL_TIMER2CC2 << 0)        /**< Shifted mode TIMER2CC2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER3CC2            (_PRS_CH_CTRL_SIGSEL_TIMER3CC2 << 0)        /**< Shifted mode TIMER3CC2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN4             (_PRS_CH_CTRL_SIGSEL_GPIOPIN4 << 0)         /**< Shifted mode GPIOPIN4 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN12            (_PRS_CH_CTRL_SIGSEL_GPIOPIN12 << 0)        /**< Shifted mode GPIOPIN12 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES4      (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES4 << 0)  /**< Shifted mode LESENSESCANRES4 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES12     (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES12 << 0) /**< Shifted mode LESENSESCANRES12 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN5             (_PRS_CH_CTRL_SIGSEL_GPIOPIN5 << 0)         /**< Shifted mode GPIOPIN5 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN13            (_PRS_CH_CTRL_SIGSEL_GPIOPIN13 << 0)        /**< Shifted mode GPIOPIN13 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES5      (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES5 << 0)  /**< Shifted mode LESENSESCANRES5 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES13     (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES13 << 0) /**< Shifted mode LESENSESCANRES13 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN6             (_PRS_CH_CTRL_SIGSEL_GPIOPIN6 << 0)         /**< Shifted mode GPIOPIN6 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN14            (_PRS_CH_CTRL_SIGSEL_GPIOPIN14 << 0)        /**< Shifted mode GPIOPIN14 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES6      (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES6 << 0)  /**< Shifted mode LESENSESCANRES6 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES14     (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES14 << 0) /**< Shifted mode LESENSESCANRES14 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN7             (_PRS_CH_CTRL_SIGSEL_GPIOPIN7 << 0)         /**< Shifted mode GPIOPIN7 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN15            (_PRS_CH_CTRL_SIGSEL_GPIOPIN15 << 0)        /**< Shifted mode GPIOPIN15 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES7      (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES7 << 0)  /**< Shifted mode LESENSESCANRES7 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LESENSESCANRES15     (_PRS_CH_CTRL_SIGSEL_LESENSESCANRES15 << 0) /**< Shifted mode LESENSESCANRES15 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_SHIFT            16                                          /**< Shift value for PRS_SOURCESEL */
#define _PRS_CH_CTRL_SOURCESEL_MASK             0x3F0000UL                                  /**< Bit mask for PRS_SOURCESEL */
#define _PRS_CH_CTRL_SOURCESEL_NONE             0x00000000UL                                /**< Mode NONE for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_VCMP             0x00000001UL                                /**< Mode VCMP for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_ACMP0            0x00000002UL                                /**< Mode ACMP0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_ACMP1            0x00000003UL                                /**< Mode ACMP1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_DAC0             0x00000006UL                                /**< Mode DAC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_ADC0             0x00000008UL                                /**< Mode ADC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_USART0           0x00000010UL                                /**< Mode USART0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_USART1           0x00000011UL                                /**< Mode USART1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_USART2           0x00000012UL                                /**< Mode USART2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_TIMER0           0x0000001CUL                                /**< Mode TIMER0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_TIMER1           0x0000001DUL                                /**< Mode TIMER1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_TIMER2           0x0000001EUL                                /**< Mode TIMER2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_TIMER3           0x0000001FUL                                /**< Mode TIMER3 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_USB              0x00000024UL                                /**< Mode USB for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_RTC              0x00000028UL                                /**< Mode RTC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_GPIOL            0x00000030UL                                /**< Mode GPIOL for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_GPIOH            0x00000031UL                                /**< Mode GPIOH for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_LETIMER0         0x00000034UL                                /**< Mode LETIMER0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_BURTC            0x00000037UL                                /**< Mode BURTC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_LESENSEL         0x00000039UL                                /**< Mode LESENSEL for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_LESENSEH         0x0000003AUL                                /**< Mode LESENSEH for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_LESENSED         0x0000003BUL                                /**< Mode LESENSED for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_NONE              (_PRS_CH_CTRL_SOURCESEL_NONE << 16)         /**< Shifted mode NONE for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_VCMP              (_PRS_CH_CTRL_SOURCESEL_VCMP << 16)         /**< Shifted mode VCMP for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_ACMP0             (_PRS_CH_CTRL_SOURCESEL_ACMP0 << 16)        /**< Shifted mode ACMP0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_ACMP1             (_PRS_CH_CTRL_SOURCESEL_ACMP1 << 16)        /**< Shifted mode ACMP1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_DAC0              (_PRS_CH_CTRL_SOURCESEL_DAC0 << 16)         /**< Shifted mode DAC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_ADC0              (_PRS_CH_CTRL_SOURCESEL_ADC0 << 16)         /**< Shifted mode ADC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_USART0            (_PRS_CH_CTRL_SOURCESEL_USART0 << 16)       /**< Shifted mode USART0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_USART1            (_PRS_CH_CTRL_SOURCESEL_USART1 << 16)       /**< Shifted mode USART1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_USART2            (_PRS_CH_CTRL_SOURCESEL_USART2 << 16)       /**< Shifted mode USART2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_TIMER0            (_PRS_CH_CTRL_SOURCESEL_TIMER0 << 16)       /**< Shifted mode TIMER0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_TIMER1            (_PRS_CH_CTRL_SOURCESEL_TIMER1 << 16)       /**< Shifted mode TIMER1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_TIMER2            (_PRS_CH_CTRL_SOURCESEL_TIMER2 << 16)       /**< Shifted mode TIMER2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_TIMER3            (_PRS_CH_CTRL_SOURCESEL_TIMER3 << 16)       /**< Shifted mode TIMER3 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_USB               (_PRS_CH_CTRL_SOURCESEL_USB << 16)          /**< Shifted mode USB for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_RTC               (_PRS_CH_CTRL_SOURCESEL_RTC << 16)          /**< Shifted mode RTC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_GPIOL             (_PRS_CH_CTRL_SOURCESEL_GPIOL << 16)        /**< Shifted mode GPIOL for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_GPIOH             (_PRS_CH_CTRL_SOURCESEL_GPIOH << 16)        /**< Shifted mode GPIOH for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_LETIMER0          (_PRS_CH_CTRL_SOURCESEL_LETIMER0 << 16)     /**< Shifted mode LETIMER0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_BURTC             (_PRS_CH_CTRL_SOURCESEL_BURTC << 16)        /**< Shifted mode BURTC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_LESENSEL          (_PRS_CH_CTRL_SOURCESEL_LESENSEL << 16)     /**< Shifted mode LESENSEL for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_LESENSEH          (_PRS_CH_CTRL_SOURCESEL_LESENSEH << 16)     /**< Shifted mode LESENSEH for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_LESENSED          (_PRS_CH_CTRL_SOURCESEL_LESENSED << 16)     /**< Shifted mode LESENSED for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_SHIFT                24                                          /**< Shift value for PRS_EDSEL */
#define _PRS_CH_CTRL_EDSEL_MASK                 0x3000000UL                                 /**< Bit mask for PRS_EDSEL */
#define _PRS_CH_CTRL_EDSEL_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_OFF                  0x00000000UL                                /**< Mode OFF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_POSEDGE              0x00000001UL                                /**< Mode POSEDGE for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_NEGEDGE              0x00000002UL                                /**< Mode NEGEDGE for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_BOTHEDGES            0x00000003UL                                /**< Mode BOTHEDGES for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_DEFAULT               (_PRS_CH_CTRL_EDSEL_DEFAULT << 24)          /**< Shifted mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_OFF                   (_PRS_CH_CTRL_EDSEL_OFF << 24)              /**< Shifted mode OFF for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_POSEDGE               (_PRS_CH_CTRL_EDSEL_POSEDGE << 24)          /**< Shifted mode POSEDGE for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_NEGEDGE               (_PRS_CH_CTRL_EDSEL_NEGEDGE << 24)          /**< Shifted mode NEGEDGE for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_BOTHEDGES             (_PRS_CH_CTRL_EDSEL_BOTHEDGES << 24)        /**< Shifted mode BOTHEDGES for PRS_CH_CTRL */
#define PRS_CH_CTRL_ASYNC                       (0x1UL << 28)                               /**< Asynchronous reflex */
#define _PRS_CH_CTRL_ASYNC_SHIFT                28                                          /**< Shift value for PRS_ASYNC */
#define _PRS_CH_CTRL_ASYNC_MASK                 0x10000000UL                                /**< Bit mask for PRS_ASYNC */
#define _PRS_CH_CTRL_ASYNC_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_ASYNC_DEFAULT               (_PRS_CH_CTRL_ASYNC_DEFAULT << 28)          /**< Shifted mode DEFAULT for PRS_CH_CTRL */

/** @} End of group EFM32WG942F64_PRS */



/**************************************************************************//**
 * @defgroup EFM32WG942F64_UNLOCK EFM32WG942F64 Unlock Codes
 * @{
 *****************************************************************************/
#define MSC_UNLOCK_CODE      0x1B71 /**< MSC unlock code */
#define EMU_UNLOCK_CODE      0xADE8 /**< EMU unlock code */
#define CMU_UNLOCK_CODE      0x580E /**< CMU unlock code */
#define TIMER_UNLOCK_CODE    0xCE80 /**< TIMER unlock code */
#define GPIO_UNLOCK_CODE     0xA534 /**< GPIO unlock code */
#define BURTC_UNLOCK_CODE    0xAEE8 /**< BURTC unlock code */

/** @} End of group EFM32WG942F64_UNLOCK */

/** @} End of group EFM32WG942F64_BitFields */

/**************************************************************************//**
 * @defgroup EFM32WG942F64_Alternate_Function EFM32WG942F64 Alternate Function
 * @{
 *****************************************************************************/

#include "efm32wg_af_ports.h"
#include "efm32wg_af_pins.h"

/** @} End of group EFM32WG942F64_Alternate_Function */

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

/** @} End of group EFM32WG942F64 */

/** @} End of group Parts */

#ifdef __cplusplus
}
#endif
#endif /* EFM32WG942F64_H */
