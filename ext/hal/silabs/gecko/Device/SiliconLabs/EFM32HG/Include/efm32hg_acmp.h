/**************************************************************************//**
 * @file efm32hg_acmp.h
 * @brief EFM32HG_ACMP register and bit field definitions
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
/**************************************************************************//**
* @addtogroup Parts
* @{
******************************************************************************/
/**************************************************************************//**
 * @defgroup EFM32HG_ACMP
 * @{
 * @brief EFM32HG_ACMP Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;     /**< Control Register  */
  __IOM uint32_t INPUTSEL; /**< Input Selection Register  */
  __IM uint32_t  STATUS;   /**< Status Register  */
  __IOM uint32_t IEN;      /**< Interrupt Enable Register  */
  __IM uint32_t  IF;       /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;      /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;      /**< Interrupt Flag Clear Register  */
  __IOM uint32_t ROUTE;    /**< I/O Routing Register  */
} ACMP_TypeDef;            /** @} */

/**************************************************************************//**
 * @defgroup EFM32HG_ACMP_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for ACMP CTRL */
#define _ACMP_CTRL_RESETVALUE              0x47000000UL                         /**< Default value for ACMP_CTRL */
#define _ACMP_CTRL_MASK                    0xCF03077FUL                         /**< Mask for ACMP_CTRL */
#define ACMP_CTRL_EN                       (0x1UL << 0)                         /**< Analog Comparator Enable */
#define _ACMP_CTRL_EN_SHIFT                0                                    /**< Shift value for ACMP_EN */
#define _ACMP_CTRL_EN_MASK                 0x1UL                                /**< Bit mask for ACMP_EN */
#define _ACMP_CTRL_EN_DEFAULT              0x00000000UL                         /**< Mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_EN_DEFAULT               (_ACMP_CTRL_EN_DEFAULT << 0)         /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_MUXEN                    (0x1UL << 1)                         /**< Input Mux Enable */
#define _ACMP_CTRL_MUXEN_SHIFT             1                                    /**< Shift value for ACMP_MUXEN */
#define _ACMP_CTRL_MUXEN_MASK              0x2UL                                /**< Bit mask for ACMP_MUXEN */
#define _ACMP_CTRL_MUXEN_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_MUXEN_DEFAULT            (_ACMP_CTRL_MUXEN_DEFAULT << 1)      /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_INACTVAL                 (0x1UL << 2)                         /**< Inactive Value */
#define _ACMP_CTRL_INACTVAL_SHIFT          2                                    /**< Shift value for ACMP_INACTVAL */
#define _ACMP_CTRL_INACTVAL_MASK           0x4UL                                /**< Bit mask for ACMP_INACTVAL */
#define _ACMP_CTRL_INACTVAL_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_INACTVAL_LOW            0x00000000UL                         /**< Mode LOW for ACMP_CTRL */
#define _ACMP_CTRL_INACTVAL_HIGH           0x00000001UL                         /**< Mode HIGH for ACMP_CTRL */
#define ACMP_CTRL_INACTVAL_DEFAULT         (_ACMP_CTRL_INACTVAL_DEFAULT << 2)   /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_INACTVAL_LOW             (_ACMP_CTRL_INACTVAL_LOW << 2)       /**< Shifted mode LOW for ACMP_CTRL */
#define ACMP_CTRL_INACTVAL_HIGH            (_ACMP_CTRL_INACTVAL_HIGH << 2)      /**< Shifted mode HIGH for ACMP_CTRL */
#define ACMP_CTRL_GPIOINV                  (0x1UL << 3)                         /**< Comparator GPIO Output Invert */
#define _ACMP_CTRL_GPIOINV_SHIFT           3                                    /**< Shift value for ACMP_GPIOINV */
#define _ACMP_CTRL_GPIOINV_MASK            0x8UL                                /**< Bit mask for ACMP_GPIOINV */
#define _ACMP_CTRL_GPIOINV_DEFAULT         0x00000000UL                         /**< Mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_GPIOINV_NOTINV          0x00000000UL                         /**< Mode NOTINV for ACMP_CTRL */
#define _ACMP_CTRL_GPIOINV_INV             0x00000001UL                         /**< Mode INV for ACMP_CTRL */
#define ACMP_CTRL_GPIOINV_DEFAULT          (_ACMP_CTRL_GPIOINV_DEFAULT << 3)    /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_GPIOINV_NOTINV           (_ACMP_CTRL_GPIOINV_NOTINV << 3)     /**< Shifted mode NOTINV for ACMP_CTRL */
#define ACMP_CTRL_GPIOINV_INV              (_ACMP_CTRL_GPIOINV_INV << 3)        /**< Shifted mode INV for ACMP_CTRL */
#define _ACMP_CTRL_HYSTSEL_SHIFT           4                                    /**< Shift value for ACMP_HYSTSEL */
#define _ACMP_CTRL_HYSTSEL_MASK            0x70UL                               /**< Bit mask for ACMP_HYSTSEL */
#define _ACMP_CTRL_HYSTSEL_DEFAULT         0x00000000UL                         /**< Mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_HYSTSEL_HYST0           0x00000000UL                         /**< Mode HYST0 for ACMP_CTRL */
#define _ACMP_CTRL_HYSTSEL_HYST1           0x00000001UL                         /**< Mode HYST1 for ACMP_CTRL */
#define _ACMP_CTRL_HYSTSEL_HYST2           0x00000002UL                         /**< Mode HYST2 for ACMP_CTRL */
#define _ACMP_CTRL_HYSTSEL_HYST3           0x00000003UL                         /**< Mode HYST3 for ACMP_CTRL */
#define _ACMP_CTRL_HYSTSEL_HYST4           0x00000004UL                         /**< Mode HYST4 for ACMP_CTRL */
#define _ACMP_CTRL_HYSTSEL_HYST5           0x00000005UL                         /**< Mode HYST5 for ACMP_CTRL */
#define _ACMP_CTRL_HYSTSEL_HYST6           0x00000006UL                         /**< Mode HYST6 for ACMP_CTRL */
#define _ACMP_CTRL_HYSTSEL_HYST7           0x00000007UL                         /**< Mode HYST7 for ACMP_CTRL */
#define ACMP_CTRL_HYSTSEL_DEFAULT          (_ACMP_CTRL_HYSTSEL_DEFAULT << 4)    /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_HYSTSEL_HYST0            (_ACMP_CTRL_HYSTSEL_HYST0 << 4)      /**< Shifted mode HYST0 for ACMP_CTRL */
#define ACMP_CTRL_HYSTSEL_HYST1            (_ACMP_CTRL_HYSTSEL_HYST1 << 4)      /**< Shifted mode HYST1 for ACMP_CTRL */
#define ACMP_CTRL_HYSTSEL_HYST2            (_ACMP_CTRL_HYSTSEL_HYST2 << 4)      /**< Shifted mode HYST2 for ACMP_CTRL */
#define ACMP_CTRL_HYSTSEL_HYST3            (_ACMP_CTRL_HYSTSEL_HYST3 << 4)      /**< Shifted mode HYST3 for ACMP_CTRL */
#define ACMP_CTRL_HYSTSEL_HYST4            (_ACMP_CTRL_HYSTSEL_HYST4 << 4)      /**< Shifted mode HYST4 for ACMP_CTRL */
#define ACMP_CTRL_HYSTSEL_HYST5            (_ACMP_CTRL_HYSTSEL_HYST5 << 4)      /**< Shifted mode HYST5 for ACMP_CTRL */
#define ACMP_CTRL_HYSTSEL_HYST6            (_ACMP_CTRL_HYSTSEL_HYST6 << 4)      /**< Shifted mode HYST6 for ACMP_CTRL */
#define ACMP_CTRL_HYSTSEL_HYST7            (_ACMP_CTRL_HYSTSEL_HYST7 << 4)      /**< Shifted mode HYST7 for ACMP_CTRL */
#define _ACMP_CTRL_WARMTIME_SHIFT          8                                    /**< Shift value for ACMP_WARMTIME */
#define _ACMP_CTRL_WARMTIME_MASK           0x700UL                              /**< Bit mask for ACMP_WARMTIME */
#define _ACMP_CTRL_WARMTIME_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_WARMTIME_4CYCLES        0x00000000UL                         /**< Mode 4CYCLES for ACMP_CTRL */
#define _ACMP_CTRL_WARMTIME_8CYCLES        0x00000001UL                         /**< Mode 8CYCLES for ACMP_CTRL */
#define _ACMP_CTRL_WARMTIME_16CYCLES       0x00000002UL                         /**< Mode 16CYCLES for ACMP_CTRL */
#define _ACMP_CTRL_WARMTIME_32CYCLES       0x00000003UL                         /**< Mode 32CYCLES for ACMP_CTRL */
#define _ACMP_CTRL_WARMTIME_64CYCLES       0x00000004UL                         /**< Mode 64CYCLES for ACMP_CTRL */
#define _ACMP_CTRL_WARMTIME_128CYCLES      0x00000005UL                         /**< Mode 128CYCLES for ACMP_CTRL */
#define _ACMP_CTRL_WARMTIME_256CYCLES      0x00000006UL                         /**< Mode 256CYCLES for ACMP_CTRL */
#define _ACMP_CTRL_WARMTIME_512CYCLES      0x00000007UL                         /**< Mode 512CYCLES for ACMP_CTRL */
#define ACMP_CTRL_WARMTIME_DEFAULT         (_ACMP_CTRL_WARMTIME_DEFAULT << 8)   /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_WARMTIME_4CYCLES         (_ACMP_CTRL_WARMTIME_4CYCLES << 8)   /**< Shifted mode 4CYCLES for ACMP_CTRL */
#define ACMP_CTRL_WARMTIME_8CYCLES         (_ACMP_CTRL_WARMTIME_8CYCLES << 8)   /**< Shifted mode 8CYCLES for ACMP_CTRL */
#define ACMP_CTRL_WARMTIME_16CYCLES        (_ACMP_CTRL_WARMTIME_16CYCLES << 8)  /**< Shifted mode 16CYCLES for ACMP_CTRL */
#define ACMP_CTRL_WARMTIME_32CYCLES        (_ACMP_CTRL_WARMTIME_32CYCLES << 8)  /**< Shifted mode 32CYCLES for ACMP_CTRL */
#define ACMP_CTRL_WARMTIME_64CYCLES        (_ACMP_CTRL_WARMTIME_64CYCLES << 8)  /**< Shifted mode 64CYCLES for ACMP_CTRL */
#define ACMP_CTRL_WARMTIME_128CYCLES       (_ACMP_CTRL_WARMTIME_128CYCLES << 8) /**< Shifted mode 128CYCLES for ACMP_CTRL */
#define ACMP_CTRL_WARMTIME_256CYCLES       (_ACMP_CTRL_WARMTIME_256CYCLES << 8) /**< Shifted mode 256CYCLES for ACMP_CTRL */
#define ACMP_CTRL_WARMTIME_512CYCLES       (_ACMP_CTRL_WARMTIME_512CYCLES << 8) /**< Shifted mode 512CYCLES for ACMP_CTRL */
#define ACMP_CTRL_IRISE                    (0x1UL << 16)                        /**< Rising Edge Interrupt Sense */
#define _ACMP_CTRL_IRISE_SHIFT             16                                   /**< Shift value for ACMP_IRISE */
#define _ACMP_CTRL_IRISE_MASK              0x10000UL                            /**< Bit mask for ACMP_IRISE */
#define _ACMP_CTRL_IRISE_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_IRISE_DISABLED          0x00000000UL                         /**< Mode DISABLED for ACMP_CTRL */
#define _ACMP_CTRL_IRISE_ENABLED           0x00000001UL                         /**< Mode ENABLED for ACMP_CTRL */
#define ACMP_CTRL_IRISE_DEFAULT            (_ACMP_CTRL_IRISE_DEFAULT << 16)     /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_IRISE_DISABLED           (_ACMP_CTRL_IRISE_DISABLED << 16)    /**< Shifted mode DISABLED for ACMP_CTRL */
#define ACMP_CTRL_IRISE_ENABLED            (_ACMP_CTRL_IRISE_ENABLED << 16)     /**< Shifted mode ENABLED for ACMP_CTRL */
#define ACMP_CTRL_IFALL                    (0x1UL << 17)                        /**< Falling Edge Interrupt Sense */
#define _ACMP_CTRL_IFALL_SHIFT             17                                   /**< Shift value for ACMP_IFALL */
#define _ACMP_CTRL_IFALL_MASK              0x20000UL                            /**< Bit mask for ACMP_IFALL */
#define _ACMP_CTRL_IFALL_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_IFALL_DISABLED          0x00000000UL                         /**< Mode DISABLED for ACMP_CTRL */
#define _ACMP_CTRL_IFALL_ENABLED           0x00000001UL                         /**< Mode ENABLED for ACMP_CTRL */
#define ACMP_CTRL_IFALL_DEFAULT            (_ACMP_CTRL_IFALL_DEFAULT << 17)     /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_IFALL_DISABLED           (_ACMP_CTRL_IFALL_DISABLED << 17)    /**< Shifted mode DISABLED for ACMP_CTRL */
#define ACMP_CTRL_IFALL_ENABLED            (_ACMP_CTRL_IFALL_ENABLED << 17)     /**< Shifted mode ENABLED for ACMP_CTRL */
#define _ACMP_CTRL_BIASPROG_SHIFT          24                                   /**< Shift value for ACMP_BIASPROG */
#define _ACMP_CTRL_BIASPROG_MASK           0xF000000UL                          /**< Bit mask for ACMP_BIASPROG */
#define _ACMP_CTRL_BIASPROG_DEFAULT        0x00000007UL                         /**< Mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_BIASPROG_DEFAULT         (_ACMP_CTRL_BIASPROG_DEFAULT << 24)  /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_HALFBIAS                 (0x1UL << 30)                        /**< Half Bias Current */
#define _ACMP_CTRL_HALFBIAS_SHIFT          30                                   /**< Shift value for ACMP_HALFBIAS */
#define _ACMP_CTRL_HALFBIAS_MASK           0x40000000UL                         /**< Bit mask for ACMP_HALFBIAS */
#define _ACMP_CTRL_HALFBIAS_DEFAULT        0x00000001UL                         /**< Mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_HALFBIAS_DEFAULT         (_ACMP_CTRL_HALFBIAS_DEFAULT << 30)  /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_FULLBIAS                 (0x1UL << 31)                        /**< Full Bias Current */
#define _ACMP_CTRL_FULLBIAS_SHIFT          31                                   /**< Shift value for ACMP_FULLBIAS */
#define _ACMP_CTRL_FULLBIAS_MASK           0x80000000UL                         /**< Bit mask for ACMP_FULLBIAS */
#define _ACMP_CTRL_FULLBIAS_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_FULLBIAS_DEFAULT         (_ACMP_CTRL_FULLBIAS_DEFAULT << 31)  /**< Shifted mode DEFAULT for ACMP_CTRL */

/* Bit fields for ACMP INPUTSEL */
#define _ACMP_INPUTSEL_RESETVALUE          0x00010080UL                            /**< Default value for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_MASK                0x31013FF7UL                            /**< Mask for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_SHIFT        0                                       /**< Shift value for ACMP_POSSEL */
#define _ACMP_INPUTSEL_POSSEL_MASK         0x7UL                                   /**< Bit mask for ACMP_POSSEL */
#define _ACMP_INPUTSEL_POSSEL_DEFAULT      0x00000000UL                            /**< Mode DEFAULT for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_CH0          0x00000000UL                            /**< Mode CH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_CH1          0x00000001UL                            /**< Mode CH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_CH2          0x00000002UL                            /**< Mode CH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_CH3          0x00000003UL                            /**< Mode CH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_CH4          0x00000004UL                            /**< Mode CH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_CH5          0x00000005UL                            /**< Mode CH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_CH6          0x00000006UL                            /**< Mode CH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_CH7          0x00000007UL                            /**< Mode CH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_DEFAULT       (_ACMP_INPUTSEL_POSSEL_DEFAULT << 0)    /**< Shifted mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_CH0           (_ACMP_INPUTSEL_POSSEL_CH0 << 0)        /**< Shifted mode CH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_CH1           (_ACMP_INPUTSEL_POSSEL_CH1 << 0)        /**< Shifted mode CH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_CH2           (_ACMP_INPUTSEL_POSSEL_CH2 << 0)        /**< Shifted mode CH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_CH3           (_ACMP_INPUTSEL_POSSEL_CH3 << 0)        /**< Shifted mode CH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_CH4           (_ACMP_INPUTSEL_POSSEL_CH4 << 0)        /**< Shifted mode CH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_CH5           (_ACMP_INPUTSEL_POSSEL_CH5 << 0)        /**< Shifted mode CH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_CH6           (_ACMP_INPUTSEL_POSSEL_CH6 << 0)        /**< Shifted mode CH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_CH7           (_ACMP_INPUTSEL_POSSEL_CH7 << 0)        /**< Shifted mode CH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_SHIFT        4                                       /**< Shift value for ACMP_NEGSEL */
#define _ACMP_INPUTSEL_NEGSEL_MASK         0xF0UL                                  /**< Bit mask for ACMP_NEGSEL */
#define _ACMP_INPUTSEL_NEGSEL_CH0          0x00000000UL                            /**< Mode CH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_CH1          0x00000001UL                            /**< Mode CH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_CH2          0x00000002UL                            /**< Mode CH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_CH3          0x00000003UL                            /**< Mode CH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_CH4          0x00000004UL                            /**< Mode CH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_CH5          0x00000005UL                            /**< Mode CH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_CH6          0x00000006UL                            /**< Mode CH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_CH7          0x00000007UL                            /**< Mode CH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_DEFAULT      0x00000008UL                            /**< Mode DEFAULT for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_1V25         0x00000008UL                            /**< Mode 1V25 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_2V5          0x00000009UL                            /**< Mode 2V5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_VDD          0x0000000AUL                            /**< Mode VDD for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_CAPSENSE     0x0000000BUL                            /**< Mode CAPSENSE for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_CH0           (_ACMP_INPUTSEL_NEGSEL_CH0 << 4)        /**< Shifted mode CH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_CH1           (_ACMP_INPUTSEL_NEGSEL_CH1 << 4)        /**< Shifted mode CH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_CH2           (_ACMP_INPUTSEL_NEGSEL_CH2 << 4)        /**< Shifted mode CH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_CH3           (_ACMP_INPUTSEL_NEGSEL_CH3 << 4)        /**< Shifted mode CH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_CH4           (_ACMP_INPUTSEL_NEGSEL_CH4 << 4)        /**< Shifted mode CH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_CH5           (_ACMP_INPUTSEL_NEGSEL_CH5 << 4)        /**< Shifted mode CH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_CH6           (_ACMP_INPUTSEL_NEGSEL_CH6 << 4)        /**< Shifted mode CH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_CH7           (_ACMP_INPUTSEL_NEGSEL_CH7 << 4)        /**< Shifted mode CH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_DEFAULT       (_ACMP_INPUTSEL_NEGSEL_DEFAULT << 4)    /**< Shifted mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_1V25          (_ACMP_INPUTSEL_NEGSEL_1V25 << 4)       /**< Shifted mode 1V25 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_2V5           (_ACMP_INPUTSEL_NEGSEL_2V5 << 4)        /**< Shifted mode 2V5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_VDD           (_ACMP_INPUTSEL_NEGSEL_VDD << 4)        /**< Shifted mode VDD for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_CAPSENSE      (_ACMP_INPUTSEL_NEGSEL_CAPSENSE << 4)   /**< Shifted mode CAPSENSE for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VDDLEVEL_SHIFT      8                                       /**< Shift value for ACMP_VDDLEVEL */
#define _ACMP_INPUTSEL_VDDLEVEL_MASK       0x3F00UL                                /**< Bit mask for ACMP_VDDLEVEL */
#define _ACMP_INPUTSEL_VDDLEVEL_DEFAULT    0x00000000UL                            /**< Mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VDDLEVEL_DEFAULT     (_ACMP_INPUTSEL_VDDLEVEL_DEFAULT << 8)  /**< Shifted mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_LPREF                (0x1UL << 16)                           /**< Low Power Reference Mode */
#define _ACMP_INPUTSEL_LPREF_SHIFT         16                                      /**< Shift value for ACMP_LPREF */
#define _ACMP_INPUTSEL_LPREF_MASK          0x10000UL                               /**< Bit mask for ACMP_LPREF */
#define _ACMP_INPUTSEL_LPREF_DEFAULT       0x00000001UL                            /**< Mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_LPREF_DEFAULT        (_ACMP_INPUTSEL_LPREF_DEFAULT << 16)    /**< Shifted mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESEN              (0x1UL << 24)                           /**< Capacitive Sense Mode Internal Resistor Enable */
#define _ACMP_INPUTSEL_CSRESEN_SHIFT       24                                      /**< Shift value for ACMP_CSRESEN */
#define _ACMP_INPUTSEL_CSRESEN_MASK        0x1000000UL                             /**< Bit mask for ACMP_CSRESEN */
#define _ACMP_INPUTSEL_CSRESEN_DEFAULT     0x00000000UL                            /**< Mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESEN_DEFAULT      (_ACMP_INPUTSEL_CSRESEN_DEFAULT << 24)  /**< Shifted mode DEFAULT for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_SHIFT      28                                      /**< Shift value for ACMP_CSRESSEL */
#define _ACMP_INPUTSEL_CSRESSEL_MASK       0x30000000UL                            /**< Bit mask for ACMP_CSRESSEL */
#define _ACMP_INPUTSEL_CSRESSEL_DEFAULT    0x00000000UL                            /**< Mode DEFAULT for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_RES0       0x00000000UL                            /**< Mode RES0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_RES1       0x00000001UL                            /**< Mode RES1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_RES2       0x00000002UL                            /**< Mode RES2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_RES3       0x00000003UL                            /**< Mode RES3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_DEFAULT     (_ACMP_INPUTSEL_CSRESSEL_DEFAULT << 28) /**< Shifted mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_RES0        (_ACMP_INPUTSEL_CSRESSEL_RES0 << 28)    /**< Shifted mode RES0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_RES1        (_ACMP_INPUTSEL_CSRESSEL_RES1 << 28)    /**< Shifted mode RES1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_RES2        (_ACMP_INPUTSEL_CSRESSEL_RES2 << 28)    /**< Shifted mode RES2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_RES3        (_ACMP_INPUTSEL_CSRESSEL_RES3 << 28)    /**< Shifted mode RES3 for ACMP_INPUTSEL */

/* Bit fields for ACMP STATUS */
#define _ACMP_STATUS_RESETVALUE            0x00000000UL                        /**< Default value for ACMP_STATUS */
#define _ACMP_STATUS_MASK                  0x00000003UL                        /**< Mask for ACMP_STATUS */
#define ACMP_STATUS_ACMPACT                (0x1UL << 0)                        /**< Analog Comparator Active */
#define _ACMP_STATUS_ACMPACT_SHIFT         0                                   /**< Shift value for ACMP_ACMPACT */
#define _ACMP_STATUS_ACMPACT_MASK          0x1UL                               /**< Bit mask for ACMP_ACMPACT */
#define _ACMP_STATUS_ACMPACT_DEFAULT       0x00000000UL                        /**< Mode DEFAULT for ACMP_STATUS */
#define ACMP_STATUS_ACMPACT_DEFAULT        (_ACMP_STATUS_ACMPACT_DEFAULT << 0) /**< Shifted mode DEFAULT for ACMP_STATUS */
#define ACMP_STATUS_ACMPOUT                (0x1UL << 1)                        /**< Analog Comparator Output */
#define _ACMP_STATUS_ACMPOUT_SHIFT         1                                   /**< Shift value for ACMP_ACMPOUT */
#define _ACMP_STATUS_ACMPOUT_MASK          0x2UL                               /**< Bit mask for ACMP_ACMPOUT */
#define _ACMP_STATUS_ACMPOUT_DEFAULT       0x00000000UL                        /**< Mode DEFAULT for ACMP_STATUS */
#define ACMP_STATUS_ACMPOUT_DEFAULT        (_ACMP_STATUS_ACMPOUT_DEFAULT << 1) /**< Shifted mode DEFAULT for ACMP_STATUS */

/* Bit fields for ACMP IEN */
#define _ACMP_IEN_RESETVALUE               0x00000000UL                    /**< Default value for ACMP_IEN */
#define _ACMP_IEN_MASK                     0x00000003UL                    /**< Mask for ACMP_IEN */
#define ACMP_IEN_EDGE                      (0x1UL << 0)                    /**< Edge Trigger Interrupt Enable */
#define _ACMP_IEN_EDGE_SHIFT               0                               /**< Shift value for ACMP_EDGE */
#define _ACMP_IEN_EDGE_MASK                0x1UL                           /**< Bit mask for ACMP_EDGE */
#define _ACMP_IEN_EDGE_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for ACMP_IEN */
#define ACMP_IEN_EDGE_DEFAULT              (_ACMP_IEN_EDGE_DEFAULT << 0)   /**< Shifted mode DEFAULT for ACMP_IEN */
#define ACMP_IEN_WARMUP                    (0x1UL << 1)                    /**< Warm-up Interrupt Enable */
#define _ACMP_IEN_WARMUP_SHIFT             1                               /**< Shift value for ACMP_WARMUP */
#define _ACMP_IEN_WARMUP_MASK              0x2UL                           /**< Bit mask for ACMP_WARMUP */
#define _ACMP_IEN_WARMUP_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for ACMP_IEN */
#define ACMP_IEN_WARMUP_DEFAULT            (_ACMP_IEN_WARMUP_DEFAULT << 1) /**< Shifted mode DEFAULT for ACMP_IEN */

/* Bit fields for ACMP IF */
#define _ACMP_IF_RESETVALUE                0x00000000UL                   /**< Default value for ACMP_IF */
#define _ACMP_IF_MASK                      0x00000003UL                   /**< Mask for ACMP_IF */
#define ACMP_IF_EDGE                       (0x1UL << 0)                   /**< Edge Triggered Interrupt Flag */
#define _ACMP_IF_EDGE_SHIFT                0                              /**< Shift value for ACMP_EDGE */
#define _ACMP_IF_EDGE_MASK                 0x1UL                          /**< Bit mask for ACMP_EDGE */
#define _ACMP_IF_EDGE_DEFAULT              0x00000000UL                   /**< Mode DEFAULT for ACMP_IF */
#define ACMP_IF_EDGE_DEFAULT               (_ACMP_IF_EDGE_DEFAULT << 0)   /**< Shifted mode DEFAULT for ACMP_IF */
#define ACMP_IF_WARMUP                     (0x1UL << 1)                   /**< Warm-up Interrupt Flag */
#define _ACMP_IF_WARMUP_SHIFT              1                              /**< Shift value for ACMP_WARMUP */
#define _ACMP_IF_WARMUP_MASK               0x2UL                          /**< Bit mask for ACMP_WARMUP */
#define _ACMP_IF_WARMUP_DEFAULT            0x00000000UL                   /**< Mode DEFAULT for ACMP_IF */
#define ACMP_IF_WARMUP_DEFAULT             (_ACMP_IF_WARMUP_DEFAULT << 1) /**< Shifted mode DEFAULT for ACMP_IF */

/* Bit fields for ACMP IFS */
#define _ACMP_IFS_RESETVALUE               0x00000000UL                    /**< Default value for ACMP_IFS */
#define _ACMP_IFS_MASK                     0x00000003UL                    /**< Mask for ACMP_IFS */
#define ACMP_IFS_EDGE                      (0x1UL << 0)                    /**< Edge Triggered Interrupt Flag Set */
#define _ACMP_IFS_EDGE_SHIFT               0                               /**< Shift value for ACMP_EDGE */
#define _ACMP_IFS_EDGE_MASK                0x1UL                           /**< Bit mask for ACMP_EDGE */
#define _ACMP_IFS_EDGE_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for ACMP_IFS */
#define ACMP_IFS_EDGE_DEFAULT              (_ACMP_IFS_EDGE_DEFAULT << 0)   /**< Shifted mode DEFAULT for ACMP_IFS */
#define ACMP_IFS_WARMUP                    (0x1UL << 1)                    /**< Warm-up Interrupt Flag Set */
#define _ACMP_IFS_WARMUP_SHIFT             1                               /**< Shift value for ACMP_WARMUP */
#define _ACMP_IFS_WARMUP_MASK              0x2UL                           /**< Bit mask for ACMP_WARMUP */
#define _ACMP_IFS_WARMUP_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for ACMP_IFS */
#define ACMP_IFS_WARMUP_DEFAULT            (_ACMP_IFS_WARMUP_DEFAULT << 1) /**< Shifted mode DEFAULT for ACMP_IFS */

/* Bit fields for ACMP IFC */
#define _ACMP_IFC_RESETVALUE               0x00000000UL                    /**< Default value for ACMP_IFC */
#define _ACMP_IFC_MASK                     0x00000003UL                    /**< Mask for ACMP_IFC */
#define ACMP_IFC_EDGE                      (0x1UL << 0)                    /**< Edge Triggered Interrupt Flag Clear */
#define _ACMP_IFC_EDGE_SHIFT               0                               /**< Shift value for ACMP_EDGE */
#define _ACMP_IFC_EDGE_MASK                0x1UL                           /**< Bit mask for ACMP_EDGE */
#define _ACMP_IFC_EDGE_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for ACMP_IFC */
#define ACMP_IFC_EDGE_DEFAULT              (_ACMP_IFC_EDGE_DEFAULT << 0)   /**< Shifted mode DEFAULT for ACMP_IFC */
#define ACMP_IFC_WARMUP                    (0x1UL << 1)                    /**< Warm-up Interrupt Flag Clear */
#define _ACMP_IFC_WARMUP_SHIFT             1                               /**< Shift value for ACMP_WARMUP */
#define _ACMP_IFC_WARMUP_MASK              0x2UL                           /**< Bit mask for ACMP_WARMUP */
#define _ACMP_IFC_WARMUP_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for ACMP_IFC */
#define ACMP_IFC_WARMUP_DEFAULT            (_ACMP_IFC_WARMUP_DEFAULT << 1) /**< Shifted mode DEFAULT for ACMP_IFC */

/* Bit fields for ACMP ROUTE */
#define _ACMP_ROUTE_RESETVALUE             0x00000000UL                        /**< Default value for ACMP_ROUTE */
#define _ACMP_ROUTE_MASK                   0x00000701UL                        /**< Mask for ACMP_ROUTE */
#define ACMP_ROUTE_ACMPPEN                 (0x1UL << 0)                        /**< ACMP Output Pin Enable */
#define _ACMP_ROUTE_ACMPPEN_SHIFT          0                                   /**< Shift value for ACMP_ACMPPEN */
#define _ACMP_ROUTE_ACMPPEN_MASK           0x1UL                               /**< Bit mask for ACMP_ACMPPEN */
#define _ACMP_ROUTE_ACMPPEN_DEFAULT        0x00000000UL                        /**< Mode DEFAULT for ACMP_ROUTE */
#define ACMP_ROUTE_ACMPPEN_DEFAULT         (_ACMP_ROUTE_ACMPPEN_DEFAULT << 0)  /**< Shifted mode DEFAULT for ACMP_ROUTE */
#define _ACMP_ROUTE_LOCATION_SHIFT         8                                   /**< Shift value for ACMP_LOCATION */
#define _ACMP_ROUTE_LOCATION_MASK          0x700UL                             /**< Bit mask for ACMP_LOCATION */
#define _ACMP_ROUTE_LOCATION_LOC0          0x00000000UL                        /**< Mode LOC0 for ACMP_ROUTE */
#define _ACMP_ROUTE_LOCATION_DEFAULT       0x00000000UL                        /**< Mode DEFAULT for ACMP_ROUTE */
#define _ACMP_ROUTE_LOCATION_LOC1          0x00000001UL                        /**< Mode LOC1 for ACMP_ROUTE */
#define _ACMP_ROUTE_LOCATION_LOC2          0x00000002UL                        /**< Mode LOC2 for ACMP_ROUTE */
#define _ACMP_ROUTE_LOCATION_LOC3          0x00000003UL                        /**< Mode LOC3 for ACMP_ROUTE */
#define ACMP_ROUTE_LOCATION_LOC0           (_ACMP_ROUTE_LOCATION_LOC0 << 8)    /**< Shifted mode LOC0 for ACMP_ROUTE */
#define ACMP_ROUTE_LOCATION_DEFAULT        (_ACMP_ROUTE_LOCATION_DEFAULT << 8) /**< Shifted mode DEFAULT for ACMP_ROUTE */
#define ACMP_ROUTE_LOCATION_LOC1           (_ACMP_ROUTE_LOCATION_LOC1 << 8)    /**< Shifted mode LOC1 for ACMP_ROUTE */
#define ACMP_ROUTE_LOCATION_LOC2           (_ACMP_ROUTE_LOCATION_LOC2 << 8)    /**< Shifted mode LOC2 for ACMP_ROUTE */
#define ACMP_ROUTE_LOCATION_LOC3           (_ACMP_ROUTE_LOCATION_LOC3 << 8)    /**< Shifted mode LOC3 for ACMP_ROUTE */

/** @} End of group EFM32HG_ACMP */
/** @} End of group Parts */

