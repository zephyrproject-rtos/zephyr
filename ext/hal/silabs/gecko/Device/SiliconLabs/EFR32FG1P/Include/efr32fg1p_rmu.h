/**************************************************************************//**
 * @file efr32fg1p_rmu.h
 * @brief EFR32FG1P_RMU register and bit field definitions
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
 * @defgroup EFR32FG1P_RMU
 * @{
 * @brief EFR32FG1P_RMU Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;     /**< Control Register  */
  __IM uint32_t  RSTCAUSE; /**< Reset Cause Register  */
  __IOM uint32_t CMD;      /**< Command Register  */
  __IOM uint32_t RST;      /**< Reset Control Register  */
  __IOM uint32_t LOCK;     /**< Configuration Lock Register  */
} RMU_TypeDef;             /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_RMU_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for RMU CTRL */
#define _RMU_CTRL_RESETVALUE               0x00004224UL                          /**< Default value for RMU_CTRL */
#define _RMU_CTRL_MASK                     0x03007777UL                          /**< Mask for RMU_CTRL */
#define _RMU_CTRL_WDOGRMODE_SHIFT          0                                     /**< Shift value for RMU_WDOGRMODE */
#define _RMU_CTRL_WDOGRMODE_MASK           0x7UL                                 /**< Bit mask for RMU_WDOGRMODE */
#define _RMU_CTRL_WDOGRMODE_DISABLED       0x00000000UL                          /**< Mode DISABLED for RMU_CTRL */
#define _RMU_CTRL_WDOGRMODE_LIMITED        0x00000001UL                          /**< Mode LIMITED for RMU_CTRL */
#define _RMU_CTRL_WDOGRMODE_EXTENDED       0x00000002UL                          /**< Mode EXTENDED for RMU_CTRL */
#define _RMU_CTRL_WDOGRMODE_DEFAULT        0x00000004UL                          /**< Mode DEFAULT for RMU_CTRL */
#define _RMU_CTRL_WDOGRMODE_FULL           0x00000004UL                          /**< Mode FULL for RMU_CTRL */
#define RMU_CTRL_WDOGRMODE_DISABLED        (_RMU_CTRL_WDOGRMODE_DISABLED << 0)   /**< Shifted mode DISABLED for RMU_CTRL */
#define RMU_CTRL_WDOGRMODE_LIMITED         (_RMU_CTRL_WDOGRMODE_LIMITED << 0)    /**< Shifted mode LIMITED for RMU_CTRL */
#define RMU_CTRL_WDOGRMODE_EXTENDED        (_RMU_CTRL_WDOGRMODE_EXTENDED << 0)   /**< Shifted mode EXTENDED for RMU_CTRL */
#define RMU_CTRL_WDOGRMODE_DEFAULT         (_RMU_CTRL_WDOGRMODE_DEFAULT << 0)    /**< Shifted mode DEFAULT for RMU_CTRL */
#define RMU_CTRL_WDOGRMODE_FULL            (_RMU_CTRL_WDOGRMODE_FULL << 0)       /**< Shifted mode FULL for RMU_CTRL */
#define _RMU_CTRL_LOCKUPRMODE_SHIFT        4                                     /**< Shift value for RMU_LOCKUPRMODE */
#define _RMU_CTRL_LOCKUPRMODE_MASK         0x70UL                                /**< Bit mask for RMU_LOCKUPRMODE */
#define _RMU_CTRL_LOCKUPRMODE_DISABLED     0x00000000UL                          /**< Mode DISABLED for RMU_CTRL */
#define _RMU_CTRL_LOCKUPRMODE_LIMITED      0x00000001UL                          /**< Mode LIMITED for RMU_CTRL */
#define _RMU_CTRL_LOCKUPRMODE_DEFAULT      0x00000002UL                          /**< Mode DEFAULT for RMU_CTRL */
#define _RMU_CTRL_LOCKUPRMODE_EXTENDED     0x00000002UL                          /**< Mode EXTENDED for RMU_CTRL */
#define _RMU_CTRL_LOCKUPRMODE_FULL         0x00000004UL                          /**< Mode FULL for RMU_CTRL */
#define RMU_CTRL_LOCKUPRMODE_DISABLED      (_RMU_CTRL_LOCKUPRMODE_DISABLED << 4) /**< Shifted mode DISABLED for RMU_CTRL */
#define RMU_CTRL_LOCKUPRMODE_LIMITED       (_RMU_CTRL_LOCKUPRMODE_LIMITED << 4)  /**< Shifted mode LIMITED for RMU_CTRL */
#define RMU_CTRL_LOCKUPRMODE_DEFAULT       (_RMU_CTRL_LOCKUPRMODE_DEFAULT << 4)  /**< Shifted mode DEFAULT for RMU_CTRL */
#define RMU_CTRL_LOCKUPRMODE_EXTENDED      (_RMU_CTRL_LOCKUPRMODE_EXTENDED << 4) /**< Shifted mode EXTENDED for RMU_CTRL */
#define RMU_CTRL_LOCKUPRMODE_FULL          (_RMU_CTRL_LOCKUPRMODE_FULL << 4)     /**< Shifted mode FULL for RMU_CTRL */
#define _RMU_CTRL_SYSRMODE_SHIFT           8                                     /**< Shift value for RMU_SYSRMODE */
#define _RMU_CTRL_SYSRMODE_MASK            0x700UL                               /**< Bit mask for RMU_SYSRMODE */
#define _RMU_CTRL_SYSRMODE_DISABLED        0x00000000UL                          /**< Mode DISABLED for RMU_CTRL */
#define _RMU_CTRL_SYSRMODE_LIMITED         0x00000001UL                          /**< Mode LIMITED for RMU_CTRL */
#define _RMU_CTRL_SYSRMODE_DEFAULT         0x00000002UL                          /**< Mode DEFAULT for RMU_CTRL */
#define _RMU_CTRL_SYSRMODE_EXTENDED        0x00000002UL                          /**< Mode EXTENDED for RMU_CTRL */
#define _RMU_CTRL_SYSRMODE_FULL            0x00000004UL                          /**< Mode FULL for RMU_CTRL */
#define RMU_CTRL_SYSRMODE_DISABLED         (_RMU_CTRL_SYSRMODE_DISABLED << 8)    /**< Shifted mode DISABLED for RMU_CTRL */
#define RMU_CTRL_SYSRMODE_LIMITED          (_RMU_CTRL_SYSRMODE_LIMITED << 8)     /**< Shifted mode LIMITED for RMU_CTRL */
#define RMU_CTRL_SYSRMODE_DEFAULT          (_RMU_CTRL_SYSRMODE_DEFAULT << 8)     /**< Shifted mode DEFAULT for RMU_CTRL */
#define RMU_CTRL_SYSRMODE_EXTENDED         (_RMU_CTRL_SYSRMODE_EXTENDED << 8)    /**< Shifted mode EXTENDED for RMU_CTRL */
#define RMU_CTRL_SYSRMODE_FULL             (_RMU_CTRL_SYSRMODE_FULL << 8)        /**< Shifted mode FULL for RMU_CTRL */
#define _RMU_CTRL_PINRMODE_SHIFT           12                                    /**< Shift value for RMU_PINRMODE */
#define _RMU_CTRL_PINRMODE_MASK            0x7000UL                              /**< Bit mask for RMU_PINRMODE */
#define _RMU_CTRL_PINRMODE_DISABLED        0x00000000UL                          /**< Mode DISABLED for RMU_CTRL */
#define _RMU_CTRL_PINRMODE_LIMITED         0x00000001UL                          /**< Mode LIMITED for RMU_CTRL */
#define _RMU_CTRL_PINRMODE_EXTENDED        0x00000002UL                          /**< Mode EXTENDED for RMU_CTRL */
#define _RMU_CTRL_PINRMODE_DEFAULT         0x00000004UL                          /**< Mode DEFAULT for RMU_CTRL */
#define _RMU_CTRL_PINRMODE_FULL            0x00000004UL                          /**< Mode FULL for RMU_CTRL */
#define RMU_CTRL_PINRMODE_DISABLED         (_RMU_CTRL_PINRMODE_DISABLED << 12)   /**< Shifted mode DISABLED for RMU_CTRL */
#define RMU_CTRL_PINRMODE_LIMITED          (_RMU_CTRL_PINRMODE_LIMITED << 12)    /**< Shifted mode LIMITED for RMU_CTRL */
#define RMU_CTRL_PINRMODE_EXTENDED         (_RMU_CTRL_PINRMODE_EXTENDED << 12)   /**< Shifted mode EXTENDED for RMU_CTRL */
#define RMU_CTRL_PINRMODE_DEFAULT          (_RMU_CTRL_PINRMODE_DEFAULT << 12)    /**< Shifted mode DEFAULT for RMU_CTRL */
#define RMU_CTRL_PINRMODE_FULL             (_RMU_CTRL_PINRMODE_FULL << 12)       /**< Shifted mode FULL for RMU_CTRL */
#define _RMU_CTRL_RESETSTATE_SHIFT         24                                    /**< Shift value for RMU_RESETSTATE */
#define _RMU_CTRL_RESETSTATE_MASK          0x3000000UL                           /**< Bit mask for RMU_RESETSTATE */
#define _RMU_CTRL_RESETSTATE_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for RMU_CTRL */
#define RMU_CTRL_RESETSTATE_DEFAULT        (_RMU_CTRL_RESETSTATE_DEFAULT << 24)  /**< Shifted mode DEFAULT for RMU_CTRL */

/* Bit fields for RMU RSTCAUSE */
#define _RMU_RSTCAUSE_RESETVALUE           0x00000000UL                            /**< Default value for RMU_RSTCAUSE */
#define _RMU_RSTCAUSE_MASK                 0x00010F1DUL                            /**< Mask for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_PORST                 (0x1UL << 0)                            /**< Power On Reset */
#define _RMU_RSTCAUSE_PORST_SHIFT          0                                       /**< Shift value for RMU_PORST */
#define _RMU_RSTCAUSE_PORST_MASK           0x1UL                                   /**< Bit mask for RMU_PORST */
#define _RMU_RSTCAUSE_PORST_DEFAULT        0x00000000UL                            /**< Mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_PORST_DEFAULT         (_RMU_RSTCAUSE_PORST_DEFAULT << 0)      /**< Shifted mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_AVDDBOD               (0x1UL << 2)                            /**< Brown Out Detector AVDD Reset */
#define _RMU_RSTCAUSE_AVDDBOD_SHIFT        2                                       /**< Shift value for RMU_AVDDBOD */
#define _RMU_RSTCAUSE_AVDDBOD_MASK         0x4UL                                   /**< Bit mask for RMU_AVDDBOD */
#define _RMU_RSTCAUSE_AVDDBOD_DEFAULT      0x00000000UL                            /**< Mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_AVDDBOD_DEFAULT       (_RMU_RSTCAUSE_AVDDBOD_DEFAULT << 2)    /**< Shifted mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_DVDDBOD               (0x1UL << 3)                            /**< Brown Out Detector DVDD Reset */
#define _RMU_RSTCAUSE_DVDDBOD_SHIFT        3                                       /**< Shift value for RMU_DVDDBOD */
#define _RMU_RSTCAUSE_DVDDBOD_MASK         0x8UL                                   /**< Bit mask for RMU_DVDDBOD */
#define _RMU_RSTCAUSE_DVDDBOD_DEFAULT      0x00000000UL                            /**< Mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_DVDDBOD_DEFAULT       (_RMU_RSTCAUSE_DVDDBOD_DEFAULT << 3)    /**< Shifted mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_DECBOD                (0x1UL << 4)                            /**< Brown Out Detector Decouple Domain Reset */
#define _RMU_RSTCAUSE_DECBOD_SHIFT         4                                       /**< Shift value for RMU_DECBOD */
#define _RMU_RSTCAUSE_DECBOD_MASK          0x10UL                                  /**< Bit mask for RMU_DECBOD */
#define _RMU_RSTCAUSE_DECBOD_DEFAULT       0x00000000UL                            /**< Mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_DECBOD_DEFAULT        (_RMU_RSTCAUSE_DECBOD_DEFAULT << 4)     /**< Shifted mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_EXTRST                (0x1UL << 8)                            /**< External Pin Reset */
#define _RMU_RSTCAUSE_EXTRST_SHIFT         8                                       /**< Shift value for RMU_EXTRST */
#define _RMU_RSTCAUSE_EXTRST_MASK          0x100UL                                 /**< Bit mask for RMU_EXTRST */
#define _RMU_RSTCAUSE_EXTRST_DEFAULT       0x00000000UL                            /**< Mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_EXTRST_DEFAULT        (_RMU_RSTCAUSE_EXTRST_DEFAULT << 8)     /**< Shifted mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_LOCKUPRST             (0x1UL << 9)                            /**< LOCKUP Reset */
#define _RMU_RSTCAUSE_LOCKUPRST_SHIFT      9                                       /**< Shift value for RMU_LOCKUPRST */
#define _RMU_RSTCAUSE_LOCKUPRST_MASK       0x200UL                                 /**< Bit mask for RMU_LOCKUPRST */
#define _RMU_RSTCAUSE_LOCKUPRST_DEFAULT    0x00000000UL                            /**< Mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_LOCKUPRST_DEFAULT     (_RMU_RSTCAUSE_LOCKUPRST_DEFAULT << 9)  /**< Shifted mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_SYSREQRST             (0x1UL << 10)                           /**< System Request Reset */
#define _RMU_RSTCAUSE_SYSREQRST_SHIFT      10                                      /**< Shift value for RMU_SYSREQRST */
#define _RMU_RSTCAUSE_SYSREQRST_MASK       0x400UL                                 /**< Bit mask for RMU_SYSREQRST */
#define _RMU_RSTCAUSE_SYSREQRST_DEFAULT    0x00000000UL                            /**< Mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_SYSREQRST_DEFAULT     (_RMU_RSTCAUSE_SYSREQRST_DEFAULT << 10) /**< Shifted mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_WDOGRST               (0x1UL << 11)                           /**< Watchdog Reset */
#define _RMU_RSTCAUSE_WDOGRST_SHIFT        11                                      /**< Shift value for RMU_WDOGRST */
#define _RMU_RSTCAUSE_WDOGRST_MASK         0x800UL                                 /**< Bit mask for RMU_WDOGRST */
#define _RMU_RSTCAUSE_WDOGRST_DEFAULT      0x00000000UL                            /**< Mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_WDOGRST_DEFAULT       (_RMU_RSTCAUSE_WDOGRST_DEFAULT << 11)   /**< Shifted mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_EM4RST                (0x1UL << 16)                           /**< EM4 Reset */
#define _RMU_RSTCAUSE_EM4RST_SHIFT         16                                      /**< Shift value for RMU_EM4RST */
#define _RMU_RSTCAUSE_EM4RST_MASK          0x10000UL                               /**< Bit mask for RMU_EM4RST */
#define _RMU_RSTCAUSE_EM4RST_DEFAULT       0x00000000UL                            /**< Mode DEFAULT for RMU_RSTCAUSE */
#define RMU_RSTCAUSE_EM4RST_DEFAULT        (_RMU_RSTCAUSE_EM4RST_DEFAULT << 16)    /**< Shifted mode DEFAULT for RMU_RSTCAUSE */

/* Bit fields for RMU CMD */
#define _RMU_CMD_RESETVALUE                0x00000000UL                  /**< Default value for RMU_CMD */
#define _RMU_CMD_MASK                      0x00000001UL                  /**< Mask for RMU_CMD */
#define RMU_CMD_RCCLR                      (0x1UL << 0)                  /**< Reset Cause Clear */
#define _RMU_CMD_RCCLR_SHIFT               0                             /**< Shift value for RMU_RCCLR */
#define _RMU_CMD_RCCLR_MASK                0x1UL                         /**< Bit mask for RMU_RCCLR */
#define _RMU_CMD_RCCLR_DEFAULT             0x00000000UL                  /**< Mode DEFAULT for RMU_CMD */
#define RMU_CMD_RCCLR_DEFAULT              (_RMU_CMD_RCCLR_DEFAULT << 0) /**< Shifted mode DEFAULT for RMU_CMD */

/* Bit fields for RMU RST */
#define _RMU_RST_RESETVALUE                0x00000000UL /**< Default value for RMU_RST */
#define _RMU_RST_MASK                      0x00000000UL /**< Mask for RMU_RST */

/* Bit fields for RMU LOCK */
#define _RMU_LOCK_RESETVALUE               0x00000000UL                      /**< Default value for RMU_LOCK */
#define _RMU_LOCK_MASK                     0x0000FFFFUL                      /**< Mask for RMU_LOCK */
#define _RMU_LOCK_LOCKKEY_SHIFT            0                                 /**< Shift value for RMU_LOCKKEY */
#define _RMU_LOCK_LOCKKEY_MASK             0xFFFFUL                          /**< Bit mask for RMU_LOCKKEY */
#define _RMU_LOCK_LOCKKEY_DEFAULT          0x00000000UL                      /**< Mode DEFAULT for RMU_LOCK */
#define _RMU_LOCK_LOCKKEY_LOCK             0x00000000UL                      /**< Mode LOCK for RMU_LOCK */
#define _RMU_LOCK_LOCKKEY_UNLOCKED         0x00000000UL                      /**< Mode UNLOCKED for RMU_LOCK */
#define _RMU_LOCK_LOCKKEY_LOCKED           0x00000001UL                      /**< Mode LOCKED for RMU_LOCK */
#define _RMU_LOCK_LOCKKEY_UNLOCK           0x0000E084UL                      /**< Mode UNLOCK for RMU_LOCK */
#define RMU_LOCK_LOCKKEY_DEFAULT           (_RMU_LOCK_LOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for RMU_LOCK */
#define RMU_LOCK_LOCKKEY_LOCK              (_RMU_LOCK_LOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for RMU_LOCK */
#define RMU_LOCK_LOCKKEY_UNLOCKED          (_RMU_LOCK_LOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for RMU_LOCK */
#define RMU_LOCK_LOCKKEY_LOCKED            (_RMU_LOCK_LOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for RMU_LOCK */
#define RMU_LOCK_LOCKKEY_UNLOCK            (_RMU_LOCK_LOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for RMU_LOCK */

/** @} End of group EFR32FG1P_RMU */
/** @} End of group Parts */

