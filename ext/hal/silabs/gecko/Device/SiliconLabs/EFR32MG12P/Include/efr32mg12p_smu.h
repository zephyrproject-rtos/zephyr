/**************************************************************************//**
 * @file efr32mg12p_smu.h
 * @brief EFR32MG12P_SMU register and bit field definitions
 * @version 5.5.0
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

/**************************************************************************//**
* @addtogroup Parts
* @{
******************************************************************************/
/**************************************************************************//**
 * @defgroup EFR32MG12P_SMU SMU
 * @{
 * @brief EFR32MG12P_SMU Register Declaration
 *****************************************************************************/
/** SMU Register Declaration */
typedef struct {
  uint32_t       RESERVED0[3];  /**< Reserved for future use **/
  __IM uint32_t  IF;            /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;           /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;           /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;           /**< Interrupt Enable Register  */

  uint32_t       RESERVED1[9];  /**< Reserved for future use **/
  __IOM uint32_t PPUCTRL;       /**< PPU Control Register  */
  uint32_t       RESERVED2[3];  /**< Reserved for future use **/
  __IOM uint32_t PPUPATD0;      /**< PPU Privilege Access Type Descriptor 0  */
  __IOM uint32_t PPUPATD1;      /**< PPU Privilege Access Type Descriptor 1  */

  uint32_t       RESERVED3[14]; /**< Reserved for future use **/
  __IM uint32_t  PPUFS;         /**< PPU Fault Status  */
} SMU_TypeDef;                  /** @} */

/**************************************************************************//**
 * @addtogroup EFR32MG12P_SMU
 * @{
 * @defgroup EFR32MG12P_SMU_BitFields  SMU Bit Fields
 * @{
 *****************************************************************************/

/* Bit fields for SMU IF */
#define _SMU_IF_RESETVALUE                 0x00000000UL                   /**< Default value for SMU_IF */
#define _SMU_IF_MASK                       0x00000001UL                   /**< Mask for SMU_IF */
#define SMU_IF_PPUPRIV                     (0x1UL << 0)                   /**< PPU Privilege Interrupt Flag */
#define _SMU_IF_PPUPRIV_SHIFT              0                              /**< Shift value for SMU_PPUPRIV */
#define _SMU_IF_PPUPRIV_MASK               0x1UL                          /**< Bit mask for SMU_PPUPRIV */
#define _SMU_IF_PPUPRIV_DEFAULT            0x00000000UL                   /**< Mode DEFAULT for SMU_IF */
#define SMU_IF_PPUPRIV_DEFAULT             (_SMU_IF_PPUPRIV_DEFAULT << 0) /**< Shifted mode DEFAULT for SMU_IF */

/* Bit fields for SMU IFS */
#define _SMU_IFS_RESETVALUE                0x00000000UL                    /**< Default value for SMU_IFS */
#define _SMU_IFS_MASK                      0x00000001UL                    /**< Mask for SMU_IFS */
#define SMU_IFS_PPUPRIV                    (0x1UL << 0)                    /**< Set PPUPRIV Interrupt Flag */
#define _SMU_IFS_PPUPRIV_SHIFT             0                               /**< Shift value for SMU_PPUPRIV */
#define _SMU_IFS_PPUPRIV_MASK              0x1UL                           /**< Bit mask for SMU_PPUPRIV */
#define _SMU_IFS_PPUPRIV_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for SMU_IFS */
#define SMU_IFS_PPUPRIV_DEFAULT            (_SMU_IFS_PPUPRIV_DEFAULT << 0) /**< Shifted mode DEFAULT for SMU_IFS */

/* Bit fields for SMU IFC */
#define _SMU_IFC_RESETVALUE                0x00000000UL                    /**< Default value for SMU_IFC */
#define _SMU_IFC_MASK                      0x00000001UL                    /**< Mask for SMU_IFC */
#define SMU_IFC_PPUPRIV                    (0x1UL << 0)                    /**< Clear PPUPRIV Interrupt Flag */
#define _SMU_IFC_PPUPRIV_SHIFT             0                               /**< Shift value for SMU_PPUPRIV */
#define _SMU_IFC_PPUPRIV_MASK              0x1UL                           /**< Bit mask for SMU_PPUPRIV */
#define _SMU_IFC_PPUPRIV_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for SMU_IFC */
#define SMU_IFC_PPUPRIV_DEFAULT            (_SMU_IFC_PPUPRIV_DEFAULT << 0) /**< Shifted mode DEFAULT for SMU_IFC */

/* Bit fields for SMU IEN */
#define _SMU_IEN_RESETVALUE                0x00000000UL                    /**< Default value for SMU_IEN */
#define _SMU_IEN_MASK                      0x00000001UL                    /**< Mask for SMU_IEN */
#define SMU_IEN_PPUPRIV                    (0x1UL << 0)                    /**< PPUPRIV Interrupt Enable */
#define _SMU_IEN_PPUPRIV_SHIFT             0                               /**< Shift value for SMU_PPUPRIV */
#define _SMU_IEN_PPUPRIV_MASK              0x1UL                           /**< Bit mask for SMU_PPUPRIV */
#define _SMU_IEN_PPUPRIV_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for SMU_IEN */
#define SMU_IEN_PPUPRIV_DEFAULT            (_SMU_IEN_PPUPRIV_DEFAULT << 0) /**< Shifted mode DEFAULT for SMU_IEN */

/* Bit fields for SMU PPUCTRL */
#define _SMU_PPUCTRL_RESETVALUE            0x00000000UL                       /**< Default value for SMU_PPUCTRL */
#define _SMU_PPUCTRL_MASK                  0x00000001UL                       /**< Mask for SMU_PPUCTRL */
#define SMU_PPUCTRL_ENABLE                 (0x1UL << 0)                       /**<  */
#define _SMU_PPUCTRL_ENABLE_SHIFT          0                                  /**< Shift value for SMU_ENABLE */
#define _SMU_PPUCTRL_ENABLE_MASK           0x1UL                              /**< Bit mask for SMU_ENABLE */
#define _SMU_PPUCTRL_ENABLE_DEFAULT        0x00000000UL                       /**< Mode DEFAULT for SMU_PPUCTRL */
#define SMU_PPUCTRL_ENABLE_DEFAULT         (_SMU_PPUCTRL_ENABLE_DEFAULT << 0) /**< Shifted mode DEFAULT for SMU_PPUCTRL */

/* Bit fields for SMU PPUPATD0 */
#define _SMU_PPUPATD0_RESETVALUE           0x00000000UL                           /**< Default value for SMU_PPUPATD0 */
#define _SMU_PPUPATD0_MASK                 0x3BFF7FA7UL                           /**< Mask for SMU_PPUPATD0 */
#define SMU_PPUPATD0_ACMP0                 (0x1UL << 0)                           /**< Analog Comparator 0 access control bit */
#define _SMU_PPUPATD0_ACMP0_SHIFT          0                                      /**< Shift value for SMU_ACMP0 */
#define _SMU_PPUPATD0_ACMP0_MASK           0x1UL                                  /**< Bit mask for SMU_ACMP0 */
#define _SMU_PPUPATD0_ACMP0_DEFAULT        0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_ACMP0_DEFAULT         (_SMU_PPUPATD0_ACMP0_DEFAULT << 0)     /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_ACMP1                 (0x1UL << 1)                           /**< Analog Comparator 1 access control bit */
#define _SMU_PPUPATD0_ACMP1_SHIFT          1                                      /**< Shift value for SMU_ACMP1 */
#define _SMU_PPUPATD0_ACMP1_MASK           0x2UL                                  /**< Bit mask for SMU_ACMP1 */
#define _SMU_PPUPATD0_ACMP1_DEFAULT        0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_ACMP1_DEFAULT         (_SMU_PPUPATD0_ACMP1_DEFAULT << 1)     /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_ADC0                  (0x1UL << 2)                           /**< Analog to Digital Converter 0 access control bit */
#define _SMU_PPUPATD0_ADC0_SHIFT           2                                      /**< Shift value for SMU_ADC0 */
#define _SMU_PPUPATD0_ADC0_MASK            0x4UL                                  /**< Bit mask for SMU_ADC0 */
#define _SMU_PPUPATD0_ADC0_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_ADC0_DEFAULT          (_SMU_PPUPATD0_ADC0_DEFAULT << 2)      /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_CMU                   (0x1UL << 5)                           /**< Clock Management Unit access control bit */
#define _SMU_PPUPATD0_CMU_SHIFT            5                                      /**< Shift value for SMU_CMU */
#define _SMU_PPUPATD0_CMU_MASK             0x20UL                                 /**< Bit mask for SMU_CMU */
#define _SMU_PPUPATD0_CMU_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_CMU_DEFAULT           (_SMU_PPUPATD0_CMU_DEFAULT << 5)       /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_CRYOTIMER             (0x1UL << 7)                           /**< CryoTimer access control bit */
#define _SMU_PPUPATD0_CRYOTIMER_SHIFT      7                                      /**< Shift value for SMU_CRYOTIMER */
#define _SMU_PPUPATD0_CRYOTIMER_MASK       0x80UL                                 /**< Bit mask for SMU_CRYOTIMER */
#define _SMU_PPUPATD0_CRYOTIMER_DEFAULT    0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_CRYOTIMER_DEFAULT     (_SMU_PPUPATD0_CRYOTIMER_DEFAULT << 7) /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_CRYPTO0               (0x1UL << 8)                           /**< Advanced Encryption Standard Accelerator 0 access control bit */
#define _SMU_PPUPATD0_CRYPTO0_SHIFT        8                                      /**< Shift value for SMU_CRYPTO0 */
#define _SMU_PPUPATD0_CRYPTO0_MASK         0x100UL                                /**< Bit mask for SMU_CRYPTO0 */
#define _SMU_PPUPATD0_CRYPTO0_DEFAULT      0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_CRYPTO0_DEFAULT       (_SMU_PPUPATD0_CRYPTO0_DEFAULT << 8)   /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_CRYPTO1               (0x1UL << 9)                           /**< Advanced Encryption Standard Accelerator 1 access control bit */
#define _SMU_PPUPATD0_CRYPTO1_SHIFT        9                                      /**< Shift value for SMU_CRYPTO1 */
#define _SMU_PPUPATD0_CRYPTO1_MASK         0x200UL                                /**< Bit mask for SMU_CRYPTO1 */
#define _SMU_PPUPATD0_CRYPTO1_DEFAULT      0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_CRYPTO1_DEFAULT       (_SMU_PPUPATD0_CRYPTO1_DEFAULT << 9)   /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_CSEN                  (0x1UL << 10)                          /**< Capacitive touch sense module access control bit */
#define _SMU_PPUPATD0_CSEN_SHIFT           10                                     /**< Shift value for SMU_CSEN */
#define _SMU_PPUPATD0_CSEN_MASK            0x400UL                                /**< Bit mask for SMU_CSEN */
#define _SMU_PPUPATD0_CSEN_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_CSEN_DEFAULT          (_SMU_PPUPATD0_CSEN_DEFAULT << 10)     /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_VDAC0                 (0x1UL << 11)                          /**< Digital to Analog Converter 0 access control bit */
#define _SMU_PPUPATD0_VDAC0_SHIFT          11                                     /**< Shift value for SMU_VDAC0 */
#define _SMU_PPUPATD0_VDAC0_MASK           0x800UL                                /**< Bit mask for SMU_VDAC0 */
#define _SMU_PPUPATD0_VDAC0_DEFAULT        0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_VDAC0_DEFAULT         (_SMU_PPUPATD0_VDAC0_DEFAULT << 11)    /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_PRS                   (0x1UL << 12)                          /**< Peripheral Reflex System access control bit */
#define _SMU_PPUPATD0_PRS_SHIFT            12                                     /**< Shift value for SMU_PRS */
#define _SMU_PPUPATD0_PRS_MASK             0x1000UL                               /**< Bit mask for SMU_PRS */
#define _SMU_PPUPATD0_PRS_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_PRS_DEFAULT           (_SMU_PPUPATD0_PRS_DEFAULT << 12)      /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_EMU                   (0x1UL << 13)                          /**< Energy Management Unit access control bit */
#define _SMU_PPUPATD0_EMU_SHIFT            13                                     /**< Shift value for SMU_EMU */
#define _SMU_PPUPATD0_EMU_MASK             0x2000UL                               /**< Bit mask for SMU_EMU */
#define _SMU_PPUPATD0_EMU_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_EMU_DEFAULT           (_SMU_PPUPATD0_EMU_DEFAULT << 13)      /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_FPUEH                 (0x1UL << 14)                          /**< FPU Exception Handler access control bit */
#define _SMU_PPUPATD0_FPUEH_SHIFT          14                                     /**< Shift value for SMU_FPUEH */
#define _SMU_PPUPATD0_FPUEH_MASK           0x4000UL                               /**< Bit mask for SMU_FPUEH */
#define _SMU_PPUPATD0_FPUEH_DEFAULT        0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_FPUEH_DEFAULT         (_SMU_PPUPATD0_FPUEH_DEFAULT << 14)    /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_GPCRC                 (0x1UL << 16)                          /**< General Purpose CRC access control bit */
#define _SMU_PPUPATD0_GPCRC_SHIFT          16                                     /**< Shift value for SMU_GPCRC */
#define _SMU_PPUPATD0_GPCRC_MASK           0x10000UL                              /**< Bit mask for SMU_GPCRC */
#define _SMU_PPUPATD0_GPCRC_DEFAULT        0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_GPCRC_DEFAULT         (_SMU_PPUPATD0_GPCRC_DEFAULT << 16)    /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_GPIO                  (0x1UL << 17)                          /**< General purpose Input/Output access control bit */
#define _SMU_PPUPATD0_GPIO_SHIFT           17                                     /**< Shift value for SMU_GPIO */
#define _SMU_PPUPATD0_GPIO_MASK            0x20000UL                              /**< Bit mask for SMU_GPIO */
#define _SMU_PPUPATD0_GPIO_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_GPIO_DEFAULT          (_SMU_PPUPATD0_GPIO_DEFAULT << 17)     /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_I2C0                  (0x1UL << 18)                          /**< I2C 0 access control bit */
#define _SMU_PPUPATD0_I2C0_SHIFT           18                                     /**< Shift value for SMU_I2C0 */
#define _SMU_PPUPATD0_I2C0_MASK            0x40000UL                              /**< Bit mask for SMU_I2C0 */
#define _SMU_PPUPATD0_I2C0_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_I2C0_DEFAULT          (_SMU_PPUPATD0_I2C0_DEFAULT << 18)     /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_I2C1                  (0x1UL << 19)                          /**< I2C 1 access control bit */
#define _SMU_PPUPATD0_I2C1_SHIFT           19                                     /**< Shift value for SMU_I2C1 */
#define _SMU_PPUPATD0_I2C1_MASK            0x80000UL                              /**< Bit mask for SMU_I2C1 */
#define _SMU_PPUPATD0_I2C1_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_I2C1_DEFAULT          (_SMU_PPUPATD0_I2C1_DEFAULT << 19)     /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_IDAC0                 (0x1UL << 20)                          /**< Current Digital to Analog Converter 0 access control bit */
#define _SMU_PPUPATD0_IDAC0_SHIFT          20                                     /**< Shift value for SMU_IDAC0 */
#define _SMU_PPUPATD0_IDAC0_MASK           0x100000UL                             /**< Bit mask for SMU_IDAC0 */
#define _SMU_PPUPATD0_IDAC0_DEFAULT        0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_IDAC0_DEFAULT         (_SMU_PPUPATD0_IDAC0_DEFAULT << 20)    /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_MSC                   (0x1UL << 21)                          /**< Memory System Controller access control bit */
#define _SMU_PPUPATD0_MSC_SHIFT            21                                     /**< Shift value for SMU_MSC */
#define _SMU_PPUPATD0_MSC_MASK             0x200000UL                             /**< Bit mask for SMU_MSC */
#define _SMU_PPUPATD0_MSC_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_MSC_DEFAULT           (_SMU_PPUPATD0_MSC_DEFAULT << 21)      /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_LDMA                  (0x1UL << 22)                          /**< Linked Direct Memory Access Controller access control bit */
#define _SMU_PPUPATD0_LDMA_SHIFT           22                                     /**< Shift value for SMU_LDMA */
#define _SMU_PPUPATD0_LDMA_MASK            0x400000UL                             /**< Bit mask for SMU_LDMA */
#define _SMU_PPUPATD0_LDMA_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_LDMA_DEFAULT          (_SMU_PPUPATD0_LDMA_DEFAULT << 22)     /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_LESENSE               (0x1UL << 23)                          /**< Low Energy Sensor Interface access control bit */
#define _SMU_PPUPATD0_LESENSE_SHIFT        23                                     /**< Shift value for SMU_LESENSE */
#define _SMU_PPUPATD0_LESENSE_MASK         0x800000UL                             /**< Bit mask for SMU_LESENSE */
#define _SMU_PPUPATD0_LESENSE_DEFAULT      0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_LESENSE_DEFAULT       (_SMU_PPUPATD0_LESENSE_DEFAULT << 23)  /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_LETIMER0              (0x1UL << 24)                          /**< Low Energy Timer 0 access control bit */
#define _SMU_PPUPATD0_LETIMER0_SHIFT       24                                     /**< Shift value for SMU_LETIMER0 */
#define _SMU_PPUPATD0_LETIMER0_MASK        0x1000000UL                            /**< Bit mask for SMU_LETIMER0 */
#define _SMU_PPUPATD0_LETIMER0_DEFAULT     0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_LETIMER0_DEFAULT      (_SMU_PPUPATD0_LETIMER0_DEFAULT << 24) /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_LEUART0               (0x1UL << 25)                          /**< Low Energy UART 0 access control bit */
#define _SMU_PPUPATD0_LEUART0_SHIFT        25                                     /**< Shift value for SMU_LEUART0 */
#define _SMU_PPUPATD0_LEUART0_MASK         0x2000000UL                            /**< Bit mask for SMU_LEUART0 */
#define _SMU_PPUPATD0_LEUART0_DEFAULT      0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_LEUART0_DEFAULT       (_SMU_PPUPATD0_LEUART0_DEFAULT << 25)  /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_PCNT0                 (0x1UL << 27)                          /**< Pulse Counter 0 access control bit */
#define _SMU_PPUPATD0_PCNT0_SHIFT          27                                     /**< Shift value for SMU_PCNT0 */
#define _SMU_PPUPATD0_PCNT0_MASK           0x8000000UL                            /**< Bit mask for SMU_PCNT0 */
#define _SMU_PPUPATD0_PCNT0_DEFAULT        0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_PCNT0_DEFAULT         (_SMU_PPUPATD0_PCNT0_DEFAULT << 27)    /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_PCNT1                 (0x1UL << 28)                          /**< Pulse Counter 1 access control bit */
#define _SMU_PPUPATD0_PCNT1_SHIFT          28                                     /**< Shift value for SMU_PCNT1 */
#define _SMU_PPUPATD0_PCNT1_MASK           0x10000000UL                           /**< Bit mask for SMU_PCNT1 */
#define _SMU_PPUPATD0_PCNT1_DEFAULT        0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_PCNT1_DEFAULT         (_SMU_PPUPATD0_PCNT1_DEFAULT << 28)    /**< Shifted mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_PCNT2                 (0x1UL << 29)                          /**< Pulse Counter 2 access control bit */
#define _SMU_PPUPATD0_PCNT2_SHIFT          29                                     /**< Shift value for SMU_PCNT2 */
#define _SMU_PPUPATD0_PCNT2_MASK           0x20000000UL                           /**< Bit mask for SMU_PCNT2 */
#define _SMU_PPUPATD0_PCNT2_DEFAULT        0x00000000UL                           /**< Mode DEFAULT for SMU_PPUPATD0 */
#define SMU_PPUPATD0_PCNT2_DEFAULT         (_SMU_PPUPATD0_PCNT2_DEFAULT << 29)    /**< Shifted mode DEFAULT for SMU_PPUPATD0 */

/* Bit fields for SMU PPUPATD1 */
#define _SMU_PPUPATD1_RESETVALUE           0x00000000UL                          /**< Default value for SMU_PPUPATD1 */
#define _SMU_PPUPATD1_MASK                 0x0000FFEEUL                          /**< Mask for SMU_PPUPATD1 */
#define SMU_PPUPATD1_RMU                   (0x1UL << 1)                          /**< Reset Management Unit access control bit */
#define _SMU_PPUPATD1_RMU_SHIFT            1                                     /**< Shift value for SMU_RMU */
#define _SMU_PPUPATD1_RMU_MASK             0x2UL                                 /**< Bit mask for SMU_RMU */
#define _SMU_PPUPATD1_RMU_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_RMU_DEFAULT           (_SMU_PPUPATD1_RMU_DEFAULT << 1)      /**< Shifted mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_RTCC                  (0x1UL << 2)                          /**< Real-Time Counter and Calendar access control bit */
#define _SMU_PPUPATD1_RTCC_SHIFT           2                                     /**< Shift value for SMU_RTCC */
#define _SMU_PPUPATD1_RTCC_MASK            0x4UL                                 /**< Bit mask for SMU_RTCC */
#define _SMU_PPUPATD1_RTCC_DEFAULT         0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_RTCC_DEFAULT          (_SMU_PPUPATD1_RTCC_DEFAULT << 2)     /**< Shifted mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_SMU                   (0x1UL << 3)                          /**< Security Management Unit access control bit */
#define _SMU_PPUPATD1_SMU_SHIFT            3                                     /**< Shift value for SMU_SMU */
#define _SMU_PPUPATD1_SMU_MASK             0x8UL                                 /**< Bit mask for SMU_SMU */
#define _SMU_PPUPATD1_SMU_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_SMU_DEFAULT           (_SMU_PPUPATD1_SMU_DEFAULT << 3)      /**< Shifted mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_TIMER0                (0x1UL << 5)                          /**< Timer 0 access control bit */
#define _SMU_PPUPATD1_TIMER0_SHIFT         5                                     /**< Shift value for SMU_TIMER0 */
#define _SMU_PPUPATD1_TIMER0_MASK          0x20UL                                /**< Bit mask for SMU_TIMER0 */
#define _SMU_PPUPATD1_TIMER0_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_TIMER0_DEFAULT        (_SMU_PPUPATD1_TIMER0_DEFAULT << 5)   /**< Shifted mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_TIMER1                (0x1UL << 6)                          /**< Timer 1 access control bit */
#define _SMU_PPUPATD1_TIMER1_SHIFT         6                                     /**< Shift value for SMU_TIMER1 */
#define _SMU_PPUPATD1_TIMER1_MASK          0x40UL                                /**< Bit mask for SMU_TIMER1 */
#define _SMU_PPUPATD1_TIMER1_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_TIMER1_DEFAULT        (_SMU_PPUPATD1_TIMER1_DEFAULT << 6)   /**< Shifted mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_TRNG0                 (0x1UL << 7)                          /**< True Random Number Generator 0 access control bit */
#define _SMU_PPUPATD1_TRNG0_SHIFT          7                                     /**< Shift value for SMU_TRNG0 */
#define _SMU_PPUPATD1_TRNG0_MASK           0x80UL                                /**< Bit mask for SMU_TRNG0 */
#define _SMU_PPUPATD1_TRNG0_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_TRNG0_DEFAULT         (_SMU_PPUPATD1_TRNG0_DEFAULT << 7)    /**< Shifted mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_USART0                (0x1UL << 8)                          /**< Universal Synchronous/Asynchronous Receiver/Transmitter 0 access control bit */
#define _SMU_PPUPATD1_USART0_SHIFT         8                                     /**< Shift value for SMU_USART0 */
#define _SMU_PPUPATD1_USART0_MASK          0x100UL                               /**< Bit mask for SMU_USART0 */
#define _SMU_PPUPATD1_USART0_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_USART0_DEFAULT        (_SMU_PPUPATD1_USART0_DEFAULT << 8)   /**< Shifted mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_USART1                (0x1UL << 9)                          /**< Universal Synchronous/Asynchronous Receiver/Transmitter 1 access control bit */
#define _SMU_PPUPATD1_USART1_SHIFT         9                                     /**< Shift value for SMU_USART1 */
#define _SMU_PPUPATD1_USART1_MASK          0x200UL                               /**< Bit mask for SMU_USART1 */
#define _SMU_PPUPATD1_USART1_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_USART1_DEFAULT        (_SMU_PPUPATD1_USART1_DEFAULT << 9)   /**< Shifted mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_USART2                (0x1UL << 10)                         /**< Universal Synchronous/Asynchronous Receiver/Transmitter 2 access control bit */
#define _SMU_PPUPATD1_USART2_SHIFT         10                                    /**< Shift value for SMU_USART2 */
#define _SMU_PPUPATD1_USART2_MASK          0x400UL                               /**< Bit mask for SMU_USART2 */
#define _SMU_PPUPATD1_USART2_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_USART2_DEFAULT        (_SMU_PPUPATD1_USART2_DEFAULT << 10)  /**< Shifted mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_USART3                (0x1UL << 11)                         /**< Universal Synchronous/Asynchronous Receiver/Transmitter 3 access control bit */
#define _SMU_PPUPATD1_USART3_SHIFT         11                                    /**< Shift value for SMU_USART3 */
#define _SMU_PPUPATD1_USART3_MASK          0x800UL                               /**< Bit mask for SMU_USART3 */
#define _SMU_PPUPATD1_USART3_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_USART3_DEFAULT        (_SMU_PPUPATD1_USART3_DEFAULT << 11)  /**< Shifted mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_WDOG0                 (0x1UL << 12)                         /**< Watchdog 0 access control bit */
#define _SMU_PPUPATD1_WDOG0_SHIFT          12                                    /**< Shift value for SMU_WDOG0 */
#define _SMU_PPUPATD1_WDOG0_MASK           0x1000UL                              /**< Bit mask for SMU_WDOG0 */
#define _SMU_PPUPATD1_WDOG0_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_WDOG0_DEFAULT         (_SMU_PPUPATD1_WDOG0_DEFAULT << 12)   /**< Shifted mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_WDOG1                 (0x1UL << 13)                         /**< Watchdog 1 access control bit */
#define _SMU_PPUPATD1_WDOG1_SHIFT          13                                    /**< Shift value for SMU_WDOG1 */
#define _SMU_PPUPATD1_WDOG1_MASK           0x2000UL                              /**< Bit mask for SMU_WDOG1 */
#define _SMU_PPUPATD1_WDOG1_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_WDOG1_DEFAULT         (_SMU_PPUPATD1_WDOG1_DEFAULT << 13)   /**< Shifted mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_WTIMER0               (0x1UL << 14)                         /**< Wide Timer 0 access control bit */
#define _SMU_PPUPATD1_WTIMER0_SHIFT        14                                    /**< Shift value for SMU_WTIMER0 */
#define _SMU_PPUPATD1_WTIMER0_MASK         0x4000UL                              /**< Bit mask for SMU_WTIMER0 */
#define _SMU_PPUPATD1_WTIMER0_DEFAULT      0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_WTIMER0_DEFAULT       (_SMU_PPUPATD1_WTIMER0_DEFAULT << 14) /**< Shifted mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_WTIMER1               (0x1UL << 15)                         /**< Wide Timer 1 access control bit */
#define _SMU_PPUPATD1_WTIMER1_SHIFT        15                                    /**< Shift value for SMU_WTIMER1 */
#define _SMU_PPUPATD1_WTIMER1_MASK         0x8000UL                              /**< Bit mask for SMU_WTIMER1 */
#define _SMU_PPUPATD1_WTIMER1_DEFAULT      0x00000000UL                          /**< Mode DEFAULT for SMU_PPUPATD1 */
#define SMU_PPUPATD1_WTIMER1_DEFAULT       (_SMU_PPUPATD1_WTIMER1_DEFAULT << 15) /**< Shifted mode DEFAULT for SMU_PPUPATD1 */

/* Bit fields for SMU PPUFS */
#define _SMU_PPUFS_RESETVALUE              0x00000000UL                         /**< Default value for SMU_PPUFS */
#define _SMU_PPUFS_MASK                    0x0000007FUL                         /**< Mask for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_SHIFT          0                                    /**< Shift value for SMU_PERIPHID */
#define _SMU_PPUFS_PERIPHID_MASK           0x7FUL                               /**< Bit mask for SMU_PERIPHID */
#define _SMU_PPUFS_PERIPHID_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_ACMP0          0x00000000UL                         /**< Mode ACMP0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_ACMP1          0x00000001UL                         /**< Mode ACMP1 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_ADC0           0x00000002UL                         /**< Mode ADC0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_CMU            0x00000005UL                         /**< Mode CMU for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_CRYOTIMER      0x00000007UL                         /**< Mode CRYOTIMER for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_CRYPTO0        0x00000008UL                         /**< Mode CRYPTO0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_CRYPTO1        0x00000009UL                         /**< Mode CRYPTO1 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_CSEN           0x0000000AUL                         /**< Mode CSEN for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_VDAC0          0x0000000BUL                         /**< Mode VDAC0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_PRS            0x0000000CUL                         /**< Mode PRS for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_EMU            0x0000000DUL                         /**< Mode EMU for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_FPUEH          0x0000000EUL                         /**< Mode FPUEH for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_GPCRC          0x00000010UL                         /**< Mode GPCRC for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_GPIO           0x00000011UL                         /**< Mode GPIO for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_I2C0           0x00000012UL                         /**< Mode I2C0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_I2C1           0x00000013UL                         /**< Mode I2C1 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_IDAC0          0x00000014UL                         /**< Mode IDAC0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_MSC            0x00000015UL                         /**< Mode MSC for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_LDMA           0x00000016UL                         /**< Mode LDMA for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_LESENSE        0x00000017UL                         /**< Mode LESENSE for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_LETIMER0       0x00000018UL                         /**< Mode LETIMER0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_LEUART0        0x00000019UL                         /**< Mode LEUART0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_PCNT0          0x0000001BUL                         /**< Mode PCNT0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_PCNT1          0x0000001CUL                         /**< Mode PCNT1 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_PCNT2          0x0000001DUL                         /**< Mode PCNT2 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_RMU            0x00000021UL                         /**< Mode RMU for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_RTCC           0x00000022UL                         /**< Mode RTCC for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_SMU            0x00000023UL                         /**< Mode SMU for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_TIMER0         0x00000025UL                         /**< Mode TIMER0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_TIMER1         0x00000026UL                         /**< Mode TIMER1 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_TRNG0          0x00000027UL                         /**< Mode TRNG0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_USART0         0x00000028UL                         /**< Mode USART0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_USART1         0x00000029UL                         /**< Mode USART1 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_USART2         0x0000002AUL                         /**< Mode USART2 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_USART3         0x0000002BUL                         /**< Mode USART3 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_WDOG0          0x0000002CUL                         /**< Mode WDOG0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_WDOG1          0x0000002DUL                         /**< Mode WDOG1 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_WTIMER0        0x0000002EUL                         /**< Mode WTIMER0 for SMU_PPUFS */
#define _SMU_PPUFS_PERIPHID_WTIMER1        0x0000002FUL                         /**< Mode WTIMER1 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_DEFAULT         (_SMU_PPUFS_PERIPHID_DEFAULT << 0)   /**< Shifted mode DEFAULT for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_ACMP0           (_SMU_PPUFS_PERIPHID_ACMP0 << 0)     /**< Shifted mode ACMP0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_ACMP1           (_SMU_PPUFS_PERIPHID_ACMP1 << 0)     /**< Shifted mode ACMP1 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_ADC0            (_SMU_PPUFS_PERIPHID_ADC0 << 0)      /**< Shifted mode ADC0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_CMU             (_SMU_PPUFS_PERIPHID_CMU << 0)       /**< Shifted mode CMU for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_CRYOTIMER       (_SMU_PPUFS_PERIPHID_CRYOTIMER << 0) /**< Shifted mode CRYOTIMER for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_CRYPTO0         (_SMU_PPUFS_PERIPHID_CRYPTO0 << 0)   /**< Shifted mode CRYPTO0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_CRYPTO1         (_SMU_PPUFS_PERIPHID_CRYPTO1 << 0)   /**< Shifted mode CRYPTO1 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_CSEN            (_SMU_PPUFS_PERIPHID_CSEN << 0)      /**< Shifted mode CSEN for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_VDAC0           (_SMU_PPUFS_PERIPHID_VDAC0 << 0)     /**< Shifted mode VDAC0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_PRS             (_SMU_PPUFS_PERIPHID_PRS << 0)       /**< Shifted mode PRS for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_EMU             (_SMU_PPUFS_PERIPHID_EMU << 0)       /**< Shifted mode EMU for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_FPUEH           (_SMU_PPUFS_PERIPHID_FPUEH << 0)     /**< Shifted mode FPUEH for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_GPCRC           (_SMU_PPUFS_PERIPHID_GPCRC << 0)     /**< Shifted mode GPCRC for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_GPIO            (_SMU_PPUFS_PERIPHID_GPIO << 0)      /**< Shifted mode GPIO for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_I2C0            (_SMU_PPUFS_PERIPHID_I2C0 << 0)      /**< Shifted mode I2C0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_I2C1            (_SMU_PPUFS_PERIPHID_I2C1 << 0)      /**< Shifted mode I2C1 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_IDAC0           (_SMU_PPUFS_PERIPHID_IDAC0 << 0)     /**< Shifted mode IDAC0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_MSC             (_SMU_PPUFS_PERIPHID_MSC << 0)       /**< Shifted mode MSC for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_LDMA            (_SMU_PPUFS_PERIPHID_LDMA << 0)      /**< Shifted mode LDMA for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_LESENSE         (_SMU_PPUFS_PERIPHID_LESENSE << 0)   /**< Shifted mode LESENSE for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_LETIMER0        (_SMU_PPUFS_PERIPHID_LETIMER0 << 0)  /**< Shifted mode LETIMER0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_LEUART0         (_SMU_PPUFS_PERIPHID_LEUART0 << 0)   /**< Shifted mode LEUART0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_PCNT0           (_SMU_PPUFS_PERIPHID_PCNT0 << 0)     /**< Shifted mode PCNT0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_PCNT1           (_SMU_PPUFS_PERIPHID_PCNT1 << 0)     /**< Shifted mode PCNT1 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_PCNT2           (_SMU_PPUFS_PERIPHID_PCNT2 << 0)     /**< Shifted mode PCNT2 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_RMU             (_SMU_PPUFS_PERIPHID_RMU << 0)       /**< Shifted mode RMU for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_RTCC            (_SMU_PPUFS_PERIPHID_RTCC << 0)      /**< Shifted mode RTCC for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_SMU             (_SMU_PPUFS_PERIPHID_SMU << 0)       /**< Shifted mode SMU for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_TIMER0          (_SMU_PPUFS_PERIPHID_TIMER0 << 0)    /**< Shifted mode TIMER0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_TIMER1          (_SMU_PPUFS_PERIPHID_TIMER1 << 0)    /**< Shifted mode TIMER1 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_TRNG0           (_SMU_PPUFS_PERIPHID_TRNG0 << 0)     /**< Shifted mode TRNG0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_USART0          (_SMU_PPUFS_PERIPHID_USART0 << 0)    /**< Shifted mode USART0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_USART1          (_SMU_PPUFS_PERIPHID_USART1 << 0)    /**< Shifted mode USART1 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_USART2          (_SMU_PPUFS_PERIPHID_USART2 << 0)    /**< Shifted mode USART2 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_USART3          (_SMU_PPUFS_PERIPHID_USART3 << 0)    /**< Shifted mode USART3 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_WDOG0           (_SMU_PPUFS_PERIPHID_WDOG0 << 0)     /**< Shifted mode WDOG0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_WDOG1           (_SMU_PPUFS_PERIPHID_WDOG1 << 0)     /**< Shifted mode WDOG1 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_WTIMER0         (_SMU_PPUFS_PERIPHID_WTIMER0 << 0)   /**< Shifted mode WTIMER0 for SMU_PPUFS */
#define SMU_PPUFS_PERIPHID_WTIMER1         (_SMU_PPUFS_PERIPHID_WTIMER1 << 0)   /**< Shifted mode WTIMER1 for SMU_PPUFS */

/** @} */
/** @} End of group EFR32MG12P_SMU */
/** @} End of group Parts */
