/**************************************************************************//**
 * @file efm32wg_pcnt.h
 * @brief EFM32WG_PCNT register and bit field definitions
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
 * @defgroup EFM32WG_PCNT
 * @{
 * @brief EFM32WG_PCNT Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;         /**< Control Register  */
  __IOM uint32_t CMD;          /**< Command Register  */
  __IM uint32_t  STATUS;       /**< Status Register  */
  __IM uint32_t  CNT;          /**< Counter Value Register  */
  __IM uint32_t  TOP;          /**< Top Value Register  */
  __IOM uint32_t TOPB;         /**< Top Value Buffer Register  */
  __IM uint32_t  IF;           /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;          /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;          /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;          /**< Interrupt Enable Register  */
  __IOM uint32_t ROUTE;        /**< I/O Routing Register  */

  __IOM uint32_t FREEZE;       /**< Freeze Register  */
  __IM uint32_t  SYNCBUSY;     /**< Synchronization Busy Register  */

  uint32_t       RESERVED0[1]; /**< Reserved for future use **/
  __IOM uint32_t AUXCNT;       /**< Auxiliary Counter Value Register  */
  __IOM uint32_t INPUT;        /**< PCNT Input Register  */
} PCNT_TypeDef;                /** @} */

/**************************************************************************//**
 * @defgroup EFM32WG_PCNT_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for PCNT CTRL */
#define _PCNT_CTRL_RESETVALUE             0x00000000UL                        /**< Default value for PCNT_CTRL */
#define _PCNT_CTRL_MASK                   0x0000CF3FUL                        /**< Mask for PCNT_CTRL */
#define _PCNT_CTRL_MODE_SHIFT             0                                   /**< Shift value for PCNT_MODE */
#define _PCNT_CTRL_MODE_MASK              0x3UL                               /**< Bit mask for PCNT_MODE */
#define _PCNT_CTRL_MODE_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_MODE_DISABLE           0x00000000UL                        /**< Mode DISABLE for PCNT_CTRL */
#define _PCNT_CTRL_MODE_OVSSINGLE         0x00000001UL                        /**< Mode OVSSINGLE for PCNT_CTRL */
#define _PCNT_CTRL_MODE_EXTCLKSINGLE      0x00000002UL                        /**< Mode EXTCLKSINGLE for PCNT_CTRL */
#define _PCNT_CTRL_MODE_EXTCLKQUAD        0x00000003UL                        /**< Mode EXTCLKQUAD for PCNT_CTRL */
#define PCNT_CTRL_MODE_DEFAULT            (_PCNT_CTRL_MODE_DEFAULT << 0)      /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_MODE_DISABLE            (_PCNT_CTRL_MODE_DISABLE << 0)      /**< Shifted mode DISABLE for PCNT_CTRL */
#define PCNT_CTRL_MODE_OVSSINGLE          (_PCNT_CTRL_MODE_OVSSINGLE << 0)    /**< Shifted mode OVSSINGLE for PCNT_CTRL */
#define PCNT_CTRL_MODE_EXTCLKSINGLE       (_PCNT_CTRL_MODE_EXTCLKSINGLE << 0) /**< Shifted mode EXTCLKSINGLE for PCNT_CTRL */
#define PCNT_CTRL_MODE_EXTCLKQUAD         (_PCNT_CTRL_MODE_EXTCLKQUAD << 0)   /**< Shifted mode EXTCLKQUAD for PCNT_CTRL */
#define PCNT_CTRL_CNTDIR                  (0x1UL << 2)                        /**< Non-Quadrature Mode Counter Direction Control */
#define _PCNT_CTRL_CNTDIR_SHIFT           2                                   /**< Shift value for PCNT_CNTDIR */
#define _PCNT_CTRL_CNTDIR_MASK            0x4UL                               /**< Bit mask for PCNT_CNTDIR */
#define _PCNT_CTRL_CNTDIR_DEFAULT         0x00000000UL                        /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_CNTDIR_UP              0x00000000UL                        /**< Mode UP for PCNT_CTRL */
#define _PCNT_CTRL_CNTDIR_DOWN            0x00000001UL                        /**< Mode DOWN for PCNT_CTRL */
#define PCNT_CTRL_CNTDIR_DEFAULT          (_PCNT_CTRL_CNTDIR_DEFAULT << 2)    /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_CNTDIR_UP               (_PCNT_CTRL_CNTDIR_UP << 2)         /**< Shifted mode UP for PCNT_CTRL */
#define PCNT_CTRL_CNTDIR_DOWN             (_PCNT_CTRL_CNTDIR_DOWN << 2)       /**< Shifted mode DOWN for PCNT_CTRL */
#define PCNT_CTRL_EDGE                    (0x1UL << 3)                        /**< Edge Select */
#define _PCNT_CTRL_EDGE_SHIFT             3                                   /**< Shift value for PCNT_EDGE */
#define _PCNT_CTRL_EDGE_MASK              0x8UL                               /**< Bit mask for PCNT_EDGE */
#define _PCNT_CTRL_EDGE_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_EDGE_POS               0x00000000UL                        /**< Mode POS for PCNT_CTRL */
#define _PCNT_CTRL_EDGE_NEG               0x00000001UL                        /**< Mode NEG for PCNT_CTRL */
#define PCNT_CTRL_EDGE_DEFAULT            (_PCNT_CTRL_EDGE_DEFAULT << 3)      /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_EDGE_POS                (_PCNT_CTRL_EDGE_POS << 3)          /**< Shifted mode POS for PCNT_CTRL */
#define PCNT_CTRL_EDGE_NEG                (_PCNT_CTRL_EDGE_NEG << 3)          /**< Shifted mode NEG for PCNT_CTRL */
#define PCNT_CTRL_FILT                    (0x1UL << 4)                        /**< Enable Digital Pulse Width Filter */
#define _PCNT_CTRL_FILT_SHIFT             4                                   /**< Shift value for PCNT_FILT */
#define _PCNT_CTRL_FILT_MASK              0x10UL                              /**< Bit mask for PCNT_FILT */
#define _PCNT_CTRL_FILT_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_FILT_DEFAULT            (_PCNT_CTRL_FILT_DEFAULT << 4)      /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_RSTEN                   (0x1UL << 5)                        /**< Enable PCNT Clock Domain Reset */
#define _PCNT_CTRL_RSTEN_SHIFT            5                                   /**< Shift value for PCNT_RSTEN */
#define _PCNT_CTRL_RSTEN_MASK             0x20UL                              /**< Bit mask for PCNT_RSTEN */
#define _PCNT_CTRL_RSTEN_DEFAULT          0x00000000UL                        /**< Mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_RSTEN_DEFAULT           (_PCNT_CTRL_RSTEN_DEFAULT << 5)     /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_HYST                    (0x1UL << 8)                        /**< Enable Hysteresis */
#define _PCNT_CTRL_HYST_SHIFT             8                                   /**< Shift value for PCNT_HYST */
#define _PCNT_CTRL_HYST_MASK              0x100UL                             /**< Bit mask for PCNT_HYST */
#define _PCNT_CTRL_HYST_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_HYST_DEFAULT            (_PCNT_CTRL_HYST_DEFAULT << 8)      /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_S1CDIR                  (0x1UL << 9)                        /**< Count direction determined by S1 */
#define _PCNT_CTRL_S1CDIR_SHIFT           9                                   /**< Shift value for PCNT_S1CDIR */
#define _PCNT_CTRL_S1CDIR_MASK            0x200UL                             /**< Bit mask for PCNT_S1CDIR */
#define _PCNT_CTRL_S1CDIR_DEFAULT         0x00000000UL                        /**< Mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_S1CDIR_DEFAULT          (_PCNT_CTRL_S1CDIR_DEFAULT << 9)    /**< Shifted mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_CNTEV_SHIFT            10                                  /**< Shift value for PCNT_CNTEV */
#define _PCNT_CTRL_CNTEV_MASK             0xC00UL                             /**< Bit mask for PCNT_CNTEV */
#define _PCNT_CTRL_CNTEV_DEFAULT          0x00000000UL                        /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_CNTEV_BOTH             0x00000000UL                        /**< Mode BOTH for PCNT_CTRL */
#define _PCNT_CTRL_CNTEV_UP               0x00000001UL                        /**< Mode UP for PCNT_CTRL */
#define _PCNT_CTRL_CNTEV_DOWN             0x00000002UL                        /**< Mode DOWN for PCNT_CTRL */
#define _PCNT_CTRL_CNTEV_NONE             0x00000003UL                        /**< Mode NONE for PCNT_CTRL */
#define PCNT_CTRL_CNTEV_DEFAULT           (_PCNT_CTRL_CNTEV_DEFAULT << 10)    /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_CNTEV_BOTH              (_PCNT_CTRL_CNTEV_BOTH << 10)       /**< Shifted mode BOTH for PCNT_CTRL */
#define PCNT_CTRL_CNTEV_UP                (_PCNT_CTRL_CNTEV_UP << 10)         /**< Shifted mode UP for PCNT_CTRL */
#define PCNT_CTRL_CNTEV_DOWN              (_PCNT_CTRL_CNTEV_DOWN << 10)       /**< Shifted mode DOWN for PCNT_CTRL */
#define PCNT_CTRL_CNTEV_NONE              (_PCNT_CTRL_CNTEV_NONE << 10)       /**< Shifted mode NONE for PCNT_CTRL */
#define _PCNT_CTRL_AUXCNTEV_SHIFT         14                                  /**< Shift value for PCNT_AUXCNTEV */
#define _PCNT_CTRL_AUXCNTEV_MASK          0xC000UL                            /**< Bit mask for PCNT_AUXCNTEV */
#define _PCNT_CTRL_AUXCNTEV_DEFAULT       0x00000000UL                        /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_AUXCNTEV_NONE          0x00000000UL                        /**< Mode NONE for PCNT_CTRL */
#define _PCNT_CTRL_AUXCNTEV_UP            0x00000001UL                        /**< Mode UP for PCNT_CTRL */
#define _PCNT_CTRL_AUXCNTEV_DOWN          0x00000002UL                        /**< Mode DOWN for PCNT_CTRL */
#define _PCNT_CTRL_AUXCNTEV_BOTH          0x00000003UL                        /**< Mode BOTH for PCNT_CTRL */
#define PCNT_CTRL_AUXCNTEV_DEFAULT        (_PCNT_CTRL_AUXCNTEV_DEFAULT << 14) /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_AUXCNTEV_NONE           (_PCNT_CTRL_AUXCNTEV_NONE << 14)    /**< Shifted mode NONE for PCNT_CTRL */
#define PCNT_CTRL_AUXCNTEV_UP             (_PCNT_CTRL_AUXCNTEV_UP << 14)      /**< Shifted mode UP for PCNT_CTRL */
#define PCNT_CTRL_AUXCNTEV_DOWN           (_PCNT_CTRL_AUXCNTEV_DOWN << 14)    /**< Shifted mode DOWN for PCNT_CTRL */
#define PCNT_CTRL_AUXCNTEV_BOTH           (_PCNT_CTRL_AUXCNTEV_BOTH << 14)    /**< Shifted mode BOTH for PCNT_CTRL */

/* Bit fields for PCNT CMD */
#define _PCNT_CMD_RESETVALUE              0x00000000UL                     /**< Default value for PCNT_CMD */
#define _PCNT_CMD_MASK                    0x00000003UL                     /**< Mask for PCNT_CMD */
#define PCNT_CMD_LCNTIM                   (0x1UL << 0)                     /**< Load CNT Immediately */
#define _PCNT_CMD_LCNTIM_SHIFT            0                                /**< Shift value for PCNT_LCNTIM */
#define _PCNT_CMD_LCNTIM_MASK             0x1UL                            /**< Bit mask for PCNT_LCNTIM */
#define _PCNT_CMD_LCNTIM_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for PCNT_CMD */
#define PCNT_CMD_LCNTIM_DEFAULT           (_PCNT_CMD_LCNTIM_DEFAULT << 0)  /**< Shifted mode DEFAULT for PCNT_CMD */
#define PCNT_CMD_LTOPBIM                  (0x1UL << 1)                     /**< Load TOPB Immediately */
#define _PCNT_CMD_LTOPBIM_SHIFT           1                                /**< Shift value for PCNT_LTOPBIM */
#define _PCNT_CMD_LTOPBIM_MASK            0x2UL                            /**< Bit mask for PCNT_LTOPBIM */
#define _PCNT_CMD_LTOPBIM_DEFAULT         0x00000000UL                     /**< Mode DEFAULT for PCNT_CMD */
#define PCNT_CMD_LTOPBIM_DEFAULT          (_PCNT_CMD_LTOPBIM_DEFAULT << 1) /**< Shifted mode DEFAULT for PCNT_CMD */

/* Bit fields for PCNT STATUS */
#define _PCNT_STATUS_RESETVALUE           0x00000000UL                    /**< Default value for PCNT_STATUS */
#define _PCNT_STATUS_MASK                 0x00000001UL                    /**< Mask for PCNT_STATUS */
#define PCNT_STATUS_DIR                   (0x1UL << 0)                    /**< Current Counter Direction */
#define _PCNT_STATUS_DIR_SHIFT            0                               /**< Shift value for PCNT_DIR */
#define _PCNT_STATUS_DIR_MASK             0x1UL                           /**< Bit mask for PCNT_DIR */
#define _PCNT_STATUS_DIR_DEFAULT          0x00000000UL                    /**< Mode DEFAULT for PCNT_STATUS */
#define _PCNT_STATUS_DIR_UP               0x00000000UL                    /**< Mode UP for PCNT_STATUS */
#define _PCNT_STATUS_DIR_DOWN             0x00000001UL                    /**< Mode DOWN for PCNT_STATUS */
#define PCNT_STATUS_DIR_DEFAULT           (_PCNT_STATUS_DIR_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_STATUS */
#define PCNT_STATUS_DIR_UP                (_PCNT_STATUS_DIR_UP << 0)      /**< Shifted mode UP for PCNT_STATUS */
#define PCNT_STATUS_DIR_DOWN              (_PCNT_STATUS_DIR_DOWN << 0)    /**< Shifted mode DOWN for PCNT_STATUS */

/* Bit fields for PCNT CNT */
#define _PCNT_CNT_RESETVALUE              0x00000000UL                 /**< Default value for PCNT_CNT */
#define _PCNT_CNT_MASK                    0x0000FFFFUL                 /**< Mask for PCNT_CNT */
#define _PCNT_CNT_CNT_SHIFT               0                            /**< Shift value for PCNT_CNT */
#define _PCNT_CNT_CNT_MASK                0xFFFFUL                     /**< Bit mask for PCNT_CNT */
#define _PCNT_CNT_CNT_DEFAULT             0x00000000UL                 /**< Mode DEFAULT for PCNT_CNT */
#define PCNT_CNT_CNT_DEFAULT              (_PCNT_CNT_CNT_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_CNT */

/* Bit fields for PCNT TOP */
#define _PCNT_TOP_RESETVALUE              0x000000FFUL                 /**< Default value for PCNT_TOP */
#define _PCNT_TOP_MASK                    0x0000FFFFUL                 /**< Mask for PCNT_TOP */
#define _PCNT_TOP_TOP_SHIFT               0                            /**< Shift value for PCNT_TOP */
#define _PCNT_TOP_TOP_MASK                0xFFFFUL                     /**< Bit mask for PCNT_TOP */
#define _PCNT_TOP_TOP_DEFAULT             0x000000FFUL                 /**< Mode DEFAULT for PCNT_TOP */
#define PCNT_TOP_TOP_DEFAULT              (_PCNT_TOP_TOP_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_TOP */

/* Bit fields for PCNT TOPB */
#define _PCNT_TOPB_RESETVALUE             0x000000FFUL                   /**< Default value for PCNT_TOPB */
#define _PCNT_TOPB_MASK                   0x0000FFFFUL                   /**< Mask for PCNT_TOPB */
#define _PCNT_TOPB_TOPB_SHIFT             0                              /**< Shift value for PCNT_TOPB */
#define _PCNT_TOPB_TOPB_MASK              0xFFFFUL                       /**< Bit mask for PCNT_TOPB */
#define _PCNT_TOPB_TOPB_DEFAULT           0x000000FFUL                   /**< Mode DEFAULT for PCNT_TOPB */
#define PCNT_TOPB_TOPB_DEFAULT            (_PCNT_TOPB_TOPB_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_TOPB */

/* Bit fields for PCNT IF */
#define _PCNT_IF_RESETVALUE               0x00000000UL                   /**< Default value for PCNT_IF */
#define _PCNT_IF_MASK                     0x0000000FUL                   /**< Mask for PCNT_IF */
#define PCNT_IF_UF                        (0x1UL << 0)                   /**< Underflow Interrupt Read Flag */
#define _PCNT_IF_UF_SHIFT                 0                              /**< Shift value for PCNT_UF */
#define _PCNT_IF_UF_MASK                  0x1UL                          /**< Bit mask for PCNT_UF */
#define _PCNT_IF_UF_DEFAULT               0x00000000UL                   /**< Mode DEFAULT for PCNT_IF */
#define PCNT_IF_UF_DEFAULT                (_PCNT_IF_UF_DEFAULT << 0)     /**< Shifted mode DEFAULT for PCNT_IF */
#define PCNT_IF_OF                        (0x1UL << 1)                   /**< Overflow Interrupt Read Flag */
#define _PCNT_IF_OF_SHIFT                 1                              /**< Shift value for PCNT_OF */
#define _PCNT_IF_OF_MASK                  0x2UL                          /**< Bit mask for PCNT_OF */
#define _PCNT_IF_OF_DEFAULT               0x00000000UL                   /**< Mode DEFAULT for PCNT_IF */
#define PCNT_IF_OF_DEFAULT                (_PCNT_IF_OF_DEFAULT << 1)     /**< Shifted mode DEFAULT for PCNT_IF */
#define PCNT_IF_DIRCNG                    (0x1UL << 2)                   /**< Direction Change Detect Interrupt Flag */
#define _PCNT_IF_DIRCNG_SHIFT             2                              /**< Shift value for PCNT_DIRCNG */
#define _PCNT_IF_DIRCNG_MASK              0x4UL                          /**< Bit mask for PCNT_DIRCNG */
#define _PCNT_IF_DIRCNG_DEFAULT           0x00000000UL                   /**< Mode DEFAULT for PCNT_IF */
#define PCNT_IF_DIRCNG_DEFAULT            (_PCNT_IF_DIRCNG_DEFAULT << 2) /**< Shifted mode DEFAULT for PCNT_IF */
#define PCNT_IF_AUXOF                     (0x1UL << 3)                   /**< Overflow Interrupt Read Flag */
#define _PCNT_IF_AUXOF_SHIFT              3                              /**< Shift value for PCNT_AUXOF */
#define _PCNT_IF_AUXOF_MASK               0x8UL                          /**< Bit mask for PCNT_AUXOF */
#define _PCNT_IF_AUXOF_DEFAULT            0x00000000UL                   /**< Mode DEFAULT for PCNT_IF */
#define PCNT_IF_AUXOF_DEFAULT             (_PCNT_IF_AUXOF_DEFAULT << 3)  /**< Shifted mode DEFAULT for PCNT_IF */

/* Bit fields for PCNT IFS */
#define _PCNT_IFS_RESETVALUE              0x00000000UL                    /**< Default value for PCNT_IFS */
#define _PCNT_IFS_MASK                    0x0000000FUL                    /**< Mask for PCNT_IFS */
#define PCNT_IFS_UF                       (0x1UL << 0)                    /**< Underflow interrupt set */
#define _PCNT_IFS_UF_SHIFT                0                               /**< Shift value for PCNT_UF */
#define _PCNT_IFS_UF_MASK                 0x1UL                           /**< Bit mask for PCNT_UF */
#define _PCNT_IFS_UF_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_UF_DEFAULT               (_PCNT_IFS_UF_DEFAULT << 0)     /**< Shifted mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_OF                       (0x1UL << 1)                    /**< Overflow Interrupt Set */
#define _PCNT_IFS_OF_SHIFT                1                               /**< Shift value for PCNT_OF */
#define _PCNT_IFS_OF_MASK                 0x2UL                           /**< Bit mask for PCNT_OF */
#define _PCNT_IFS_OF_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_OF_DEFAULT               (_PCNT_IFS_OF_DEFAULT << 1)     /**< Shifted mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_DIRCNG                   (0x1UL << 2)                    /**< Direction Change Detect Interrupt Set */
#define _PCNT_IFS_DIRCNG_SHIFT            2                               /**< Shift value for PCNT_DIRCNG */
#define _PCNT_IFS_DIRCNG_MASK             0x4UL                           /**< Bit mask for PCNT_DIRCNG */
#define _PCNT_IFS_DIRCNG_DEFAULT          0x00000000UL                    /**< Mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_DIRCNG_DEFAULT           (_PCNT_IFS_DIRCNG_DEFAULT << 2) /**< Shifted mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_AUXOF                    (0x1UL << 3)                    /**< Auxiliary Overflow Interrupt Set */
#define _PCNT_IFS_AUXOF_SHIFT             3                               /**< Shift value for PCNT_AUXOF */
#define _PCNT_IFS_AUXOF_MASK              0x8UL                           /**< Bit mask for PCNT_AUXOF */
#define _PCNT_IFS_AUXOF_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_AUXOF_DEFAULT            (_PCNT_IFS_AUXOF_DEFAULT << 3)  /**< Shifted mode DEFAULT for PCNT_IFS */

/* Bit fields for PCNT IFC */
#define _PCNT_IFC_RESETVALUE              0x00000000UL                    /**< Default value for PCNT_IFC */
#define _PCNT_IFC_MASK                    0x0000000FUL                    /**< Mask for PCNT_IFC */
#define PCNT_IFC_UF                       (0x1UL << 0)                    /**< Underflow Interrupt Clear */
#define _PCNT_IFC_UF_SHIFT                0                               /**< Shift value for PCNT_UF */
#define _PCNT_IFC_UF_MASK                 0x1UL                           /**< Bit mask for PCNT_UF */
#define _PCNT_IFC_UF_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_UF_DEFAULT               (_PCNT_IFC_UF_DEFAULT << 0)     /**< Shifted mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_OF                       (0x1UL << 1)                    /**< Overflow Interrupt Clear */
#define _PCNT_IFC_OF_SHIFT                1                               /**< Shift value for PCNT_OF */
#define _PCNT_IFC_OF_MASK                 0x2UL                           /**< Bit mask for PCNT_OF */
#define _PCNT_IFC_OF_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_OF_DEFAULT               (_PCNT_IFC_OF_DEFAULT << 1)     /**< Shifted mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_DIRCNG                   (0x1UL << 2)                    /**< Direction Change Detect Interrupt Clear */
#define _PCNT_IFC_DIRCNG_SHIFT            2                               /**< Shift value for PCNT_DIRCNG */
#define _PCNT_IFC_DIRCNG_MASK             0x4UL                           /**< Bit mask for PCNT_DIRCNG */
#define _PCNT_IFC_DIRCNG_DEFAULT          0x00000000UL                    /**< Mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_DIRCNG_DEFAULT           (_PCNT_IFC_DIRCNG_DEFAULT << 2) /**< Shifted mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_AUXOF                    (0x1UL << 3)                    /**< Auxiliary Overflow Interrupt Clear */
#define _PCNT_IFC_AUXOF_SHIFT             3                               /**< Shift value for PCNT_AUXOF */
#define _PCNT_IFC_AUXOF_MASK              0x8UL                           /**< Bit mask for PCNT_AUXOF */
#define _PCNT_IFC_AUXOF_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_AUXOF_DEFAULT            (_PCNT_IFC_AUXOF_DEFAULT << 3)  /**< Shifted mode DEFAULT for PCNT_IFC */

/* Bit fields for PCNT IEN */
#define _PCNT_IEN_RESETVALUE              0x00000000UL                    /**< Default value for PCNT_IEN */
#define _PCNT_IEN_MASK                    0x0000000FUL                    /**< Mask for PCNT_IEN */
#define PCNT_IEN_UF                       (0x1UL << 0)                    /**< Underflow Interrupt Enable */
#define _PCNT_IEN_UF_SHIFT                0                               /**< Shift value for PCNT_UF */
#define _PCNT_IEN_UF_MASK                 0x1UL                           /**< Bit mask for PCNT_UF */
#define _PCNT_IEN_UF_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_UF_DEFAULT               (_PCNT_IEN_UF_DEFAULT << 0)     /**< Shifted mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_OF                       (0x1UL << 1)                    /**< Overflow Interrupt Enable */
#define _PCNT_IEN_OF_SHIFT                1                               /**< Shift value for PCNT_OF */
#define _PCNT_IEN_OF_MASK                 0x2UL                           /**< Bit mask for PCNT_OF */
#define _PCNT_IEN_OF_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_OF_DEFAULT               (_PCNT_IEN_OF_DEFAULT << 1)     /**< Shifted mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_DIRCNG                   (0x1UL << 2)                    /**< Direction Change Detect Interrupt Enable */
#define _PCNT_IEN_DIRCNG_SHIFT            2                               /**< Shift value for PCNT_DIRCNG */
#define _PCNT_IEN_DIRCNG_MASK             0x4UL                           /**< Bit mask for PCNT_DIRCNG */
#define _PCNT_IEN_DIRCNG_DEFAULT          0x00000000UL                    /**< Mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_DIRCNG_DEFAULT           (_PCNT_IEN_DIRCNG_DEFAULT << 2) /**< Shifted mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_AUXOF                    (0x1UL << 3)                    /**< Auxiliary Overflow Interrupt Enable */
#define _PCNT_IEN_AUXOF_SHIFT             3                               /**< Shift value for PCNT_AUXOF */
#define _PCNT_IEN_AUXOF_MASK              0x8UL                           /**< Bit mask for PCNT_AUXOF */
#define _PCNT_IEN_AUXOF_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_AUXOF_DEFAULT            (_PCNT_IEN_AUXOF_DEFAULT << 3)  /**< Shifted mode DEFAULT for PCNT_IEN */

/* Bit fields for PCNT ROUTE */
#define _PCNT_ROUTE_RESETVALUE            0x00000000UL                        /**< Default value for PCNT_ROUTE */
#define _PCNT_ROUTE_MASK                  0x00000700UL                        /**< Mask for PCNT_ROUTE */
#define _PCNT_ROUTE_LOCATION_SHIFT        8                                   /**< Shift value for PCNT_LOCATION */
#define _PCNT_ROUTE_LOCATION_MASK         0x700UL                             /**< Bit mask for PCNT_LOCATION */
#define _PCNT_ROUTE_LOCATION_LOC0         0x00000000UL                        /**< Mode LOC0 for PCNT_ROUTE */
#define _PCNT_ROUTE_LOCATION_DEFAULT      0x00000000UL                        /**< Mode DEFAULT for PCNT_ROUTE */
#define _PCNT_ROUTE_LOCATION_LOC1         0x00000001UL                        /**< Mode LOC1 for PCNT_ROUTE */
#define _PCNT_ROUTE_LOCATION_LOC2         0x00000002UL                        /**< Mode LOC2 for PCNT_ROUTE */
#define _PCNT_ROUTE_LOCATION_LOC3         0x00000003UL                        /**< Mode LOC3 for PCNT_ROUTE */
#define PCNT_ROUTE_LOCATION_LOC0          (_PCNT_ROUTE_LOCATION_LOC0 << 8)    /**< Shifted mode LOC0 for PCNT_ROUTE */
#define PCNT_ROUTE_LOCATION_DEFAULT       (_PCNT_ROUTE_LOCATION_DEFAULT << 8) /**< Shifted mode DEFAULT for PCNT_ROUTE */
#define PCNT_ROUTE_LOCATION_LOC1          (_PCNT_ROUTE_LOCATION_LOC1 << 8)    /**< Shifted mode LOC1 for PCNT_ROUTE */
#define PCNT_ROUTE_LOCATION_LOC2          (_PCNT_ROUTE_LOCATION_LOC2 << 8)    /**< Shifted mode LOC2 for PCNT_ROUTE */
#define PCNT_ROUTE_LOCATION_LOC3          (_PCNT_ROUTE_LOCATION_LOC3 << 8)    /**< Shifted mode LOC3 for PCNT_ROUTE */

/* Bit fields for PCNT FREEZE */
#define _PCNT_FREEZE_RESETVALUE           0x00000000UL                          /**< Default value for PCNT_FREEZE */
#define _PCNT_FREEZE_MASK                 0x00000001UL                          /**< Mask for PCNT_FREEZE */
#define PCNT_FREEZE_REGFREEZE             (0x1UL << 0)                          /**< Register Update Freeze */
#define _PCNT_FREEZE_REGFREEZE_SHIFT      0                                     /**< Shift value for PCNT_REGFREEZE */
#define _PCNT_FREEZE_REGFREEZE_MASK       0x1UL                                 /**< Bit mask for PCNT_REGFREEZE */
#define _PCNT_FREEZE_REGFREEZE_DEFAULT    0x00000000UL                          /**< Mode DEFAULT for PCNT_FREEZE */
#define _PCNT_FREEZE_REGFREEZE_UPDATE     0x00000000UL                          /**< Mode UPDATE for PCNT_FREEZE */
#define _PCNT_FREEZE_REGFREEZE_FREEZE     0x00000001UL                          /**< Mode FREEZE for PCNT_FREEZE */
#define PCNT_FREEZE_REGFREEZE_DEFAULT     (_PCNT_FREEZE_REGFREEZE_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_FREEZE */
#define PCNT_FREEZE_REGFREEZE_UPDATE      (_PCNT_FREEZE_REGFREEZE_UPDATE << 0)  /**< Shifted mode UPDATE for PCNT_FREEZE */
#define PCNT_FREEZE_REGFREEZE_FREEZE      (_PCNT_FREEZE_REGFREEZE_FREEZE << 0)  /**< Shifted mode FREEZE for PCNT_FREEZE */

/* Bit fields for PCNT SYNCBUSY */
#define _PCNT_SYNCBUSY_RESETVALUE         0x00000000UL                       /**< Default value for PCNT_SYNCBUSY */
#define _PCNT_SYNCBUSY_MASK               0x00000007UL                       /**< Mask for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_CTRL                (0x1UL << 0)                       /**< CTRL Register Busy */
#define _PCNT_SYNCBUSY_CTRL_SHIFT         0                                  /**< Shift value for PCNT_CTRL */
#define _PCNT_SYNCBUSY_CTRL_MASK          0x1UL                              /**< Bit mask for PCNT_CTRL */
#define _PCNT_SYNCBUSY_CTRL_DEFAULT       0x00000000UL                       /**< Mode DEFAULT for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_CTRL_DEFAULT        (_PCNT_SYNCBUSY_CTRL_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_CMD                 (0x1UL << 1)                       /**< CMD Register Busy */
#define _PCNT_SYNCBUSY_CMD_SHIFT          1                                  /**< Shift value for PCNT_CMD */
#define _PCNT_SYNCBUSY_CMD_MASK           0x2UL                              /**< Bit mask for PCNT_CMD */
#define _PCNT_SYNCBUSY_CMD_DEFAULT        0x00000000UL                       /**< Mode DEFAULT for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_CMD_DEFAULT         (_PCNT_SYNCBUSY_CMD_DEFAULT << 1)  /**< Shifted mode DEFAULT for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_TOPB                (0x1UL << 2)                       /**< TOPB Register Busy */
#define _PCNT_SYNCBUSY_TOPB_SHIFT         2                                  /**< Shift value for PCNT_TOPB */
#define _PCNT_SYNCBUSY_TOPB_MASK          0x4UL                              /**< Bit mask for PCNT_TOPB */
#define _PCNT_SYNCBUSY_TOPB_DEFAULT       0x00000000UL                       /**< Mode DEFAULT for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_TOPB_DEFAULT        (_PCNT_SYNCBUSY_TOPB_DEFAULT << 2) /**< Shifted mode DEFAULT for PCNT_SYNCBUSY */

/* Bit fields for PCNT AUXCNT */
#define _PCNT_AUXCNT_RESETVALUE           0x00000000UL                       /**< Default value for PCNT_AUXCNT */
#define _PCNT_AUXCNT_MASK                 0x0000FFFFUL                       /**< Mask for PCNT_AUXCNT */
#define _PCNT_AUXCNT_AUXCNT_SHIFT         0                                  /**< Shift value for PCNT_AUXCNT */
#define _PCNT_AUXCNT_AUXCNT_MASK          0xFFFFUL                           /**< Bit mask for PCNT_AUXCNT */
#define _PCNT_AUXCNT_AUXCNT_DEFAULT       0x00000000UL                       /**< Mode DEFAULT for PCNT_AUXCNT */
#define PCNT_AUXCNT_AUXCNT_DEFAULT        (_PCNT_AUXCNT_AUXCNT_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_AUXCNT */

/* Bit fields for PCNT INPUT */
#define _PCNT_INPUT_RESETVALUE            0x00000000UL                        /**< Default value for PCNT_INPUT */
#define _PCNT_INPUT_MASK                  0x000007DFUL                        /**< Mask for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_SHIFT        0                                   /**< Shift value for PCNT_S0PRSSEL */
#define _PCNT_INPUT_S0PRSSEL_MASK         0xFUL                               /**< Bit mask for PCNT_S0PRSSEL */
#define _PCNT_INPUT_S0PRSSEL_DEFAULT      0x00000000UL                        /**< Mode DEFAULT for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH0       0x00000000UL                        /**< Mode PRSCH0 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH1       0x00000001UL                        /**< Mode PRSCH1 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH2       0x00000002UL                        /**< Mode PRSCH2 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH3       0x00000003UL                        /**< Mode PRSCH3 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH4       0x00000004UL                        /**< Mode PRSCH4 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH5       0x00000005UL                        /**< Mode PRSCH5 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH6       0x00000006UL                        /**< Mode PRSCH6 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH7       0x00000007UL                        /**< Mode PRSCH7 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH8       0x00000008UL                        /**< Mode PRSCH8 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH9       0x00000009UL                        /**< Mode PRSCH9 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH10      0x0000000AUL                        /**< Mode PRSCH10 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH11      0x0000000BUL                        /**< Mode PRSCH11 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_DEFAULT       (_PCNT_INPUT_S0PRSSEL_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH0        (_PCNT_INPUT_S0PRSSEL_PRSCH0 << 0)  /**< Shifted mode PRSCH0 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH1        (_PCNT_INPUT_S0PRSSEL_PRSCH1 << 0)  /**< Shifted mode PRSCH1 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH2        (_PCNT_INPUT_S0PRSSEL_PRSCH2 << 0)  /**< Shifted mode PRSCH2 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH3        (_PCNT_INPUT_S0PRSSEL_PRSCH3 << 0)  /**< Shifted mode PRSCH3 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH4        (_PCNT_INPUT_S0PRSSEL_PRSCH4 << 0)  /**< Shifted mode PRSCH4 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH5        (_PCNT_INPUT_S0PRSSEL_PRSCH5 << 0)  /**< Shifted mode PRSCH5 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH6        (_PCNT_INPUT_S0PRSSEL_PRSCH6 << 0)  /**< Shifted mode PRSCH6 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH7        (_PCNT_INPUT_S0PRSSEL_PRSCH7 << 0)  /**< Shifted mode PRSCH7 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH8        (_PCNT_INPUT_S0PRSSEL_PRSCH8 << 0)  /**< Shifted mode PRSCH8 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH9        (_PCNT_INPUT_S0PRSSEL_PRSCH9 << 0)  /**< Shifted mode PRSCH9 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH10       (_PCNT_INPUT_S0PRSSEL_PRSCH10 << 0) /**< Shifted mode PRSCH10 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH11       (_PCNT_INPUT_S0PRSSEL_PRSCH11 << 0) /**< Shifted mode PRSCH11 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSEN                (0x1UL << 4)                        /**< S0IN PRS Enable */
#define _PCNT_INPUT_S0PRSEN_SHIFT         4                                   /**< Shift value for PCNT_S0PRSEN */
#define _PCNT_INPUT_S0PRSEN_MASK          0x10UL                              /**< Bit mask for PCNT_S0PRSEN */
#define _PCNT_INPUT_S0PRSEN_DEFAULT       0x00000000UL                        /**< Mode DEFAULT for PCNT_INPUT */
#define PCNT_INPUT_S0PRSEN_DEFAULT        (_PCNT_INPUT_S0PRSEN_DEFAULT << 4)  /**< Shifted mode DEFAULT for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_SHIFT        6                                   /**< Shift value for PCNT_S1PRSSEL */
#define _PCNT_INPUT_S1PRSSEL_MASK         0x3C0UL                             /**< Bit mask for PCNT_S1PRSSEL */
#define _PCNT_INPUT_S1PRSSEL_DEFAULT      0x00000000UL                        /**< Mode DEFAULT for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH0       0x00000000UL                        /**< Mode PRSCH0 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH1       0x00000001UL                        /**< Mode PRSCH1 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH2       0x00000002UL                        /**< Mode PRSCH2 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH3       0x00000003UL                        /**< Mode PRSCH3 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH4       0x00000004UL                        /**< Mode PRSCH4 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH5       0x00000005UL                        /**< Mode PRSCH5 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH6       0x00000006UL                        /**< Mode PRSCH6 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH7       0x00000007UL                        /**< Mode PRSCH7 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH8       0x00000008UL                        /**< Mode PRSCH8 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH9       0x00000009UL                        /**< Mode PRSCH9 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH10      0x0000000AUL                        /**< Mode PRSCH10 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH11      0x0000000BUL                        /**< Mode PRSCH11 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_DEFAULT       (_PCNT_INPUT_S1PRSSEL_DEFAULT << 6) /**< Shifted mode DEFAULT for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH0        (_PCNT_INPUT_S1PRSSEL_PRSCH0 << 6)  /**< Shifted mode PRSCH0 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH1        (_PCNT_INPUT_S1PRSSEL_PRSCH1 << 6)  /**< Shifted mode PRSCH1 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH2        (_PCNT_INPUT_S1PRSSEL_PRSCH2 << 6)  /**< Shifted mode PRSCH2 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH3        (_PCNT_INPUT_S1PRSSEL_PRSCH3 << 6)  /**< Shifted mode PRSCH3 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH4        (_PCNT_INPUT_S1PRSSEL_PRSCH4 << 6)  /**< Shifted mode PRSCH4 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH5        (_PCNT_INPUT_S1PRSSEL_PRSCH5 << 6)  /**< Shifted mode PRSCH5 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH6        (_PCNT_INPUT_S1PRSSEL_PRSCH6 << 6)  /**< Shifted mode PRSCH6 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH7        (_PCNT_INPUT_S1PRSSEL_PRSCH7 << 6)  /**< Shifted mode PRSCH7 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH8        (_PCNT_INPUT_S1PRSSEL_PRSCH8 << 6)  /**< Shifted mode PRSCH8 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH9        (_PCNT_INPUT_S1PRSSEL_PRSCH9 << 6)  /**< Shifted mode PRSCH9 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH10       (_PCNT_INPUT_S1PRSSEL_PRSCH10 << 6) /**< Shifted mode PRSCH10 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH11       (_PCNT_INPUT_S1PRSSEL_PRSCH11 << 6) /**< Shifted mode PRSCH11 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSEN                (0x1UL << 10)                       /**< S1IN PRS Enable */
#define _PCNT_INPUT_S1PRSEN_SHIFT         10                                  /**< Shift value for PCNT_S1PRSEN */
#define _PCNT_INPUT_S1PRSEN_MASK          0x400UL                             /**< Bit mask for PCNT_S1PRSEN */
#define _PCNT_INPUT_S1PRSEN_DEFAULT       0x00000000UL                        /**< Mode DEFAULT for PCNT_INPUT */
#define PCNT_INPUT_S1PRSEN_DEFAULT        (_PCNT_INPUT_S1PRSEN_DEFAULT << 10) /**< Shifted mode DEFAULT for PCNT_INPUT */

/** @} End of group EFM32WG_PCNT */
/** @} End of group Parts */

