/**************************************************************************//**
 * @file efr32mg12p232f1024gm68.h
 * @brief CMSIS Cortex-M Peripheral Access Layer Header File
 *        for EFR32MG12P232F1024GM68
 * @version 5.6.0
 ******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories, Inc. www.silabs.com</b>
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

#if defined(__ICCARM__)
#pragma system_include       /* Treat file as system include file. */
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang system_header  /* Treat file as system include file. */
#endif

#ifndef EFR32MG12P232F1024GM68_H
#define EFR32MG12P232F1024GM68_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************//**
 * @addtogroup Parts
 * @{
 *****************************************************************************/

/**************************************************************************//**
 * @defgroup EFR32MG12P232F1024GM68 EFR32MG12P232F1024GM68
 * @{
 *****************************************************************************/

/** Interrupt Number Definition */
typedef enum IRQn{
/******  Cortex-M4 Processor Exceptions Numbers ********************************************/
  NonMaskableInt_IRQn   = -14,              /*!< 2  Cortex-M4 Non Maskable Interrupt      */
  HardFault_IRQn        = -13,              /*!< 3  Cortex-M4 Hard Fault Interrupt        */
  MemoryManagement_IRQn = -12,              /*!< 4  Cortex-M4 Memory Management Interrupt */
  BusFault_IRQn         = -11,              /*!< 5  Cortex-M4 Bus Fault Interrupt         */
  UsageFault_IRQn       = -10,              /*!< 6  Cortex-M4 Usage Fault Interrupt       */
  SVCall_IRQn           = -5,               /*!< 11 Cortex-M4 SV Call Interrupt           */
  DebugMonitor_IRQn     = -4,               /*!< 12 Cortex-M4 Debug Monitor Interrupt     */
  PendSV_IRQn           = -2,               /*!< 14 Cortex-M4 Pend SV Interrupt           */
  SysTick_IRQn          = -1,               /*!< 15 Cortex-M4 System Tick Interrupt       */

/******  EFR32MG12P Peripheral Interrupt Numbers ********************************************/

  EMU_IRQn              = 0,  /*!< 16+0 EFR32 EMU Interrupt */
  FRC_PRI_IRQn          = 1,  /*!< 16+1 EFR32 FRC_PRI Interrupt */
  WDOG0_IRQn            = 2,  /*!< 16+2 EFR32 WDOG0 Interrupt */
  WDOG1_IRQn            = 3,  /*!< 16+3 EFR32 WDOG1 Interrupt */
  FRC_IRQn              = 4,  /*!< 16+4 EFR32 FRC Interrupt */
  MODEM_IRQn            = 5,  /*!< 16+5 EFR32 MODEM Interrupt */
  RAC_SEQ_IRQn          = 6,  /*!< 16+6 EFR32 RAC_SEQ Interrupt */
  RAC_RSM_IRQn          = 7,  /*!< 16+7 EFR32 RAC_RSM Interrupt */
  BUFC_IRQn             = 8,  /*!< 16+8 EFR32 BUFC Interrupt */
  LDMA_IRQn             = 9,  /*!< 16+9 EFR32 LDMA Interrupt */
  GPIO_EVEN_IRQn        = 10, /*!< 16+10 EFR32 GPIO_EVEN Interrupt */
  TIMER0_IRQn           = 11, /*!< 16+11 EFR32 TIMER0 Interrupt */
  USART0_RX_IRQn        = 12, /*!< 16+12 EFR32 USART0_RX Interrupt */
  USART0_TX_IRQn        = 13, /*!< 16+13 EFR32 USART0_TX Interrupt */
  ACMP0_IRQn            = 14, /*!< 16+14 EFR32 ACMP0 Interrupt */
  ADC0_IRQn             = 15, /*!< 16+15 EFR32 ADC0 Interrupt */
  IDAC0_IRQn            = 16, /*!< 16+16 EFR32 IDAC0 Interrupt */
  I2C0_IRQn             = 17, /*!< 16+17 EFR32 I2C0 Interrupt */
  GPIO_ODD_IRQn         = 18, /*!< 16+18 EFR32 GPIO_ODD Interrupt */
  TIMER1_IRQn           = 19, /*!< 16+19 EFR32 TIMER1 Interrupt */
  USART1_RX_IRQn        = 20, /*!< 16+20 EFR32 USART1_RX Interrupt */
  USART1_TX_IRQn        = 21, /*!< 16+21 EFR32 USART1_TX Interrupt */
  LEUART0_IRQn          = 22, /*!< 16+22 EFR32 LEUART0 Interrupt */
  PCNT0_IRQn            = 23, /*!< 16+23 EFR32 PCNT0 Interrupt */
  CMU_IRQn              = 24, /*!< 16+24 EFR32 CMU Interrupt */
  MSC_IRQn              = 25, /*!< 16+25 EFR32 MSC Interrupt */
  CRYPTO0_IRQn          = 26, /*!< 16+26 EFR32 CRYPTO0 Interrupt */
  LETIMER0_IRQn         = 27, /*!< 16+27 EFR32 LETIMER0 Interrupt */
  AGC_IRQn              = 28, /*!< 16+28 EFR32 AGC Interrupt */
  PROTIMER_IRQn         = 29, /*!< 16+29 EFR32 PROTIMER Interrupt */
  RTCC_IRQn             = 30, /*!< 16+30 EFR32 RTCC Interrupt */
  SYNTH_IRQn            = 31, /*!< 16+31 EFR32 SYNTH Interrupt */
  CRYOTIMER_IRQn        = 32, /*!< 16+32 EFR32 CRYOTIMER Interrupt */
  RFSENSE_IRQn          = 33, /*!< 16+33 EFR32 RFSENSE Interrupt */
  FPUEH_IRQn            = 34, /*!< 16+34 EFR32 FPUEH Interrupt */
  SMU_IRQn              = 35, /*!< 16+35 EFR32 SMU Interrupt */
  WTIMER0_IRQn          = 36, /*!< 16+36 EFR32 WTIMER0 Interrupt */
  WTIMER1_IRQn          = 37, /*!< 16+37 EFR32 WTIMER1 Interrupt */
  PCNT1_IRQn            = 38, /*!< 16+38 EFR32 PCNT1 Interrupt */
  PCNT2_IRQn            = 39, /*!< 16+39 EFR32 PCNT2 Interrupt */
  USART2_RX_IRQn        = 40, /*!< 16+40 EFR32 USART2_RX Interrupt */
  USART2_TX_IRQn        = 41, /*!< 16+41 EFR32 USART2_TX Interrupt */
  I2C1_IRQn             = 42, /*!< 16+42 EFR32 I2C1 Interrupt */
  USART3_RX_IRQn        = 43, /*!< 16+43 EFR32 USART3_RX Interrupt */
  USART3_TX_IRQn        = 44, /*!< 16+44 EFR32 USART3_TX Interrupt */
  VDAC0_IRQn            = 45, /*!< 16+45 EFR32 VDAC0 Interrupt */
  CSEN_IRQn             = 46, /*!< 16+46 EFR32 CSEN Interrupt */
  LESENSE_IRQn          = 47, /*!< 16+47 EFR32 LESENSE Interrupt */
  CRYPTO1_IRQn          = 48, /*!< 16+48 EFR32 CRYPTO1 Interrupt */
  TRNG0_IRQn            = 49, /*!< 16+49 EFR32 TRNG0 Interrupt */
} IRQn_Type;

#define CRYPTO_IRQn               CRYPTO0_IRQn /*!< Alias for CRYPTO0_IRQn */

/**************************************************************************//**
 * @defgroup EFR32MG12P232F1024GM68_Core Core
 * @{
 * @brief Processor and Core Peripheral Section
 *****************************************************************************/
#define __MPU_PRESENT             1U /**< Presence of MPU  */
#define __FPU_PRESENT             1U /**< Presence of FPU  */
#define __VTOR_PRESENT            1U /**< Presence of VTOR register in SCB */
#define __NVIC_PRIO_BITS          3U /**< NVIC interrupt priority bits */
#define __Vendor_SysTickConfig    0U /**< Is 1 if different SysTick counter is used */

/** @} End of group EFR32MG12P232F1024GM68_Core */

/**************************************************************************//**
* @defgroup EFR32MG12P232F1024GM68_Part Part
* @{
******************************************************************************/

/** Part family */
#define _EFR32_MIGHTY_FAMILY                    1                               /**< MIGHTY Gecko RF SoC Family  */
#define _EFR_DEVICE                                                             /**< Silicon Labs EFR-type RF SoC */
#define _SILICON_LABS_32B_SERIES_1                                              /**< Silicon Labs series number */
#define _SILICON_LABS_32B_SERIES                1                               /**< Silicon Labs series number */
#define _SILICON_LABS_32B_SERIES_1_CONFIG_2                                     /**< Series 1, Configuration 2 */
#define _SILICON_LABS_32B_SERIES_1_CONFIG       2                               /**< Series 1, Configuration 2 */
#define _SILICON_LABS_GECKO_INTERNAL_SDID       84                              /**< Silicon Labs internal use only, may change any time */
#define _SILICON_LABS_GECKO_INTERNAL_SDID_84                                    /**< Silicon Labs internal use only, may change any time */
#define _SILICON_LABS_EFR32_RADIO_SUBGHZ        1                               /**< Radio supports Sub-GHz */
#define _SILICON_LABS_EFR32_RADIO_2G4HZ         2                               /**< Radio supports 2.4 GHz */
#define _SILICON_LABS_EFR32_RADIO_DUALBAND      3                               /**< Radio supports dual band */
#define _SILICON_LABS_EFR32_RADIO_TYPE          _SILICON_LABS_EFR32_RADIO_2G4HZ /**< Radio type */
#define _SILICON_LABS_32B_PLATFORM_2                                            /**< @deprecated Silicon Labs platform name */
#define _SILICON_LABS_32B_PLATFORM              2                               /**< @deprecated Silicon Labs platform name */
#define _SILICON_LABS_32B_PLATFORM_2_GEN_2                                      /**< @deprecated Platform 2, generation 2 */
#define _SILICON_LABS_32B_PLATFORM_2_GEN        2                               /**< @deprecated Platform 2, generation 2 */

/* If part number is not defined as compiler option, define it */
#if !defined(EFR32MG12P232F1024GM68)
#define EFR32MG12P232F1024GM68    1 /**< MIGHTY Gecko Part */
#endif

/** Configure part number */
#define PART_NUMBER                "EFR32MG12P232F1024GM68" /**< Part Number */

/** Memory Base addresses and limits */
#define RAM0_CODE_MEM_BASE         ((uint32_t) 0x10000000UL) /**< RAM0_CODE base address  */
#define RAM0_CODE_MEM_SIZE         ((uint32_t) 0x20000UL)    /**< RAM0_CODE available address space  */
#define RAM0_CODE_MEM_END          ((uint32_t) 0x1001FFFFUL) /**< RAM0_CODE end address  */
#define RAM0_CODE_MEM_BITS         ((uint32_t) 0x00000011UL) /**< RAM0_CODE used bits  */
#define RAM2_MEM_BASE              ((uint32_t) 0x20040000UL) /**< RAM2 base address  */
#define RAM2_MEM_SIZE              ((uint32_t) 0x800UL)      /**< RAM2 available address space  */
#define RAM2_MEM_END               ((uint32_t) 0x200407FFUL) /**< RAM2 end address  */
#define RAM2_MEM_BITS              ((uint32_t) 0x0000000BUL) /**< RAM2 used bits  */
#define RAM1_MEM_BASE              ((uint32_t) 0x20020000UL) /**< RAM1 base address  */
#define RAM1_MEM_SIZE              ((uint32_t) 0x20000UL)    /**< RAM1 available address space  */
#define RAM1_MEM_END               ((uint32_t) 0x2003FFFFUL) /**< RAM1 end address  */
#define RAM1_MEM_BITS              ((uint32_t) 0x00000011UL) /**< RAM1 used bits  */
#define CRYPTO1_BITCLR_MEM_BASE    ((uint32_t) 0x440F0400UL) /**< CRYPTO1_BITCLR base address  */
#define CRYPTO1_BITCLR_MEM_SIZE    ((uint32_t) 0x400UL)      /**< CRYPTO1_BITCLR available address space  */
#define CRYPTO1_BITCLR_MEM_END     ((uint32_t) 0x440F07FFUL) /**< CRYPTO1_BITCLR end address  */
#define CRYPTO1_BITCLR_MEM_BITS    ((uint32_t) 0x0000000AUL) /**< CRYPTO1_BITCLR used bits  */
#define PER_MEM_BASE               ((uint32_t) 0x40000000UL) /**< PER base address  */
#define PER_MEM_SIZE               ((uint32_t) 0xF0000UL)    /**< PER available address space  */
#define PER_MEM_END                ((uint32_t) 0x400EFFFFUL) /**< PER end address  */
#define PER_MEM_BITS               ((uint32_t) 0x00000014UL) /**< PER used bits  */
#define RAM1_CODE_MEM_BASE         ((uint32_t) 0x10020000UL) /**< RAM1_CODE base address  */
#define RAM1_CODE_MEM_SIZE         ((uint32_t) 0x20000UL)    /**< RAM1_CODE available address space  */
#define RAM1_CODE_MEM_END          ((uint32_t) 0x1003FFFFUL) /**< RAM1_CODE end address  */
#define RAM1_CODE_MEM_BITS         ((uint32_t) 0x00000011UL) /**< RAM1_CODE used bits  */
#define CRYPTO1_MEM_BASE           ((uint32_t) 0x400F0400UL) /**< CRYPTO1 base address  */
#define CRYPTO1_MEM_SIZE           ((uint32_t) 0x400UL)      /**< CRYPTO1 available address space  */
#define CRYPTO1_MEM_END            ((uint32_t) 0x400F07FFUL) /**< CRYPTO1 end address  */
#define CRYPTO1_MEM_BITS           ((uint32_t) 0x0000000AUL) /**< CRYPTO1 used bits  */
#define FLASH_MEM_BASE             ((uint32_t) 0x00000000UL) /**< FLASH base address  */
#define FLASH_MEM_SIZE             ((uint32_t) 0x10000000UL) /**< FLASH available address space  */
#define FLASH_MEM_END              ((uint32_t) 0x0FFFFFFFUL) /**< FLASH end address  */
#define FLASH_MEM_BITS             ((uint32_t) 0x0000001CUL) /**< FLASH used bits  */
#define CRYPTO0_MEM_BASE           ((uint32_t) 0x400F0000UL) /**< CRYPTO0 base address  */
#define CRYPTO0_MEM_SIZE           ((uint32_t) 0x400UL)      /**< CRYPTO0 available address space  */
#define CRYPTO0_MEM_END            ((uint32_t) 0x400F03FFUL) /**< CRYPTO0 end address  */
#define CRYPTO0_MEM_BITS           ((uint32_t) 0x0000000AUL) /**< CRYPTO0 used bits  */
#define CRYPTO_MEM_BASE            CRYPTO0_MEM_BASE          /**< Alias for CRYPTO0_MEM_BASE */
#define CRYPTO_MEM_SIZE            CRYPTO0_MEM_SIZE          /**< Alias for CRYPTO0_MEM_SIZE */
#define CRYPTO_MEM_END             CRYPTO0_MEM_END           /**< Alias for CRYPTO0_MEM_END  */
#define CRYPTO_MEM_BITS            CRYPTO0_MEM_BITS          /**< Alias for CRYPTO0_MEM_BITS */
#define PER_BITCLR_MEM_BASE        ((uint32_t) 0x44000000UL) /**< PER_BITCLR base address  */
#define PER_BITCLR_MEM_SIZE        ((uint32_t) 0xF0000UL)    /**< PER_BITCLR available address space  */
#define PER_BITCLR_MEM_END         ((uint32_t) 0x440EFFFFUL) /**< PER_BITCLR end address  */
#define PER_BITCLR_MEM_BITS        ((uint32_t) 0x00000014UL) /**< PER_BITCLR used bits  */
#define CRYPTO0_BITSET_MEM_BASE    ((uint32_t) 0x460F0000UL) /**< CRYPTO0_BITSET base address  */
#define CRYPTO0_BITSET_MEM_SIZE    ((uint32_t) 0x400UL)      /**< CRYPTO0_BITSET available address space  */
#define CRYPTO0_BITSET_MEM_END     ((uint32_t) 0x460F03FFUL) /**< CRYPTO0_BITSET end address  */
#define CRYPTO0_BITSET_MEM_BITS    ((uint32_t) 0x0000000AUL) /**< CRYPTO0_BITSET used bits  */
#define CRYPTO_BITSET_MEM_BASE     CRYPTO0_BITSET_MEM_BASE   /**< Alias for CRYPTO0_BITSET_MEM_BASE */
#define CRYPTO_BITSET_MEM_SIZE     CRYPTO0_BITSET_MEM_SIZE   /**< Alias for CRYPTO0_BITSET_MEM_SIZE */
#define CRYPTO_BITSET_MEM_END      CRYPTO0_BITSET_MEM_END    /**< Alias for CRYPTO0_BITSET_MEM_END  */
#define CRYPTO_BITSET_MEM_BITS     CRYPTO0_BITSET_MEM_BITS   /**< Alias for CRYPTO0_BITSET_MEM_BITS */
#define CRYPTO0_BITCLR_MEM_BASE    ((uint32_t) 0x440F0000UL) /**< CRYPTO0_BITCLR base address  */
#define CRYPTO0_BITCLR_MEM_SIZE    ((uint32_t) 0x400UL)      /**< CRYPTO0_BITCLR available address space  */
#define CRYPTO0_BITCLR_MEM_END     ((uint32_t) 0x440F03FFUL) /**< CRYPTO0_BITCLR end address  */
#define CRYPTO0_BITCLR_MEM_BITS    ((uint32_t) 0x0000000AUL) /**< CRYPTO0_BITCLR used bits  */
#define CRYPTO_BITCLR_MEM_BASE     CRYPTO0_BITCLR_MEM_BASE   /**< Alias for CRYPTO0_BITCLR_MEM_BASE */
#define CRYPTO_BITCLR_MEM_SIZE     CRYPTO0_BITCLR_MEM_SIZE   /**< Alias for CRYPTO0_BITCLR_MEM_SIZE */
#define CRYPTO_BITCLR_MEM_END      CRYPTO0_BITCLR_MEM_END    /**< Alias for CRYPTO0_BITCLR_MEM_END  */
#define CRYPTO_BITCLR_MEM_BITS     CRYPTO0_BITCLR_MEM_BITS   /**< Alias for CRYPTO0_BITCLR_MEM_BITS */
#define PER_BITSET_MEM_BASE        ((uint32_t) 0x46000000UL) /**< PER_BITSET base address  */
#define PER_BITSET_MEM_SIZE        ((uint32_t) 0xF0000UL)    /**< PER_BITSET available address space  */
#define PER_BITSET_MEM_END         ((uint32_t) 0x460EFFFFUL) /**< PER_BITSET end address  */
#define PER_BITSET_MEM_BITS        ((uint32_t) 0x00000014UL) /**< PER_BITSET used bits  */
#define CRYPTO1_BITSET_MEM_BASE    ((uint32_t) 0x460F0400UL) /**< CRYPTO1_BITSET base address  */
#define CRYPTO1_BITSET_MEM_SIZE    ((uint32_t) 0x400UL)      /**< CRYPTO1_BITSET available address space  */
#define CRYPTO1_BITSET_MEM_END     ((uint32_t) 0x460F07FFUL) /**< CRYPTO1_BITSET end address  */
#define CRYPTO1_BITSET_MEM_BITS    ((uint32_t) 0x0000000AUL) /**< CRYPTO1_BITSET used bits  */
#define RAM2_CODE_MEM_BASE         ((uint32_t) 0x10040000UL) /**< RAM2_CODE base address  */
#define RAM2_CODE_MEM_SIZE         ((uint32_t) 0x800UL)      /**< RAM2_CODE available address space  */
#define RAM2_CODE_MEM_END          ((uint32_t) 0x100407FFUL) /**< RAM2_CODE end address  */
#define RAM2_CODE_MEM_BITS         ((uint32_t) 0x0000000BUL) /**< RAM2_CODE used bits  */
#define RAM_MEM_BASE               ((uint32_t) 0x20000000UL) /**< RAM base address  */
#define RAM_MEM_SIZE               ((uint32_t) 0x20000UL)    /**< RAM available address space  */
#define RAM_MEM_END                ((uint32_t) 0x2001FFFFUL) /**< RAM end address  */
#define RAM_MEM_BITS               ((uint32_t) 0x00000011UL) /**< RAM used bits  */

/** Bit banding area */
#define BITBAND_PER_BASE           ((uint32_t) 0x42000000UL) /**< Peripheral Address Space bit-band area */
#define BITBAND_RAM_BASE           ((uint32_t) 0x22000000UL) /**< SRAM Address Space bit-band area */

/** Flash and SRAM limits for EFR32MG12P232F1024GM68 */
#define FLASH_BASE                 (0x00000000UL) /**< Flash Base Address */
#define FLASH_SIZE                 (0x00100000UL) /**< Available Flash Memory */
#define FLASH_PAGE_SIZE            2048U          /**< Flash Memory page size (interleaving off) */
#define SRAM_BASE                  (0x20000000UL) /**< SRAM Base Address */
#define SRAM_SIZE                  (0x00020000UL) /**< Available SRAM Memory */
#define __CM4_REV                  0x0001U        /**< Cortex-M4 Core revision r0p1 */
#define PRS_CHAN_COUNT             12             /**< Number of PRS channels */
#define DMA_CHAN_COUNT             8              /**< Number of DMA channels */
#define EXT_IRQ_COUNT              51             /**< Number of External (NVIC) interrupts */

/** AF channels connect the different on-chip peripherals with the af-mux */
#define AFCHAN_MAX                 136U
/** AF channel maximum location number */
#define AFCHANLOC_MAX              32U
/** Analog AF channels */
#define AFACHAN_MAX                125U

/* Part number capabilities */

#define CRYPTO_PRESENT          /**< CRYPTO is available in this part */
#define CRYPTO_COUNT          2 /**< 2 CRYPTOs available  */
#define TIMER_PRESENT           /**< TIMER is available in this part */
#define TIMER_COUNT           2 /**< 2 TIMERs available  */
#define WTIMER_PRESENT          /**< WTIMER is available in this part */
#define WTIMER_COUNT          2 /**< 2 WTIMERs available  */
#define USART_PRESENT           /**< USART is available in this part */
#define USART_COUNT           4 /**< 4 USARTs available  */
#define LEUART_PRESENT          /**< LEUART is available in this part */
#define LEUART_COUNT          1 /**< 1 LEUARTs available  */
#define LETIMER_PRESENT         /**< LETIMER is available in this part */
#define LETIMER_COUNT         1 /**< 1 LETIMERs available  */
#define PCNT_PRESENT            /**< PCNT is available in this part */
#define PCNT_COUNT            3 /**< 3 PCNTs available  */
#define I2C_PRESENT             /**< I2C is available in this part */
#define I2C_COUNT             2 /**< 2 I2Cs available  */
#define ADC_PRESENT             /**< ADC is available in this part */
#define ADC_COUNT             1 /**< 1 ADCs available  */
#define ACMP_PRESENT            /**< ACMP is available in this part */
#define ACMP_COUNT            2 /**< 2 ACMPs available  */
#define IDAC_PRESENT            /**< IDAC is available in this part */
#define IDAC_COUNT            1 /**< 1 IDACs available  */
#define VDAC_PRESENT            /**< VDAC is available in this part */
#define VDAC_COUNT            1 /**< 1 VDACs available  */
#define WDOG_PRESENT            /**< WDOG is available in this part */
#define WDOG_COUNT            2 /**< 2 WDOGs available  */
#define TRNG_PRESENT            /**< TRNG is available in this part */
#define TRNG_COUNT            1 /**< 1 TRNGs available  */
#define MSC_PRESENT             /**< MSC is available in this part */
#define MSC_COUNT             1 /**< 1 MSC available */
#define EMU_PRESENT             /**< EMU is available in this part */
#define EMU_COUNT             1 /**< 1 EMU available */
#define RMU_PRESENT             /**< RMU is available in this part */
#define RMU_COUNT             1 /**< 1 RMU available */
#define CMU_PRESENT             /**< CMU is available in this part */
#define CMU_COUNT             1 /**< 1 CMU available */
#define GPIO_PRESENT            /**< GPIO is available in this part */
#define GPIO_COUNT            1 /**< 1 GPIO available */
#define PRS_PRESENT             /**< PRS is available in this part */
#define PRS_COUNT             1 /**< 1 PRS available */
#define LDMA_PRESENT            /**< LDMA is available in this part */
#define LDMA_COUNT            1 /**< 1 LDMA available */
#define FPUEH_PRESENT           /**< FPUEH is available in this part */
#define FPUEH_COUNT           1 /**< 1 FPUEH available */
#define GPCRC_PRESENT           /**< GPCRC is available in this part */
#define GPCRC_COUNT           1 /**< 1 GPCRC available */
#define CRYOTIMER_PRESENT       /**< CRYOTIMER is available in this part */
#define CRYOTIMER_COUNT       1 /**< 1 CRYOTIMER available */
#define CSEN_PRESENT            /**< CSEN is available in this part */
#define CSEN_COUNT            1 /**< 1 CSEN available */
#define LESENSE_PRESENT         /**< LESENSE is available in this part */
#define LESENSE_COUNT         1 /**< 1 LESENSE available */
#define RTCC_PRESENT            /**< RTCC is available in this part */
#define RTCC_COUNT            1 /**< 1 RTCC available */
#define ETM_PRESENT             /**< ETM is available in this part */
#define ETM_COUNT             1 /**< 1 ETM available */
#define BOOTLOADER_PRESENT      /**< BOOTLOADER is available in this part */
#define BOOTLOADER_COUNT      1 /**< 1 BOOTLOADER available */
#define SMU_PRESENT             /**< SMU is available in this part */
#define SMU_COUNT             1 /**< 1 SMU available */
#define DCDC_PRESENT            /**< DCDC is available in this part */
#define DCDC_COUNT            1 /**< 1 DCDC available */

#include "core_cm4.h"           /* Cortex-M4 processor and core peripherals */
#include "system_efr32mg12p.h"  /* System Header File */

/** @} End of group EFR32MG12P232F1024GM68_Part */

/**************************************************************************//**
 * @defgroup EFR32MG12P232F1024GM68_Peripheral_TypeDefs Peripheral TypeDefs
 * @{
 * @brief Device Specific Peripheral Register Structures
 *****************************************************************************/

#include "efr32mg12p_msc.h"
#include "efr32mg12p_emu.h"
#include "efr32mg12p_rmu.h"
#include "efr32mg12p_cmu.h"
#include "efr32mg12p_crypto.h"
#include "efr32mg12p_gpio_p.h"
#include "efr32mg12p_gpio.h"
#include "efr32mg12p_prs_ch.h"
#include "efr32mg12p_prs.h"
#include "efr32mg12p_ldma_ch.h"
#include "efr32mg12p_ldma.h"
#include "efr32mg12p_fpueh.h"
#include "efr32mg12p_gpcrc.h"
#include "efr32mg12p_timer_cc.h"
#include "efr32mg12p_timer.h"
#include "efr32mg12p_usart.h"
#include "efr32mg12p_leuart.h"
#include "efr32mg12p_letimer.h"
#include "efr32mg12p_cryotimer.h"
#include "efr32mg12p_pcnt.h"
#include "efr32mg12p_i2c.h"
#include "efr32mg12p_adc.h"
#include "efr32mg12p_acmp.h"
#include "efr32mg12p_idac.h"
#include "efr32mg12p_vdac_opa.h"
#include "efr32mg12p_vdac.h"
#include "efr32mg12p_csen.h"
#include "efr32mg12p_lesense_st.h"
#include "efr32mg12p_lesense_buf.h"
#include "efr32mg12p_lesense_ch.h"
#include "efr32mg12p_lesense.h"
#include "efr32mg12p_rtcc_cc.h"
#include "efr32mg12p_rtcc_ret.h"
#include "efr32mg12p_rtcc.h"
#include "efr32mg12p_wdog_pch.h"
#include "efr32mg12p_wdog.h"
#include "efr32mg12p_etm.h"
#include "efr32mg12p_smu.h"
#include "efr32mg12p_trng.h"
#include "efr32mg12p_dma_descriptor.h"
#include "efr32mg12p_devinfo.h"
#include "efr32mg12p_romtable.h"

/** @} End of group EFR32MG12P232F1024GM68_Peripheral_TypeDefs  */

/**************************************************************************//**
 * @defgroup EFR32MG12P232F1024GM68_Peripheral_Base Peripheral Memory Map
 * @{
 *****************************************************************************/

#define MSC_BASE          (0x400E0000UL) /**< MSC base address  */
#define EMU_BASE          (0x400E3000UL) /**< EMU base address  */
#define RMU_BASE          (0x400E5000UL) /**< RMU base address  */
#define CMU_BASE          (0x400E4000UL) /**< CMU base address  */
#define CRYPTO0_BASE      (0x400F0000UL) /**< CRYPTO0 base address  */
#define CRYPTO_BASE       CRYPTO0_BASE   /**< Alias for CRYPTO0 base address */
#define CRYPTO1_BASE      (0x400F0400UL) /**< CRYPTO1 base address  */
#define GPIO_BASE         (0x4000A000UL) /**< GPIO base address  */
#define PRS_BASE          (0x400E6000UL) /**< PRS base address  */
#define LDMA_BASE         (0x400E2000UL) /**< LDMA base address  */
#define FPUEH_BASE        (0x400E1000UL) /**< FPUEH base address  */
#define GPCRC_BASE        (0x4001C000UL) /**< GPCRC base address  */
#define TIMER0_BASE       (0x40018000UL) /**< TIMER0 base address  */
#define TIMER1_BASE       (0x40018400UL) /**< TIMER1 base address  */
#define WTIMER0_BASE      (0x4001A000UL) /**< WTIMER0 base address  */
#define WTIMER1_BASE      (0x4001A400UL) /**< WTIMER1 base address  */
#define USART0_BASE       (0x40010000UL) /**< USART0 base address  */
#define USART1_BASE       (0x40010400UL) /**< USART1 base address  */
#define USART2_BASE       (0x40010800UL) /**< USART2 base address  */
#define USART3_BASE       (0x40010C00UL) /**< USART3 base address  */
#define LEUART0_BASE      (0x4004A000UL) /**< LEUART0 base address  */
#define LETIMER0_BASE     (0x40046000UL) /**< LETIMER0 base address  */
#define CRYOTIMER_BASE    (0x4001E000UL) /**< CRYOTIMER base address  */
#define PCNT0_BASE        (0x4004E000UL) /**< PCNT0 base address  */
#define PCNT1_BASE        (0x4004E400UL) /**< PCNT1 base address  */
#define PCNT2_BASE        (0x4004E800UL) /**< PCNT2 base address  */
#define I2C0_BASE         (0x4000C000UL) /**< I2C0 base address  */
#define I2C1_BASE         (0x4000C400UL) /**< I2C1 base address  */
#define ADC0_BASE         (0x40002000UL) /**< ADC0 base address  */
#define ACMP0_BASE        (0x40000000UL) /**< ACMP0 base address  */
#define ACMP1_BASE        (0x40000400UL) /**< ACMP1 base address  */
#define IDAC0_BASE        (0x40006000UL) /**< IDAC0 base address  */
#define VDAC0_BASE        (0x40008000UL) /**< VDAC0 base address  */
#define CSEN_BASE         (0x4001F000UL) /**< CSEN base address  */
#define LESENSE_BASE      (0x40055000UL) /**< LESENSE base address  */
#define RTCC_BASE         (0x40042000UL) /**< RTCC base address  */
#define WDOG0_BASE        (0x40052000UL) /**< WDOG0 base address  */
#define WDOG1_BASE        (0x40052400UL) /**< WDOG1 base address  */
#define ETM_BASE          (0xE0041000UL) /**< ETM base address  */
#define SMU_BASE          (0x40022000UL) /**< SMU base address  */
#define TRNG0_BASE        (0x4001D000UL) /**< TRNG0 base address  */
#define DEVINFO_BASE      (0x0FE081B0UL) /**< DEVINFO base address */
#define ROMTABLE_BASE     (0xE00FFFD0UL) /**< ROMTABLE base address */
#define LOCKBITS_BASE     (0x0FE04000UL) /**< Lock-bits page base address */
#define USERDATA_BASE     (0x0FE00000UL) /**< User data page base address */

/** @} End of group EFR32MG12P232F1024GM68_Peripheral_Base */

/**************************************************************************//**
 * @defgroup EFR32MG12P232F1024GM68_Peripheral_Declaration Peripheral Declarations
 * @{
 *****************************************************************************/

#define MSC          ((MSC_TypeDef *) MSC_BASE)             /**< MSC base pointer */
#define EMU          ((EMU_TypeDef *) EMU_BASE)             /**< EMU base pointer */
#define RMU          ((RMU_TypeDef *) RMU_BASE)             /**< RMU base pointer */
#define CMU          ((CMU_TypeDef *) CMU_BASE)             /**< CMU base pointer */
#define CRYPTO0      ((CRYPTO_TypeDef *) CRYPTO0_BASE)      /**< CRYPTO0 base pointer */
#define CRYPTO       CRYPTO0                                /**< Alias for CRYPTO0 base pointer */
#define CRYPTO1      ((CRYPTO_TypeDef *) CRYPTO1_BASE)      /**< CRYPTO1 base pointer */
#define GPIO         ((GPIO_TypeDef *) GPIO_BASE)           /**< GPIO base pointer */
#define PRS          ((PRS_TypeDef *) PRS_BASE)             /**< PRS base pointer */
#define LDMA         ((LDMA_TypeDef *) LDMA_BASE)           /**< LDMA base pointer */
#define FPUEH        ((FPUEH_TypeDef *) FPUEH_BASE)         /**< FPUEH base pointer */
#define GPCRC        ((GPCRC_TypeDef *) GPCRC_BASE)         /**< GPCRC base pointer */
#define TIMER0       ((TIMER_TypeDef *) TIMER0_BASE)        /**< TIMER0 base pointer */
#define TIMER1       ((TIMER_TypeDef *) TIMER1_BASE)        /**< TIMER1 base pointer */
#define WTIMER0      ((TIMER_TypeDef *) WTIMER0_BASE)       /**< WTIMER0 base pointer */
#define WTIMER1      ((TIMER_TypeDef *) WTIMER1_BASE)       /**< WTIMER1 base pointer */
#define USART0       ((USART_TypeDef *) USART0_BASE)        /**< USART0 base pointer */
#define USART1       ((USART_TypeDef *) USART1_BASE)        /**< USART1 base pointer */
#define USART2       ((USART_TypeDef *) USART2_BASE)        /**< USART2 base pointer */
#define USART3       ((USART_TypeDef *) USART3_BASE)        /**< USART3 base pointer */
#define LEUART0      ((LEUART_TypeDef *) LEUART0_BASE)      /**< LEUART0 base pointer */
#define LETIMER0     ((LETIMER_TypeDef *) LETIMER0_BASE)    /**< LETIMER0 base pointer */
#define CRYOTIMER    ((CRYOTIMER_TypeDef *) CRYOTIMER_BASE) /**< CRYOTIMER base pointer */
#define PCNT0        ((PCNT_TypeDef *) PCNT0_BASE)          /**< PCNT0 base pointer */
#define PCNT1        ((PCNT_TypeDef *) PCNT1_BASE)          /**< PCNT1 base pointer */
#define PCNT2        ((PCNT_TypeDef *) PCNT2_BASE)          /**< PCNT2 base pointer */
#define I2C0         ((I2C_TypeDef *) I2C0_BASE)            /**< I2C0 base pointer */
#define I2C1         ((I2C_TypeDef *) I2C1_BASE)            /**< I2C1 base pointer */
#define ADC0         ((ADC_TypeDef *) ADC0_BASE)            /**< ADC0 base pointer */
#define ACMP0        ((ACMP_TypeDef *) ACMP0_BASE)          /**< ACMP0 base pointer */
#define ACMP1        ((ACMP_TypeDef *) ACMP1_BASE)          /**< ACMP1 base pointer */
#define IDAC0        ((IDAC_TypeDef *) IDAC0_BASE)          /**< IDAC0 base pointer */
#define VDAC0        ((VDAC_TypeDef *) VDAC0_BASE)          /**< VDAC0 base pointer */
#define CSEN         ((CSEN_TypeDef *) CSEN_BASE)           /**< CSEN base pointer */
#define LESENSE      ((LESENSE_TypeDef *) LESENSE_BASE)     /**< LESENSE base pointer */
#define RTCC         ((RTCC_TypeDef *) RTCC_BASE)           /**< RTCC base pointer */
#define WDOG0        ((WDOG_TypeDef *) WDOG0_BASE)          /**< WDOG0 base pointer */
#define WDOG1        ((WDOG_TypeDef *) WDOG1_BASE)          /**< WDOG1 base pointer */
#define ETM          ((ETM_TypeDef *) ETM_BASE)             /**< ETM base pointer */
#define SMU          ((SMU_TypeDef *) SMU_BASE)             /**< SMU base pointer */
#define TRNG0        ((TRNG_TypeDef *) TRNG0_BASE)          /**< TRNG0 base pointer */
#define DEVINFO      ((DEVINFO_TypeDef *) DEVINFO_BASE)     /**< DEVINFO base pointer */
#define ROMTABLE     ((ROMTABLE_TypeDef *) ROMTABLE_BASE)   /**< ROMTABLE base pointer */

/** @} End of group EFR32MG12P232F1024GM68_Peripheral_Declaration */

/**************************************************************************//**
 * @defgroup EFR32MG12P232F1024GM68_Peripheral_Offsets Peripheral Offsets
 * @{
 *****************************************************************************/

#define CRYPTO_OFFSET     0x400 /**< Offset in bytes between CRYPTO instances */
#define TIMER_OFFSET      0x400 /**< Offset in bytes between TIMER instances */
#define WTIMER_OFFSET     0x400 /**< Offset in bytes between WTIMER instances */
#define USART_OFFSET      0x400 /**< Offset in bytes between USART instances */
#define LEUART_OFFSET     0x400 /**< Offset in bytes between LEUART instances */
#define LETIMER_OFFSET    0x400 /**< Offset in bytes between LETIMER instances */
#define PCNT_OFFSET       0x400 /**< Offset in bytes between PCNT instances */
#define I2C_OFFSET        0x400 /**< Offset in bytes between I2C instances */
#define ADC_OFFSET        0x400 /**< Offset in bytes between ADC instances */
#define ACMP_OFFSET       0x400 /**< Offset in bytes between ACMP instances */
#define IDAC_OFFSET       0x400 /**< Offset in bytes between IDAC instances */
#define VDAC_OFFSET       0x400 /**< Offset in bytes between VDAC instances */
#define WDOG_OFFSET       0x400 /**< Offset in bytes between WDOG instances */
#define TRNG_OFFSET       0x400 /**< Offset in bytes between TRNG instances */

/** @} End of group EFR32MG12P232F1024GM68_Peripheral_Offsets */

/**************************************************************************//**
 * @defgroup EFR32MG12P232F1024GM68_BitFields Bit Fields
 * @{
 *****************************************************************************/

#include "efr32mg12p_prs_signals.h"
#include "efr32mg12p_dmareq.h"

/**************************************************************************//**
 * @addtogroup EFR32MG12P232F1024GM68_WTIMER
 * @{
 * @defgroup EFR32MG12P232F1024GM68_WTIMER_BitFields  WTIMER Bit Fields
 * @{
 *****************************************************************************/

/* Bit fields for WTIMER CTRL */
#define _WTIMER_CTRL_RESETVALUE                     0x00000000UL                              /**< Default value for WTIMER_CTRL */
#define _WTIMER_CTRL_MASK                           0x3F032FFBUL                              /**< Mask for WTIMER_CTRL */
#define _WTIMER_CTRL_MODE_SHIFT                     0                                         /**< Shift value for TIMER_MODE */
#define _WTIMER_CTRL_MODE_MASK                      0x3UL                                     /**< Bit mask for TIMER_MODE */
#define _WTIMER_CTRL_MODE_DEFAULT                   0x00000000UL                              /**< Mode DEFAULT for WTIMER_CTRL */
#define _WTIMER_CTRL_MODE_UP                        0x00000000UL                              /**< Mode UP for WTIMER_CTRL */
#define _WTIMER_CTRL_MODE_DOWN                      0x00000001UL                              /**< Mode DOWN for WTIMER_CTRL */
#define _WTIMER_CTRL_MODE_UPDOWN                    0x00000002UL                              /**< Mode UPDOWN for WTIMER_CTRL */
#define _WTIMER_CTRL_MODE_QDEC                      0x00000003UL                              /**< Mode QDEC for WTIMER_CTRL */
#define WTIMER_CTRL_MODE_DEFAULT                    (_WTIMER_CTRL_MODE_DEFAULT << 0)          /**< Shifted mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_MODE_UP                         (_WTIMER_CTRL_MODE_UP << 0)               /**< Shifted mode UP for WTIMER_CTRL */
#define WTIMER_CTRL_MODE_DOWN                       (_WTIMER_CTRL_MODE_DOWN << 0)             /**< Shifted mode DOWN for WTIMER_CTRL */
#define WTIMER_CTRL_MODE_UPDOWN                     (_WTIMER_CTRL_MODE_UPDOWN << 0)           /**< Shifted mode UPDOWN for WTIMER_CTRL */
#define WTIMER_CTRL_MODE_QDEC                       (_WTIMER_CTRL_MODE_QDEC << 0)             /**< Shifted mode QDEC for WTIMER_CTRL */
#define WTIMER_CTRL_SYNC                            (0x1UL << 3)                              /**< Timer Start/Stop/Reload Synchronization */
#define _WTIMER_CTRL_SYNC_SHIFT                     3                                         /**< Shift value for TIMER_SYNC */
#define _WTIMER_CTRL_SYNC_MASK                      0x8UL                                     /**< Bit mask for TIMER_SYNC */
#define _WTIMER_CTRL_SYNC_DEFAULT                   0x00000000UL                              /**< Mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_SYNC_DEFAULT                    (_WTIMER_CTRL_SYNC_DEFAULT << 3)          /**< Shifted mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_OSMEN                           (0x1UL << 4)                              /**< One-shot Mode Enable */
#define _WTIMER_CTRL_OSMEN_SHIFT                    4                                         /**< Shift value for TIMER_OSMEN */
#define _WTIMER_CTRL_OSMEN_MASK                     0x10UL                                    /**< Bit mask for TIMER_OSMEN */
#define _WTIMER_CTRL_OSMEN_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_OSMEN_DEFAULT                   (_WTIMER_CTRL_OSMEN_DEFAULT << 4)         /**< Shifted mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_QDM                             (0x1UL << 5)                              /**< Quadrature Decoder Mode Selection */
#define _WTIMER_CTRL_QDM_SHIFT                      5                                         /**< Shift value for TIMER_QDM */
#define _WTIMER_CTRL_QDM_MASK                       0x20UL                                    /**< Bit mask for TIMER_QDM */
#define _WTIMER_CTRL_QDM_DEFAULT                    0x00000000UL                              /**< Mode DEFAULT for WTIMER_CTRL */
#define _WTIMER_CTRL_QDM_X2                         0x00000000UL                              /**< Mode X2 for WTIMER_CTRL */
#define _WTIMER_CTRL_QDM_X4                         0x00000001UL                              /**< Mode X4 for WTIMER_CTRL */
#define WTIMER_CTRL_QDM_DEFAULT                     (_WTIMER_CTRL_QDM_DEFAULT << 5)           /**< Shifted mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_QDM_X2                          (_WTIMER_CTRL_QDM_X2 << 5)                /**< Shifted mode X2 for WTIMER_CTRL */
#define WTIMER_CTRL_QDM_X4                          (_WTIMER_CTRL_QDM_X4 << 5)                /**< Shifted mode X4 for WTIMER_CTRL */
#define WTIMER_CTRL_DEBUGRUN                        (0x1UL << 6)                              /**< Debug Mode Run Enable */
#define _WTIMER_CTRL_DEBUGRUN_SHIFT                 6                                         /**< Shift value for TIMER_DEBUGRUN */
#define _WTIMER_CTRL_DEBUGRUN_MASK                  0x40UL                                    /**< Bit mask for TIMER_DEBUGRUN */
#define _WTIMER_CTRL_DEBUGRUN_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_DEBUGRUN_DEFAULT                (_WTIMER_CTRL_DEBUGRUN_DEFAULT << 6)      /**< Shifted mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_DMACLRACT                       (0x1UL << 7)                              /**< DMA Request Clear on Active */
#define _WTIMER_CTRL_DMACLRACT_SHIFT                7                                         /**< Shift value for TIMER_DMACLRACT */
#define _WTIMER_CTRL_DMACLRACT_MASK                 0x80UL                                    /**< Bit mask for TIMER_DMACLRACT */
#define _WTIMER_CTRL_DMACLRACT_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_DMACLRACT_DEFAULT               (_WTIMER_CTRL_DMACLRACT_DEFAULT << 7)     /**< Shifted mode DEFAULT for WTIMER_CTRL */
#define _WTIMER_CTRL_RISEA_SHIFT                    8                                         /**< Shift value for TIMER_RISEA */
#define _WTIMER_CTRL_RISEA_MASK                     0x300UL                                   /**< Bit mask for TIMER_RISEA */
#define _WTIMER_CTRL_RISEA_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for WTIMER_CTRL */
#define _WTIMER_CTRL_RISEA_NONE                     0x00000000UL                              /**< Mode NONE for WTIMER_CTRL */
#define _WTIMER_CTRL_RISEA_START                    0x00000001UL                              /**< Mode START for WTIMER_CTRL */
#define _WTIMER_CTRL_RISEA_STOP                     0x00000002UL                              /**< Mode STOP for WTIMER_CTRL */
#define _WTIMER_CTRL_RISEA_RELOADSTART              0x00000003UL                              /**< Mode RELOADSTART for WTIMER_CTRL */
#define WTIMER_CTRL_RISEA_DEFAULT                   (_WTIMER_CTRL_RISEA_DEFAULT << 8)         /**< Shifted mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_RISEA_NONE                      (_WTIMER_CTRL_RISEA_NONE << 8)            /**< Shifted mode NONE for WTIMER_CTRL */
#define WTIMER_CTRL_RISEA_START                     (_WTIMER_CTRL_RISEA_START << 8)           /**< Shifted mode START for WTIMER_CTRL */
#define WTIMER_CTRL_RISEA_STOP                      (_WTIMER_CTRL_RISEA_STOP << 8)            /**< Shifted mode STOP for WTIMER_CTRL */
#define WTIMER_CTRL_RISEA_RELOADSTART               (_WTIMER_CTRL_RISEA_RELOADSTART << 8)     /**< Shifted mode RELOADSTART for WTIMER_CTRL */
#define _WTIMER_CTRL_FALLA_SHIFT                    10                                        /**< Shift value for TIMER_FALLA */
#define _WTIMER_CTRL_FALLA_MASK                     0xC00UL                                   /**< Bit mask for TIMER_FALLA */
#define _WTIMER_CTRL_FALLA_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for WTIMER_CTRL */
#define _WTIMER_CTRL_FALLA_NONE                     0x00000000UL                              /**< Mode NONE for WTIMER_CTRL */
#define _WTIMER_CTRL_FALLA_START                    0x00000001UL                              /**< Mode START for WTIMER_CTRL */
#define _WTIMER_CTRL_FALLA_STOP                     0x00000002UL                              /**< Mode STOP for WTIMER_CTRL */
#define _WTIMER_CTRL_FALLA_RELOADSTART              0x00000003UL                              /**< Mode RELOADSTART for WTIMER_CTRL */
#define WTIMER_CTRL_FALLA_DEFAULT                   (_WTIMER_CTRL_FALLA_DEFAULT << 10)        /**< Shifted mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_FALLA_NONE                      (_WTIMER_CTRL_FALLA_NONE << 10)           /**< Shifted mode NONE for WTIMER_CTRL */
#define WTIMER_CTRL_FALLA_START                     (_WTIMER_CTRL_FALLA_START << 10)          /**< Shifted mode START for WTIMER_CTRL */
#define WTIMER_CTRL_FALLA_STOP                      (_WTIMER_CTRL_FALLA_STOP << 10)           /**< Shifted mode STOP for WTIMER_CTRL */
#define WTIMER_CTRL_FALLA_RELOADSTART               (_WTIMER_CTRL_FALLA_RELOADSTART << 10)    /**< Shifted mode RELOADSTART for WTIMER_CTRL */
#define WTIMER_CTRL_X2CNT                           (0x1UL << 13)                             /**< 2x Count Mode */
#define _WTIMER_CTRL_X2CNT_SHIFT                    13                                        /**< Shift value for TIMER_X2CNT */
#define _WTIMER_CTRL_X2CNT_MASK                     0x2000UL                                  /**< Bit mask for TIMER_X2CNT */
#define _WTIMER_CTRL_X2CNT_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_X2CNT_DEFAULT                   (_WTIMER_CTRL_X2CNT_DEFAULT << 13)        /**< Shifted mode DEFAULT for WTIMER_CTRL */
#define _WTIMER_CTRL_CLKSEL_SHIFT                   16                                        /**< Shift value for TIMER_CLKSEL */
#define _WTIMER_CTRL_CLKSEL_MASK                    0x30000UL                                 /**< Bit mask for TIMER_CLKSEL */
#define _WTIMER_CTRL_CLKSEL_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for WTIMER_CTRL */
#define _WTIMER_CTRL_CLKSEL_PRESCHFPERCLK           0x00000000UL                              /**< Mode PRESCHFPERCLK for WTIMER_CTRL */
#define _WTIMER_CTRL_CLKSEL_CC1                     0x00000001UL                              /**< Mode CC1 for WTIMER_CTRL */
#define _WTIMER_CTRL_CLKSEL_TIMEROUF                0x00000002UL                              /**< Mode TIMEROUF for WTIMER_CTRL */
#define WTIMER_CTRL_CLKSEL_DEFAULT                  (_WTIMER_CTRL_CLKSEL_DEFAULT << 16)       /**< Shifted mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_CLKSEL_PRESCHFPERCLK            (_WTIMER_CTRL_CLKSEL_PRESCHFPERCLK << 16) /**< Shifted mode PRESCHFPERCLK for WTIMER_CTRL */
#define WTIMER_CTRL_CLKSEL_CC1                      (_WTIMER_CTRL_CLKSEL_CC1 << 16)           /**< Shifted mode CC1 for WTIMER_CTRL */
#define WTIMER_CTRL_CLKSEL_TIMEROUF                 (_WTIMER_CTRL_CLKSEL_TIMEROUF << 16)      /**< Shifted mode TIMEROUF for WTIMER_CTRL */
#define _WTIMER_CTRL_PRESC_SHIFT                    24                                        /**< Shift value for TIMER_PRESC */
#define _WTIMER_CTRL_PRESC_MASK                     0xF000000UL                               /**< Bit mask for TIMER_PRESC */
#define _WTIMER_CTRL_PRESC_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for WTIMER_CTRL */
#define _WTIMER_CTRL_PRESC_DIV1                     0x00000000UL                              /**< Mode DIV1 for WTIMER_CTRL */
#define _WTIMER_CTRL_PRESC_DIV2                     0x00000001UL                              /**< Mode DIV2 for WTIMER_CTRL */
#define _WTIMER_CTRL_PRESC_DIV4                     0x00000002UL                              /**< Mode DIV4 for WTIMER_CTRL */
#define _WTIMER_CTRL_PRESC_DIV8                     0x00000003UL                              /**< Mode DIV8 for WTIMER_CTRL */
#define _WTIMER_CTRL_PRESC_DIV16                    0x00000004UL                              /**< Mode DIV16 for WTIMER_CTRL */
#define _WTIMER_CTRL_PRESC_DIV32                    0x00000005UL                              /**< Mode DIV32 for WTIMER_CTRL */
#define _WTIMER_CTRL_PRESC_DIV64                    0x00000006UL                              /**< Mode DIV64 for WTIMER_CTRL */
#define _WTIMER_CTRL_PRESC_DIV128                   0x00000007UL                              /**< Mode DIV128 for WTIMER_CTRL */
#define _WTIMER_CTRL_PRESC_DIV256                   0x00000008UL                              /**< Mode DIV256 for WTIMER_CTRL */
#define _WTIMER_CTRL_PRESC_DIV512                   0x00000009UL                              /**< Mode DIV512 for WTIMER_CTRL */
#define _WTIMER_CTRL_PRESC_DIV1024                  0x0000000AUL                              /**< Mode DIV1024 for WTIMER_CTRL */
#define WTIMER_CTRL_PRESC_DEFAULT                   (_WTIMER_CTRL_PRESC_DEFAULT << 24)        /**< Shifted mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_PRESC_DIV1                      (_WTIMER_CTRL_PRESC_DIV1 << 24)           /**< Shifted mode DIV1 for WTIMER_CTRL */
#define WTIMER_CTRL_PRESC_DIV2                      (_WTIMER_CTRL_PRESC_DIV2 << 24)           /**< Shifted mode DIV2 for WTIMER_CTRL */
#define WTIMER_CTRL_PRESC_DIV4                      (_WTIMER_CTRL_PRESC_DIV4 << 24)           /**< Shifted mode DIV4 for WTIMER_CTRL */
#define WTIMER_CTRL_PRESC_DIV8                      (_WTIMER_CTRL_PRESC_DIV8 << 24)           /**< Shifted mode DIV8 for WTIMER_CTRL */
#define WTIMER_CTRL_PRESC_DIV16                     (_WTIMER_CTRL_PRESC_DIV16 << 24)          /**< Shifted mode DIV16 for WTIMER_CTRL */
#define WTIMER_CTRL_PRESC_DIV32                     (_WTIMER_CTRL_PRESC_DIV32 << 24)          /**< Shifted mode DIV32 for WTIMER_CTRL */
#define WTIMER_CTRL_PRESC_DIV64                     (_WTIMER_CTRL_PRESC_DIV64 << 24)          /**< Shifted mode DIV64 for WTIMER_CTRL */
#define WTIMER_CTRL_PRESC_DIV128                    (_WTIMER_CTRL_PRESC_DIV128 << 24)         /**< Shifted mode DIV128 for WTIMER_CTRL */
#define WTIMER_CTRL_PRESC_DIV256                    (_WTIMER_CTRL_PRESC_DIV256 << 24)         /**< Shifted mode DIV256 for WTIMER_CTRL */
#define WTIMER_CTRL_PRESC_DIV512                    (_WTIMER_CTRL_PRESC_DIV512 << 24)         /**< Shifted mode DIV512 for WTIMER_CTRL */
#define WTIMER_CTRL_PRESC_DIV1024                   (_WTIMER_CTRL_PRESC_DIV1024 << 24)        /**< Shifted mode DIV1024 for WTIMER_CTRL */
#define WTIMER_CTRL_ATI                             (0x1UL << 28)                             /**< Always Track Inputs */
#define _WTIMER_CTRL_ATI_SHIFT                      28                                        /**< Shift value for TIMER_ATI */
#define _WTIMER_CTRL_ATI_MASK                       0x10000000UL                              /**< Bit mask for TIMER_ATI */
#define _WTIMER_CTRL_ATI_DEFAULT                    0x00000000UL                              /**< Mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_ATI_DEFAULT                     (_WTIMER_CTRL_ATI_DEFAULT << 28)          /**< Shifted mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_RSSCOIST                        (0x1UL << 29)                             /**< Reload-Start Sets Compare Output Initial State */
#define _WTIMER_CTRL_RSSCOIST_SHIFT                 29                                        /**< Shift value for TIMER_RSSCOIST */
#define _WTIMER_CTRL_RSSCOIST_MASK                  0x20000000UL                              /**< Bit mask for TIMER_RSSCOIST */
#define _WTIMER_CTRL_RSSCOIST_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for WTIMER_CTRL */
#define WTIMER_CTRL_RSSCOIST_DEFAULT                (_WTIMER_CTRL_RSSCOIST_DEFAULT << 29)     /**< Shifted mode DEFAULT for WTIMER_CTRL */

/* Bit fields for WTIMER CMD */
#define _WTIMER_CMD_RESETVALUE                      0x00000000UL                     /**< Default value for WTIMER_CMD */
#define _WTIMER_CMD_MASK                            0x00000003UL                     /**< Mask for WTIMER_CMD */
#define WTIMER_CMD_START                            (0x1UL << 0)                     /**< Start Timer */
#define _WTIMER_CMD_START_SHIFT                     0                                /**< Shift value for TIMER_START */
#define _WTIMER_CMD_START_MASK                      0x1UL                            /**< Bit mask for TIMER_START */
#define _WTIMER_CMD_START_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for WTIMER_CMD */
#define WTIMER_CMD_START_DEFAULT                    (_WTIMER_CMD_START_DEFAULT << 0) /**< Shifted mode DEFAULT for WTIMER_CMD */
#define WTIMER_CMD_STOP                             (0x1UL << 1)                     /**< Stop Timer */
#define _WTIMER_CMD_STOP_SHIFT                      1                                /**< Shift value for TIMER_STOP */
#define _WTIMER_CMD_STOP_MASK                       0x2UL                            /**< Bit mask for TIMER_STOP */
#define _WTIMER_CMD_STOP_DEFAULT                    0x00000000UL                     /**< Mode DEFAULT for WTIMER_CMD */
#define WTIMER_CMD_STOP_DEFAULT                     (_WTIMER_CMD_STOP_DEFAULT << 1)  /**< Shifted mode DEFAULT for WTIMER_CMD */

/* Bit fields for WTIMER STATUS */
#define _WTIMER_STATUS_RESETVALUE                   0x00000000UL                           /**< Default value for WTIMER_STATUS */
#define _WTIMER_STATUS_MASK                         0x0F0F0F07UL                           /**< Mask for WTIMER_STATUS */
#define WTIMER_STATUS_RUNNING                       (0x1UL << 0)                           /**< Running */
#define _WTIMER_STATUS_RUNNING_SHIFT                0                                      /**< Shift value for TIMER_RUNNING */
#define _WTIMER_STATUS_RUNNING_MASK                 0x1UL                                  /**< Bit mask for TIMER_RUNNING */
#define _WTIMER_STATUS_RUNNING_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_RUNNING_DEFAULT               (_WTIMER_STATUS_RUNNING_DEFAULT << 0)  /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_DIR                           (0x1UL << 1)                           /**< Direction */
#define _WTIMER_STATUS_DIR_SHIFT                    1                                      /**< Shift value for TIMER_DIR */
#define _WTIMER_STATUS_DIR_MASK                     0x2UL                                  /**< Bit mask for TIMER_DIR */
#define _WTIMER_STATUS_DIR_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define _WTIMER_STATUS_DIR_UP                       0x00000000UL                           /**< Mode UP for WTIMER_STATUS */
#define _WTIMER_STATUS_DIR_DOWN                     0x00000001UL                           /**< Mode DOWN for WTIMER_STATUS */
#define WTIMER_STATUS_DIR_DEFAULT                   (_WTIMER_STATUS_DIR_DEFAULT << 1)      /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_DIR_UP                        (_WTIMER_STATUS_DIR_UP << 1)           /**< Shifted mode UP for WTIMER_STATUS */
#define WTIMER_STATUS_DIR_DOWN                      (_WTIMER_STATUS_DIR_DOWN << 1)         /**< Shifted mode DOWN for WTIMER_STATUS */
#define WTIMER_STATUS_TOPBV                         (0x1UL << 2)                           /**< TOPB Valid */
#define _WTIMER_STATUS_TOPBV_SHIFT                  2                                      /**< Shift value for TIMER_TOPBV */
#define _WTIMER_STATUS_TOPBV_MASK                   0x4UL                                  /**< Bit mask for TIMER_TOPBV */
#define _WTIMER_STATUS_TOPBV_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_TOPBV_DEFAULT                 (_WTIMER_STATUS_TOPBV_DEFAULT << 2)    /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_CCVBV0                        (0x1UL << 8)                           /**< CC0 CCVB Valid */
#define _WTIMER_STATUS_CCVBV0_SHIFT                 8                                      /**< Shift value for TIMER_CCVBV0 */
#define _WTIMER_STATUS_CCVBV0_MASK                  0x100UL                                /**< Bit mask for TIMER_CCVBV0 */
#define _WTIMER_STATUS_CCVBV0_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_CCVBV0_DEFAULT                (_WTIMER_STATUS_CCVBV0_DEFAULT << 8)   /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_CCVBV1                        (0x1UL << 9)                           /**< CC1 CCVB Valid */
#define _WTIMER_STATUS_CCVBV1_SHIFT                 9                                      /**< Shift value for TIMER_CCVBV1 */
#define _WTIMER_STATUS_CCVBV1_MASK                  0x200UL                                /**< Bit mask for TIMER_CCVBV1 */
#define _WTIMER_STATUS_CCVBV1_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_CCVBV1_DEFAULT                (_WTIMER_STATUS_CCVBV1_DEFAULT << 9)   /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_CCVBV2                        (0x1UL << 10)                          /**< CC2 CCVB Valid */
#define _WTIMER_STATUS_CCVBV2_SHIFT                 10                                     /**< Shift value for TIMER_CCVBV2 */
#define _WTIMER_STATUS_CCVBV2_MASK                  0x400UL                                /**< Bit mask for TIMER_CCVBV2 */
#define _WTIMER_STATUS_CCVBV2_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_CCVBV2_DEFAULT                (_WTIMER_STATUS_CCVBV2_DEFAULT << 10)  /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_CCVBV3                        (0x1UL << 11)                          /**< CC3 CCVB Valid */
#define _WTIMER_STATUS_CCVBV3_SHIFT                 11                                     /**< Shift value for TIMER_CCVBV3 */
#define _WTIMER_STATUS_CCVBV3_MASK                  0x800UL                                /**< Bit mask for TIMER_CCVBV3 */
#define _WTIMER_STATUS_CCVBV3_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_CCVBV3_DEFAULT                (_WTIMER_STATUS_CCVBV3_DEFAULT << 11)  /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_ICV0                          (0x1UL << 16)                          /**< CC0 Input Capture Valid */
#define _WTIMER_STATUS_ICV0_SHIFT                   16                                     /**< Shift value for TIMER_ICV0 */
#define _WTIMER_STATUS_ICV0_MASK                    0x10000UL                              /**< Bit mask for TIMER_ICV0 */
#define _WTIMER_STATUS_ICV0_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_ICV0_DEFAULT                  (_WTIMER_STATUS_ICV0_DEFAULT << 16)    /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_ICV1                          (0x1UL << 17)                          /**< CC1 Input Capture Valid */
#define _WTIMER_STATUS_ICV1_SHIFT                   17                                     /**< Shift value for TIMER_ICV1 */
#define _WTIMER_STATUS_ICV1_MASK                    0x20000UL                              /**< Bit mask for TIMER_ICV1 */
#define _WTIMER_STATUS_ICV1_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_ICV1_DEFAULT                  (_WTIMER_STATUS_ICV1_DEFAULT << 17)    /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_ICV2                          (0x1UL << 18)                          /**< CC2 Input Capture Valid */
#define _WTIMER_STATUS_ICV2_SHIFT                   18                                     /**< Shift value for TIMER_ICV2 */
#define _WTIMER_STATUS_ICV2_MASK                    0x40000UL                              /**< Bit mask for TIMER_ICV2 */
#define _WTIMER_STATUS_ICV2_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_ICV2_DEFAULT                  (_WTIMER_STATUS_ICV2_DEFAULT << 18)    /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_ICV3                          (0x1UL << 19)                          /**< CC3 Input Capture Valid */
#define _WTIMER_STATUS_ICV3_SHIFT                   19                                     /**< Shift value for TIMER_ICV3 */
#define _WTIMER_STATUS_ICV3_MASK                    0x80000UL                              /**< Bit mask for TIMER_ICV3 */
#define _WTIMER_STATUS_ICV3_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_ICV3_DEFAULT                  (_WTIMER_STATUS_ICV3_DEFAULT << 19)    /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL0                        (0x1UL << 24)                          /**< CC0 Polarity */
#define _WTIMER_STATUS_CCPOL0_SHIFT                 24                                     /**< Shift value for TIMER_CCPOL0 */
#define _WTIMER_STATUS_CCPOL0_MASK                  0x1000000UL                            /**< Bit mask for TIMER_CCPOL0 */
#define _WTIMER_STATUS_CCPOL0_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define _WTIMER_STATUS_CCPOL0_LOWRISE               0x00000000UL                           /**< Mode LOWRISE for WTIMER_STATUS */
#define _WTIMER_STATUS_CCPOL0_HIGHFALL              0x00000001UL                           /**< Mode HIGHFALL for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL0_DEFAULT                (_WTIMER_STATUS_CCPOL0_DEFAULT << 24)  /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL0_LOWRISE                (_WTIMER_STATUS_CCPOL0_LOWRISE << 24)  /**< Shifted mode LOWRISE for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL0_HIGHFALL               (_WTIMER_STATUS_CCPOL0_HIGHFALL << 24) /**< Shifted mode HIGHFALL for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL1                        (0x1UL << 25)                          /**< CC1 Polarity */
#define _WTIMER_STATUS_CCPOL1_SHIFT                 25                                     /**< Shift value for TIMER_CCPOL1 */
#define _WTIMER_STATUS_CCPOL1_MASK                  0x2000000UL                            /**< Bit mask for TIMER_CCPOL1 */
#define _WTIMER_STATUS_CCPOL1_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define _WTIMER_STATUS_CCPOL1_LOWRISE               0x00000000UL                           /**< Mode LOWRISE for WTIMER_STATUS */
#define _WTIMER_STATUS_CCPOL1_HIGHFALL              0x00000001UL                           /**< Mode HIGHFALL for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL1_DEFAULT                (_WTIMER_STATUS_CCPOL1_DEFAULT << 25)  /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL1_LOWRISE                (_WTIMER_STATUS_CCPOL1_LOWRISE << 25)  /**< Shifted mode LOWRISE for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL1_HIGHFALL               (_WTIMER_STATUS_CCPOL1_HIGHFALL << 25) /**< Shifted mode HIGHFALL for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL2                        (0x1UL << 26)                          /**< CC2 Polarity */
#define _WTIMER_STATUS_CCPOL2_SHIFT                 26                                     /**< Shift value for TIMER_CCPOL2 */
#define _WTIMER_STATUS_CCPOL2_MASK                  0x4000000UL                            /**< Bit mask for TIMER_CCPOL2 */
#define _WTIMER_STATUS_CCPOL2_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define _WTIMER_STATUS_CCPOL2_LOWRISE               0x00000000UL                           /**< Mode LOWRISE for WTIMER_STATUS */
#define _WTIMER_STATUS_CCPOL2_HIGHFALL              0x00000001UL                           /**< Mode HIGHFALL for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL2_DEFAULT                (_WTIMER_STATUS_CCPOL2_DEFAULT << 26)  /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL2_LOWRISE                (_WTIMER_STATUS_CCPOL2_LOWRISE << 26)  /**< Shifted mode LOWRISE for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL2_HIGHFALL               (_WTIMER_STATUS_CCPOL2_HIGHFALL << 26) /**< Shifted mode HIGHFALL for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL3                        (0x1UL << 27)                          /**< CC3 Polarity */
#define _WTIMER_STATUS_CCPOL3_SHIFT                 27                                     /**< Shift value for TIMER_CCPOL3 */
#define _WTIMER_STATUS_CCPOL3_MASK                  0x8000000UL                            /**< Bit mask for TIMER_CCPOL3 */
#define _WTIMER_STATUS_CCPOL3_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for WTIMER_STATUS */
#define _WTIMER_STATUS_CCPOL3_LOWRISE               0x00000000UL                           /**< Mode LOWRISE for WTIMER_STATUS */
#define _WTIMER_STATUS_CCPOL3_HIGHFALL              0x00000001UL                           /**< Mode HIGHFALL for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL3_DEFAULT                (_WTIMER_STATUS_CCPOL3_DEFAULT << 27)  /**< Shifted mode DEFAULT for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL3_LOWRISE                (_WTIMER_STATUS_CCPOL3_LOWRISE << 27)  /**< Shifted mode LOWRISE for WTIMER_STATUS */
#define WTIMER_STATUS_CCPOL3_HIGHFALL               (_WTIMER_STATUS_CCPOL3_HIGHFALL << 27) /**< Shifted mode HIGHFALL for WTIMER_STATUS */

/* Bit fields for WTIMER IF */
#define _WTIMER_IF_RESETVALUE                       0x00000000UL                      /**< Default value for WTIMER_IF */
#define _WTIMER_IF_MASK                             0x00000FF7UL                      /**< Mask for WTIMER_IF */
#define WTIMER_IF_OF                                (0x1UL << 0)                      /**< Overflow Interrupt Flag */
#define _WTIMER_IF_OF_SHIFT                         0                                 /**< Shift value for TIMER_OF */
#define _WTIMER_IF_OF_MASK                          0x1UL                             /**< Bit mask for TIMER_OF */
#define _WTIMER_IF_OF_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_OF_DEFAULT                        (_WTIMER_IF_OF_DEFAULT << 0)      /**< Shifted mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_UF                                (0x1UL << 1)                      /**< Underflow Interrupt Flag */
#define _WTIMER_IF_UF_SHIFT                         1                                 /**< Shift value for TIMER_UF */
#define _WTIMER_IF_UF_MASK                          0x2UL                             /**< Bit mask for TIMER_UF */
#define _WTIMER_IF_UF_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_UF_DEFAULT                        (_WTIMER_IF_UF_DEFAULT << 1)      /**< Shifted mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_DIRCHG                            (0x1UL << 2)                      /**< Direction Change Detect Interrupt Flag */
#define _WTIMER_IF_DIRCHG_SHIFT                     2                                 /**< Shift value for TIMER_DIRCHG */
#define _WTIMER_IF_DIRCHG_MASK                      0x4UL                             /**< Bit mask for TIMER_DIRCHG */
#define _WTIMER_IF_DIRCHG_DEFAULT                   0x00000000UL                      /**< Mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_DIRCHG_DEFAULT                    (_WTIMER_IF_DIRCHG_DEFAULT << 2)  /**< Shifted mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_CC0                               (0x1UL << 4)                      /**< CC Channel 0 Interrupt Flag */
#define _WTIMER_IF_CC0_SHIFT                        4                                 /**< Shift value for TIMER_CC0 */
#define _WTIMER_IF_CC0_MASK                         0x10UL                            /**< Bit mask for TIMER_CC0 */
#define _WTIMER_IF_CC0_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_CC0_DEFAULT                       (_WTIMER_IF_CC0_DEFAULT << 4)     /**< Shifted mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_CC1                               (0x1UL << 5)                      /**< CC Channel 1 Interrupt Flag */
#define _WTIMER_IF_CC1_SHIFT                        5                                 /**< Shift value for TIMER_CC1 */
#define _WTIMER_IF_CC1_MASK                         0x20UL                            /**< Bit mask for TIMER_CC1 */
#define _WTIMER_IF_CC1_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_CC1_DEFAULT                       (_WTIMER_IF_CC1_DEFAULT << 5)     /**< Shifted mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_CC2                               (0x1UL << 6)                      /**< CC Channel 2 Interrupt Flag */
#define _WTIMER_IF_CC2_SHIFT                        6                                 /**< Shift value for TIMER_CC2 */
#define _WTIMER_IF_CC2_MASK                         0x40UL                            /**< Bit mask for TIMER_CC2 */
#define _WTIMER_IF_CC2_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_CC2_DEFAULT                       (_WTIMER_IF_CC2_DEFAULT << 6)     /**< Shifted mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_CC3                               (0x1UL << 7)                      /**< CC Channel 3 Interrupt Flag */
#define _WTIMER_IF_CC3_SHIFT                        7                                 /**< Shift value for TIMER_CC3 */
#define _WTIMER_IF_CC3_MASK                         0x80UL                            /**< Bit mask for TIMER_CC3 */
#define _WTIMER_IF_CC3_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_CC3_DEFAULT                       (_WTIMER_IF_CC3_DEFAULT << 7)     /**< Shifted mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_ICBOF0                            (0x1UL << 8)                      /**< CC Channel 0 Input Capture Buffer Overflow Interrupt Flag */
#define _WTIMER_IF_ICBOF0_SHIFT                     8                                 /**< Shift value for TIMER_ICBOF0 */
#define _WTIMER_IF_ICBOF0_MASK                      0x100UL                           /**< Bit mask for TIMER_ICBOF0 */
#define _WTIMER_IF_ICBOF0_DEFAULT                   0x00000000UL                      /**< Mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_ICBOF0_DEFAULT                    (_WTIMER_IF_ICBOF0_DEFAULT << 8)  /**< Shifted mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_ICBOF1                            (0x1UL << 9)                      /**< CC Channel 1 Input Capture Buffer Overflow Interrupt Flag */
#define _WTIMER_IF_ICBOF1_SHIFT                     9                                 /**< Shift value for TIMER_ICBOF1 */
#define _WTIMER_IF_ICBOF1_MASK                      0x200UL                           /**< Bit mask for TIMER_ICBOF1 */
#define _WTIMER_IF_ICBOF1_DEFAULT                   0x00000000UL                      /**< Mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_ICBOF1_DEFAULT                    (_WTIMER_IF_ICBOF1_DEFAULT << 9)  /**< Shifted mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_ICBOF2                            (0x1UL << 10)                     /**< CC Channel 2 Input Capture Buffer Overflow Interrupt Flag */
#define _WTIMER_IF_ICBOF2_SHIFT                     10                                /**< Shift value for TIMER_ICBOF2 */
#define _WTIMER_IF_ICBOF2_MASK                      0x400UL                           /**< Bit mask for TIMER_ICBOF2 */
#define _WTIMER_IF_ICBOF2_DEFAULT                   0x00000000UL                      /**< Mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_ICBOF2_DEFAULT                    (_WTIMER_IF_ICBOF2_DEFAULT << 10) /**< Shifted mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_ICBOF3                            (0x1UL << 11)                     /**< CC Channel 3 Input Capture Buffer Overflow Interrupt Flag */
#define _WTIMER_IF_ICBOF3_SHIFT                     11                                /**< Shift value for TIMER_ICBOF3 */
#define _WTIMER_IF_ICBOF3_MASK                      0x800UL                           /**< Bit mask for TIMER_ICBOF3 */
#define _WTIMER_IF_ICBOF3_DEFAULT                   0x00000000UL                      /**< Mode DEFAULT for WTIMER_IF */
#define WTIMER_IF_ICBOF3_DEFAULT                    (_WTIMER_IF_ICBOF3_DEFAULT << 11) /**< Shifted mode DEFAULT for WTIMER_IF */

/* Bit fields for WTIMER IFS */
#define _WTIMER_IFS_RESETVALUE                      0x00000000UL                       /**< Default value for WTIMER_IFS */
#define _WTIMER_IFS_MASK                            0x00000FF7UL                       /**< Mask for WTIMER_IFS */
#define WTIMER_IFS_OF                               (0x1UL << 0)                       /**< Set OF Interrupt Flag */
#define _WTIMER_IFS_OF_SHIFT                        0                                  /**< Shift value for TIMER_OF */
#define _WTIMER_IFS_OF_MASK                         0x1UL                              /**< Bit mask for TIMER_OF */
#define _WTIMER_IFS_OF_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_OF_DEFAULT                       (_WTIMER_IFS_OF_DEFAULT << 0)      /**< Shifted mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_UF                               (0x1UL << 1)                       /**< Set UF Interrupt Flag */
#define _WTIMER_IFS_UF_SHIFT                        1                                  /**< Shift value for TIMER_UF */
#define _WTIMER_IFS_UF_MASK                         0x2UL                              /**< Bit mask for TIMER_UF */
#define _WTIMER_IFS_UF_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_UF_DEFAULT                       (_WTIMER_IFS_UF_DEFAULT << 1)      /**< Shifted mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_DIRCHG                           (0x1UL << 2)                       /**< Set DIRCHG Interrupt Flag */
#define _WTIMER_IFS_DIRCHG_SHIFT                    2                                  /**< Shift value for TIMER_DIRCHG */
#define _WTIMER_IFS_DIRCHG_MASK                     0x4UL                              /**< Bit mask for TIMER_DIRCHG */
#define _WTIMER_IFS_DIRCHG_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_DIRCHG_DEFAULT                   (_WTIMER_IFS_DIRCHG_DEFAULT << 2)  /**< Shifted mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_CC0                              (0x1UL << 4)                       /**< Set CC0 Interrupt Flag */
#define _WTIMER_IFS_CC0_SHIFT                       4                                  /**< Shift value for TIMER_CC0 */
#define _WTIMER_IFS_CC0_MASK                        0x10UL                             /**< Bit mask for TIMER_CC0 */
#define _WTIMER_IFS_CC0_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_CC0_DEFAULT                      (_WTIMER_IFS_CC0_DEFAULT << 4)     /**< Shifted mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_CC1                              (0x1UL << 5)                       /**< Set CC1 Interrupt Flag */
#define _WTIMER_IFS_CC1_SHIFT                       5                                  /**< Shift value for TIMER_CC1 */
#define _WTIMER_IFS_CC1_MASK                        0x20UL                             /**< Bit mask for TIMER_CC1 */
#define _WTIMER_IFS_CC1_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_CC1_DEFAULT                      (_WTIMER_IFS_CC1_DEFAULT << 5)     /**< Shifted mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_CC2                              (0x1UL << 6)                       /**< Set CC2 Interrupt Flag */
#define _WTIMER_IFS_CC2_SHIFT                       6                                  /**< Shift value for TIMER_CC2 */
#define _WTIMER_IFS_CC2_MASK                        0x40UL                             /**< Bit mask for TIMER_CC2 */
#define _WTIMER_IFS_CC2_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_CC2_DEFAULT                      (_WTIMER_IFS_CC2_DEFAULT << 6)     /**< Shifted mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_CC3                              (0x1UL << 7)                       /**< Set CC3 Interrupt Flag */
#define _WTIMER_IFS_CC3_SHIFT                       7                                  /**< Shift value for TIMER_CC3 */
#define _WTIMER_IFS_CC3_MASK                        0x80UL                             /**< Bit mask for TIMER_CC3 */
#define _WTIMER_IFS_CC3_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_CC3_DEFAULT                      (_WTIMER_IFS_CC3_DEFAULT << 7)     /**< Shifted mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_ICBOF0                           (0x1UL << 8)                       /**< Set ICBOF0 Interrupt Flag */
#define _WTIMER_IFS_ICBOF0_SHIFT                    8                                  /**< Shift value for TIMER_ICBOF0 */
#define _WTIMER_IFS_ICBOF0_MASK                     0x100UL                            /**< Bit mask for TIMER_ICBOF0 */
#define _WTIMER_IFS_ICBOF0_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_ICBOF0_DEFAULT                   (_WTIMER_IFS_ICBOF0_DEFAULT << 8)  /**< Shifted mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_ICBOF1                           (0x1UL << 9)                       /**< Set ICBOF1 Interrupt Flag */
#define _WTIMER_IFS_ICBOF1_SHIFT                    9                                  /**< Shift value for TIMER_ICBOF1 */
#define _WTIMER_IFS_ICBOF1_MASK                     0x200UL                            /**< Bit mask for TIMER_ICBOF1 */
#define _WTIMER_IFS_ICBOF1_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_ICBOF1_DEFAULT                   (_WTIMER_IFS_ICBOF1_DEFAULT << 9)  /**< Shifted mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_ICBOF2                           (0x1UL << 10)                      /**< Set ICBOF2 Interrupt Flag */
#define _WTIMER_IFS_ICBOF2_SHIFT                    10                                 /**< Shift value for TIMER_ICBOF2 */
#define _WTIMER_IFS_ICBOF2_MASK                     0x400UL                            /**< Bit mask for TIMER_ICBOF2 */
#define _WTIMER_IFS_ICBOF2_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_ICBOF2_DEFAULT                   (_WTIMER_IFS_ICBOF2_DEFAULT << 10) /**< Shifted mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_ICBOF3                           (0x1UL << 11)                      /**< Set ICBOF3 Interrupt Flag */
#define _WTIMER_IFS_ICBOF3_SHIFT                    11                                 /**< Shift value for TIMER_ICBOF3 */
#define _WTIMER_IFS_ICBOF3_MASK                     0x800UL                            /**< Bit mask for TIMER_ICBOF3 */
#define _WTIMER_IFS_ICBOF3_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFS */
#define WTIMER_IFS_ICBOF3_DEFAULT                   (_WTIMER_IFS_ICBOF3_DEFAULT << 11) /**< Shifted mode DEFAULT for WTIMER_IFS */

/* Bit fields for WTIMER IFC */
#define _WTIMER_IFC_RESETVALUE                      0x00000000UL                       /**< Default value for WTIMER_IFC */
#define _WTIMER_IFC_MASK                            0x00000FF7UL                       /**< Mask for WTIMER_IFC */
#define WTIMER_IFC_OF                               (0x1UL << 0)                       /**< Clear OF Interrupt Flag */
#define _WTIMER_IFC_OF_SHIFT                        0                                  /**< Shift value for TIMER_OF */
#define _WTIMER_IFC_OF_MASK                         0x1UL                              /**< Bit mask for TIMER_OF */
#define _WTIMER_IFC_OF_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_OF_DEFAULT                       (_WTIMER_IFC_OF_DEFAULT << 0)      /**< Shifted mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_UF                               (0x1UL << 1)                       /**< Clear UF Interrupt Flag */
#define _WTIMER_IFC_UF_SHIFT                        1                                  /**< Shift value for TIMER_UF */
#define _WTIMER_IFC_UF_MASK                         0x2UL                              /**< Bit mask for TIMER_UF */
#define _WTIMER_IFC_UF_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_UF_DEFAULT                       (_WTIMER_IFC_UF_DEFAULT << 1)      /**< Shifted mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_DIRCHG                           (0x1UL << 2)                       /**< Clear DIRCHG Interrupt Flag */
#define _WTIMER_IFC_DIRCHG_SHIFT                    2                                  /**< Shift value for TIMER_DIRCHG */
#define _WTIMER_IFC_DIRCHG_MASK                     0x4UL                              /**< Bit mask for TIMER_DIRCHG */
#define _WTIMER_IFC_DIRCHG_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_DIRCHG_DEFAULT                   (_WTIMER_IFC_DIRCHG_DEFAULT << 2)  /**< Shifted mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_CC0                              (0x1UL << 4)                       /**< Clear CC0 Interrupt Flag */
#define _WTIMER_IFC_CC0_SHIFT                       4                                  /**< Shift value for TIMER_CC0 */
#define _WTIMER_IFC_CC0_MASK                        0x10UL                             /**< Bit mask for TIMER_CC0 */
#define _WTIMER_IFC_CC0_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_CC0_DEFAULT                      (_WTIMER_IFC_CC0_DEFAULT << 4)     /**< Shifted mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_CC1                              (0x1UL << 5)                       /**< Clear CC1 Interrupt Flag */
#define _WTIMER_IFC_CC1_SHIFT                       5                                  /**< Shift value for TIMER_CC1 */
#define _WTIMER_IFC_CC1_MASK                        0x20UL                             /**< Bit mask for TIMER_CC1 */
#define _WTIMER_IFC_CC1_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_CC1_DEFAULT                      (_WTIMER_IFC_CC1_DEFAULT << 5)     /**< Shifted mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_CC2                              (0x1UL << 6)                       /**< Clear CC2 Interrupt Flag */
#define _WTIMER_IFC_CC2_SHIFT                       6                                  /**< Shift value for TIMER_CC2 */
#define _WTIMER_IFC_CC2_MASK                        0x40UL                             /**< Bit mask for TIMER_CC2 */
#define _WTIMER_IFC_CC2_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_CC2_DEFAULT                      (_WTIMER_IFC_CC2_DEFAULT << 6)     /**< Shifted mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_CC3                              (0x1UL << 7)                       /**< Clear CC3 Interrupt Flag */
#define _WTIMER_IFC_CC3_SHIFT                       7                                  /**< Shift value for TIMER_CC3 */
#define _WTIMER_IFC_CC3_MASK                        0x80UL                             /**< Bit mask for TIMER_CC3 */
#define _WTIMER_IFC_CC3_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_CC3_DEFAULT                      (_WTIMER_IFC_CC3_DEFAULT << 7)     /**< Shifted mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_ICBOF0                           (0x1UL << 8)                       /**< Clear ICBOF0 Interrupt Flag */
#define _WTIMER_IFC_ICBOF0_SHIFT                    8                                  /**< Shift value for TIMER_ICBOF0 */
#define _WTIMER_IFC_ICBOF0_MASK                     0x100UL                            /**< Bit mask for TIMER_ICBOF0 */
#define _WTIMER_IFC_ICBOF0_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_ICBOF0_DEFAULT                   (_WTIMER_IFC_ICBOF0_DEFAULT << 8)  /**< Shifted mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_ICBOF1                           (0x1UL << 9)                       /**< Clear ICBOF1 Interrupt Flag */
#define _WTIMER_IFC_ICBOF1_SHIFT                    9                                  /**< Shift value for TIMER_ICBOF1 */
#define _WTIMER_IFC_ICBOF1_MASK                     0x200UL                            /**< Bit mask for TIMER_ICBOF1 */
#define _WTIMER_IFC_ICBOF1_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_ICBOF1_DEFAULT                   (_WTIMER_IFC_ICBOF1_DEFAULT << 9)  /**< Shifted mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_ICBOF2                           (0x1UL << 10)                      /**< Clear ICBOF2 Interrupt Flag */
#define _WTIMER_IFC_ICBOF2_SHIFT                    10                                 /**< Shift value for TIMER_ICBOF2 */
#define _WTIMER_IFC_ICBOF2_MASK                     0x400UL                            /**< Bit mask for TIMER_ICBOF2 */
#define _WTIMER_IFC_ICBOF2_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_ICBOF2_DEFAULT                   (_WTIMER_IFC_ICBOF2_DEFAULT << 10) /**< Shifted mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_ICBOF3                           (0x1UL << 11)                      /**< Clear ICBOF3 Interrupt Flag */
#define _WTIMER_IFC_ICBOF3_SHIFT                    11                                 /**< Shift value for TIMER_ICBOF3 */
#define _WTIMER_IFC_ICBOF3_MASK                     0x800UL                            /**< Bit mask for TIMER_ICBOF3 */
#define _WTIMER_IFC_ICBOF3_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IFC */
#define WTIMER_IFC_ICBOF3_DEFAULT                   (_WTIMER_IFC_ICBOF3_DEFAULT << 11) /**< Shifted mode DEFAULT for WTIMER_IFC */

/* Bit fields for WTIMER IEN */
#define _WTIMER_IEN_RESETVALUE                      0x00000000UL                       /**< Default value for WTIMER_IEN */
#define _WTIMER_IEN_MASK                            0x00000FF7UL                       /**< Mask for WTIMER_IEN */
#define WTIMER_IEN_OF                               (0x1UL << 0)                       /**< OF Interrupt Enable */
#define _WTIMER_IEN_OF_SHIFT                        0                                  /**< Shift value for TIMER_OF */
#define _WTIMER_IEN_OF_MASK                         0x1UL                              /**< Bit mask for TIMER_OF */
#define _WTIMER_IEN_OF_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_OF_DEFAULT                       (_WTIMER_IEN_OF_DEFAULT << 0)      /**< Shifted mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_UF                               (0x1UL << 1)                       /**< UF Interrupt Enable */
#define _WTIMER_IEN_UF_SHIFT                        1                                  /**< Shift value for TIMER_UF */
#define _WTIMER_IEN_UF_MASK                         0x2UL                              /**< Bit mask for TIMER_UF */
#define _WTIMER_IEN_UF_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_UF_DEFAULT                       (_WTIMER_IEN_UF_DEFAULT << 1)      /**< Shifted mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_DIRCHG                           (0x1UL << 2)                       /**< DIRCHG Interrupt Enable */
#define _WTIMER_IEN_DIRCHG_SHIFT                    2                                  /**< Shift value for TIMER_DIRCHG */
#define _WTIMER_IEN_DIRCHG_MASK                     0x4UL                              /**< Bit mask for TIMER_DIRCHG */
#define _WTIMER_IEN_DIRCHG_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_DIRCHG_DEFAULT                   (_WTIMER_IEN_DIRCHG_DEFAULT << 2)  /**< Shifted mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_CC0                              (0x1UL << 4)                       /**< CC0 Interrupt Enable */
#define _WTIMER_IEN_CC0_SHIFT                       4                                  /**< Shift value for TIMER_CC0 */
#define _WTIMER_IEN_CC0_MASK                        0x10UL                             /**< Bit mask for TIMER_CC0 */
#define _WTIMER_IEN_CC0_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_CC0_DEFAULT                      (_WTIMER_IEN_CC0_DEFAULT << 4)     /**< Shifted mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_CC1                              (0x1UL << 5)                       /**< CC1 Interrupt Enable */
#define _WTIMER_IEN_CC1_SHIFT                       5                                  /**< Shift value for TIMER_CC1 */
#define _WTIMER_IEN_CC1_MASK                        0x20UL                             /**< Bit mask for TIMER_CC1 */
#define _WTIMER_IEN_CC1_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_CC1_DEFAULT                      (_WTIMER_IEN_CC1_DEFAULT << 5)     /**< Shifted mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_CC2                              (0x1UL << 6)                       /**< CC2 Interrupt Enable */
#define _WTIMER_IEN_CC2_SHIFT                       6                                  /**< Shift value for TIMER_CC2 */
#define _WTIMER_IEN_CC2_MASK                        0x40UL                             /**< Bit mask for TIMER_CC2 */
#define _WTIMER_IEN_CC2_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_CC2_DEFAULT                      (_WTIMER_IEN_CC2_DEFAULT << 6)     /**< Shifted mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_CC3                              (0x1UL << 7)                       /**< CC3 Interrupt Enable */
#define _WTIMER_IEN_CC3_SHIFT                       7                                  /**< Shift value for TIMER_CC3 */
#define _WTIMER_IEN_CC3_MASK                        0x80UL                             /**< Bit mask for TIMER_CC3 */
#define _WTIMER_IEN_CC3_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_CC3_DEFAULT                      (_WTIMER_IEN_CC3_DEFAULT << 7)     /**< Shifted mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_ICBOF0                           (0x1UL << 8)                       /**< ICBOF0 Interrupt Enable */
#define _WTIMER_IEN_ICBOF0_SHIFT                    8                                  /**< Shift value for TIMER_ICBOF0 */
#define _WTIMER_IEN_ICBOF0_MASK                     0x100UL                            /**< Bit mask for TIMER_ICBOF0 */
#define _WTIMER_IEN_ICBOF0_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_ICBOF0_DEFAULT                   (_WTIMER_IEN_ICBOF0_DEFAULT << 8)  /**< Shifted mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_ICBOF1                           (0x1UL << 9)                       /**< ICBOF1 Interrupt Enable */
#define _WTIMER_IEN_ICBOF1_SHIFT                    9                                  /**< Shift value for TIMER_ICBOF1 */
#define _WTIMER_IEN_ICBOF1_MASK                     0x200UL                            /**< Bit mask for TIMER_ICBOF1 */
#define _WTIMER_IEN_ICBOF1_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_ICBOF1_DEFAULT                   (_WTIMER_IEN_ICBOF1_DEFAULT << 9)  /**< Shifted mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_ICBOF2                           (0x1UL << 10)                      /**< ICBOF2 Interrupt Enable */
#define _WTIMER_IEN_ICBOF2_SHIFT                    10                                 /**< Shift value for TIMER_ICBOF2 */
#define _WTIMER_IEN_ICBOF2_MASK                     0x400UL                            /**< Bit mask for TIMER_ICBOF2 */
#define _WTIMER_IEN_ICBOF2_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_ICBOF2_DEFAULT                   (_WTIMER_IEN_ICBOF2_DEFAULT << 10) /**< Shifted mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_ICBOF3                           (0x1UL << 11)                      /**< ICBOF3 Interrupt Enable */
#define _WTIMER_IEN_ICBOF3_SHIFT                    11                                 /**< Shift value for TIMER_ICBOF3 */
#define _WTIMER_IEN_ICBOF3_MASK                     0x800UL                            /**< Bit mask for TIMER_ICBOF3 */
#define _WTIMER_IEN_ICBOF3_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for WTIMER_IEN */
#define WTIMER_IEN_ICBOF3_DEFAULT                   (_WTIMER_IEN_ICBOF3_DEFAULT << 11) /**< Shifted mode DEFAULT for WTIMER_IEN */

/* Bit fields for WTIMER TOP */
#define _WTIMER_TOP_RESETVALUE                      0x0000FFFFUL                   /**< Default value for WTIMER_TOP */
#define _WTIMER_TOP_MASK                            0xFFFFFFFFUL                   /**< Mask for WTIMER_TOP */
#define _WTIMER_TOP_TOP_SHIFT                       0                              /**< Shift value for TIMER_TOP */
#define _WTIMER_TOP_TOP_MASK                        0xFFFFFFFFUL                   /**< Bit mask for TIMER_TOP */
#define _WTIMER_TOP_TOP_DEFAULT                     0x0000FFFFUL                   /**< Mode DEFAULT for WTIMER_TOP */
#define WTIMER_TOP_TOP_DEFAULT                      (_WTIMER_TOP_TOP_DEFAULT << 0) /**< Shifted mode DEFAULT for WTIMER_TOP */

/* Bit fields for WTIMER TOPB */
#define _WTIMER_TOPB_RESETVALUE                     0x00000000UL                     /**< Default value for WTIMER_TOPB */
#define _WTIMER_TOPB_MASK                           0xFFFFFFFFUL                     /**< Mask for WTIMER_TOPB */
#define _WTIMER_TOPB_TOPB_SHIFT                     0                                /**< Shift value for TIMER_TOPB */
#define _WTIMER_TOPB_TOPB_MASK                      0xFFFFFFFFUL                     /**< Bit mask for TIMER_TOPB */
#define _WTIMER_TOPB_TOPB_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for WTIMER_TOPB */
#define WTIMER_TOPB_TOPB_DEFAULT                    (_WTIMER_TOPB_TOPB_DEFAULT << 0) /**< Shifted mode DEFAULT for WTIMER_TOPB */

/* Bit fields for WTIMER CNT */
#define _WTIMER_CNT_RESETVALUE                      0x00000000UL                   /**< Default value for WTIMER_CNT */
#define _WTIMER_CNT_MASK                            0xFFFFFFFFUL                   /**< Mask for WTIMER_CNT */
#define _WTIMER_CNT_CNT_SHIFT                       0                              /**< Shift value for TIMER_CNT */
#define _WTIMER_CNT_CNT_MASK                        0xFFFFFFFFUL                   /**< Bit mask for TIMER_CNT */
#define _WTIMER_CNT_CNT_DEFAULT                     0x00000000UL                   /**< Mode DEFAULT for WTIMER_CNT */
#define WTIMER_CNT_CNT_DEFAULT                      (_WTIMER_CNT_CNT_DEFAULT << 0) /**< Shifted mode DEFAULT for WTIMER_CNT */

/* Bit fields for WTIMER LOCK */
#define _WTIMER_LOCK_RESETVALUE                     0x00000000UL                              /**< Default value for WTIMER_LOCK */
#define _WTIMER_LOCK_MASK                           0x0000FFFFUL                              /**< Mask for WTIMER_LOCK */
#define _WTIMER_LOCK_TIMERLOCKKEY_SHIFT             0                                         /**< Shift value for TIMER_TIMERLOCKKEY */
#define _WTIMER_LOCK_TIMERLOCKKEY_MASK              0xFFFFUL                                  /**< Bit mask for TIMER_TIMERLOCKKEY */
#define _WTIMER_LOCK_TIMERLOCKKEY_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for WTIMER_LOCK */
#define _WTIMER_LOCK_TIMERLOCKKEY_LOCK              0x00000000UL                              /**< Mode LOCK for WTIMER_LOCK */
#define _WTIMER_LOCK_TIMERLOCKKEY_UNLOCKED          0x00000000UL                              /**< Mode UNLOCKED for WTIMER_LOCK */
#define _WTIMER_LOCK_TIMERLOCKKEY_LOCKED            0x00000001UL                              /**< Mode LOCKED for WTIMER_LOCK */
#define _WTIMER_LOCK_TIMERLOCKKEY_UNLOCK            0x0000CE80UL                              /**< Mode UNLOCK for WTIMER_LOCK */
#define WTIMER_LOCK_TIMERLOCKKEY_DEFAULT            (_WTIMER_LOCK_TIMERLOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for WTIMER_LOCK */
#define WTIMER_LOCK_TIMERLOCKKEY_LOCK               (_WTIMER_LOCK_TIMERLOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for WTIMER_LOCK */
#define WTIMER_LOCK_TIMERLOCKKEY_UNLOCKED           (_WTIMER_LOCK_TIMERLOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for WTIMER_LOCK */
#define WTIMER_LOCK_TIMERLOCKKEY_LOCKED             (_WTIMER_LOCK_TIMERLOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for WTIMER_LOCK */
#define WTIMER_LOCK_TIMERLOCKKEY_UNLOCK             (_WTIMER_LOCK_TIMERLOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for WTIMER_LOCK */

/* Bit fields for WTIMER ROUTEPEN */
#define _WTIMER_ROUTEPEN_RESETVALUE                 0x00000000UL                              /**< Default value for WTIMER_ROUTEPEN */
#define _WTIMER_ROUTEPEN_MASK                       0x0000070FUL                              /**< Mask for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CC0PEN                      (0x1UL << 0)                              /**< CC Channel 0 Pin Enable */
#define _WTIMER_ROUTEPEN_CC0PEN_SHIFT               0                                         /**< Shift value for TIMER_CC0PEN */
#define _WTIMER_ROUTEPEN_CC0PEN_MASK                0x1UL                                     /**< Bit mask for TIMER_CC0PEN */
#define _WTIMER_ROUTEPEN_CC0PEN_DEFAULT             0x00000000UL                              /**< Mode DEFAULT for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CC0PEN_DEFAULT              (_WTIMER_ROUTEPEN_CC0PEN_DEFAULT << 0)    /**< Shifted mode DEFAULT for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CC1PEN                      (0x1UL << 1)                              /**< CC Channel 1 Pin Enable */
#define _WTIMER_ROUTEPEN_CC1PEN_SHIFT               1                                         /**< Shift value for TIMER_CC1PEN */
#define _WTIMER_ROUTEPEN_CC1PEN_MASK                0x2UL                                     /**< Bit mask for TIMER_CC1PEN */
#define _WTIMER_ROUTEPEN_CC1PEN_DEFAULT             0x00000000UL                              /**< Mode DEFAULT for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CC1PEN_DEFAULT              (_WTIMER_ROUTEPEN_CC1PEN_DEFAULT << 1)    /**< Shifted mode DEFAULT for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CC2PEN                      (0x1UL << 2)                              /**< CC Channel 2 Pin Enable */
#define _WTIMER_ROUTEPEN_CC2PEN_SHIFT               2                                         /**< Shift value for TIMER_CC2PEN */
#define _WTIMER_ROUTEPEN_CC2PEN_MASK                0x4UL                                     /**< Bit mask for TIMER_CC2PEN */
#define _WTIMER_ROUTEPEN_CC2PEN_DEFAULT             0x00000000UL                              /**< Mode DEFAULT for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CC2PEN_DEFAULT              (_WTIMER_ROUTEPEN_CC2PEN_DEFAULT << 2)    /**< Shifted mode DEFAULT for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CC3PEN                      (0x1UL << 3)                              /**< CC Channel 3 Pin Enable */
#define _WTIMER_ROUTEPEN_CC3PEN_SHIFT               3                                         /**< Shift value for TIMER_CC3PEN */
#define _WTIMER_ROUTEPEN_CC3PEN_MASK                0x8UL                                     /**< Bit mask for TIMER_CC3PEN */
#define _WTIMER_ROUTEPEN_CC3PEN_DEFAULT             0x00000000UL                              /**< Mode DEFAULT for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CC3PEN_DEFAULT              (_WTIMER_ROUTEPEN_CC3PEN_DEFAULT << 3)    /**< Shifted mode DEFAULT for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CDTI0PEN                    (0x1UL << 8)                              /**< CC Channel 0 Complementary Dead-Time Insertion Pin Enable */
#define _WTIMER_ROUTEPEN_CDTI0PEN_SHIFT             8                                         /**< Shift value for TIMER_CDTI0PEN */
#define _WTIMER_ROUTEPEN_CDTI0PEN_MASK              0x100UL                                   /**< Bit mask for TIMER_CDTI0PEN */
#define _WTIMER_ROUTEPEN_CDTI0PEN_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CDTI0PEN_DEFAULT            (_WTIMER_ROUTEPEN_CDTI0PEN_DEFAULT << 8)  /**< Shifted mode DEFAULT for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CDTI1PEN                    (0x1UL << 9)                              /**< CC Channel 1 Complementary Dead-Time Insertion Pin Enable */
#define _WTIMER_ROUTEPEN_CDTI1PEN_SHIFT             9                                         /**< Shift value for TIMER_CDTI1PEN */
#define _WTIMER_ROUTEPEN_CDTI1PEN_MASK              0x200UL                                   /**< Bit mask for TIMER_CDTI1PEN */
#define _WTIMER_ROUTEPEN_CDTI1PEN_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CDTI1PEN_DEFAULT            (_WTIMER_ROUTEPEN_CDTI1PEN_DEFAULT << 9)  /**< Shifted mode DEFAULT for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CDTI2PEN                    (0x1UL << 10)                             /**< CC Channel 2 Complementary Dead-Time Insertion Pin Enable */
#define _WTIMER_ROUTEPEN_CDTI2PEN_SHIFT             10                                        /**< Shift value for TIMER_CDTI2PEN */
#define _WTIMER_ROUTEPEN_CDTI2PEN_MASK              0x400UL                                   /**< Bit mask for TIMER_CDTI2PEN */
#define _WTIMER_ROUTEPEN_CDTI2PEN_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for WTIMER_ROUTEPEN */
#define WTIMER_ROUTEPEN_CDTI2PEN_DEFAULT            (_WTIMER_ROUTEPEN_CDTI2PEN_DEFAULT << 10) /**< Shifted mode DEFAULT for WTIMER_ROUTEPEN */

/* Bit fields for WTIMER ROUTELOC0 */
#define _WTIMER_ROUTELOC0_RESETVALUE                0x00000000UL                             /**< Default value for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_MASK                      0x1F1F1F1FUL                             /**< Mask for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_SHIFT              0                                        /**< Shift value for TIMER_CC0LOC */
#define _WTIMER_ROUTELOC0_CC0LOC_MASK               0x1FUL                                   /**< Bit mask for TIMER_CC0LOC */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC0               0x00000000UL                             /**< Mode LOC0 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC1               0x00000001UL                             /**< Mode LOC1 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC2               0x00000002UL                             /**< Mode LOC2 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC3               0x00000003UL                             /**< Mode LOC3 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC4               0x00000004UL                             /**< Mode LOC4 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC5               0x00000005UL                             /**< Mode LOC5 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC6               0x00000006UL                             /**< Mode LOC6 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC7               0x00000007UL                             /**< Mode LOC7 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC8               0x00000008UL                             /**< Mode LOC8 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC9               0x00000009UL                             /**< Mode LOC9 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC10              0x0000000AUL                             /**< Mode LOC10 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC11              0x0000000BUL                             /**< Mode LOC11 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC12              0x0000000CUL                             /**< Mode LOC12 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC13              0x0000000DUL                             /**< Mode LOC13 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC14              0x0000000EUL                             /**< Mode LOC14 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC15              0x0000000FUL                             /**< Mode LOC15 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC16              0x00000010UL                             /**< Mode LOC16 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC17              0x00000011UL                             /**< Mode LOC17 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC18              0x00000012UL                             /**< Mode LOC18 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC19              0x00000013UL                             /**< Mode LOC19 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC20              0x00000014UL                             /**< Mode LOC20 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC21              0x00000015UL                             /**< Mode LOC21 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC22              0x00000016UL                             /**< Mode LOC22 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC23              0x00000017UL                             /**< Mode LOC23 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC24              0x00000018UL                             /**< Mode LOC24 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC25              0x00000019UL                             /**< Mode LOC25 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC26              0x0000001AUL                             /**< Mode LOC26 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC27              0x0000001BUL                             /**< Mode LOC27 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC28              0x0000001CUL                             /**< Mode LOC28 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC29              0x0000001DUL                             /**< Mode LOC29 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC30              0x0000001EUL                             /**< Mode LOC30 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC0LOC_LOC31              0x0000001FUL                             /**< Mode LOC31 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC0                (_WTIMER_ROUTELOC0_CC0LOC_LOC0 << 0)     /**< Shifted mode LOC0 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_DEFAULT             (_WTIMER_ROUTELOC0_CC0LOC_DEFAULT << 0)  /**< Shifted mode DEFAULT for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC1                (_WTIMER_ROUTELOC0_CC0LOC_LOC1 << 0)     /**< Shifted mode LOC1 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC2                (_WTIMER_ROUTELOC0_CC0LOC_LOC2 << 0)     /**< Shifted mode LOC2 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC3                (_WTIMER_ROUTELOC0_CC0LOC_LOC3 << 0)     /**< Shifted mode LOC3 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC4                (_WTIMER_ROUTELOC0_CC0LOC_LOC4 << 0)     /**< Shifted mode LOC4 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC5                (_WTIMER_ROUTELOC0_CC0LOC_LOC5 << 0)     /**< Shifted mode LOC5 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC6                (_WTIMER_ROUTELOC0_CC0LOC_LOC6 << 0)     /**< Shifted mode LOC6 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC7                (_WTIMER_ROUTELOC0_CC0LOC_LOC7 << 0)     /**< Shifted mode LOC7 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC8                (_WTIMER_ROUTELOC0_CC0LOC_LOC8 << 0)     /**< Shifted mode LOC8 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC9                (_WTIMER_ROUTELOC0_CC0LOC_LOC9 << 0)     /**< Shifted mode LOC9 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC10               (_WTIMER_ROUTELOC0_CC0LOC_LOC10 << 0)    /**< Shifted mode LOC10 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC11               (_WTIMER_ROUTELOC0_CC0LOC_LOC11 << 0)    /**< Shifted mode LOC11 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC12               (_WTIMER_ROUTELOC0_CC0LOC_LOC12 << 0)    /**< Shifted mode LOC12 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC13               (_WTIMER_ROUTELOC0_CC0LOC_LOC13 << 0)    /**< Shifted mode LOC13 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC14               (_WTIMER_ROUTELOC0_CC0LOC_LOC14 << 0)    /**< Shifted mode LOC14 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC15               (_WTIMER_ROUTELOC0_CC0LOC_LOC15 << 0)    /**< Shifted mode LOC15 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC16               (_WTIMER_ROUTELOC0_CC0LOC_LOC16 << 0)    /**< Shifted mode LOC16 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC17               (_WTIMER_ROUTELOC0_CC0LOC_LOC17 << 0)    /**< Shifted mode LOC17 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC18               (_WTIMER_ROUTELOC0_CC0LOC_LOC18 << 0)    /**< Shifted mode LOC18 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC19               (_WTIMER_ROUTELOC0_CC0LOC_LOC19 << 0)    /**< Shifted mode LOC19 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC20               (_WTIMER_ROUTELOC0_CC0LOC_LOC20 << 0)    /**< Shifted mode LOC20 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC21               (_WTIMER_ROUTELOC0_CC0LOC_LOC21 << 0)    /**< Shifted mode LOC21 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC22               (_WTIMER_ROUTELOC0_CC0LOC_LOC22 << 0)    /**< Shifted mode LOC22 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC23               (_WTIMER_ROUTELOC0_CC0LOC_LOC23 << 0)    /**< Shifted mode LOC23 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC24               (_WTIMER_ROUTELOC0_CC0LOC_LOC24 << 0)    /**< Shifted mode LOC24 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC25               (_WTIMER_ROUTELOC0_CC0LOC_LOC25 << 0)    /**< Shifted mode LOC25 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC26               (_WTIMER_ROUTELOC0_CC0LOC_LOC26 << 0)    /**< Shifted mode LOC26 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC27               (_WTIMER_ROUTELOC0_CC0LOC_LOC27 << 0)    /**< Shifted mode LOC27 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC28               (_WTIMER_ROUTELOC0_CC0LOC_LOC28 << 0)    /**< Shifted mode LOC28 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC29               (_WTIMER_ROUTELOC0_CC0LOC_LOC29 << 0)    /**< Shifted mode LOC29 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC30               (_WTIMER_ROUTELOC0_CC0LOC_LOC30 << 0)    /**< Shifted mode LOC30 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC0LOC_LOC31               (_WTIMER_ROUTELOC0_CC0LOC_LOC31 << 0)    /**< Shifted mode LOC31 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_SHIFT              8                                        /**< Shift value for TIMER_CC1LOC */
#define _WTIMER_ROUTELOC0_CC1LOC_MASK               0x1F00UL                                 /**< Bit mask for TIMER_CC1LOC */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC0               0x00000000UL                             /**< Mode LOC0 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC1               0x00000001UL                             /**< Mode LOC1 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC2               0x00000002UL                             /**< Mode LOC2 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC3               0x00000003UL                             /**< Mode LOC3 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC4               0x00000004UL                             /**< Mode LOC4 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC5               0x00000005UL                             /**< Mode LOC5 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC6               0x00000006UL                             /**< Mode LOC6 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC7               0x00000007UL                             /**< Mode LOC7 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC8               0x00000008UL                             /**< Mode LOC8 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC9               0x00000009UL                             /**< Mode LOC9 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC10              0x0000000AUL                             /**< Mode LOC10 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC11              0x0000000BUL                             /**< Mode LOC11 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC12              0x0000000CUL                             /**< Mode LOC12 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC13              0x0000000DUL                             /**< Mode LOC13 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC14              0x0000000EUL                             /**< Mode LOC14 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC15              0x0000000FUL                             /**< Mode LOC15 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC16              0x00000010UL                             /**< Mode LOC16 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC17              0x00000011UL                             /**< Mode LOC17 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC18              0x00000012UL                             /**< Mode LOC18 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC19              0x00000013UL                             /**< Mode LOC19 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC20              0x00000014UL                             /**< Mode LOC20 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC21              0x00000015UL                             /**< Mode LOC21 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC22              0x00000016UL                             /**< Mode LOC22 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC23              0x00000017UL                             /**< Mode LOC23 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC24              0x00000018UL                             /**< Mode LOC24 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC25              0x00000019UL                             /**< Mode LOC25 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC26              0x0000001AUL                             /**< Mode LOC26 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC27              0x0000001BUL                             /**< Mode LOC27 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC28              0x0000001CUL                             /**< Mode LOC28 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC29              0x0000001DUL                             /**< Mode LOC29 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC30              0x0000001EUL                             /**< Mode LOC30 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC1LOC_LOC31              0x0000001FUL                             /**< Mode LOC31 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC0                (_WTIMER_ROUTELOC0_CC1LOC_LOC0 << 8)     /**< Shifted mode LOC0 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_DEFAULT             (_WTIMER_ROUTELOC0_CC1LOC_DEFAULT << 8)  /**< Shifted mode DEFAULT for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC1                (_WTIMER_ROUTELOC0_CC1LOC_LOC1 << 8)     /**< Shifted mode LOC1 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC2                (_WTIMER_ROUTELOC0_CC1LOC_LOC2 << 8)     /**< Shifted mode LOC2 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC3                (_WTIMER_ROUTELOC0_CC1LOC_LOC3 << 8)     /**< Shifted mode LOC3 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC4                (_WTIMER_ROUTELOC0_CC1LOC_LOC4 << 8)     /**< Shifted mode LOC4 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC5                (_WTIMER_ROUTELOC0_CC1LOC_LOC5 << 8)     /**< Shifted mode LOC5 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC6                (_WTIMER_ROUTELOC0_CC1LOC_LOC6 << 8)     /**< Shifted mode LOC6 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC7                (_WTIMER_ROUTELOC0_CC1LOC_LOC7 << 8)     /**< Shifted mode LOC7 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC8                (_WTIMER_ROUTELOC0_CC1LOC_LOC8 << 8)     /**< Shifted mode LOC8 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC9                (_WTIMER_ROUTELOC0_CC1LOC_LOC9 << 8)     /**< Shifted mode LOC9 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC10               (_WTIMER_ROUTELOC0_CC1LOC_LOC10 << 8)    /**< Shifted mode LOC10 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC11               (_WTIMER_ROUTELOC0_CC1LOC_LOC11 << 8)    /**< Shifted mode LOC11 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC12               (_WTIMER_ROUTELOC0_CC1LOC_LOC12 << 8)    /**< Shifted mode LOC12 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC13               (_WTIMER_ROUTELOC0_CC1LOC_LOC13 << 8)    /**< Shifted mode LOC13 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC14               (_WTIMER_ROUTELOC0_CC1LOC_LOC14 << 8)    /**< Shifted mode LOC14 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC15               (_WTIMER_ROUTELOC0_CC1LOC_LOC15 << 8)    /**< Shifted mode LOC15 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC16               (_WTIMER_ROUTELOC0_CC1LOC_LOC16 << 8)    /**< Shifted mode LOC16 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC17               (_WTIMER_ROUTELOC0_CC1LOC_LOC17 << 8)    /**< Shifted mode LOC17 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC18               (_WTIMER_ROUTELOC0_CC1LOC_LOC18 << 8)    /**< Shifted mode LOC18 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC19               (_WTIMER_ROUTELOC0_CC1LOC_LOC19 << 8)    /**< Shifted mode LOC19 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC20               (_WTIMER_ROUTELOC0_CC1LOC_LOC20 << 8)    /**< Shifted mode LOC20 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC21               (_WTIMER_ROUTELOC0_CC1LOC_LOC21 << 8)    /**< Shifted mode LOC21 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC22               (_WTIMER_ROUTELOC0_CC1LOC_LOC22 << 8)    /**< Shifted mode LOC22 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC23               (_WTIMER_ROUTELOC0_CC1LOC_LOC23 << 8)    /**< Shifted mode LOC23 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC24               (_WTIMER_ROUTELOC0_CC1LOC_LOC24 << 8)    /**< Shifted mode LOC24 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC25               (_WTIMER_ROUTELOC0_CC1LOC_LOC25 << 8)    /**< Shifted mode LOC25 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC26               (_WTIMER_ROUTELOC0_CC1LOC_LOC26 << 8)    /**< Shifted mode LOC26 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC27               (_WTIMER_ROUTELOC0_CC1LOC_LOC27 << 8)    /**< Shifted mode LOC27 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC28               (_WTIMER_ROUTELOC0_CC1LOC_LOC28 << 8)    /**< Shifted mode LOC28 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC29               (_WTIMER_ROUTELOC0_CC1LOC_LOC29 << 8)    /**< Shifted mode LOC29 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC30               (_WTIMER_ROUTELOC0_CC1LOC_LOC30 << 8)    /**< Shifted mode LOC30 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC1LOC_LOC31               (_WTIMER_ROUTELOC0_CC1LOC_LOC31 << 8)    /**< Shifted mode LOC31 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_SHIFT              16                                       /**< Shift value for TIMER_CC2LOC */
#define _WTIMER_ROUTELOC0_CC2LOC_MASK               0x1F0000UL                               /**< Bit mask for TIMER_CC2LOC */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC0               0x00000000UL                             /**< Mode LOC0 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC1               0x00000001UL                             /**< Mode LOC1 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC2               0x00000002UL                             /**< Mode LOC2 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC3               0x00000003UL                             /**< Mode LOC3 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC4               0x00000004UL                             /**< Mode LOC4 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC5               0x00000005UL                             /**< Mode LOC5 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC6               0x00000006UL                             /**< Mode LOC6 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC7               0x00000007UL                             /**< Mode LOC7 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC8               0x00000008UL                             /**< Mode LOC8 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC9               0x00000009UL                             /**< Mode LOC9 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC10              0x0000000AUL                             /**< Mode LOC10 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC11              0x0000000BUL                             /**< Mode LOC11 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC12              0x0000000CUL                             /**< Mode LOC12 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC13              0x0000000DUL                             /**< Mode LOC13 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC14              0x0000000EUL                             /**< Mode LOC14 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC15              0x0000000FUL                             /**< Mode LOC15 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC16              0x00000010UL                             /**< Mode LOC16 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC17              0x00000011UL                             /**< Mode LOC17 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC18              0x00000012UL                             /**< Mode LOC18 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC19              0x00000013UL                             /**< Mode LOC19 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC20              0x00000014UL                             /**< Mode LOC20 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC21              0x00000015UL                             /**< Mode LOC21 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC22              0x00000016UL                             /**< Mode LOC22 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC23              0x00000017UL                             /**< Mode LOC23 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC24              0x00000018UL                             /**< Mode LOC24 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC25              0x00000019UL                             /**< Mode LOC25 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC26              0x0000001AUL                             /**< Mode LOC26 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC27              0x0000001BUL                             /**< Mode LOC27 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC28              0x0000001CUL                             /**< Mode LOC28 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC29              0x0000001DUL                             /**< Mode LOC29 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC30              0x0000001EUL                             /**< Mode LOC30 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC2LOC_LOC31              0x0000001FUL                             /**< Mode LOC31 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC0                (_WTIMER_ROUTELOC0_CC2LOC_LOC0 << 16)    /**< Shifted mode LOC0 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_DEFAULT             (_WTIMER_ROUTELOC0_CC2LOC_DEFAULT << 16) /**< Shifted mode DEFAULT for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC1                (_WTIMER_ROUTELOC0_CC2LOC_LOC1 << 16)    /**< Shifted mode LOC1 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC2                (_WTIMER_ROUTELOC0_CC2LOC_LOC2 << 16)    /**< Shifted mode LOC2 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC3                (_WTIMER_ROUTELOC0_CC2LOC_LOC3 << 16)    /**< Shifted mode LOC3 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC4                (_WTIMER_ROUTELOC0_CC2LOC_LOC4 << 16)    /**< Shifted mode LOC4 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC5                (_WTIMER_ROUTELOC0_CC2LOC_LOC5 << 16)    /**< Shifted mode LOC5 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC6                (_WTIMER_ROUTELOC0_CC2LOC_LOC6 << 16)    /**< Shifted mode LOC6 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC7                (_WTIMER_ROUTELOC0_CC2LOC_LOC7 << 16)    /**< Shifted mode LOC7 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC8                (_WTIMER_ROUTELOC0_CC2LOC_LOC8 << 16)    /**< Shifted mode LOC8 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC9                (_WTIMER_ROUTELOC0_CC2LOC_LOC9 << 16)    /**< Shifted mode LOC9 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC10               (_WTIMER_ROUTELOC0_CC2LOC_LOC10 << 16)   /**< Shifted mode LOC10 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC11               (_WTIMER_ROUTELOC0_CC2LOC_LOC11 << 16)   /**< Shifted mode LOC11 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC12               (_WTIMER_ROUTELOC0_CC2LOC_LOC12 << 16)   /**< Shifted mode LOC12 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC13               (_WTIMER_ROUTELOC0_CC2LOC_LOC13 << 16)   /**< Shifted mode LOC13 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC14               (_WTIMER_ROUTELOC0_CC2LOC_LOC14 << 16)   /**< Shifted mode LOC14 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC15               (_WTIMER_ROUTELOC0_CC2LOC_LOC15 << 16)   /**< Shifted mode LOC15 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC16               (_WTIMER_ROUTELOC0_CC2LOC_LOC16 << 16)   /**< Shifted mode LOC16 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC17               (_WTIMER_ROUTELOC0_CC2LOC_LOC17 << 16)   /**< Shifted mode LOC17 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC18               (_WTIMER_ROUTELOC0_CC2LOC_LOC18 << 16)   /**< Shifted mode LOC18 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC19               (_WTIMER_ROUTELOC0_CC2LOC_LOC19 << 16)   /**< Shifted mode LOC19 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC20               (_WTIMER_ROUTELOC0_CC2LOC_LOC20 << 16)   /**< Shifted mode LOC20 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC21               (_WTIMER_ROUTELOC0_CC2LOC_LOC21 << 16)   /**< Shifted mode LOC21 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC22               (_WTIMER_ROUTELOC0_CC2LOC_LOC22 << 16)   /**< Shifted mode LOC22 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC23               (_WTIMER_ROUTELOC0_CC2LOC_LOC23 << 16)   /**< Shifted mode LOC23 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC24               (_WTIMER_ROUTELOC0_CC2LOC_LOC24 << 16)   /**< Shifted mode LOC24 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC25               (_WTIMER_ROUTELOC0_CC2LOC_LOC25 << 16)   /**< Shifted mode LOC25 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC26               (_WTIMER_ROUTELOC0_CC2LOC_LOC26 << 16)   /**< Shifted mode LOC26 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC27               (_WTIMER_ROUTELOC0_CC2LOC_LOC27 << 16)   /**< Shifted mode LOC27 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC28               (_WTIMER_ROUTELOC0_CC2LOC_LOC28 << 16)   /**< Shifted mode LOC28 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC29               (_WTIMER_ROUTELOC0_CC2LOC_LOC29 << 16)   /**< Shifted mode LOC29 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC30               (_WTIMER_ROUTELOC0_CC2LOC_LOC30 << 16)   /**< Shifted mode LOC30 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC2LOC_LOC31               (_WTIMER_ROUTELOC0_CC2LOC_LOC31 << 16)   /**< Shifted mode LOC31 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_SHIFT              24                                       /**< Shift value for TIMER_CC3LOC */
#define _WTIMER_ROUTELOC0_CC3LOC_MASK               0x1F000000UL                             /**< Bit mask for TIMER_CC3LOC */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC0               0x00000000UL                             /**< Mode LOC0 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC1               0x00000001UL                             /**< Mode LOC1 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC2               0x00000002UL                             /**< Mode LOC2 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC3               0x00000003UL                             /**< Mode LOC3 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC4               0x00000004UL                             /**< Mode LOC4 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC5               0x00000005UL                             /**< Mode LOC5 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC6               0x00000006UL                             /**< Mode LOC6 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC7               0x00000007UL                             /**< Mode LOC7 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC8               0x00000008UL                             /**< Mode LOC8 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC9               0x00000009UL                             /**< Mode LOC9 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC10              0x0000000AUL                             /**< Mode LOC10 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC11              0x0000000BUL                             /**< Mode LOC11 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC12              0x0000000CUL                             /**< Mode LOC12 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC13              0x0000000DUL                             /**< Mode LOC13 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC14              0x0000000EUL                             /**< Mode LOC14 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC15              0x0000000FUL                             /**< Mode LOC15 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC16              0x00000010UL                             /**< Mode LOC16 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC17              0x00000011UL                             /**< Mode LOC17 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC18              0x00000012UL                             /**< Mode LOC18 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC19              0x00000013UL                             /**< Mode LOC19 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC20              0x00000014UL                             /**< Mode LOC20 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC21              0x00000015UL                             /**< Mode LOC21 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC22              0x00000016UL                             /**< Mode LOC22 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC23              0x00000017UL                             /**< Mode LOC23 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC24              0x00000018UL                             /**< Mode LOC24 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC25              0x00000019UL                             /**< Mode LOC25 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC26              0x0000001AUL                             /**< Mode LOC26 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC27              0x0000001BUL                             /**< Mode LOC27 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC28              0x0000001CUL                             /**< Mode LOC28 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC29              0x0000001DUL                             /**< Mode LOC29 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC30              0x0000001EUL                             /**< Mode LOC30 for WTIMER_ROUTELOC0 */
#define _WTIMER_ROUTELOC0_CC3LOC_LOC31              0x0000001FUL                             /**< Mode LOC31 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC0                (_WTIMER_ROUTELOC0_CC3LOC_LOC0 << 24)    /**< Shifted mode LOC0 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_DEFAULT             (_WTIMER_ROUTELOC0_CC3LOC_DEFAULT << 24) /**< Shifted mode DEFAULT for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC1                (_WTIMER_ROUTELOC0_CC3LOC_LOC1 << 24)    /**< Shifted mode LOC1 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC2                (_WTIMER_ROUTELOC0_CC3LOC_LOC2 << 24)    /**< Shifted mode LOC2 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC3                (_WTIMER_ROUTELOC0_CC3LOC_LOC3 << 24)    /**< Shifted mode LOC3 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC4                (_WTIMER_ROUTELOC0_CC3LOC_LOC4 << 24)    /**< Shifted mode LOC4 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC5                (_WTIMER_ROUTELOC0_CC3LOC_LOC5 << 24)    /**< Shifted mode LOC5 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC6                (_WTIMER_ROUTELOC0_CC3LOC_LOC6 << 24)    /**< Shifted mode LOC6 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC7                (_WTIMER_ROUTELOC0_CC3LOC_LOC7 << 24)    /**< Shifted mode LOC7 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC8                (_WTIMER_ROUTELOC0_CC3LOC_LOC8 << 24)    /**< Shifted mode LOC8 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC9                (_WTIMER_ROUTELOC0_CC3LOC_LOC9 << 24)    /**< Shifted mode LOC9 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC10               (_WTIMER_ROUTELOC0_CC3LOC_LOC10 << 24)   /**< Shifted mode LOC10 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC11               (_WTIMER_ROUTELOC0_CC3LOC_LOC11 << 24)   /**< Shifted mode LOC11 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC12               (_WTIMER_ROUTELOC0_CC3LOC_LOC12 << 24)   /**< Shifted mode LOC12 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC13               (_WTIMER_ROUTELOC0_CC3LOC_LOC13 << 24)   /**< Shifted mode LOC13 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC14               (_WTIMER_ROUTELOC0_CC3LOC_LOC14 << 24)   /**< Shifted mode LOC14 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC15               (_WTIMER_ROUTELOC0_CC3LOC_LOC15 << 24)   /**< Shifted mode LOC15 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC16               (_WTIMER_ROUTELOC0_CC3LOC_LOC16 << 24)   /**< Shifted mode LOC16 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC17               (_WTIMER_ROUTELOC0_CC3LOC_LOC17 << 24)   /**< Shifted mode LOC17 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC18               (_WTIMER_ROUTELOC0_CC3LOC_LOC18 << 24)   /**< Shifted mode LOC18 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC19               (_WTIMER_ROUTELOC0_CC3LOC_LOC19 << 24)   /**< Shifted mode LOC19 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC20               (_WTIMER_ROUTELOC0_CC3LOC_LOC20 << 24)   /**< Shifted mode LOC20 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC21               (_WTIMER_ROUTELOC0_CC3LOC_LOC21 << 24)   /**< Shifted mode LOC21 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC22               (_WTIMER_ROUTELOC0_CC3LOC_LOC22 << 24)   /**< Shifted mode LOC22 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC23               (_WTIMER_ROUTELOC0_CC3LOC_LOC23 << 24)   /**< Shifted mode LOC23 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC24               (_WTIMER_ROUTELOC0_CC3LOC_LOC24 << 24)   /**< Shifted mode LOC24 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC25               (_WTIMER_ROUTELOC0_CC3LOC_LOC25 << 24)   /**< Shifted mode LOC25 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC26               (_WTIMER_ROUTELOC0_CC3LOC_LOC26 << 24)   /**< Shifted mode LOC26 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC27               (_WTIMER_ROUTELOC0_CC3LOC_LOC27 << 24)   /**< Shifted mode LOC27 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC28               (_WTIMER_ROUTELOC0_CC3LOC_LOC28 << 24)   /**< Shifted mode LOC28 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC29               (_WTIMER_ROUTELOC0_CC3LOC_LOC29 << 24)   /**< Shifted mode LOC29 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC30               (_WTIMER_ROUTELOC0_CC3LOC_LOC30 << 24)   /**< Shifted mode LOC30 for WTIMER_ROUTELOC0 */
#define WTIMER_ROUTELOC0_CC3LOC_LOC31               (_WTIMER_ROUTELOC0_CC3LOC_LOC31 << 24)   /**< Shifted mode LOC31 for WTIMER_ROUTELOC0 */

/* Bit fields for WTIMER ROUTELOC2 */
#define _WTIMER_ROUTELOC2_RESETVALUE                0x00000000UL                               /**< Default value for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_MASK                      0x001F1F1FUL                               /**< Mask for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_SHIFT            0                                          /**< Shift value for TIMER_CDTI0LOC */
#define _WTIMER_ROUTELOC2_CDTI0LOC_MASK             0x1FUL                                     /**< Bit mask for TIMER_CDTI0LOC */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC0             0x00000000UL                               /**< Mode LOC0 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_DEFAULT          0x00000000UL                               /**< Mode DEFAULT for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC1             0x00000001UL                               /**< Mode LOC1 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC2             0x00000002UL                               /**< Mode LOC2 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC3             0x00000003UL                               /**< Mode LOC3 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC4             0x00000004UL                               /**< Mode LOC4 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC5             0x00000005UL                               /**< Mode LOC5 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC6             0x00000006UL                               /**< Mode LOC6 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC7             0x00000007UL                               /**< Mode LOC7 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC8             0x00000008UL                               /**< Mode LOC8 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC9             0x00000009UL                               /**< Mode LOC9 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC10            0x0000000AUL                               /**< Mode LOC10 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC11            0x0000000BUL                               /**< Mode LOC11 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC12            0x0000000CUL                               /**< Mode LOC12 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC13            0x0000000DUL                               /**< Mode LOC13 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC14            0x0000000EUL                               /**< Mode LOC14 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC15            0x0000000FUL                               /**< Mode LOC15 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC16            0x00000010UL                               /**< Mode LOC16 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC17            0x00000011UL                               /**< Mode LOC17 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC18            0x00000012UL                               /**< Mode LOC18 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC19            0x00000013UL                               /**< Mode LOC19 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC20            0x00000014UL                               /**< Mode LOC20 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC21            0x00000015UL                               /**< Mode LOC21 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC22            0x00000016UL                               /**< Mode LOC22 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC23            0x00000017UL                               /**< Mode LOC23 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC24            0x00000018UL                               /**< Mode LOC24 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC25            0x00000019UL                               /**< Mode LOC25 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC26            0x0000001AUL                               /**< Mode LOC26 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC27            0x0000001BUL                               /**< Mode LOC27 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC28            0x0000001CUL                               /**< Mode LOC28 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC29            0x0000001DUL                               /**< Mode LOC29 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC30            0x0000001EUL                               /**< Mode LOC30 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI0LOC_LOC31            0x0000001FUL                               /**< Mode LOC31 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC0              (_WTIMER_ROUTELOC2_CDTI0LOC_LOC0 << 0)     /**< Shifted mode LOC0 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_DEFAULT           (_WTIMER_ROUTELOC2_CDTI0LOC_DEFAULT << 0)  /**< Shifted mode DEFAULT for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC1              (_WTIMER_ROUTELOC2_CDTI0LOC_LOC1 << 0)     /**< Shifted mode LOC1 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC2              (_WTIMER_ROUTELOC2_CDTI0LOC_LOC2 << 0)     /**< Shifted mode LOC2 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC3              (_WTIMER_ROUTELOC2_CDTI0LOC_LOC3 << 0)     /**< Shifted mode LOC3 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC4              (_WTIMER_ROUTELOC2_CDTI0LOC_LOC4 << 0)     /**< Shifted mode LOC4 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC5              (_WTIMER_ROUTELOC2_CDTI0LOC_LOC5 << 0)     /**< Shifted mode LOC5 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC6              (_WTIMER_ROUTELOC2_CDTI0LOC_LOC6 << 0)     /**< Shifted mode LOC6 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC7              (_WTIMER_ROUTELOC2_CDTI0LOC_LOC7 << 0)     /**< Shifted mode LOC7 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC8              (_WTIMER_ROUTELOC2_CDTI0LOC_LOC8 << 0)     /**< Shifted mode LOC8 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC9              (_WTIMER_ROUTELOC2_CDTI0LOC_LOC9 << 0)     /**< Shifted mode LOC9 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC10             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC10 << 0)    /**< Shifted mode LOC10 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC11             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC11 << 0)    /**< Shifted mode LOC11 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC12             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC12 << 0)    /**< Shifted mode LOC12 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC13             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC13 << 0)    /**< Shifted mode LOC13 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC14             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC14 << 0)    /**< Shifted mode LOC14 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC15             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC15 << 0)    /**< Shifted mode LOC15 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC16             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC16 << 0)    /**< Shifted mode LOC16 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC17             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC17 << 0)    /**< Shifted mode LOC17 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC18             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC18 << 0)    /**< Shifted mode LOC18 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC19             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC19 << 0)    /**< Shifted mode LOC19 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC20             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC20 << 0)    /**< Shifted mode LOC20 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC21             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC21 << 0)    /**< Shifted mode LOC21 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC22             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC22 << 0)    /**< Shifted mode LOC22 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC23             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC23 << 0)    /**< Shifted mode LOC23 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC24             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC24 << 0)    /**< Shifted mode LOC24 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC25             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC25 << 0)    /**< Shifted mode LOC25 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC26             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC26 << 0)    /**< Shifted mode LOC26 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC27             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC27 << 0)    /**< Shifted mode LOC27 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC28             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC28 << 0)    /**< Shifted mode LOC28 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC29             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC29 << 0)    /**< Shifted mode LOC29 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC30             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC30 << 0)    /**< Shifted mode LOC30 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI0LOC_LOC31             (_WTIMER_ROUTELOC2_CDTI0LOC_LOC31 << 0)    /**< Shifted mode LOC31 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_SHIFT            8                                          /**< Shift value for TIMER_CDTI1LOC */
#define _WTIMER_ROUTELOC2_CDTI1LOC_MASK             0x1F00UL                                   /**< Bit mask for TIMER_CDTI1LOC */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC0             0x00000000UL                               /**< Mode LOC0 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_DEFAULT          0x00000000UL                               /**< Mode DEFAULT for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC1             0x00000001UL                               /**< Mode LOC1 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC2             0x00000002UL                               /**< Mode LOC2 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC3             0x00000003UL                               /**< Mode LOC3 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC4             0x00000004UL                               /**< Mode LOC4 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC5             0x00000005UL                               /**< Mode LOC5 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC6             0x00000006UL                               /**< Mode LOC6 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC7             0x00000007UL                               /**< Mode LOC7 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC8             0x00000008UL                               /**< Mode LOC8 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC9             0x00000009UL                               /**< Mode LOC9 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC10            0x0000000AUL                               /**< Mode LOC10 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC11            0x0000000BUL                               /**< Mode LOC11 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC12            0x0000000CUL                               /**< Mode LOC12 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC13            0x0000000DUL                               /**< Mode LOC13 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC14            0x0000000EUL                               /**< Mode LOC14 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC15            0x0000000FUL                               /**< Mode LOC15 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC16            0x00000010UL                               /**< Mode LOC16 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC17            0x00000011UL                               /**< Mode LOC17 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC18            0x00000012UL                               /**< Mode LOC18 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC19            0x00000013UL                               /**< Mode LOC19 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC20            0x00000014UL                               /**< Mode LOC20 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC21            0x00000015UL                               /**< Mode LOC21 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC22            0x00000016UL                               /**< Mode LOC22 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC23            0x00000017UL                               /**< Mode LOC23 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC24            0x00000018UL                               /**< Mode LOC24 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC25            0x00000019UL                               /**< Mode LOC25 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC26            0x0000001AUL                               /**< Mode LOC26 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC27            0x0000001BUL                               /**< Mode LOC27 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC28            0x0000001CUL                               /**< Mode LOC28 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC29            0x0000001DUL                               /**< Mode LOC29 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC30            0x0000001EUL                               /**< Mode LOC30 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI1LOC_LOC31            0x0000001FUL                               /**< Mode LOC31 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC0              (_WTIMER_ROUTELOC2_CDTI1LOC_LOC0 << 8)     /**< Shifted mode LOC0 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_DEFAULT           (_WTIMER_ROUTELOC2_CDTI1LOC_DEFAULT << 8)  /**< Shifted mode DEFAULT for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC1              (_WTIMER_ROUTELOC2_CDTI1LOC_LOC1 << 8)     /**< Shifted mode LOC1 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC2              (_WTIMER_ROUTELOC2_CDTI1LOC_LOC2 << 8)     /**< Shifted mode LOC2 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC3              (_WTIMER_ROUTELOC2_CDTI1LOC_LOC3 << 8)     /**< Shifted mode LOC3 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC4              (_WTIMER_ROUTELOC2_CDTI1LOC_LOC4 << 8)     /**< Shifted mode LOC4 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC5              (_WTIMER_ROUTELOC2_CDTI1LOC_LOC5 << 8)     /**< Shifted mode LOC5 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC6              (_WTIMER_ROUTELOC2_CDTI1LOC_LOC6 << 8)     /**< Shifted mode LOC6 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC7              (_WTIMER_ROUTELOC2_CDTI1LOC_LOC7 << 8)     /**< Shifted mode LOC7 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC8              (_WTIMER_ROUTELOC2_CDTI1LOC_LOC8 << 8)     /**< Shifted mode LOC8 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC9              (_WTIMER_ROUTELOC2_CDTI1LOC_LOC9 << 8)     /**< Shifted mode LOC9 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC10             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC10 << 8)    /**< Shifted mode LOC10 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC11             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC11 << 8)    /**< Shifted mode LOC11 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC12             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC12 << 8)    /**< Shifted mode LOC12 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC13             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC13 << 8)    /**< Shifted mode LOC13 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC14             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC14 << 8)    /**< Shifted mode LOC14 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC15             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC15 << 8)    /**< Shifted mode LOC15 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC16             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC16 << 8)    /**< Shifted mode LOC16 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC17             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC17 << 8)    /**< Shifted mode LOC17 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC18             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC18 << 8)    /**< Shifted mode LOC18 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC19             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC19 << 8)    /**< Shifted mode LOC19 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC20             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC20 << 8)    /**< Shifted mode LOC20 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC21             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC21 << 8)    /**< Shifted mode LOC21 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC22             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC22 << 8)    /**< Shifted mode LOC22 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC23             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC23 << 8)    /**< Shifted mode LOC23 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC24             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC24 << 8)    /**< Shifted mode LOC24 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC25             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC25 << 8)    /**< Shifted mode LOC25 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC26             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC26 << 8)    /**< Shifted mode LOC26 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC27             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC27 << 8)    /**< Shifted mode LOC27 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC28             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC28 << 8)    /**< Shifted mode LOC28 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC29             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC29 << 8)    /**< Shifted mode LOC29 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC30             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC30 << 8)    /**< Shifted mode LOC30 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI1LOC_LOC31             (_WTIMER_ROUTELOC2_CDTI1LOC_LOC31 << 8)    /**< Shifted mode LOC31 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_SHIFT            16                                         /**< Shift value for TIMER_CDTI2LOC */
#define _WTIMER_ROUTELOC2_CDTI2LOC_MASK             0x1F0000UL                                 /**< Bit mask for TIMER_CDTI2LOC */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC0             0x00000000UL                               /**< Mode LOC0 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_DEFAULT          0x00000000UL                               /**< Mode DEFAULT for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC1             0x00000001UL                               /**< Mode LOC1 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC2             0x00000002UL                               /**< Mode LOC2 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC3             0x00000003UL                               /**< Mode LOC3 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC4             0x00000004UL                               /**< Mode LOC4 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC5             0x00000005UL                               /**< Mode LOC5 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC6             0x00000006UL                               /**< Mode LOC6 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC7             0x00000007UL                               /**< Mode LOC7 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC8             0x00000008UL                               /**< Mode LOC8 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC9             0x00000009UL                               /**< Mode LOC9 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC10            0x0000000AUL                               /**< Mode LOC10 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC11            0x0000000BUL                               /**< Mode LOC11 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC12            0x0000000CUL                               /**< Mode LOC12 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC13            0x0000000DUL                               /**< Mode LOC13 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC14            0x0000000EUL                               /**< Mode LOC14 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC15            0x0000000FUL                               /**< Mode LOC15 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC16            0x00000010UL                               /**< Mode LOC16 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC17            0x00000011UL                               /**< Mode LOC17 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC18            0x00000012UL                               /**< Mode LOC18 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC19            0x00000013UL                               /**< Mode LOC19 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC20            0x00000014UL                               /**< Mode LOC20 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC21            0x00000015UL                               /**< Mode LOC21 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC22            0x00000016UL                               /**< Mode LOC22 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC23            0x00000017UL                               /**< Mode LOC23 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC24            0x00000018UL                               /**< Mode LOC24 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC25            0x00000019UL                               /**< Mode LOC25 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC26            0x0000001AUL                               /**< Mode LOC26 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC27            0x0000001BUL                               /**< Mode LOC27 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC28            0x0000001CUL                               /**< Mode LOC28 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC29            0x0000001DUL                               /**< Mode LOC29 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC30            0x0000001EUL                               /**< Mode LOC30 for WTIMER_ROUTELOC2 */
#define _WTIMER_ROUTELOC2_CDTI2LOC_LOC31            0x0000001FUL                               /**< Mode LOC31 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC0              (_WTIMER_ROUTELOC2_CDTI2LOC_LOC0 << 16)    /**< Shifted mode LOC0 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_DEFAULT           (_WTIMER_ROUTELOC2_CDTI2LOC_DEFAULT << 16) /**< Shifted mode DEFAULT for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC1              (_WTIMER_ROUTELOC2_CDTI2LOC_LOC1 << 16)    /**< Shifted mode LOC1 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC2              (_WTIMER_ROUTELOC2_CDTI2LOC_LOC2 << 16)    /**< Shifted mode LOC2 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC3              (_WTIMER_ROUTELOC2_CDTI2LOC_LOC3 << 16)    /**< Shifted mode LOC3 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC4              (_WTIMER_ROUTELOC2_CDTI2LOC_LOC4 << 16)    /**< Shifted mode LOC4 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC5              (_WTIMER_ROUTELOC2_CDTI2LOC_LOC5 << 16)    /**< Shifted mode LOC5 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC6              (_WTIMER_ROUTELOC2_CDTI2LOC_LOC6 << 16)    /**< Shifted mode LOC6 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC7              (_WTIMER_ROUTELOC2_CDTI2LOC_LOC7 << 16)    /**< Shifted mode LOC7 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC8              (_WTIMER_ROUTELOC2_CDTI2LOC_LOC8 << 16)    /**< Shifted mode LOC8 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC9              (_WTIMER_ROUTELOC2_CDTI2LOC_LOC9 << 16)    /**< Shifted mode LOC9 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC10             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC10 << 16)   /**< Shifted mode LOC10 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC11             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC11 << 16)   /**< Shifted mode LOC11 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC12             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC12 << 16)   /**< Shifted mode LOC12 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC13             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC13 << 16)   /**< Shifted mode LOC13 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC14             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC14 << 16)   /**< Shifted mode LOC14 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC15             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC15 << 16)   /**< Shifted mode LOC15 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC16             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC16 << 16)   /**< Shifted mode LOC16 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC17             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC17 << 16)   /**< Shifted mode LOC17 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC18             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC18 << 16)   /**< Shifted mode LOC18 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC19             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC19 << 16)   /**< Shifted mode LOC19 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC20             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC20 << 16)   /**< Shifted mode LOC20 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC21             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC21 << 16)   /**< Shifted mode LOC21 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC22             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC22 << 16)   /**< Shifted mode LOC22 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC23             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC23 << 16)   /**< Shifted mode LOC23 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC24             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC24 << 16)   /**< Shifted mode LOC24 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC25             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC25 << 16)   /**< Shifted mode LOC25 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC26             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC26 << 16)   /**< Shifted mode LOC26 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC27             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC27 << 16)   /**< Shifted mode LOC27 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC28             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC28 << 16)   /**< Shifted mode LOC28 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC29             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC29 << 16)   /**< Shifted mode LOC29 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC30             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC30 << 16)   /**< Shifted mode LOC30 for WTIMER_ROUTELOC2 */
#define WTIMER_ROUTELOC2_CDTI2LOC_LOC31             (_WTIMER_ROUTELOC2_CDTI2LOC_LOC31 << 16)   /**< Shifted mode LOC31 for WTIMER_ROUTELOC2 */

/* Bit fields for WTIMER CC_CTRL */
#define _WTIMER_CC_CTRL_RESETVALUE                  0x00000000UL                                     /**< Default value for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_MASK                        0x7F0F3F17UL                                     /**< Mask for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_MODE_SHIFT                  0                                                /**< Shift value for TIMER_MODE */
#define _WTIMER_CC_CTRL_MODE_MASK                   0x3UL                                            /**< Bit mask for TIMER_MODE */
#define _WTIMER_CC_CTRL_MODE_DEFAULT                0x00000000UL                                     /**< Mode DEFAULT for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_MODE_OFF                    0x00000000UL                                     /**< Mode OFF for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_MODE_INPUTCAPTURE           0x00000001UL                                     /**< Mode INPUTCAPTURE for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_MODE_OUTPUTCOMPARE          0x00000002UL                                     /**< Mode OUTPUTCOMPARE for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_MODE_PWM                    0x00000003UL                                     /**< Mode PWM for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_MODE_DEFAULT                 (_WTIMER_CC_CTRL_MODE_DEFAULT << 0)              /**< Shifted mode DEFAULT for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_MODE_OFF                     (_WTIMER_CC_CTRL_MODE_OFF << 0)                  /**< Shifted mode OFF for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_MODE_INPUTCAPTURE            (_WTIMER_CC_CTRL_MODE_INPUTCAPTURE << 0)         /**< Shifted mode INPUTCAPTURE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_MODE_OUTPUTCOMPARE           (_WTIMER_CC_CTRL_MODE_OUTPUTCOMPARE << 0)        /**< Shifted mode OUTPUTCOMPARE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_MODE_PWM                     (_WTIMER_CC_CTRL_MODE_PWM << 0)                  /**< Shifted mode PWM for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_OUTINV                       (0x1UL << 2)                                     /**< Output Invert */
#define _WTIMER_CC_CTRL_OUTINV_SHIFT                2                                                /**< Shift value for TIMER_OUTINV */
#define _WTIMER_CC_CTRL_OUTINV_MASK                 0x4UL                                            /**< Bit mask for TIMER_OUTINV */
#define _WTIMER_CC_CTRL_OUTINV_DEFAULT              0x00000000UL                                     /**< Mode DEFAULT for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_OUTINV_DEFAULT               (_WTIMER_CC_CTRL_OUTINV_DEFAULT << 2)            /**< Shifted mode DEFAULT for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_COIST                        (0x1UL << 4)                                     /**< Compare Output Initial State */
#define _WTIMER_CC_CTRL_COIST_SHIFT                 4                                                /**< Shift value for TIMER_COIST */
#define _WTIMER_CC_CTRL_COIST_MASK                  0x10UL                                           /**< Bit mask for TIMER_COIST */
#define _WTIMER_CC_CTRL_COIST_DEFAULT               0x00000000UL                                     /**< Mode DEFAULT for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_COIST_DEFAULT                (_WTIMER_CC_CTRL_COIST_DEFAULT << 4)             /**< Shifted mode DEFAULT for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_CMOA_SHIFT                  8                                                /**< Shift value for TIMER_CMOA */
#define _WTIMER_CC_CTRL_CMOA_MASK                   0x300UL                                          /**< Bit mask for TIMER_CMOA */
#define _WTIMER_CC_CTRL_CMOA_DEFAULT                0x00000000UL                                     /**< Mode DEFAULT for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_CMOA_NONE                   0x00000000UL                                     /**< Mode NONE for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_CMOA_TOGGLE                 0x00000001UL                                     /**< Mode TOGGLE for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_CMOA_CLEAR                  0x00000002UL                                     /**< Mode CLEAR for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_CMOA_SET                    0x00000003UL                                     /**< Mode SET for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_CMOA_DEFAULT                 (_WTIMER_CC_CTRL_CMOA_DEFAULT << 8)              /**< Shifted mode DEFAULT for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_CMOA_NONE                    (_WTIMER_CC_CTRL_CMOA_NONE << 8)                 /**< Shifted mode NONE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_CMOA_TOGGLE                  (_WTIMER_CC_CTRL_CMOA_TOGGLE << 8)               /**< Shifted mode TOGGLE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_CMOA_CLEAR                   (_WTIMER_CC_CTRL_CMOA_CLEAR << 8)                /**< Shifted mode CLEAR for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_CMOA_SET                     (_WTIMER_CC_CTRL_CMOA_SET << 8)                  /**< Shifted mode SET for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_COFOA_SHIFT                 10                                               /**< Shift value for TIMER_COFOA */
#define _WTIMER_CC_CTRL_COFOA_MASK                  0xC00UL                                          /**< Bit mask for TIMER_COFOA */
#define _WTIMER_CC_CTRL_COFOA_DEFAULT               0x00000000UL                                     /**< Mode DEFAULT for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_COFOA_NONE                  0x00000000UL                                     /**< Mode NONE for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_COFOA_TOGGLE                0x00000001UL                                     /**< Mode TOGGLE for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_COFOA_CLEAR                 0x00000002UL                                     /**< Mode CLEAR for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_COFOA_SET                   0x00000003UL                                     /**< Mode SET for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_COFOA_DEFAULT                (_WTIMER_CC_CTRL_COFOA_DEFAULT << 10)            /**< Shifted mode DEFAULT for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_COFOA_NONE                   (_WTIMER_CC_CTRL_COFOA_NONE << 10)               /**< Shifted mode NONE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_COFOA_TOGGLE                 (_WTIMER_CC_CTRL_COFOA_TOGGLE << 10)             /**< Shifted mode TOGGLE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_COFOA_CLEAR                  (_WTIMER_CC_CTRL_COFOA_CLEAR << 10)              /**< Shifted mode CLEAR for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_COFOA_SET                    (_WTIMER_CC_CTRL_COFOA_SET << 10)                /**< Shifted mode SET for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_CUFOA_SHIFT                 12                                               /**< Shift value for TIMER_CUFOA */
#define _WTIMER_CC_CTRL_CUFOA_MASK                  0x3000UL                                         /**< Bit mask for TIMER_CUFOA */
#define _WTIMER_CC_CTRL_CUFOA_DEFAULT               0x00000000UL                                     /**< Mode DEFAULT for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_CUFOA_NONE                  0x00000000UL                                     /**< Mode NONE for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_CUFOA_TOGGLE                0x00000001UL                                     /**< Mode TOGGLE for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_CUFOA_CLEAR                 0x00000002UL                                     /**< Mode CLEAR for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_CUFOA_SET                   0x00000003UL                                     /**< Mode SET for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_CUFOA_DEFAULT                (_WTIMER_CC_CTRL_CUFOA_DEFAULT << 12)            /**< Shifted mode DEFAULT for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_CUFOA_NONE                   (_WTIMER_CC_CTRL_CUFOA_NONE << 12)               /**< Shifted mode NONE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_CUFOA_TOGGLE                 (_WTIMER_CC_CTRL_CUFOA_TOGGLE << 12)             /**< Shifted mode TOGGLE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_CUFOA_CLEAR                  (_WTIMER_CC_CTRL_CUFOA_CLEAR << 12)              /**< Shifted mode CLEAR for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_CUFOA_SET                    (_WTIMER_CC_CTRL_CUFOA_SET << 12)                /**< Shifted mode SET for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSSEL_SHIFT                16                                               /**< Shift value for TIMER_PRSSEL */
#define _WTIMER_CC_CTRL_PRSSEL_MASK                 0xF0000UL                                        /**< Bit mask for TIMER_PRSSEL */
#define _WTIMER_CC_CTRL_PRSSEL_DEFAULT              0x00000000UL                                     /**< Mode DEFAULT for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSSEL_PRSCH0               0x00000000UL                                     /**< Mode PRSCH0 for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSSEL_PRSCH1               0x00000001UL                                     /**< Mode PRSCH1 for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSSEL_PRSCH2               0x00000002UL                                     /**< Mode PRSCH2 for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSSEL_PRSCH3               0x00000003UL                                     /**< Mode PRSCH3 for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSSEL_PRSCH4               0x00000004UL                                     /**< Mode PRSCH4 for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSSEL_PRSCH5               0x00000005UL                                     /**< Mode PRSCH5 for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSSEL_PRSCH6               0x00000006UL                                     /**< Mode PRSCH6 for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSSEL_PRSCH7               0x00000007UL                                     /**< Mode PRSCH7 for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSSEL_PRSCH8               0x00000008UL                                     /**< Mode PRSCH8 for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSSEL_PRSCH9               0x00000009UL                                     /**< Mode PRSCH9 for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSSEL_PRSCH10              0x0000000AUL                                     /**< Mode PRSCH10 for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSSEL_PRSCH11              0x0000000BUL                                     /**< Mode PRSCH11 for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSSEL_DEFAULT               (_WTIMER_CC_CTRL_PRSSEL_DEFAULT << 16)           /**< Shifted mode DEFAULT for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSSEL_PRSCH0                (_WTIMER_CC_CTRL_PRSSEL_PRSCH0 << 16)            /**< Shifted mode PRSCH0 for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSSEL_PRSCH1                (_WTIMER_CC_CTRL_PRSSEL_PRSCH1 << 16)            /**< Shifted mode PRSCH1 for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSSEL_PRSCH2                (_WTIMER_CC_CTRL_PRSSEL_PRSCH2 << 16)            /**< Shifted mode PRSCH2 for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSSEL_PRSCH3                (_WTIMER_CC_CTRL_PRSSEL_PRSCH3 << 16)            /**< Shifted mode PRSCH3 for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSSEL_PRSCH4                (_WTIMER_CC_CTRL_PRSSEL_PRSCH4 << 16)            /**< Shifted mode PRSCH4 for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSSEL_PRSCH5                (_WTIMER_CC_CTRL_PRSSEL_PRSCH5 << 16)            /**< Shifted mode PRSCH5 for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSSEL_PRSCH6                (_WTIMER_CC_CTRL_PRSSEL_PRSCH6 << 16)            /**< Shifted mode PRSCH6 for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSSEL_PRSCH7                (_WTIMER_CC_CTRL_PRSSEL_PRSCH7 << 16)            /**< Shifted mode PRSCH7 for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSSEL_PRSCH8                (_WTIMER_CC_CTRL_PRSSEL_PRSCH8 << 16)            /**< Shifted mode PRSCH8 for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSSEL_PRSCH9                (_WTIMER_CC_CTRL_PRSSEL_PRSCH9 << 16)            /**< Shifted mode PRSCH9 for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSSEL_PRSCH10               (_WTIMER_CC_CTRL_PRSSEL_PRSCH10 << 16)           /**< Shifted mode PRSCH10 for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSSEL_PRSCH11               (_WTIMER_CC_CTRL_PRSSEL_PRSCH11 << 16)           /**< Shifted mode PRSCH11 for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_ICEDGE_SHIFT                24                                               /**< Shift value for TIMER_ICEDGE */
#define _WTIMER_CC_CTRL_ICEDGE_MASK                 0x3000000UL                                      /**< Bit mask for TIMER_ICEDGE */
#define _WTIMER_CC_CTRL_ICEDGE_DEFAULT              0x00000000UL                                     /**< Mode DEFAULT for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_ICEDGE_RISING               0x00000000UL                                     /**< Mode RISING for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_ICEDGE_FALLING              0x00000001UL                                     /**< Mode FALLING for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_ICEDGE_BOTH                 0x00000002UL                                     /**< Mode BOTH for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_ICEDGE_NONE                 0x00000003UL                                     /**< Mode NONE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_ICEDGE_DEFAULT               (_WTIMER_CC_CTRL_ICEDGE_DEFAULT << 24)           /**< Shifted mode DEFAULT for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_ICEDGE_RISING                (_WTIMER_CC_CTRL_ICEDGE_RISING << 24)            /**< Shifted mode RISING for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_ICEDGE_FALLING               (_WTIMER_CC_CTRL_ICEDGE_FALLING << 24)           /**< Shifted mode FALLING for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_ICEDGE_BOTH                  (_WTIMER_CC_CTRL_ICEDGE_BOTH << 24)              /**< Shifted mode BOTH for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_ICEDGE_NONE                  (_WTIMER_CC_CTRL_ICEDGE_NONE << 24)              /**< Shifted mode NONE for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_ICEVCTRL_SHIFT              26                                               /**< Shift value for TIMER_ICEVCTRL */
#define _WTIMER_CC_CTRL_ICEVCTRL_MASK               0xC000000UL                                      /**< Bit mask for TIMER_ICEVCTRL */
#define _WTIMER_CC_CTRL_ICEVCTRL_DEFAULT            0x00000000UL                                     /**< Mode DEFAULT for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_ICEVCTRL_EVERYEDGE          0x00000000UL                                     /**< Mode EVERYEDGE for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_ICEVCTRL_EVERYSECONDEDGE    0x00000001UL                                     /**< Mode EVERYSECONDEDGE for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_ICEVCTRL_RISING             0x00000002UL                                     /**< Mode RISING for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_ICEVCTRL_FALLING            0x00000003UL                                     /**< Mode FALLING for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_ICEVCTRL_DEFAULT             (_WTIMER_CC_CTRL_ICEVCTRL_DEFAULT << 26)         /**< Shifted mode DEFAULT for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_ICEVCTRL_EVERYEDGE           (_WTIMER_CC_CTRL_ICEVCTRL_EVERYEDGE << 26)       /**< Shifted mode EVERYEDGE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_ICEVCTRL_EVERYSECONDEDGE     (_WTIMER_CC_CTRL_ICEVCTRL_EVERYSECONDEDGE << 26) /**< Shifted mode EVERYSECONDEDGE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_ICEVCTRL_RISING              (_WTIMER_CC_CTRL_ICEVCTRL_RISING << 26)          /**< Shifted mode RISING for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_ICEVCTRL_FALLING             (_WTIMER_CC_CTRL_ICEVCTRL_FALLING << 26)         /**< Shifted mode FALLING for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSCONF                      (0x1UL << 28)                                    /**< PRS Configuration */
#define _WTIMER_CC_CTRL_PRSCONF_SHIFT               28                                               /**< Shift value for TIMER_PRSCONF */
#define _WTIMER_CC_CTRL_PRSCONF_MASK                0x10000000UL                                     /**< Bit mask for TIMER_PRSCONF */
#define _WTIMER_CC_CTRL_PRSCONF_DEFAULT             0x00000000UL                                     /**< Mode DEFAULT for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSCONF_PULSE               0x00000000UL                                     /**< Mode PULSE for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_PRSCONF_LEVEL               0x00000001UL                                     /**< Mode LEVEL for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSCONF_DEFAULT              (_WTIMER_CC_CTRL_PRSCONF_DEFAULT << 28)          /**< Shifted mode DEFAULT for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSCONF_PULSE                (_WTIMER_CC_CTRL_PRSCONF_PULSE << 28)            /**< Shifted mode PULSE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_PRSCONF_LEVEL                (_WTIMER_CC_CTRL_PRSCONF_LEVEL << 28)            /**< Shifted mode LEVEL for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_INSEL                        (0x1UL << 29)                                    /**< Input Selection */
#define _WTIMER_CC_CTRL_INSEL_SHIFT                 29                                               /**< Shift value for TIMER_INSEL */
#define _WTIMER_CC_CTRL_INSEL_MASK                  0x20000000UL                                     /**< Bit mask for TIMER_INSEL */
#define _WTIMER_CC_CTRL_INSEL_DEFAULT               0x00000000UL                                     /**< Mode DEFAULT for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_INSEL_PIN                   0x00000000UL                                     /**< Mode PIN for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_INSEL_PRS                   0x00000001UL                                     /**< Mode PRS for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_INSEL_DEFAULT                (_WTIMER_CC_CTRL_INSEL_DEFAULT << 29)            /**< Shifted mode DEFAULT for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_INSEL_PIN                    (_WTIMER_CC_CTRL_INSEL_PIN << 29)                /**< Shifted mode PIN for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_INSEL_PRS                    (_WTIMER_CC_CTRL_INSEL_PRS << 29)                /**< Shifted mode PRS for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_FILT                         (0x1UL << 30)                                    /**< Digital Filter */
#define _WTIMER_CC_CTRL_FILT_SHIFT                  30                                               /**< Shift value for TIMER_FILT */
#define _WTIMER_CC_CTRL_FILT_MASK                   0x40000000UL                                     /**< Bit mask for TIMER_FILT */
#define _WTIMER_CC_CTRL_FILT_DEFAULT                0x00000000UL                                     /**< Mode DEFAULT for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_FILT_DISABLE                0x00000000UL                                     /**< Mode DISABLE for WTIMER_CC_CTRL */
#define _WTIMER_CC_CTRL_FILT_ENABLE                 0x00000001UL                                     /**< Mode ENABLE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_FILT_DEFAULT                 (_WTIMER_CC_CTRL_FILT_DEFAULT << 30)             /**< Shifted mode DEFAULT for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_FILT_DISABLE                 (_WTIMER_CC_CTRL_FILT_DISABLE << 30)             /**< Shifted mode DISABLE for WTIMER_CC_CTRL */
#define WTIMER_CC_CTRL_FILT_ENABLE                  (_WTIMER_CC_CTRL_FILT_ENABLE << 30)              /**< Shifted mode ENABLE for WTIMER_CC_CTRL */

/* Bit fields for WTIMER CC_CCV */
#define _WTIMER_CC_CCV_RESETVALUE                   0x00000000UL                      /**< Default value for WTIMER_CC_CCV */
#define _WTIMER_CC_CCV_MASK                         0xFFFFFFFFUL                      /**< Mask for WTIMER_CC_CCV */
#define _WTIMER_CC_CCV_CCV_SHIFT                    0                                 /**< Shift value for TIMER_CCV */
#define _WTIMER_CC_CCV_CCV_MASK                     0xFFFFFFFFUL                      /**< Bit mask for TIMER_CCV */
#define _WTIMER_CC_CCV_CCV_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for WTIMER_CC_CCV */
#define WTIMER_CC_CCV_CCV_DEFAULT                   (_WTIMER_CC_CCV_CCV_DEFAULT << 0) /**< Shifted mode DEFAULT for WTIMER_CC_CCV */

/* Bit fields for WTIMER CC_CCVP */
#define _WTIMER_CC_CCVP_RESETVALUE                  0x00000000UL                        /**< Default value for WTIMER_CC_CCVP */
#define _WTIMER_CC_CCVP_MASK                        0xFFFFFFFFUL                        /**< Mask for WTIMER_CC_CCVP */
#define _WTIMER_CC_CCVP_CCVP_SHIFT                  0                                   /**< Shift value for TIMER_CCVP */
#define _WTIMER_CC_CCVP_CCVP_MASK                   0xFFFFFFFFUL                        /**< Bit mask for TIMER_CCVP */
#define _WTIMER_CC_CCVP_CCVP_DEFAULT                0x00000000UL                        /**< Mode DEFAULT for WTIMER_CC_CCVP */
#define WTIMER_CC_CCVP_CCVP_DEFAULT                 (_WTIMER_CC_CCVP_CCVP_DEFAULT << 0) /**< Shifted mode DEFAULT for WTIMER_CC_CCVP */

/* Bit fields for WTIMER CC_CCVB */
#define _WTIMER_CC_CCVB_RESETVALUE                  0x00000000UL                        /**< Default value for WTIMER_CC_CCVB */
#define _WTIMER_CC_CCVB_MASK                        0xFFFFFFFFUL                        /**< Mask for WTIMER_CC_CCVB */
#define _WTIMER_CC_CCVB_CCVB_SHIFT                  0                                   /**< Shift value for TIMER_CCVB */
#define _WTIMER_CC_CCVB_CCVB_MASK                   0xFFFFFFFFUL                        /**< Bit mask for TIMER_CCVB */
#define _WTIMER_CC_CCVB_CCVB_DEFAULT                0x00000000UL                        /**< Mode DEFAULT for WTIMER_CC_CCVB */
#define WTIMER_CC_CCVB_CCVB_DEFAULT                 (_WTIMER_CC_CCVB_CCVB_DEFAULT << 0) /**< Shifted mode DEFAULT for WTIMER_CC_CCVB */

/* Bit fields for WTIMER DTCTRL */
#define _WTIMER_DTCTRL_RESETVALUE                   0x00000000UL                           /**< Default value for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_MASK                         0x010006FFUL                           /**< Mask for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTEN                          (0x1UL << 0)                           /**< DTI Enable */
#define _WTIMER_DTCTRL_DTEN_SHIFT                   0                                      /**< Shift value for TIMER_DTEN */
#define _WTIMER_DTCTRL_DTEN_MASK                    0x1UL                                  /**< Bit mask for TIMER_DTEN */
#define _WTIMER_DTCTRL_DTEN_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTEN_DEFAULT                  (_WTIMER_DTCTRL_DTEN_DEFAULT << 0)     /**< Shifted mode DEFAULT for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTDAS                         (0x1UL << 1)                           /**< DTI Automatic Start-up Functionality */
#define _WTIMER_DTCTRL_DTDAS_SHIFT                  1                                      /**< Shift value for TIMER_DTDAS */
#define _WTIMER_DTCTRL_DTDAS_MASK                   0x2UL                                  /**< Bit mask for TIMER_DTDAS */
#define _WTIMER_DTCTRL_DTDAS_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTDAS_NORESTART              0x00000000UL                           /**< Mode NORESTART for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTDAS_RESTART                0x00000001UL                           /**< Mode RESTART for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTDAS_DEFAULT                 (_WTIMER_DTCTRL_DTDAS_DEFAULT << 1)    /**< Shifted mode DEFAULT for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTDAS_NORESTART               (_WTIMER_DTCTRL_DTDAS_NORESTART << 1)  /**< Shifted mode NORESTART for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTDAS_RESTART                 (_WTIMER_DTCTRL_DTDAS_RESTART << 1)    /**< Shifted mode RESTART for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTIPOL                        (0x1UL << 2)                           /**< DTI Inactive Polarity */
#define _WTIMER_DTCTRL_DTIPOL_SHIFT                 2                                      /**< Shift value for TIMER_DTIPOL */
#define _WTIMER_DTCTRL_DTIPOL_MASK                  0x4UL                                  /**< Bit mask for TIMER_DTIPOL */
#define _WTIMER_DTCTRL_DTIPOL_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTIPOL_DEFAULT                (_WTIMER_DTCTRL_DTIPOL_DEFAULT << 2)   /**< Shifted mode DEFAULT for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTCINV                        (0x1UL << 3)                           /**< DTI Complementary Output Invert */
#define _WTIMER_DTCTRL_DTCINV_SHIFT                 3                                      /**< Shift value for TIMER_DTCINV */
#define _WTIMER_DTCTRL_DTCINV_MASK                  0x8UL                                  /**< Bit mask for TIMER_DTCINV */
#define _WTIMER_DTCTRL_DTCINV_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTCINV_DEFAULT                (_WTIMER_DTCTRL_DTCINV_DEFAULT << 3)   /**< Shifted mode DEFAULT for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTPRSSEL_SHIFT               4                                      /**< Shift value for TIMER_DTPRSSEL */
#define _WTIMER_DTCTRL_DTPRSSEL_MASK                0xF0UL                                 /**< Bit mask for TIMER_DTPRSSEL */
#define _WTIMER_DTCTRL_DTPRSSEL_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTPRSSEL_PRSCH0              0x00000000UL                           /**< Mode PRSCH0 for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTPRSSEL_PRSCH1              0x00000001UL                           /**< Mode PRSCH1 for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTPRSSEL_PRSCH2              0x00000002UL                           /**< Mode PRSCH2 for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTPRSSEL_PRSCH3              0x00000003UL                           /**< Mode PRSCH3 for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTPRSSEL_PRSCH4              0x00000004UL                           /**< Mode PRSCH4 for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTPRSSEL_PRSCH5              0x00000005UL                           /**< Mode PRSCH5 for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTPRSSEL_PRSCH6              0x00000006UL                           /**< Mode PRSCH6 for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTPRSSEL_PRSCH7              0x00000007UL                           /**< Mode PRSCH7 for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTPRSSEL_PRSCH8              0x00000008UL                           /**< Mode PRSCH8 for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTPRSSEL_PRSCH9              0x00000009UL                           /**< Mode PRSCH9 for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTPRSSEL_PRSCH10             0x0000000AUL                           /**< Mode PRSCH10 for WTIMER_DTCTRL */
#define _WTIMER_DTCTRL_DTPRSSEL_PRSCH11             0x0000000BUL                           /**< Mode PRSCH11 for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSSEL_DEFAULT              (_WTIMER_DTCTRL_DTPRSSEL_DEFAULT << 4) /**< Shifted mode DEFAULT for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSSEL_PRSCH0               (_WTIMER_DTCTRL_DTPRSSEL_PRSCH0 << 4)  /**< Shifted mode PRSCH0 for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSSEL_PRSCH1               (_WTIMER_DTCTRL_DTPRSSEL_PRSCH1 << 4)  /**< Shifted mode PRSCH1 for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSSEL_PRSCH2               (_WTIMER_DTCTRL_DTPRSSEL_PRSCH2 << 4)  /**< Shifted mode PRSCH2 for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSSEL_PRSCH3               (_WTIMER_DTCTRL_DTPRSSEL_PRSCH3 << 4)  /**< Shifted mode PRSCH3 for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSSEL_PRSCH4               (_WTIMER_DTCTRL_DTPRSSEL_PRSCH4 << 4)  /**< Shifted mode PRSCH4 for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSSEL_PRSCH5               (_WTIMER_DTCTRL_DTPRSSEL_PRSCH5 << 4)  /**< Shifted mode PRSCH5 for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSSEL_PRSCH6               (_WTIMER_DTCTRL_DTPRSSEL_PRSCH6 << 4)  /**< Shifted mode PRSCH6 for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSSEL_PRSCH7               (_WTIMER_DTCTRL_DTPRSSEL_PRSCH7 << 4)  /**< Shifted mode PRSCH7 for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSSEL_PRSCH8               (_WTIMER_DTCTRL_DTPRSSEL_PRSCH8 << 4)  /**< Shifted mode PRSCH8 for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSSEL_PRSCH9               (_WTIMER_DTCTRL_DTPRSSEL_PRSCH9 << 4)  /**< Shifted mode PRSCH9 for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSSEL_PRSCH10              (_WTIMER_DTCTRL_DTPRSSEL_PRSCH10 << 4) /**< Shifted mode PRSCH10 for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSSEL_PRSCH11              (_WTIMER_DTCTRL_DTPRSSEL_PRSCH11 << 4) /**< Shifted mode PRSCH11 for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTAR                          (0x1UL << 9)                           /**< DTI Always Run */
#define _WTIMER_DTCTRL_DTAR_SHIFT                   9                                      /**< Shift value for TIMER_DTAR */
#define _WTIMER_DTCTRL_DTAR_MASK                    0x200UL                                /**< Bit mask for TIMER_DTAR */
#define _WTIMER_DTCTRL_DTAR_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTAR_DEFAULT                  (_WTIMER_DTCTRL_DTAR_DEFAULT << 9)     /**< Shifted mode DEFAULT for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTFATS                        (0x1UL << 10)                          /**< DTI Fault Action on Timer Stop */
#define _WTIMER_DTCTRL_DTFATS_SHIFT                 10                                     /**< Shift value for TIMER_DTFATS */
#define _WTIMER_DTCTRL_DTFATS_MASK                  0x400UL                                /**< Bit mask for TIMER_DTFATS */
#define _WTIMER_DTCTRL_DTFATS_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTFATS_DEFAULT                (_WTIMER_DTCTRL_DTFATS_DEFAULT << 10)  /**< Shifted mode DEFAULT for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSEN                       (0x1UL << 24)                          /**< DTI PRS Source Enable */
#define _WTIMER_DTCTRL_DTPRSEN_SHIFT                24                                     /**< Shift value for TIMER_DTPRSEN */
#define _WTIMER_DTCTRL_DTPRSEN_MASK                 0x1000000UL                            /**< Bit mask for TIMER_DTPRSEN */
#define _WTIMER_DTCTRL_DTPRSEN_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for WTIMER_DTCTRL */
#define WTIMER_DTCTRL_DTPRSEN_DEFAULT               (_WTIMER_DTCTRL_DTPRSEN_DEFAULT << 24) /**< Shifted mode DEFAULT for WTIMER_DTCTRL */

/* Bit fields for WTIMER DTTIME */
#define _WTIMER_DTTIME_RESETVALUE                   0x00000000UL                           /**< Default value for WTIMER_DTTIME */
#define _WTIMER_DTTIME_MASK                         0x003F3F0FUL                           /**< Mask for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTPRESC_SHIFT                0                                      /**< Shift value for TIMER_DTPRESC */
#define _WTIMER_DTTIME_DTPRESC_MASK                 0xFUL                                  /**< Bit mask for TIMER_DTPRESC */
#define _WTIMER_DTTIME_DTPRESC_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTPRESC_DIV1                 0x00000000UL                           /**< Mode DIV1 for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTPRESC_DIV2                 0x00000001UL                           /**< Mode DIV2 for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTPRESC_DIV4                 0x00000002UL                           /**< Mode DIV4 for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTPRESC_DIV8                 0x00000003UL                           /**< Mode DIV8 for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTPRESC_DIV16                0x00000004UL                           /**< Mode DIV16 for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTPRESC_DIV32                0x00000005UL                           /**< Mode DIV32 for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTPRESC_DIV64                0x00000006UL                           /**< Mode DIV64 for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTPRESC_DIV128               0x00000007UL                           /**< Mode DIV128 for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTPRESC_DIV256               0x00000008UL                           /**< Mode DIV256 for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTPRESC_DIV512               0x00000009UL                           /**< Mode DIV512 for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTPRESC_DIV1024              0x0000000AUL                           /**< Mode DIV1024 for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTPRESC_DEFAULT               (_WTIMER_DTTIME_DTPRESC_DEFAULT << 0)  /**< Shifted mode DEFAULT for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTPRESC_DIV1                  (_WTIMER_DTTIME_DTPRESC_DIV1 << 0)     /**< Shifted mode DIV1 for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTPRESC_DIV2                  (_WTIMER_DTTIME_DTPRESC_DIV2 << 0)     /**< Shifted mode DIV2 for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTPRESC_DIV4                  (_WTIMER_DTTIME_DTPRESC_DIV4 << 0)     /**< Shifted mode DIV4 for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTPRESC_DIV8                  (_WTIMER_DTTIME_DTPRESC_DIV8 << 0)     /**< Shifted mode DIV8 for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTPRESC_DIV16                 (_WTIMER_DTTIME_DTPRESC_DIV16 << 0)    /**< Shifted mode DIV16 for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTPRESC_DIV32                 (_WTIMER_DTTIME_DTPRESC_DIV32 << 0)    /**< Shifted mode DIV32 for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTPRESC_DIV64                 (_WTIMER_DTTIME_DTPRESC_DIV64 << 0)    /**< Shifted mode DIV64 for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTPRESC_DIV128                (_WTIMER_DTTIME_DTPRESC_DIV128 << 0)   /**< Shifted mode DIV128 for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTPRESC_DIV256                (_WTIMER_DTTIME_DTPRESC_DIV256 << 0)   /**< Shifted mode DIV256 for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTPRESC_DIV512                (_WTIMER_DTTIME_DTPRESC_DIV512 << 0)   /**< Shifted mode DIV512 for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTPRESC_DIV1024               (_WTIMER_DTTIME_DTPRESC_DIV1024 << 0)  /**< Shifted mode DIV1024 for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTRISET_SHIFT                8                                      /**< Shift value for TIMER_DTRISET */
#define _WTIMER_DTTIME_DTRISET_MASK                 0x3F00UL                               /**< Bit mask for TIMER_DTRISET */
#define _WTIMER_DTTIME_DTRISET_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTRISET_DEFAULT               (_WTIMER_DTTIME_DTRISET_DEFAULT << 8)  /**< Shifted mode DEFAULT for WTIMER_DTTIME */
#define _WTIMER_DTTIME_DTFALLT_SHIFT                16                                     /**< Shift value for TIMER_DTFALLT */
#define _WTIMER_DTTIME_DTFALLT_MASK                 0x3F0000UL                             /**< Bit mask for TIMER_DTFALLT */
#define _WTIMER_DTTIME_DTFALLT_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for WTIMER_DTTIME */
#define WTIMER_DTTIME_DTFALLT_DEFAULT               (_WTIMER_DTTIME_DTFALLT_DEFAULT << 16) /**< Shifted mode DEFAULT for WTIMER_DTTIME */

/* Bit fields for WTIMER DTFC */
#define _WTIMER_DTFC_RESETVALUE                     0x00000000UL                             /**< Default value for WTIMER_DTFC */
#define _WTIMER_DTFC_MASK                           0x0F030F0FUL                             /**< Mask for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS0FSEL_SHIFT               0                                        /**< Shift value for TIMER_DTPRS0FSEL */
#define _WTIMER_DTFC_DTPRS0FSEL_MASK                0xFUL                                    /**< Bit mask for TIMER_DTPRS0FSEL */
#define _WTIMER_DTFC_DTPRS0FSEL_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS0FSEL_PRSCH0              0x00000000UL                             /**< Mode PRSCH0 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS0FSEL_PRSCH1              0x00000001UL                             /**< Mode PRSCH1 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS0FSEL_PRSCH2              0x00000002UL                             /**< Mode PRSCH2 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS0FSEL_PRSCH3              0x00000003UL                             /**< Mode PRSCH3 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS0FSEL_PRSCH4              0x00000004UL                             /**< Mode PRSCH4 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS0FSEL_PRSCH5              0x00000005UL                             /**< Mode PRSCH5 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS0FSEL_PRSCH6              0x00000006UL                             /**< Mode PRSCH6 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS0FSEL_PRSCH7              0x00000007UL                             /**< Mode PRSCH7 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS0FSEL_PRSCH8              0x00000008UL                             /**< Mode PRSCH8 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS0FSEL_PRSCH9              0x00000009UL                             /**< Mode PRSCH9 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS0FSEL_PRSCH10             0x0000000AUL                             /**< Mode PRSCH10 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS0FSEL_PRSCH11             0x0000000BUL                             /**< Mode PRSCH11 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FSEL_DEFAULT              (_WTIMER_DTFC_DTPRS0FSEL_DEFAULT << 0)   /**< Shifted mode DEFAULT for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FSEL_PRSCH0               (_WTIMER_DTFC_DTPRS0FSEL_PRSCH0 << 0)    /**< Shifted mode PRSCH0 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FSEL_PRSCH1               (_WTIMER_DTFC_DTPRS0FSEL_PRSCH1 << 0)    /**< Shifted mode PRSCH1 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FSEL_PRSCH2               (_WTIMER_DTFC_DTPRS0FSEL_PRSCH2 << 0)    /**< Shifted mode PRSCH2 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FSEL_PRSCH3               (_WTIMER_DTFC_DTPRS0FSEL_PRSCH3 << 0)    /**< Shifted mode PRSCH3 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FSEL_PRSCH4               (_WTIMER_DTFC_DTPRS0FSEL_PRSCH4 << 0)    /**< Shifted mode PRSCH4 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FSEL_PRSCH5               (_WTIMER_DTFC_DTPRS0FSEL_PRSCH5 << 0)    /**< Shifted mode PRSCH5 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FSEL_PRSCH6               (_WTIMER_DTFC_DTPRS0FSEL_PRSCH6 << 0)    /**< Shifted mode PRSCH6 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FSEL_PRSCH7               (_WTIMER_DTFC_DTPRS0FSEL_PRSCH7 << 0)    /**< Shifted mode PRSCH7 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FSEL_PRSCH8               (_WTIMER_DTFC_DTPRS0FSEL_PRSCH8 << 0)    /**< Shifted mode PRSCH8 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FSEL_PRSCH9               (_WTIMER_DTFC_DTPRS0FSEL_PRSCH9 << 0)    /**< Shifted mode PRSCH9 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FSEL_PRSCH10              (_WTIMER_DTFC_DTPRS0FSEL_PRSCH10 << 0)   /**< Shifted mode PRSCH10 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FSEL_PRSCH11              (_WTIMER_DTFC_DTPRS0FSEL_PRSCH11 << 0)   /**< Shifted mode PRSCH11 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS1FSEL_SHIFT               8                                        /**< Shift value for TIMER_DTPRS1FSEL */
#define _WTIMER_DTFC_DTPRS1FSEL_MASK                0xF00UL                                  /**< Bit mask for TIMER_DTPRS1FSEL */
#define _WTIMER_DTFC_DTPRS1FSEL_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS1FSEL_PRSCH0              0x00000000UL                             /**< Mode PRSCH0 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS1FSEL_PRSCH1              0x00000001UL                             /**< Mode PRSCH1 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS1FSEL_PRSCH2              0x00000002UL                             /**< Mode PRSCH2 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS1FSEL_PRSCH3              0x00000003UL                             /**< Mode PRSCH3 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS1FSEL_PRSCH4              0x00000004UL                             /**< Mode PRSCH4 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS1FSEL_PRSCH5              0x00000005UL                             /**< Mode PRSCH5 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS1FSEL_PRSCH6              0x00000006UL                             /**< Mode PRSCH6 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS1FSEL_PRSCH7              0x00000007UL                             /**< Mode PRSCH7 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS1FSEL_PRSCH8              0x00000008UL                             /**< Mode PRSCH8 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS1FSEL_PRSCH9              0x00000009UL                             /**< Mode PRSCH9 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS1FSEL_PRSCH10             0x0000000AUL                             /**< Mode PRSCH10 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTPRS1FSEL_PRSCH11             0x0000000BUL                             /**< Mode PRSCH11 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FSEL_DEFAULT              (_WTIMER_DTFC_DTPRS1FSEL_DEFAULT << 8)   /**< Shifted mode DEFAULT for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FSEL_PRSCH0               (_WTIMER_DTFC_DTPRS1FSEL_PRSCH0 << 8)    /**< Shifted mode PRSCH0 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FSEL_PRSCH1               (_WTIMER_DTFC_DTPRS1FSEL_PRSCH1 << 8)    /**< Shifted mode PRSCH1 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FSEL_PRSCH2               (_WTIMER_DTFC_DTPRS1FSEL_PRSCH2 << 8)    /**< Shifted mode PRSCH2 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FSEL_PRSCH3               (_WTIMER_DTFC_DTPRS1FSEL_PRSCH3 << 8)    /**< Shifted mode PRSCH3 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FSEL_PRSCH4               (_WTIMER_DTFC_DTPRS1FSEL_PRSCH4 << 8)    /**< Shifted mode PRSCH4 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FSEL_PRSCH5               (_WTIMER_DTFC_DTPRS1FSEL_PRSCH5 << 8)    /**< Shifted mode PRSCH5 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FSEL_PRSCH6               (_WTIMER_DTFC_DTPRS1FSEL_PRSCH6 << 8)    /**< Shifted mode PRSCH6 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FSEL_PRSCH7               (_WTIMER_DTFC_DTPRS1FSEL_PRSCH7 << 8)    /**< Shifted mode PRSCH7 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FSEL_PRSCH8               (_WTIMER_DTFC_DTPRS1FSEL_PRSCH8 << 8)    /**< Shifted mode PRSCH8 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FSEL_PRSCH9               (_WTIMER_DTFC_DTPRS1FSEL_PRSCH9 << 8)    /**< Shifted mode PRSCH9 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FSEL_PRSCH10              (_WTIMER_DTFC_DTPRS1FSEL_PRSCH10 << 8)   /**< Shifted mode PRSCH10 for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FSEL_PRSCH11              (_WTIMER_DTFC_DTPRS1FSEL_PRSCH11 << 8)   /**< Shifted mode PRSCH11 for WTIMER_DTFC */
#define _WTIMER_DTFC_DTFA_SHIFT                     16                                       /**< Shift value for TIMER_DTFA */
#define _WTIMER_DTFC_DTFA_MASK                      0x30000UL                                /**< Bit mask for TIMER_DTFA */
#define _WTIMER_DTFC_DTFA_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for WTIMER_DTFC */
#define _WTIMER_DTFC_DTFA_NONE                      0x00000000UL                             /**< Mode NONE for WTIMER_DTFC */
#define _WTIMER_DTFC_DTFA_INACTIVE                  0x00000001UL                             /**< Mode INACTIVE for WTIMER_DTFC */
#define _WTIMER_DTFC_DTFA_CLEAR                     0x00000002UL                             /**< Mode CLEAR for WTIMER_DTFC */
#define _WTIMER_DTFC_DTFA_TRISTATE                  0x00000003UL                             /**< Mode TRISTATE for WTIMER_DTFC */
#define WTIMER_DTFC_DTFA_DEFAULT                    (_WTIMER_DTFC_DTFA_DEFAULT << 16)        /**< Shifted mode DEFAULT for WTIMER_DTFC */
#define WTIMER_DTFC_DTFA_NONE                       (_WTIMER_DTFC_DTFA_NONE << 16)           /**< Shifted mode NONE for WTIMER_DTFC */
#define WTIMER_DTFC_DTFA_INACTIVE                   (_WTIMER_DTFC_DTFA_INACTIVE << 16)       /**< Shifted mode INACTIVE for WTIMER_DTFC */
#define WTIMER_DTFC_DTFA_CLEAR                      (_WTIMER_DTFC_DTFA_CLEAR << 16)          /**< Shifted mode CLEAR for WTIMER_DTFC */
#define WTIMER_DTFC_DTFA_TRISTATE                   (_WTIMER_DTFC_DTFA_TRISTATE << 16)       /**< Shifted mode TRISTATE for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FEN                       (0x1UL << 24)                            /**< DTI PRS 0 Fault Enable */
#define _WTIMER_DTFC_DTPRS0FEN_SHIFT                24                                       /**< Shift value for TIMER_DTPRS0FEN */
#define _WTIMER_DTFC_DTPRS0FEN_MASK                 0x1000000UL                              /**< Bit mask for TIMER_DTPRS0FEN */
#define _WTIMER_DTFC_DTPRS0FEN_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS0FEN_DEFAULT               (_WTIMER_DTFC_DTPRS0FEN_DEFAULT << 24)   /**< Shifted mode DEFAULT for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FEN                       (0x1UL << 25)                            /**< DTI PRS 1 Fault Enable */
#define _WTIMER_DTFC_DTPRS1FEN_SHIFT                25                                       /**< Shift value for TIMER_DTPRS1FEN */
#define _WTIMER_DTFC_DTPRS1FEN_MASK                 0x2000000UL                              /**< Bit mask for TIMER_DTPRS1FEN */
#define _WTIMER_DTFC_DTPRS1FEN_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for WTIMER_DTFC */
#define WTIMER_DTFC_DTPRS1FEN_DEFAULT               (_WTIMER_DTFC_DTPRS1FEN_DEFAULT << 25)   /**< Shifted mode DEFAULT for WTIMER_DTFC */
#define WTIMER_DTFC_DTDBGFEN                        (0x1UL << 26)                            /**< DTI Debugger Fault Enable */
#define _WTIMER_DTFC_DTDBGFEN_SHIFT                 26                                       /**< Shift value for TIMER_DTDBGFEN */
#define _WTIMER_DTFC_DTDBGFEN_MASK                  0x4000000UL                              /**< Bit mask for TIMER_DTDBGFEN */
#define _WTIMER_DTFC_DTDBGFEN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for WTIMER_DTFC */
#define WTIMER_DTFC_DTDBGFEN_DEFAULT                (_WTIMER_DTFC_DTDBGFEN_DEFAULT << 26)    /**< Shifted mode DEFAULT for WTIMER_DTFC */
#define WTIMER_DTFC_DTLOCKUPFEN                     (0x1UL << 27)                            /**< DTI Lockup Fault Enable */
#define _WTIMER_DTFC_DTLOCKUPFEN_SHIFT              27                                       /**< Shift value for TIMER_DTLOCKUPFEN */
#define _WTIMER_DTFC_DTLOCKUPFEN_MASK               0x8000000UL                              /**< Bit mask for TIMER_DTLOCKUPFEN */
#define _WTIMER_DTFC_DTLOCKUPFEN_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for WTIMER_DTFC */
#define WTIMER_DTFC_DTLOCKUPFEN_DEFAULT             (_WTIMER_DTFC_DTLOCKUPFEN_DEFAULT << 27) /**< Shifted mode DEFAULT for WTIMER_DTFC */

/* Bit fields for WTIMER DTOGEN */
#define _WTIMER_DTOGEN_RESETVALUE                   0x00000000UL                              /**< Default value for WTIMER_DTOGEN */
#define _WTIMER_DTOGEN_MASK                         0x0000003FUL                              /**< Mask for WTIMER_DTOGEN */
#define WTIMER_DTOGEN_DTOGCC0EN                     (0x1UL << 0)                              /**< DTI CC0 Output Generation Enable */
#define _WTIMER_DTOGEN_DTOGCC0EN_SHIFT              0                                         /**< Shift value for TIMER_DTOGCC0EN */
#define _WTIMER_DTOGEN_DTOGCC0EN_MASK               0x1UL                                     /**< Bit mask for TIMER_DTOGCC0EN */
#define _WTIMER_DTOGEN_DTOGCC0EN_DEFAULT            0x00000000UL                              /**< Mode DEFAULT for WTIMER_DTOGEN */
#define WTIMER_DTOGEN_DTOGCC0EN_DEFAULT             (_WTIMER_DTOGEN_DTOGCC0EN_DEFAULT << 0)   /**< Shifted mode DEFAULT for WTIMER_DTOGEN */
#define WTIMER_DTOGEN_DTOGCC1EN                     (0x1UL << 1)                              /**< DTI CC1 Output Generation Enable */
#define _WTIMER_DTOGEN_DTOGCC1EN_SHIFT              1                                         /**< Shift value for TIMER_DTOGCC1EN */
#define _WTIMER_DTOGEN_DTOGCC1EN_MASK               0x2UL                                     /**< Bit mask for TIMER_DTOGCC1EN */
#define _WTIMER_DTOGEN_DTOGCC1EN_DEFAULT            0x00000000UL                              /**< Mode DEFAULT for WTIMER_DTOGEN */
#define WTIMER_DTOGEN_DTOGCC1EN_DEFAULT             (_WTIMER_DTOGEN_DTOGCC1EN_DEFAULT << 1)   /**< Shifted mode DEFAULT for WTIMER_DTOGEN */
#define WTIMER_DTOGEN_DTOGCC2EN                     (0x1UL << 2)                              /**< DTI CC2 Output Generation Enable */
#define _WTIMER_DTOGEN_DTOGCC2EN_SHIFT              2                                         /**< Shift value for TIMER_DTOGCC2EN */
#define _WTIMER_DTOGEN_DTOGCC2EN_MASK               0x4UL                                     /**< Bit mask for TIMER_DTOGCC2EN */
#define _WTIMER_DTOGEN_DTOGCC2EN_DEFAULT            0x00000000UL                              /**< Mode DEFAULT for WTIMER_DTOGEN */
#define WTIMER_DTOGEN_DTOGCC2EN_DEFAULT             (_WTIMER_DTOGEN_DTOGCC2EN_DEFAULT << 2)   /**< Shifted mode DEFAULT for WTIMER_DTOGEN */
#define WTIMER_DTOGEN_DTOGCDTI0EN                   (0x1UL << 3)                              /**< DTI CDTI0 Output Generation Enable */
#define _WTIMER_DTOGEN_DTOGCDTI0EN_SHIFT            3                                         /**< Shift value for TIMER_DTOGCDTI0EN */
#define _WTIMER_DTOGEN_DTOGCDTI0EN_MASK             0x8UL                                     /**< Bit mask for TIMER_DTOGCDTI0EN */
#define _WTIMER_DTOGEN_DTOGCDTI0EN_DEFAULT          0x00000000UL                              /**< Mode DEFAULT for WTIMER_DTOGEN */
#define WTIMER_DTOGEN_DTOGCDTI0EN_DEFAULT           (_WTIMER_DTOGEN_DTOGCDTI0EN_DEFAULT << 3) /**< Shifted mode DEFAULT for WTIMER_DTOGEN */
#define WTIMER_DTOGEN_DTOGCDTI1EN                   (0x1UL << 4)                              /**< DTI CDTI1 Output Generation Enable */
#define _WTIMER_DTOGEN_DTOGCDTI1EN_SHIFT            4                                         /**< Shift value for TIMER_DTOGCDTI1EN */
#define _WTIMER_DTOGEN_DTOGCDTI1EN_MASK             0x10UL                                    /**< Bit mask for TIMER_DTOGCDTI1EN */
#define _WTIMER_DTOGEN_DTOGCDTI1EN_DEFAULT          0x00000000UL                              /**< Mode DEFAULT for WTIMER_DTOGEN */
#define WTIMER_DTOGEN_DTOGCDTI1EN_DEFAULT           (_WTIMER_DTOGEN_DTOGCDTI1EN_DEFAULT << 4) /**< Shifted mode DEFAULT for WTIMER_DTOGEN */
#define WTIMER_DTOGEN_DTOGCDTI2EN                   (0x1UL << 5)                              /**< DTI CDTI2 Output Generation Enable */
#define _WTIMER_DTOGEN_DTOGCDTI2EN_SHIFT            5                                         /**< Shift value for TIMER_DTOGCDTI2EN */
#define _WTIMER_DTOGEN_DTOGCDTI2EN_MASK             0x20UL                                    /**< Bit mask for TIMER_DTOGCDTI2EN */
#define _WTIMER_DTOGEN_DTOGCDTI2EN_DEFAULT          0x00000000UL                              /**< Mode DEFAULT for WTIMER_DTOGEN */
#define WTIMER_DTOGEN_DTOGCDTI2EN_DEFAULT           (_WTIMER_DTOGEN_DTOGCDTI2EN_DEFAULT << 5) /**< Shifted mode DEFAULT for WTIMER_DTOGEN */

/* Bit fields for WTIMER DTFAULT */
#define _WTIMER_DTFAULT_RESETVALUE                  0x00000000UL                             /**< Default value for WTIMER_DTFAULT */
#define _WTIMER_DTFAULT_MASK                        0x0000000FUL                             /**< Mask for WTIMER_DTFAULT */
#define WTIMER_DTFAULT_DTPRS0F                      (0x1UL << 0)                             /**< DTI PRS 0 Fault */
#define _WTIMER_DTFAULT_DTPRS0F_SHIFT               0                                        /**< Shift value for TIMER_DTPRS0F */
#define _WTIMER_DTFAULT_DTPRS0F_MASK                0x1UL                                    /**< Bit mask for TIMER_DTPRS0F */
#define _WTIMER_DTFAULT_DTPRS0F_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for WTIMER_DTFAULT */
#define WTIMER_DTFAULT_DTPRS0F_DEFAULT              (_WTIMER_DTFAULT_DTPRS0F_DEFAULT << 0)   /**< Shifted mode DEFAULT for WTIMER_DTFAULT */
#define WTIMER_DTFAULT_DTPRS1F                      (0x1UL << 1)                             /**< DTI PRS 1 Fault */
#define _WTIMER_DTFAULT_DTPRS1F_SHIFT               1                                        /**< Shift value for TIMER_DTPRS1F */
#define _WTIMER_DTFAULT_DTPRS1F_MASK                0x2UL                                    /**< Bit mask for TIMER_DTPRS1F */
#define _WTIMER_DTFAULT_DTPRS1F_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for WTIMER_DTFAULT */
#define WTIMER_DTFAULT_DTPRS1F_DEFAULT              (_WTIMER_DTFAULT_DTPRS1F_DEFAULT << 1)   /**< Shifted mode DEFAULT for WTIMER_DTFAULT */
#define WTIMER_DTFAULT_DTDBGF                       (0x1UL << 2)                             /**< DTI Debugger Fault */
#define _WTIMER_DTFAULT_DTDBGF_SHIFT                2                                        /**< Shift value for TIMER_DTDBGF */
#define _WTIMER_DTFAULT_DTDBGF_MASK                 0x4UL                                    /**< Bit mask for TIMER_DTDBGF */
#define _WTIMER_DTFAULT_DTDBGF_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for WTIMER_DTFAULT */
#define WTIMER_DTFAULT_DTDBGF_DEFAULT               (_WTIMER_DTFAULT_DTDBGF_DEFAULT << 2)    /**< Shifted mode DEFAULT for WTIMER_DTFAULT */
#define WTIMER_DTFAULT_DTLOCKUPF                    (0x1UL << 3)                             /**< DTI Lockup Fault */
#define _WTIMER_DTFAULT_DTLOCKUPF_SHIFT             3                                        /**< Shift value for TIMER_DTLOCKUPF */
#define _WTIMER_DTFAULT_DTLOCKUPF_MASK              0x8UL                                    /**< Bit mask for TIMER_DTLOCKUPF */
#define _WTIMER_DTFAULT_DTLOCKUPF_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for WTIMER_DTFAULT */
#define WTIMER_DTFAULT_DTLOCKUPF_DEFAULT            (_WTIMER_DTFAULT_DTLOCKUPF_DEFAULT << 3) /**< Shifted mode DEFAULT for WTIMER_DTFAULT */

/* Bit fields for WTIMER DTFAULTC */
#define _WTIMER_DTFAULTC_RESETVALUE                 0x00000000UL                              /**< Default value for WTIMER_DTFAULTC */
#define _WTIMER_DTFAULTC_MASK                       0x0000000FUL                              /**< Mask for WTIMER_DTFAULTC */
#define WTIMER_DTFAULTC_DTPRS0FC                    (0x1UL << 0)                              /**< DTI PRS0 Fault Clear */
#define _WTIMER_DTFAULTC_DTPRS0FC_SHIFT             0                                         /**< Shift value for TIMER_DTPRS0FC */
#define _WTIMER_DTFAULTC_DTPRS0FC_MASK              0x1UL                                     /**< Bit mask for TIMER_DTPRS0FC */
#define _WTIMER_DTFAULTC_DTPRS0FC_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for WTIMER_DTFAULTC */
#define WTIMER_DTFAULTC_DTPRS0FC_DEFAULT            (_WTIMER_DTFAULTC_DTPRS0FC_DEFAULT << 0)  /**< Shifted mode DEFAULT for WTIMER_DTFAULTC */
#define WTIMER_DTFAULTC_DTPRS1FC                    (0x1UL << 1)                              /**< DTI PRS1 Fault Clear */
#define _WTIMER_DTFAULTC_DTPRS1FC_SHIFT             1                                         /**< Shift value for TIMER_DTPRS1FC */
#define _WTIMER_DTFAULTC_DTPRS1FC_MASK              0x2UL                                     /**< Bit mask for TIMER_DTPRS1FC */
#define _WTIMER_DTFAULTC_DTPRS1FC_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for WTIMER_DTFAULTC */
#define WTIMER_DTFAULTC_DTPRS1FC_DEFAULT            (_WTIMER_DTFAULTC_DTPRS1FC_DEFAULT << 1)  /**< Shifted mode DEFAULT for WTIMER_DTFAULTC */
#define WTIMER_DTFAULTC_DTDBGFC                     (0x1UL << 2)                              /**< DTI Debugger Fault Clear */
#define _WTIMER_DTFAULTC_DTDBGFC_SHIFT              2                                         /**< Shift value for TIMER_DTDBGFC */
#define _WTIMER_DTFAULTC_DTDBGFC_MASK               0x4UL                                     /**< Bit mask for TIMER_DTDBGFC */
#define _WTIMER_DTFAULTC_DTDBGFC_DEFAULT            0x00000000UL                              /**< Mode DEFAULT for WTIMER_DTFAULTC */
#define WTIMER_DTFAULTC_DTDBGFC_DEFAULT             (_WTIMER_DTFAULTC_DTDBGFC_DEFAULT << 2)   /**< Shifted mode DEFAULT for WTIMER_DTFAULTC */
#define WTIMER_DTFAULTC_TLOCKUPFC                   (0x1UL << 3)                              /**< DTI Lockup Fault Clear */
#define _WTIMER_DTFAULTC_TLOCKUPFC_SHIFT            3                                         /**< Shift value for TIMER_TLOCKUPFC */
#define _WTIMER_DTFAULTC_TLOCKUPFC_MASK             0x8UL                                     /**< Bit mask for TIMER_TLOCKUPFC */
#define _WTIMER_DTFAULTC_TLOCKUPFC_DEFAULT          0x00000000UL                              /**< Mode DEFAULT for WTIMER_DTFAULTC */
#define WTIMER_DTFAULTC_TLOCKUPFC_DEFAULT           (_WTIMER_DTFAULTC_TLOCKUPFC_DEFAULT << 3) /**< Shifted mode DEFAULT for WTIMER_DTFAULTC */

/* Bit fields for WTIMER DTLOCK */
#define _WTIMER_DTLOCK_RESETVALUE                   0x00000000UL                           /**< Default value for WTIMER_DTLOCK */
#define _WTIMER_DTLOCK_MASK                         0x0000FFFFUL                           /**< Mask for WTIMER_DTLOCK */
#define _WTIMER_DTLOCK_LOCKKEY_SHIFT                0                                      /**< Shift value for TIMER_LOCKKEY */
#define _WTIMER_DTLOCK_LOCKKEY_MASK                 0xFFFFUL                               /**< Bit mask for TIMER_LOCKKEY */
#define _WTIMER_DTLOCK_LOCKKEY_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for WTIMER_DTLOCK */
#define _WTIMER_DTLOCK_LOCKKEY_LOCK                 0x00000000UL                           /**< Mode LOCK for WTIMER_DTLOCK */
#define _WTIMER_DTLOCK_LOCKKEY_UNLOCKED             0x00000000UL                           /**< Mode UNLOCKED for WTIMER_DTLOCK */
#define _WTIMER_DTLOCK_LOCKKEY_LOCKED               0x00000001UL                           /**< Mode LOCKED for WTIMER_DTLOCK */
#define _WTIMER_DTLOCK_LOCKKEY_UNLOCK               0x0000CE80UL                           /**< Mode UNLOCK for WTIMER_DTLOCK */
#define WTIMER_DTLOCK_LOCKKEY_DEFAULT               (_WTIMER_DTLOCK_LOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for WTIMER_DTLOCK */
#define WTIMER_DTLOCK_LOCKKEY_LOCK                  (_WTIMER_DTLOCK_LOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for WTIMER_DTLOCK */
#define WTIMER_DTLOCK_LOCKKEY_UNLOCKED              (_WTIMER_DTLOCK_LOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for WTIMER_DTLOCK */
#define WTIMER_DTLOCK_LOCKKEY_LOCKED                (_WTIMER_DTLOCK_LOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for WTIMER_DTLOCK */
#define WTIMER_DTLOCK_LOCKKEY_UNLOCK                (_WTIMER_DTLOCK_LOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for WTIMER_DTLOCK */

/** @} */
/** @} End of group EFR32MG12P232F1024GM68_WTIMER */

/**************************************************************************//**
 * @defgroup EFR32MG12P232F1024GM68_UNLOCK Unlock Codes
 * @{
 *****************************************************************************/
#define MSC_UNLOCK_CODE      0x1B71 /**< MSC unlock code */
#define EMU_UNLOCK_CODE      0xADE8 /**< EMU unlock code */
#define RMU_UNLOCK_CODE      0xE084 /**< RMU unlock code */
#define CMU_UNLOCK_CODE      0x580E /**< CMU unlock code */
#define GPIO_UNLOCK_CODE     0xA534 /**< GPIO unlock code */
#define TIMER_UNLOCK_CODE    0xCE80 /**< TIMER unlock code */
#define RTCC_UNLOCK_CODE     0xAEE8 /**< RTCC unlock code */

/** @} End of group EFR32MG12P232F1024GM68_UNLOCK */

/** @} End of group EFR32MG12P232F1024GM68_BitFields */

#include "efr32mg12p_af_ports.h"
#include "efr32mg12p_af_pins.h"

/** @} End of group EFR32MG12P232F1024GM68 */

/** @} End of group Parts */

#ifdef __cplusplus
}
#endif
#endif /* EFR32MG12P232F1024GM68_H */
