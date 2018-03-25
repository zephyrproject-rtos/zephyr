/**************************************************************************//**
 * @file efr32fg1p_pcnt.h
 * @brief EFR32FG1P_PCNT register and bit field definitions
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
 * @defgroup EFR32FG1P_PCNT
 * @{
 * @brief EFR32FG1P_PCNT Register Declaration
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
  uint32_t       RESERVED0[1]; /**< Reserved for future use **/
  __IOM uint32_t ROUTELOC0;    /**< I/O Routing Location Register  */

  uint32_t       RESERVED1[4]; /**< Reserved for future use **/
  __IOM uint32_t FREEZE;       /**< Freeze Register  */
  __IM uint32_t  SYNCBUSY;     /**< Synchronization Busy Register  */

  uint32_t       RESERVED2[7]; /**< Reserved for future use **/
  __IM uint32_t  AUXCNT;       /**< Auxiliary Counter Value Register  */
  __IOM uint32_t INPUT;        /**< PCNT Input Register  */
  __IOM uint32_t OVSCFG;       /**< Oversampling Config Register  */
} PCNT_TypeDef;                /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_PCNT_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for PCNT CTRL */
#define _PCNT_CTRL_RESETVALUE              0x00000000UL                          /**< Default value for PCNT_CTRL */
#define _PCNT_CTRL_MASK                    0xBFDBFFFFUL                          /**< Mask for PCNT_CTRL */
#define _PCNT_CTRL_MODE_SHIFT              0                                     /**< Shift value for PCNT_MODE */
#define _PCNT_CTRL_MODE_MASK               0x7UL                                 /**< Bit mask for PCNT_MODE */
#define _PCNT_CTRL_MODE_DEFAULT            0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_MODE_DISABLE            0x00000000UL                          /**< Mode DISABLE for PCNT_CTRL */
#define _PCNT_CTRL_MODE_OVSSINGLE          0x00000001UL                          /**< Mode OVSSINGLE for PCNT_CTRL */
#define _PCNT_CTRL_MODE_EXTCLKSINGLE       0x00000002UL                          /**< Mode EXTCLKSINGLE for PCNT_CTRL */
#define _PCNT_CTRL_MODE_EXTCLKQUAD         0x00000003UL                          /**< Mode EXTCLKQUAD for PCNT_CTRL */
#define _PCNT_CTRL_MODE_OVSQUAD1X          0x00000004UL                          /**< Mode OVSQUAD1X for PCNT_CTRL */
#define _PCNT_CTRL_MODE_OVSQUAD2X          0x00000005UL                          /**< Mode OVSQUAD2X for PCNT_CTRL */
#define _PCNT_CTRL_MODE_OVSQUAD4X          0x00000006UL                          /**< Mode OVSQUAD4X for PCNT_CTRL */
#define PCNT_CTRL_MODE_DEFAULT             (_PCNT_CTRL_MODE_DEFAULT << 0)        /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_MODE_DISABLE             (_PCNT_CTRL_MODE_DISABLE << 0)        /**< Shifted mode DISABLE for PCNT_CTRL */
#define PCNT_CTRL_MODE_OVSSINGLE           (_PCNT_CTRL_MODE_OVSSINGLE << 0)      /**< Shifted mode OVSSINGLE for PCNT_CTRL */
#define PCNT_CTRL_MODE_EXTCLKSINGLE        (_PCNT_CTRL_MODE_EXTCLKSINGLE << 0)   /**< Shifted mode EXTCLKSINGLE for PCNT_CTRL */
#define PCNT_CTRL_MODE_EXTCLKQUAD          (_PCNT_CTRL_MODE_EXTCLKQUAD << 0)     /**< Shifted mode EXTCLKQUAD for PCNT_CTRL */
#define PCNT_CTRL_MODE_OVSQUAD1X           (_PCNT_CTRL_MODE_OVSQUAD1X << 0)      /**< Shifted mode OVSQUAD1X for PCNT_CTRL */
#define PCNT_CTRL_MODE_OVSQUAD2X           (_PCNT_CTRL_MODE_OVSQUAD2X << 0)      /**< Shifted mode OVSQUAD2X for PCNT_CTRL */
#define PCNT_CTRL_MODE_OVSQUAD4X           (_PCNT_CTRL_MODE_OVSQUAD4X << 0)      /**< Shifted mode OVSQUAD4X for PCNT_CTRL */
#define PCNT_CTRL_FILT                     (0x1UL << 3)                          /**< Enable Digital Pulse Width Filter */
#define _PCNT_CTRL_FILT_SHIFT              3                                     /**< Shift value for PCNT_FILT */
#define _PCNT_CTRL_FILT_MASK               0x8UL                                 /**< Bit mask for PCNT_FILT */
#define _PCNT_CTRL_FILT_DEFAULT            0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_FILT_DEFAULT             (_PCNT_CTRL_FILT_DEFAULT << 3)        /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_RSTEN                    (0x1UL << 4)                          /**< Enable PCNT Clock Domain Reset */
#define _PCNT_CTRL_RSTEN_SHIFT             4                                     /**< Shift value for PCNT_RSTEN */
#define _PCNT_CTRL_RSTEN_MASK              0x10UL                                /**< Bit mask for PCNT_RSTEN */
#define _PCNT_CTRL_RSTEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_RSTEN_DEFAULT            (_PCNT_CTRL_RSTEN_DEFAULT << 4)       /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_CNTRSTEN                 (0x1UL << 5)                          /**< Enable CNT Reset */
#define _PCNT_CTRL_CNTRSTEN_SHIFT          5                                     /**< Shift value for PCNT_CNTRSTEN */
#define _PCNT_CTRL_CNTRSTEN_MASK           0x20UL                                /**< Bit mask for PCNT_CNTRSTEN */
#define _PCNT_CTRL_CNTRSTEN_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_CNTRSTEN_DEFAULT         (_PCNT_CTRL_CNTRSTEN_DEFAULT << 5)    /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_AUXCNTRSTEN              (0x1UL << 6)                          /**< Enable AUXCNT Reset */
#define _PCNT_CTRL_AUXCNTRSTEN_SHIFT       6                                     /**< Shift value for PCNT_AUXCNTRSTEN */
#define _PCNT_CTRL_AUXCNTRSTEN_MASK        0x40UL                                /**< Bit mask for PCNT_AUXCNTRSTEN */
#define _PCNT_CTRL_AUXCNTRSTEN_DEFAULT     0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_AUXCNTRSTEN_DEFAULT      (_PCNT_CTRL_AUXCNTRSTEN_DEFAULT << 6) /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_DEBUGHALT                (0x1UL << 7)                          /**< Debug Mode Halt Enable */
#define _PCNT_CTRL_DEBUGHALT_SHIFT         7                                     /**< Shift value for PCNT_DEBUGHALT */
#define _PCNT_CTRL_DEBUGHALT_MASK          0x80UL                                /**< Bit mask for PCNT_DEBUGHALT */
#define _PCNT_CTRL_DEBUGHALT_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_DEBUGHALT_DEFAULT        (_PCNT_CTRL_DEBUGHALT_DEFAULT << 7)   /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_HYST                     (0x1UL << 8)                          /**< Enable Hysteresis */
#define _PCNT_CTRL_HYST_SHIFT              8                                     /**< Shift value for PCNT_HYST */
#define _PCNT_CTRL_HYST_MASK               0x100UL                               /**< Bit mask for PCNT_HYST */
#define _PCNT_CTRL_HYST_DEFAULT            0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_HYST_DEFAULT             (_PCNT_CTRL_HYST_DEFAULT << 8)        /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_S1CDIR                   (0x1UL << 9)                          /**< Count direction determined by S1 */
#define _PCNT_CTRL_S1CDIR_SHIFT            9                                     /**< Shift value for PCNT_S1CDIR */
#define _PCNT_CTRL_S1CDIR_MASK             0x200UL                               /**< Bit mask for PCNT_S1CDIR */
#define _PCNT_CTRL_S1CDIR_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_S1CDIR_DEFAULT           (_PCNT_CTRL_S1CDIR_DEFAULT << 9)      /**< Shifted mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_CNTEV_SHIFT             10                                    /**< Shift value for PCNT_CNTEV */
#define _PCNT_CTRL_CNTEV_MASK              0xC00UL                               /**< Bit mask for PCNT_CNTEV */
#define _PCNT_CTRL_CNTEV_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_CNTEV_BOTH              0x00000000UL                          /**< Mode BOTH for PCNT_CTRL */
#define _PCNT_CTRL_CNTEV_UP                0x00000001UL                          /**< Mode UP for PCNT_CTRL */
#define _PCNT_CTRL_CNTEV_DOWN              0x00000002UL                          /**< Mode DOWN for PCNT_CTRL */
#define _PCNT_CTRL_CNTEV_NONE              0x00000003UL                          /**< Mode NONE for PCNT_CTRL */
#define PCNT_CTRL_CNTEV_DEFAULT            (_PCNT_CTRL_CNTEV_DEFAULT << 10)      /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_CNTEV_BOTH               (_PCNT_CTRL_CNTEV_BOTH << 10)         /**< Shifted mode BOTH for PCNT_CTRL */
#define PCNT_CTRL_CNTEV_UP                 (_PCNT_CTRL_CNTEV_UP << 10)           /**< Shifted mode UP for PCNT_CTRL */
#define PCNT_CTRL_CNTEV_DOWN               (_PCNT_CTRL_CNTEV_DOWN << 10)         /**< Shifted mode DOWN for PCNT_CTRL */
#define PCNT_CTRL_CNTEV_NONE               (_PCNT_CTRL_CNTEV_NONE << 10)         /**< Shifted mode NONE for PCNT_CTRL */
#define _PCNT_CTRL_AUXCNTEV_SHIFT          12                                    /**< Shift value for PCNT_AUXCNTEV */
#define _PCNT_CTRL_AUXCNTEV_MASK           0x3000UL                              /**< Bit mask for PCNT_AUXCNTEV */
#define _PCNT_CTRL_AUXCNTEV_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_AUXCNTEV_NONE           0x00000000UL                          /**< Mode NONE for PCNT_CTRL */
#define _PCNT_CTRL_AUXCNTEV_UP             0x00000001UL                          /**< Mode UP for PCNT_CTRL */
#define _PCNT_CTRL_AUXCNTEV_DOWN           0x00000002UL                          /**< Mode DOWN for PCNT_CTRL */
#define _PCNT_CTRL_AUXCNTEV_BOTH           0x00000003UL                          /**< Mode BOTH for PCNT_CTRL */
#define PCNT_CTRL_AUXCNTEV_DEFAULT         (_PCNT_CTRL_AUXCNTEV_DEFAULT << 12)   /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_AUXCNTEV_NONE            (_PCNT_CTRL_AUXCNTEV_NONE << 12)      /**< Shifted mode NONE for PCNT_CTRL */
#define PCNT_CTRL_AUXCNTEV_UP              (_PCNT_CTRL_AUXCNTEV_UP << 12)        /**< Shifted mode UP for PCNT_CTRL */
#define PCNT_CTRL_AUXCNTEV_DOWN            (_PCNT_CTRL_AUXCNTEV_DOWN << 12)      /**< Shifted mode DOWN for PCNT_CTRL */
#define PCNT_CTRL_AUXCNTEV_BOTH            (_PCNT_CTRL_AUXCNTEV_BOTH << 12)      /**< Shifted mode BOTH for PCNT_CTRL */
#define PCNT_CTRL_CNTDIR                   (0x1UL << 14)                         /**< Non-Quadrature Mode Counter Direction Control */
#define _PCNT_CTRL_CNTDIR_SHIFT            14                                    /**< Shift value for PCNT_CNTDIR */
#define _PCNT_CTRL_CNTDIR_MASK             0x4000UL                              /**< Bit mask for PCNT_CNTDIR */
#define _PCNT_CTRL_CNTDIR_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_CNTDIR_UP               0x00000000UL                          /**< Mode UP for PCNT_CTRL */
#define _PCNT_CTRL_CNTDIR_DOWN             0x00000001UL                          /**< Mode DOWN for PCNT_CTRL */
#define PCNT_CTRL_CNTDIR_DEFAULT           (_PCNT_CTRL_CNTDIR_DEFAULT << 14)     /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_CNTDIR_UP                (_PCNT_CTRL_CNTDIR_UP << 14)          /**< Shifted mode UP for PCNT_CTRL */
#define PCNT_CTRL_CNTDIR_DOWN              (_PCNT_CTRL_CNTDIR_DOWN << 14)        /**< Shifted mode DOWN for PCNT_CTRL */
#define PCNT_CTRL_EDGE                     (0x1UL << 15)                         /**< Edge Select */
#define _PCNT_CTRL_EDGE_SHIFT              15                                    /**< Shift value for PCNT_EDGE */
#define _PCNT_CTRL_EDGE_MASK               0x8000UL                              /**< Bit mask for PCNT_EDGE */
#define _PCNT_CTRL_EDGE_DEFAULT            0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_EDGE_POS                0x00000000UL                          /**< Mode POS for PCNT_CTRL */
#define _PCNT_CTRL_EDGE_NEG                0x00000001UL                          /**< Mode NEG for PCNT_CTRL */
#define PCNT_CTRL_EDGE_DEFAULT             (_PCNT_CTRL_EDGE_DEFAULT << 15)       /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_EDGE_POS                 (_PCNT_CTRL_EDGE_POS << 15)           /**< Shifted mode POS for PCNT_CTRL */
#define PCNT_CTRL_EDGE_NEG                 (_PCNT_CTRL_EDGE_NEG << 15)           /**< Shifted mode NEG for PCNT_CTRL */
#define _PCNT_CTRL_TCCMODE_SHIFT           16                                    /**< Shift value for PCNT_TCCMODE */
#define _PCNT_CTRL_TCCMODE_MASK            0x30000UL                             /**< Bit mask for PCNT_TCCMODE */
#define _PCNT_CTRL_TCCMODE_DEFAULT         0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_TCCMODE_DISABLED        0x00000000UL                          /**< Mode DISABLED for PCNT_CTRL */
#define _PCNT_CTRL_TCCMODE_LFA             0x00000001UL                          /**< Mode LFA for PCNT_CTRL */
#define _PCNT_CTRL_TCCMODE_PRS             0x00000002UL                          /**< Mode PRS for PCNT_CTRL */
#define PCNT_CTRL_TCCMODE_DEFAULT          (_PCNT_CTRL_TCCMODE_DEFAULT << 16)    /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_TCCMODE_DISABLED         (_PCNT_CTRL_TCCMODE_DISABLED << 16)   /**< Shifted mode DISABLED for PCNT_CTRL */
#define PCNT_CTRL_TCCMODE_LFA              (_PCNT_CTRL_TCCMODE_LFA << 16)        /**< Shifted mode LFA for PCNT_CTRL */
#define PCNT_CTRL_TCCMODE_PRS              (_PCNT_CTRL_TCCMODE_PRS << 16)        /**< Shifted mode PRS for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRESC_SHIFT          19                                    /**< Shift value for PCNT_TCCPRESC */
#define _PCNT_CTRL_TCCPRESC_MASK           0x180000UL                            /**< Bit mask for PCNT_TCCPRESC */
#define _PCNT_CTRL_TCCPRESC_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRESC_DIV1           0x00000000UL                          /**< Mode DIV1 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRESC_DIV2           0x00000001UL                          /**< Mode DIV2 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRESC_DIV4           0x00000002UL                          /**< Mode DIV4 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRESC_DIV8           0x00000003UL                          /**< Mode DIV8 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRESC_DEFAULT         (_PCNT_CTRL_TCCPRESC_DEFAULT << 19)   /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_TCCPRESC_DIV1            (_PCNT_CTRL_TCCPRESC_DIV1 << 19)      /**< Shifted mode DIV1 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRESC_DIV2            (_PCNT_CTRL_TCCPRESC_DIV2 << 19)      /**< Shifted mode DIV2 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRESC_DIV4            (_PCNT_CTRL_TCCPRESC_DIV4 << 19)      /**< Shifted mode DIV4 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRESC_DIV8            (_PCNT_CTRL_TCCPRESC_DIV8 << 19)      /**< Shifted mode DIV8 for PCNT_CTRL */
#define _PCNT_CTRL_TCCCOMP_SHIFT           22                                    /**< Shift value for PCNT_TCCCOMP */
#define _PCNT_CTRL_TCCCOMP_MASK            0xC00000UL                            /**< Bit mask for PCNT_TCCCOMP */
#define _PCNT_CTRL_TCCCOMP_DEFAULT         0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_TCCCOMP_LTOE            0x00000000UL                          /**< Mode LTOE for PCNT_CTRL */
#define _PCNT_CTRL_TCCCOMP_GTOE            0x00000001UL                          /**< Mode GTOE for PCNT_CTRL */
#define _PCNT_CTRL_TCCCOMP_RANGE           0x00000002UL                          /**< Mode RANGE for PCNT_CTRL */
#define PCNT_CTRL_TCCCOMP_DEFAULT          (_PCNT_CTRL_TCCCOMP_DEFAULT << 22)    /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_TCCCOMP_LTOE             (_PCNT_CTRL_TCCCOMP_LTOE << 22)       /**< Shifted mode LTOE for PCNT_CTRL */
#define PCNT_CTRL_TCCCOMP_GTOE             (_PCNT_CTRL_TCCCOMP_GTOE << 22)       /**< Shifted mode GTOE for PCNT_CTRL */
#define PCNT_CTRL_TCCCOMP_RANGE            (_PCNT_CTRL_TCCCOMP_RANGE << 22)      /**< Shifted mode RANGE for PCNT_CTRL */
#define PCNT_CTRL_PRSGATEEN                (0x1UL << 24)                         /**< PRS gate enable */
#define _PCNT_CTRL_PRSGATEEN_SHIFT         24                                    /**< Shift value for PCNT_PRSGATEEN */
#define _PCNT_CTRL_PRSGATEEN_MASK          0x1000000UL                           /**< Bit mask for PCNT_PRSGATEEN */
#define _PCNT_CTRL_PRSGATEEN_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_PRSGATEEN_DEFAULT        (_PCNT_CTRL_PRSGATEEN_DEFAULT << 24)  /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSPOL                (0x1UL << 25)                         /**< TCC PRS polarity select */
#define _PCNT_CTRL_TCCPRSPOL_SHIFT         25                                    /**< Shift value for PCNT_TCCPRSPOL */
#define _PCNT_CTRL_TCCPRSPOL_MASK          0x2000000UL                           /**< Bit mask for PCNT_TCCPRSPOL */
#define _PCNT_CTRL_TCCPRSPOL_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSPOL_RISING        0x00000000UL                          /**< Mode RISING for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSPOL_FALLING       0x00000001UL                          /**< Mode FALLING for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSPOL_DEFAULT        (_PCNT_CTRL_TCCPRSPOL_DEFAULT << 25)  /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSPOL_RISING         (_PCNT_CTRL_TCCPRSPOL_RISING << 25)   /**< Shifted mode RISING for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSPOL_FALLING        (_PCNT_CTRL_TCCPRSPOL_FALLING << 25)  /**< Shifted mode FALLING for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSSEL_SHIFT         26                                    /**< Shift value for PCNT_TCCPRSSEL */
#define _PCNT_CTRL_TCCPRSSEL_MASK          0x3C000000UL                          /**< Bit mask for PCNT_TCCPRSSEL */
#define _PCNT_CTRL_TCCPRSSEL_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSSEL_PRSCH0        0x00000000UL                          /**< Mode PRSCH0 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSSEL_PRSCH1        0x00000001UL                          /**< Mode PRSCH1 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSSEL_PRSCH2        0x00000002UL                          /**< Mode PRSCH2 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSSEL_PRSCH3        0x00000003UL                          /**< Mode PRSCH3 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSSEL_PRSCH4        0x00000004UL                          /**< Mode PRSCH4 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSSEL_PRSCH5        0x00000005UL                          /**< Mode PRSCH5 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSSEL_PRSCH6        0x00000006UL                          /**< Mode PRSCH6 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSSEL_PRSCH7        0x00000007UL                          /**< Mode PRSCH7 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSSEL_PRSCH8        0x00000008UL                          /**< Mode PRSCH8 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSSEL_PRSCH9        0x00000009UL                          /**< Mode PRSCH9 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSSEL_PRSCH10       0x0000000AUL                          /**< Mode PRSCH10 for PCNT_CTRL */
#define _PCNT_CTRL_TCCPRSSEL_PRSCH11       0x0000000BUL                          /**< Mode PRSCH11 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSSEL_DEFAULT        (_PCNT_CTRL_TCCPRSSEL_DEFAULT << 26)  /**< Shifted mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSSEL_PRSCH0         (_PCNT_CTRL_TCCPRSSEL_PRSCH0 << 26)   /**< Shifted mode PRSCH0 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSSEL_PRSCH1         (_PCNT_CTRL_TCCPRSSEL_PRSCH1 << 26)   /**< Shifted mode PRSCH1 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSSEL_PRSCH2         (_PCNT_CTRL_TCCPRSSEL_PRSCH2 << 26)   /**< Shifted mode PRSCH2 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSSEL_PRSCH3         (_PCNT_CTRL_TCCPRSSEL_PRSCH3 << 26)   /**< Shifted mode PRSCH3 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSSEL_PRSCH4         (_PCNT_CTRL_TCCPRSSEL_PRSCH4 << 26)   /**< Shifted mode PRSCH4 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSSEL_PRSCH5         (_PCNT_CTRL_TCCPRSSEL_PRSCH5 << 26)   /**< Shifted mode PRSCH5 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSSEL_PRSCH6         (_PCNT_CTRL_TCCPRSSEL_PRSCH6 << 26)   /**< Shifted mode PRSCH6 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSSEL_PRSCH7         (_PCNT_CTRL_TCCPRSSEL_PRSCH7 << 26)   /**< Shifted mode PRSCH7 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSSEL_PRSCH8         (_PCNT_CTRL_TCCPRSSEL_PRSCH8 << 26)   /**< Shifted mode PRSCH8 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSSEL_PRSCH9         (_PCNT_CTRL_TCCPRSSEL_PRSCH9 << 26)   /**< Shifted mode PRSCH9 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSSEL_PRSCH10        (_PCNT_CTRL_TCCPRSSEL_PRSCH10 << 26)  /**< Shifted mode PRSCH10 for PCNT_CTRL */
#define PCNT_CTRL_TCCPRSSEL_PRSCH11        (_PCNT_CTRL_TCCPRSSEL_PRSCH11 << 26)  /**< Shifted mode PRSCH11 for PCNT_CTRL */
#define PCNT_CTRL_TOPBHFSEL                (0x1UL << 31)                         /**< TOPB High frequency value select */
#define _PCNT_CTRL_TOPBHFSEL_SHIFT         31                                    /**< Shift value for PCNT_TOPBHFSEL */
#define _PCNT_CTRL_TOPBHFSEL_MASK          0x80000000UL                          /**< Bit mask for PCNT_TOPBHFSEL */
#define _PCNT_CTRL_TOPBHFSEL_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for PCNT_CTRL */
#define PCNT_CTRL_TOPBHFSEL_DEFAULT        (_PCNT_CTRL_TOPBHFSEL_DEFAULT << 31)  /**< Shifted mode DEFAULT for PCNT_CTRL */

/* Bit fields for PCNT CMD */
#define _PCNT_CMD_RESETVALUE               0x00000000UL                     /**< Default value for PCNT_CMD */
#define _PCNT_CMD_MASK                     0x00000003UL                     /**< Mask for PCNT_CMD */
#define PCNT_CMD_LCNTIM                    (0x1UL << 0)                     /**< Load CNT Immediately */
#define _PCNT_CMD_LCNTIM_SHIFT             0                                /**< Shift value for PCNT_LCNTIM */
#define _PCNT_CMD_LCNTIM_MASK              0x1UL                            /**< Bit mask for PCNT_LCNTIM */
#define _PCNT_CMD_LCNTIM_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for PCNT_CMD */
#define PCNT_CMD_LCNTIM_DEFAULT            (_PCNT_CMD_LCNTIM_DEFAULT << 0)  /**< Shifted mode DEFAULT for PCNT_CMD */
#define PCNT_CMD_LTOPBIM                   (0x1UL << 1)                     /**< Load TOPB Immediately */
#define _PCNT_CMD_LTOPBIM_SHIFT            1                                /**< Shift value for PCNT_LTOPBIM */
#define _PCNT_CMD_LTOPBIM_MASK             0x2UL                            /**< Bit mask for PCNT_LTOPBIM */
#define _PCNT_CMD_LTOPBIM_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for PCNT_CMD */
#define PCNT_CMD_LTOPBIM_DEFAULT           (_PCNT_CMD_LTOPBIM_DEFAULT << 1) /**< Shifted mode DEFAULT for PCNT_CMD */

/* Bit fields for PCNT STATUS */
#define _PCNT_STATUS_RESETVALUE            0x00000000UL                    /**< Default value for PCNT_STATUS */
#define _PCNT_STATUS_MASK                  0x00000001UL                    /**< Mask for PCNT_STATUS */
#define PCNT_STATUS_DIR                    (0x1UL << 0)                    /**< Current Counter Direction */
#define _PCNT_STATUS_DIR_SHIFT             0                               /**< Shift value for PCNT_DIR */
#define _PCNT_STATUS_DIR_MASK              0x1UL                           /**< Bit mask for PCNT_DIR */
#define _PCNT_STATUS_DIR_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for PCNT_STATUS */
#define _PCNT_STATUS_DIR_UP                0x00000000UL                    /**< Mode UP for PCNT_STATUS */
#define _PCNT_STATUS_DIR_DOWN              0x00000001UL                    /**< Mode DOWN for PCNT_STATUS */
#define PCNT_STATUS_DIR_DEFAULT            (_PCNT_STATUS_DIR_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_STATUS */
#define PCNT_STATUS_DIR_UP                 (_PCNT_STATUS_DIR_UP << 0)      /**< Shifted mode UP for PCNT_STATUS */
#define PCNT_STATUS_DIR_DOWN               (_PCNT_STATUS_DIR_DOWN << 0)    /**< Shifted mode DOWN for PCNT_STATUS */

/* Bit fields for PCNT CNT */
#define _PCNT_CNT_RESETVALUE               0x00000000UL                 /**< Default value for PCNT_CNT */
#define _PCNT_CNT_MASK                     0x0000FFFFUL                 /**< Mask for PCNT_CNT */
#define _PCNT_CNT_CNT_SHIFT                0                            /**< Shift value for PCNT_CNT */
#define _PCNT_CNT_CNT_MASK                 0xFFFFUL                     /**< Bit mask for PCNT_CNT */
#define _PCNT_CNT_CNT_DEFAULT              0x00000000UL                 /**< Mode DEFAULT for PCNT_CNT */
#define PCNT_CNT_CNT_DEFAULT               (_PCNT_CNT_CNT_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_CNT */

/* Bit fields for PCNT TOP */
#define _PCNT_TOP_RESETVALUE               0x000000FFUL                 /**< Default value for PCNT_TOP */
#define _PCNT_TOP_MASK                     0x0000FFFFUL                 /**< Mask for PCNT_TOP */
#define _PCNT_TOP_TOP_SHIFT                0                            /**< Shift value for PCNT_TOP */
#define _PCNT_TOP_TOP_MASK                 0xFFFFUL                     /**< Bit mask for PCNT_TOP */
#define _PCNT_TOP_TOP_DEFAULT              0x000000FFUL                 /**< Mode DEFAULT for PCNT_TOP */
#define PCNT_TOP_TOP_DEFAULT               (_PCNT_TOP_TOP_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_TOP */

/* Bit fields for PCNT TOPB */
#define _PCNT_TOPB_RESETVALUE              0x000000FFUL                   /**< Default value for PCNT_TOPB */
#define _PCNT_TOPB_MASK                    0x0000FFFFUL                   /**< Mask for PCNT_TOPB */
#define _PCNT_TOPB_TOPB_SHIFT              0                              /**< Shift value for PCNT_TOPB */
#define _PCNT_TOPB_TOPB_MASK               0xFFFFUL                       /**< Bit mask for PCNT_TOPB */
#define _PCNT_TOPB_TOPB_DEFAULT            0x000000FFUL                   /**< Mode DEFAULT for PCNT_TOPB */
#define PCNT_TOPB_TOPB_DEFAULT             (_PCNT_TOPB_TOPB_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_TOPB */

/* Bit fields for PCNT IF */
#define _PCNT_IF_RESETVALUE                0x00000000UL                    /**< Default value for PCNT_IF */
#define _PCNT_IF_MASK                      0x0000003FUL                    /**< Mask for PCNT_IF */
#define PCNT_IF_UF                         (0x1UL << 0)                    /**< Underflow Interrupt Read Flag */
#define _PCNT_IF_UF_SHIFT                  0                               /**< Shift value for PCNT_UF */
#define _PCNT_IF_UF_MASK                   0x1UL                           /**< Bit mask for PCNT_UF */
#define _PCNT_IF_UF_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for PCNT_IF */
#define PCNT_IF_UF_DEFAULT                 (_PCNT_IF_UF_DEFAULT << 0)      /**< Shifted mode DEFAULT for PCNT_IF */
#define PCNT_IF_OF                         (0x1UL << 1)                    /**< Overflow Interrupt Read Flag */
#define _PCNT_IF_OF_SHIFT                  1                               /**< Shift value for PCNT_OF */
#define _PCNT_IF_OF_MASK                   0x2UL                           /**< Bit mask for PCNT_OF */
#define _PCNT_IF_OF_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for PCNT_IF */
#define PCNT_IF_OF_DEFAULT                 (_PCNT_IF_OF_DEFAULT << 1)      /**< Shifted mode DEFAULT for PCNT_IF */
#define PCNT_IF_DIRCNG                     (0x1UL << 2)                    /**< Direction Change Detect Interrupt Flag */
#define _PCNT_IF_DIRCNG_SHIFT              2                               /**< Shift value for PCNT_DIRCNG */
#define _PCNT_IF_DIRCNG_MASK               0x4UL                           /**< Bit mask for PCNT_DIRCNG */
#define _PCNT_IF_DIRCNG_DEFAULT            0x00000000UL                    /**< Mode DEFAULT for PCNT_IF */
#define PCNT_IF_DIRCNG_DEFAULT             (_PCNT_IF_DIRCNG_DEFAULT << 2)  /**< Shifted mode DEFAULT for PCNT_IF */
#define PCNT_IF_AUXOF                      (0x1UL << 3)                    /**< Auxiliary Overflow Interrupt Read Flag */
#define _PCNT_IF_AUXOF_SHIFT               3                               /**< Shift value for PCNT_AUXOF */
#define _PCNT_IF_AUXOF_MASK                0x8UL                           /**< Bit mask for PCNT_AUXOF */
#define _PCNT_IF_AUXOF_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for PCNT_IF */
#define PCNT_IF_AUXOF_DEFAULT              (_PCNT_IF_AUXOF_DEFAULT << 3)   /**< Shifted mode DEFAULT for PCNT_IF */
#define PCNT_IF_TCC                        (0x1UL << 4)                    /**< Triggered compare Interrupt Read Flag */
#define _PCNT_IF_TCC_SHIFT                 4                               /**< Shift value for PCNT_TCC */
#define _PCNT_IF_TCC_MASK                  0x10UL                          /**< Bit mask for PCNT_TCC */
#define _PCNT_IF_TCC_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for PCNT_IF */
#define PCNT_IF_TCC_DEFAULT                (_PCNT_IF_TCC_DEFAULT << 4)     /**< Shifted mode DEFAULT for PCNT_IF */
#define PCNT_IF_OQSTERR                    (0x1UL << 5)                    /**< Oversampling Quadrature State Error Interrupt */
#define _PCNT_IF_OQSTERR_SHIFT             5                               /**< Shift value for PCNT_OQSTERR */
#define _PCNT_IF_OQSTERR_MASK              0x20UL                          /**< Bit mask for PCNT_OQSTERR */
#define _PCNT_IF_OQSTERR_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for PCNT_IF */
#define PCNT_IF_OQSTERR_DEFAULT            (_PCNT_IF_OQSTERR_DEFAULT << 5) /**< Shifted mode DEFAULT for PCNT_IF */

/* Bit fields for PCNT IFS */
#define _PCNT_IFS_RESETVALUE               0x00000000UL                     /**< Default value for PCNT_IFS */
#define _PCNT_IFS_MASK                     0x0000003FUL                     /**< Mask for PCNT_IFS */
#define PCNT_IFS_UF                        (0x1UL << 0)                     /**< Set UF Interrupt Flag */
#define _PCNT_IFS_UF_SHIFT                 0                                /**< Shift value for PCNT_UF */
#define _PCNT_IFS_UF_MASK                  0x1UL                            /**< Bit mask for PCNT_UF */
#define _PCNT_IFS_UF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_UF_DEFAULT                (_PCNT_IFS_UF_DEFAULT << 0)      /**< Shifted mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_OF                        (0x1UL << 1)                     /**< Set OF Interrupt Flag */
#define _PCNT_IFS_OF_SHIFT                 1                                /**< Shift value for PCNT_OF */
#define _PCNT_IFS_OF_MASK                  0x2UL                            /**< Bit mask for PCNT_OF */
#define _PCNT_IFS_OF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_OF_DEFAULT                (_PCNT_IFS_OF_DEFAULT << 1)      /**< Shifted mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_DIRCNG                    (0x1UL << 2)                     /**< Set DIRCNG Interrupt Flag */
#define _PCNT_IFS_DIRCNG_SHIFT             2                                /**< Shift value for PCNT_DIRCNG */
#define _PCNT_IFS_DIRCNG_MASK              0x4UL                            /**< Bit mask for PCNT_DIRCNG */
#define _PCNT_IFS_DIRCNG_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_DIRCNG_DEFAULT            (_PCNT_IFS_DIRCNG_DEFAULT << 2)  /**< Shifted mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_AUXOF                     (0x1UL << 3)                     /**< Set AUXOF Interrupt Flag */
#define _PCNT_IFS_AUXOF_SHIFT              3                                /**< Shift value for PCNT_AUXOF */
#define _PCNT_IFS_AUXOF_MASK               0x8UL                            /**< Bit mask for PCNT_AUXOF */
#define _PCNT_IFS_AUXOF_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_AUXOF_DEFAULT             (_PCNT_IFS_AUXOF_DEFAULT << 3)   /**< Shifted mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_TCC                       (0x1UL << 4)                     /**< Set TCC Interrupt Flag */
#define _PCNT_IFS_TCC_SHIFT                4                                /**< Shift value for PCNT_TCC */
#define _PCNT_IFS_TCC_MASK                 0x10UL                           /**< Bit mask for PCNT_TCC */
#define _PCNT_IFS_TCC_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_TCC_DEFAULT               (_PCNT_IFS_TCC_DEFAULT << 4)     /**< Shifted mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_OQSTERR                   (0x1UL << 5)                     /**< Set OQSTERR Interrupt Flag */
#define _PCNT_IFS_OQSTERR_SHIFT            5                                /**< Shift value for PCNT_OQSTERR */
#define _PCNT_IFS_OQSTERR_MASK             0x20UL                           /**< Bit mask for PCNT_OQSTERR */
#define _PCNT_IFS_OQSTERR_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for PCNT_IFS */
#define PCNT_IFS_OQSTERR_DEFAULT           (_PCNT_IFS_OQSTERR_DEFAULT << 5) /**< Shifted mode DEFAULT for PCNT_IFS */

/* Bit fields for PCNT IFC */
#define _PCNT_IFC_RESETVALUE               0x00000000UL                     /**< Default value for PCNT_IFC */
#define _PCNT_IFC_MASK                     0x0000003FUL                     /**< Mask for PCNT_IFC */
#define PCNT_IFC_UF                        (0x1UL << 0)                     /**< Clear UF Interrupt Flag */
#define _PCNT_IFC_UF_SHIFT                 0                                /**< Shift value for PCNT_UF */
#define _PCNT_IFC_UF_MASK                  0x1UL                            /**< Bit mask for PCNT_UF */
#define _PCNT_IFC_UF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_UF_DEFAULT                (_PCNT_IFC_UF_DEFAULT << 0)      /**< Shifted mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_OF                        (0x1UL << 1)                     /**< Clear OF Interrupt Flag */
#define _PCNT_IFC_OF_SHIFT                 1                                /**< Shift value for PCNT_OF */
#define _PCNT_IFC_OF_MASK                  0x2UL                            /**< Bit mask for PCNT_OF */
#define _PCNT_IFC_OF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_OF_DEFAULT                (_PCNT_IFC_OF_DEFAULT << 1)      /**< Shifted mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_DIRCNG                    (0x1UL << 2)                     /**< Clear DIRCNG Interrupt Flag */
#define _PCNT_IFC_DIRCNG_SHIFT             2                                /**< Shift value for PCNT_DIRCNG */
#define _PCNT_IFC_DIRCNG_MASK              0x4UL                            /**< Bit mask for PCNT_DIRCNG */
#define _PCNT_IFC_DIRCNG_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_DIRCNG_DEFAULT            (_PCNT_IFC_DIRCNG_DEFAULT << 2)  /**< Shifted mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_AUXOF                     (0x1UL << 3)                     /**< Clear AUXOF Interrupt Flag */
#define _PCNT_IFC_AUXOF_SHIFT              3                                /**< Shift value for PCNT_AUXOF */
#define _PCNT_IFC_AUXOF_MASK               0x8UL                            /**< Bit mask for PCNT_AUXOF */
#define _PCNT_IFC_AUXOF_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_AUXOF_DEFAULT             (_PCNT_IFC_AUXOF_DEFAULT << 3)   /**< Shifted mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_TCC                       (0x1UL << 4)                     /**< Clear TCC Interrupt Flag */
#define _PCNT_IFC_TCC_SHIFT                4                                /**< Shift value for PCNT_TCC */
#define _PCNT_IFC_TCC_MASK                 0x10UL                           /**< Bit mask for PCNT_TCC */
#define _PCNT_IFC_TCC_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_TCC_DEFAULT               (_PCNT_IFC_TCC_DEFAULT << 4)     /**< Shifted mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_OQSTERR                   (0x1UL << 5)                     /**< Clear OQSTERR Interrupt Flag */
#define _PCNT_IFC_OQSTERR_SHIFT            5                                /**< Shift value for PCNT_OQSTERR */
#define _PCNT_IFC_OQSTERR_MASK             0x20UL                           /**< Bit mask for PCNT_OQSTERR */
#define _PCNT_IFC_OQSTERR_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for PCNT_IFC */
#define PCNT_IFC_OQSTERR_DEFAULT           (_PCNT_IFC_OQSTERR_DEFAULT << 5) /**< Shifted mode DEFAULT for PCNT_IFC */

/* Bit fields for PCNT IEN */
#define _PCNT_IEN_RESETVALUE               0x00000000UL                     /**< Default value for PCNT_IEN */
#define _PCNT_IEN_MASK                     0x0000003FUL                     /**< Mask for PCNT_IEN */
#define PCNT_IEN_UF                        (0x1UL << 0)                     /**< UF Interrupt Enable */
#define _PCNT_IEN_UF_SHIFT                 0                                /**< Shift value for PCNT_UF */
#define _PCNT_IEN_UF_MASK                  0x1UL                            /**< Bit mask for PCNT_UF */
#define _PCNT_IEN_UF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_UF_DEFAULT                (_PCNT_IEN_UF_DEFAULT << 0)      /**< Shifted mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_OF                        (0x1UL << 1)                     /**< OF Interrupt Enable */
#define _PCNT_IEN_OF_SHIFT                 1                                /**< Shift value for PCNT_OF */
#define _PCNT_IEN_OF_MASK                  0x2UL                            /**< Bit mask for PCNT_OF */
#define _PCNT_IEN_OF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_OF_DEFAULT                (_PCNT_IEN_OF_DEFAULT << 1)      /**< Shifted mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_DIRCNG                    (0x1UL << 2)                     /**< DIRCNG Interrupt Enable */
#define _PCNT_IEN_DIRCNG_SHIFT             2                                /**< Shift value for PCNT_DIRCNG */
#define _PCNT_IEN_DIRCNG_MASK              0x4UL                            /**< Bit mask for PCNT_DIRCNG */
#define _PCNT_IEN_DIRCNG_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_DIRCNG_DEFAULT            (_PCNT_IEN_DIRCNG_DEFAULT << 2)  /**< Shifted mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_AUXOF                     (0x1UL << 3)                     /**< AUXOF Interrupt Enable */
#define _PCNT_IEN_AUXOF_SHIFT              3                                /**< Shift value for PCNT_AUXOF */
#define _PCNT_IEN_AUXOF_MASK               0x8UL                            /**< Bit mask for PCNT_AUXOF */
#define _PCNT_IEN_AUXOF_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_AUXOF_DEFAULT             (_PCNT_IEN_AUXOF_DEFAULT << 3)   /**< Shifted mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_TCC                       (0x1UL << 4)                     /**< TCC Interrupt Enable */
#define _PCNT_IEN_TCC_SHIFT                4                                /**< Shift value for PCNT_TCC */
#define _PCNT_IEN_TCC_MASK                 0x10UL                           /**< Bit mask for PCNT_TCC */
#define _PCNT_IEN_TCC_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_TCC_DEFAULT               (_PCNT_IEN_TCC_DEFAULT << 4)     /**< Shifted mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_OQSTERR                   (0x1UL << 5)                     /**< OQSTERR Interrupt Enable */
#define _PCNT_IEN_OQSTERR_SHIFT            5                                /**< Shift value for PCNT_OQSTERR */
#define _PCNT_IEN_OQSTERR_MASK             0x20UL                           /**< Bit mask for PCNT_OQSTERR */
#define _PCNT_IEN_OQSTERR_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for PCNT_IEN */
#define PCNT_IEN_OQSTERR_DEFAULT           (_PCNT_IEN_OQSTERR_DEFAULT << 5) /**< Shifted mode DEFAULT for PCNT_IEN */

/* Bit fields for PCNT ROUTELOC0 */
#define _PCNT_ROUTELOC0_RESETVALUE         0x00000000UL                           /**< Default value for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_MASK               0x00001F1FUL                           /**< Mask for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_SHIFT      0                                      /**< Shift value for PCNT_S0INLOC */
#define _PCNT_ROUTELOC0_S0INLOC_MASK       0x1FUL                                 /**< Bit mask for PCNT_S0INLOC */
#define _PCNT_ROUTELOC0_S0INLOC_LOC0       0x00000000UL                           /**< Mode LOC0 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_DEFAULT    0x00000000UL                           /**< Mode DEFAULT for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC1       0x00000001UL                           /**< Mode LOC1 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC2       0x00000002UL                           /**< Mode LOC2 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC3       0x00000003UL                           /**< Mode LOC3 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC4       0x00000004UL                           /**< Mode LOC4 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC5       0x00000005UL                           /**< Mode LOC5 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC6       0x00000006UL                           /**< Mode LOC6 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC7       0x00000007UL                           /**< Mode LOC7 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC8       0x00000008UL                           /**< Mode LOC8 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC9       0x00000009UL                           /**< Mode LOC9 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC10      0x0000000AUL                           /**< Mode LOC10 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC11      0x0000000BUL                           /**< Mode LOC11 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC12      0x0000000CUL                           /**< Mode LOC12 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC13      0x0000000DUL                           /**< Mode LOC13 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC14      0x0000000EUL                           /**< Mode LOC14 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC15      0x0000000FUL                           /**< Mode LOC15 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC16      0x00000010UL                           /**< Mode LOC16 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC17      0x00000011UL                           /**< Mode LOC17 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC18      0x00000012UL                           /**< Mode LOC18 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC19      0x00000013UL                           /**< Mode LOC19 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC20      0x00000014UL                           /**< Mode LOC20 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC21      0x00000015UL                           /**< Mode LOC21 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC22      0x00000016UL                           /**< Mode LOC22 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC23      0x00000017UL                           /**< Mode LOC23 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC24      0x00000018UL                           /**< Mode LOC24 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC25      0x00000019UL                           /**< Mode LOC25 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC26      0x0000001AUL                           /**< Mode LOC26 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC27      0x0000001BUL                           /**< Mode LOC27 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC28      0x0000001CUL                           /**< Mode LOC28 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC29      0x0000001DUL                           /**< Mode LOC29 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC30      0x0000001EUL                           /**< Mode LOC30 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S0INLOC_LOC31      0x0000001FUL                           /**< Mode LOC31 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC0        (_PCNT_ROUTELOC0_S0INLOC_LOC0 << 0)    /**< Shifted mode LOC0 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_DEFAULT     (_PCNT_ROUTELOC0_S0INLOC_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC1        (_PCNT_ROUTELOC0_S0INLOC_LOC1 << 0)    /**< Shifted mode LOC1 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC2        (_PCNT_ROUTELOC0_S0INLOC_LOC2 << 0)    /**< Shifted mode LOC2 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC3        (_PCNT_ROUTELOC0_S0INLOC_LOC3 << 0)    /**< Shifted mode LOC3 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC4        (_PCNT_ROUTELOC0_S0INLOC_LOC4 << 0)    /**< Shifted mode LOC4 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC5        (_PCNT_ROUTELOC0_S0INLOC_LOC5 << 0)    /**< Shifted mode LOC5 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC6        (_PCNT_ROUTELOC0_S0INLOC_LOC6 << 0)    /**< Shifted mode LOC6 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC7        (_PCNT_ROUTELOC0_S0INLOC_LOC7 << 0)    /**< Shifted mode LOC7 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC8        (_PCNT_ROUTELOC0_S0INLOC_LOC8 << 0)    /**< Shifted mode LOC8 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC9        (_PCNT_ROUTELOC0_S0INLOC_LOC9 << 0)    /**< Shifted mode LOC9 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC10       (_PCNT_ROUTELOC0_S0INLOC_LOC10 << 0)   /**< Shifted mode LOC10 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC11       (_PCNT_ROUTELOC0_S0INLOC_LOC11 << 0)   /**< Shifted mode LOC11 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC12       (_PCNT_ROUTELOC0_S0INLOC_LOC12 << 0)   /**< Shifted mode LOC12 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC13       (_PCNT_ROUTELOC0_S0INLOC_LOC13 << 0)   /**< Shifted mode LOC13 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC14       (_PCNT_ROUTELOC0_S0INLOC_LOC14 << 0)   /**< Shifted mode LOC14 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC15       (_PCNT_ROUTELOC0_S0INLOC_LOC15 << 0)   /**< Shifted mode LOC15 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC16       (_PCNT_ROUTELOC0_S0INLOC_LOC16 << 0)   /**< Shifted mode LOC16 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC17       (_PCNT_ROUTELOC0_S0INLOC_LOC17 << 0)   /**< Shifted mode LOC17 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC18       (_PCNT_ROUTELOC0_S0INLOC_LOC18 << 0)   /**< Shifted mode LOC18 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC19       (_PCNT_ROUTELOC0_S0INLOC_LOC19 << 0)   /**< Shifted mode LOC19 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC20       (_PCNT_ROUTELOC0_S0INLOC_LOC20 << 0)   /**< Shifted mode LOC20 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC21       (_PCNT_ROUTELOC0_S0INLOC_LOC21 << 0)   /**< Shifted mode LOC21 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC22       (_PCNT_ROUTELOC0_S0INLOC_LOC22 << 0)   /**< Shifted mode LOC22 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC23       (_PCNT_ROUTELOC0_S0INLOC_LOC23 << 0)   /**< Shifted mode LOC23 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC24       (_PCNT_ROUTELOC0_S0INLOC_LOC24 << 0)   /**< Shifted mode LOC24 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC25       (_PCNT_ROUTELOC0_S0INLOC_LOC25 << 0)   /**< Shifted mode LOC25 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC26       (_PCNT_ROUTELOC0_S0INLOC_LOC26 << 0)   /**< Shifted mode LOC26 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC27       (_PCNT_ROUTELOC0_S0INLOC_LOC27 << 0)   /**< Shifted mode LOC27 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC28       (_PCNT_ROUTELOC0_S0INLOC_LOC28 << 0)   /**< Shifted mode LOC28 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC29       (_PCNT_ROUTELOC0_S0INLOC_LOC29 << 0)   /**< Shifted mode LOC29 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC30       (_PCNT_ROUTELOC0_S0INLOC_LOC30 << 0)   /**< Shifted mode LOC30 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S0INLOC_LOC31       (_PCNT_ROUTELOC0_S0INLOC_LOC31 << 0)   /**< Shifted mode LOC31 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_SHIFT      8                                      /**< Shift value for PCNT_S1INLOC */
#define _PCNT_ROUTELOC0_S1INLOC_MASK       0x1F00UL                               /**< Bit mask for PCNT_S1INLOC */
#define _PCNT_ROUTELOC0_S1INLOC_LOC0       0x00000000UL                           /**< Mode LOC0 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_DEFAULT    0x00000000UL                           /**< Mode DEFAULT for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC1       0x00000001UL                           /**< Mode LOC1 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC2       0x00000002UL                           /**< Mode LOC2 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC3       0x00000003UL                           /**< Mode LOC3 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC4       0x00000004UL                           /**< Mode LOC4 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC5       0x00000005UL                           /**< Mode LOC5 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC6       0x00000006UL                           /**< Mode LOC6 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC7       0x00000007UL                           /**< Mode LOC7 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC8       0x00000008UL                           /**< Mode LOC8 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC9       0x00000009UL                           /**< Mode LOC9 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC10      0x0000000AUL                           /**< Mode LOC10 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC11      0x0000000BUL                           /**< Mode LOC11 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC12      0x0000000CUL                           /**< Mode LOC12 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC13      0x0000000DUL                           /**< Mode LOC13 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC14      0x0000000EUL                           /**< Mode LOC14 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC15      0x0000000FUL                           /**< Mode LOC15 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC16      0x00000010UL                           /**< Mode LOC16 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC17      0x00000011UL                           /**< Mode LOC17 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC18      0x00000012UL                           /**< Mode LOC18 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC19      0x00000013UL                           /**< Mode LOC19 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC20      0x00000014UL                           /**< Mode LOC20 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC21      0x00000015UL                           /**< Mode LOC21 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC22      0x00000016UL                           /**< Mode LOC22 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC23      0x00000017UL                           /**< Mode LOC23 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC24      0x00000018UL                           /**< Mode LOC24 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC25      0x00000019UL                           /**< Mode LOC25 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC26      0x0000001AUL                           /**< Mode LOC26 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC27      0x0000001BUL                           /**< Mode LOC27 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC28      0x0000001CUL                           /**< Mode LOC28 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC29      0x0000001DUL                           /**< Mode LOC29 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC30      0x0000001EUL                           /**< Mode LOC30 for PCNT_ROUTELOC0 */
#define _PCNT_ROUTELOC0_S1INLOC_LOC31      0x0000001FUL                           /**< Mode LOC31 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC0        (_PCNT_ROUTELOC0_S1INLOC_LOC0 << 8)    /**< Shifted mode LOC0 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_DEFAULT     (_PCNT_ROUTELOC0_S1INLOC_DEFAULT << 8) /**< Shifted mode DEFAULT for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC1        (_PCNT_ROUTELOC0_S1INLOC_LOC1 << 8)    /**< Shifted mode LOC1 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC2        (_PCNT_ROUTELOC0_S1INLOC_LOC2 << 8)    /**< Shifted mode LOC2 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC3        (_PCNT_ROUTELOC0_S1INLOC_LOC3 << 8)    /**< Shifted mode LOC3 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC4        (_PCNT_ROUTELOC0_S1INLOC_LOC4 << 8)    /**< Shifted mode LOC4 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC5        (_PCNT_ROUTELOC0_S1INLOC_LOC5 << 8)    /**< Shifted mode LOC5 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC6        (_PCNT_ROUTELOC0_S1INLOC_LOC6 << 8)    /**< Shifted mode LOC6 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC7        (_PCNT_ROUTELOC0_S1INLOC_LOC7 << 8)    /**< Shifted mode LOC7 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC8        (_PCNT_ROUTELOC0_S1INLOC_LOC8 << 8)    /**< Shifted mode LOC8 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC9        (_PCNT_ROUTELOC0_S1INLOC_LOC9 << 8)    /**< Shifted mode LOC9 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC10       (_PCNT_ROUTELOC0_S1INLOC_LOC10 << 8)   /**< Shifted mode LOC10 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC11       (_PCNT_ROUTELOC0_S1INLOC_LOC11 << 8)   /**< Shifted mode LOC11 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC12       (_PCNT_ROUTELOC0_S1INLOC_LOC12 << 8)   /**< Shifted mode LOC12 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC13       (_PCNT_ROUTELOC0_S1INLOC_LOC13 << 8)   /**< Shifted mode LOC13 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC14       (_PCNT_ROUTELOC0_S1INLOC_LOC14 << 8)   /**< Shifted mode LOC14 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC15       (_PCNT_ROUTELOC0_S1INLOC_LOC15 << 8)   /**< Shifted mode LOC15 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC16       (_PCNT_ROUTELOC0_S1INLOC_LOC16 << 8)   /**< Shifted mode LOC16 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC17       (_PCNT_ROUTELOC0_S1INLOC_LOC17 << 8)   /**< Shifted mode LOC17 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC18       (_PCNT_ROUTELOC0_S1INLOC_LOC18 << 8)   /**< Shifted mode LOC18 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC19       (_PCNT_ROUTELOC0_S1INLOC_LOC19 << 8)   /**< Shifted mode LOC19 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC20       (_PCNT_ROUTELOC0_S1INLOC_LOC20 << 8)   /**< Shifted mode LOC20 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC21       (_PCNT_ROUTELOC0_S1INLOC_LOC21 << 8)   /**< Shifted mode LOC21 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC22       (_PCNT_ROUTELOC0_S1INLOC_LOC22 << 8)   /**< Shifted mode LOC22 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC23       (_PCNT_ROUTELOC0_S1INLOC_LOC23 << 8)   /**< Shifted mode LOC23 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC24       (_PCNT_ROUTELOC0_S1INLOC_LOC24 << 8)   /**< Shifted mode LOC24 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC25       (_PCNT_ROUTELOC0_S1INLOC_LOC25 << 8)   /**< Shifted mode LOC25 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC26       (_PCNT_ROUTELOC0_S1INLOC_LOC26 << 8)   /**< Shifted mode LOC26 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC27       (_PCNT_ROUTELOC0_S1INLOC_LOC27 << 8)   /**< Shifted mode LOC27 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC28       (_PCNT_ROUTELOC0_S1INLOC_LOC28 << 8)   /**< Shifted mode LOC28 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC29       (_PCNT_ROUTELOC0_S1INLOC_LOC29 << 8)   /**< Shifted mode LOC29 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC30       (_PCNT_ROUTELOC0_S1INLOC_LOC30 << 8)   /**< Shifted mode LOC30 for PCNT_ROUTELOC0 */
#define PCNT_ROUTELOC0_S1INLOC_LOC31       (_PCNT_ROUTELOC0_S1INLOC_LOC31 << 8)   /**< Shifted mode LOC31 for PCNT_ROUTELOC0 */

/* Bit fields for PCNT FREEZE */
#define _PCNT_FREEZE_RESETVALUE            0x00000000UL                          /**< Default value for PCNT_FREEZE */
#define _PCNT_FREEZE_MASK                  0x00000001UL                          /**< Mask for PCNT_FREEZE */
#define PCNT_FREEZE_REGFREEZE              (0x1UL << 0)                          /**< Register Update Freeze */
#define _PCNT_FREEZE_REGFREEZE_SHIFT       0                                     /**< Shift value for PCNT_REGFREEZE */
#define _PCNT_FREEZE_REGFREEZE_MASK        0x1UL                                 /**< Bit mask for PCNT_REGFREEZE */
#define _PCNT_FREEZE_REGFREEZE_DEFAULT     0x00000000UL                          /**< Mode DEFAULT for PCNT_FREEZE */
#define _PCNT_FREEZE_REGFREEZE_UPDATE      0x00000000UL                          /**< Mode UPDATE for PCNT_FREEZE */
#define _PCNT_FREEZE_REGFREEZE_FREEZE      0x00000001UL                          /**< Mode FREEZE for PCNT_FREEZE */
#define PCNT_FREEZE_REGFREEZE_DEFAULT      (_PCNT_FREEZE_REGFREEZE_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_FREEZE */
#define PCNT_FREEZE_REGFREEZE_UPDATE       (_PCNT_FREEZE_REGFREEZE_UPDATE << 0)  /**< Shifted mode UPDATE for PCNT_FREEZE */
#define PCNT_FREEZE_REGFREEZE_FREEZE       (_PCNT_FREEZE_REGFREEZE_FREEZE << 0)  /**< Shifted mode FREEZE for PCNT_FREEZE */

/* Bit fields for PCNT SYNCBUSY */
#define _PCNT_SYNCBUSY_RESETVALUE          0x00000000UL                         /**< Default value for PCNT_SYNCBUSY */
#define _PCNT_SYNCBUSY_MASK                0x0000000FUL                         /**< Mask for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_CTRL                 (0x1UL << 0)                         /**< CTRL Register Busy */
#define _PCNT_SYNCBUSY_CTRL_SHIFT          0                                    /**< Shift value for PCNT_CTRL */
#define _PCNT_SYNCBUSY_CTRL_MASK           0x1UL                                /**< Bit mask for PCNT_CTRL */
#define _PCNT_SYNCBUSY_CTRL_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_CTRL_DEFAULT         (_PCNT_SYNCBUSY_CTRL_DEFAULT << 0)   /**< Shifted mode DEFAULT for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_CMD                  (0x1UL << 1)                         /**< CMD Register Busy */
#define _PCNT_SYNCBUSY_CMD_SHIFT           1                                    /**< Shift value for PCNT_CMD */
#define _PCNT_SYNCBUSY_CMD_MASK            0x2UL                                /**< Bit mask for PCNT_CMD */
#define _PCNT_SYNCBUSY_CMD_DEFAULT         0x00000000UL                         /**< Mode DEFAULT for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_CMD_DEFAULT          (_PCNT_SYNCBUSY_CMD_DEFAULT << 1)    /**< Shifted mode DEFAULT for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_TOPB                 (0x1UL << 2)                         /**< TOPB Register Busy */
#define _PCNT_SYNCBUSY_TOPB_SHIFT          2                                    /**< Shift value for PCNT_TOPB */
#define _PCNT_SYNCBUSY_TOPB_MASK           0x4UL                                /**< Bit mask for PCNT_TOPB */
#define _PCNT_SYNCBUSY_TOPB_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_TOPB_DEFAULT         (_PCNT_SYNCBUSY_TOPB_DEFAULT << 2)   /**< Shifted mode DEFAULT for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_OVSCFG               (0x1UL << 3)                         /**< OVSCFG Register Busy */
#define _PCNT_SYNCBUSY_OVSCFG_SHIFT        3                                    /**< Shift value for PCNT_OVSCFG */
#define _PCNT_SYNCBUSY_OVSCFG_MASK         0x8UL                                /**< Bit mask for PCNT_OVSCFG */
#define _PCNT_SYNCBUSY_OVSCFG_DEFAULT      0x00000000UL                         /**< Mode DEFAULT for PCNT_SYNCBUSY */
#define PCNT_SYNCBUSY_OVSCFG_DEFAULT       (_PCNT_SYNCBUSY_OVSCFG_DEFAULT << 3) /**< Shifted mode DEFAULT for PCNT_SYNCBUSY */

/* Bit fields for PCNT AUXCNT */
#define _PCNT_AUXCNT_RESETVALUE            0x00000000UL                       /**< Default value for PCNT_AUXCNT */
#define _PCNT_AUXCNT_MASK                  0x0000FFFFUL                       /**< Mask for PCNT_AUXCNT */
#define _PCNT_AUXCNT_AUXCNT_SHIFT          0                                  /**< Shift value for PCNT_AUXCNT */
#define _PCNT_AUXCNT_AUXCNT_MASK           0xFFFFUL                           /**< Bit mask for PCNT_AUXCNT */
#define _PCNT_AUXCNT_AUXCNT_DEFAULT        0x00000000UL                       /**< Mode DEFAULT for PCNT_AUXCNT */
#define PCNT_AUXCNT_AUXCNT_DEFAULT         (_PCNT_AUXCNT_AUXCNT_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_AUXCNT */

/* Bit fields for PCNT INPUT */
#define _PCNT_INPUT_RESETVALUE             0x00000000UL                        /**< Default value for PCNT_INPUT */
#define _PCNT_INPUT_MASK                   0x00000BEFUL                        /**< Mask for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_SHIFT         0                                   /**< Shift value for PCNT_S0PRSSEL */
#define _PCNT_INPUT_S0PRSSEL_MASK          0xFUL                               /**< Bit mask for PCNT_S0PRSSEL */
#define _PCNT_INPUT_S0PRSSEL_DEFAULT       0x00000000UL                        /**< Mode DEFAULT for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH0        0x00000000UL                        /**< Mode PRSCH0 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH1        0x00000001UL                        /**< Mode PRSCH1 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH2        0x00000002UL                        /**< Mode PRSCH2 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH3        0x00000003UL                        /**< Mode PRSCH3 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH4        0x00000004UL                        /**< Mode PRSCH4 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH5        0x00000005UL                        /**< Mode PRSCH5 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH6        0x00000006UL                        /**< Mode PRSCH6 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH7        0x00000007UL                        /**< Mode PRSCH7 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH8        0x00000008UL                        /**< Mode PRSCH8 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH9        0x00000009UL                        /**< Mode PRSCH9 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH10       0x0000000AUL                        /**< Mode PRSCH10 for PCNT_INPUT */
#define _PCNT_INPUT_S0PRSSEL_PRSCH11       0x0000000BUL                        /**< Mode PRSCH11 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_DEFAULT        (_PCNT_INPUT_S0PRSSEL_DEFAULT << 0) /**< Shifted mode DEFAULT for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH0         (_PCNT_INPUT_S0PRSSEL_PRSCH0 << 0)  /**< Shifted mode PRSCH0 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH1         (_PCNT_INPUT_S0PRSSEL_PRSCH1 << 0)  /**< Shifted mode PRSCH1 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH2         (_PCNT_INPUT_S0PRSSEL_PRSCH2 << 0)  /**< Shifted mode PRSCH2 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH3         (_PCNT_INPUT_S0PRSSEL_PRSCH3 << 0)  /**< Shifted mode PRSCH3 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH4         (_PCNT_INPUT_S0PRSSEL_PRSCH4 << 0)  /**< Shifted mode PRSCH4 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH5         (_PCNT_INPUT_S0PRSSEL_PRSCH5 << 0)  /**< Shifted mode PRSCH5 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH6         (_PCNT_INPUT_S0PRSSEL_PRSCH6 << 0)  /**< Shifted mode PRSCH6 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH7         (_PCNT_INPUT_S0PRSSEL_PRSCH7 << 0)  /**< Shifted mode PRSCH7 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH8         (_PCNT_INPUT_S0PRSSEL_PRSCH8 << 0)  /**< Shifted mode PRSCH8 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH9         (_PCNT_INPUT_S0PRSSEL_PRSCH9 << 0)  /**< Shifted mode PRSCH9 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH10        (_PCNT_INPUT_S0PRSSEL_PRSCH10 << 0) /**< Shifted mode PRSCH10 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSSEL_PRSCH11        (_PCNT_INPUT_S0PRSSEL_PRSCH11 << 0) /**< Shifted mode PRSCH11 for PCNT_INPUT */
#define PCNT_INPUT_S0PRSEN                 (0x1UL << 5)                        /**< S0IN PRS Enable */
#define _PCNT_INPUT_S0PRSEN_SHIFT          5                                   /**< Shift value for PCNT_S0PRSEN */
#define _PCNT_INPUT_S0PRSEN_MASK           0x20UL                              /**< Bit mask for PCNT_S0PRSEN */
#define _PCNT_INPUT_S0PRSEN_DEFAULT        0x00000000UL                        /**< Mode DEFAULT for PCNT_INPUT */
#define PCNT_INPUT_S0PRSEN_DEFAULT         (_PCNT_INPUT_S0PRSEN_DEFAULT << 5)  /**< Shifted mode DEFAULT for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_SHIFT         6                                   /**< Shift value for PCNT_S1PRSSEL */
#define _PCNT_INPUT_S1PRSSEL_MASK          0x3C0UL                             /**< Bit mask for PCNT_S1PRSSEL */
#define _PCNT_INPUT_S1PRSSEL_DEFAULT       0x00000000UL                        /**< Mode DEFAULT for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH0        0x00000000UL                        /**< Mode PRSCH0 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH1        0x00000001UL                        /**< Mode PRSCH1 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH2        0x00000002UL                        /**< Mode PRSCH2 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH3        0x00000003UL                        /**< Mode PRSCH3 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH4        0x00000004UL                        /**< Mode PRSCH4 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH5        0x00000005UL                        /**< Mode PRSCH5 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH6        0x00000006UL                        /**< Mode PRSCH6 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH7        0x00000007UL                        /**< Mode PRSCH7 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH8        0x00000008UL                        /**< Mode PRSCH8 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH9        0x00000009UL                        /**< Mode PRSCH9 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH10       0x0000000AUL                        /**< Mode PRSCH10 for PCNT_INPUT */
#define _PCNT_INPUT_S1PRSSEL_PRSCH11       0x0000000BUL                        /**< Mode PRSCH11 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_DEFAULT        (_PCNT_INPUT_S1PRSSEL_DEFAULT << 6) /**< Shifted mode DEFAULT for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH0         (_PCNT_INPUT_S1PRSSEL_PRSCH0 << 6)  /**< Shifted mode PRSCH0 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH1         (_PCNT_INPUT_S1PRSSEL_PRSCH1 << 6)  /**< Shifted mode PRSCH1 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH2         (_PCNT_INPUT_S1PRSSEL_PRSCH2 << 6)  /**< Shifted mode PRSCH2 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH3         (_PCNT_INPUT_S1PRSSEL_PRSCH3 << 6)  /**< Shifted mode PRSCH3 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH4         (_PCNT_INPUT_S1PRSSEL_PRSCH4 << 6)  /**< Shifted mode PRSCH4 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH5         (_PCNT_INPUT_S1PRSSEL_PRSCH5 << 6)  /**< Shifted mode PRSCH5 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH6         (_PCNT_INPUT_S1PRSSEL_PRSCH6 << 6)  /**< Shifted mode PRSCH6 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH7         (_PCNT_INPUT_S1PRSSEL_PRSCH7 << 6)  /**< Shifted mode PRSCH7 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH8         (_PCNT_INPUT_S1PRSSEL_PRSCH8 << 6)  /**< Shifted mode PRSCH8 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH9         (_PCNT_INPUT_S1PRSSEL_PRSCH9 << 6)  /**< Shifted mode PRSCH9 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH10        (_PCNT_INPUT_S1PRSSEL_PRSCH10 << 6) /**< Shifted mode PRSCH10 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSSEL_PRSCH11        (_PCNT_INPUT_S1PRSSEL_PRSCH11 << 6) /**< Shifted mode PRSCH11 for PCNT_INPUT */
#define PCNT_INPUT_S1PRSEN                 (0x1UL << 11)                       /**< S1IN PRS Enable */
#define _PCNT_INPUT_S1PRSEN_SHIFT          11                                  /**< Shift value for PCNT_S1PRSEN */
#define _PCNT_INPUT_S1PRSEN_MASK           0x800UL                             /**< Bit mask for PCNT_S1PRSEN */
#define _PCNT_INPUT_S1PRSEN_DEFAULT        0x00000000UL                        /**< Mode DEFAULT for PCNT_INPUT */
#define PCNT_INPUT_S1PRSEN_DEFAULT         (_PCNT_INPUT_S1PRSEN_DEFAULT << 11) /**< Shifted mode DEFAULT for PCNT_INPUT */

/* Bit fields for PCNT OVSCFG */
#define _PCNT_OVSCFG_RESETVALUE            0x00000000UL                           /**< Default value for PCNT_OVSCFG */
#define _PCNT_OVSCFG_MASK                  0x000010FFUL                           /**< Mask for PCNT_OVSCFG */
#define _PCNT_OVSCFG_FILTLEN_SHIFT         0                                      /**< Shift value for PCNT_FILTLEN */
#define _PCNT_OVSCFG_FILTLEN_MASK          0xFFUL                                 /**< Bit mask for PCNT_FILTLEN */
#define _PCNT_OVSCFG_FILTLEN_DEFAULT       0x00000000UL                           /**< Mode DEFAULT for PCNT_OVSCFG */
#define PCNT_OVSCFG_FILTLEN_DEFAULT        (_PCNT_OVSCFG_FILTLEN_DEFAULT << 0)    /**< Shifted mode DEFAULT for PCNT_OVSCFG */
#define PCNT_OVSCFG_FLUTTERRM              (0x1UL << 12)                          /**< Flutter Remove */
#define _PCNT_OVSCFG_FLUTTERRM_SHIFT       12                                     /**< Shift value for PCNT_FLUTTERRM */
#define _PCNT_OVSCFG_FLUTTERRM_MASK        0x1000UL                               /**< Bit mask for PCNT_FLUTTERRM */
#define _PCNT_OVSCFG_FLUTTERRM_DEFAULT     0x00000000UL                           /**< Mode DEFAULT for PCNT_OVSCFG */
#define PCNT_OVSCFG_FLUTTERRM_DEFAULT      (_PCNT_OVSCFG_FLUTTERRM_DEFAULT << 12) /**< Shifted mode DEFAULT for PCNT_OVSCFG */

/** @} End of group EFR32FG1P_PCNT */
/** @} End of group Parts */

