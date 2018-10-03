/**************************************************************************//**
 * @file efr32fg1p_prs.h
 * @brief EFR32FG1P_PRS register and bit field definitions
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
 * @defgroup EFR32FG1P_PRS
 * @{
 * @brief EFR32FG1P_PRS Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t SWPULSE;      /**< Software Pulse Register  */
  __IOM uint32_t SWLEVEL;      /**< Software Level Register  */
  __IOM uint32_t ROUTEPEN;     /**< I/O Routing Pin Enable Register  */
  uint32_t       RESERVED0[1]; /**< Reserved for future use **/
  __IOM uint32_t ROUTELOC0;    /**< I/O Routing Location Register  */
  __IOM uint32_t ROUTELOC1;    /**< I/O Routing Location Register  */
  __IOM uint32_t ROUTELOC2;    /**< I/O Routing Location Register  */

  uint32_t       RESERVED1[1]; /**< Reserved for future use **/
  __IOM uint32_t CTRL;         /**< Control Register  */
  __IOM uint32_t DMAREQ0;      /**< DMA Request 0 Register  */
  __IOM uint32_t DMAREQ1;      /**< DMA Request 1 Register  */
  uint32_t       RESERVED2[1]; /**< Reserved for future use **/
  __IM uint32_t  PEEK;         /**< PRS Channel Values  */

  uint32_t       RESERVED3[3]; /**< Reserved registers */
  PRS_CH_TypeDef CH[12];       /**< Channel registers */
} PRS_TypeDef;                 /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_PRS_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for PRS SWPULSE */
#define _PRS_SWPULSE_RESETVALUE                0x00000000UL                           /**< Default value for PRS_SWPULSE */
#define _PRS_SWPULSE_MASK                      0x00000FFFUL                           /**< Mask for PRS_SWPULSE */
#define PRS_SWPULSE_CH0PULSE                   (0x1UL << 0)                           /**< Channel 0 Pulse Generation */
#define _PRS_SWPULSE_CH0PULSE_SHIFT            0                                      /**< Shift value for PRS_CH0PULSE */
#define _PRS_SWPULSE_CH0PULSE_MASK             0x1UL                                  /**< Bit mask for PRS_CH0PULSE */
#define _PRS_SWPULSE_CH0PULSE_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH0PULSE_DEFAULT           (_PRS_SWPULSE_CH0PULSE_DEFAULT << 0)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH1PULSE                   (0x1UL << 1)                           /**< Channel 1 Pulse Generation */
#define _PRS_SWPULSE_CH1PULSE_SHIFT            1                                      /**< Shift value for PRS_CH1PULSE */
#define _PRS_SWPULSE_CH1PULSE_MASK             0x2UL                                  /**< Bit mask for PRS_CH1PULSE */
#define _PRS_SWPULSE_CH1PULSE_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH1PULSE_DEFAULT           (_PRS_SWPULSE_CH1PULSE_DEFAULT << 1)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH2PULSE                   (0x1UL << 2)                           /**< Channel 2 Pulse Generation */
#define _PRS_SWPULSE_CH2PULSE_SHIFT            2                                      /**< Shift value for PRS_CH2PULSE */
#define _PRS_SWPULSE_CH2PULSE_MASK             0x4UL                                  /**< Bit mask for PRS_CH2PULSE */
#define _PRS_SWPULSE_CH2PULSE_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH2PULSE_DEFAULT           (_PRS_SWPULSE_CH2PULSE_DEFAULT << 2)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH3PULSE                   (0x1UL << 3)                           /**< Channel 3 Pulse Generation */
#define _PRS_SWPULSE_CH3PULSE_SHIFT            3                                      /**< Shift value for PRS_CH3PULSE */
#define _PRS_SWPULSE_CH3PULSE_MASK             0x8UL                                  /**< Bit mask for PRS_CH3PULSE */
#define _PRS_SWPULSE_CH3PULSE_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH3PULSE_DEFAULT           (_PRS_SWPULSE_CH3PULSE_DEFAULT << 3)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH4PULSE                   (0x1UL << 4)                           /**< Channel 4 Pulse Generation */
#define _PRS_SWPULSE_CH4PULSE_SHIFT            4                                      /**< Shift value for PRS_CH4PULSE */
#define _PRS_SWPULSE_CH4PULSE_MASK             0x10UL                                 /**< Bit mask for PRS_CH4PULSE */
#define _PRS_SWPULSE_CH4PULSE_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH4PULSE_DEFAULT           (_PRS_SWPULSE_CH4PULSE_DEFAULT << 4)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH5PULSE                   (0x1UL << 5)                           /**< Channel 5 Pulse Generation */
#define _PRS_SWPULSE_CH5PULSE_SHIFT            5                                      /**< Shift value for PRS_CH5PULSE */
#define _PRS_SWPULSE_CH5PULSE_MASK             0x20UL                                 /**< Bit mask for PRS_CH5PULSE */
#define _PRS_SWPULSE_CH5PULSE_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH5PULSE_DEFAULT           (_PRS_SWPULSE_CH5PULSE_DEFAULT << 5)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH6PULSE                   (0x1UL << 6)                           /**< Channel 6 Pulse Generation */
#define _PRS_SWPULSE_CH6PULSE_SHIFT            6                                      /**< Shift value for PRS_CH6PULSE */
#define _PRS_SWPULSE_CH6PULSE_MASK             0x40UL                                 /**< Bit mask for PRS_CH6PULSE */
#define _PRS_SWPULSE_CH6PULSE_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH6PULSE_DEFAULT           (_PRS_SWPULSE_CH6PULSE_DEFAULT << 6)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH7PULSE                   (0x1UL << 7)                           /**< Channel 7 Pulse Generation */
#define _PRS_SWPULSE_CH7PULSE_SHIFT            7                                      /**< Shift value for PRS_CH7PULSE */
#define _PRS_SWPULSE_CH7PULSE_MASK             0x80UL                                 /**< Bit mask for PRS_CH7PULSE */
#define _PRS_SWPULSE_CH7PULSE_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH7PULSE_DEFAULT           (_PRS_SWPULSE_CH7PULSE_DEFAULT << 7)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH8PULSE                   (0x1UL << 8)                           /**< Channel 8 Pulse Generation */
#define _PRS_SWPULSE_CH8PULSE_SHIFT            8                                      /**< Shift value for PRS_CH8PULSE */
#define _PRS_SWPULSE_CH8PULSE_MASK             0x100UL                                /**< Bit mask for PRS_CH8PULSE */
#define _PRS_SWPULSE_CH8PULSE_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH8PULSE_DEFAULT           (_PRS_SWPULSE_CH8PULSE_DEFAULT << 8)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH9PULSE                   (0x1UL << 9)                           /**< Channel 9 Pulse Generation */
#define _PRS_SWPULSE_CH9PULSE_SHIFT            9                                      /**< Shift value for PRS_CH9PULSE */
#define _PRS_SWPULSE_CH9PULSE_MASK             0x200UL                                /**< Bit mask for PRS_CH9PULSE */
#define _PRS_SWPULSE_CH9PULSE_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH9PULSE_DEFAULT           (_PRS_SWPULSE_CH9PULSE_DEFAULT << 9)   /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH10PULSE                  (0x1UL << 10)                          /**< Channel 10 Pulse Generation */
#define _PRS_SWPULSE_CH10PULSE_SHIFT           10                                     /**< Shift value for PRS_CH10PULSE */
#define _PRS_SWPULSE_CH10PULSE_MASK            0x400UL                                /**< Bit mask for PRS_CH10PULSE */
#define _PRS_SWPULSE_CH10PULSE_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH10PULSE_DEFAULT          (_PRS_SWPULSE_CH10PULSE_DEFAULT << 10) /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH11PULSE                  (0x1UL << 11)                          /**< Channel 11 Pulse Generation */
#define _PRS_SWPULSE_CH11PULSE_SHIFT           11                                     /**< Shift value for PRS_CH11PULSE */
#define _PRS_SWPULSE_CH11PULSE_MASK            0x800UL                                /**< Bit mask for PRS_CH11PULSE */
#define _PRS_SWPULSE_CH11PULSE_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH11PULSE_DEFAULT          (_PRS_SWPULSE_CH11PULSE_DEFAULT << 11) /**< Shifted mode DEFAULT for PRS_SWPULSE */

/* Bit fields for PRS SWLEVEL */
#define _PRS_SWLEVEL_RESETVALUE                0x00000000UL                           /**< Default value for PRS_SWLEVEL */
#define _PRS_SWLEVEL_MASK                      0x00000FFFUL                           /**< Mask for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH0LEVEL                   (0x1UL << 0)                           /**< Channel 0 Software Level */
#define _PRS_SWLEVEL_CH0LEVEL_SHIFT            0                                      /**< Shift value for PRS_CH0LEVEL */
#define _PRS_SWLEVEL_CH0LEVEL_MASK             0x1UL                                  /**< Bit mask for PRS_CH0LEVEL */
#define _PRS_SWLEVEL_CH0LEVEL_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH0LEVEL_DEFAULT           (_PRS_SWLEVEL_CH0LEVEL_DEFAULT << 0)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH1LEVEL                   (0x1UL << 1)                           /**< Channel 1 Software Level */
#define _PRS_SWLEVEL_CH1LEVEL_SHIFT            1                                      /**< Shift value for PRS_CH1LEVEL */
#define _PRS_SWLEVEL_CH1LEVEL_MASK             0x2UL                                  /**< Bit mask for PRS_CH1LEVEL */
#define _PRS_SWLEVEL_CH1LEVEL_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH1LEVEL_DEFAULT           (_PRS_SWLEVEL_CH1LEVEL_DEFAULT << 1)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH2LEVEL                   (0x1UL << 2)                           /**< Channel 2 Software Level */
#define _PRS_SWLEVEL_CH2LEVEL_SHIFT            2                                      /**< Shift value for PRS_CH2LEVEL */
#define _PRS_SWLEVEL_CH2LEVEL_MASK             0x4UL                                  /**< Bit mask for PRS_CH2LEVEL */
#define _PRS_SWLEVEL_CH2LEVEL_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH2LEVEL_DEFAULT           (_PRS_SWLEVEL_CH2LEVEL_DEFAULT << 2)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH3LEVEL                   (0x1UL << 3)                           /**< Channel 3 Software Level */
#define _PRS_SWLEVEL_CH3LEVEL_SHIFT            3                                      /**< Shift value for PRS_CH3LEVEL */
#define _PRS_SWLEVEL_CH3LEVEL_MASK             0x8UL                                  /**< Bit mask for PRS_CH3LEVEL */
#define _PRS_SWLEVEL_CH3LEVEL_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH3LEVEL_DEFAULT           (_PRS_SWLEVEL_CH3LEVEL_DEFAULT << 3)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH4LEVEL                   (0x1UL << 4)                           /**< Channel 4 Software Level */
#define _PRS_SWLEVEL_CH4LEVEL_SHIFT            4                                      /**< Shift value for PRS_CH4LEVEL */
#define _PRS_SWLEVEL_CH4LEVEL_MASK             0x10UL                                 /**< Bit mask for PRS_CH4LEVEL */
#define _PRS_SWLEVEL_CH4LEVEL_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH4LEVEL_DEFAULT           (_PRS_SWLEVEL_CH4LEVEL_DEFAULT << 4)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH5LEVEL                   (0x1UL << 5)                           /**< Channel 5 Software Level */
#define _PRS_SWLEVEL_CH5LEVEL_SHIFT            5                                      /**< Shift value for PRS_CH5LEVEL */
#define _PRS_SWLEVEL_CH5LEVEL_MASK             0x20UL                                 /**< Bit mask for PRS_CH5LEVEL */
#define _PRS_SWLEVEL_CH5LEVEL_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH5LEVEL_DEFAULT           (_PRS_SWLEVEL_CH5LEVEL_DEFAULT << 5)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH6LEVEL                   (0x1UL << 6)                           /**< Channel 6 Software Level */
#define _PRS_SWLEVEL_CH6LEVEL_SHIFT            6                                      /**< Shift value for PRS_CH6LEVEL */
#define _PRS_SWLEVEL_CH6LEVEL_MASK             0x40UL                                 /**< Bit mask for PRS_CH6LEVEL */
#define _PRS_SWLEVEL_CH6LEVEL_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH6LEVEL_DEFAULT           (_PRS_SWLEVEL_CH6LEVEL_DEFAULT << 6)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH7LEVEL                   (0x1UL << 7)                           /**< Channel 7 Software Level */
#define _PRS_SWLEVEL_CH7LEVEL_SHIFT            7                                      /**< Shift value for PRS_CH7LEVEL */
#define _PRS_SWLEVEL_CH7LEVEL_MASK             0x80UL                                 /**< Bit mask for PRS_CH7LEVEL */
#define _PRS_SWLEVEL_CH7LEVEL_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH7LEVEL_DEFAULT           (_PRS_SWLEVEL_CH7LEVEL_DEFAULT << 7)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH8LEVEL                   (0x1UL << 8)                           /**< Channel 8 Software Level */
#define _PRS_SWLEVEL_CH8LEVEL_SHIFT            8                                      /**< Shift value for PRS_CH8LEVEL */
#define _PRS_SWLEVEL_CH8LEVEL_MASK             0x100UL                                /**< Bit mask for PRS_CH8LEVEL */
#define _PRS_SWLEVEL_CH8LEVEL_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH8LEVEL_DEFAULT           (_PRS_SWLEVEL_CH8LEVEL_DEFAULT << 8)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH9LEVEL                   (0x1UL << 9)                           /**< Channel 9 Software Level */
#define _PRS_SWLEVEL_CH9LEVEL_SHIFT            9                                      /**< Shift value for PRS_CH9LEVEL */
#define _PRS_SWLEVEL_CH9LEVEL_MASK             0x200UL                                /**< Bit mask for PRS_CH9LEVEL */
#define _PRS_SWLEVEL_CH9LEVEL_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH9LEVEL_DEFAULT           (_PRS_SWLEVEL_CH9LEVEL_DEFAULT << 9)   /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH10LEVEL                  (0x1UL << 10)                          /**< Channel 10 Software Level */
#define _PRS_SWLEVEL_CH10LEVEL_SHIFT           10                                     /**< Shift value for PRS_CH10LEVEL */
#define _PRS_SWLEVEL_CH10LEVEL_MASK            0x400UL                                /**< Bit mask for PRS_CH10LEVEL */
#define _PRS_SWLEVEL_CH10LEVEL_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH10LEVEL_DEFAULT          (_PRS_SWLEVEL_CH10LEVEL_DEFAULT << 10) /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH11LEVEL                  (0x1UL << 11)                          /**< Channel 11 Software Level */
#define _PRS_SWLEVEL_CH11LEVEL_SHIFT           11                                     /**< Shift value for PRS_CH11LEVEL */
#define _PRS_SWLEVEL_CH11LEVEL_MASK            0x800UL                                /**< Bit mask for PRS_CH11LEVEL */
#define _PRS_SWLEVEL_CH11LEVEL_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH11LEVEL_DEFAULT          (_PRS_SWLEVEL_CH11LEVEL_DEFAULT << 11) /**< Shifted mode DEFAULT for PRS_SWLEVEL */

/* Bit fields for PRS ROUTEPEN */
#define _PRS_ROUTEPEN_RESETVALUE               0x00000000UL                          /**< Default value for PRS_ROUTEPEN */
#define _PRS_ROUTEPEN_MASK                     0x00000FFFUL                          /**< Mask for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH0PEN                    (0x1UL << 0)                          /**< CH0 Pin Enable */
#define _PRS_ROUTEPEN_CH0PEN_SHIFT             0                                     /**< Shift value for PRS_CH0PEN */
#define _PRS_ROUTEPEN_CH0PEN_MASK              0x1UL                                 /**< Bit mask for PRS_CH0PEN */
#define _PRS_ROUTEPEN_CH0PEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH0PEN_DEFAULT            (_PRS_ROUTEPEN_CH0PEN_DEFAULT << 0)   /**< Shifted mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH1PEN                    (0x1UL << 1)                          /**< CH1 Pin Enable */
#define _PRS_ROUTEPEN_CH1PEN_SHIFT             1                                     /**< Shift value for PRS_CH1PEN */
#define _PRS_ROUTEPEN_CH1PEN_MASK              0x2UL                                 /**< Bit mask for PRS_CH1PEN */
#define _PRS_ROUTEPEN_CH1PEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH1PEN_DEFAULT            (_PRS_ROUTEPEN_CH1PEN_DEFAULT << 1)   /**< Shifted mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH2PEN                    (0x1UL << 2)                          /**< CH2 Pin Enable */
#define _PRS_ROUTEPEN_CH2PEN_SHIFT             2                                     /**< Shift value for PRS_CH2PEN */
#define _PRS_ROUTEPEN_CH2PEN_MASK              0x4UL                                 /**< Bit mask for PRS_CH2PEN */
#define _PRS_ROUTEPEN_CH2PEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH2PEN_DEFAULT            (_PRS_ROUTEPEN_CH2PEN_DEFAULT << 2)   /**< Shifted mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH3PEN                    (0x1UL << 3)                          /**< CH3 Pin Enable */
#define _PRS_ROUTEPEN_CH3PEN_SHIFT             3                                     /**< Shift value for PRS_CH3PEN */
#define _PRS_ROUTEPEN_CH3PEN_MASK              0x8UL                                 /**< Bit mask for PRS_CH3PEN */
#define _PRS_ROUTEPEN_CH3PEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH3PEN_DEFAULT            (_PRS_ROUTEPEN_CH3PEN_DEFAULT << 3)   /**< Shifted mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH4PEN                    (0x1UL << 4)                          /**< CH4 Pin Enable */
#define _PRS_ROUTEPEN_CH4PEN_SHIFT             4                                     /**< Shift value for PRS_CH4PEN */
#define _PRS_ROUTEPEN_CH4PEN_MASK              0x10UL                                /**< Bit mask for PRS_CH4PEN */
#define _PRS_ROUTEPEN_CH4PEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH4PEN_DEFAULT            (_PRS_ROUTEPEN_CH4PEN_DEFAULT << 4)   /**< Shifted mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH5PEN                    (0x1UL << 5)                          /**< CH5 Pin Enable */
#define _PRS_ROUTEPEN_CH5PEN_SHIFT             5                                     /**< Shift value for PRS_CH5PEN */
#define _PRS_ROUTEPEN_CH5PEN_MASK              0x20UL                                /**< Bit mask for PRS_CH5PEN */
#define _PRS_ROUTEPEN_CH5PEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH5PEN_DEFAULT            (_PRS_ROUTEPEN_CH5PEN_DEFAULT << 5)   /**< Shifted mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH6PEN                    (0x1UL << 6)                          /**< CH6 Pin Enable */
#define _PRS_ROUTEPEN_CH6PEN_SHIFT             6                                     /**< Shift value for PRS_CH6PEN */
#define _PRS_ROUTEPEN_CH6PEN_MASK              0x40UL                                /**< Bit mask for PRS_CH6PEN */
#define _PRS_ROUTEPEN_CH6PEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH6PEN_DEFAULT            (_PRS_ROUTEPEN_CH6PEN_DEFAULT << 6)   /**< Shifted mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH7PEN                    (0x1UL << 7)                          /**< CH7 Pin Enable */
#define _PRS_ROUTEPEN_CH7PEN_SHIFT             7                                     /**< Shift value for PRS_CH7PEN */
#define _PRS_ROUTEPEN_CH7PEN_MASK              0x80UL                                /**< Bit mask for PRS_CH7PEN */
#define _PRS_ROUTEPEN_CH7PEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH7PEN_DEFAULT            (_PRS_ROUTEPEN_CH7PEN_DEFAULT << 7)   /**< Shifted mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH8PEN                    (0x1UL << 8)                          /**< CH8 Pin Enable */
#define _PRS_ROUTEPEN_CH8PEN_SHIFT             8                                     /**< Shift value for PRS_CH8PEN */
#define _PRS_ROUTEPEN_CH8PEN_MASK              0x100UL                               /**< Bit mask for PRS_CH8PEN */
#define _PRS_ROUTEPEN_CH8PEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH8PEN_DEFAULT            (_PRS_ROUTEPEN_CH8PEN_DEFAULT << 8)   /**< Shifted mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH9PEN                    (0x1UL << 9)                          /**< CH9 Pin Enable */
#define _PRS_ROUTEPEN_CH9PEN_SHIFT             9                                     /**< Shift value for PRS_CH9PEN */
#define _PRS_ROUTEPEN_CH9PEN_MASK              0x200UL                               /**< Bit mask for PRS_CH9PEN */
#define _PRS_ROUTEPEN_CH9PEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH9PEN_DEFAULT            (_PRS_ROUTEPEN_CH9PEN_DEFAULT << 9)   /**< Shifted mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH10PEN                   (0x1UL << 10)                         /**< CH10 Pin Enable */
#define _PRS_ROUTEPEN_CH10PEN_SHIFT            10                                    /**< Shift value for PRS_CH10PEN */
#define _PRS_ROUTEPEN_CH10PEN_MASK             0x400UL                               /**< Bit mask for PRS_CH10PEN */
#define _PRS_ROUTEPEN_CH10PEN_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH10PEN_DEFAULT           (_PRS_ROUTEPEN_CH10PEN_DEFAULT << 10) /**< Shifted mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH11PEN                   (0x1UL << 11)                         /**< CH11 Pin Enable */
#define _PRS_ROUTEPEN_CH11PEN_SHIFT            11                                    /**< Shift value for PRS_CH11PEN */
#define _PRS_ROUTEPEN_CH11PEN_MASK             0x800UL                               /**< Bit mask for PRS_CH11PEN */
#define _PRS_ROUTEPEN_CH11PEN_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTEPEN */
#define PRS_ROUTEPEN_CH11PEN_DEFAULT           (_PRS_ROUTEPEN_CH11PEN_DEFAULT << 11) /**< Shifted mode DEFAULT for PRS_ROUTEPEN */

/* Bit fields for PRS ROUTELOC0 */
#define _PRS_ROUTELOC0_RESETVALUE              0x00000000UL                          /**< Default value for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_MASK                    0x0F07070FUL                          /**< Mask for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_SHIFT            0                                     /**< Shift value for PRS_CH0LOC */
#define _PRS_ROUTELOC0_CH0LOC_MASK             0xFUL                                 /**< Bit mask for PRS_CH0LOC */
#define _PRS_ROUTELOC0_CH0LOC_LOC0             0x00000000UL                          /**< Mode LOC0 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_LOC1             0x00000001UL                          /**< Mode LOC1 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_LOC2             0x00000002UL                          /**< Mode LOC2 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_LOC3             0x00000003UL                          /**< Mode LOC3 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_LOC4             0x00000004UL                          /**< Mode LOC4 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_LOC5             0x00000005UL                          /**< Mode LOC5 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_LOC6             0x00000006UL                          /**< Mode LOC6 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_LOC7             0x00000007UL                          /**< Mode LOC7 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_LOC8             0x00000008UL                          /**< Mode LOC8 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_LOC9             0x00000009UL                          /**< Mode LOC9 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_LOC10            0x0000000AUL                          /**< Mode LOC10 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_LOC11            0x0000000BUL                          /**< Mode LOC11 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_LOC12            0x0000000CUL                          /**< Mode LOC12 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH0LOC_LOC13            0x0000000DUL                          /**< Mode LOC13 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC0              (_PRS_ROUTELOC0_CH0LOC_LOC0 << 0)     /**< Shifted mode LOC0 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_DEFAULT           (_PRS_ROUTELOC0_CH0LOC_DEFAULT << 0)  /**< Shifted mode DEFAULT for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC1              (_PRS_ROUTELOC0_CH0LOC_LOC1 << 0)     /**< Shifted mode LOC1 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC2              (_PRS_ROUTELOC0_CH0LOC_LOC2 << 0)     /**< Shifted mode LOC2 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC3              (_PRS_ROUTELOC0_CH0LOC_LOC3 << 0)     /**< Shifted mode LOC3 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC4              (_PRS_ROUTELOC0_CH0LOC_LOC4 << 0)     /**< Shifted mode LOC4 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC5              (_PRS_ROUTELOC0_CH0LOC_LOC5 << 0)     /**< Shifted mode LOC5 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC6              (_PRS_ROUTELOC0_CH0LOC_LOC6 << 0)     /**< Shifted mode LOC6 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC7              (_PRS_ROUTELOC0_CH0LOC_LOC7 << 0)     /**< Shifted mode LOC7 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC8              (_PRS_ROUTELOC0_CH0LOC_LOC8 << 0)     /**< Shifted mode LOC8 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC9              (_PRS_ROUTELOC0_CH0LOC_LOC9 << 0)     /**< Shifted mode LOC9 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC10             (_PRS_ROUTELOC0_CH0LOC_LOC10 << 0)    /**< Shifted mode LOC10 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC11             (_PRS_ROUTELOC0_CH0LOC_LOC11 << 0)    /**< Shifted mode LOC11 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC12             (_PRS_ROUTELOC0_CH0LOC_LOC12 << 0)    /**< Shifted mode LOC12 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH0LOC_LOC13             (_PRS_ROUTELOC0_CH0LOC_LOC13 << 0)    /**< Shifted mode LOC13 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH1LOC_SHIFT            8                                     /**< Shift value for PRS_CH1LOC */
#define _PRS_ROUTELOC0_CH1LOC_MASK             0x700UL                               /**< Bit mask for PRS_CH1LOC */
#define _PRS_ROUTELOC0_CH1LOC_LOC0             0x00000000UL                          /**< Mode LOC0 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH1LOC_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH1LOC_LOC1             0x00000001UL                          /**< Mode LOC1 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH1LOC_LOC2             0x00000002UL                          /**< Mode LOC2 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH1LOC_LOC3             0x00000003UL                          /**< Mode LOC3 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH1LOC_LOC4             0x00000004UL                          /**< Mode LOC4 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH1LOC_LOC5             0x00000005UL                          /**< Mode LOC5 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH1LOC_LOC6             0x00000006UL                          /**< Mode LOC6 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH1LOC_LOC7             0x00000007UL                          /**< Mode LOC7 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH1LOC_LOC0              (_PRS_ROUTELOC0_CH1LOC_LOC0 << 8)     /**< Shifted mode LOC0 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH1LOC_DEFAULT           (_PRS_ROUTELOC0_CH1LOC_DEFAULT << 8)  /**< Shifted mode DEFAULT for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH1LOC_LOC1              (_PRS_ROUTELOC0_CH1LOC_LOC1 << 8)     /**< Shifted mode LOC1 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH1LOC_LOC2              (_PRS_ROUTELOC0_CH1LOC_LOC2 << 8)     /**< Shifted mode LOC2 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH1LOC_LOC3              (_PRS_ROUTELOC0_CH1LOC_LOC3 << 8)     /**< Shifted mode LOC3 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH1LOC_LOC4              (_PRS_ROUTELOC0_CH1LOC_LOC4 << 8)     /**< Shifted mode LOC4 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH1LOC_LOC5              (_PRS_ROUTELOC0_CH1LOC_LOC5 << 8)     /**< Shifted mode LOC5 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH1LOC_LOC6              (_PRS_ROUTELOC0_CH1LOC_LOC6 << 8)     /**< Shifted mode LOC6 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH1LOC_LOC7              (_PRS_ROUTELOC0_CH1LOC_LOC7 << 8)     /**< Shifted mode LOC7 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH2LOC_SHIFT            16                                    /**< Shift value for PRS_CH2LOC */
#define _PRS_ROUTELOC0_CH2LOC_MASK             0x70000UL                             /**< Bit mask for PRS_CH2LOC */
#define _PRS_ROUTELOC0_CH2LOC_LOC0             0x00000000UL                          /**< Mode LOC0 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH2LOC_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH2LOC_LOC1             0x00000001UL                          /**< Mode LOC1 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH2LOC_LOC2             0x00000002UL                          /**< Mode LOC2 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH2LOC_LOC3             0x00000003UL                          /**< Mode LOC3 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH2LOC_LOC4             0x00000004UL                          /**< Mode LOC4 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH2LOC_LOC5             0x00000005UL                          /**< Mode LOC5 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH2LOC_LOC6             0x00000006UL                          /**< Mode LOC6 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH2LOC_LOC7             0x00000007UL                          /**< Mode LOC7 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH2LOC_LOC0              (_PRS_ROUTELOC0_CH2LOC_LOC0 << 16)    /**< Shifted mode LOC0 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH2LOC_DEFAULT           (_PRS_ROUTELOC0_CH2LOC_DEFAULT << 16) /**< Shifted mode DEFAULT for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH2LOC_LOC1              (_PRS_ROUTELOC0_CH2LOC_LOC1 << 16)    /**< Shifted mode LOC1 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH2LOC_LOC2              (_PRS_ROUTELOC0_CH2LOC_LOC2 << 16)    /**< Shifted mode LOC2 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH2LOC_LOC3              (_PRS_ROUTELOC0_CH2LOC_LOC3 << 16)    /**< Shifted mode LOC3 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH2LOC_LOC4              (_PRS_ROUTELOC0_CH2LOC_LOC4 << 16)    /**< Shifted mode LOC4 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH2LOC_LOC5              (_PRS_ROUTELOC0_CH2LOC_LOC5 << 16)    /**< Shifted mode LOC5 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH2LOC_LOC6              (_PRS_ROUTELOC0_CH2LOC_LOC6 << 16)    /**< Shifted mode LOC6 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH2LOC_LOC7              (_PRS_ROUTELOC0_CH2LOC_LOC7 << 16)    /**< Shifted mode LOC7 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_SHIFT            24                                    /**< Shift value for PRS_CH3LOC */
#define _PRS_ROUTELOC0_CH3LOC_MASK             0xF000000UL                           /**< Bit mask for PRS_CH3LOC */
#define _PRS_ROUTELOC0_CH3LOC_LOC0             0x00000000UL                          /**< Mode LOC0 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC1             0x00000001UL                          /**< Mode LOC1 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC2             0x00000002UL                          /**< Mode LOC2 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC3             0x00000003UL                          /**< Mode LOC3 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC4             0x00000004UL                          /**< Mode LOC4 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC5             0x00000005UL                          /**< Mode LOC5 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC6             0x00000006UL                          /**< Mode LOC6 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC7             0x00000007UL                          /**< Mode LOC7 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC8             0x00000008UL                          /**< Mode LOC8 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC9             0x00000009UL                          /**< Mode LOC9 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC10            0x0000000AUL                          /**< Mode LOC10 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC11            0x0000000BUL                          /**< Mode LOC11 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC12            0x0000000CUL                          /**< Mode LOC12 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC13            0x0000000DUL                          /**< Mode LOC13 for PRS_ROUTELOC0 */
#define _PRS_ROUTELOC0_CH3LOC_LOC14            0x0000000EUL                          /**< Mode LOC14 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC0              (_PRS_ROUTELOC0_CH3LOC_LOC0 << 24)    /**< Shifted mode LOC0 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_DEFAULT           (_PRS_ROUTELOC0_CH3LOC_DEFAULT << 24) /**< Shifted mode DEFAULT for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC1              (_PRS_ROUTELOC0_CH3LOC_LOC1 << 24)    /**< Shifted mode LOC1 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC2              (_PRS_ROUTELOC0_CH3LOC_LOC2 << 24)    /**< Shifted mode LOC2 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC3              (_PRS_ROUTELOC0_CH3LOC_LOC3 << 24)    /**< Shifted mode LOC3 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC4              (_PRS_ROUTELOC0_CH3LOC_LOC4 << 24)    /**< Shifted mode LOC4 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC5              (_PRS_ROUTELOC0_CH3LOC_LOC5 << 24)    /**< Shifted mode LOC5 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC6              (_PRS_ROUTELOC0_CH3LOC_LOC6 << 24)    /**< Shifted mode LOC6 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC7              (_PRS_ROUTELOC0_CH3LOC_LOC7 << 24)    /**< Shifted mode LOC7 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC8              (_PRS_ROUTELOC0_CH3LOC_LOC8 << 24)    /**< Shifted mode LOC8 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC9              (_PRS_ROUTELOC0_CH3LOC_LOC9 << 24)    /**< Shifted mode LOC9 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC10             (_PRS_ROUTELOC0_CH3LOC_LOC10 << 24)   /**< Shifted mode LOC10 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC11             (_PRS_ROUTELOC0_CH3LOC_LOC11 << 24)   /**< Shifted mode LOC11 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC12             (_PRS_ROUTELOC0_CH3LOC_LOC12 << 24)   /**< Shifted mode LOC12 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC13             (_PRS_ROUTELOC0_CH3LOC_LOC13 << 24)   /**< Shifted mode LOC13 for PRS_ROUTELOC0 */
#define PRS_ROUTELOC0_CH3LOC_LOC14             (_PRS_ROUTELOC0_CH3LOC_LOC14 << 24)   /**< Shifted mode LOC14 for PRS_ROUTELOC0 */

/* Bit fields for PRS ROUTELOC1 */
#define _PRS_ROUTELOC1_RESETVALUE              0x00000000UL                          /**< Default value for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_MASK                    0x0F1F0707UL                          /**< Mask for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH4LOC_SHIFT            0                                     /**< Shift value for PRS_CH4LOC */
#define _PRS_ROUTELOC1_CH4LOC_MASK             0x7UL                                 /**< Bit mask for PRS_CH4LOC */
#define _PRS_ROUTELOC1_CH4LOC_LOC0             0x00000000UL                          /**< Mode LOC0 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH4LOC_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH4LOC_LOC1             0x00000001UL                          /**< Mode LOC1 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH4LOC_LOC2             0x00000002UL                          /**< Mode LOC2 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH4LOC_LOC3             0x00000003UL                          /**< Mode LOC3 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH4LOC_LOC4             0x00000004UL                          /**< Mode LOC4 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH4LOC_LOC5             0x00000005UL                          /**< Mode LOC5 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH4LOC_LOC6             0x00000006UL                          /**< Mode LOC6 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH4LOC_LOC0              (_PRS_ROUTELOC1_CH4LOC_LOC0 << 0)     /**< Shifted mode LOC0 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH4LOC_DEFAULT           (_PRS_ROUTELOC1_CH4LOC_DEFAULT << 0)  /**< Shifted mode DEFAULT for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH4LOC_LOC1              (_PRS_ROUTELOC1_CH4LOC_LOC1 << 0)     /**< Shifted mode LOC1 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH4LOC_LOC2              (_PRS_ROUTELOC1_CH4LOC_LOC2 << 0)     /**< Shifted mode LOC2 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH4LOC_LOC3              (_PRS_ROUTELOC1_CH4LOC_LOC3 << 0)     /**< Shifted mode LOC3 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH4LOC_LOC4              (_PRS_ROUTELOC1_CH4LOC_LOC4 << 0)     /**< Shifted mode LOC4 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH4LOC_LOC5              (_PRS_ROUTELOC1_CH4LOC_LOC5 << 0)     /**< Shifted mode LOC5 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH4LOC_LOC6              (_PRS_ROUTELOC1_CH4LOC_LOC6 << 0)     /**< Shifted mode LOC6 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH5LOC_SHIFT            8                                     /**< Shift value for PRS_CH5LOC */
#define _PRS_ROUTELOC1_CH5LOC_MASK             0x700UL                               /**< Bit mask for PRS_CH5LOC */
#define _PRS_ROUTELOC1_CH5LOC_LOC0             0x00000000UL                          /**< Mode LOC0 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH5LOC_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH5LOC_LOC1             0x00000001UL                          /**< Mode LOC1 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH5LOC_LOC2             0x00000002UL                          /**< Mode LOC2 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH5LOC_LOC3             0x00000003UL                          /**< Mode LOC3 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH5LOC_LOC4             0x00000004UL                          /**< Mode LOC4 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH5LOC_LOC5             0x00000005UL                          /**< Mode LOC5 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH5LOC_LOC6             0x00000006UL                          /**< Mode LOC6 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH5LOC_LOC0              (_PRS_ROUTELOC1_CH5LOC_LOC0 << 8)     /**< Shifted mode LOC0 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH5LOC_DEFAULT           (_PRS_ROUTELOC1_CH5LOC_DEFAULT << 8)  /**< Shifted mode DEFAULT for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH5LOC_LOC1              (_PRS_ROUTELOC1_CH5LOC_LOC1 << 8)     /**< Shifted mode LOC1 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH5LOC_LOC2              (_PRS_ROUTELOC1_CH5LOC_LOC2 << 8)     /**< Shifted mode LOC2 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH5LOC_LOC3              (_PRS_ROUTELOC1_CH5LOC_LOC3 << 8)     /**< Shifted mode LOC3 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH5LOC_LOC4              (_PRS_ROUTELOC1_CH5LOC_LOC4 << 8)     /**< Shifted mode LOC4 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH5LOC_LOC5              (_PRS_ROUTELOC1_CH5LOC_LOC5 << 8)     /**< Shifted mode LOC5 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH5LOC_LOC6              (_PRS_ROUTELOC1_CH5LOC_LOC6 << 8)     /**< Shifted mode LOC6 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_SHIFT            16                                    /**< Shift value for PRS_CH6LOC */
#define _PRS_ROUTELOC1_CH6LOC_MASK             0x1F0000UL                            /**< Bit mask for PRS_CH6LOC */
#define _PRS_ROUTELOC1_CH6LOC_LOC0             0x00000000UL                          /**< Mode LOC0 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC1             0x00000001UL                          /**< Mode LOC1 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC2             0x00000002UL                          /**< Mode LOC2 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC3             0x00000003UL                          /**< Mode LOC3 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC4             0x00000004UL                          /**< Mode LOC4 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC5             0x00000005UL                          /**< Mode LOC5 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC6             0x00000006UL                          /**< Mode LOC6 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC7             0x00000007UL                          /**< Mode LOC7 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC8             0x00000008UL                          /**< Mode LOC8 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC9             0x00000009UL                          /**< Mode LOC9 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC10            0x0000000AUL                          /**< Mode LOC10 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC11            0x0000000BUL                          /**< Mode LOC11 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC12            0x0000000CUL                          /**< Mode LOC12 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC13            0x0000000DUL                          /**< Mode LOC13 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC14            0x0000000EUL                          /**< Mode LOC14 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC15            0x0000000FUL                          /**< Mode LOC15 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC16            0x00000010UL                          /**< Mode LOC16 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH6LOC_LOC17            0x00000011UL                          /**< Mode LOC17 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC0              (_PRS_ROUTELOC1_CH6LOC_LOC0 << 16)    /**< Shifted mode LOC0 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_DEFAULT           (_PRS_ROUTELOC1_CH6LOC_DEFAULT << 16) /**< Shifted mode DEFAULT for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC1              (_PRS_ROUTELOC1_CH6LOC_LOC1 << 16)    /**< Shifted mode LOC1 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC2              (_PRS_ROUTELOC1_CH6LOC_LOC2 << 16)    /**< Shifted mode LOC2 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC3              (_PRS_ROUTELOC1_CH6LOC_LOC3 << 16)    /**< Shifted mode LOC3 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC4              (_PRS_ROUTELOC1_CH6LOC_LOC4 << 16)    /**< Shifted mode LOC4 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC5              (_PRS_ROUTELOC1_CH6LOC_LOC5 << 16)    /**< Shifted mode LOC5 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC6              (_PRS_ROUTELOC1_CH6LOC_LOC6 << 16)    /**< Shifted mode LOC6 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC7              (_PRS_ROUTELOC1_CH6LOC_LOC7 << 16)    /**< Shifted mode LOC7 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC8              (_PRS_ROUTELOC1_CH6LOC_LOC8 << 16)    /**< Shifted mode LOC8 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC9              (_PRS_ROUTELOC1_CH6LOC_LOC9 << 16)    /**< Shifted mode LOC9 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC10             (_PRS_ROUTELOC1_CH6LOC_LOC10 << 16)   /**< Shifted mode LOC10 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC11             (_PRS_ROUTELOC1_CH6LOC_LOC11 << 16)   /**< Shifted mode LOC11 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC12             (_PRS_ROUTELOC1_CH6LOC_LOC12 << 16)   /**< Shifted mode LOC12 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC13             (_PRS_ROUTELOC1_CH6LOC_LOC13 << 16)   /**< Shifted mode LOC13 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC14             (_PRS_ROUTELOC1_CH6LOC_LOC14 << 16)   /**< Shifted mode LOC14 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC15             (_PRS_ROUTELOC1_CH6LOC_LOC15 << 16)   /**< Shifted mode LOC15 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC16             (_PRS_ROUTELOC1_CH6LOC_LOC16 << 16)   /**< Shifted mode LOC16 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH6LOC_LOC17             (_PRS_ROUTELOC1_CH6LOC_LOC17 << 16)   /**< Shifted mode LOC17 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH7LOC_SHIFT            24                                    /**< Shift value for PRS_CH7LOC */
#define _PRS_ROUTELOC1_CH7LOC_MASK             0xF000000UL                           /**< Bit mask for PRS_CH7LOC */
#define _PRS_ROUTELOC1_CH7LOC_LOC0             0x00000000UL                          /**< Mode LOC0 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH7LOC_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH7LOC_LOC1             0x00000001UL                          /**< Mode LOC1 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH7LOC_LOC2             0x00000002UL                          /**< Mode LOC2 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH7LOC_LOC3             0x00000003UL                          /**< Mode LOC3 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH7LOC_LOC4             0x00000004UL                          /**< Mode LOC4 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH7LOC_LOC5             0x00000005UL                          /**< Mode LOC5 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH7LOC_LOC6             0x00000006UL                          /**< Mode LOC6 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH7LOC_LOC7             0x00000007UL                          /**< Mode LOC7 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH7LOC_LOC8             0x00000008UL                          /**< Mode LOC8 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH7LOC_LOC9             0x00000009UL                          /**< Mode LOC9 for PRS_ROUTELOC1 */
#define _PRS_ROUTELOC1_CH7LOC_LOC10            0x0000000AUL                          /**< Mode LOC10 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH7LOC_LOC0              (_PRS_ROUTELOC1_CH7LOC_LOC0 << 24)    /**< Shifted mode LOC0 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH7LOC_DEFAULT           (_PRS_ROUTELOC1_CH7LOC_DEFAULT << 24) /**< Shifted mode DEFAULT for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH7LOC_LOC1              (_PRS_ROUTELOC1_CH7LOC_LOC1 << 24)    /**< Shifted mode LOC1 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH7LOC_LOC2              (_PRS_ROUTELOC1_CH7LOC_LOC2 << 24)    /**< Shifted mode LOC2 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH7LOC_LOC3              (_PRS_ROUTELOC1_CH7LOC_LOC3 << 24)    /**< Shifted mode LOC3 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH7LOC_LOC4              (_PRS_ROUTELOC1_CH7LOC_LOC4 << 24)    /**< Shifted mode LOC4 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH7LOC_LOC5              (_PRS_ROUTELOC1_CH7LOC_LOC5 << 24)    /**< Shifted mode LOC5 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH7LOC_LOC6              (_PRS_ROUTELOC1_CH7LOC_LOC6 << 24)    /**< Shifted mode LOC6 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH7LOC_LOC7              (_PRS_ROUTELOC1_CH7LOC_LOC7 << 24)    /**< Shifted mode LOC7 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH7LOC_LOC8              (_PRS_ROUTELOC1_CH7LOC_LOC8 << 24)    /**< Shifted mode LOC8 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH7LOC_LOC9              (_PRS_ROUTELOC1_CH7LOC_LOC9 << 24)    /**< Shifted mode LOC9 for PRS_ROUTELOC1 */
#define PRS_ROUTELOC1_CH7LOC_LOC10             (_PRS_ROUTELOC1_CH7LOC_LOC10 << 24)   /**< Shifted mode LOC10 for PRS_ROUTELOC1 */

/* Bit fields for PRS ROUTELOC2 */
#define _PRS_ROUTELOC2_RESETVALUE              0x00000000UL                           /**< Default value for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_MASK                    0x07071F0FUL                           /**< Mask for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH8LOC_SHIFT            0                                      /**< Shift value for PRS_CH8LOC */
#define _PRS_ROUTELOC2_CH8LOC_MASK             0xFUL                                  /**< Bit mask for PRS_CH8LOC */
#define _PRS_ROUTELOC2_CH8LOC_LOC0             0x00000000UL                           /**< Mode LOC0 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH8LOC_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH8LOC_LOC1             0x00000001UL                           /**< Mode LOC1 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH8LOC_LOC2             0x00000002UL                           /**< Mode LOC2 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH8LOC_LOC3             0x00000003UL                           /**< Mode LOC3 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH8LOC_LOC4             0x00000004UL                           /**< Mode LOC4 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH8LOC_LOC5             0x00000005UL                           /**< Mode LOC5 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH8LOC_LOC6             0x00000006UL                           /**< Mode LOC6 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH8LOC_LOC7             0x00000007UL                           /**< Mode LOC7 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH8LOC_LOC8             0x00000008UL                           /**< Mode LOC8 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH8LOC_LOC9             0x00000009UL                           /**< Mode LOC9 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH8LOC_LOC10            0x0000000AUL                           /**< Mode LOC10 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH8LOC_LOC0              (_PRS_ROUTELOC2_CH8LOC_LOC0 << 0)      /**< Shifted mode LOC0 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH8LOC_DEFAULT           (_PRS_ROUTELOC2_CH8LOC_DEFAULT << 0)   /**< Shifted mode DEFAULT for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH8LOC_LOC1              (_PRS_ROUTELOC2_CH8LOC_LOC1 << 0)      /**< Shifted mode LOC1 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH8LOC_LOC2              (_PRS_ROUTELOC2_CH8LOC_LOC2 << 0)      /**< Shifted mode LOC2 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH8LOC_LOC3              (_PRS_ROUTELOC2_CH8LOC_LOC3 << 0)      /**< Shifted mode LOC3 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH8LOC_LOC4              (_PRS_ROUTELOC2_CH8LOC_LOC4 << 0)      /**< Shifted mode LOC4 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH8LOC_LOC5              (_PRS_ROUTELOC2_CH8LOC_LOC5 << 0)      /**< Shifted mode LOC5 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH8LOC_LOC6              (_PRS_ROUTELOC2_CH8LOC_LOC6 << 0)      /**< Shifted mode LOC6 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH8LOC_LOC7              (_PRS_ROUTELOC2_CH8LOC_LOC7 << 0)      /**< Shifted mode LOC7 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH8LOC_LOC8              (_PRS_ROUTELOC2_CH8LOC_LOC8 << 0)      /**< Shifted mode LOC8 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH8LOC_LOC9              (_PRS_ROUTELOC2_CH8LOC_LOC9 << 0)      /**< Shifted mode LOC9 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH8LOC_LOC10             (_PRS_ROUTELOC2_CH8LOC_LOC10 << 0)     /**< Shifted mode LOC10 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_SHIFT            8                                      /**< Shift value for PRS_CH9LOC */
#define _PRS_ROUTELOC2_CH9LOC_MASK             0x1F00UL                               /**< Bit mask for PRS_CH9LOC */
#define _PRS_ROUTELOC2_CH9LOC_LOC0             0x00000000UL                           /**< Mode LOC0 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC1             0x00000001UL                           /**< Mode LOC1 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC2             0x00000002UL                           /**< Mode LOC2 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC3             0x00000003UL                           /**< Mode LOC3 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC4             0x00000004UL                           /**< Mode LOC4 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC5             0x00000005UL                           /**< Mode LOC5 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC6             0x00000006UL                           /**< Mode LOC6 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC7             0x00000007UL                           /**< Mode LOC7 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC8             0x00000008UL                           /**< Mode LOC8 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC9             0x00000009UL                           /**< Mode LOC9 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC10            0x0000000AUL                           /**< Mode LOC10 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC11            0x0000000BUL                           /**< Mode LOC11 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC12            0x0000000CUL                           /**< Mode LOC12 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC13            0x0000000DUL                           /**< Mode LOC13 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC14            0x0000000EUL                           /**< Mode LOC14 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC15            0x0000000FUL                           /**< Mode LOC15 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH9LOC_LOC16            0x00000010UL                           /**< Mode LOC16 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC0              (_PRS_ROUTELOC2_CH9LOC_LOC0 << 8)      /**< Shifted mode LOC0 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_DEFAULT           (_PRS_ROUTELOC2_CH9LOC_DEFAULT << 8)   /**< Shifted mode DEFAULT for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC1              (_PRS_ROUTELOC2_CH9LOC_LOC1 << 8)      /**< Shifted mode LOC1 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC2              (_PRS_ROUTELOC2_CH9LOC_LOC2 << 8)      /**< Shifted mode LOC2 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC3              (_PRS_ROUTELOC2_CH9LOC_LOC3 << 8)      /**< Shifted mode LOC3 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC4              (_PRS_ROUTELOC2_CH9LOC_LOC4 << 8)      /**< Shifted mode LOC4 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC5              (_PRS_ROUTELOC2_CH9LOC_LOC5 << 8)      /**< Shifted mode LOC5 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC6              (_PRS_ROUTELOC2_CH9LOC_LOC6 << 8)      /**< Shifted mode LOC6 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC7              (_PRS_ROUTELOC2_CH9LOC_LOC7 << 8)      /**< Shifted mode LOC7 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC8              (_PRS_ROUTELOC2_CH9LOC_LOC8 << 8)      /**< Shifted mode LOC8 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC9              (_PRS_ROUTELOC2_CH9LOC_LOC9 << 8)      /**< Shifted mode LOC9 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC10             (_PRS_ROUTELOC2_CH9LOC_LOC10 << 8)     /**< Shifted mode LOC10 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC11             (_PRS_ROUTELOC2_CH9LOC_LOC11 << 8)     /**< Shifted mode LOC11 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC12             (_PRS_ROUTELOC2_CH9LOC_LOC12 << 8)     /**< Shifted mode LOC12 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC13             (_PRS_ROUTELOC2_CH9LOC_LOC13 << 8)     /**< Shifted mode LOC13 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC14             (_PRS_ROUTELOC2_CH9LOC_LOC14 << 8)     /**< Shifted mode LOC14 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC15             (_PRS_ROUTELOC2_CH9LOC_LOC15 << 8)     /**< Shifted mode LOC15 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH9LOC_LOC16             (_PRS_ROUTELOC2_CH9LOC_LOC16 << 8)     /**< Shifted mode LOC16 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH10LOC_SHIFT           16                                     /**< Shift value for PRS_CH10LOC */
#define _PRS_ROUTELOC2_CH10LOC_MASK            0x70000UL                              /**< Bit mask for PRS_CH10LOC */
#define _PRS_ROUTELOC2_CH10LOC_LOC0            0x00000000UL                           /**< Mode LOC0 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH10LOC_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH10LOC_LOC1            0x00000001UL                           /**< Mode LOC1 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH10LOC_LOC2            0x00000002UL                           /**< Mode LOC2 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH10LOC_LOC3            0x00000003UL                           /**< Mode LOC3 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH10LOC_LOC4            0x00000004UL                           /**< Mode LOC4 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH10LOC_LOC5            0x00000005UL                           /**< Mode LOC5 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH10LOC_LOC0             (_PRS_ROUTELOC2_CH10LOC_LOC0 << 16)    /**< Shifted mode LOC0 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH10LOC_DEFAULT          (_PRS_ROUTELOC2_CH10LOC_DEFAULT << 16) /**< Shifted mode DEFAULT for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH10LOC_LOC1             (_PRS_ROUTELOC2_CH10LOC_LOC1 << 16)    /**< Shifted mode LOC1 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH10LOC_LOC2             (_PRS_ROUTELOC2_CH10LOC_LOC2 << 16)    /**< Shifted mode LOC2 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH10LOC_LOC3             (_PRS_ROUTELOC2_CH10LOC_LOC3 << 16)    /**< Shifted mode LOC3 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH10LOC_LOC4             (_PRS_ROUTELOC2_CH10LOC_LOC4 << 16)    /**< Shifted mode LOC4 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH10LOC_LOC5             (_PRS_ROUTELOC2_CH10LOC_LOC5 << 16)    /**< Shifted mode LOC5 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH11LOC_SHIFT           24                                     /**< Shift value for PRS_CH11LOC */
#define _PRS_ROUTELOC2_CH11LOC_MASK            0x7000000UL                            /**< Bit mask for PRS_CH11LOC */
#define _PRS_ROUTELOC2_CH11LOC_LOC0            0x00000000UL                           /**< Mode LOC0 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH11LOC_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH11LOC_LOC1            0x00000001UL                           /**< Mode LOC1 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH11LOC_LOC2            0x00000002UL                           /**< Mode LOC2 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH11LOC_LOC3            0x00000003UL                           /**< Mode LOC3 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH11LOC_LOC4            0x00000004UL                           /**< Mode LOC4 for PRS_ROUTELOC2 */
#define _PRS_ROUTELOC2_CH11LOC_LOC5            0x00000005UL                           /**< Mode LOC5 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH11LOC_LOC0             (_PRS_ROUTELOC2_CH11LOC_LOC0 << 24)    /**< Shifted mode LOC0 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH11LOC_DEFAULT          (_PRS_ROUTELOC2_CH11LOC_DEFAULT << 24) /**< Shifted mode DEFAULT for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH11LOC_LOC1             (_PRS_ROUTELOC2_CH11LOC_LOC1 << 24)    /**< Shifted mode LOC1 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH11LOC_LOC2             (_PRS_ROUTELOC2_CH11LOC_LOC2 << 24)    /**< Shifted mode LOC2 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH11LOC_LOC3             (_PRS_ROUTELOC2_CH11LOC_LOC3 << 24)    /**< Shifted mode LOC3 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH11LOC_LOC4             (_PRS_ROUTELOC2_CH11LOC_LOC4 << 24)    /**< Shifted mode LOC4 for PRS_ROUTELOC2 */
#define PRS_ROUTELOC2_CH11LOC_LOC5             (_PRS_ROUTELOC2_CH11LOC_LOC5 << 24)    /**< Shifted mode LOC5 for PRS_ROUTELOC2 */

/* Bit fields for PRS CTRL */
#define _PRS_CTRL_RESETVALUE                   0x00000000UL                         /**< Default value for PRS_CTRL */
#define _PRS_CTRL_MASK                         0x0000001FUL                         /**< Mask for PRS_CTRL */
#define PRS_CTRL_SEVONPRS                      (0x1UL << 0)                         /**< Set Event on PRS */
#define _PRS_CTRL_SEVONPRS_SHIFT               0                                    /**< Shift value for PRS_SEVONPRS */
#define _PRS_CTRL_SEVONPRS_MASK                0x1UL                                /**< Bit mask for PRS_SEVONPRS */
#define _PRS_CTRL_SEVONPRS_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for PRS_CTRL */
#define PRS_CTRL_SEVONPRS_DEFAULT              (_PRS_CTRL_SEVONPRS_DEFAULT << 0)    /**< Shifted mode DEFAULT for PRS_CTRL */
#define _PRS_CTRL_SEVONPRSSEL_SHIFT            1                                    /**< Shift value for PRS_SEVONPRSSEL */
#define _PRS_CTRL_SEVONPRSSEL_MASK             0x1EUL                               /**< Bit mask for PRS_SEVONPRSSEL */
#define _PRS_CTRL_SEVONPRSSEL_DEFAULT          0x00000000UL                         /**< Mode DEFAULT for PRS_CTRL */
#define _PRS_CTRL_SEVONPRSSEL_PRSCH0           0x00000000UL                         /**< Mode PRSCH0 for PRS_CTRL */
#define _PRS_CTRL_SEVONPRSSEL_PRSCH1           0x00000001UL                         /**< Mode PRSCH1 for PRS_CTRL */
#define _PRS_CTRL_SEVONPRSSEL_PRSCH2           0x00000002UL                         /**< Mode PRSCH2 for PRS_CTRL */
#define _PRS_CTRL_SEVONPRSSEL_PRSCH3           0x00000003UL                         /**< Mode PRSCH3 for PRS_CTRL */
#define _PRS_CTRL_SEVONPRSSEL_PRSCH4           0x00000004UL                         /**< Mode PRSCH4 for PRS_CTRL */
#define _PRS_CTRL_SEVONPRSSEL_PRSCH5           0x00000005UL                         /**< Mode PRSCH5 for PRS_CTRL */
#define _PRS_CTRL_SEVONPRSSEL_PRSCH6           0x00000006UL                         /**< Mode PRSCH6 for PRS_CTRL */
#define _PRS_CTRL_SEVONPRSSEL_PRSCH7           0x00000007UL                         /**< Mode PRSCH7 for PRS_CTRL */
#define _PRS_CTRL_SEVONPRSSEL_PRSCH8           0x00000008UL                         /**< Mode PRSCH8 for PRS_CTRL */
#define _PRS_CTRL_SEVONPRSSEL_PRSCH9           0x00000009UL                         /**< Mode PRSCH9 for PRS_CTRL */
#define _PRS_CTRL_SEVONPRSSEL_PRSCH10          0x0000000AUL                         /**< Mode PRSCH10 for PRS_CTRL */
#define _PRS_CTRL_SEVONPRSSEL_PRSCH11          0x0000000BUL                         /**< Mode PRSCH11 for PRS_CTRL */
#define PRS_CTRL_SEVONPRSSEL_DEFAULT           (_PRS_CTRL_SEVONPRSSEL_DEFAULT << 1) /**< Shifted mode DEFAULT for PRS_CTRL */
#define PRS_CTRL_SEVONPRSSEL_PRSCH0            (_PRS_CTRL_SEVONPRSSEL_PRSCH0 << 1)  /**< Shifted mode PRSCH0 for PRS_CTRL */
#define PRS_CTRL_SEVONPRSSEL_PRSCH1            (_PRS_CTRL_SEVONPRSSEL_PRSCH1 << 1)  /**< Shifted mode PRSCH1 for PRS_CTRL */
#define PRS_CTRL_SEVONPRSSEL_PRSCH2            (_PRS_CTRL_SEVONPRSSEL_PRSCH2 << 1)  /**< Shifted mode PRSCH2 for PRS_CTRL */
#define PRS_CTRL_SEVONPRSSEL_PRSCH3            (_PRS_CTRL_SEVONPRSSEL_PRSCH3 << 1)  /**< Shifted mode PRSCH3 for PRS_CTRL */
#define PRS_CTRL_SEVONPRSSEL_PRSCH4            (_PRS_CTRL_SEVONPRSSEL_PRSCH4 << 1)  /**< Shifted mode PRSCH4 for PRS_CTRL */
#define PRS_CTRL_SEVONPRSSEL_PRSCH5            (_PRS_CTRL_SEVONPRSSEL_PRSCH5 << 1)  /**< Shifted mode PRSCH5 for PRS_CTRL */
#define PRS_CTRL_SEVONPRSSEL_PRSCH6            (_PRS_CTRL_SEVONPRSSEL_PRSCH6 << 1)  /**< Shifted mode PRSCH6 for PRS_CTRL */
#define PRS_CTRL_SEVONPRSSEL_PRSCH7            (_PRS_CTRL_SEVONPRSSEL_PRSCH7 << 1)  /**< Shifted mode PRSCH7 for PRS_CTRL */
#define PRS_CTRL_SEVONPRSSEL_PRSCH8            (_PRS_CTRL_SEVONPRSSEL_PRSCH8 << 1)  /**< Shifted mode PRSCH8 for PRS_CTRL */
#define PRS_CTRL_SEVONPRSSEL_PRSCH9            (_PRS_CTRL_SEVONPRSSEL_PRSCH9 << 1)  /**< Shifted mode PRSCH9 for PRS_CTRL */
#define PRS_CTRL_SEVONPRSSEL_PRSCH10           (_PRS_CTRL_SEVONPRSSEL_PRSCH10 << 1) /**< Shifted mode PRSCH10 for PRS_CTRL */
#define PRS_CTRL_SEVONPRSSEL_PRSCH11           (_PRS_CTRL_SEVONPRSSEL_PRSCH11 << 1) /**< Shifted mode PRSCH11 for PRS_CTRL */

/* Bit fields for PRS DMAREQ0 */
#define _PRS_DMAREQ0_RESETVALUE                0x00000000UL                       /**< Default value for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_MASK                      0x000003C0UL                       /**< Mask for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_PRSSEL_SHIFT              6                                  /**< Shift value for PRS_PRSSEL */
#define _PRS_DMAREQ0_PRSSEL_MASK               0x3C0UL                            /**< Bit mask for PRS_PRSSEL */
#define _PRS_DMAREQ0_PRSSEL_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_PRSSEL_PRSCH0             0x00000000UL                       /**< Mode PRSCH0 for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_PRSSEL_PRSCH1             0x00000001UL                       /**< Mode PRSCH1 for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_PRSSEL_PRSCH2             0x00000002UL                       /**< Mode PRSCH2 for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_PRSSEL_PRSCH3             0x00000003UL                       /**< Mode PRSCH3 for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_PRSSEL_PRSCH4             0x00000004UL                       /**< Mode PRSCH4 for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_PRSSEL_PRSCH5             0x00000005UL                       /**< Mode PRSCH5 for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_PRSSEL_PRSCH6             0x00000006UL                       /**< Mode PRSCH6 for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_PRSSEL_PRSCH7             0x00000007UL                       /**< Mode PRSCH7 for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_PRSSEL_PRSCH8             0x00000008UL                       /**< Mode PRSCH8 for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_PRSSEL_PRSCH9             0x00000009UL                       /**< Mode PRSCH9 for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_PRSSEL_PRSCH10            0x0000000AUL                       /**< Mode PRSCH10 for PRS_DMAREQ0 */
#define _PRS_DMAREQ0_PRSSEL_PRSCH11            0x0000000BUL                       /**< Mode PRSCH11 for PRS_DMAREQ0 */
#define PRS_DMAREQ0_PRSSEL_DEFAULT             (_PRS_DMAREQ0_PRSSEL_DEFAULT << 6) /**< Shifted mode DEFAULT for PRS_DMAREQ0 */
#define PRS_DMAREQ0_PRSSEL_PRSCH0              (_PRS_DMAREQ0_PRSSEL_PRSCH0 << 6)  /**< Shifted mode PRSCH0 for PRS_DMAREQ0 */
#define PRS_DMAREQ0_PRSSEL_PRSCH1              (_PRS_DMAREQ0_PRSSEL_PRSCH1 << 6)  /**< Shifted mode PRSCH1 for PRS_DMAREQ0 */
#define PRS_DMAREQ0_PRSSEL_PRSCH2              (_PRS_DMAREQ0_PRSSEL_PRSCH2 << 6)  /**< Shifted mode PRSCH2 for PRS_DMAREQ0 */
#define PRS_DMAREQ0_PRSSEL_PRSCH3              (_PRS_DMAREQ0_PRSSEL_PRSCH3 << 6)  /**< Shifted mode PRSCH3 for PRS_DMAREQ0 */
#define PRS_DMAREQ0_PRSSEL_PRSCH4              (_PRS_DMAREQ0_PRSSEL_PRSCH4 << 6)  /**< Shifted mode PRSCH4 for PRS_DMAREQ0 */
#define PRS_DMAREQ0_PRSSEL_PRSCH5              (_PRS_DMAREQ0_PRSSEL_PRSCH5 << 6)  /**< Shifted mode PRSCH5 for PRS_DMAREQ0 */
#define PRS_DMAREQ0_PRSSEL_PRSCH6              (_PRS_DMAREQ0_PRSSEL_PRSCH6 << 6)  /**< Shifted mode PRSCH6 for PRS_DMAREQ0 */
#define PRS_DMAREQ0_PRSSEL_PRSCH7              (_PRS_DMAREQ0_PRSSEL_PRSCH7 << 6)  /**< Shifted mode PRSCH7 for PRS_DMAREQ0 */
#define PRS_DMAREQ0_PRSSEL_PRSCH8              (_PRS_DMAREQ0_PRSSEL_PRSCH8 << 6)  /**< Shifted mode PRSCH8 for PRS_DMAREQ0 */
#define PRS_DMAREQ0_PRSSEL_PRSCH9              (_PRS_DMAREQ0_PRSSEL_PRSCH9 << 6)  /**< Shifted mode PRSCH9 for PRS_DMAREQ0 */
#define PRS_DMAREQ0_PRSSEL_PRSCH10             (_PRS_DMAREQ0_PRSSEL_PRSCH10 << 6) /**< Shifted mode PRSCH10 for PRS_DMAREQ0 */
#define PRS_DMAREQ0_PRSSEL_PRSCH11             (_PRS_DMAREQ0_PRSSEL_PRSCH11 << 6) /**< Shifted mode PRSCH11 for PRS_DMAREQ0 */

/* Bit fields for PRS DMAREQ1 */
#define _PRS_DMAREQ1_RESETVALUE                0x00000000UL                       /**< Default value for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_MASK                      0x000003C0UL                       /**< Mask for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_PRSSEL_SHIFT              6                                  /**< Shift value for PRS_PRSSEL */
#define _PRS_DMAREQ1_PRSSEL_MASK               0x3C0UL                            /**< Bit mask for PRS_PRSSEL */
#define _PRS_DMAREQ1_PRSSEL_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_PRSSEL_PRSCH0             0x00000000UL                       /**< Mode PRSCH0 for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_PRSSEL_PRSCH1             0x00000001UL                       /**< Mode PRSCH1 for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_PRSSEL_PRSCH2             0x00000002UL                       /**< Mode PRSCH2 for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_PRSSEL_PRSCH3             0x00000003UL                       /**< Mode PRSCH3 for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_PRSSEL_PRSCH4             0x00000004UL                       /**< Mode PRSCH4 for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_PRSSEL_PRSCH5             0x00000005UL                       /**< Mode PRSCH5 for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_PRSSEL_PRSCH6             0x00000006UL                       /**< Mode PRSCH6 for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_PRSSEL_PRSCH7             0x00000007UL                       /**< Mode PRSCH7 for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_PRSSEL_PRSCH8             0x00000008UL                       /**< Mode PRSCH8 for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_PRSSEL_PRSCH9             0x00000009UL                       /**< Mode PRSCH9 for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_PRSSEL_PRSCH10            0x0000000AUL                       /**< Mode PRSCH10 for PRS_DMAREQ1 */
#define _PRS_DMAREQ1_PRSSEL_PRSCH11            0x0000000BUL                       /**< Mode PRSCH11 for PRS_DMAREQ1 */
#define PRS_DMAREQ1_PRSSEL_DEFAULT             (_PRS_DMAREQ1_PRSSEL_DEFAULT << 6) /**< Shifted mode DEFAULT for PRS_DMAREQ1 */
#define PRS_DMAREQ1_PRSSEL_PRSCH0              (_PRS_DMAREQ1_PRSSEL_PRSCH0 << 6)  /**< Shifted mode PRSCH0 for PRS_DMAREQ1 */
#define PRS_DMAREQ1_PRSSEL_PRSCH1              (_PRS_DMAREQ1_PRSSEL_PRSCH1 << 6)  /**< Shifted mode PRSCH1 for PRS_DMAREQ1 */
#define PRS_DMAREQ1_PRSSEL_PRSCH2              (_PRS_DMAREQ1_PRSSEL_PRSCH2 << 6)  /**< Shifted mode PRSCH2 for PRS_DMAREQ1 */
#define PRS_DMAREQ1_PRSSEL_PRSCH3              (_PRS_DMAREQ1_PRSSEL_PRSCH3 << 6)  /**< Shifted mode PRSCH3 for PRS_DMAREQ1 */
#define PRS_DMAREQ1_PRSSEL_PRSCH4              (_PRS_DMAREQ1_PRSSEL_PRSCH4 << 6)  /**< Shifted mode PRSCH4 for PRS_DMAREQ1 */
#define PRS_DMAREQ1_PRSSEL_PRSCH5              (_PRS_DMAREQ1_PRSSEL_PRSCH5 << 6)  /**< Shifted mode PRSCH5 for PRS_DMAREQ1 */
#define PRS_DMAREQ1_PRSSEL_PRSCH6              (_PRS_DMAREQ1_PRSSEL_PRSCH6 << 6)  /**< Shifted mode PRSCH6 for PRS_DMAREQ1 */
#define PRS_DMAREQ1_PRSSEL_PRSCH7              (_PRS_DMAREQ1_PRSSEL_PRSCH7 << 6)  /**< Shifted mode PRSCH7 for PRS_DMAREQ1 */
#define PRS_DMAREQ1_PRSSEL_PRSCH8              (_PRS_DMAREQ1_PRSSEL_PRSCH8 << 6)  /**< Shifted mode PRSCH8 for PRS_DMAREQ1 */
#define PRS_DMAREQ1_PRSSEL_PRSCH9              (_PRS_DMAREQ1_PRSSEL_PRSCH9 << 6)  /**< Shifted mode PRSCH9 for PRS_DMAREQ1 */
#define PRS_DMAREQ1_PRSSEL_PRSCH10             (_PRS_DMAREQ1_PRSSEL_PRSCH10 << 6) /**< Shifted mode PRSCH10 for PRS_DMAREQ1 */
#define PRS_DMAREQ1_PRSSEL_PRSCH11             (_PRS_DMAREQ1_PRSSEL_PRSCH11 << 6) /**< Shifted mode PRSCH11 for PRS_DMAREQ1 */

/* Bit fields for PRS PEEK */
#define _PRS_PEEK_RESETVALUE                   0x00000000UL                      /**< Default value for PRS_PEEK */
#define _PRS_PEEK_MASK                         0x00000FFFUL                      /**< Mask for PRS_PEEK */
#define PRS_PEEK_CH0VAL                        (0x1UL << 0)                      /**< Channel 0 Current Value */
#define _PRS_PEEK_CH0VAL_SHIFT                 0                                 /**< Shift value for PRS_CH0VAL */
#define _PRS_PEEK_CH0VAL_MASK                  0x1UL                             /**< Bit mask for PRS_CH0VAL */
#define _PRS_PEEK_CH0VAL_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH0VAL_DEFAULT                (_PRS_PEEK_CH0VAL_DEFAULT << 0)   /**< Shifted mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH1VAL                        (0x1UL << 1)                      /**< Channel 1 Current Value */
#define _PRS_PEEK_CH1VAL_SHIFT                 1                                 /**< Shift value for PRS_CH1VAL */
#define _PRS_PEEK_CH1VAL_MASK                  0x2UL                             /**< Bit mask for PRS_CH1VAL */
#define _PRS_PEEK_CH1VAL_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH1VAL_DEFAULT                (_PRS_PEEK_CH1VAL_DEFAULT << 1)   /**< Shifted mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH2VAL                        (0x1UL << 2)                      /**< Channel 2 Current Value */
#define _PRS_PEEK_CH2VAL_SHIFT                 2                                 /**< Shift value for PRS_CH2VAL */
#define _PRS_PEEK_CH2VAL_MASK                  0x4UL                             /**< Bit mask for PRS_CH2VAL */
#define _PRS_PEEK_CH2VAL_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH2VAL_DEFAULT                (_PRS_PEEK_CH2VAL_DEFAULT << 2)   /**< Shifted mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH3VAL                        (0x1UL << 3)                      /**< Channel 3 Current Value */
#define _PRS_PEEK_CH3VAL_SHIFT                 3                                 /**< Shift value for PRS_CH3VAL */
#define _PRS_PEEK_CH3VAL_MASK                  0x8UL                             /**< Bit mask for PRS_CH3VAL */
#define _PRS_PEEK_CH3VAL_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH3VAL_DEFAULT                (_PRS_PEEK_CH3VAL_DEFAULT << 3)   /**< Shifted mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH4VAL                        (0x1UL << 4)                      /**< Channel 4 Current Value */
#define _PRS_PEEK_CH4VAL_SHIFT                 4                                 /**< Shift value for PRS_CH4VAL */
#define _PRS_PEEK_CH4VAL_MASK                  0x10UL                            /**< Bit mask for PRS_CH4VAL */
#define _PRS_PEEK_CH4VAL_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH4VAL_DEFAULT                (_PRS_PEEK_CH4VAL_DEFAULT << 4)   /**< Shifted mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH5VAL                        (0x1UL << 5)                      /**< Channel 5 Current Value */
#define _PRS_PEEK_CH5VAL_SHIFT                 5                                 /**< Shift value for PRS_CH5VAL */
#define _PRS_PEEK_CH5VAL_MASK                  0x20UL                            /**< Bit mask for PRS_CH5VAL */
#define _PRS_PEEK_CH5VAL_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH5VAL_DEFAULT                (_PRS_PEEK_CH5VAL_DEFAULT << 5)   /**< Shifted mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH6VAL                        (0x1UL << 6)                      /**< Channel 6 Current Value */
#define _PRS_PEEK_CH6VAL_SHIFT                 6                                 /**< Shift value for PRS_CH6VAL */
#define _PRS_PEEK_CH6VAL_MASK                  0x40UL                            /**< Bit mask for PRS_CH6VAL */
#define _PRS_PEEK_CH6VAL_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH6VAL_DEFAULT                (_PRS_PEEK_CH6VAL_DEFAULT << 6)   /**< Shifted mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH7VAL                        (0x1UL << 7)                      /**< Channel 7 Current Value */
#define _PRS_PEEK_CH7VAL_SHIFT                 7                                 /**< Shift value for PRS_CH7VAL */
#define _PRS_PEEK_CH7VAL_MASK                  0x80UL                            /**< Bit mask for PRS_CH7VAL */
#define _PRS_PEEK_CH7VAL_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH7VAL_DEFAULT                (_PRS_PEEK_CH7VAL_DEFAULT << 7)   /**< Shifted mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH8VAL                        (0x1UL << 8)                      /**< Channel 8 Current Value */
#define _PRS_PEEK_CH8VAL_SHIFT                 8                                 /**< Shift value for PRS_CH8VAL */
#define _PRS_PEEK_CH8VAL_MASK                  0x100UL                           /**< Bit mask for PRS_CH8VAL */
#define _PRS_PEEK_CH8VAL_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH8VAL_DEFAULT                (_PRS_PEEK_CH8VAL_DEFAULT << 8)   /**< Shifted mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH9VAL                        (0x1UL << 9)                      /**< Channel 9 Current Value */
#define _PRS_PEEK_CH9VAL_SHIFT                 9                                 /**< Shift value for PRS_CH9VAL */
#define _PRS_PEEK_CH9VAL_MASK                  0x200UL                           /**< Bit mask for PRS_CH9VAL */
#define _PRS_PEEK_CH9VAL_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH9VAL_DEFAULT                (_PRS_PEEK_CH9VAL_DEFAULT << 9)   /**< Shifted mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH10VAL                       (0x1UL << 10)                     /**< Channel 10 Current Value */
#define _PRS_PEEK_CH10VAL_SHIFT                10                                /**< Shift value for PRS_CH10VAL */
#define _PRS_PEEK_CH10VAL_MASK                 0x400UL                           /**< Bit mask for PRS_CH10VAL */
#define _PRS_PEEK_CH10VAL_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH10VAL_DEFAULT               (_PRS_PEEK_CH10VAL_DEFAULT << 10) /**< Shifted mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH11VAL                       (0x1UL << 11)                     /**< Channel 11 Current Value */
#define _PRS_PEEK_CH11VAL_SHIFT                11                                /**< Shift value for PRS_CH11VAL */
#define _PRS_PEEK_CH11VAL_MASK                 0x800UL                           /**< Bit mask for PRS_CH11VAL */
#define _PRS_PEEK_CH11VAL_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for PRS_PEEK */
#define PRS_PEEK_CH11VAL_DEFAULT               (_PRS_PEEK_CH11VAL_DEFAULT << 11) /**< Shifted mode DEFAULT for PRS_PEEK */

/* Bit fields for PRS CH_CTRL */
#define _PRS_CH_CTRL_RESETVALUE                0x00000000UL                               /**< Default value for PRS_CH_CTRL */
#define _PRS_CH_CTRL_MASK                      0x5E307F07UL                               /**< Mask for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_SHIFT              0                                          /**< Shift value for PRS_SIGSEL */
#define _PRS_CH_CTRL_SIGSEL_MASK               0x7UL                                      /**< Bit mask for PRS_SIGSEL */
#define _PRS_CH_CTRL_SIGSEL_PRSCH0             0x00000000UL                               /**< Mode PRSCH0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PRSCH8             0x00000000UL                               /**< Mode PRSCH8 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_ACMP0OUT           0x00000000UL                               /**< Mode ACMP0OUT for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_ACMP1OUT           0x00000000UL                               /**< Mode ACMP1OUT for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_ADC0SINGLE         0x00000000UL                               /**< Mode ADC0SINGLE for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART0IRTX         0x00000000UL                               /**< Mode USART0IRTX for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0UF           0x00000000UL                               /**< Mode TIMER0UF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1UF           0x00000000UL                               /**< Mode TIMER1UF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN0           0x00000000UL                               /**< Mode GPIOPIN0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN8           0x00000000UL                               /**< Mode GPIOPIN8 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LETIMER0CH0        0x00000000UL                               /**< Mode LETIMER0CH0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PCNT0TCC           0x00000000UL                               /**< Mode PCNT0TCC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_CRYOTIMERPERIOD    0x00000000UL                               /**< Mode CRYOTIMERPERIOD for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_CMUCLKOUT0         0x00000000UL                               /**< Mode CMUCLKOUT0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_CM4TXEV            0x00000000UL                               /**< Mode CM4TXEV for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PRSCH1             0x00000001UL                               /**< Mode PRSCH1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PRSCH9             0x00000001UL                               /**< Mode PRSCH9 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_ADC0SCAN           0x00000001UL                               /**< Mode ADC0SCAN for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART0TXC          0x00000001UL                               /**< Mode USART0TXC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART1TXC          0x00000001UL                               /**< Mode USART1TXC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0OF           0x00000001UL                               /**< Mode TIMER0OF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1OF           0x00000001UL                               /**< Mode TIMER1OF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_RTCCCCV0           0x00000001UL                               /**< Mode RTCCCCV0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN1           0x00000001UL                               /**< Mode GPIOPIN1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN9           0x00000001UL                               /**< Mode GPIOPIN9 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_LETIMER0CH1        0x00000001UL                               /**< Mode LETIMER0CH1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PCNT0UFOF          0x00000001UL                               /**< Mode PCNT0UFOF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_CMUCLKOUT1         0x00000001UL                               /**< Mode CMUCLKOUT1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PRSCH2             0x00000002UL                               /**< Mode PRSCH2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PRSCH10            0x00000002UL                               /**< Mode PRSCH10 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART0RXDATAV      0x00000002UL                               /**< Mode USART0RXDATAV for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART1RXDATAV      0x00000002UL                               /**< Mode USART1RXDATAV for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0CC0          0x00000002UL                               /**< Mode TIMER0CC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1CC0          0x00000002UL                               /**< Mode TIMER1CC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_RTCCCCV1           0x00000002UL                               /**< Mode RTCCCCV1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN2           0x00000002UL                               /**< Mode GPIOPIN2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN10          0x00000002UL                               /**< Mode GPIOPIN10 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PCNT0DIR           0x00000002UL                               /**< Mode PCNT0DIR for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PRSCH3             0x00000003UL                               /**< Mode PRSCH3 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PRSCH11            0x00000003UL                               /**< Mode PRSCH11 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART0RTS          0x00000003UL                               /**< Mode USART0RTS for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART1RTS          0x00000003UL                               /**< Mode USART1RTS for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0CC1          0x00000003UL                               /**< Mode TIMER0CC1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1CC1          0x00000003UL                               /**< Mode TIMER1CC1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_RTCCCCV2           0x00000003UL                               /**< Mode RTCCCCV2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN3           0x00000003UL                               /**< Mode GPIOPIN3 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN11          0x00000003UL                               /**< Mode GPIOPIN11 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PRSCH4             0x00000004UL                               /**< Mode PRSCH4 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0CC2          0x00000004UL                               /**< Mode TIMER0CC2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1CC2          0x00000004UL                               /**< Mode TIMER1CC2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN4           0x00000004UL                               /**< Mode GPIOPIN4 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN12          0x00000004UL                               /**< Mode GPIOPIN12 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PRSCH5             0x00000005UL                               /**< Mode PRSCH5 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART0TX           0x00000005UL                               /**< Mode USART0TX for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART1TX           0x00000005UL                               /**< Mode USART1TX for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1CC3          0x00000005UL                               /**< Mode TIMER1CC3 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN5           0x00000005UL                               /**< Mode GPIOPIN5 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN13          0x00000005UL                               /**< Mode GPIOPIN13 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PRSCH6             0x00000006UL                               /**< Mode PRSCH6 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART0CS           0x00000006UL                               /**< Mode USART0CS for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART1CS           0x00000006UL                               /**< Mode USART1CS for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN6           0x00000006UL                               /**< Mode GPIOPIN6 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN14          0x00000006UL                               /**< Mode GPIOPIN14 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PRSCH7             0x00000007UL                               /**< Mode PRSCH7 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN7           0x00000007UL                               /**< Mode GPIOPIN7 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN15          0x00000007UL                               /**< Mode GPIOPIN15 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PRSCH0              (_PRS_CH_CTRL_SIGSEL_PRSCH0 << 0)          /**< Shifted mode PRSCH0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PRSCH8              (_PRS_CH_CTRL_SIGSEL_PRSCH8 << 0)          /**< Shifted mode PRSCH8 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_ACMP0OUT            (_PRS_CH_CTRL_SIGSEL_ACMP0OUT << 0)        /**< Shifted mode ACMP0OUT for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_ACMP1OUT            (_PRS_CH_CTRL_SIGSEL_ACMP1OUT << 0)        /**< Shifted mode ACMP1OUT for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_ADC0SINGLE          (_PRS_CH_CTRL_SIGSEL_ADC0SINGLE << 0)      /**< Shifted mode ADC0SINGLE for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART0IRTX          (_PRS_CH_CTRL_SIGSEL_USART0IRTX << 0)      /**< Shifted mode USART0IRTX for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0UF            (_PRS_CH_CTRL_SIGSEL_TIMER0UF << 0)        /**< Shifted mode TIMER0UF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1UF            (_PRS_CH_CTRL_SIGSEL_TIMER1UF << 0)        /**< Shifted mode TIMER1UF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN0            (_PRS_CH_CTRL_SIGSEL_GPIOPIN0 << 0)        /**< Shifted mode GPIOPIN0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN8            (_PRS_CH_CTRL_SIGSEL_GPIOPIN8 << 0)        /**< Shifted mode GPIOPIN8 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LETIMER0CH0         (_PRS_CH_CTRL_SIGSEL_LETIMER0CH0 << 0)     /**< Shifted mode LETIMER0CH0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PCNT0TCC            (_PRS_CH_CTRL_SIGSEL_PCNT0TCC << 0)        /**< Shifted mode PCNT0TCC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_CRYOTIMERPERIOD     (_PRS_CH_CTRL_SIGSEL_CRYOTIMERPERIOD << 0) /**< Shifted mode CRYOTIMERPERIOD for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_CMUCLKOUT0          (_PRS_CH_CTRL_SIGSEL_CMUCLKOUT0 << 0)      /**< Shifted mode CMUCLKOUT0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_CM4TXEV             (_PRS_CH_CTRL_SIGSEL_CM4TXEV << 0)         /**< Shifted mode CM4TXEV for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PRSCH1              (_PRS_CH_CTRL_SIGSEL_PRSCH1 << 0)          /**< Shifted mode PRSCH1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PRSCH9              (_PRS_CH_CTRL_SIGSEL_PRSCH9 << 0)          /**< Shifted mode PRSCH9 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_ADC0SCAN            (_PRS_CH_CTRL_SIGSEL_ADC0SCAN << 0)        /**< Shifted mode ADC0SCAN for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART0TXC           (_PRS_CH_CTRL_SIGSEL_USART0TXC << 0)       /**< Shifted mode USART0TXC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART1TXC           (_PRS_CH_CTRL_SIGSEL_USART1TXC << 0)       /**< Shifted mode USART1TXC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0OF            (_PRS_CH_CTRL_SIGSEL_TIMER0OF << 0)        /**< Shifted mode TIMER0OF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1OF            (_PRS_CH_CTRL_SIGSEL_TIMER1OF << 0)        /**< Shifted mode TIMER1OF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_RTCCCCV0            (_PRS_CH_CTRL_SIGSEL_RTCCCCV0 << 0)        /**< Shifted mode RTCCCCV0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN1            (_PRS_CH_CTRL_SIGSEL_GPIOPIN1 << 0)        /**< Shifted mode GPIOPIN1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN9            (_PRS_CH_CTRL_SIGSEL_GPIOPIN9 << 0)        /**< Shifted mode GPIOPIN9 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_LETIMER0CH1         (_PRS_CH_CTRL_SIGSEL_LETIMER0CH1 << 0)     /**< Shifted mode LETIMER0CH1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PCNT0UFOF           (_PRS_CH_CTRL_SIGSEL_PCNT0UFOF << 0)       /**< Shifted mode PCNT0UFOF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_CMUCLKOUT1          (_PRS_CH_CTRL_SIGSEL_CMUCLKOUT1 << 0)      /**< Shifted mode CMUCLKOUT1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PRSCH2              (_PRS_CH_CTRL_SIGSEL_PRSCH2 << 0)          /**< Shifted mode PRSCH2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PRSCH10             (_PRS_CH_CTRL_SIGSEL_PRSCH10 << 0)         /**< Shifted mode PRSCH10 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART0RXDATAV       (_PRS_CH_CTRL_SIGSEL_USART0RXDATAV << 0)   /**< Shifted mode USART0RXDATAV for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART1RXDATAV       (_PRS_CH_CTRL_SIGSEL_USART1RXDATAV << 0)   /**< Shifted mode USART1RXDATAV for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0CC0           (_PRS_CH_CTRL_SIGSEL_TIMER0CC0 << 0)       /**< Shifted mode TIMER0CC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1CC0           (_PRS_CH_CTRL_SIGSEL_TIMER1CC0 << 0)       /**< Shifted mode TIMER1CC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_RTCCCCV1            (_PRS_CH_CTRL_SIGSEL_RTCCCCV1 << 0)        /**< Shifted mode RTCCCCV1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN2            (_PRS_CH_CTRL_SIGSEL_GPIOPIN2 << 0)        /**< Shifted mode GPIOPIN2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN10           (_PRS_CH_CTRL_SIGSEL_GPIOPIN10 << 0)       /**< Shifted mode GPIOPIN10 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PCNT0DIR            (_PRS_CH_CTRL_SIGSEL_PCNT0DIR << 0)        /**< Shifted mode PCNT0DIR for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PRSCH3              (_PRS_CH_CTRL_SIGSEL_PRSCH3 << 0)          /**< Shifted mode PRSCH3 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PRSCH11             (_PRS_CH_CTRL_SIGSEL_PRSCH11 << 0)         /**< Shifted mode PRSCH11 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART0RTS           (_PRS_CH_CTRL_SIGSEL_USART0RTS << 0)       /**< Shifted mode USART0RTS for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART1RTS           (_PRS_CH_CTRL_SIGSEL_USART1RTS << 0)       /**< Shifted mode USART1RTS for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0CC1           (_PRS_CH_CTRL_SIGSEL_TIMER0CC1 << 0)       /**< Shifted mode TIMER0CC1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1CC1           (_PRS_CH_CTRL_SIGSEL_TIMER1CC1 << 0)       /**< Shifted mode TIMER1CC1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_RTCCCCV2            (_PRS_CH_CTRL_SIGSEL_RTCCCCV2 << 0)        /**< Shifted mode RTCCCCV2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN3            (_PRS_CH_CTRL_SIGSEL_GPIOPIN3 << 0)        /**< Shifted mode GPIOPIN3 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN11           (_PRS_CH_CTRL_SIGSEL_GPIOPIN11 << 0)       /**< Shifted mode GPIOPIN11 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PRSCH4              (_PRS_CH_CTRL_SIGSEL_PRSCH4 << 0)          /**< Shifted mode PRSCH4 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0CC2           (_PRS_CH_CTRL_SIGSEL_TIMER0CC2 << 0)       /**< Shifted mode TIMER0CC2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1CC2           (_PRS_CH_CTRL_SIGSEL_TIMER1CC2 << 0)       /**< Shifted mode TIMER1CC2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN4            (_PRS_CH_CTRL_SIGSEL_GPIOPIN4 << 0)        /**< Shifted mode GPIOPIN4 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN12           (_PRS_CH_CTRL_SIGSEL_GPIOPIN12 << 0)       /**< Shifted mode GPIOPIN12 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PRSCH5              (_PRS_CH_CTRL_SIGSEL_PRSCH5 << 0)          /**< Shifted mode PRSCH5 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART0TX            (_PRS_CH_CTRL_SIGSEL_USART0TX << 0)        /**< Shifted mode USART0TX for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART1TX            (_PRS_CH_CTRL_SIGSEL_USART1TX << 0)        /**< Shifted mode USART1TX for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1CC3           (_PRS_CH_CTRL_SIGSEL_TIMER1CC3 << 0)       /**< Shifted mode TIMER1CC3 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN5            (_PRS_CH_CTRL_SIGSEL_GPIOPIN5 << 0)        /**< Shifted mode GPIOPIN5 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN13           (_PRS_CH_CTRL_SIGSEL_GPIOPIN13 << 0)       /**< Shifted mode GPIOPIN13 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PRSCH6              (_PRS_CH_CTRL_SIGSEL_PRSCH6 << 0)          /**< Shifted mode PRSCH6 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART0CS            (_PRS_CH_CTRL_SIGSEL_USART0CS << 0)        /**< Shifted mode USART0CS for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART1CS            (_PRS_CH_CTRL_SIGSEL_USART1CS << 0)        /**< Shifted mode USART1CS for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN6            (_PRS_CH_CTRL_SIGSEL_GPIOPIN6 << 0)        /**< Shifted mode GPIOPIN6 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN14           (_PRS_CH_CTRL_SIGSEL_GPIOPIN14 << 0)       /**< Shifted mode GPIOPIN14 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PRSCH7              (_PRS_CH_CTRL_SIGSEL_PRSCH7 << 0)          /**< Shifted mode PRSCH7 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN7            (_PRS_CH_CTRL_SIGSEL_GPIOPIN7 << 0)        /**< Shifted mode GPIOPIN7 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN15           (_PRS_CH_CTRL_SIGSEL_GPIOPIN15 << 0)       /**< Shifted mode GPIOPIN15 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_SHIFT           8                                          /**< Shift value for PRS_SOURCESEL */
#define _PRS_CH_CTRL_SOURCESEL_MASK            0x7F00UL                                   /**< Bit mask for PRS_SOURCESEL */
#define _PRS_CH_CTRL_SOURCESEL_NONE            0x00000000UL                               /**< Mode NONE for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_PRSL            0x00000001UL                               /**< Mode PRSL for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_PRSH            0x00000002UL                               /**< Mode PRSH for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_ACMP0           0x00000006UL                               /**< Mode ACMP0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_ACMP1           0x00000007UL                               /**< Mode ACMP1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_ADC0            0x00000008UL                               /**< Mode ADC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_USART0          0x00000010UL                               /**< Mode USART0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_USART1          0x00000011UL                               /**< Mode USART1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_TIMER0          0x0000001CUL                               /**< Mode TIMER0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_TIMER1          0x0000001DUL                               /**< Mode TIMER1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_RTCC            0x00000029UL                               /**< Mode RTCC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_GPIOL           0x00000030UL                               /**< Mode GPIOL for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_GPIOH           0x00000031UL                               /**< Mode GPIOH for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_LETIMER0        0x00000034UL                               /**< Mode LETIMER0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_PCNT0           0x00000036UL                               /**< Mode PCNT0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_CRYOTIMER       0x0000003CUL                               /**< Mode CRYOTIMER for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_CMU             0x0000003DUL                               /**< Mode CMU for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_CM4             0x00000043UL                               /**< Mode CM4 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_NONE             (_PRS_CH_CTRL_SOURCESEL_NONE << 8)         /**< Shifted mode NONE for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_PRSL             (_PRS_CH_CTRL_SOURCESEL_PRSL << 8)         /**< Shifted mode PRSL for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_PRSH             (_PRS_CH_CTRL_SOURCESEL_PRSH << 8)         /**< Shifted mode PRSH for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_ACMP0            (_PRS_CH_CTRL_SOURCESEL_ACMP0 << 8)        /**< Shifted mode ACMP0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_ACMP1            (_PRS_CH_CTRL_SOURCESEL_ACMP1 << 8)        /**< Shifted mode ACMP1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_ADC0             (_PRS_CH_CTRL_SOURCESEL_ADC0 << 8)         /**< Shifted mode ADC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_USART0           (_PRS_CH_CTRL_SOURCESEL_USART0 << 8)       /**< Shifted mode USART0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_USART1           (_PRS_CH_CTRL_SOURCESEL_USART1 << 8)       /**< Shifted mode USART1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_TIMER0           (_PRS_CH_CTRL_SOURCESEL_TIMER0 << 8)       /**< Shifted mode TIMER0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_TIMER1           (_PRS_CH_CTRL_SOURCESEL_TIMER1 << 8)       /**< Shifted mode TIMER1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_RTCC             (_PRS_CH_CTRL_SOURCESEL_RTCC << 8)         /**< Shifted mode RTCC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_GPIOL            (_PRS_CH_CTRL_SOURCESEL_GPIOL << 8)        /**< Shifted mode GPIOL for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_GPIOH            (_PRS_CH_CTRL_SOURCESEL_GPIOH << 8)        /**< Shifted mode GPIOH for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_LETIMER0         (_PRS_CH_CTRL_SOURCESEL_LETIMER0 << 8)     /**< Shifted mode LETIMER0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_PCNT0            (_PRS_CH_CTRL_SOURCESEL_PCNT0 << 8)        /**< Shifted mode PCNT0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_CRYOTIMER        (_PRS_CH_CTRL_SOURCESEL_CRYOTIMER << 8)    /**< Shifted mode CRYOTIMER for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_CMU              (_PRS_CH_CTRL_SOURCESEL_CMU << 8)          /**< Shifted mode CMU for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_CM4              (_PRS_CH_CTRL_SOURCESEL_CM4 << 8)          /**< Shifted mode CM4 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_SHIFT               20                                         /**< Shift value for PRS_EDSEL */
#define _PRS_CH_CTRL_EDSEL_MASK                0x300000UL                                 /**< Bit mask for PRS_EDSEL */
#define _PRS_CH_CTRL_EDSEL_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_OFF                 0x00000000UL                               /**< Mode OFF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_POSEDGE             0x00000001UL                               /**< Mode POSEDGE for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_NEGEDGE             0x00000002UL                               /**< Mode NEGEDGE for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_BOTHEDGES           0x00000003UL                               /**< Mode BOTHEDGES for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_DEFAULT              (_PRS_CH_CTRL_EDSEL_DEFAULT << 20)         /**< Shifted mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_OFF                  (_PRS_CH_CTRL_EDSEL_OFF << 20)             /**< Shifted mode OFF for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_POSEDGE              (_PRS_CH_CTRL_EDSEL_POSEDGE << 20)         /**< Shifted mode POSEDGE for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_NEGEDGE              (_PRS_CH_CTRL_EDSEL_NEGEDGE << 20)         /**< Shifted mode NEGEDGE for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_BOTHEDGES            (_PRS_CH_CTRL_EDSEL_BOTHEDGES << 20)       /**< Shifted mode BOTHEDGES for PRS_CH_CTRL */
#define PRS_CH_CTRL_STRETCH                    (0x1UL << 25)                              /**< Stretch Channel Output */
#define _PRS_CH_CTRL_STRETCH_SHIFT             25                                         /**< Shift value for PRS_STRETCH */
#define _PRS_CH_CTRL_STRETCH_MASK              0x2000000UL                                /**< Bit mask for PRS_STRETCH */
#define _PRS_CH_CTRL_STRETCH_DEFAULT           0x00000000UL                               /**< Mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_STRETCH_DEFAULT            (_PRS_CH_CTRL_STRETCH_DEFAULT << 25)       /**< Shifted mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_INV                        (0x1UL << 26)                              /**< Invert Channel */
#define _PRS_CH_CTRL_INV_SHIFT                 26                                         /**< Shift value for PRS_INV */
#define _PRS_CH_CTRL_INV_MASK                  0x4000000UL                                /**< Bit mask for PRS_INV */
#define _PRS_CH_CTRL_INV_DEFAULT               0x00000000UL                               /**< Mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_INV_DEFAULT                (_PRS_CH_CTRL_INV_DEFAULT << 26)           /**< Shifted mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_ORPREV                     (0x1UL << 27)                              /**< Or Previous */
#define _PRS_CH_CTRL_ORPREV_SHIFT              27                                         /**< Shift value for PRS_ORPREV */
#define _PRS_CH_CTRL_ORPREV_MASK               0x8000000UL                                /**< Bit mask for PRS_ORPREV */
#define _PRS_CH_CTRL_ORPREV_DEFAULT            0x00000000UL                               /**< Mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_ORPREV_DEFAULT             (_PRS_CH_CTRL_ORPREV_DEFAULT << 27)        /**< Shifted mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_ANDNEXT                    (0x1UL << 28)                              /**< And Next */
#define _PRS_CH_CTRL_ANDNEXT_SHIFT             28                                         /**< Shift value for PRS_ANDNEXT */
#define _PRS_CH_CTRL_ANDNEXT_MASK              0x10000000UL                               /**< Bit mask for PRS_ANDNEXT */
#define _PRS_CH_CTRL_ANDNEXT_DEFAULT           0x00000000UL                               /**< Mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_ANDNEXT_DEFAULT            (_PRS_CH_CTRL_ANDNEXT_DEFAULT << 28)       /**< Shifted mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_ASYNC                      (0x1UL << 30)                              /**< Asynchronous reflex */
#define _PRS_CH_CTRL_ASYNC_SHIFT               30                                         /**< Shift value for PRS_ASYNC */
#define _PRS_CH_CTRL_ASYNC_MASK                0x40000000UL                               /**< Bit mask for PRS_ASYNC */
#define _PRS_CH_CTRL_ASYNC_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_ASYNC_DEFAULT              (_PRS_CH_CTRL_ASYNC_DEFAULT << 30)         /**< Shifted mode DEFAULT for PRS_CH_CTRL */

/** @} End of group EFR32FG1P_PRS */
/** @} End of group Parts */

