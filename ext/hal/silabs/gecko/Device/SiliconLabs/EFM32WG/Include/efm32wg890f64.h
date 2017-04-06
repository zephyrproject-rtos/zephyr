/**************************************************************************//**
 * @file efm32wg890f64.h
 * @brief CMSIS Cortex-M Peripheral Access Layer Header File
 *        for EFM32WG890F64
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

#ifndef EFM32WG890F64_H
#define EFM32WG890F64_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************//**
 * @addtogroup Parts
 * @{
 *****************************************************************************/

/**************************************************************************//**
 * @defgroup EFM32WG890F64 EFM32WG890F64
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
  UART0_RX_IRQn         = 20, /*!< 20 EFM32 UART0_RX Interrupt */
  UART0_TX_IRQn         = 21, /*!< 21 EFM32 UART0_TX Interrupt */
  UART1_RX_IRQn         = 22, /*!< 22 EFM32 UART1_RX Interrupt */
  UART1_TX_IRQn         = 23, /*!< 23 EFM32 UART1_TX Interrupt */
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
  EBI_IRQn              = 37, /*!< 37 EFM32 EBI Interrupt */
  EMU_IRQn              = 38, /*!< 38 EFM32 EMU Interrupt */
  FPUEH_IRQn            = 39, /*!< 39 EFM32 FPUEH Interrupt */
} IRQn_Type;

/**************************************************************************//**
 * @defgroup EFM32WG890F64_Core EFM32WG890F64 Core
 * @{
 * @brief Processor and Core Peripheral Section
 *****************************************************************************/
#define __MPU_PRESENT             1 /**< Presence of MPU  */
#define __FPU_PRESENT             1 /**< Presence of FPU  */
#define __VTOR_PRESENT            1 /**< Presence of VTOR register in SCB */
#define __NVIC_PRIO_BITS          3 /**< NVIC interrupt priority bits */
#define __Vendor_SysTickConfig    0 /**< Is 1 if different SysTick counter is used */

/** @} End of group EFM32WG890F64_Core */

/**************************************************************************//**
* @defgroup EFM32WG890F64_Part EFM32WG890F64 Part
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
#if !defined(EFM32WG890F64)
#define EFM32WG890F64    1 /**< Wonder Gecko Part  */
#endif

/** Configure part number */
#define PART_NUMBER          "EFM32WG890F64" /**< Part Number */

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

/** Flash and SRAM limits for EFM32WG890F64 */
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
#define UART_PRESENT          /**< UART is available in this part */
#define UART_COUNT          2 /**< 2 UARTs available  */
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
#define EBI_PRESENT
#define EBI_COUNT           1
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

/** @} End of group EFM32WG890F64_Part */

/**************************************************************************//**
 * @defgroup EFM32WG890F64_Peripheral_TypeDefs EFM32WG890F64 Peripheral TypeDefs
 * @{
 * @brief Device Specific Peripheral Register Structures
 *****************************************************************************/

#include "efm32wg_dma_ch.h"
#include "efm32wg_dma.h"
#include "efm32wg_aes.h"
#include "efm32wg_msc.h"
#include "efm32wg_emu.h"
#include "efm32wg_rmu.h"

/**************************************************************************//**
 * @defgroup EFM32WG890F64_CMU EFM32WG890F64 CMU
 * @{
 * @brief EFM32WG890F64_CMU Register Declaration
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
#include "efm32wg_ebi.h"
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
 * @defgroup EFM32WG890F64_PRS EFM32WG890F64 PRS
 * @{
 * @brief EFM32WG890F64_PRS Register Declaration
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

/** @} End of group EFM32WG890F64_Peripheral_TypeDefs */

/**************************************************************************//**
 * @defgroup EFM32WG890F64_Peripheral_Base EFM32WG890F64 Peripheral Memory Map
 * @{
 *****************************************************************************/

#define DMA_BASE          (0x400C2000UL) /**< DMA base address  */
#define AES_BASE          (0x400E0000UL) /**< AES base address  */
#define MSC_BASE          (0x400C0000UL) /**< MSC base address  */
#define EMU_BASE          (0x400C6000UL) /**< EMU base address  */
#define RMU_BASE          (0x400CA000UL) /**< RMU base address  */
#define CMU_BASE          (0x400C8000UL) /**< CMU base address  */
#define LESENSE_BASE      (0x4008C000UL) /**< LESENSE base address  */
#define EBI_BASE          (0x40008000UL) /**< EBI base address  */
#define FPUEH_BASE        (0x400C1C00UL) /**< FPUEH base address  */
#define USART0_BASE       (0x4000C000UL) /**< USART0 base address  */
#define USART1_BASE       (0x4000C400UL) /**< USART1 base address  */
#define USART2_BASE       (0x4000C800UL) /**< USART2 base address  */
#define UART0_BASE        (0x4000E000UL) /**< UART0 base address  */
#define UART1_BASE        (0x4000E400UL) /**< UART1 base address  */
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

/** @} End of group EFM32WG890F64_Peripheral_Base */

/**************************************************************************//**
 * @defgroup EFM32WG890F64_Peripheral_Declaration  EFM32WG890F64 Peripheral Declarations
 * @{
 *****************************************************************************/

#define DMA          ((DMA_TypeDef *) DMA_BASE)             /**< DMA base pointer */
#define AES          ((AES_TypeDef *) AES_BASE)             /**< AES base pointer */
#define MSC          ((MSC_TypeDef *) MSC_BASE)             /**< MSC base pointer */
#define EMU          ((EMU_TypeDef *) EMU_BASE)             /**< EMU base pointer */
#define RMU          ((RMU_TypeDef *) RMU_BASE)             /**< RMU base pointer */
#define CMU          ((CMU_TypeDef *) CMU_BASE)             /**< CMU base pointer */
#define LESENSE      ((LESENSE_TypeDef *) LESENSE_BASE)     /**< LESENSE base pointer */
#define EBI          ((EBI_TypeDef *) EBI_BASE)             /**< EBI base pointer */
#define FPUEH        ((FPUEH_TypeDef *) FPUEH_BASE)         /**< FPUEH base pointer */
#define USART0       ((USART_TypeDef *) USART0_BASE)        /**< USART0 base pointer */
#define USART1       ((USART_TypeDef *) USART1_BASE)        /**< USART1 base pointer */
#define USART2       ((USART_TypeDef *) USART2_BASE)        /**< USART2 base pointer */
#define UART0        ((USART_TypeDef *) UART0_BASE)         /**< UART0 base pointer */
#define UART1        ((USART_TypeDef *) UART1_BASE)         /**< UART1 base pointer */
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

/** @} End of group EFM32WG890F64_Peripheral_Declaration */

/**************************************************************************//**
 * @defgroup EFM32WG890F64_BitFields EFM32WG890F64 Bit Fields
 * @{
 *****************************************************************************/

/**************************************************************************//**
 * @addtogroup EFM32WG890F64_PRS_Signals
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
#define PRS_RTC_OF               ((40 << 16) + 0) /**< PRS RTC Overflow */
#define PRS_RTC_COMP0            ((40 << 16) + 1) /**< PRS RTC Compare 0 */
#define PRS_RTC_COMP1            ((40 << 16) + 2) /**< PRS RTC Compare 1 */
#define PRS_UART0_TXC            ((41 << 16) + 1) /**< PRS USART 0 TX complete */
#define PRS_UART0_RXDATAV        ((41 << 16) + 2) /**< PRS USART 0 RX Data Valid */
#define PRS_UART1_TXC            ((42 << 16) + 1) /**< PRS USART 0 TX complete */
#define PRS_UART1_RXDATAV        ((42 << 16) + 2) /**< PRS USART 0 RX Data Valid */
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

/** @} End of group EFM32WG890F64_PRS */

#include "efm32wg_dmareq.h"
#include "efm32wg_dmactrl.h"
#include "efm32wg_uart.h"

/**************************************************************************//**
 * @defgroup EFM32WG890F64_CMU_BitFields  EFM32WG890F64_CMU Bit Fields
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
#define _CMU_CMD_RESETVALUE                         0x00000000UL                     /**< Default value for CMU_CMD */
#define _CMU_CMD_MASK                               0x0000001FUL                     /**< Mask for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_SHIFT                     0                                /**< Shift value for CMU_HFCLKSEL */
#define _CMU_CMD_HFCLKSEL_MASK                      0x7UL                            /**< Bit mask for CMU_HFCLKSEL */
#define _CMU_CMD_HFCLKSEL_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_HFRCO                     0x00000001UL                     /**< Mode HFRCO for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_HFXO                      0x00000002UL                     /**< Mode HFXO for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_LFRCO                     0x00000003UL                     /**< Mode LFRCO for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_LFXO                      0x00000004UL                     /**< Mode LFXO for CMU_CMD */
#define CMU_CMD_HFCLKSEL_DEFAULT                    (_CMU_CMD_HFCLKSEL_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_CMD */
#define CMU_CMD_HFCLKSEL_HFRCO                      (_CMU_CMD_HFCLKSEL_HFRCO << 0)   /**< Shifted mode HFRCO for CMU_CMD */
#define CMU_CMD_HFCLKSEL_HFXO                       (_CMU_CMD_HFCLKSEL_HFXO << 0)    /**< Shifted mode HFXO for CMU_CMD */
#define CMU_CMD_HFCLKSEL_LFRCO                      (_CMU_CMD_HFCLKSEL_LFRCO << 0)   /**< Shifted mode LFRCO for CMU_CMD */
#define CMU_CMD_HFCLKSEL_LFXO                       (_CMU_CMD_HFCLKSEL_LFXO << 0)    /**< Shifted mode LFXO for CMU_CMD */
#define CMU_CMD_CALSTART                            (0x1UL << 3)                     /**< Calibration Start */
#define _CMU_CMD_CALSTART_SHIFT                     3                                /**< Shift value for CMU_CALSTART */
#define _CMU_CMD_CALSTART_MASK                      0x8UL                            /**< Bit mask for CMU_CALSTART */
#define _CMU_CMD_CALSTART_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for CMU_CMD */
#define CMU_CMD_CALSTART_DEFAULT                    (_CMU_CMD_CALSTART_DEFAULT << 3) /**< Shifted mode DEFAULT for CMU_CMD */
#define CMU_CMD_CALSTOP                             (0x1UL << 4)                     /**< Calibration Stop */
#define _CMU_CMD_CALSTOP_SHIFT                      4                                /**< Shift value for CMU_CALSTOP */
#define _CMU_CMD_CALSTOP_MASK                       0x10UL                           /**< Bit mask for CMU_CALSTOP */
#define _CMU_CMD_CALSTOP_DEFAULT                    0x00000000UL                     /**< Mode DEFAULT for CMU_CMD */
#define CMU_CMD_CALSTOP_DEFAULT                     (_CMU_CMD_CALSTOP_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_CMD */

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
#define _CMU_STATUS_RESETVALUE                      0x00000403UL                           /**< Default value for CMU_STATUS */
#define _CMU_STATUS_MASK                            0x00007FFFUL                           /**< Mask for CMU_STATUS */
#define CMU_STATUS_HFRCOENS                         (0x1UL << 0)                           /**< HFRCO Enable Status */
#define _CMU_STATUS_HFRCOENS_SHIFT                  0                                      /**< Shift value for CMU_HFRCOENS */
#define _CMU_STATUS_HFRCOENS_MASK                   0x1UL                                  /**< Bit mask for CMU_HFRCOENS */
#define _CMU_STATUS_HFRCOENS_DEFAULT                0x00000001UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCOENS_DEFAULT                 (_CMU_STATUS_HFRCOENS_DEFAULT << 0)    /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCORDY                         (0x1UL << 1)                           /**< HFRCO Ready */
#define _CMU_STATUS_HFRCORDY_SHIFT                  1                                      /**< Shift value for CMU_HFRCORDY */
#define _CMU_STATUS_HFRCORDY_MASK                   0x2UL                                  /**< Bit mask for CMU_HFRCORDY */
#define _CMU_STATUS_HFRCORDY_DEFAULT                0x00000001UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCORDY_DEFAULT                 (_CMU_STATUS_HFRCORDY_DEFAULT << 1)    /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOENS                          (0x1UL << 2)                           /**< HFXO Enable Status */
#define _CMU_STATUS_HFXOENS_SHIFT                   2                                      /**< Shift value for CMU_HFXOENS */
#define _CMU_STATUS_HFXOENS_MASK                    0x4UL                                  /**< Bit mask for CMU_HFXOENS */
#define _CMU_STATUS_HFXOENS_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOENS_DEFAULT                  (_CMU_STATUS_HFXOENS_DEFAULT << 2)     /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXORDY                          (0x1UL << 3)                           /**< HFXO Ready */
#define _CMU_STATUS_HFXORDY_SHIFT                   3                                      /**< Shift value for CMU_HFXORDY */
#define _CMU_STATUS_HFXORDY_MASK                    0x8UL                                  /**< Bit mask for CMU_HFXORDY */
#define _CMU_STATUS_HFXORDY_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXORDY_DEFAULT                  (_CMU_STATUS_HFXORDY_DEFAULT << 3)     /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCOENS                      (0x1UL << 4)                           /**< AUXHFRCO Enable Status */
#define _CMU_STATUS_AUXHFRCOENS_SHIFT               4                                      /**< Shift value for CMU_AUXHFRCOENS */
#define _CMU_STATUS_AUXHFRCOENS_MASK                0x10UL                                 /**< Bit mask for CMU_AUXHFRCOENS */
#define _CMU_STATUS_AUXHFRCOENS_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCOENS_DEFAULT              (_CMU_STATUS_AUXHFRCOENS_DEFAULT << 4) /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCORDY                      (0x1UL << 5)                           /**< AUXHFRCO Ready */
#define _CMU_STATUS_AUXHFRCORDY_SHIFT               5                                      /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_STATUS_AUXHFRCORDY_MASK                0x20UL                                 /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_STATUS_AUXHFRCORDY_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCORDY_DEFAULT              (_CMU_STATUS_AUXHFRCORDY_DEFAULT << 5) /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOENS                         (0x1UL << 6)                           /**< LFRCO Enable Status */
#define _CMU_STATUS_LFRCOENS_SHIFT                  6                                      /**< Shift value for CMU_LFRCOENS */
#define _CMU_STATUS_LFRCOENS_MASK                   0x40UL                                 /**< Bit mask for CMU_LFRCOENS */
#define _CMU_STATUS_LFRCOENS_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOENS_DEFAULT                 (_CMU_STATUS_LFRCOENS_DEFAULT << 6)    /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCORDY                         (0x1UL << 7)                           /**< LFRCO Ready */
#define _CMU_STATUS_LFRCORDY_SHIFT                  7                                      /**< Shift value for CMU_LFRCORDY */
#define _CMU_STATUS_LFRCORDY_MASK                   0x80UL                                 /**< Bit mask for CMU_LFRCORDY */
#define _CMU_STATUS_LFRCORDY_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCORDY_DEFAULT                 (_CMU_STATUS_LFRCORDY_DEFAULT << 7)    /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOENS                          (0x1UL << 8)                           /**< LFXO Enable Status */
#define _CMU_STATUS_LFXOENS_SHIFT                   8                                      /**< Shift value for CMU_LFXOENS */
#define _CMU_STATUS_LFXOENS_MASK                    0x100UL                                /**< Bit mask for CMU_LFXOENS */
#define _CMU_STATUS_LFXOENS_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOENS_DEFAULT                  (_CMU_STATUS_LFXOENS_DEFAULT << 8)     /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXORDY                          (0x1UL << 9)                           /**< LFXO Ready */
#define _CMU_STATUS_LFXORDY_SHIFT                   9                                      /**< Shift value for CMU_LFXORDY */
#define _CMU_STATUS_LFXORDY_MASK                    0x200UL                                /**< Bit mask for CMU_LFXORDY */
#define _CMU_STATUS_LFXORDY_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXORDY_DEFAULT                  (_CMU_STATUS_LFXORDY_DEFAULT << 9)     /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCOSEL                         (0x1UL << 10)                          /**< HFRCO Selected */
#define _CMU_STATUS_HFRCOSEL_SHIFT                  10                                     /**< Shift value for CMU_HFRCOSEL */
#define _CMU_STATUS_HFRCOSEL_MASK                   0x400UL                                /**< Bit mask for CMU_HFRCOSEL */
#define _CMU_STATUS_HFRCOSEL_DEFAULT                0x00000001UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCOSEL_DEFAULT                 (_CMU_STATUS_HFRCOSEL_DEFAULT << 10)   /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOSEL                          (0x1UL << 11)                          /**< HFXO Selected */
#define _CMU_STATUS_HFXOSEL_SHIFT                   11                                     /**< Shift value for CMU_HFXOSEL */
#define _CMU_STATUS_HFXOSEL_MASK                    0x800UL                                /**< Bit mask for CMU_HFXOSEL */
#define _CMU_STATUS_HFXOSEL_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOSEL_DEFAULT                  (_CMU_STATUS_HFXOSEL_DEFAULT << 11)    /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOSEL                         (0x1UL << 12)                          /**< LFRCO Selected */
#define _CMU_STATUS_LFRCOSEL_SHIFT                  12                                     /**< Shift value for CMU_LFRCOSEL */
#define _CMU_STATUS_LFRCOSEL_MASK                   0x1000UL                               /**< Bit mask for CMU_LFRCOSEL */
#define _CMU_STATUS_LFRCOSEL_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOSEL_DEFAULT                 (_CMU_STATUS_LFRCOSEL_DEFAULT << 12)   /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOSEL                          (0x1UL << 13)                          /**< LFXO Selected */
#define _CMU_STATUS_LFXOSEL_SHIFT                   13                                     /**< Shift value for CMU_LFXOSEL */
#define _CMU_STATUS_LFXOSEL_MASK                    0x2000UL                               /**< Bit mask for CMU_LFXOSEL */
#define _CMU_STATUS_LFXOSEL_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOSEL_DEFAULT                  (_CMU_STATUS_LFXOSEL_DEFAULT << 13)    /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_CALBSY                           (0x1UL << 14)                          /**< Calibration Busy */
#define _CMU_STATUS_CALBSY_SHIFT                    14                                     /**< Shift value for CMU_CALBSY */
#define _CMU_STATUS_CALBSY_MASK                     0x4000UL                               /**< Bit mask for CMU_CALBSY */
#define _CMU_STATUS_CALBSY_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_CALBSY_DEFAULT                   (_CMU_STATUS_CALBSY_DEFAULT << 14)     /**< Shifted mode DEFAULT for CMU_STATUS */

/* Bit fields for CMU IF */
#define _CMU_IF_RESETVALUE                          0x00000001UL                       /**< Default value for CMU_IF */
#define _CMU_IF_MASK                                0x0000007FUL                       /**< Mask for CMU_IF */
#define CMU_IF_HFRCORDY                             (0x1UL << 0)                       /**< HFRCO Ready Interrupt Flag */
#define _CMU_IF_HFRCORDY_SHIFT                      0                                  /**< Shift value for CMU_HFRCORDY */
#define _CMU_IF_HFRCORDY_MASK                       0x1UL                              /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IF_HFRCORDY_DEFAULT                    0x00000001UL                       /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFRCORDY_DEFAULT                     (_CMU_IF_HFRCORDY_DEFAULT << 0)    /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_HFXORDY                              (0x1UL << 1)                       /**< HFXO Ready Interrupt Flag */
#define _CMU_IF_HFXORDY_SHIFT                       1                                  /**< Shift value for CMU_HFXORDY */
#define _CMU_IF_HFXORDY_MASK                        0x2UL                              /**< Bit mask for CMU_HFXORDY */
#define _CMU_IF_HFXORDY_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFXORDY_DEFAULT                      (_CMU_IF_HFXORDY_DEFAULT << 1)     /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_LFRCORDY                             (0x1UL << 2)                       /**< LFRCO Ready Interrupt Flag */
#define _CMU_IF_LFRCORDY_SHIFT                      2                                  /**< Shift value for CMU_LFRCORDY */
#define _CMU_IF_LFRCORDY_MASK                       0x4UL                              /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IF_LFRCORDY_DEFAULT                    0x00000000UL                       /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_LFRCORDY_DEFAULT                     (_CMU_IF_LFRCORDY_DEFAULT << 2)    /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_LFXORDY                              (0x1UL << 3)                       /**< LFXO Ready Interrupt Flag */
#define _CMU_IF_LFXORDY_SHIFT                       3                                  /**< Shift value for CMU_LFXORDY */
#define _CMU_IF_LFXORDY_MASK                        0x8UL                              /**< Bit mask for CMU_LFXORDY */
#define _CMU_IF_LFXORDY_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_LFXORDY_DEFAULT                      (_CMU_IF_LFXORDY_DEFAULT << 3)     /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_AUXHFRCORDY                          (0x1UL << 4)                       /**< AUXHFRCO Ready Interrupt Flag */
#define _CMU_IF_AUXHFRCORDY_SHIFT                   4                                  /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IF_AUXHFRCORDY_MASK                    0x10UL                             /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IF_AUXHFRCORDY_DEFAULT                 0x00000000UL                       /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_AUXHFRCORDY_DEFAULT                  (_CMU_IF_AUXHFRCORDY_DEFAULT << 4) /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_CALRDY                               (0x1UL << 5)                       /**< Calibration Ready Interrupt Flag */
#define _CMU_IF_CALRDY_SHIFT                        5                                  /**< Shift value for CMU_CALRDY */
#define _CMU_IF_CALRDY_MASK                         0x20UL                             /**< Bit mask for CMU_CALRDY */
#define _CMU_IF_CALRDY_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_CALRDY_DEFAULT                       (_CMU_IF_CALRDY_DEFAULT << 5)      /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_CALOF                                (0x1UL << 6)                       /**< Calibration Overflow Interrupt Flag */
#define _CMU_IF_CALOF_SHIFT                         6                                  /**< Shift value for CMU_CALOF */
#define _CMU_IF_CALOF_MASK                          0x40UL                             /**< Bit mask for CMU_CALOF */
#define _CMU_IF_CALOF_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_CALOF_DEFAULT                        (_CMU_IF_CALOF_DEFAULT << 6)       /**< Shifted mode DEFAULT for CMU_IF */

/* Bit fields for CMU IFS */
#define _CMU_IFS_RESETVALUE                         0x00000000UL                        /**< Default value for CMU_IFS */
#define _CMU_IFS_MASK                               0x0000007FUL                        /**< Mask for CMU_IFS */
#define CMU_IFS_HFRCORDY                            (0x1UL << 0)                        /**< HFRCO Ready Interrupt Flag Set */
#define _CMU_IFS_HFRCORDY_SHIFT                     0                                   /**< Shift value for CMU_HFRCORDY */
#define _CMU_IFS_HFRCORDY_MASK                      0x1UL                               /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IFS_HFRCORDY_DEFAULT                   0x00000000UL                        /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFRCORDY_DEFAULT                    (_CMU_IFS_HFRCORDY_DEFAULT << 0)    /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXORDY                             (0x1UL << 1)                        /**< HFXO Ready Interrupt Flag Set */
#define _CMU_IFS_HFXORDY_SHIFT                      1                                   /**< Shift value for CMU_HFXORDY */
#define _CMU_IFS_HFXORDY_MASK                       0x2UL                               /**< Bit mask for CMU_HFXORDY */
#define _CMU_IFS_HFXORDY_DEFAULT                    0x00000000UL                        /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXORDY_DEFAULT                     (_CMU_IFS_HFXORDY_DEFAULT << 1)     /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFRCORDY                            (0x1UL << 2)                        /**< LFRCO Ready Interrupt Flag Set */
#define _CMU_IFS_LFRCORDY_SHIFT                     2                                   /**< Shift value for CMU_LFRCORDY */
#define _CMU_IFS_LFRCORDY_MASK                      0x4UL                               /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IFS_LFRCORDY_DEFAULT                   0x00000000UL                        /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFRCORDY_DEFAULT                    (_CMU_IFS_LFRCORDY_DEFAULT << 2)    /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFXORDY                             (0x1UL << 3)                        /**< LFXO Ready Interrupt Flag Set */
#define _CMU_IFS_LFXORDY_SHIFT                      3                                   /**< Shift value for CMU_LFXORDY */
#define _CMU_IFS_LFXORDY_MASK                       0x8UL                               /**< Bit mask for CMU_LFXORDY */
#define _CMU_IFS_LFXORDY_DEFAULT                    0x00000000UL                        /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFXORDY_DEFAULT                     (_CMU_IFS_LFXORDY_DEFAULT << 3)     /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_AUXHFRCORDY                         (0x1UL << 4)                        /**< AUXHFRCO Ready Interrupt Flag Set */
#define _CMU_IFS_AUXHFRCORDY_SHIFT                  4                                   /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IFS_AUXHFRCORDY_MASK                   0x10UL                              /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IFS_AUXHFRCORDY_DEFAULT                0x00000000UL                        /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_AUXHFRCORDY_DEFAULT                 (_CMU_IFS_AUXHFRCORDY_DEFAULT << 4) /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALRDY                              (0x1UL << 5)                        /**< Calibration Ready Interrupt Flag Set */
#define _CMU_IFS_CALRDY_SHIFT                       5                                   /**< Shift value for CMU_CALRDY */
#define _CMU_IFS_CALRDY_MASK                        0x20UL                              /**< Bit mask for CMU_CALRDY */
#define _CMU_IFS_CALRDY_DEFAULT                     0x00000000UL                        /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALRDY_DEFAULT                      (_CMU_IFS_CALRDY_DEFAULT << 5)      /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALOF                               (0x1UL << 6)                        /**< Calibration Overflow Interrupt Flag Set */
#define _CMU_IFS_CALOF_SHIFT                        6                                   /**< Shift value for CMU_CALOF */
#define _CMU_IFS_CALOF_MASK                         0x40UL                              /**< Bit mask for CMU_CALOF */
#define _CMU_IFS_CALOF_DEFAULT                      0x00000000UL                        /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALOF_DEFAULT                       (_CMU_IFS_CALOF_DEFAULT << 6)       /**< Shifted mode DEFAULT for CMU_IFS */

/* Bit fields for CMU IFC */
#define _CMU_IFC_RESETVALUE                         0x00000000UL                        /**< Default value for CMU_IFC */
#define _CMU_IFC_MASK                               0x0000007FUL                        /**< Mask for CMU_IFC */
#define CMU_IFC_HFRCORDY                            (0x1UL << 0)                        /**< HFRCO Ready Interrupt Flag Clear */
#define _CMU_IFC_HFRCORDY_SHIFT                     0                                   /**< Shift value for CMU_HFRCORDY */
#define _CMU_IFC_HFRCORDY_MASK                      0x1UL                               /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IFC_HFRCORDY_DEFAULT                   0x00000000UL                        /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFRCORDY_DEFAULT                    (_CMU_IFC_HFRCORDY_DEFAULT << 0)    /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXORDY                             (0x1UL << 1)                        /**< HFXO Ready Interrupt Flag Clear */
#define _CMU_IFC_HFXORDY_SHIFT                      1                                   /**< Shift value for CMU_HFXORDY */
#define _CMU_IFC_HFXORDY_MASK                       0x2UL                               /**< Bit mask for CMU_HFXORDY */
#define _CMU_IFC_HFXORDY_DEFAULT                    0x00000000UL                        /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXORDY_DEFAULT                     (_CMU_IFC_HFXORDY_DEFAULT << 1)     /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFRCORDY                            (0x1UL << 2)                        /**< LFRCO Ready Interrupt Flag Clear */
#define _CMU_IFC_LFRCORDY_SHIFT                     2                                   /**< Shift value for CMU_LFRCORDY */
#define _CMU_IFC_LFRCORDY_MASK                      0x4UL                               /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IFC_LFRCORDY_DEFAULT                   0x00000000UL                        /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFRCORDY_DEFAULT                    (_CMU_IFC_LFRCORDY_DEFAULT << 2)    /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFXORDY                             (0x1UL << 3)                        /**< LFXO Ready Interrupt Flag Clear */
#define _CMU_IFC_LFXORDY_SHIFT                      3                                   /**< Shift value for CMU_LFXORDY */
#define _CMU_IFC_LFXORDY_MASK                       0x8UL                               /**< Bit mask for CMU_LFXORDY */
#define _CMU_IFC_LFXORDY_DEFAULT                    0x00000000UL                        /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFXORDY_DEFAULT                     (_CMU_IFC_LFXORDY_DEFAULT << 3)     /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_AUXHFRCORDY                         (0x1UL << 4)                        /**< AUXHFRCO Ready Interrupt Flag Clear */
#define _CMU_IFC_AUXHFRCORDY_SHIFT                  4                                   /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IFC_AUXHFRCORDY_MASK                   0x10UL                              /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IFC_AUXHFRCORDY_DEFAULT                0x00000000UL                        /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_AUXHFRCORDY_DEFAULT                 (_CMU_IFC_AUXHFRCORDY_DEFAULT << 4) /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALRDY                              (0x1UL << 5)                        /**< Calibration Ready Interrupt Flag Clear */
#define _CMU_IFC_CALRDY_SHIFT                       5                                   /**< Shift value for CMU_CALRDY */
#define _CMU_IFC_CALRDY_MASK                        0x20UL                              /**< Bit mask for CMU_CALRDY */
#define _CMU_IFC_CALRDY_DEFAULT                     0x00000000UL                        /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALRDY_DEFAULT                      (_CMU_IFC_CALRDY_DEFAULT << 5)      /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALOF                               (0x1UL << 6)                        /**< Calibration Overflow Interrupt Flag Clear */
#define _CMU_IFC_CALOF_SHIFT                        6                                   /**< Shift value for CMU_CALOF */
#define _CMU_IFC_CALOF_MASK                         0x40UL                              /**< Bit mask for CMU_CALOF */
#define _CMU_IFC_CALOF_DEFAULT                      0x00000000UL                        /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALOF_DEFAULT                       (_CMU_IFC_CALOF_DEFAULT << 6)       /**< Shifted mode DEFAULT for CMU_IFC */

/* Bit fields for CMU IEN */
#define _CMU_IEN_RESETVALUE                         0x00000000UL                        /**< Default value for CMU_IEN */
#define _CMU_IEN_MASK                               0x0000007FUL                        /**< Mask for CMU_IEN */
#define CMU_IEN_HFRCORDY                            (0x1UL << 0)                        /**< HFRCO Ready Interrupt Enable */
#define _CMU_IEN_HFRCORDY_SHIFT                     0                                   /**< Shift value for CMU_HFRCORDY */
#define _CMU_IEN_HFRCORDY_MASK                      0x1UL                               /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IEN_HFRCORDY_DEFAULT                   0x00000000UL                        /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFRCORDY_DEFAULT                    (_CMU_IEN_HFRCORDY_DEFAULT << 0)    /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXORDY                             (0x1UL << 1)                        /**< HFXO Ready Interrupt Enable */
#define _CMU_IEN_HFXORDY_SHIFT                      1                                   /**< Shift value for CMU_HFXORDY */
#define _CMU_IEN_HFXORDY_MASK                       0x2UL                               /**< Bit mask for CMU_HFXORDY */
#define _CMU_IEN_HFXORDY_DEFAULT                    0x00000000UL                        /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXORDY_DEFAULT                     (_CMU_IEN_HFXORDY_DEFAULT << 1)     /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFRCORDY                            (0x1UL << 2)                        /**< LFRCO Ready Interrupt Enable */
#define _CMU_IEN_LFRCORDY_SHIFT                     2                                   /**< Shift value for CMU_LFRCORDY */
#define _CMU_IEN_LFRCORDY_MASK                      0x4UL                               /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IEN_LFRCORDY_DEFAULT                   0x00000000UL                        /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFRCORDY_DEFAULT                    (_CMU_IEN_LFRCORDY_DEFAULT << 2)    /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFXORDY                             (0x1UL << 3)                        /**< LFXO Ready Interrupt Enable */
#define _CMU_IEN_LFXORDY_SHIFT                      3                                   /**< Shift value for CMU_LFXORDY */
#define _CMU_IEN_LFXORDY_MASK                       0x8UL                               /**< Bit mask for CMU_LFXORDY */
#define _CMU_IEN_LFXORDY_DEFAULT                    0x00000000UL                        /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFXORDY_DEFAULT                     (_CMU_IEN_LFXORDY_DEFAULT << 3)     /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_AUXHFRCORDY                         (0x1UL << 4)                        /**< AUXHFRCO Ready Interrupt Enable */
#define _CMU_IEN_AUXHFRCORDY_SHIFT                  4                                   /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IEN_AUXHFRCORDY_MASK                   0x10UL                              /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IEN_AUXHFRCORDY_DEFAULT                0x00000000UL                        /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_AUXHFRCORDY_DEFAULT                 (_CMU_IEN_AUXHFRCORDY_DEFAULT << 4) /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALRDY                              (0x1UL << 5)                        /**< Calibration Ready Interrupt Enable */
#define _CMU_IEN_CALRDY_SHIFT                       5                                   /**< Shift value for CMU_CALRDY */
#define _CMU_IEN_CALRDY_MASK                        0x20UL                              /**< Bit mask for CMU_CALRDY */
#define _CMU_IEN_CALRDY_DEFAULT                     0x00000000UL                        /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALRDY_DEFAULT                      (_CMU_IEN_CALRDY_DEFAULT << 5)      /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALOF                               (0x1UL << 6)                        /**< Calibration Overflow Interrupt Enable */
#define _CMU_IEN_CALOF_SHIFT                        6                                   /**< Shift value for CMU_CALOF */
#define _CMU_IEN_CALOF_MASK                         0x40UL                              /**< Bit mask for CMU_CALOF */
#define _CMU_IEN_CALOF_DEFAULT                      0x00000000UL                        /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALOF_DEFAULT                       (_CMU_IEN_CALOF_DEFAULT << 6)       /**< Shifted mode DEFAULT for CMU_IEN */

/* Bit fields for CMU HFCORECLKEN0 */
#define _CMU_HFCORECLKEN0_RESETVALUE                0x00000000UL                         /**< Default value for CMU_HFCORECLKEN0 */
#define _CMU_HFCORECLKEN0_MASK                      0x00000033UL                         /**< Mask for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_DMA                        (0x1UL << 0)                         /**< Direct Memory Access Controller Clock Enable */
#define _CMU_HFCORECLKEN0_DMA_SHIFT                 0                                    /**< Shift value for CMU_DMA */
#define _CMU_HFCORECLKEN0_DMA_MASK                  0x1UL                                /**< Bit mask for CMU_DMA */
#define _CMU_HFCORECLKEN0_DMA_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_DMA_DEFAULT                (_CMU_HFCORECLKEN0_DMA_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_AES                        (0x1UL << 1)                         /**< Advanced Encryption Standard Accelerator Clock Enable */
#define _CMU_HFCORECLKEN0_AES_SHIFT                 1                                    /**< Shift value for CMU_AES */
#define _CMU_HFCORECLKEN0_AES_MASK                  0x2UL                                /**< Bit mask for CMU_AES */
#define _CMU_HFCORECLKEN0_AES_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_AES_DEFAULT                (_CMU_HFCORECLKEN0_AES_DEFAULT << 1) /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_LE                         (0x1UL << 4)                         /**< Low Energy Peripheral Interface Clock Enable */
#define _CMU_HFCORECLKEN0_LE_SHIFT                  4                                    /**< Shift value for CMU_LE */
#define _CMU_HFCORECLKEN0_LE_MASK                   0x10UL                               /**< Bit mask for CMU_LE */
#define _CMU_HFCORECLKEN0_LE_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_LE_DEFAULT                 (_CMU_HFCORECLKEN0_LE_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_EBI                        (0x1UL << 5)                         /**< External Bus Interface Clock Enable */
#define _CMU_HFCORECLKEN0_EBI_SHIFT                 5                                    /**< Shift value for CMU_EBI */
#define _CMU_HFCORECLKEN0_EBI_MASK                  0x20UL                               /**< Bit mask for CMU_EBI */
#define _CMU_HFCORECLKEN0_EBI_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_EBI_DEFAULT                (_CMU_HFCORECLKEN0_EBI_DEFAULT << 5) /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */

/* Bit fields for CMU HFPERCLKEN0 */
#define _CMU_HFPERCLKEN0_RESETVALUE                 0x00000000UL                           /**< Default value for CMU_HFPERCLKEN0 */
#define _CMU_HFPERCLKEN0_MASK                       0x0003FFFFUL                           /**< Mask for CMU_HFPERCLKEN0 */
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
#define CMU_HFPERCLKEN0_UART0                       (0x1UL << 3)                           /**< Universal Asynchronous Receiver/Transmitter 0 Clock Enable */
#define _CMU_HFPERCLKEN0_UART0_SHIFT                3                                      /**< Shift value for CMU_UART0 */
#define _CMU_HFPERCLKEN0_UART0_MASK                 0x8UL                                  /**< Bit mask for CMU_UART0 */
#define _CMU_HFPERCLKEN0_UART0_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_UART0_DEFAULT               (_CMU_HFPERCLKEN0_UART0_DEFAULT << 3)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_UART1                       (0x1UL << 4)                           /**< Universal Asynchronous Receiver/Transmitter 1 Clock Enable */
#define _CMU_HFPERCLKEN0_UART1_SHIFT                4                                      /**< Shift value for CMU_UART1 */
#define _CMU_HFPERCLKEN0_UART1_MASK                 0x10UL                                 /**< Bit mask for CMU_UART1 */
#define _CMU_HFPERCLKEN0_UART1_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_UART1_DEFAULT               (_CMU_HFPERCLKEN0_UART1_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
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

/** @} End of group EFM32WG890F64_CMU */



/**************************************************************************//**
 * @defgroup EFM32WG890F64_PRS_BitFields  EFM32WG890F64_PRS Bit Fields
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
#define _PRS_CH_CTRL_SIGSEL_RTCCOMP0            0x00000001UL                                /**< Mode RTCCOMP0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_UART0TXC            0x00000001UL                                /**< Mode UART0TXC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_UART1TXC            0x00000001UL                                /**< Mode UART1TXC for PRS_CH_CTRL */
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
#define _PRS_CH_CTRL_SIGSEL_UART0RXDATAV        0x00000002UL                                /**< Mode UART0RXDATAV for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_UART1RXDATAV        0x00000002UL                                /**< Mode UART1RXDATAV for PRS_CH_CTRL */
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
#define PRS_CH_CTRL_SIGSEL_RTCCOMP0             (_PRS_CH_CTRL_SIGSEL_RTCCOMP0 << 0)         /**< Shifted mode RTCCOMP0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_UART0TXC             (_PRS_CH_CTRL_SIGSEL_UART0TXC << 0)         /**< Shifted mode UART0TXC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_UART1TXC             (_PRS_CH_CTRL_SIGSEL_UART1TXC << 0)         /**< Shifted mode UART1TXC for PRS_CH_CTRL */
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
#define PRS_CH_CTRL_SIGSEL_UART0RXDATAV         (_PRS_CH_CTRL_SIGSEL_UART0RXDATAV << 0)     /**< Shifted mode UART0RXDATAV for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_UART1RXDATAV         (_PRS_CH_CTRL_SIGSEL_UART1RXDATAV << 0)     /**< Shifted mode UART1RXDATAV for PRS_CH_CTRL */
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
#define _PRS_CH_CTRL_SOURCESEL_RTC              0x00000028UL                                /**< Mode RTC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_UART0            0x00000029UL                                /**< Mode UART0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_UART1            0x0000002AUL                                /**< Mode UART1 for PRS_CH_CTRL */
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
#define PRS_CH_CTRL_SOURCESEL_RTC               (_PRS_CH_CTRL_SOURCESEL_RTC << 16)          /**< Shifted mode RTC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_UART0             (_PRS_CH_CTRL_SOURCESEL_UART0 << 16)        /**< Shifted mode UART0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_UART1             (_PRS_CH_CTRL_SOURCESEL_UART1 << 16)        /**< Shifted mode UART1 for PRS_CH_CTRL */
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

/** @} End of group EFM32WG890F64_PRS */



/**************************************************************************//**
 * @defgroup EFM32WG890F64_UNLOCK EFM32WG890F64 Unlock Codes
 * @{
 *****************************************************************************/
#define MSC_UNLOCK_CODE      0x1B71 /**< MSC unlock code */
#define EMU_UNLOCK_CODE      0xADE8 /**< EMU unlock code */
#define CMU_UNLOCK_CODE      0x580E /**< CMU unlock code */
#define TIMER_UNLOCK_CODE    0xCE80 /**< TIMER unlock code */
#define GPIO_UNLOCK_CODE     0xA534 /**< GPIO unlock code */
#define BURTC_UNLOCK_CODE    0xAEE8 /**< BURTC unlock code */

/** @} End of group EFM32WG890F64_UNLOCK */

/** @} End of group EFM32WG890F64_BitFields */

/**************************************************************************//**
 * @defgroup EFM32WG890F64_Alternate_Function EFM32WG890F64 Alternate Function
 * @{
 *****************************************************************************/

#include "efm32wg_af_ports.h"
#include "efm32wg_af_pins.h"

/** @} End of group EFM32WG890F64_Alternate_Function */

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

/** @} End of group EFM32WG890F64 */

/** @} End of group Parts */

#ifdef __cplusplus
}
#endif
#endif /* EFM32WG890F64_H */
